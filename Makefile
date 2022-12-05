###########################################################################
#
#   Filename:           Makefile
#
#   Author:             Marcelo Mourier
#   Created:            Mon Dec  5 11:26:52 MST 2022
#
#   Description:        This makefile is used to build the whatsOnFulGaz
#                       command-line tool.
#
###########################################################################
#
#                  Copyright (c) 2022 Marcelo Mourier
#
###########################################################################

BIN_DIR = .
DEP_DIR = .
OBJ_DIR = .

CFLAGS = -m64 -D_GNU_SOURCE -I. -I./fit -ggdb -Wall -Werror -O0
LDFLAGS = -ggdb 

SOURCES = $(wildcard *.c)
OBJECTS := $(patsubst %.c,$(OBJ_DIR)/%.o,$(SOURCES))
DEPS := $(patsubst %.c,$(DEP_DIR)/%.d,$(SOURCES))

# Rule to autogenerate dependencies files
$(DEP_DIR)/%.d: %.c
	@set -e; $(RM) $@; \
         $(CC) -MM $(CPPFLAGS) $< > $@.temp; \
         sed 's,\($*\)\.o[ :]*,$(OBJ_DIR)\/\1.o $@ : ,g' < $@.temp > $@; \
         $(RM) $@.temp

# Rule to generate object files
$(OBJ_DIR)/%.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $<

all: whatsOnFulGaz

whatsOnFulGaz: $(OBJECTS) Makefile
	$(CC) $(LDFLAGS) -o $(BIN_DIR)/$@ $(OBJECTS) -lcurl

clean:
	$(RM) $(OBJECTS) $(OBJ_DIR)/build_info.o $(DEP_DIR)/*.d $(BIN_DIR)/whatsOnFulGaz

include $(DEPS)

