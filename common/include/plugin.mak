#Copyright (C) 2007 L. Donnie Smith

LIB_NAME = $(PLUGIN_NAME).so

OBJECTS = $(SOURCES:.c=.o)
DEPS    = $(SOURCES:.c=.d)

CFLAGS += -fpic

#TODO:unify the way DESTDIR is handled
#Currently, defs.mak adds DESTDIR to the plugin INST_DIR,
#so we don't do it here
#DEST_INST_DIR = $(DESTDIR)/$(INST_DIR)
DEST_INST_DIR = $(INST_DIR)

all: $(LIB_NAME)

$(LIB_NAME): $(OBJECTS)
	$(CC) -shared $(LDFLAGS) $(LDLIBS) -o $(LIB_NAME) $(OBJECTS)

install: $(LIB_NAME) $(DEST_INST_DIR)
	install $(LIB_NAME) $(INST_DIR)

$(DEST_INST_DIR):
	install -d $(DEST_INST_DIR)

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
