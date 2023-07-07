STATIC = 0
OUT_NAME = Chimp8
LSHLWAPI = 
ifeq ($(OS),Windows_NT)
	OUT_NAME = Chimp8.exe
	LSHLWAPI = -lshlwapi
endif
chimp8:
ifeq ($(STATIC),1)
	g++ src/*.cpp `sdl2-config --cflags --static-libs` `pkg-config SDL2_mixer --static --cflags --libs` $(LSHLWAPI) -static -o $(OUT_NAME)
else
	g++ src/*.cpp `sdl2-config --cflags --libs` `pkg-config SDL2_mixer --cflags --libs` $(LSHLWAPI) -o $(OUT_NAME)
endif
strip:
	strip --strip-all $(OUT_NAME)
clean:
	rm $(OUT_NAME)
