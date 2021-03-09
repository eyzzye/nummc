CXX = gcc
#CXXFLAGS = -g -I/usr/include -I/usr/include/SDL2
CXXFLAGS = -Ofast -I/usr/include -I/usr/include/SDL2
LDFLAGS = -lSDL2 -lSDL2_image -lSDL2_ttf -lSDL2_mixer -lBox2D -lstdc++ -lm -pthread

TARGET_EXEC = linux/nummc.bin
BUILD_DIR = ./linux_build
SRC_DIRS = nummc/src

SRCS := $(shell find $(SRC_DIRS) -name *.cpp)
OBJS := $(SRCS:%=$(BUILD_DIR)/%.o)
DEPS := $(OBJS:.o=.d)

INC_DIRS := $(shell find $(SRC_DIRS) -type d)
INC_FLAGS := $(addprefix -I,$(INC_DIRS))
CPPFLAGS := $(INC_FLAGS) -MMD -MP

$(TARGET_EXEC): $(OBJS)
	mkdir -p $(dir $@)
	$(CC) $(OBJS) $(LDFLAGS) -o $@ 

$(BUILD_DIR)/%.cpp.o: %.cpp
	mkdir -p $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

.PHONY: clean
clean:
	rm -r $(BUILD_DIR)
	rm $(TARGET_EXEC)

-include $(DEPS)
