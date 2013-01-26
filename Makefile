CC = gcc
DEBUG = -g
CFLAGS = -O2 -Wall -c $(DEBUG)
EXTRA_CFLAGS = $(shell pkg-config --cflags libnotify)
LFLAGS = -O2 -Wall $(shell pkg-config --libs libnotify) -lncurses -lz -lm $(DEBUG)

PROGRAM = chatsow

SOURCE = source/
BUILD = build/

THIS = Makefile
CFILES = $(wildcard $(SOURCE)*.c)
MODULES = $(patsubst $(SOURCE)%.c,%,$(CFILES))
OBJS = $(addprefix $(BUILD),$(addsuffix .o,$(MODULES)))
$(shell mkdir -p $(BUILD))

default: $(BUILD)$(PROGRAM)

clean:
	rm -rf $(BUILD)

test: $(BUILD)$(PROGRAM)
	./$(BUILD)$(PROGRAM)

$(BUILD)$(PROGRAM): $(OBJS)
	$(CC) $(LFLAGS) $(OBJS) -o $@

define module_depender
$(shell mkdir -p $(BUILD))
$(shell touch $(BUILD)$(1).d)
$(shell makedepend -f $(BUILD)$(1).d -- $(CFLAGS) -- $(SOURCE)$(1).c
	2>/dev/null)
$(shell sed -i 's~^$(SOURCE)~$(BUILD)~g' $(BUILD)$(1).d)
-include $(BUILD)$(1).d
endef

define module_compiler
$(BUILD)$(1).o: $(SOURCE)$(1).c $(THIS)
	$(CC) $(EXTRA_CFLAGS) $(CFLAGS) $$< -o $$@
endef

$(foreach module, $(MODULES), $(eval $(call module_depender,$(module))))
$(foreach module, $(MODULES), $(eval $(call module_compiler,$(module))))

.PHONY: default clean test
