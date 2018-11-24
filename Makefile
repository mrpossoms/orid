CC=gcc
CFLAGS=-g
INC=-Icfg.h/src
LINK=

cfg.h:
	git clone http://github.com/mrpossoms/cfg.h

%: src/%.c cfg.h
	$(CC) $(CFLAGS) $(INC) $< -o $@ $(LINK)
