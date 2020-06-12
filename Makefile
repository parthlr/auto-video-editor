SRC_DIR := src
INCLUDE_DIR := include
BIN_DIR := bin
EXAMPLES_DIR := examples

FFMPEG_LIBS := libavformat \ libswscale
CPPFLAGS := -I$(INCLUDE_DIR)
CFLAGS := $(shell pkg-config --cflags $(FFMPEG_LIBS))
LDLIBS := $(shell pkg-config --libs $(FFMPEG_LIBS))

SRC_FILES := $(wildcard $(SRC_DIR)/*.c)
EXAMPLES_FILES := $(wildcard $(EXAMPLES_DIR)/*.c)
BIN_FILES := $(SRC_FILES:$(SRC_DIR)/%.c=$(BIN_DIR)/%.o)

test-video: $(BIN_FILES) $(BIN_DIR)/test-video.o
		$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)

test-sequence: $(BIN_FILES) $(BIN_DIR)/test-sequence.o
		$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)

test-cut: $(BIN_FILES) $(BIN_DIR)/test-cut.o
		$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)

test-interesting: $(BIN_FILES) $(BIN_DIR)/test-interesting.o
		$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)

$(BIN_DIR)/%.o: $(SRC_DIR)/%.c | $(BIN_DIR)
		$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BIN_DIR)/%.o: $(EXAMPLES_DIR)/%.c | $(BIN_DIR)
		$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BIN_DIR):
		mkdir -p $@

clean:
		$(RM) -f -r $(BIN_DIR)/*.o
