OBJ_DIR := Obj
SRC_DIR := src
LIB_DIR := lib
INCLUDE_DIR := include
SRC_FILES := $(wildcard $(SRC_DIR)/*.c)
# OBJ_FILES := $(addprefix $(OBJ_DIR)/,$(patsubst %.c,%.o,$(SRC_FILES)))
OBJ_FILES := $(SRC_FILES:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)
CFLAGS := -I$(INCLUDE_DIR)/ -I. -ggdb
LDFLAGS := -static -s -w -L$(LIB_DIR) -L. -Ldeps/raylib -l:libraylib.a -Ldeps/zip/build -l:libzip.a -lglfw3 -lopengl32 -lgdi32 -lwinmm -Wl,-allow-multiple-definition -Wl,--subsystem,windows
OUTEXE := Build/oneButtonRhythm.exe

.DEFAULT_GOAL := $(OUTEXE)

.depend: $(SRC_FILES)
	rm -f .depend
	x86_64-w64-mingw32-gcc $(CFLAGS) -MM $^ | perl -p -e 's,^(.+?)\.o: (.+?)/\1\.c.*,$$2/$$&,;' >> .depend

include .depend

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	x86_64-w64-mingw32-gcc -c $(CFLAGS) $< -o $@

$(OUTEXE): $(OBJ_FILES) .depend
	x86_64-w64-mingw32-gcc -o Build/oneButton.exe Obj/* -static -s -w -Llib -L. -Ldeps/raylib -l:libraylib.a -Ldeps/zip/build -l:libzip.a  -lopengl32 -lgdi32 -lwinmm -Wl,-allow-multiple-definition -Wl,--subsystem,windows -lssp
#x86_64-w64-mingw32-gcc -o $(OUTEXE) $(OBJ_FILES) $(LDFLAGS) $(CFLAGS)

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
