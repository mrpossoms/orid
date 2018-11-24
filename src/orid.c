#include <stdio.h>
#include <termios.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <limits.h>

#include "cfg.h"

typedef enum {
	X_POS = 0,
	Y_POS,
	Z_POS,
	X_NEG,
	Y_NEG,
	Z_NEG,
	AXIS_COUNT
} axis_t;

const char* AXIS_NAMES[] = {
	"x+",
	"y+",
	"z+",
	"x-",
	"y-",
	"z-",
};

typedef struct {
	int number;
	struct {
		char   output[16];
		struct { int w, h; } res;
		struct { int x, y; } pos;
		char*  rotations[AXIS_COUNT];
		char   serial_dev[PATH_MAX];
	} cfg;

	axis_t major_axis;
	int sensor_fd;
	struct termios old_term;
} screen_t;


void apply_settings(screen_t* scr)
{
	const char* fmt = "xrandr --output %s --auto --mode %dx%d --pos %dx%d --rotate %s --screen %d";
	char cmd_str[512];
	char* rot = "normal";

	if (scr->cfg.rotations[scr->major_axis])
	{
		rot = scr->cfg.rotations[scr->major_axis];
	}

	int w = scr->cfg.res.w, h = scr->cfg.res.h;
	int x = scr->cfg.pos.x, y = scr->cfg.pos.y;

	sprintf(cmd_str, fmt, scr->cfg.output, w, h, x, y, rot, scr->number);
	system(cmd_str);
}


int cfg_term(screen_t* scr)
{
	int res = 0;
	if ((res = tcgetattr(scr->sensor_fd, &scr->old_term))) { return res; }

	struct termios term = scr->old_term;
	
	if ((res = cfsetspeed(&term, 115200)))                 { return res; }
	if ((res = tcsetattr(scr->sensor_fd, TCSANOW, &term))) { return res; }

	return 0;
}

int get_set_screen_cfg(screen_t* scr, char* base_path)
{
	char buf[PATH_MAX];
	sprintf(buf, "%s/%d", base_path, scr->number);
	cfg_base(buf);

	strcpy(scr->cfg.output, cfg_str("output", scr->cfg.output));
	
	scr->cfg.res.w = cfg_int("w", scr->cfg.res.w);
	scr->cfg.res.h = cfg_int("h", scr->cfg.res.h);
	scr->cfg.pos.x = cfg_int("x", scr->cfg.pos.x);
	scr->cfg.pos.y = cfg_int("y", scr->cfg.pos.y);

	
	sprintf(buf, "%s/%d/rotations", base_path, scr->number);
	cfg_base(buf);

	for (int i=AXIS_COUNT; i--;)
	{
		char* rot = scr->cfg.rotations[i] ? scr->cfg.rotations[i] : "normal";
		char* val = cfg_str(AXIS_NAMES[i], rot);
		scr->cfg.rotations[i] = (char*)malloc(strlen(val));
		strcpy(scr->cfg.rotations[i], val); 
	}

	sprintf(buf, "%s/%d", base_path, scr->number);
	cfg_base(buf);
	strcpy(scr->cfg.serial_dev, cfg_str("serial-dev", "/dev/null"));
	scr->cfg.serial_dev[strlen(scr->cfg.serial_dev) - 1] = '\0';

	return 0;
}

int init_cfgs(screen_t** screens, int* screen_count)
{
	char cfg_path[PATH_MAX];
	sprintf(cfg_path, "%s/.orid", getenv("HOME"));

	cfg_base(cfg_path);	


	FILE* proc = popen("xrandr --listmonitors", "r");

	fscanf(proc, "Monitors: %d\n", screen_count);
	*screens = (screen_t*)calloc(sizeof(screen_t), *screen_count);

	for (int i = 0; i < *screen_count; ++i)
	{
		screen_t s;
		int number;
		char output[16];
		int pix_w, pix_h;
		int real_w, real_h;
		int x, y;
		fscanf(
			proc,
			"%d: +*%s %d/%dx%d/%d+%d+%d  %s\n", 
			&s.number,
			s.cfg.output,
			&s.cfg.res.w, &real_w,
			&s.cfg.res.h, &real_h,
			&s.cfg.pos.x, &s.cfg.pos.y,
			s.cfg.output
		);
			
		get_set_screen_cfg(&s, cfg_path);
		(*screens)[i] = s;			
	}

	pclose(proc);

	return 0;
} 

int main (int argc, char* argv[])
{
	screen_t* screens = NULL;
	int screen_count = 0;

	init_cfgs(&screens, &screen_count);

	for (int i = screen_count; i--;)
	{
		screens[i].sensor_fd = open(screens[i].cfg.serial_dev, O_RDONLY);
		if (cfg_term(screens + i)) { return 1; }
	}


	char buf[256] = {};
	while(read(screens[0].sensor_fd, buf, sizeof(buf)))
	{
		if (strncmp("ori:", buf, 4) == 0)
		{
			char* axis_start = buf;
			while (axis_start[0] != ':') axis_start++;
			axis_start++;

			for(int i = AXIS_COUNT; i--;)
			{
				if (!strncmp(axis_start, AXIS_NAMES[i], strlen(AXIS_NAMES[i])))
				{
					screens[0].major_axis = (axis_t)i;
				}
			}

			apply_settings(screens);
		}

		bzero(buf, sizeof(buf));
	}

	return 0;
}
