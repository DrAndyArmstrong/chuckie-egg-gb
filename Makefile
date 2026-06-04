GBDK_HOME ?= $(CURDIR)/sdk/gbdk
LCC        = $(GBDK_HOME)/bin/lcc

HELLO_SRC   = src/hello/hello.c
HELLO_ROM   = roms/hello.gb

CHUCKIE_SRC = src/chuckie/main.c
CHUCKIE_ROM = roms/chuckie.gb
LEVEL_DATA  = src/chuckie/level_data.h
TILESET_H   = src/chuckie/tileset.h
HARRY_H     = src/chuckie/harry.h
BIRD_H      = src/chuckie/bird_tiles.h

.PHONY: all clean

all: $(HELLO_ROM) $(CHUCKIE_ROM)

roms:
	mkdir -p roms

$(HELLO_ROM): $(HELLO_SRC) | roms
	$(LCC) -o $@ $<

$(LEVEL_DATA): scenes.basm tools/convert_levels.py
	python3 tools/convert_levels.py > $@

$(CHUCKIE_ROM): $(CHUCKIE_SRC) $(LEVEL_DATA) $(TILESET_H) $(HARRY_H) $(BIRD_H) | roms
	$(LCC) -Wm-yc -Wm-yn"CHUCKIE EGG" -o $@ $(CHUCKIE_SRC)

clean:
	rm -f roms/*.gb
	find src -name "*.o" -o -name "*.lst" -o -name "*.map" -o -name "*.noi" -o -name "*.sym" | xargs rm -f
