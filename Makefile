OBJ_DIR := Obj
SRC_DIR := src
INCLUDE_DIR := include
SRC_FILES := $(wildcard $(SRC_DIR)/*.c)
# OBJ_FILES := $(addprefix $(OBJ_DIR)/,$(patsubst %.c,%.o,$(SRC_FILES)))
OBJ_FILES := $(SRC_FILES:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)
CFLAGS := -I$(INCLUDE_DIR)/ -I. -ggdb
LDFLAGS := -lraylib -lGL -lm -lpthread -ldl -lrt -lX11
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