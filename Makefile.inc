OBJDIR := $(BUILD_TYPE)-$(PLATFORM)

# ------------------------------------------------------------------

TARGET_SO := $(OBJDIR)/BrowserAdapter.so

TARGET_SO_OBJS := \
	$(OBJDIR)/BrowserClientBase.o \
	$(OBJDIR)/BrowserAdapter.o \
	$(OBJDIR)/BrowserAdapterManager.o \
	$(OBJDIR)/Rectangle.o \
	$(OBJDIR)/UrlInfo.o \
	$(OBJDIR)/InteractiveInfo.o \
	$(OBJDIR)/ElementInfo.o \
	$(OBJDIR)/ImageInfo.o \
	$(OBJDIR)/JsonNPObject.o \
	$(OBJDIR)/NPObjectEvent.o \
	$(OBJDIR)/KineticScroller.o \
	$(OBJDIR)/BrowserOffscreen.o

# ------------------------------------------------------------------

FLAGS_COMMON := -fno-exceptions -fno-rtti -fvisibility=hidden -fPIC -DXP_UNIX -DXP_WEBOS

ifeq ("$(BUILD_TYPE)", "debug")
FLAGS_OPT := -O0 -g $(FLAGS_COMMON)
#-DDEBUG
else
FLAGS_OPT := -O2 -g $(FLAGS_COMMON) -DNDEBUG
endif

INCLUDES := $(INCLUDES) \
	-I. \
	`pkg-config --cflags glib-2.0`

LIBS := $(LIBS) \
	-lglib-2.0 \
	-lpng -lrt -lpthread -lcjson -lpbnjson_cpp

LOCAL_CFLAGS := $(CFLAGS) -MD -Wall -Werror $(FLAGS_OPT)

EXPORT_FLAGS = -Wl,--version-script=BrowserAdapter.exports

LOCAL_LFLAGS += $(LDFLAGS) $(EXPORT_FLAGS) $(FLAGS_OPT) $(LIBS) -Wl,--no-undefined

# ------------------------------------------------------------------

all: setup $(TARGET_SO)

setup:
	@mkdir -p $(OBJDIR)

$(TARGET_SO): $(TARGET_SO_OBJS)
	$(CXX) -o $(TARGET_SO) $(TARGET_SO_OBJS) $(LOCAL_LFLAGS) -shared -fPIC

vpath %.cpp

$(OBJDIR)/%.o: %.cpp
	$(CXX) $(INCLUDES) $(LOCAL_CFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJDIR)

-include $(wildcard $(OBJDIR)/*.d)
