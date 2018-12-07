#include <stdio.h>
#include <termios.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <limits.h>
#include <dirent.h>

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
		char   rotations[AXIS_COUNT][32];
		char   serial_dev[PATH_MAX];
	} cfg;

	axis_t major_axis;
	int sensor_fd;
	struct termios old_term;
} screen_t;


void apply_settings(screen_t* scr)
{
	const char* fmt = "xrandr --output %s --auto --mode %dx%d --pos %dx%d --screen %d --rotate %s";
	char cmd_str[512];
	char* rot = "normal";

	if (scr->cfg.rotations[scr->major_axis])
	{
		rot = scr->cfg.rotations[scr->major_axis];
	}

	int w = scr->cfg.res.w, h = scr->cfg.res.h;
	int x = scr->cfg.pos.x, y = scr->cfg.pos.y;

	sprintf(cmd_str, fmt, scr->cfg.output, w, h, x, y, scr->number, rot);
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


char* get_next_gemma()
{
	static DIR* d;
	static struct dirent* dir;
	static char buf[PATH_MAX + NAME_MAX];

	if (!d) { d = opendir("/dev/serial/by-id"); }

	if (d)
	{
		const char* stem = "usb-Adafruit_Gemma_M0_";
		int stem_len = strlen(stem);

		if ((dir = readdir(d)) != NULL)
		{
			for (int i = 0; i < NAME_MAX - stem_len; ++i)
			if (!strncmp(dir->d_name + i, stem, strlen(stem)))
			{
				sprintf(buf, "/dev/serial/by-id/%s", dir->d_name + i);
				return buf;
			}
		}
		else
		{
			closedir(d);
			d = NULL;
		}
	}

	return NULL;
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
		char* rot = scr->cfg.rotations[i][0] ? scr->cfg.rotations[i] : "normal";
		char* val = cfg_str(AXIS_NAMES[i], rot);
		strncpy(scr->cfg.rotations[i], val, sizeof(scr->cfg.rotations[0])); 
	}

	sprintf(buf, "%s/%d", base_path, scr->number);
	cfg_base(buf);

	char* dev = get_next_gemma();
	strcpy(scr->cfg.serial_dev, cfg_str("serial-dev", dev ? dev : "/dev/null"));

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


int uninit_screens(screen_t** screens, int screen_count)
{
	for (int i = screen_count; i--;)
	{
		close((*screens)[i].sensor_fd);
	}

	free(*screens);
	*screens = NULL;
	
	return 0;
}


int init_screens(screen_t** screens, int* screen_count)
{

	init_cfgs(screens, screen_count);

	for (int i = *screen_count; i--;)
	{
		(*screens)[i].sensor_fd = open((*screens)[i].cfg.serial_dev, O_RDONLY);
		if (cfg_term(*screens + i))
		{
			uninit_screens(screens, *screen_count);
			return 1;
		}
	}

	return 0;
}



int main (int argc, char* argv[])
{
	screen_t* screens = NULL;
	int screen_count = 0;
	int res = 0;

	while(1)
	{
		fd_set sensor_fds;
		int max_fd = 0;

		// initialize the screen sensors, load configs
		if (screens == NULL)
		{
			if (init_screens(&screens, &screen_count))
			{
				// something is clearly wrong, pause for a second
				// to not burn cpu cycles
				sleep(1);
				continue;
			}
		}

		// select on all the sensor serials
		FD_ZERO(&sensor_fds);
		for (int i = screen_count; i--;)
		{
			if (screens[i].sensor_fd > max_fd) { max_fd = screens[i].sensor_fd; }
			FD_SET(screens[i].sensor_fd, &sensor_fds);
		}

		switch (select(max_fd + 1, &sensor_fds, NULL, NULL, NULL))
		{
		// A timeout cant occur, so if case 0, or 1 happen something bad occured
		// uninitalize the screen sensors, to try opening connections again
		case 0:
		case -1:
			uninit_screens(&screens, screen_count);	
			break;
		default:
			for (int i = screen_count; i--;)
			if (FD_ISSET(screens[i].sensor_fd, &sensor_fds))
			{
				char buf[256] = {};

				if (read(screens[i].sensor_fd, buf, sizeof(buf)) <= 0)
				{
					// an error has occurred, lets reset and try to straigten it out
					uninit_screens(&screens, screen_count);
					continue;
				}
				
				// orientation change message all begin with 'ori:' then
				// indicate the axis experiencing gravity the most.
				if (strncmp("ori:", buf, 4) == 0)
				{
					char* axis_start = buf;
					while (axis_start[0] != ':') axis_start++;
					axis_start++;

					// find which axis is the one reported
					for(int i = AXIS_COUNT; i--;)
					{
						if (!strncmp(axis_start, AXIS_NAMES[i], strlen(AXIS_NAMES[i])))
						{
							screens[0].major_axis = (axis_t)i;
						}
					}

					// apply orientation changes if needed
					apply_settings(screens);
				}

			}
		}
	}

	return 0;
}
