OBJ_DIR := Obj
SRC_DIR := src
LIB_DIR := lib
INCLUDE_DIR := include
SRC_FILES := $(wildcard $(SRC_DIR)/*.c)
# OBJ_FILES := $(addprefix $(OBJ_DIR)/,$(patsubst %.c,%.o,$(SRC_FILES)))
OBJ_FILES := $(SRC_FILES:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)
CFLAGS := -I$(INCLUDE_DIR)/ -I. -ggdb
LDFLAGS := -Ldeps/raylib -l:libraylib.a -lGL -lm -lpthread -ldl -lrt -lX11 -Ldeps/zip/build -l:libzip.a
OUTEXE := Build/oneButtonRhythm

.DEFAULT_GOAL := $(OUTEXE)

.depend: $(SRC_FILES)
	rm -f .depend
	gcc $(CFLAGS) -MM $^ | perl -p -e 's,^(.+?)\.o: (.+?)/\1\.c.*,$$2/$$&,;' >> .depend

include .depend

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	gcc -c $(CFLAGS) $< -o $@

$(OUTEXE): $(OBJ_FILES) .depend
	gcc -o $(OUTEXE) $(OBJ_FILES) $(LDFLAGS) $(CFLAGS)

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
