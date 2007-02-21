#Copyright (C) 2007 L. Donnie Smith

LIB_NAME = $(PLUGIN_NAME).so

OBJECTS = $(SOURCES:.c=.o)
DEPS    = $(SOURCES:.c=.d)

CFLAGS += -fpic

all: $(LIB_NAME)

$(LIB_NAME): $(OBJECTS)
	$(CC) -shared $(LDFLAGS) $(LDLIBS) -o $(LIB_NAME) $(OBJECTS)

install: $(LIB_NAME)
	cp $(LIB_NAME) $(INST_DIR)

clean:
	rm -f $(LIB_NAME) $(OBJECTS) $(DEPS)

uninstall:
	rm -f $(INST_DIR)/$(LIB_NAME)

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(MAKECMDGOALS),distclean)
include $(COMMON)/include/dep.mak
-include $(DEPS)
endif
endif

.PHONY: all install clean uninstall
