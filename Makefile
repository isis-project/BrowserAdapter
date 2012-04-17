BUILD_TYPE := release
PLATFORM   := $(TARGET_ARCH)

STAGING_INC_DIR := $(OPIEDIR)/include
STAGING_LIB_DIR := $(OPIEDIR)/lib
QT_INSTALL_PREFIX := $(OPIEDIR)

INCLUDES := -I$(STAGING_INC_DIR) -I$(STAGING_INC_DIR)/Yap -I$(STAGING_INC_DIR)/BrowserServer
LIBS := -L$(STAGING_LIB_DIR) -lYap $(STAGING_LIB_DIR)/AdapterBase.a

INCLUDES += \
    -I$(STAGING_INC_DIR)/webkit/npapi \
    -I$(QT_INSTALL_PREFIX)/include/ \
    -I$(QT_INSTALL_PREFIX)/include/Qt \
    -I$(QT_INSTALL_PREFIX)/include/QtCore \
    -I$(QT_INSTALL_PREFIX)/include/QtGui \
    -I$(QT_INSTALL_PREFIX)/include/QtNetwork

LIBS += \
    -Wl,-rpath $(STAGING_LIB_DIR) \
    -L$(QT_INSTALL_PREFIX)/lib \
	-L$(STAGING_LIB_DIR) \
    -lQtGui \
    -lQtNetwork \
    -lQtCore

include Makefile.inc

install: all
	@mkdir -p $(INSTALL_DIR)/lib/BrowserPlugins
	install -m 0755 $(BUILD_TYPE)-$(PLATFORM)/BrowserAdapter.so $(INSTALL_DIR)/lib/BrowserPlugins

stage:
	@echo "nothing to do"
