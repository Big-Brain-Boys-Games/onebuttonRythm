OBJ_DIR := Obj
SRC_DIR := src
LIB_DIR := lib
INCLUDE_DIR := include
SRC_FILES := $(wildcard $(SRC_DIR)/*.c)
SRCPP_FILES := $(wildcard $(SRC_DIR)/*.cpp)



# OBJ_FILES := $(addprefix $(OBJ_DIR)/,$(patsubst %.c,%.o,$(SRC_FILES)))
OBJ_FILES := $(SRC_FILES:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)
CFLAGS := -I$(INCLUDE_DIR)/ -I. -ggdb
LDFLAGS := -Ldeps/raylib -l:libraylib.a -lGL -lm -lpthread -ldl -lrt -lX11 -Ldeps/zip/build -l:libzip.a
OUTEXE := Build/oneButtonRhythm
GAMEJOLT_FILES := $(wildcard deps/game-jolt-api-cpp-library/source/*cpp)
GAMEJOLT_LIBRARY := -L deps/game-jolt-api-cpp-library/libraries/bin/linux/x64/ -l:libcurl.so.4
ifeq ($(OS),Windows_NT)
	LDFLAGS = -static -s -w -Llib -L. -Ldeps/raylib -l:libraylib.a -Ldeps/zip/build -l:libzip.a  -lopengl32 -lgdi32 -lwinmm -Wl,-allow-multiple-definition -Wl,--subsystem,windows -lssp
	OUTEXE = Build/oneButtonRhythm
endif
.DEFAULT_GOAL := $(OUTEXE)
#windows build on linux
#x86_64-w64-mingw32-gcc Obj/* -o Build/oneButton.exe -static -s -w -Llib -L. -Ldeps/raylib -l:libraylib.a -Ldeps/zip/build -l:libzip.a  -lopengl32 -lgdi32 -lwinmm -Wl,-allow-multiple-definition -Wl,--subsystem,windows -lssp

.depend: $(SRC_FILES)
	rm -f .depend
	$(CC) $(CFLAGS) -MM $^ | perl -p -e 's,^(.+?)\.o: (.+?)/\1\.c.*,$$2/$$&,;' >> .depend

include .depend

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) -c $(CFLAGS) $< -o $@

$(OUTEXE): $(OBJ_FILES) .depend
	$(CC) $(SRC_DIR)/*cpp $(OBJ_FILES) -o $(OUTEXE) $(LDFLAGS) $(CFLAGS) 

.PHONY: gamejolt

gamejolt:
	$(MAKE) CFLAGS=-DGAMEJOLT ; $(CXX) -DGAMEJOLT src/gamejolt.cpp -c -o Obj/gamejolt.o ; $(CXX) deps/game-jolt-api-cpp-library/source/*cpp -lcurl -DGAMEJOLT Obj/* -o $(OUTEXE) $(LDFLAGS)
	
	



.PHONY: clean
clean:
	rm $(OBJ_FILES) .depend $(OUTEXE)

.PHONY: deps
deps:
	@if [ ! -d "deps/zip/build" ]; then \
	echo "Directory doesn't exist, making..."; \
	mkdir deps/zip/build; \
	fi
	cd deps/zip/build && cmake -DBUILD_SHARED_LIBS=false .. && cmake --build .
	cd deps/raylib/src && make PLATFORM=PLATFORM_DESKTOP CFLAGS=-DSUPPORT_FILEFORMAT_JPG
