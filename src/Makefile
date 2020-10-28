CC=gcc 

TARGET_EXEC ?= nano.exe

BUILD_DIR ?= ./build
SRC_DIRS ?= .
TARGET_DIR ?= .

# contrib/str_to_epoch.h\
#	terminal.h \
#	pool.h \
#	metadata.h \
#	stats.h \
#	query.h \

SRCS := \
	contrib/cJSON.c \
	contrib/cJSON_Utils.c \
	contrib/str_to_epoch.c \
	contrib/map.c \
	contrib/mongoose.c \
	common.c \
	tc_insert.c \
	tc_remove.c \
	logit.c \
	nodeset.c \
	node_std.c \
	schema.c \
	dump1.c \
	header.c \
	teste_block.c \
	metadata_parse.c \
	metadata.c \
	mlnet.c \
	stats.c \
	pool.c \
	terminal.c \
	query.c \
	query_parse.c \
	query_out.c \
	register.c \
	class_cat.c \
	class_geo.c \
	contents.c \
	contents_stats.c \
	container_binlist.c \
	container_binlist_addonly.c \
	iserver.c \
	webserver_mg.c \
	mestrado_01.c \
	main.c

OBJS := $(SRCS:%=$(BUILD_DIR)/%.o)
DEPS := $(OBJS:.o=.d)

# INC_DIRS := $(shell find $(SRC_DIRS) -type d)
# INC_FLAGS := $(addprefix -I,$(INC_DIRS))
LDFLAGS  := -lm  -l:libtcmalloc_minimal.so.4

CPPFLAGS ?= $(INC_FLAGS) -MMD -MP

$(TARGET_DIR)/$(TARGET_EXEC): $(OBJS)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)

# c source
$(BUILD_DIR)/%.c.o: %.c
	$(MKDIR_P) $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

.PHONY: clean

clean:
	$(RM) -r $(BUILD_DIR)

-include $(DEPS)

MKDIR_P ?= mkdir -p


