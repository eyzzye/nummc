CXX = gcc
#CXXFLAGS = -g -I/usr/include -I/usr/include/SDL2
CXXFLAGS = -Ofast -I/usr/include -I/usr/include/SDL2
LDFLAGS = -lSDL2 -lSDL2_image -lSDL2_ttf -lSDL2_mixer -lBox2D -lstdc++ -lm -pthread

TARGET_EXEC = linux/nummc
BUILD_DIR = ./linux_build
SRC_DIRS = nummc/src
ICON_DIR = nummc

SRCS := $(shell find $(SRC_DIRS) -name *.cpp)
OBJS := $(SRCS:%=$(BUILD_DIR)/%.o)
DEPS := $(OBJS:.o=.d)

INC_DIRS := $(shell find $(SRC_DIRS) -type d)
INC_FLAGS := $(addprefix -I,$(INC_DIRS))
CPPFLAGS := $(INC_FLAGS) -MMD -MP

all: $(TARGET_EXEC)
	@echo "Done."

$(TARGET_EXEC): $(OBJS)
	mkdir -p $(dir $@)
	$(CC) $(OBJS) $(LDFLAGS) -o $@ 

$(BUILD_DIR)/%.cpp.o: %.cpp
	mkdir -p $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

install: $(TARGET_EXEC)
	mkdir -p $(DESTDIR)/usr/games
	mkdir -p $(DESTDIR)/usr/share/games/nummc
	cp $(TARGET_EXEC) $(DESTDIR)/usr/games/nummc
	cp -r data $(DESTDIR)/usr/share/games/nummc
	find $(DESTDIR)/usr/share/games/nummc -type d | xargs chmod 755
	find $(DESTDIR)/usr/share/games/nummc -type f | xargs chmod 644
# Desktop
	mkdir -p $(DESTDIR)/usr/share/icons/hicolor/16x16/apps
	mkdir -p $(DESTDIR)/usr/share/icons/hicolor/32x32/apps
	mkdir -p $(DESTDIR)/usr/share/icons/hicolor/48x48/apps
	mkdir -p $(DESTDIR)/usr/share/icons/hicolor/64x64/apps
	mkdir -p $(DESTDIR)/usr/share/applications/
	cp $(ICON_DIR)/icon16x16.png $(DESTDIR)/usr/share/icons/hicolor/16x16/apps/nummc.png
	cp $(ICON_DIR)/icon32x32.png $(DESTDIR)/usr/share/icons/hicolor/32x32/apps/nummc.png
	cp $(ICON_DIR)/icon48x48.png $(DESTDIR)/usr/share/icons/hicolor/48x48/apps/nummc.png
	cp $(ICON_DIR)/icon64x64.png $(DESTDIR)/usr/share/icons/hicolor/64x64/apps/nummc.png
	chmod 644 $(DESTDIR)/usr/share/icons/hicolor/16x16/apps/nummc.png
	chmod 644 $(DESTDIR)/usr/share/icons/hicolor/32x32/apps/nummc.png
	chmod 644 $(DESTDIR)/usr/share/icons/hicolor/48x48/apps/nummc.png
	chmod 644 $(DESTDIR)/usr/share/icons/hicolor/64x64/apps/nummc.png
	cp nummc.desktop $(DESTDIR)/usr/share/applications/
	chmod 644 $(DESTDIR)/usr/share/applications/nummc.desktop
# man mode
	chmod 644 docs/man/nummc.6

.PHONY: clean
clean:
	rm -r $(BUILD_DIR)
#	rm $(TARGET_EXEC)

-include $(DEPS)
