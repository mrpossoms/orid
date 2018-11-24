CC=gcc
CFLAGS=-g
INC=-Icfg.h/src
LINK=

cfg.h:
	git clone http://github.com/mrpossoms/cfg.h

bin:
	mkdir bin

%: src/%.c cfg.h bin
	$(CC) $(CFLAGS) $(INC) $< -o bin/$@ $(LINK)

.PHONY: install
install: orid
	cp bin/* /usr/bin
