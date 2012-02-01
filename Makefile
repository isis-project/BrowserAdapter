BUILD_TYPE := release
PLATFORM   := $(TARGET_ARCH)

INCLUDES := -I$(INCLUDE_DIR)/Yap -I$(INCLUDE_DIR)/mjson
INCLUDES += -I$(INCLUDE_DIR)/BrowserServer

INCLUDES += \
		-isystem $(INCLUDE_DIR)/Piranha \
		-isystem $(INCLUDE_DIR)/Piranha/Backend \
		-isystem $(INCLUDE_DIR)/Piranha/Font \
		-isystem $(INCLUDE_DIR)/Piranha/Utils/lfp \
		-isystem $(INCLUDE_DIR)/webkit/ \
		-isystem $(INCLUDE_DIR)/webkit/npapi

LIBS := -L$(LIB_DIR) -lglib-2.0 -lgthread-2.0 -lYap -lmjson -lWebKitLuna -lPiranha $(LIB_DIR)/AdapterBase.a

include Makefile.inc

install:
	@mkdir -p $(INSTALL_DIR)/usr/lib/BrowserPlugins
	install -m 755 $(BUILD_TYPE)-$(PLATFORM)/BrowserAdapter.so $(INSTALL_DIR)/usr/lib/BrowserPlugins

stage:
	@echo "nothing to do"
