#include <stdio.h>
#include <termios.h>
#include <fcntl.h>
#include <stdlib.h>

typedef struct {
	struct {
		char output[16];
		char mode[16];
		char pos[16];
		char rotate[16];
	} cfg;

	int sensor_fd;
} screen_t;


void apply_settings(screen_t* scr)
{
	const char* fmt = "xrandr --output %s --auto --mode %s --pos %s --rotate %s --primary";
	char cmd_str[512];
	sprintf(cmd_str, fmt, scr->cfg.output, scr->cfg.mode, scr->cfg.pos, scr->cfg.rotate);
	system(cmd_str);
}


int main (int argc, char* argv[])
{
	screen_t screen = {
		.cfg = {
			"DVI-D-0",
			"1920x1080",
			"0x0",
			"right"
		},
		.sensor_fd = open("/dev/ttyAMA0", O_RDONLY)
	};

	apply_settings(&screen);

	return 0;
}
