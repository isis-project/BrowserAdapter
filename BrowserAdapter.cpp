/* @@@LICENSE
*
*      Copyright (c) 2012 Hewlett-Packard Development Company, L.P.
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*
LICENSE@@@ */

#include <sys/time.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <math.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <regex.h>
#include <unistd.h>
#include <libgen.h>
#include <assert.h>
#include <memory>
#include <string.h>
#include <glib.h>
#include <json.h>
#include <syslog.h>

#include <sys/types.h>
#include <pwd.h>

#include <palmwebtypes.h>

#include "BrowserAdapter.h"
#include "BrowserAdapterManager.h"
#include "BrowserCenteredZoom.h"
#include "BrowserMetaViewport.h"
#include "BrowserOffscreen.h"
#include "BrowserRect.h"
#include <BufferLock.h>
#include "Debug.h"

#include <BrowserClientBase.h>
#include "UrlInfo.h"
#include "InteractiveInfo.h"
#include "ImageInfo.h"
#include "ElementInfo.h"
#include "JsonNPObject.h"
#include "NPObjectEvent.h"
#include <PFilters.h>

#include <cjson/json.h>
#include <pbnjson.hpp>

extern "C" {
#include <png.h>
}

// what user does browserserver run as?
#define BROWSERVER_USER "luna"

/**
 * Define this value to write out each update rectangle to a separate PNG file.
 */
#undef DEBUG_PAINTS

/**
 * Define this value to show the flash rects
 */
#undef DEBUG_FLASH_RECTS

/** Define this to get more verbose logging. */
#undef VERBOSE_MESSAGESF

#ifdef VERBOSE_MESSAGES
#define VERBOSE_TRACE(...) TRACE(__VA_ARGS__)
#else
#define VERBOSE_TRACE(...) (void)0
#endif

const int ESC_KEY = 27;

static const float kFrozenSurfaceScale = 0.5f;

static const int kInvalidParam = -1;
static const double kDoubleEqualityTolerance = 0.00001;

static const double kMaxZoom = 4.0;
static const double kOverZoomFactor = 1.2;

static const int32_t kVirtualPageWidth= 960;
static const int32_t kVirtualPageHeight= 1400;

static const int32_t kDefaultLayoutWidth = kVirtualPageWidth;

static const int kHysteresis = 4;
static const int kMouseHoldInterval = 350;
static const int kDoubleClickInterval = 400;

static const int kTimerInterval = 16;
static const int kZoomAnimationSteps = 5;

static const int kNumRecordedGestures = 5;
static const int s_recordedGestureAvgWeights[] = { 1, 2, 4, 8, 16 };

static NPIdentifier gLoadProgressHandler = NULL;
static NPIdentifier gTitleURLChangeHandler = NULL;
static NPIdentifier gTitleChangeHandler = NULL;
static NPIdentifier gUrlChangeHandler = NULL;
static NPIdentifier gLinkClickedHandler = NULL;
static NPIdentifier gActionDataHandler = NULL;
static NPIdentifier gPageDimensionsHandler = NULL;
static NPIdentifier gCreatePageHandler = NULL;
static NPIdentifier gDownloadFinishedHandler = NULL;
static NPIdentifier gMimeHandoffUrlHandler = NULL;
static NPIdentifier gMimeNotSupportedHandler = NULL;
static NPIdentifier gDownloadErrorHandler = NULL;
static NPIdentifier gDownloadProgressHandler = NULL;
static NPIdentifier gDownloadStartHandler = NULL;
static NPIdentifier gDialogUserPasswordHandler = NULL;
static NPIdentifier gDialogAlertHandler = NULL;
static NPIdentifier gDialogConfirmHandler = NULL;
static NPIdentifier gDialogConfirmSSLHandler = NULL;
static NPIdentifier gDialogPromptHandler = NULL;
static NPIdentifier gLoadStartedHandler = NULL;
static NPIdentifier gLoadStoppedHandler = NULL;
static NPIdentifier gScrollToHandler = NULL;
static NPIdentifier gScrolledToHandler = NULL;
static NPIdentifier gClickRejectedHandler = NULL;
static NPIdentifier gPopupMenuShowHandler = NULL;
static NPIdentifier gPopupMenuHideHandler = NULL;
static NPIdentifier gSmartZoomCalculateResponseSimpleHandler = NULL;
static NPIdentifier gAdapterInitializedHandler = NULL;
static NPIdentifier gFailedLoadHandler = NULL;
static NPIdentifier gEditorFocusedHandler = NULL;
static NPIdentifier gReportErrorHandler = NULL;
static NPIdentifier gDidFinishDocumentLoadHandler = NULL;
static NPIdentifier gUpdateGlobalHistoryHandler = NULL;
static NPIdentifier gMainDocumentErrorHandler = NULL;
static NPIdentifier gBrowserServerDisconnectHandler = NULL;
static NPIdentifier gFirstPaintCompleteHandler = NULL;
static NPIdentifier gUrlRedirectedHandler = NULL;
static NPIdentifier gMetaViewportSetHandler = NULL;
static NPIdentifier gEval = NULL;
static NPIdentifier gPluginSpotlightCreate = NULL;
static NPIdentifier gPluginSpotlightRemove = NULL;
static NPIdentifier gMsgSpellingWidgetVisibleRectUpdate = NULL ;
static NPIdentifier gEventFiredHandler = NULL;
static NPIdentifier gMouseInFlashChangeHandler = NULL;
static NPIdentifier gFlashGestureLockHandler = NULL;
static NPIdentifier gMouseInInteractiveChangeHandler = NULL;
static NPIdentifier gShowPrintDialogHandler = NULL;
static NPIdentifier gServerConnectedHandler = NULL;

static char gDialogResponsePipe[50];

const char* kIconMaskFile = "/usr/lib/BrowserPlugins/BrowserAdapterData/launcher-bookmark-alpha.png";
const char* kIconOverlayFile = "/usr/lib/BrowserPlugins/BrowserAdapterData/launcher-bookmark-overlay.png";
const char* kSelectionReticleFile = "/usr/lib/BrowserPlugins/BrowserAdapterData/shift-tap-reticle.png";

//images for drop shadow
const char* kImgBotFile = "/usr/lib/BrowserPlugins/BrowserAdapterData/fl-win-shdw-bot.png";
const char* kImgBotLeftFile = "/usr/lib/BrowserPlugins/BrowserAdapterData/fl-win-shdw-bot-left-corner.png";
const char* kImgBotRightFile = "/usr/lib/BrowserPlugins/BrowserAdapterData/fl-win-shdw-bot-right-corner.png";
const char*  kImgLeftFile= "/usr/lib/BrowserPlugins/BrowserAdapterData/fl-win-shdw-left.png";
const char*  kImgRightFile= "/usr/lib/BrowserPlugins/BrowserAdapterData/fl-win-shdw-right.png";
const char*  kImgTopFile= "/usr/lib/BrowserPlugins/BrowserAdapterData/fl-win-shdw-top.png";
const char*  kImgTopRightFile= "/usr/lib/BrowserPlugins/BrowserAdapterData/fl-win-shdw-top-right-corner.png";
const char*  kImgTopLeftFile= "/usr/lib/BrowserPlugins/BrowserAdapterData/fl-win-shdw-top-left-corner.png";
// -----------------------------------------------------------------------------------
// Mandatory Plugin Stub Implementations
// -----------------------------------------------------------------------------------

AdapterBase* AdapterCreate(NPP instance, GMainLoop* mainLoop, int16_t argc, char* argn[], char* argv[])
{
    return new BrowserAdapter(instance, g_main_loop_get_context(mainLoop), argc, argn, argv);
}


NPError AdapterLibInitialize(void)// Called when proxy library is loaded by browser, may be empty. Returns NPERR_NO_ERROR on success.
{
    // Look up identifiers for JavaScript side event handler methods:
    gActionDataHandler = AdapterBase::NPN_GetStringIdentifier("actionData");
    gCreatePageHandler = AdapterBase::NPN_GetStringIdentifier("createPage");
    gDialogAlertHandler = AdapterBase::NPN_GetStringIdentifier("dialogAlert");
    gDialogConfirmHandler = AdapterBase::NPN_GetStringIdentifier("dialogConfirm");
    gDialogConfirmSSLHandler = AdapterBase::NPN_GetStringIdentifier("dialogSSLConfirm");
    gDialogUserPasswordHandler = AdapterBase::NPN_GetStringIdentifier("dialogUserPassword");
    gDialogPromptHandler = AdapterBase::NPN_GetStringIdentifier("dialogPrompt");
    gDownloadErrorHandler = AdapterBase::NPN_GetStringIdentifier("downloadError");
    gDownloadProgressHandler = AdapterBase::NPN_GetStringIdentifier("downloadProgress");
    gDownloadFinishedHandler = AdapterBase::NPN_GetStringIdentifier("downloadFinished");
    gDownloadStartHandler = AdapterBase::NPN_GetStringIdentifier("downloadStart");
    gLinkClickedHandler = AdapterBase::NPN_GetStringIdentifier("linkClicked");
    gLoadProgressHandler = AdapterBase::NPN_GetStringIdentifier("loadProgress");
    gLoadStartedHandler = AdapterBase::NPN_GetStringIdentifier("loadStarted");
    gLoadStoppedHandler = AdapterBase::NPN_GetStringIdentifier("loadStopped");
    gMimeHandoffUrlHandler = AdapterBase::NPN_GetStringIdentifier("mimeHandoffUrl");
    gMimeNotSupportedHandler = AdapterBase::NPN_GetStringIdentifier("mimeNotSupported");
    gPageDimensionsHandler = AdapterBase::NPN_GetStringIdentifier("pageDimensions");
    gScrollToHandler = AdapterBase::NPN_GetStringIdentifier("scrollTo");
    gScrolledToHandler = AdapterBase::NPN_GetStringIdentifier("scrolledTo");
    gPopupMenuShowHandler = AdapterBase::NPN_GetStringIdentifier("showPopupMenu");
    gPopupMenuHideHandler = AdapterBase::NPN_GetStringIdentifier("hidePopupMenu");
    gTitleChangeHandler = AdapterBase::NPN_GetStringIdentifier("titleChange");
    gTitleURLChangeHandler = AdapterBase::NPN_GetStringIdentifier("titleURLChange");
    gUrlChangeHandler = AdapterBase::NPN_GetStringIdentifier("urlChange");
    gClickRejectedHandler = AdapterBase::NPN_GetStringIdentifier("clickRejected");
    gSmartZoomCalculateResponseSimpleHandler = AdapterBase::NPN_GetStringIdentifier("smartZoomCalculateResponseSimple");
    gAdapterInitializedHandler = AdapterBase::NPN_GetStringIdentifier("adapterInitialized");
    gFailedLoadHandler = AdapterBase::NPN_GetStringIdentifier("failedLoad");
    gEditorFocusedHandler = AdapterBase::NPN_GetStringIdentifier("editorFocused");
    gReportErrorHandler = AdapterBase::NPN_GetStringIdentifier("reportError");
    gDidFinishDocumentLoadHandler = AdapterBase::NPN_GetStringIdentifier("didFinishDocumentLoad");
    gUpdateGlobalHistoryHandler = AdapterBase::NPN_GetStringIdentifier("updateGlobalHistory");
    gMainDocumentErrorHandler = AdapterBase::NPN_GetStringIdentifier("setMainDocumentError");
    gBrowserServerDisconnectHandler = AdapterBase::NPN_GetStringIdentifier("browserServerDisconnected");
    gFirstPaintCompleteHandler = AdapterBase::NPN_GetStringIdentifier("firstPaintComplete");
    gUrlRedirectedHandler = AdapterBase::NPN_GetStringIdentifier("urlRedirected");
    gMetaViewportSetHandler = AdapterBase::NPN_GetStringIdentifier("metaViewportSet");
    gEval = AdapterBase::NPN_GetStringIdentifier("eval");
    gPluginSpotlightCreate = AdapterBase::NPN_GetStringIdentifier("pluginSpotlightCreate");
    gPluginSpotlightRemove = AdapterBase::NPN_GetStringIdentifier("pluginSpotlightRemove");
    gMsgSpellingWidgetVisibleRectUpdate = AdapterBase::NPN_GetStringIdentifier("spellingWidgetRectUpdate");
    gEventFiredHandler = AdapterBase::NPN_GetStringIdentifier("eventFired");
    gMouseInFlashChangeHandler = AdapterBase::NPN_GetStringIdentifier("mouseInFlashChange");
    gFlashGestureLockHandler = AdapterBase::NPN_GetStringIdentifier("flashGestureLockChange");
    gMouseInInteractiveChangeHandler = AdapterBase::NPN_GetStringIdentifier("mouseInInteractiveChange");
    gShowPrintDialogHandler = AdapterBase::NPN_GetStringIdentifier("showPrintDialog");
    gServerConnectedHandler = AdapterBase::NPN_GetStringIdentifier("serverConnected");
    return NPERR_NO_ERROR;
}

const char* AdapterGetMIMEDescription(void)
{
    TRACE;
    return "application/x-palm-browser";
}


uint32_t AdapterGetMethods(const char*** outNames, const JSMethodPtr **outMethods, NPIdentifier **outIDs)
{
    static const char* names[]= {
        "clearHistory",
        "deleteImage",
        "generateIconFromFile",
        "getHistoryState",
        "goBack",
        "goForward",
        "interrogateClicks",
        "openURL",
        "pageScaleAndScroll",
        "reloadPage",
        "resizeImage",
        "saveViewToFile",
        "setMinFontSize",
        "setPageIdentifier",
        "smartZoom",
        "setViewportSize",
        "stopLoad",
        "setMagnification",
        "clickAt",
        "selectPopupMenuItem",
        "sendDialogResponse",
        "gestureStart",
        "gestureChange",
        "gestureEnd",
        "scrollStarting",
        "scrollEnding",
        "setEnableJavaScript",
        "setBlockPopups",
        "setHeaderHeight",
        "setAcceptCookies",
        "setShowClickedLink",
        "mouseEvent",
        "connectBrowserServer",
        "disconnectBrowserServer",
        "inspectUrlAtPoint",
        "addUrlRedirect",
        "pageFocused",
        "addElementHighlight",
        "removeElementHighlight",
        "isEditing",
        "insertStringAtCursor",
        "enableSelectionMode",
        "disableSelectionMode",
        "clearSelection",
        "saveImageAtPoint",
        "selectAll",
        "cut",
        "copy",
        "paste",
        "getImageInfoAtPoint",
        "interactiveAtPoint",
        "elementInfoAtPoint",
        "setMouseMode",
        "startDeferWindowChange",
        "stopDeferWindowChange",
        "setSpotlight",
        "removeSpotlight",
        "setHTML",
        "disableEnhancedViewport",
        "ignoreMetaTags",
        "setNetworkInterface",
        "enableFastScaling",
        "setDefaultLayoutWidth",
        "printFrame",
        "skipPaintHack",
        "findInPage",
        "mouseHoldAt",
        "handleFlick",
        "setVisibleSize",
        "setDNSServers"
    };

    const size_t kExposedMethodCount = G_N_ELEMENTS(names);
    static const JSMethodPtr methods[kExposedMethodCount]= {
        BrowserAdapter::js_clearHistory,
        BrowserAdapter::js_deleteImage,
        BrowserAdapter::js_generateIconFromFile,
        BrowserAdapter::js_getHistoryState,
        BrowserAdapter::js_goBack,
        BrowserAdapter::js_goForward,
        BrowserAdapter::js_interrogateClicks,
        BrowserAdapter::js_openURL,
        BrowserAdapter::js_pageScaleAndScroll,
        BrowserAdapter::js_reloadPage,
        BrowserAdapter::js_resizeImage,
        BrowserAdapter::js_saveViewToFile,
        BrowserAdapter::js_setMinFontSize,
        BrowserAdapter::js_setPageIdentifier,
        BrowserAdapter::js_smartZoom,
        BrowserAdapter::js_setViewportSize,
        BrowserAdapter::js_stopLoad,
        BrowserAdapter::js_setMagnification,
        BrowserAdapter::js_clickAt,
        BrowserAdapter::js_selectPopupMenuItem,
        BrowserAdapter::js_sendDialogResponse,
        BrowserAdapter::js_gestureStart,
        BrowserAdapter::js_gestureChange,
        BrowserAdapter::js_gestureEnd,
        BrowserAdapter::js_scrollStarting,
        BrowserAdapter::js_scrollEnding,
        BrowserAdapter::js_setEnableJavaScript,
        BrowserAdapter::js_setBlockPopups,
        BrowserAdapter::js_setHeaderHeight,
        BrowserAdapter::js_setAcceptCookies,
        BrowserAdapter::js_setShowClickedLink,
        BrowserAdapter::js_mouseEvent,
        BrowserAdapter::js_connectBrowserServer,
        BrowserAdapter::js_disconnectBrowserServer,
        BrowserAdapter::js_inspectUrlAtPoint,
        BrowserAdapter::js_addUrlRedirect,
        BrowserAdapter::js_pageFocused,
        BrowserAdapter::js_addElementHighlight,
        BrowserAdapter::js_removeElementHighlight,
        BrowserAdapter::js_isEditing,
        BrowserAdapter::js_insertStringAtCursor,
        BrowserAdapter::js_enableSelectionMode,
        BrowserAdapter::js_disableSelectionMode,
        BrowserAdapter::js_clearSelection,
        BrowserAdapter::js_saveImageAtPoint,
        BrowserAdapter::js_selectAll,
        BrowserAdapter::js_cut,
        BrowserAdapter::js_copy,
        BrowserAdapter::js_paste,
        BrowserAdapter::js_getImageInfoAtPoint,
        BrowserAdapter::js_interactiveAtPoint,
        BrowserAdapter::js_getElementInfoAtPoint,
        BrowserAdapter::js_setMouseMode,
        BrowserAdapter::js_startDeferWindowChange,
        BrowserAdapter::js_stopDeferWindowChange,
        BrowserAdapter::js_setSpotlight,
        BrowserAdapter::js_removeSpotlight,
        BrowserAdapter::js_setHTML,
        BrowserAdapter::js_disableEnhancedViewport,
        BrowserAdapter::js_ignoreMetaTags,
        BrowserAdapter::js_setNetworkInterface,
        BrowserAdapter::js_enableFastScaling,
        BrowserAdapter::js_setDefaultLayoutWidth,
        BrowserAdapter::js_printFrame,
        BrowserAdapter::js_skipPaintHack,
        BrowserAdapter::js_findInPage,
        BrowserAdapter::js_mouseHoldAt,
        BrowserAdapter::js_handleFlick,
        BrowserAdapter::js_setVisibleSize,
        BrowserAdapter::js_setDNSServers
    };

    static NPIdentifier ids[kExposedMethodCount] = {NULL};

    *outNames = names;
    *outMethods = methods;
    *outIDs = ids;
    return kExposedMethodCount;
}

// -----------------------------------------------------------------------------------
// BrowserAdapter Implementation
// -----------------------------------------------------------------------------------

static inline bool PrvIsEqual(double a, double b)
{
    return (::fabs(a - b) < kDoubleEqualityTolerance);
}

/**
 * Constructor. The
 */
BrowserAdapter::BrowserAdapter(NPP instance, GMainContext *ctxt, int16_t argc, char* argn[], char* argv[])
    : BrowserClientBase("browser", ctxt)
    , AdapterBase(instance, true, true)
    , mScroller(0)
    , mDirtyPattern(0)
    , m_pageOffset(0, 0)
    , m_touchPtDoc(0, 0)
    , mViewportWidth(0)
    , mViewportHeight(0)
    , mZoomLevel(1.0)
    , mZoomFit(true)
    , mPageWidth(0)
    , mPageHeight(0)
    , mContentWidth(0)
    , mContentHeight(0)
    , mScrollPos(0, 0)
    , mInGestureChange(false)
    , mMetaViewport(0)
    , mCenteredZoom(0)
    , mOffscreen0(0)
    , mOffscreen1(0)
    , mOffscreenCurrent(0)
    , mFrozenSurface(0)
    , mFrozen(false)
    , mFrozenRenderPos(0, 0)
    , m_passInputEvents(false)
    , mFirstPaintComplete(false)
    , mEnableJavaScript(true)
    , mBlockPopups(true)
    , mAcceptCookies(true)
    , mShowClickedLink(true)
    , m_defaultLayoutWidth(kDefaultLayoutWidth)
    , mBrowserServerConnected(false)
    , mNotifiedOfBrowserServerDisconnect(false)
    , mSendFinishDocumentLoadNotification(false)
    , mArgc(argc)
    , mArgn(NULL)
    , mArgv(NULL)
    , mPaintEnabled(true)
    , mInScroll(false)
    , bEditorFocused(false)
    , m_useFastScaling(false)
    , mPageIdentifier(-1)
    , mBsQueryNum(0)
    , m_interrogateClicks(false)
    , mMouseMode(0)
    , mLogAlertMessages(getenv("LOG_BROWSER_ALERTS") != NULL)
    , mPageFocused(true)
    , mServerConnectedInvoked(false)
    , mServerConnectedInvoking(false)
    , mShowHighlight(false)
    , mLastPointSentToFlash(-1, -1)
    , mMouseInFlashRect(false)
    , mFlashGestureLock(false)
    , mMouseInInteractiveRect(false)
    , m_spotlightHandle(0)
    , m_hitTestSchema(pbnjson::JSchemaFile("/etc/palm/browser/HitTest.schema"))
    , m_ft(0)
    , m_clickPt(0, 0)
    , m_penDownDoc(0, 0)
    , m_dragging(false)
    , m_didDrag(false)
    , m_didHold(false)
    , m_sentToBS(false)
    , m_clickTimerSource(0)
    , m_mouseHoldTimerSource(0)
    , m_zoomTimerSource(0)
    , m_zoomPt(0,0)
    , m_zoomTarget(0.0)
    , m_zoomXInterval(0)
    , m_zoomYInterval(0)
    , m_zoomLevelInterval(0)
    , m_headerHeight(0)
    , mScrollbarOpacity(0)
    , mScrollbarFadeSource(0)
    , m_bufferLock(0)
    , m_bufferLockName(0)
{

    //openlog("browser-adapter", 0, LOG_USER);
    g_message("%s: %p", __PRETTY_FUNCTION__, this);

    TRACE;

    mScroller = new KineticScroller(ctxt);
    mScroller->setListener(this);

    memset(gDialogResponsePipe, 0, G_N_ELEMENTS(gDialogResponsePipe));

    resetScrollableLayerScrollSession();

    // Duplicate the args for lazy connection to the browser server.
    mArgn = new char*[argc];
    mArgv = new char*[argc];
    for (int16_t i = 0; i < argc; i++) {
        mArgn[i] = strdup(argn[i]);
        mArgv[i] = strdup(argv[i]);
    }

    /* Parse arguments */
    for (int i = 0; i < mArgc; i++) {
        if (0 == strcasecmp(mArgn[i], "usemouseevents")) {
            m_passInputEvents = (strcasecmp(mArgv[i], "true") == 0);
        } else if (0 == strcasecmp(mArgn[i], "viewportwidth")) {
            mViewportWidth = atoi(mArgv[i]);
        } else if (0 == strcasecmp(mArgn[i], "viewportheight")) {
            mViewportHeight = atoi(mArgv[i]);
        }
    }

    if ((mViewportWidth == 0) || (mViewportHeight == 0)) {
        syslog(LOG_DEBUG, "viewport dimensions are unset");
        setDefaultViewportSize();
    }

    // Freeze at startup
    freeze();

    // selection reticle setup
    initSelectionReticleSurface();

    BrowserAdapterManager::instance()->registerAdapter(this);

    TRACEF("pass events %d viewport %dx%d", m_passInputEvents,
           mViewportWidth, mViewportHeight);

    // Only at the end shall we inform our listener that we're initialized.
    InvokeEventListener(gAdapterInitializedHandler, NULL, 0, NULL);
}

BrowserAdapter::~BrowserAdapter()
{
    TRACE;

    g_message("%s: %p", __PRETTY_FUNCTION__, this);

    destroyBufferLock();

    stopClickTimer();
    stopMouseHoldTimer();
    stopZoomAnimation();

    delete mScroller;
    delete mMetaViewport;
    delete mCenteredZoom;

    for (int16_t i = 0; i < mArgc; i++) {
        free(mArgn[i]);
        free(mArgv[i]);
    }
    delete [] mArgn;
    delete [] mArgv;

    if (mDirtyPattern)
        mDirtyPattern->releaseRef();

    if (mFrozenSurface)
        mFrozenSurface->releaseRef();

    delete mOffscreen0;
    delete mOffscreen1;

    std::list<UrlRedirectInfo*>::iterator i;
    for (i = m_urlRedirects.begin(); i != m_urlRedirects.end(); ++i) {
        delete *i;
    }

    if (mSelectionReticle.timeoutSource != NULL) {
        g_source_destroy(mSelectionReticle.timeoutSource);
        g_source_unref(mSelectionReticle.timeoutSource);
        mSelectionReticle.timeoutSource = NULL;
    }

    if (mSelectionReticle.surface != NULL) {
        mSelectionReticle.surface->releaseRef();
        mSelectionReticle.surface = NULL;
    }

    BrowserAdapterManager::instance()->unregisterAdapter(this);

    stopFadeScrollbar();

    closelog();
}

/**
 * Is the directory that this file will be written to safe?
 */
bool BrowserAdapter::isSafeDir(const char* pszFileName)
{
    return strncasecmp("/var/", pszFileName, 5) == 0 || strncasecmp("/tmp/", pszFileName, 5) == 0;
}

BrowserAdapter::InspectUrlAtPointArgs::InspectUrlAtPointArgs(int x, int y, NPObject* cb, int queryNum) :
    BrowserServerCallArgs(queryNum), m_x(x), m_y(y), m_successCb(cb) {
    if (m_successCb) {
        AdapterBase::NPN_RetainObject(m_successCb);
    }
}

BrowserAdapter::InspectUrlAtPointArgs::~InspectUrlAtPointArgs ()
{
    if (m_successCb) {
        AdapterBase::NPN_ReleaseObject(m_successCb);
    }
}

BrowserAdapter::SaveImageAtPointArgs::SaveImageAtPointArgs(int x, int y, NPObject* cb, int queryNum) :
    BrowserServerCallArgs(queryNum), m_x(x), m_y(y), m_callback(cb) {
    if (m_callback) {
        AdapterBase::NPN_RetainObject(m_callback);
    }
}

BrowserAdapter::SaveImageAtPointArgs::~SaveImageAtPointArgs ()
{
    if (m_callback) {
        AdapterBase::NPN_ReleaseObject(m_callback);
    }
}

BrowserAdapter::GetImageInfoAtPointArgs::GetImageInfoAtPointArgs(int x, int y, NPObject* cb, int queryNum) :
    BrowserServerCallArgs(queryNum), m_x(x), m_y(y), m_callback(cb) {
    if (m_callback) {
        AdapterBase::NPN_RetainObject(m_callback);
    }
}

BrowserAdapter::GetImageInfoAtPointArgs::~GetImageInfoAtPointArgs ()
{
    if (m_callback) {
        AdapterBase::NPN_ReleaseObject(m_callback);
    }
}

BrowserAdapter::IsInteractiveAtPointArgs::IsInteractiveAtPointArgs(int x, int y, NPObject* cb, int queryNum) :
    BrowserServerCallArgs(queryNum), m_x(x), m_y(y), m_callback(cb) {
    if (m_callback) {
        AdapterBase::NPN_RetainObject(m_callback);
    }
}

BrowserAdapter::IsInteractiveAtPointArgs::~IsInteractiveAtPointArgs ()
{
    if (m_callback) {
        AdapterBase::NPN_ReleaseObject(m_callback);
    }
}

BrowserAdapter::GetElementInfoAtPointArgs::GetElementInfoAtPointArgs(int x, int y, NPObject* cb, int queryNum) :
    BrowserServerCallArgs(queryNum), m_x(x), m_y(y), m_callback(cb) {
    if (m_callback) {
        AdapterBase::NPN_RetainObject(m_callback);
    }
}

BrowserAdapter::GetElementInfoAtPointArgs::~GetElementInfoAtPointArgs ()
{
    if (m_callback) {
        AdapterBase::NPN_ReleaseObject(m_callback);
    }
}

BrowserAdapter::GetHistoryStateArgs::GetHistoryStateArgs(NPObject* cb, int queryNum) :
    BrowserServerCallArgs(queryNum), m_successCb(cb) {
    if (m_successCb) {
        AdapterBase::NPN_RetainObject(m_successCb);
    }
}

BrowserAdapter::GetHistoryStateArgs::~GetHistoryStateArgs ()
{
    if (m_successCb) {
        AdapterBase::NPN_ReleaseObject(m_successCb);
    }
}

BrowserAdapter::IsEditingArgs::IsEditingArgs(NPObject* cb, int queryNum) :
    BrowserServerCallArgs(queryNum), m_callback(cb) {
    if (m_callback) {
        AdapterBase::NPN_RetainObject(m_callback);
    }
}

BrowserAdapter::IsEditingArgs::~IsEditingArgs ()
{
    if (m_callback) {
        AdapterBase::NPN_ReleaseObject(m_callback);
    }
}


BrowserAdapter::CopySuccessCallbackArgs::CopySuccessCallbackArgs(NPObject* cb, int queryNum) :
    BrowserServerCallArgs(queryNum), m_callback(cb) {
    if (m_callback) {
        AdapterBase::NPN_RetainObject(m_callback);
    }
}

BrowserAdapter::CopySuccessCallbackArgs::~CopySuccessCallbackArgs ()
{
    if (m_callback) {
        AdapterBase::NPN_ReleaseObject(m_callback);
    }
}

BrowserAdapter::HitTestArgs::HitTestArgs(const char *type, const Point& pt,
        int modifiers, int queryNum) :
    BrowserServerCallArgs(queryNum),
    m_type(type),
    m_pt(pt),
    m_modifiers(modifiers)
{
}

BrowserAdapter::HitTestArgs::~HitTestArgs()
{
}

BrowserAdapter::GetTextCaretArgs::GetTextCaretArgs(NPObject *cb, int queryNum) :
    BrowserServerCallArgs(queryNum),
    m_callback(cb)
{
    if (m_callback) {
        AdapterBase::NPN_RetainObject(m_callback);
    }
}

BrowserAdapter::GetTextCaretArgs::~GetTextCaretArgs()
{
}

/**
 * Send the adapter's state down the server (happens whenever we successfully connect).
 */
void BrowserAdapter::sendStateToServer()
{
    int32_t virtualPageWidth = kVirtualPageWidth;
    int32_t virtualPageHeight = kVirtualPageHeight;

    for (int i=0; i<mArgc; i++) {
        TRACEF("mArgn[%d]=%s, mArgv[%d]=%s", i, mArgn[i], i, mArgv[i]);
        if (0==strcasecmp(mArgn[i], "virtualpagewidth")) {
            virtualPageWidth = atoi(mArgv[i]);
        } else if (0==strcasecmp(mArgn[i], "virtualpageheight")) {
            virtualPageHeight = atoi(mArgv[i]);
        }
    }

    asyncCmdConnect(virtualPageWidth, virtualPageHeight, mOffscreen0->key(),
                    mOffscreen1->key(), mOffscreen0->size(), mPageIdentifier);
    asyncCmdSetWindowSize(mViewportWidth, mViewportHeight);
    asyncCmdPageFocused(mPageFocused);
    asyncCmdSetMouseMode(mMouseMode);
    asyncCmdInterrogateClicks(m_interrogateClicks);

    asyncCmdSetEnableJavaScript(mEnableJavaScript);
    asyncCmdSetBlockPopups(mBlockPopups);
    asyncCmdSetAcceptCookies(mAcceptCookies);
    asyncCmdSetShowClickedLink(mShowClickedLink);
    asyncCmdSetZoomAndScroll(mZoomLevel, mScrollPos.x, mScrollPos.y);

    const char* identifier = (const char*) NPN_GetValue((NPNVariable) npPalmApplicationIdentifier);
    if (identifier)
        asyncCmdSetAppIdentifier(identifier);

    std::list<UrlRedirectInfo*>::const_iterator i;
    for (i = m_urlRedirects.begin(); i != m_urlRedirects.end(); ++i) {
        asyncCmdAddUrlRedirect((*i)->re.c_str(), (*i)->type, (*i)->redir, (*i)->udata.c_str());
    }
}

/**
 * Return the value of PalmSystem.isActivated.
 */
bool BrowserAdapter::isActivated()
{
    bool activated(false);

    NPVariant jsCallResult, jsCallArgs;
    STRINGZ_TO_NPVARIANT("PalmSystem.isActivated", jsCallArgs);
    if (InvokeAdapter(gEval, &jsCallArgs, 1, &jsCallResult)) {
        if (IsBooleanVariant(jsCallResult)) {
            activated = VariantToBoolean(jsCallResult);
        }
    }

    return activated;
}

/**
 * Complete the initialization of this object. Parses parameters and
 * establishes connection to the BrowserServer.
 *
 * @return true if successful, false if not.
 */
bool BrowserAdapter::init()
{
    if (!isActivated()) {
        return false;
    }

    if (!initializeIpcBuffer()) {
        serverDisconnected();
        return false;
    }

    if (mBrowserServerConnected) {
        serverConnected();
        return true;
    }

    createBufferLock();

    bool successful = connect();
    if (successful) {
        sendStateToServer();
        serverConnected();

        if (mFrozen)
            BrowserAdapterManager::instance()->adapterActivated(this, true);

    } else {

        TRACEF("%s: unable to connect to BrowserServer.", __FUNCTION__);

        serverDisconnected();
    }

    if (!mDirtyPattern) {
        const int patternSize = 8;
        mDirtyPattern = PGSurface::create(patternSize * 2, patternSize * 2, false);

        PGContext* gc = PGContext::create();
        gc->setSurface(mDirtyPattern);
        gc->setStrokeColor(PColor32(0x00, 0x00, 0x00, 0x00));

        gc->setFillColor(PColor32(0xCC, 0xCC, 0xCC, 0xFF));
        gc->drawRect(0, 0, patternSize, patternSize);
        gc->drawRect(patternSize, patternSize, patternSize * 2, patternSize * 2);

        gc->setFillColor(PColor32(0xEE, 0xEE, 0xEE, 0xFF));
        gc->drawRect(patternSize, 0, patternSize * 2, patternSize);
        gc->drawRect(0, patternSize, patternSize, patternSize * 2);

        gc->releaseRef();
    }

    return successful;
}

bool BrowserAdapter::initializeIpcBuffer()
{
    if (!mOffscreen0)
        mOffscreen0 = BrowserOffscreen::create();

    if (!mOffscreen0)
        return false;

    if (!mOffscreen1)
        mOffscreen1 = BrowserOffscreen::create();

    if (!mOffscreen1) {
        delete mOffscreen0;
        mOffscreen0 = 0;
        return false;
    }

    return true;
}

void BrowserAdapter::setDefaultViewportSize()
{
    NPVariant jsCallResult, jsCallArgs;

    GetScreenResolution(mViewportWidth, mViewportHeight);

    STRINGZ_TO_NPVARIANT("PalmSystem.deviceInfo.maximumCardWidth", jsCallArgs);
    if (InvokeAdapter(gEval, &jsCallArgs, 1, &jsCallResult)) {
        if (IsIntegerVariant(jsCallResult)) {
            mViewportWidth = VariantToInteger(jsCallResult);
        }
    }

    STRINGZ_TO_NPVARIANT("PalmSystem.deviceInfo.maximumCardHeight", jsCallArgs);
    if (InvokeAdapter(gEval, &jsCallArgs, 1, &jsCallResult)) {
        if (IsIntegerVariant(jsCallResult)) {
            mViewportHeight = VariantToInteger(jsCallResult);
        }
    }
}

/**
 * We lazily connect to the BrowserServer. This is just a handly routine
 * that will cast and then initialize the BrowserAdapter.
 */
BrowserAdapter* BrowserAdapter::GetAndInitAdapter( AdapterBase* adapter )
{
    BrowserAdapter* pAdapter = static_cast<BrowserAdapter*>(adapter);
    pAdapter->init();// Safe to call multiple times.
    return pAdapter;
}

static PColor32 colorNoOffscreen = PColor32(0xff, 0xff, 0x00, 0xff);
static PColor32 colorNoConnection = PColor32(0x00, 0xff, 0xff, 0xff);
static PColor32 colorOffscreenSurfEmpty = PColor32(0xff, 0x00, 0xff, 0xff);
static PColor32 colorGenericBorder = PColor32(0x00, 0x00, 0xff, 0xff);

//#define DRAW_DEBUG_COLORS
#if defined(DRAW_DEBUG_COLORS)

static void drawDebugBorder(PGContext* gc, NPWindow* window, PColor32 color)
{
    gc->push();
    gc->translate(window->x, window->y);
    gc->addClipRect(0, 0, window->width, window->height);
    gc->setFillColor(PColor32(0,0,0,0));
    gc->setStrokeThickness(2);
    gc->setStrokeColor(color);
    gc->drawRect(0, 0, window->width, window->height);
    gc->pop();
}

static void drawDebugFill(PGContext* gc, NPWindow* window, PColor32 color)
{
    gc->push();
    gc->translate(window->x, window->y);
    gc->addClipRect(0, 0, window->width, window->height);
    gc->setFillColor(color);
    gc->drawRect(0, 0, window->width, window->height);
    gc->pop();
}

#else

static void drawDebugBorder(PGContext* gc, NPWindow* window, PColor32 color)
{
}

static void drawDebugFill(PGContext* gc, NPWindow* window, PColor32 color)
{
}

#endif


void BrowserAdapter::handlePaint(NpPalmDrawEvent* event)
{
    // even mFrozen = false, we might still draw with mFrozenSurface
    // Waiting msgPainted event has not come
    if (mFrozenSurface) {
        handlePaintInFrozenState(event);
        drawDebugBorder((PGContext*) event->graphicsContext, &mWindow, colorGenericBorder);
        return;
    }

    if (mOffscreenCurrent == 0) {
        drawDebugFill((PGContext*) event->graphicsContext, &mWindow, mBrowserServerConnected ? colorNoOffscreen : colorNoConnection);
        return;
    }

    BrowserOffscreenInfo* info = mOffscreenCurrent->header();
    PGSurface* offscreenSurf = mOffscreenCurrent->surface();
    if (!offscreenSurf ||
            offscreenSurf->width() == 0 ||
            offscreenSurf->height() == 0) {
        drawDebugFill((PGContext*) event->graphicsContext, &mWindow, mBrowserServerConnected ? colorOffscreenSurfEmpty : colorNoConnection);
        return;
    }

    PGContext* gc = (PGContext*) event->graphicsContext;
    gc->push();

    gc->translate(mWindow.x, mWindow.y);
    gc->addClipRect(0, 0, mWindow.width, mWindow.height);

    // For debug drawing
    //gc->setFillColor(PColor32(0xff, 0, 0, 0xff));
    //gc->drawRect(0, 0, mWindow.width, mWindow.height);

    // Paint checkerboard
    {
        // Content rect

        BrowserRect offscreenRect(info->renderedX,
                                  info->renderedY,
                                  info->renderedWidth,
                                  info->renderedHeight);

        if (!PrvIsEqual(info->contentZoom, mZoomLevel)) {
            int centerX = offscreenRect.x() + offscreenRect.w() / 2;
            int centerY = offscreenRect.y() + offscreenRect.h() / 2;

            int w = offscreenRect.w() / info->contentZoom * mZoomLevel;
            int h = offscreenRect.h() / info->contentZoom * mZoomLevel;

            offscreenRect = BrowserRect(centerX - w / 2,
                                        centerY - h / 2,
                                        w, h);
        }

        BrowserRect windowRect(mScrollPos.x,
                               mScrollPos.y,
                               mWindow.width,
                               mWindow.height);

        BrowserRect contentRect(0, 0, mContentWidth, mContentHeight);
        windowRect.intersect(contentRect);

        if (!offscreenRect.overlaps(windowRect)) {

            // Need to paint checker-board

            gc->push();
            gc->translate(-mScrollPos.x, m_headerHeight-mScrollPos.y);
            gc->translate(windowRect.x(), windowRect.y());

            int pw = mDirtyPattern->width();
            int ph = mDirtyPattern->height();

            int dstX = windowRect.x() % pw - pw;
            int dstY = windowRect.y() % ph - ph;
            int dstR = windowRect.r() % pw + pw + windowRect.w();
            int dstB = windowRect.b() % ph + ph + windowRect.h();

            gc->addClipRect(0, 0, windowRect.w(), windowRect.h());

            gc->drawPattern(mDirtyPattern,
                            windowRect.x() % pw,
                            windowRect.y() % ph,
                            dstX, dstY, dstR, dstB);
            gc->pop();
        }
    }

    gc->translate(-mScrollPos.x, m_headerHeight-mScrollPos.y);

    if (!PrvIsEqual(info->contentZoom, mZoomLevel)) {

        int centerOfSurfX = info->renderedX + info->renderedWidth / 2;
        int centerOfSurfY = info->renderedY + info->renderedHeight / 2;

        float zoomFactor = mZoomLevel / info->contentZoom;

        centerOfSurfX *= zoomFactor;
        centerOfSurfY *= zoomFactor;

        gc->translate(centerOfSurfX, centerOfSurfY);
        gc->scale(zoomFactor, zoomFactor);

        gc->bitblt(offscreenSurf,
                   - info->renderedWidth / 2,
                   - info->renderedHeight / 2,
                   info->renderedWidth / 2,
                   info->renderedHeight / 2);
    }
    else {
        gc->translate(info->renderedX, info->renderedY);
        gc->bitblt(offscreenSurf,
                   0, 0,
                   offscreenSurf->width(),
                   offscreenSurf->height());
    }

    gc->pop();

    if (mScrollableLayerScrollSession.isActive)
        showActiveScrollableLayer((PGContext*)event->graphicsContext);

    if (mShowHighlight)
        showHighlight((PGContext*)event->graphicsContext);

    bool showYScrollbar = false;
    bool showXScrollbar = false;

    showXScrollbar = mContentWidth > (int) mWindow.width;
    showYScrollbar = mContentHeight > (int) mWindow.height;

    if (mScrollbarOpacity > 0 && (showXScrollbar || showYScrollbar) && (m_headerHeight == 0)) {

        const int kScrollbarEdgeMargin = 8;
        const int kScrollbarEdgeWidth  = 3;
        const int kScrollbarMinLength  = 50;

        int xScrollbarLength = (int) (((mWindow.width * 1.0) / mContentWidth) * mWindow.width);
        int xScrollbarPosition = (int) (((mScrollPos.x * 1.0) / mContentWidth) * mWindow.width);
        int yScrollbarLength = (int) (((mWindow.height * 1.0) / mContentHeight) * mWindow.height);
        int yScrollbarPosition = (int) (((mScrollPos.y * 1.0) / mContentHeight) * mWindow.height);

        xScrollbarLength = MAX(xScrollbarLength, kScrollbarMinLength);
        yScrollbarLength = MAX(yScrollbarLength, kScrollbarMinLength);

        gc->push();
        gc->translate(mWindow.x, mWindow.y);
        gc->addClipRect(0, 0, mWindow.width, mWindow.height);

        gc->translate(0, m_headerHeight);

        gc->setStrokeColor(PColor32(0x00, 0x00, 0x00, 0xFF));
        gc->setFillColor(PColor32(0xAA, 0xAA, 0xAA, 0xD0));

        int opacity = MIN(mScrollbarOpacity, 0xFF);
        gc->setFillOpacity(opacity);
        gc->setStrokeOpacity(opacity);
        gc->setStrokeThickness(0.2f);

        if (showXScrollbar)
            gc->drawRect(xScrollbarPosition + kScrollbarEdgeWidth + kScrollbarEdgeMargin,
                         mWindow.height - kScrollbarEdgeMargin - kScrollbarEdgeWidth,
                         xScrollbarPosition + xScrollbarLength - kScrollbarEdgeWidth - kScrollbarEdgeMargin,
                         mWindow.height - kScrollbarEdgeMargin + kScrollbarEdgeWidth);

        if (showYScrollbar)
            gc->drawRect(mWindow.width - kScrollbarEdgeMargin - kScrollbarEdgeWidth,
                         yScrollbarPosition + kScrollbarEdgeWidth + kScrollbarEdgeMargin,
                         mWindow.width - kScrollbarEdgeMargin + kScrollbarEdgeWidth,
                         yScrollbarPosition + yScrollbarLength - kScrollbarEdgeWidth - kScrollbarEdgeMargin);

        gc->pop();

    }

    drawDebugBorder((PGContext*) event->graphicsContext, &mWindow, colorGenericBorder);

    /*FIXME: RR

    #if DEBUG_FLASH_RECTS
    gc->push();
        gc->setStrokeColor(PColor32(0, 0, 0, 0));
        gc->setFillColor(PColor32(255, 0, 0, 60));
        for (RectMap::const_iterator i = mFlashRects.begin();
             i != mFlashRects.end();
             ++i)
        {
            BrowserRect r = i->second;
            gc->drawRect(r.x() * mZoomLevel + mJsScrollX,
                    r.y() * mZoomLevel + mJsScrollY,
                    r.r() * mZoomLevel + mJsScrollX,
                    r.b() * mZoomLevel + mJsScrollY);
        }
        gc->pop();

    if (mSelectionReticle.show) {
      showSelectionReticle((PGContext*)event->graphicsContext);
    }
    #endif
    */
}

void BrowserAdapter::handleWindowChange(NPWindow* window)
{
    TRACEF("BrowserAdapter::handleWindowChange: %ld, %ld\n", mWindow.width, mWindow.height);
    TRACEF("scroll %d,%d", mScrollPos.x, mScrollPos.y);

    m_pageOffset.set(window->x, window->y);

    int oldWindowWidth = mViewportWidth;
    int oldWindowHeight = mViewportHeight;
    double oldZoom = mZoomLevel;

    mViewportWidth = mWindow.width;
    mViewportHeight = mWindow.height;

    asyncCmdSetWindowSize(mViewportWidth, mViewportHeight);
    mScroller->setViewportDimensions(mWindow.width, mWindow.height);

    // If window size changes and adapter is zoom-fitting, need to update the zoomLevel
    if (mZoomFit &&
            oldWindowWidth != mViewportHeight &&
            mWindow.width != 0 &&
            mPageWidth != 0) {

        mZoomLevel = mWindow.width * 1.0 / mPageWidth;

        mContentWidth = ::round(mZoomLevel * mPageWidth);
        mContentHeight = ::round(mZoomLevel * mPageHeight);

        mScroller->setContentDimensions(mContentWidth, mContentHeight + m_headerHeight);
        asyncCmdSetZoomAndScroll(mZoomLevel, mScrollPos.x, mScrollPos.y);
    }

    scrollCaretIntoViewAfterResize(oldWindowWidth, oldWindowHeight, oldZoom);
}

bool BrowserAdapter::handleFocus(bool val)
{
    const char* identifier = (const char*) NPN_GetValue((NPNVariable) npPalmApplicationIdentifier);
    g_message("%s: %p: %s, %d", __PRETTY_FUNCTION__, this, identifier, val);

    mPageFocused = val;

    init();

    asyncCmdPageFocused(val);

    BrowserAdapterManager::instance()->adapterActivated(this, val);

    if (!val)
        mInGestureChange = false;

    startFadeScrollbar();

    return true;
}

/*
Event Handling
*/


bool BrowserAdapter::handlePenDown(NpPalmPenEvent *event)
{
    mFrozen = false;
    init();

    Point eventPt(event->xCoord, event->yCoord - m_headerHeight);

    // mouse hold timer
    m_penDownDoc.set((mScrollPos.x + eventPt.x) / mZoomLevel,
                     (mScrollPos.y + eventPt.y) / mZoomLevel);
    m_dragging = false;

    EVENT_TRACEF("pen down (%d, %d) (%d, %d)\n", eventPt.x, eventPt.y, m_penDownDoc.x, m_penDownDoc.y);

    bool doScroll = true;
    bool passToFlash = false;
    bool containsFlashPoint = flashRectContainsPoint(m_penDownDoc);
    bool containsInteractivePoint = interactiveRectContainsPoint(m_penDownDoc);

    detectScrollableLayerUnderMouseDown(m_penDownDoc, Point(eventPt.x / mZoomLevel, eventPt.y / mZoomLevel));

    if (mFlashGestureLock) {
        passToFlash = containsFlashPoint;
    }

    if (passToFlash || containsInteractivePoint || shouldPassInputEvents()) {
        m_sentToBS = true;
        doScroll = false;
        if (passToFlash) {
            mLastPointSentToFlash = Point(m_penDownDoc.x, m_penDownDoc.y);
        }
        gettimeofday(&m_lastPassedEventTime, NULL);
        asyncCmdMouseEvent(0 /*mousedown*/, m_penDownDoc.x, m_penDownDoc.y, 1);
    }
    else {
        startMouseHoldTimer();
        if (!containsFlashPoint && !containsInteractivePoint) {
            asyncCmdGetInteractiveNodeRects(m_penDownDoc.x, m_penDownDoc.y);
            mShowHighlight = true;
        }
    }

    if (doScroll) {
        mScroller->handleMouseDown(eventPt.x, eventPt.y);
    }
    else
        startFadeScrollbar();

    if (!containsFlashPoint) {
        updateFlashGestureLockStatus(false);
    }

    updateMouseInFlashStatus(containsFlashPoint);
    updateMouseInInteractiveStatus(containsInteractivePoint);

    return true;
}

bool BrowserAdapter::handlePenUp(NpPalmPenEvent *event)
{
    init();

    stopMouseHoldTimer();
    m_didDrag = m_dragging;
    m_dragging = false;
    resetScrollableLayerScrollSession();

    Point eventPt(event->xCoord, event->yCoord - m_headerHeight);
    int cx = (mScrollPos.x + eventPt.x) / mZoomLevel;
    int cy = (mScrollPos.y + eventPt.y) / mZoomLevel;

    int dx = m_penDownDoc.x - cx;
    int dy = m_penDownDoc.y - cy;
    if ((dx * dx + dy * dy) >= kHysteresis * kHysteresis) {
        stopMouseHoldTimer();
    }

    EVENT_TRACEF("pen up (%d, %d) (%d, %d)\n", eventPt.x, eventPt.y, cx, cy);

    removeHighlight();

    bool doScroll = true;
    bool passToFlash = (mMouseInFlashRect && mFlashGestureLock);

    if (passToFlash || mMouseInInteractiveRect || shouldPassInputEvents()) {
        doScroll = false;
        if (passToFlash) {
            mLastPointSentToFlash = Point(-1, -1);
        }
        gettimeofday(&m_lastPassedEventTime, NULL);
        asyncCmdMouseEvent(1 /*mouseup*/, cx, cy, 1);
    }

    if (doScroll) {
        mScroller->handleMouseUp(eventPt.x, eventPt.y);
    }

    updateMouseInFlashStatus(false);
    updateMouseInInteractiveStatus(false);

    return true;
}

bool BrowserAdapter::handlePenMove(NpPalmPenEvent *event)
{
    init();

    removeHighlight();

    m_sentMouseHoldEvent.reset();

    Point eventPt(event->xCoord, event->yCoord - m_headerHeight);
    int cx = (mScrollPos.x + eventPt.x) / mZoomLevel;
    int cy = (mScrollPos.y + eventPt.y) / mZoomLevel;

    int dx = m_penDownDoc.x - cx;
    int dy = m_penDownDoc.y - cy;
    if ((dx * dx + dy * dy) >= kHysteresis * kHysteresis) {
        m_dragging = true;
        stopMouseHoldTimer();
    }

    EVENT_TRACEF("pen move (%d, %d) (%d, %d) interactive %d\n", eventPt.x, eventPt.y, cx, cy, mMouseInInteractiveRect);
    bool doScroll = true;
    bool passToFlash = mMouseInFlashRect && mFlashGestureLock;

    if (passToFlash || mMouseInInteractiveRect || shouldPassInputEvents()) {
        if (passToFlash) {
            mLastPointSentToFlash = Point(cx, cy);
            doScroll = false;
        }
        gettimeofday(&m_lastPassedEventTime, NULL);
        asyncCmdMouseEvent(2 /*mousemove*/, cx, cy, 1);
    }
    else if (mScrollableLayerScrollSession.layer) {

        Point moveToDoc(eventPt.x / mZoomLevel, eventPt.y / mZoomLevel);

        scrollLayerUnderMouse(moveToDoc);
        if (mScrollableLayerScrollSession.isActive) {
            mScroller->handleMouseDown(eventPt.x, eventPt.y);
            doScroll = false;
        }
        else
            resetScrollableLayerScrollSession();
    }

    if (doScroll) {
        mScroller->handleMouseMove(eventPt.x, eventPt.y);
    }

    return true;
}

bool BrowserAdapter::handlePenClick(NpPalmPenEvent *event)
{
    Point eventPt(event->xCoord, event->yCoord - m_headerHeight);
    Point penDocPt((mScrollPos.x + eventPt.x) / mZoomLevel,
                   (mScrollPos.y + eventPt.y) / mZoomLevel);

    EVENT_TRACEF("pen click (%d, %d) (%d, %d) %d %d %d\n", eventPt.x, eventPt.y, penDocPt.x, penDocPt.y, m_didDrag, m_sentToBS, m_didHold);

    if (m_didDrag || m_sentToBS || m_didHold) {
        // squelch the click after drag, hold, or sent to browserserver
        m_didDrag = false;
        m_sentToBS = false;
        m_didHold = false;
        return true;
    }

    // use timer to squelch this click if a double click is coming
    m_clickPt = penDocPt;
    startClickTimer();

    // don't consume this to allow dblclick to arrive
    return false;
}

bool BrowserAdapter::handlePenDoubleClick(NpPalmPenEvent *event)
{
    mShowHighlight = true;

    stopClickTimer();

    Point eventPt(event->xCoord, event->yCoord - m_headerHeight);
    Point penDocPt((mScrollPos.x + eventPt.x) / mZoomLevel,
                   (mScrollPos.y + eventPt.y) / mZoomLevel);

    EVENT_TRACEF("pen double click (%d, %d) (%d, %d)\n", eventPt.x, eventPt.y, penDocPt.x, penDocPt.y);

    bool inFlashRect = flashRectContainsPoint(penDocPt);
    if (flashGestureLock()) {
        if (!inFlashRect) {
            updateFlashGestureLockStatus(false);
        } else {
            // ignore; taps will be sent through with handle* methods
            return true;
        }
    }
    else if (inFlashRect) {
        updateFlashGestureLockStatus(true);
    }

    prvSmartZoom(penDocPt);

    return true;
}

bool BrowserAdapter::handleKeyDown(NpPalmKeyEvent *event)
{
    EVENT_TRACEF("KeyDown %d/0x%08x\n", event->rawkeyCode, event->rawModifier);
    asyncCmdKeyDown(event->rawkeyCode, event->rawModifier);
    return event->rawkeyCode != ESC_KEY && bEditorFocused;
}

bool BrowserAdapter::handleKeyUp(NpPalmKeyEvent *event)
{
    EVENT_TRACEF("KeyUp %d/0x%08x\n", event->rawkeyCode, event->rawModifier);
    asyncCmdKeyUp(event->rawkeyCode, event->rawModifier);
    return  event->rawkeyCode != ESC_KEY && bEditorFocused;
}

bool BrowserAdapter::handleTouchStart(NpPalmTouchEvent *event)
{
    return doTouchEvent(0, event);
}

bool BrowserAdapter::handleTouchMove(NpPalmTouchEvent *event)
{
    return doTouchEvent(1, event);
}

bool BrowserAdapter::handleTouchEnd(NpPalmTouchEvent *event)
{
    return doTouchEvent(2, event);
}

bool BrowserAdapter::handleTouchCancelled(NpPalmTouchEvent *event)
{
    return doTouchEvent(3, event);
}

bool BrowserAdapter::doTouchEvent(int32_t type, NpPalmTouchEvent *event)
{
    if (shouldPassTouchEvents()) {
        pbnjson::JValue arr = pbnjson::Array();
        for (int ix = 0; ix < event->touches.length; ix++) {
            Point touchPoint (event->touches.points[ix].xCoord, event->touches.points[ix].yCoord);
            Palm::TouchPointPalm::State touchState = Palm::TouchPointPalm::TouchStationary;

            if (isPointInList(touchPoint, event->changedTouches)) {
                if (type == 0) // TouchStart
                    touchState = Palm::TouchPointPalm::TouchPressed;
                else if (type == 1) // TouchMove
                    touchState = Palm::TouchPointPalm::TouchMoved;
            }

            Point eventPt(touchPoint.x - m_pageOffset.x, touchPoint.y - (m_pageOffset.y + m_headerHeight));
            Point touchDocPt((mScrollPos.x + eventPt.x) / mZoomLevel,
                             (mScrollPos.y + eventPt.y) / mZoomLevel);

            m_touchPtDoc = touchDocPt; // Save this point in case we need it for a TouchReleased event

            pbnjson::JValue obj = pbnjson::Object();
            obj.put("x", touchDocPt.x);
            obj.put("y", touchDocPt.y);
            obj.put("state", (int)touchState);
            arr.append(obj);
        }

        // released or cancelled touches are not in the event->touches list
        // so we need to add them from the event->changedTouches list
        if (type == 2 || type == 3) { // TouchEnd or TouchCancelled
            Palm::TouchPointPalm::State touchState = type == 2 ? Palm::TouchPointPalm::TouchReleased : Palm::TouchPointPalm::TouchCancelled;
            for (int ix = 0; ix < event->changedTouches.length; ix++) {
                Point touchPoint (event->touches.points[ix].xCoord, event->touches.points[ix].yCoord);
                Point eventPt(touchPoint.x - m_pageOffset.x, touchPoint.y - (m_pageOffset.y + m_headerHeight));
                Point touchDocPt((mScrollPos.x + eventPt.x) / mZoomLevel,
                                 (mScrollPos.y + eventPt.y) / mZoomLevel);

                pbnjson::JValue obj = pbnjson::Object();
                if ((touchDocPt.x < 0 || touchDocPt.x > mContentWidth)
                        || (touchDocPt.y < 0 || touchDocPt.y > mContentHeight)) {
                    // In testing, it was observed that, for a TouchReleased event, the x and y
                    // coordinates where absurdly high values in excess of 7,000,000.  Therefore,
                    // if the x and y values are out of range, just set them to the saved value
                    // in m_touchPtDoc which is more likely to be the point that the user actually
                    // released their touch of the screen.
                    touchDocPt = m_touchPtDoc;
                }
                obj.put("x", touchDocPt.x);
                obj.put("y", touchDocPt.y);
                obj.put("state", (int)touchState);
                arr.append(obj);
            }
        }

        pbnjson::JGenerator ser(NULL);
        pbnjson::JSchemaFragment schema("{}");
        std::string json;
        if (!ser.toString(arr, schema, json)) {
            syslog(LOG_DEBUG, "error generating json, dropping event");
        } else {
            EVENT_TRACEF("touches: %d, json: %s", arr.arraySize(), json.c_str());
            asyncCmdTouchEvent(type, arr.arraySize(), event->modifiers, json.c_str());
        }
    }
    return true;
}

bool BrowserAdapter::isPointInList(const Point &point, const NpPalmTouchList &list)
{
    bool retVal = false;
    for (int ix = 0; ix < list.length; ix++) {
        if (point.x == list.points[ix].xCoord && point.y == list.points[ix].yCoord) {
            retVal = true;
            break;
        }
    }
    return retVal;
}

bool BrowserAdapter::handleGesture(NpPalmGestureEvent *event)
{
    //EVENT_TRACEF("window (%ld,%ld)\n", mWindow.x, mWindow.y);
    EVENT_TRACEF("type %d (%d,%d) c (%d,%d) scale %f rot %f mod 0x%08x",
                 event->type, event->x, event->y, event->center_x, event->center_y,
                 event->scale, event->rotate, event->modifiers);

    mShowHighlight = false;
    stopMouseHoldTimer();

    // do not handle gestures in page to page transition (page size is zero)
    if (mPageWidth == 0 || mPageHeight == 0)
        return true;

    bool passGestureEvents = m_passInputEvents || (mMetaViewport && !mMetaViewport->userScalable);

    switch (event->type) {
    case npPalmGestureStartEvent:
        if (passGestureEvents)
            sendGestureStart(event->x, event->y,
                             event->scale, event->rotate,
                             event->center_x, event->center_y);
        else
            doGestureStart(event->x, event->y,
                           event->scale, event->rotate,
                           event->center_x, event->center_y);
        break;
    case npPalmGestureChangeEvent:
        if (passGestureEvents)
            sendGestureChange(event->x, event->y,
                              event->scale, event->rotate,
                              event->center_x, event->center_y);
        else
            doGestureChange(event->x, event->y,
                            event->scale, event->rotate,
                            event->center_x, event->center_y);
        break;
    case npPalmGestureEndEvent:
        if (passGestureEvents)
            sendGestureEnd(event->x, event->y,
                           event->scale, event->rotate,
                           event->center_x, event->center_y);
        else
            doGestureEnd(event->x, event->y,
                         event->scale, event->rotate,
                         event->center_x, event->center_y);
        break;
    case npPalmGestureSingleTapEvent:
        //sendSingleTap(event->x, event->y, event->modifiers);
        break;
    }

    return true;
}

void BrowserAdapter::doGestureStart(int cx, int cy, float scale, float rotate,
                                    int centerX, int centerY)
{
    cy -= m_headerHeight;
    centerY -= m_headerHeight;

    m_recordedGestures.clear();
    recordGesture(scale, rotate, centerX, centerY);

    // All gestures (scrolls, pinches, etc.) should hide the spelling widget.
    asyncCmdHideSpellingWidget();

    // break out of flash gesture lock if necessary
    updateFlashGestureLockStatus(false);

    if (!mCenteredZoom)
        mCenteredZoom = new BrowserCenteredZoom;

    mCenteredZoom->scale = scale;
    mCenteredZoom->centerX = centerX;
    mCenteredZoom->centerY = centerY;
    mCenteredZoom->scrollX = mScrollPos.x;
    mCenteredZoom->scrollY = mScrollPos.y;
    mCenteredZoom->zoomLevel = mZoomLevel;

    mZoomFit = false;
    mInGestureChange = true;
}

void BrowserAdapter::doGestureChange(int cx, int cy, float scale, float rotate,
                                     int centerX, int centerY, bool isGestureEnd)
{
    mInGestureChange = true;

    // Scale down the scale value to create a sense of physical resistance
    scale = 1.0f + (scale - 1.0f) * 0.6f;

    // round to two decimal places to reduce jitter
    scale = ::round(scale * 100) / 100;

    if (!mCenteredZoom) {
        doGestureStart(cx, cy, scale, rotate, centerX, centerY);
    }

    recordGesture(scale, rotate, centerX, centerY);
    RecordedGestureEntry avgGesture = getAveragedGesture();
    scale = avgGesture.scale;
    rotate = avgGesture.rotate;
    centerX = avgGesture.centerX;
    centerY = avgGesture.centerY;

    cy -= m_headerHeight;
    centerY -= m_headerHeight;

    mZoomLevel = mCenteredZoom->zoomLevel * scale;

    double maxZoom = mMetaViewport ? mMetaViewport->maximumScale : kMaxZoom;
    double minZoom = mMetaViewport ? mMetaViewport->minimumScale : 0.0;
    double fitZoom = mWindow.width * 1.0 / mPageWidth;
    minZoom = MAX(minZoom, fitZoom);

    if (!isGestureEnd) {

        // Friction for overzooming in either direction.
        const double kDamping = 0.2;

        if (mZoomLevel > maxZoom)
            mZoomLevel = maxZoom + kDamping * (mZoomLevel - maxZoom);

        if (mZoomLevel < minZoom)
            mZoomLevel = minZoom - kDamping * (minZoom - mZoomLevel);
    }
    else {
        if (mZoomLevel > maxZoom)
            mZoomLevel = maxZoom;

        if (mZoomLevel < minZoom)
            mZoomLevel = minZoom;
    }

    scale = mZoomLevel / mCenteredZoom->zoomLevel;

    // this is the offset after scaling
    double x = (scale - 1) * mCenteredZoom->centerX;

    // add the scaled scroll offset
    x += scale * mCenteredZoom->scrollX;

    // now account for the moving center
    x += mCenteredZoom->centerX - centerX;

    // do the y direction
    double y = (scale - 1) * mCenteredZoom->centerY +
               scale * mCenteredZoom->scrollY +
               mCenteredZoom->centerY - centerY;

    mZoomFit = false;

    mContentWidth = ::round(mZoomLevel * mPageWidth);
    mContentHeight = ::round(mZoomLevel * mPageHeight);
    mScrollPos.x = ::round(x);
    mScrollPos.y = ::round(y);

    mScroller->setContentDimensions(mContentWidth, mContentHeight + m_headerHeight);
    mScroller->scrollTo(-mScrollPos.x, -mScrollPos.y, false);
    invalidate();
    fireScrolledToEvent(-mScrollPos.x, -mScrollPos.y);
}

void BrowserAdapter::doGestureEnd(int cx, int cy, float scale, float rotate,
                                  int centerX, int centerY)
{
    doGestureChange(cx, cy, scale, rotate, centerX, centerY, true);

    cy -= m_headerHeight;
    centerY -= m_headerHeight;

    mInGestureChange = false;

    delete mCenteredZoom;
    mCenteredZoom = 0;

    if (mPageWidth) {
        double fitZoom = mWindow.width * 1.0 / mPageWidth;
        if (PrvIsEqual(mZoomLevel, fitZoom))
            mZoomFit = true;
    }

    asyncCmdSetZoomAndScroll(mZoomLevel, mScrollPos.x, mScrollPos.y);
}

void BrowserAdapter::sendGestureStart(int cx, int cy, float scale, float rotate,
                                      int center_x, int center_y)
{
    // All gestures (scrolls, pinches, etc.) should hide the spelling widget.
    asyncCmdHideSpellingWidget();

    // break out of flash gesture lock if necessary
    updateFlashGestureLockStatus(false);

    // do not start pinch zoom in page to page transition (page size is zero)
    if (mPageWidth == 0 || mPageHeight == 0) {
        return;
    }

    if (m_passInputEvents || m_spotlightHandle) {
        TRACEF("x: %d, y: %d, scale: %f, rotate: %f, centerX: %d, centerY: %d",
               cx, cy, scale, rotate, center_x, center_y);
        asyncCmdGestureEvent(0,  // gesture start
                             cx, cy, scale, rotate, center_x, center_y);
    }
}

void BrowserAdapter::sendGestureChange(int cx, int cy, float scale, float rotate,
                                       int center_x, int center_y)
{
    if (m_passInputEvents || m_spotlightHandle) {
        TRACEF("x: %d, y: %d, scale: %f, rotate: %f, centerX: %d, centerY: %d",
               cx, cy, scale, rotate, center_x, center_y);
        asyncCmdGestureEvent(1,  // gesture change
                             cx, cy, scale, rotate, center_x, center_y);
    }
}

void BrowserAdapter::sendGestureEnd(int cx, int cy, float scale, float rotate,
                                    int center_x, int center_y)
{
    if (m_passInputEvents || m_spotlightHandle) {
        TRACEF("x: %d, y: %d, scale: %f, rotate: %f, centerX: %d, centerY: %d",
               cx, cy, scale, rotate, center_x, center_y);
        asyncCmdGestureEvent(2,  // gesture end
                             cx, cy, scale, rotate, center_x, center_y);
    }
}

void BrowserAdapter::sendSingleTap(int cx, int cy, int modifiers)
{
    Point docPt((cx + mScrollPos.x) / mZoomLevel,
                (cy + mScrollPos.y) / mZoomLevel);

    if (mBrowserServerConnected) {
        if (flashRectContainsPoint(docPt)) {
            // single taps for flash go through if we're not in flash lock mode
            if (!flashGestureLock()) {
                asyncCmdMouseEvent(0 /*mousedown*/, docPt.x, docPt.y, 1);
                asyncCmdMouseEvent(1 /*mouseup*/, docPt.x, docPt.y, 1);
            }
        } else {
            int q = mBsQueryNum++;
            HitTestArgs *callArgs = new HitTestArgs("click", docPt, modifiers, q);
            m_hitTestArgs.add(callArgs);
            TRACEF("x: %d, y: %d, q: %d", docPt.x, docPt.y, q);
            asyncCmdHitTest(q, docPt.x, docPt.y);
        }
    }
}

const char* BrowserAdapter::js_openURL(AdapterBase *adapter, const NPVariant *args, uint32_t argCount, NPVariant *result)
{
    if(argCount != 1 || NPVARIANT_IS_STRING(args[0]) == false)
    {
        return "BrowserAdapter::OpenURL(string): Bad arguments.";
    }

    BrowserAdapter *a = GetAndInitAdapter(adapter);
    if (a->mBrowserServerConnected) {
        char *url = NPStringToString(NPVARIANT_TO_STRING(args[0]));
        TRACEF("%s\n", url);
        a->asyncCmdOpenUrl(url);
        ::free(url);

        if (a->mFrozenSurface) {
            a->mFrozenSurface->releaseRef();
            a->mFrozenSurface = 0;
        }

        return NULL;
    }
    else {
        return "BrowserAdapter::openURL: Disconnected from server";
    }
}

const char* BrowserAdapter::js_setPageIdentifier(AdapterBase *adapter, const NPVariant *args, uint32_t argCount, NPVariant *result)
{
    if(argCount != 1 || !NPVARIANT_IS_STRING(args[0]))
    {
        INT32_TO_NPVARIANT( kInvalidParam, *result );
        return "BrowserAdapter::setPageIdentifier(string): Bad arguments.";
    }

    char *idStr = NPStringToString(NPVARIANT_TO_STRING(args[0]));
    int32_t identifier = atoi(idStr);

    TRACEF("%s\n", idStr);

    ::free(idStr);

    BrowserAdapter* pAdapter = static_cast<BrowserAdapter*>(adapter);
    pAdapter->setPageIdentifier(identifier);

    return NULL;
}

const char* BrowserAdapter::js_pageScaleAndScroll(AdapterBase *adapter, const NPVariant *args, uint32_t argCount, NPVariant *result)
{
    if( argCount < 3 )
    {
        return "BrowserAdapter::pageScaleAndScroll(number, number, number): Bad Arguments";
    }

    double scaleFactor = VariantToDouble(args[0]);
    int32_t scroll_x = VariantToInteger(args[1]);
    int32_t scroll_y = VariantToInteger(args[2]);

    TRACEF("%f, %d, %d\n", scaleFactor, scroll_x, scroll_y);

    BrowserAdapter *a = GetAndInitAdapter(adapter);
    a->scaleAndScrollTo(scaleFactor, scroll_x, scroll_y);

    return NULL;
}

// FIXME: Remove?
const char* BrowserAdapter::js_setMagnification(AdapterBase *adapter, const NPVariant *args, uint32_t argCount, NPVariant *result)
{
    if( argCount < 1 )
    {
        return "BrowserAdapter::setMagnification(number): Bad Arguments";
    }

    TRACEF("%f\n", VariantToDouble(args[0]));

    BrowserAdapter *a = GetAndInitAdapter(adapter);
    a->scale( VariantToDouble(args[0]) );

    return 0;
}

const char* BrowserAdapter::js_goBack(AdapterBase *adapter, const NPVariant *args, uint32_t argCount, NPVariant *result)
{
    if(argCount != 0) {
        return "BrowserAdapter::GoBack(): Bad arguments, expected none.";
    }
    TRACE;
    BrowserAdapter *proxy = GetAndInitAdapter(adapter);
    proxy->asyncCmdBack();
    return NULL;
}

const char* BrowserAdapter::js_goForward(AdapterBase *adapter, const NPVariant *args, uint32_t argCount, NPVariant *result)
{
    if(argCount != 0) {
        return "BrowserAdapter::GoForward(): Bad arguments, expected none.";
    }
    TRACE;
    BrowserAdapter *proxy = GetAndInitAdapter(adapter);
    proxy->asyncCmdForward();
    return NULL;
}

const char* BrowserAdapter::js_clearHistory(AdapterBase *adapter, const NPVariant *args, uint32_t argCount, NPVariant *result)
{
    if(argCount != 0) {
        return "BrowserAdapter::clearHistory(): Bad arguments, expected none.";
    }

    TRACE;

    BrowserAdapter *proxy = GetAndInitAdapter(adapter);
    if (NULL != proxy) {
        proxy->asyncCmdClearHistory();
    }

    return NULL;
}

const char* BrowserAdapter::js_reloadPage(AdapterBase *adapter, const NPVariant *args, uint32_t argCount, NPVariant *result)
{
    if(argCount != 0) {
        return "BrowserAdapter::ReloadPage(): Bad arguments, expected none.";
    }
    TRACE;
    BrowserAdapter *proxy = GetAndInitAdapter(adapter);
    proxy->asyncCmdReload();
    return NULL;
}

const char* BrowserAdapter::js_stopLoad(AdapterBase *adapter, const NPVariant *args, uint32_t argCount, NPVariant *result)
{
    if(argCount != 0) {
        return "BrowserAdapter::StopLoad(): Bad arguments, expected none.";
    }
    TRACE;
    BrowserAdapter *proxy = GetAndInitAdapter(adapter);
    proxy->asyncCmdStop();
    return NULL;
}

const char* BrowserAdapter::js_addUrlRedirect(AdapterBase *adapterBase, const NPVariant *args, uint32_t argCount, NPVariant *result)
{
    if (argCount != 4
            || !NPVARIANT_IS_STRING(args[0])
            || !IsBooleanVariant(args[1])
            || !NPVARIANT_IS_STRING(args[2])
            || !IsIntegerVariant(&args[3])) {
        return "addUrlRedirect: invalid args (string, bool, string, int)";
    }

    int type = VariantToInteger(&args[3]);
    if (type < 0 || type > 1) {
        return "addUrlRedirect: incorrect type (0 or 1)";
    }
    char* re = NPStringToString(NPVARIANT_TO_STRING(args[0]));
    char* userData = NPStringToString(NPVARIANT_TO_STRING(args[2]));

    TRACEF("Redirecting '%s' -> '%s'\n", re, userData);
    BrowserAdapter *adapter = GetAndInitAdapter(adapterBase);
    regex_t reg = {0};
    if (regcomp(&reg, re, REG_EXTENDED | REG_ICASE | REG_NOSUB) != 0) {
        char* msg(NULL);
        if ( -1 != ::asprintf(&msg, "addUrlRedirect: Can't compile RE '%s'", re) ) {
            adapter->ThrowException(msg);
            free(msg);
        }
        else {
            adapter->ThrowException("Can't compile RE");
        }
    }
    else {
        regfree(&reg);
        adapter->asyncCmdAddUrlRedirect(re, type, NPVARIANT_TO_BOOLEAN(args[1]), userData);

        // Save for later so that if we reconnect to the server we can resend our redirect info.
        adapter->m_urlRedirects.push_back( new UrlRedirectInfo(re, NPVARIANT_TO_BOOLEAN(args[1]), userData, type) );
    }

    free(re);
    free(userData);

    return NULL;
}

const char* BrowserAdapter::js_pageFocused(AdapterBase *adapter, const NPVariant *args, uint32_t argCount, NPVariant *result)
{
    return NULL;
}

// deprecated: remove me after change widget_webview.js
const char* BrowserAdapter::js_startDeferWindowChange(AdapterBase *adapter, const NPVariant *args, uint32_t argCount, NPVariant *result)
{
    return NULL;
}

// deprecated: remove me after change widget_webview.js
const char* BrowserAdapter::js_stopDeferWindowChange(AdapterBase *adapter, const NPVariant *args, uint32_t argCount, NPVariant *result)
{
    return NULL;
}


const char* BrowserAdapter::js_setSpotlight(AdapterBase *adapter, const NPVariant *args, uint32_t argCount, NPVariant *result)
{
    BrowserAdapter *proxy = GetAndInitAdapter(adapter);

    if (argCount != 5 && argCount != 6) {
        return "BrowserAdapter::js_setSpotlight(): Bad arguments";
    }
    int left = VariantToInteger(args[0]);
    int top = VariantToInteger(args[1]);
    int right = VariantToInteger(args[2]);
    int bottom = VariantToInteger(args[3]);
    int handle = VariantToInteger(args[4]);

    TRACEF("%d, %d, %d, %d, %d\n", left, top, right, bottom, handle);

    if (handle == 0) {
        return "BrowserAdapter::js_setSpotlight(): Invalid parameter spotlight handle\n";
    }

    if (argCount == 6) {
        proxy->m_spotlightAlpha = (int)NPVARIANT_TO_INT32(args[5]);

    } else {
        // scrim around spotlight is opaque if no alpha channel argument is provided
        proxy->m_spotlightAlpha = 0xff;
    }

    if (handle != proxy->m_spotlightHandle) {
        proxy->m_spotlightHandle = handle;
    }

    proxy->m_spotlightRect.setX(left);
    proxy->m_spotlightRect.setY(top);
    proxy->m_spotlightRect.setWidth((right - left));
    proxy->m_spotlightRect.setHeight((bottom - top));

    proxy->invalidate();

    // notify BS/plugin
    proxy->asyncCmdPluginSpotlightStart(left, top, right-left, bottom-top);

    return NULL;
}

const char* BrowserAdapter::js_removeSpotlight(AdapterBase *adapter, const NPVariant *args, uint32_t argCount, NPVariant *result)
{
    BrowserAdapter *proxy = GetAndInitAdapter(adapter);

    if (argCount != 0) {
        return "BrowserAdapter::js_removeSpotlight(): Bad arguments";
    }

    TRACE;

    if (proxy->m_spotlightHandle) {
        proxy->m_spotlightHandle = 0;
        proxy->invalidate();
    }

    // notify BS/plugin
    proxy->asyncCmdPluginSpotlightEnd();

    return NULL;
}

const char* BrowserAdapter::js_setHTML(AdapterBase *adapter, const NPVariant *args, uint32_t argCount, NPVariant *result)
{
    BrowserAdapter *proxy = GetAndInitAdapter(adapter);

    if (argCount != 2
            ||  !NPVARIANT_IS_STRING(args[0])
            || !NPVARIANT_IS_STRING(args[1]))
    {
        return "BrowserAdapter::js_setHTML(): Bad arguments";
    }
    TRACE;
    char* arg0 =NPStringToString(NPVARIANT_TO_STRING(args[0]));
    char* arg1 =NPStringToString(NPVARIANT_TO_STRING(args[1]));
    proxy->asyncCmdSetHtml(arg0,arg1);

    if (proxy->mFrozenSurface) {
        proxy->mFrozenSurface->releaseRef();
        proxy->mFrozenSurface = 0;
    }

    ::free(arg0);
    ::free(arg1);
    return NULL;
}
const char* BrowserAdapter::js_disableEnhancedViewport(AdapterBase *adapter, const NPVariant *args, uint32_t argCount, NPVariant *result)
{
    BrowserAdapter *proxy = GetAndInitAdapter(adapter);
    if (argCount != 1
            ||  !IsBooleanVariant(args[0]))
    {
        return "BrowserAdapter::js_disableEnhancedViewport(): Bad arguments";
    }

    bool disable = VariantToBoolean(args[0]);
    TRACEF("%d\n", disable);
    proxy->asyncCmdDisableEnhancedViewport(disable);
    return NULL;
}
/**
 * See if there is a URL at a certain point and return information about it.
 */
const char* BrowserAdapter::js_inspectUrlAtPoint(AdapterBase *adapter, const NPVariant *args, uint32_t argCount, NPVariant *result)
{
    if ( argCount != 3
            || !IsIntegerVariant(args[0])
            || !IsIntegerVariant(args[1])
            || !NPVARIANT_IS_OBJECT(args[2])
       ) {
        INT32_TO_NPVARIANT( kInvalidParam, *result );
        return "BrowserAdapter::inspectUrlAtPoint(x, y, cb): Bad arguments.";
    }

    BrowserAdapter *proxy = GetAndInitAdapter(adapter);
    if (proxy->mBrowserServerConnected) {

        int x = VariantToInteger(args[0]);
        int y = VariantToInteger(args[1]);
        int nQueryNum = proxy->mBsQueryNum++;
        InspectUrlAtPointArgs*  inspectArgs = new InspectUrlAtPointArgs(x, y,
                NPVARIANT_TO_OBJECT(args[2]), nQueryNum);

        TRACEF("%d, %d, %f\n", x, y, proxy->mZoomLevel);

        x = (x + proxy->mScrollPos.x) / proxy->mZoomLevel;
        y = (y + proxy->mScrollPos.y) / proxy->mZoomLevel;

        proxy->m_inspectUrlArgs.add(inspectArgs);
        proxy->asyncCmdInspectUrlAtPoint(nQueryNum, x, y);
        return NULL;
    }
    else {
        return "Not connected to server";
    }
}

const char* BrowserAdapter::js_getImageInfoAtPoint(AdapterBase *adapter, const NPVariant *args, uint32_t argCount, NPVariant *result)
{
    if ( argCount != 3
            || !IsIntegerVariant(args[0])
            || !IsIntegerVariant(args[1])
            || !NPVARIANT_IS_OBJECT(args[2])
       ) {
        INT32_TO_NPVARIANT( kInvalidParam, *result );
        return "BrowserAdapter::getImageInfoAtPoint(x, y, cb): Bad arguments.";
    }

    BrowserAdapter *proxy = GetAndInitAdapter(adapter);
    if (proxy->mBrowserServerConnected) {

        int x = VariantToInteger(args[0]);
        int y = VariantToInteger(args[1]);
        int nQueryNum = proxy->mBsQueryNum++;
        GetImageInfoAtPointArgs*  callArgs = new GetImageInfoAtPointArgs(x, y,
                NPVARIANT_TO_OBJECT(args[2]), nQueryNum);

        TRACEF("%d, %d, %f\n", x, y, proxy->mZoomLevel);

        x = (x + proxy->mScrollPos.x) / proxy->mZoomLevel;
        y = (y + proxy->mScrollPos.y) / proxy->mZoomLevel;

        proxy->m_getImageInfoAtPointArgs.add(callArgs);
        proxy->asyncCmdGetImageInfoAtPoint(nQueryNum, x, y);
        return NULL;
    }
    else {
        return "Not connected to server";
    }
}

const char* BrowserAdapter::js_getElementInfoAtPoint(AdapterBase *adapter, const NPVariant *args, uint32_t argCount, NPVariant *result)
{
    if ( argCount != 3
            || !IsIntegerVariant(args[0])
            || !IsIntegerVariant(args[1])
            || !NPVARIANT_IS_OBJECT(args[2])
       ) {
        INT32_TO_NPVARIANT( kInvalidParam, *result );
        return "BrowserAdapter::getElementInfoAtPoint(x, y, cb): Bad arguments.";
    }

    BrowserAdapter *proxy = GetAndInitAdapter(adapter);
    if (proxy->mBrowserServerConnected) {

        int x = VariantToInteger(args[0]);
        int y = VariantToInteger(args[1]);
        int nQueryNum = proxy->mBsQueryNum++;
        GetElementInfoAtPointArgs*  callArgs = new GetElementInfoAtPointArgs(x, y,
                NPVARIANT_TO_OBJECT(args[2]), nQueryNum);

        TRACEF("%d, %d, %f\n", x, y, proxy->mZoomLevel);

        x = (x + proxy->mScrollPos.x) / proxy->mZoomLevel;
        y = (y + proxy->mScrollPos.y) / proxy->mZoomLevel;

        proxy->m_getElementInfoAtPointArgs.add(callArgs);
        proxy->asyncCmdGetElementInfoAtPoint(nQueryNum, x, y);
        return NULL;
    }
    else {
        return "Not connected to server";
    }
}

const char* BrowserAdapter::js_ignoreMetaTags(AdapterBase *adapter, const NPVariant *args, uint32_t argCount, NPVariant *result)
{
    TRACE;
    BrowserAdapter *proxy = GetAndInitAdapter(adapter);
    if (argCount != 1
            ||  !IsBooleanVariant(args[0]))
    {
        return "BrowserAdapter::js_disableEnhancedViewport(): Bad arguments";
    }
    bool ignore = VariantToBoolean(args[0]);
    if (proxy->mBrowserServerConnected) {

        proxy->asyncCmdIgnoreMetaTags(ignore);
        return NULL;
    }
    else {
        return "Not connected to server";
    }
}

const char* BrowserAdapter::js_setNetworkInterface(AdapterBase *adapter, const NPVariant *args, uint32_t argCount, NPVariant *result)
{
    TRACE;
    BrowserAdapter *proxy = GetAndInitAdapter(adapter);
    if (argCount != 1
            ||  !NPVARIANT_IS_STRING(args[0]))
    {
        return "BrowserAdapter::js_setNetworkInterface(): Bad arguments";
    }
    char* interfaceName =NPStringToString(NPVARIANT_TO_STRING(args[0]));

    proxy->asyncCmdSetNetworkInterface(interfaceName);

    ::free(interfaceName);

    return NULL;

}

const char* BrowserAdapter::js_enableFastScaling(AdapterBase *adapter,
        const NPVariant *args, uint32_t argCount, NPVariant *result)
{
    TRACE;
    BrowserAdapter *proxy = GetAndInitAdapter(adapter);
    if ((argCount != 1) || !IsBooleanVariant(args[0])) {
        return "BrowserAdapter::js_enableFastScaling(): Bad arguments";
    }

    bool enable = VariantToBoolean(args[0]);
    proxy->enableFastScaling(enable);
    return NULL;
}

const char* BrowserAdapter::js_setDefaultLayoutWidth(AdapterBase *adapter,
        const NPVariant *args, uint32_t argCount, NPVariant *result)
{
    if ((argCount == 1 && !IsIntegerVariant(args[0]))
            || (argCount == 2 && (!IsIntegerVariant(args[0]) || !IsIntegerVariant(args[1]))) || argCount > 2) {
        return "BrowserAdapter::setDefaultLayoutWidth(int): Bad arguments.";
    }

    BrowserAdapter *a = GetAndInitAdapter(adapter);
    if (!a->mBrowserServerConnected) {
        return "Not connected to server.";
    }

    int layoutWidth = VariantToInteger(args[0]);
    int layoutHeight = 0;
    if (argCount == 2) {
        layoutHeight = VariantToInteger(args[1]);
    }
    else {
        layoutHeight = layoutWidth / a->mViewportWidth * a->mViewportHeight;
    }

    //TRACEF("%d -> %d", a->m_defaultLayoutWidth, layoutWidth);

    //if (layoutWidth != a->m_defaultLayoutWidth) {
    a->m_defaultLayoutWidth = layoutWidth;
    TRACEF("%d -> %dx%d", a->m_defaultLayoutWidth, layoutWidth, layoutHeight);
    a->asyncCmdSetVirtualWindowSize(layoutWidth, layoutHeight);
    //}

    return NULL;
}

void
BrowserAdapter::msgGetImageInfoAtPointResponse(int32_t queryNum, bool succeeded, const char* baseUri, const char* src,
        const char* title, const char* altText, int32_t width, int32_t height, const char* mimeType)
{
    TRACEF("%c, '%s'", succeeded ? 'T' : 'F', baseUri);
    std::auto_ptr<GetImageInfoAtPointArgs> args(m_getImageInfoAtPointArgs.get(queryNum));
    if (!args.get()) {
        TRACEF("Can't find response for this call.");
        return;
    }

    NPVariant jsCallResult, jsCallArgs;
    NPObject* info = NPN_CreateObject(&ImageInfo::sImageInfoClass);
    if (info) {
        OBJECT_TO_NPVARIANT(info, jsCallArgs);
        static_cast<ImageInfo*>(info)->initialize(succeeded, baseUri, src, title, altText, width, height, mimeType);
        if (NPN_InvokeDefault(args->m_callback, &jsCallArgs, 1, &jsCallResult))
            TRACEF("getImageInfoAtPoint response call success.");
        else
            TRACEF("getImageInfoAtPoint response call FAILED.");

        AdapterBase::NPN_ReleaseVariantValue(&jsCallArgs);
    }
    else {
        TRACEF("getImageInfoAtPoint: response obj NOT created.");
    }
}

const char* BrowserAdapter::js_interactiveAtPoint(AdapterBase *adapter, const NPVariant *args, uint32_t argCount, NPVariant *result)
{
    if ( argCount != 3
            || !IsIntegerVariant(args[0])
            || !IsIntegerVariant(args[1])
            || !NPVARIANT_IS_OBJECT(args[2])
       ) {
        INT32_TO_NPVARIANT( kInvalidParam, *result );
        return "BrowserAdapter::isInteractiveAtPoint(x, y, cb): Bad arguments.";
    }

    BrowserAdapter *proxy = GetAndInitAdapter(adapter);
    if (proxy->mBrowserServerConnected) {

        int x = VariantToInteger(args[0]);
        int y = VariantToInteger(args[1]);
        syslog(LOG_DEBUG, "isInteractiveAtPoint: (%d, %d)\n", x, y);
        int nQueryNum = proxy->mBsQueryNum++;
        IsInteractiveAtPointArgs*  callArgs = new IsInteractiveAtPointArgs(x, y,
                NPVARIANT_TO_OBJECT(args[2]), nQueryNum);

        TRACEF("%d, %d, %f\n", x, y, proxy->mZoomLevel);

        x = (x + proxy->mScrollPos.x) / proxy->mZoomLevel;
        y = (y + proxy->mScrollPos.y) / proxy->mZoomLevel;

        proxy->m_isInteractiveAtPointArgs.add(callArgs);
        proxy->asyncCmdIsInteractiveAtPoint(nQueryNum, x, y);
        return NULL;
    }
    else {
        return "Not connected to server";
    }
}

void BrowserAdapter::msgIsInteractiveAtPointResponse(int32_t queryNum, bool interactive)
{
    TRACEF("'%c'", interactive ? 'T' : 'F');
    std::auto_ptr<IsInteractiveAtPointArgs> args(m_isInteractiveAtPointArgs.get(queryNum));
    if (!args.get()) {
        TRACEF("Can't find response for this call.");
        return;
    }

    NPVariant jsCallResult, jsCallArgs;
    NPObject* info = NPN_CreateObject(&InteractiveInfo::sInteractiveInfoClass);
    if (info) {
        OBJECT_TO_NPVARIANT(info, jsCallArgs);
        static_cast<InteractiveInfo*>(info)->initialize(interactive, args->m_x, args->m_y);
        if (NPN_InvokeDefault(args->m_callback, &jsCallArgs, 1, &jsCallResult))
            TRACEF("isInteractiveAtPoint response call success.");
        else
            TRACEF("isInteractiveAtPoint response call FAILED.");

        AdapterBase::NPN_ReleaseVariantValue(&jsCallArgs);
    }
    else {
        TRACEF("isInteractiveAtPoint: response obj NOT created.");
    }
}

/**
 * Get the state (canGoBack/canGoForward) for the session history.
 */
const char* BrowserAdapter::js_getHistoryState(AdapterBase *adapter, const NPVariant *args, uint32_t argCount, NPVariant *result)
{
    if ( argCount != 1 || !NPVARIANT_IS_OBJECT(args[0])
       ) {
        return "BrowserAdapter::getHistoryState(stateCb): Bad arguments.";
    }

    TRACE;

    BrowserAdapter *proxy = GetAndInitAdapter(adapter);
    if (proxy->mBrowserServerConnected) {

        int nQueryNum = proxy->mBsQueryNum++;
        GetHistoryStateArgs*  callArgs = new GetHistoryStateArgs(NPVARIANT_TO_OBJECT(args[0]), nQueryNum);

        proxy->m_historyStateArgs.add(callArgs);
        proxy->asyncCmdGetHistoryState(nQueryNum);
        return NULL;
    }
    else {
        return "Not connected to server";
    }
}

const char* BrowserAdapter::js_isEditing(AdapterBase *adapter, const NPVariant *args, uint32_t argCount, NPVariant *result)
{
    if ( argCount != 1 || !NPVARIANT_IS_OBJECT(args[0])) {
        return "BrowserAdapter::isEditing(callback): Bad arguments.";
    }

    TRACE;

    BrowserAdapter* proxy = GetAndInitAdapter(adapter);
    if (!proxy->mBrowserServerConnected) {
        return "Not connected to server";
    }

    int queryNum = proxy->mBsQueryNum;
    proxy->mBsQueryNum++;

    IsEditingArgs* callArgs = new IsEditingArgs(NPVARIANT_TO_OBJECT(args[0]), queryNum);

    // save async request args with unique query id to avoid overlapping responses
    proxy->m_isEditingArgs.add(callArgs);

    // send message to BS with query number. BA will then match reply queryNum
    // with request queryNum
    proxy->asyncCmdIsEditing(queryNum);

    return NULL;
}

const char* BrowserAdapter::js_insertStringAtCursor(AdapterBase *adapter, const NPVariant *args, uint32_t argCount, NPVariant *result)
{
    if(argCount != 1 || NPVARIANT_IS_STRING(args[0]) == false)
    {
        return "BrowserAdapter::insertStringAtCursor(string): Bad arguments.";
    }

    BrowserAdapter *a = GetAndInitAdapter(adapter);
    if (!a->mBrowserServerConnected) {
        return "BrowserAdapter::insertStringAtCursor: Disconnected from server";
    }

    char* text = NPStringToString(NPVARIANT_TO_STRING(args[0]));
    TRACEF("'%s'\n", text);

    a->asyncCmdInsertStringAtCursor(text);

    ::free(text);

    return NULL;
}

/**
 * Save the current view to the supplied file in PNG format. Caller is responsible for deleting
 * newly generated file.
 *
 * @param [0] Output file path.
 * @param [1] Left view coordinate of clipping rectangle. (optional)
 * @param [2] Top view coordinate of clipping rectangle. (optional)
 * @param [3] Right view coordinate of clipping rectangle. (optional)
 * @param [4] Bottom view coordinate of clipping rectangle. (optional)
 */
const char* BrowserAdapter::js_saveViewToFile(AdapterBase *adapter, const NPVariant *args, uint32_t argCount, NPVariant *result)
{
    BrowserAdapter *a = GetAndInitAdapter(adapter);

    int viewLeft = - a->mScrollPos.x;  // raw coords scaled by PGContext in renderToFile
    int viewTop =  - a->mScrollPos.y;

    // Source Rectangle is optional
    if ( !((argCount == 1 && NPVARIANT_IS_STRING(args[0])) ||
            (argCount == 5 && NPVARIANT_IS_STRING(args[0]) &&
             IsIntegerVariant(args[1]) &&
             IsIntegerVariant(args[2]) &&
             IsIntegerVariant(args[3]) &&
             IsIntegerVariant(args[4])))

       ) {
        INT32_TO_NPVARIANT( kInvalidParam, *result );
        return "BrowserAdapter::saveViewToFile(string [, left, top, right, bottom]): Bad arguments.";
    }

    char* pszOutFile = NPStringToString(NPVARIANT_TO_STRING(args[0]));
    if (!isSafeDir(pszOutFile)) {
        // A security measure to ensure that only files in /var can be created
        ::free(pszOutFile);
        INT32_TO_NPVARIANT( kInvalidParam, *result );
        return "BrowserAdapter::saveViewToFile - Invalid filename";
    }

    int nImageWidth;
    int nImageHeight;

    if (argCount == 5) {
        viewLeft  = VariantToInteger(args[1]);
        viewTop   = VariantToInteger(args[2]);
        int viewRight  = VariantToInteger(args[3]);
        int viewBottom = VariantToInteger(args[4]);

        nImageWidth  = viewRight  - viewLeft;  // size of image
        nImageHeight = viewBottom - viewTop;
    } else {
        // size of viewport if no dimensions are passed via npapi
        nImageWidth = a->mViewportWidth;
        nImageHeight = a->mViewportHeight;
    }

    char* pszDirName = ::strdup(pszOutFile);
    if (NULL == pszDirName) {
        INT32_TO_NPVARIANT( ENOMEM, *result );
        return "BrowserAdapter::saveViewToFile - Insufficient space";
    }

    // Create the output directory (ignore error)
    ::g_mkdir_with_parents(::dirname(pszDirName), S_IRWXU );

    // set the directory ownership and permission so non-root browserserver
    // can write to it
    {
        struct passwd *pw;
        pw = getpwnam(BROWSERVER_USER);

        if (pw) {
            ::chown(pszDirName,pw->pw_uid,pw->pw_gid);
        }
    }
    ::free(pszDirName);
    pszDirName = NULL;

    int nErr;
    a->syncCmdRenderToFile(pszOutFile, viewLeft, viewTop, nImageWidth, nImageHeight, nErr);

    if (nErr) {
        char msg[kExceptionMessageLength];
        ::snprintf(msg, G_N_ELEMENTS(msg), "BrowserAdapter::saveViewToFile - ERROR %d saving view to '%s'", nErr, pszOutFile);
        a->ThrowException(msg);
    }

    ::free( pszOutFile );

    return NULL;
}

/**
 * Read in a 32-bit RGBA PNG image.
 *
 * @param pszFileName  The name of the PNG file to read.
 * @param pPixelData   The 32-bit pixel data will be allocated and read into this pointer. The caller
 *                     is responsible for deleting this memory (delete [] pPixelData).
 * @param nImageWidth  Will return the width of the image.
 * @param nImageHeight Will return the height of the image.
 *
 * @return Zero if successful, non-zero on error.
 */
int BrowserAdapter::readPngFile(const char* pszFileName, uint32_t* &pPixelData, int &nImageWidth, int &nImageHeight)
{
    int nErr = 0;

    FILE *fp = fopen(pszFileName, "rb");
    if (NULL == fp) {
        nErr = errno;
    }

    png_structp png_ptr(NULL);
    if (!nErr) {
        png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
        if (NULL == png_ptr) {
            nErr = EBADF;
        }
    }

    png_infop info_ptr(NULL);
    if (!nErr) {

        if (setjmp(png_jmpbuf(png_ptr))) {
            nErr = EIO;
        }
        else {
            info_ptr = png_create_info_struct(png_ptr);
            if (NULL == info_ptr) {
                nErr = EBADF;
            }
        }
    }

    if (!nErr) {
        if (setjmp(png_jmpbuf(png_ptr))) {
            nErr = EIO;
        }
        else {
            png_init_io(png_ptr, fp);
        }
    }

    if (!nErr) {
        if (setjmp(png_jmpbuf(png_ptr))) {
            nErr = EIO;
        }
        else {
            png_read_png(png_ptr, info_ptr, PNG_TRANSFORM_IDENTITY, NULL);
        }
    }

    if (NULL != fp) {
        ::fclose(fp);
    }

    if (!nErr && info_ptr->bit_depth != 8) {
        nErr = EILSEQ;
    }
    if (!nErr && info_ptr->color_type != PNG_COLOR_TYPE_RGBA) {
        nErr = EILSEQ;
    }

    if (!nErr) {
        nImageWidth = info_ptr->width;
        nImageHeight = info_ptr->height;
        pPixelData = new uint32_t[info_ptr->width * info_ptr->height];
        if (NULL == pPixelData) {
            nErr = ENOMEM;
        }
    }

    if (!nErr) {
        if (setjmp(png_jmpbuf(png_ptr))) {
            nErr = EIO;
        }
        else {
            png_bytep* row_pointers = png_get_rows(png_ptr, info_ptr);
            png_byte* r = reinterpret_cast<png_byte*>(pPixelData);
            for (png_uint_32 row = 0; row < info_ptr->height; row++) {
                int idx = 0;
                for (png_uint_32 col = 0; col < info_ptr->width; col++) {
                    *r++ = row_pointers[row][idx++];
                    *r++ = row_pointers[row][idx++];
                    *r++ = row_pointers[row][idx++];
                    *r++ = row_pointers[row][idx++];
                }
            }
        }
    }

    if (NULL != png_ptr) {
        if (setjmp(png_jmpbuf(png_ptr))) {
            nErr = EIO;
        }
        else {
            png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp)NULL);
        }

    }

    return nErr;
}

/**
 * Some images come "pre-multiplied" - this function will scale the RGB values
 * by the alpha channel value.
 */
int BrowserAdapter::preScaleImage(PPixmap* pPixmap, int nImageWidth, int nImageHeight)
{
    uint32_t* pPixelData = static_cast<uint32_t*>(pPixmap->GetBuffer());
    int nNumPixels = nImageWidth * nImageHeight;
    while (nNumPixels--) {
        png_byte* pixel = (png_byte*)pPixelData;
        if (pixel[3] == 0) {
            *pPixelData = 0x0;
        }
        else if (pixel[3] == 255) {
        }
        else {
            float scale = static_cast<float>(pixel[3]) / 255.0;
            for (int i = 0; i < 3; i++) {
                pixel[i] = static_cast<png_byte>(static_cast<float>(pixel[i]) * scale);
            }
        }
        pPixelData++;
    }

    return 0;
}

/**
 * Read in a 32-bit PNG file and create a Pixmap from it.
 */
int BrowserAdapter::readPngFile(const char* pszFileName, PContext2D& context, PPixmap* &pPixmap, int &nImageWidth, int &nImageHeight)
{
    pPixmap = NULL;
    uint32_t* pPixelData(NULL);

    int nErr = readPngFile(pszFileName, pPixelData, nImageWidth, nImageHeight);

    if (!nErr) {
        pPixmap = context.CreatePixmap(nImageWidth, nImageHeight);
        if (NULL != pPixmap) {
            pPixmap->SetQuality(PSBilinear);
            // FIXME: use real pitch (row length in bytes) here.
            unsigned int Pitch = 0;
            pPixmap->Set (PFORMAT_8888, pPixelData, Pitch, true /*allow access*/);
        }
        else {
            nErr = ENOMEM;
        }
    }

    delete [] pPixelData;

    return nErr;
}

/**
 * Write the 32-bit RGBA pixel data to a file in PNG format.
 *
 * @param pszFileName  The file in which to put the pixel data.
 * @param pPixelData   The pixel data to write.
 * @param nImageWidth  The width of the image.
 * @param nImageHeight The height of the image.
 *
 * @return The error code. Zero means success.
 */
int BrowserAdapter::writePngFile(const char* pszFileName, const uint32_t* pPixelData, int nImageWidth, int nImageHeight)
{
    if (NULL == pszFileName || NULL == pPixelData)
        return EFAULT;

    int nErr = 0;

    const png_byte byBitDepth(8);

    png_structp pPngImage = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (NULL == pPngImage) {
        nErr = ENOMEM;// Don't know why this will fail, let's say insufficient memory.
    }

    png_infop pPngInfo(NULL);
    if (!nErr) {
        pPngInfo = png_create_info_struct(pPngImage);
        if (NULL == pPngInfo) {
            nErr = ENOMEM;// Don't know why this will fail, let's say insufficient memory.
        }
    }

    FILE* pFile(NULL);
    if (!nErr) {
        pFile = ::fopen(pszFileName, "wb");
        if (NULL == pFile) {
            nErr = EIO;
        }
    }

    // Initialize I/O
    if ( !nErr ) {
        if ( setjmp( png_jmpbuf(pPngImage) ) )
            nErr = EIO;
        else
            png_init_io(pPngImage, pFile);
    }

    // Write the header
    if ( !nErr ) {
        if (setjmp(png_jmpbuf(pPngImage))) {
            nErr = EIO;
        }
        else {
            png_set_IHDR(pPngImage, pPngInfo, nImageWidth, nImageHeight,
                         byBitDepth, PNG_COLOR_TYPE_RGBA, PNG_INTERLACE_NONE,
                         PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);

            png_write_info(pPngImage, pPngInfo);
        }
    }

    // Write the image data
    for (int row = 0; row < nImageHeight; row++) {
        if (setjmp(png_jmpbuf(pPngImage))) {
            nErr = EIO;
        }
        else {
            png_write_row( pPngImage, (png_byte*)(pPixelData) );
            pPixelData += nImageWidth;
        }
    }

    // Close file
    if ( NULL != pPngImage ) {
        if (setjmp(png_jmpbuf(pPngImage))) {
            nErr = EIO;
        }
        else {
            png_write_end(pPngImage, NULL);
        }
    }

    if ( NULL != pPngImage ) {
        if (setjmp(png_jmpbuf(pPngImage))) {
            nErr = EIO;
        }
        else {
            png_destroy_write_struct( &pPngImage, &pPngInfo );
        }
    }

    if (NULL != pFile) {
        ::fclose(pFile);
    }

    return nErr;
}


void
BrowserAdapter::msgGetElementInfoAtPointResponse(int32_t queryNum, bool succeeded, const char* element, const char* id,
        const char* name, const char* cname, const char* type, int32_t left, int32_t top, int32_t right, int32_t bottom,
        bool isEditable)
{
    TRACEF("GetElementInfoAtPointResponse: succeeded: %c, element:'%s'", succeeded ? 'T' : 'F', element);
    std::auto_ptr<GetElementInfoAtPointArgs> args(m_getElementInfoAtPointArgs.get(queryNum));
    if (!args.get()) {
        TRACEF("Can't find response for this call.");
        return;
    }

    NPVariant jsCallResult, jsCallArgs;
    NPObject* info = NPN_CreateObject(&ElementInfo::sElementInfoClass);
    if (info) {
        OBJECT_TO_NPVARIANT(info, jsCallArgs);
        static_cast<ElementInfo*>(info)->initialize(succeeded, element, id, name, cname, type, left, top, right,
                bottom, args->m_x, args->m_y, isEditable);
        if (NPN_InvokeDefault(args->m_callback, &jsCallArgs, 1, &jsCallResult))
            TRACEF("getElementInfoAtPoint response call success.");
        else
            TRACEF("getElementInfoAtPoint response call FAILED.");

        AdapterBase::NPN_ReleaseVariantValue(&jsCallArgs);
    }
    else {
        TRACEF("getElementInfoAtPoint: response obj NOT created.");
    }
}

/**
 * Generate an icon from the specified file. This routine will do the following:
 *
 * 1. Read in the source file.
 * 2. Clip the source image using the supplied rectangle.
 * 3. Resize the resultant image to 64x64 pixels.
 * 4. "Embellish" icon to make it "pretty".
 * 5. Write out the new icon to the supplied output file.
 *
 * @note Both files must be PNG files.
 *
 * @param [0] Source file path.
 * @param [1] Output file path.
 * @param [2] Left coordinate of clipping rectangle.
 * @param [3] Top coordinate of clipping rectangle.
 * @param [4] Right coordinate of clipping rectangle.
 * @param [5] Bottom coordinate of clipping rectangle.
 */
const char* BrowserAdapter::js_generateIconFromFile(AdapterBase *adapter, const NPVariant *args, uint32_t argCount, NPVariant *result)
{
    if(argCount != 6 ||
            !NPVARIANT_IS_STRING(args[0]) ||
            !NPVARIANT_IS_STRING(args[1]) ||
            !IsIntegerVariant(args[2]) ||
            !IsIntegerVariant(args[3]) ||
            !IsIntegerVariant(args[4]) ||
            !IsIntegerVariant(args[5]) ) {
        INT32_TO_NPVARIANT( kInvalidParam, *result );
        return "BrowserAdapter::generateIconFromFile(string, string, number, number, number, number): Bad arguments.";
    }

    char* pszSrcFile = NPStringToString(NPVARIANT_TO_STRING(args[0]));
    char* pszOutFile = NPStringToString(NPVARIANT_TO_STRING(args[1]));
    int left  = VariantToInteger(args[2]);
    int top   = VariantToInteger(args[3]);
    int right = VariantToInteger(args[4]);
    int bottom= VariantToInteger(args[5]);

    if (NULL == pszSrcFile || '\0' == *pszSrcFile || NULL == strstr(pszSrcFile, ".png") ) {
        INT32_TO_NPVARIANT( kInvalidParam, *result );
        return "BrowserAdapter::generateIconFromFile - bad source file name";
    }
    if (NULL == pszOutFile || '\0' == *pszOutFile || NULL == strstr(pszOutFile, ".png") ) {
        INT32_TO_NPVARIANT( kInvalidParam, *result );
        return "BrowserAdapter::generateIconFromFile - bad output file name";
    }

    TRACEF("(l:%d, t:%d, r:%d, b:%d), '%s'->'%s'", left, top, right, bottom,
           pszSrcFile, pszOutFile);
    if (!isSafeDir(pszOutFile)) {
        ::free(pszOutFile);
        ::free(pszSrcFile);
        INT32_TO_NPVARIANT( kInvalidParam, *result );
        return "BrowserAdapter::generateIconFromFile: Invalid filename";
    }

    char* pszFileName = ::strdup(pszOutFile);
    if (NULL == pszFileName) {
        INT32_TO_NPVARIANT( ENOMEM, *result );
        return "BrowserAdapter::generateIconFromFile: Insufficient space";
    }

    // Create the output directory (ignore error)
    ::g_mkdir_with_parents(::dirname(pszFileName), S_IRWXU );
    ::free(pszFileName);

    int nErr = 0;

    // Read in the source file
    int nImageWidth(0), nImageHeight(0);
    PPixmap* pInputPixmap(NULL);
    PContext2D context;
    nErr = readPngFile(pszSrcFile, context, pInputPixmap, nImageWidth, nImageHeight);

    if (right < left)
        right = left;
    if (right > nImageWidth)
        right = nImageWidth;
    if (bottom < top)
        bottom = top;
    if (bottom > nImageHeight)
        bottom = nImageHeight;

    const int nMargin = 4;// Must agree with pixel data in image files
    const int nIconSize = 64;// Width & height of output image
    const int nIconWidth = nIconSize-2*nMargin;// Width of icon image within file
    const int nIconHeight = nIconSize-2*nMargin;
    const PVertex2D iconTL(nMargin, nMargin);
    const PVertex2D iconBR(nMargin + nIconWidth, nMargin + nIconHeight);

    PPixmap* pFinalPixmap(NULL);
    if (!nErr) {
        pFinalPixmap = context.CreatePixmap(nIconSize, nIconSize);
        if (NULL != pFinalPixmap) {
            pFinalPixmap->Set (PFORMAT_8888, NULL, 0, true /*allow access*/);
            pFinalPixmap->SetQuality(PSBilinear);
            context.RenderToTexture(pFinalPixmap);
            context.Clear(PColor32(0x00,0x00,0x00,0x0));

            const PVertex2D srcTL(left, top);
            const PVertex2D srcBR(right, bottom);
            context.DrawSubPixmap(pInputPixmap, iconTL, iconBR, srcTL, srcBR);
        }
        else {
            nErr = ENOMEM;
        }
    }

    // Mask the icon to give it the rounded corners.
    if (!nErr) {
        PPixmap* pMaskPixmap(NULL);
        int nWidth(0), nHeight(0);
        nErr = readPngFile(kIconMaskFile, context, pMaskPixmap, nWidth, nHeight);
        if (!nErr) {
            assert( nWidth == nIconSize );
            int nNumPixels = nWidth * nHeight;
            uint32_t* pImgData = static_cast<uint32_t*>(pFinalPixmap->GetBuffer());
            const uint32_t* pMaskData = static_cast<uint32_t*>(pMaskPixmap->GetBuffer());
            while (nNumPixels--) {
                *pImgData = (*pImgData & 0x00ffffff) | (*pMaskData & 0xff000000);
                pImgData++;
                pMaskData++;
            }
        }
        if (NULL != pMaskPixmap)
            context.DestroyPixmap(pMaskPixmap);
    }

    // Overlay the icon to give it a drop shadow and a slight 3D look.
    if (!nErr) {
        PPixmap* pOverlayPixmap(NULL);
        int nWidth(0), nHeight(0);
        nErr = readPngFile(kIconOverlayFile, context, pOverlayPixmap, nWidth, nHeight);

        if (!nErr)
            nErr = preScaleImage(pFinalPixmap, nWidth, nHeight);

        if (!nErr)
            nErr = preScaleImage(pOverlayPixmap, nWidth, nHeight);

        if (!nErr)
            context.DrawPixmap(pOverlayPixmap, PVertex2D(0,0), PVertex2D(nIconSize,nIconSize));

        if (NULL != pOverlayPixmap)
            context.DestroyPixmap(pOverlayPixmap);
    }

    if (!nErr) {
        nErr = writePngFile(pszOutFile, static_cast<const uint32_t*>(pFinalPixmap->GetBuffer()), nIconSize, nIconSize);
    }

    if (NULL != pInputPixmap) {
        context.DestroyPixmap(pInputPixmap);
    }
    if (NULL != pFinalPixmap) {
        context.DestroyPixmap(pFinalPixmap);
    }

    if (nErr) {
        char msg[kExceptionMessageLength];
        ::snprintf(msg, G_N_ELEMENTS(msg), "ERROR %d generating icon '%s'", nErr, pszOutFile);
        BrowserAdapter *a = GetAndInitAdapter(adapter);
        a->ThrowException(msg);
    }

    ::free( pszSrcFile );
    ::free( pszOutFile );

    return NULL;
}

const char* BrowserAdapter::js_deleteImage(AdapterBase *adapter, const NPVariant *args, uint32_t argCount, NPVariant *result)
{
    if( !(argCount == 1 && NPVARIANT_IS_STRING(args[0])) ) {
        INT32_TO_NPVARIANT( kInvalidParam, *result );
        return "BrowserAdapter::deleteImage(string): Bad arguments.";
    }

    char* pszFile = NPStringToString(NPVARIANT_TO_STRING(args[0]));

    // For security reasons I only delete PNG files.
    if (NULL == pszFile || '\0' == *pszFile || NULL == strstr(pszFile, ".png") ) {
        INT32_TO_NPVARIANT( kInvalidParam, *result );
        TRACEF("%s: bad file name '%s'", __FUNCTION__, pszFile);
        return "BrowserAdapter::deleteImage - bad file name";
    }

    TRACEF("Deleting '%s'", pszFile);
    int nErr = 0;

    if (::unlink(pszFile) == -1) {
        nErr = errno;
    }

    if (nErr) {
        char msg[kExceptionMessageLength];
        ::snprintf(msg, G_N_ELEMENTS(msg), "ERROR %d deleting '%s'", nErr, pszFile);
        BrowserAdapter *a = GetAndInitAdapter(adapter);
        a->ThrowException(msg);
    }

    ::free(pszFile);

    return NULL;
}

const char* BrowserAdapter::js_resizeImage(AdapterBase *adapter, const NPVariant *args, uint32_t argCount, NPVariant *result)
{
    if( !(argCount == 4 &&
            NPVARIANT_IS_STRING(args[0]) &&
            NPVARIANT_IS_STRING(args[1]) &&
            IsIntegerVariant(args[2]) &&
            IsIntegerVariant(args[3])) ) {
        INT32_TO_NPVARIANT( kInvalidParam, *result );
        return "BrowserAdapter::resizeImage(string, string, number, number): Bad arguments.";
    }

    int nErr = 0;

    char* pszSrcFile = NPStringToString(NPVARIANT_TO_STRING(args[0]));
    char* pszDstFile = NPStringToString(NPVARIANT_TO_STRING(args[1]));
    int nDstWidth  = VariantToInteger(args[2]);
    int nDstHeight = VariantToInteger(args[3]);

    TRACEF("Resizing '%s'->'%s' %d x %d\n", pszSrcFile, pszDstFile, nDstWidth, nDstHeight);

    int nSrcWidth(0), nSrcHeight(0);
    PPixmap* pSrcPixmap(NULL);
    PContext2D context;
    nErr = readPngFile(pszSrcFile, context, pSrcPixmap, nSrcWidth, nSrcHeight);

    PPixmap* pFinalPixmap(NULL);
    if (!nErr) {
        pFinalPixmap = context.CreatePixmap(nDstWidth, nDstHeight);
        if (NULL != pFinalPixmap) {

            pFinalPixmap->Set (PFORMAT_8888, NULL, 0, true /*allow access*/);
            PFilters filters;
            filters.DownSample(pFinalPixmap, pSrcPixmap);
        }
        else {
            nErr = ENOMEM;
        }
    }

    if (!nErr) {
        nErr = writePngFile(pszDstFile, static_cast<const uint32_t*>(pFinalPixmap->GetBuffer()), nDstWidth, nDstHeight);
    }

    if (NULL != pSrcPixmap) {
        context.DestroyPixmap(pSrcPixmap);
    }
    if (NULL != pFinalPixmap) {
        context.DestroyPixmap(pFinalPixmap);
    }

    if (nErr) {
        char msg[kExceptionMessageLength];
        ::snprintf(msg, G_N_ELEMENTS(msg), "ERROR %d resizing '%s'", nErr, pszSrcFile);
        BrowserAdapter *a = GetAndInitAdapter(adapter);
        a->ThrowException(msg);
    }

    ::free(pszSrcFile);
    ::free(pszDstFile);

    return NULL;
}

/**
 * Calculate where to smart zoom into a page.
 */
const char* BrowserAdapter::js_smartZoom(AdapterBase *adapter, const NPVariant *args, uint32_t argCount, NPVariant *result)
{
    if(argCount != 2 || IsIntegerVariant(args[0]) == false || IsIntegerVariant(args[1]) == false) {
        return "BrowserAdapter::smartZoom(int, int): Bad arguments.";
    }

    TRACEF("%d, %d\n", VariantToInteger(args[0]), VariantToInteger(args[1]));
    BrowserAdapter *a = GetAndInitAdapter(adapter);

    Point docPt(VariantToInteger(args[0]) / a->mZoomLevel,
                VariantToInteger(args[1]) / a->mZoomLevel);

    bool inFlashRect = a->flashRectContainsPoint(docPt);
    if (a->flashGestureLock()) {
        if (!inFlashRect) {
            a->updateFlashGestureLockStatus(false);
        } else {
            /* ignore; taps will be sent through with handle* methods */
            return NULL;
        }
    } else if (inFlashRect) {
        a->updateFlashGestureLockStatus(true);
    }

    a->prvSmartZoom(docPt);

    return NULL;
}

const char* BrowserAdapter::js_setViewportSize(AdapterBase *adapter, const NPVariant *args, uint32_t argCount, NPVariant *result)
{
    if (argCount != 2 || IsIntegerVariant(args[0]) == false || IsIntegerVariant(args[1]) == false) {
        return "BrowserAdapter::setViewportSize(int, int): Bad arguments.";
    }

    BrowserAdapter *a = GetAndInitAdapter(adapter);
    int newWidth = VariantToInteger(args[0]);
    int newHeight = VariantToInteger(args[1]);

    if (newWidth == 0 || newHeight == 0) {
        return "BrowserAdapter::setViewportSize(int, int): cannot set to 0";
    }

    TRACEF("%dx%d -> %dx%d", a->mViewportWidth, a->mViewportHeight,
           newWidth, newHeight);

    if (a->mViewportWidth != newWidth || a->mViewportHeight != newHeight) {
        a->mViewportWidth = newWidth;
        a->mViewportHeight = newHeight;
        a->asyncCmdSetWindowSize(a->mViewportWidth, a->mViewportHeight);
    }

    return NULL;
}

/**
 * Set the current list of DNS server to use for lookups
 *
 * @param [0] Comma seperated list of DNS servers.
 */
const char* BrowserAdapter::js_setDNSServers(AdapterBase *adapter, const NPVariant *args, uint32_t argCount, NPVariant *result)
{
    TRACE;

    if (argCount != 1 || !NPVARIANT_IS_STRING(args[0])) {
        INT32_TO_NPVARIANT( kInvalidParam, *result );
        return "BrowserAdapter::js_setDNSServers(string): Bad argument.";
    }

    char *inServers = NPStringToString(NPVARIANT_TO_STRING(args[0]));
    TRACEF("Setting DNS servers: %s", inServers);

    BrowserAdapter *a = GetAndInitAdapter(adapter);
    a->asyncCmdSetDNSServers(inServers);

    ::free(inServers);

    return NULL;
}

const char* BrowserAdapter::js_setMinFontSize(AdapterBase *adapter, const NPVariant *args, uint32_t argCount, NPVariant *result)
{
    if(argCount != 1 || IsIntegerVariant(args[0]) == false) {
        return "BrowserAdapter::setMinFontSize(int): Bad arguments.";
    }
    int32_t minFontSize = VariantToInteger(args[0]);
    TRACEF("Setting minFontSize: %d", minFontSize);
    BrowserAdapter *a = GetAndInitAdapter(adapter);

    a->asyncCmdSetMinFontSize(minFontSize);

    return NULL;
}

const char* BrowserAdapter::js_interrogateClicks(AdapterBase *adapter, const NPVariant *args, uint32_t argCount, NPVariant *result)
{
    if(argCount != 1 || IsBooleanVariant(args[0]) == false) {
        return "BrowserAdapter::interrogateClicks(int): Bad arguments.";
    }
    BrowserAdapter *a = GetAndInitAdapter(adapter);
    a->m_interrogateClicks = VariantToBoolean(args[0]);
    TRACEF("Interrogate clicks: %c", a->m_interrogateClicks ? 'Y' : 'N');

    a->asyncCmdInterrogateClicks(a->m_interrogateClicks);

    return NULL;
}

const char* BrowserAdapter::js_clickAt(AdapterBase *adapter, const NPVariant *args, uint32_t argCount, NPVariant *result)
{
    if (argCount != 3 || IsIntegerVariant(args[0]) == false ||
            IsIntegerVariant(args[1]) == false || IsIntegerVariant(args[2]) == false) {
        return "BrowserAdapter::js_clickAt(int, int): Bad arguments.";
    }

    BrowserAdapter *a = GetAndInitAdapter(adapter);

    if (a->m_passInputEvents || a->m_spotlightHandle) {
        return NULL;
    }

    Point clickPt(VariantToInteger(args[0]), VariantToInteger(args[1]) - a->m_headerHeight);
    int counter = VariantToInteger(args[2]);

    TRACEF("%d, %d, %d", clickPt.x, clickPt.y, counter);
    struct timeval tv, elapsed;
    gettimeofday(&tv, NULL);

    timersub(&tv, &a->m_lastPassedEventTime, &elapsed);
    if (elapsed.tv_sec == 0 && elapsed.tv_usec < (50 * 1000)) {
        return NULL;
    }

    clickPt.x /= a->mZoomLevel;
    clickPt.y /= a->mZoomLevel;

    /* filter clicks out in flash rects when gesture lock enabled since we
     * send the events directly via handlePen* */
    if (a->flashGestureLock() && a->flashRectContainsPoint(clickPt)) {
        return NULL;
    }

    if (a->m_sentMouseHoldEvent.sent &&
            a->m_sentMouseHoldEvent.pt == clickPt) {

        a->m_sentMouseHoldEvent.reset();
        return NULL;
    }

    a->m_sentMouseHoldEvent.reset();
    a->asyncCmdClickAt(clickPt.x, clickPt.y, 1, counter);

    return NULL;
}

/**
 * Send a response for a popup menu selection.
 *
 * @param param[0] Numerical index of selected menu item. Pass a negative value to indicate that no selection was
 *                 made.
 */
const char* BrowserAdapter::js_selectPopupMenuItem(AdapterBase *adapter, const NPVariant *args, uint32_t argCount, NPVariant *result)
{
    TRACE;

    if (argCount != 2 || !NPVARIANT_IS_STRING(args[0]) || !IsIntegerVariant(args[1]))
        return "BrowserAdapter::js_selectPopupMenuItem(string, double): Bad arguments.";

    char *identifier = NPStringToString(NPVARIANT_TO_STRING(args[0]));
    int32_t selIdx = VariantToInteger(args[1]);

    TRACEF("Sending popup menu selection: %d\n", selIdx);
    BrowserAdapter *a = GetAndInitAdapter(adapter);

    a->asyncCmdPopupMenuSelect(identifier, selIdx);
    ::free(identifier);

    return NULL;
}

/**
 * Send a response to a Javascript dialog.  The first parameter is the sync pipe path followed by the
 * return values.
 */
const char* BrowserAdapter::js_sendDialogResponse(AdapterBase *adapter, const NPVariant *args, uint32_t argCount, NPVariant *result)
{
    bool paramsOkay(false);
    uint32_t msgLen = 0, i;
    TRACE;

    // Verify the arguments
    // 0 = "1" for Okay, "0" for "Cancel"
    // 1, 2 (optional) = prompt response or username & password
    if (argCount >= 1 && argCount <= 3) {
        paramsOkay = true;
        for (i=0; i< argCount; i++)
            if (NPVARIANT_IS_STRING(args[i]) == false)
                paramsOkay = false;
            else {
                msgLen += 1 + strlen(NPStringToString(NPVARIANT_TO_STRING(args[i])));
            }
    }
    if (!paramsOkay) {
        return "BrowserAdapter::js_sendDialogResponse(string, string, ...): Bad arguments";
    }

    if (strlen(gDialogResponsePipe) == 0) {
        return "BrowserAdapter::js_sendDialogResponse(string, string, ...): Invalid state";
    }

    FILE* f = fopen(gDialogResponsePipe, "wb" );
    if (f) {
        // Write the length of the response (in FIFO format)
        const char *buf = (const char *) &msgLen;
        fwrite(buf+3, 1, 1, f);
        fwrite(buf+2, 1, 1, f);
        fwrite(buf+1, 1, 1, f);
        fwrite(buf, 1, 1, f);

        // Write the arguments out
        for (i=0; i< argCount; i++) {
            char *str = NPStringToString(NPVARIANT_TO_STRING(args[i]));
            fwrite(str, sizeof(char), strlen(str)+1, f);
            ::free(str);
        }
        fclose(f);
    }

    memset(gDialogResponsePipe, 0, G_N_ELEMENTS(gDialogResponsePipe));

    return NULL;
}

const char* BrowserAdapter::js_gestureStart(AdapterBase *adapter, const NPVariant *args, uint32_t argCount, NPVariant *result)
{
    BrowserAdapter *a = GetAndInitAdapter(adapter);

    if (argCount != 6 ||
            !IsIntegerVariant(args[0]) ||
            !IsIntegerVariant(args[1]) ||
            !IsDoubleVariant(args[2]) ||
            !IsDoubleVariant(args[3]) ||
            !IsIntegerVariant(args[4]) ||
            !IsIntegerVariant(args[5])) {
        return "BrowserAdapter::js_gestureStart(6*double): Bad arguments.";
    }

    TRACEF("contentX: %d, contentY: %d, zoomScale: %f, rotation: %f, centerX: %d, centerY: %d",
           (int)(VariantToInteger(args[0]) / a->mZoomLevel), //contentX
           (int)(VariantToInteger(args[1]) / a->mZoomLevel), //contentY
           VariantToDouble(args[2]), // zoomScale
           VariantToDouble(args[3]), // rotation
           (int)(VariantToInteger(args[4]) / a->mZoomLevel), // centerX
           (int)(VariantToInteger(args[5]) / a->mZoomLevel));

    a->doGestureStart(VariantToInteger(args[0]) / a->mZoomLevel, //contentX
                      VariantToInteger(args[1]) / a->mZoomLevel, //contentY
                      VariantToDouble(args[2]), // zoomScale
                      VariantToDouble(args[3]), // rotation
                      VariantToInteger(args[4]) / a->mZoomLevel, // centerX
                      VariantToInteger(args[5]) / a->mZoomLevel); // centerY

    return NULL;
}

const char* BrowserAdapter::js_gestureChange(AdapterBase *adapter, const NPVariant *args, uint32_t argCount, NPVariant *result)
{
    BrowserAdapter *a = GetAndInitAdapter(adapter);

    if (argCount != 6 ||
            !IsIntegerVariant(args[0]) ||
            !IsIntegerVariant(args[1]) ||
            !IsDoubleVariant(args[2]) ||
            !IsDoubleVariant(args[3]) ||
            !IsIntegerVariant(args[4]) ||
            !IsIntegerVariant(args[5])) {
        return "BrowserAdapter::js_gestureChange(6*double): Bad arguments.";
    }

    TRACEF("contentX: %d, contentY: %d, zoomScale: %f, rotation: %f, centerX: %d, centerY: %d",
           (int)(VariantToInteger(args[0]) / a->mZoomLevel), //contentX
           (int)(VariantToInteger(args[1]) / a->mZoomLevel), //contentY
           VariantToDouble(args[2]), // zoomScale
           VariantToDouble(args[3]), // rotation
           (int)(VariantToInteger(args[4]) / a->mZoomLevel), // centerX
           (int)(VariantToInteger(args[5]) / a->mZoomLevel));

    a->doGestureChange(VariantToInteger(args[0]) / a->mZoomLevel, //contentX
                       VariantToInteger(args[1]) / a->mZoomLevel, //contentY
                       VariantToDouble(args[2]), // zoomScale
                       VariantToDouble(args[3]), // rotation
                       VariantToInteger(args[4]) / a->mZoomLevel, // centerX
                       VariantToInteger(args[5]) / a->mZoomLevel); // centerY

    return NULL;
}

const char* BrowserAdapter::js_gestureEnd(AdapterBase *adapter, const NPVariant *args, uint32_t argCount, NPVariant *result)
{
    BrowserAdapter *a = GetAndInitAdapter(adapter);

    if (argCount != 6 ||
            !IsIntegerVariant(args[0]) ||
            !IsIntegerVariant(args[1]) ||
            !IsDoubleVariant(args[2]) ||
            !IsDoubleVariant(args[3]) ||
            !IsIntegerVariant(args[4]) ||
            !IsIntegerVariant(args[5])) {
        return "BrowserAdapter::js_gestureChange(6*double): Bad arguments.";
    }

    TRACEF("contentX: %d, contentY: %d, zoomScale: %f, rotation: %f, centerX: %d, centerY: %d",
           (int)(VariantToInteger(args[0]) / a->mZoomLevel), //contentX
           (int)(VariantToInteger(args[1]) / a->mZoomLevel), //contentY
           VariantToDouble(args[2]), // zoomScale
           VariantToDouble(args[3]), // rotation
           (int)(VariantToInteger(args[4]) / a->mZoomLevel), // centerX
           (int)(VariantToInteger(args[5]) / a->mZoomLevel));

    a->doGestureEnd(VariantToInteger(args[0]) / a->mZoomLevel, //contentX
                    VariantToInteger(args[1]) / a->mZoomLevel, //contentY
                    VariantToDouble(args[2]), // zoomScale
                    VariantToDouble(args[3]), // rotation
                    VariantToInteger(args[4]) / a->mZoomLevel, // centerX
                    VariantToInteger(args[5]) / a->mZoomLevel); // centerY

    return NULL;
}


const char* BrowserAdapter::js_scrollStarting(AdapterBase *adapter, const NPVariant *args, uint32_t argCount, NPVariant *result)
{
    BrowserAdapter* a = static_cast<BrowserAdapter*>(adapter);

    TRACE;
    a->mInScroll = true;

    return NULL;
}

const char* BrowserAdapter::js_scrollEnding(AdapterBase *adapter, const NPVariant *args, uint32_t argCount, NPVariant *result)
{
    BrowserAdapter* a = static_cast<BrowserAdapter*>(adapter);

    TRACE;
    a->mInScroll = false;

    return NULL;
}

const char* BrowserAdapter::js_setEnableJavaScript(AdapterBase *adapter, const NPVariant *args, uint32_t argCount, NPVariant *result)
{
    if(argCount != 1 || IsBooleanVariant(args[0]) == false) {
        return "BrowserAdapter::js_setEnableJavaScript(bool): Bad arguments.";
    }
    bool enable = VariantToBoolean(args[0]);
    TRACEF("Enable JavaScript: %c", enable ? 'Y' : 'N');
    BrowserAdapter *a = GetAndInitAdapter(adapter);
    a->mEnableJavaScript = enable;

    a->asyncCmdSetEnableJavaScript(enable);

    return NULL;
}

const char* BrowserAdapter::js_setBlockPopups(AdapterBase *adapter, const NPVariant *args, uint32_t argCount, NPVariant *result)
{
    if(argCount != 1 || IsBooleanVariant(args[0]) == false) {
        return "BrowserAdapter::js_setBlockPopups(bool): Bad arguments.";
    }
    bool enable = VariantToBoolean(args[0]);
    TRACEF("Block Popups: %c", enable ? 'Y' : 'N');
    BrowserAdapter *a = GetAndInitAdapter(adapter);
    a->mBlockPopups = enable;

    a->asyncCmdSetBlockPopups(enable);

    return NULL;
}

const char* BrowserAdapter::js_setHeaderHeight(AdapterBase *adapter, const NPVariant *args, uint32_t argCount, NPVariant *result)
{
    if(argCount != 1 || !IsIntegerVariant(args[0])) {
        return "BrowserAdapter::js_setHeaderHeight(number): Bad arguments.";
    }
    int height = VariantToInteger(args[0]);
    TRACEF("HeaderHeight: %d", height);
    BrowserAdapter *a = GetAndInitAdapter(adapter);
    a->m_headerHeight = height;
    a->invalidate();

    return NULL;
}

const char* BrowserAdapter::js_setAcceptCookies(AdapterBase *adapter, const NPVariant *args, uint32_t argCount, NPVariant *result)
{
    if(argCount != 1 || IsBooleanVariant(args[0]) == false) {
        return "BrowserAdapter::js_setAcceptCookies(bool): Bad arguments.";
    }
    bool enable = VariantToBoolean(args[0]);
    TRACEF("Accept Cookies: %c", enable ? 'Y' : 'N');
    BrowserAdapter *a = GetAndInitAdapter(adapter);
    a->mAcceptCookies = enable;

    a->asyncCmdSetAcceptCookies(enable);

    return NULL;
}

const char* BrowserAdapter::js_setShowClickedLink(AdapterBase *adapter, const NPVariant *args, uint32_t argCount, NPVariant *result)
{
    if(argCount != 1 || IsBooleanVariant(args[0]) == false) {
        return "BrowserAdapter::js_setShowClickedLink(bool): Bad arguments.";
    }
    bool enable = VariantToBoolean(args[0]);
    TRACEF("Show Clicked Link: %c", enable ? 'Y' : 'N');
    BrowserAdapter *a = GetAndInitAdapter(adapter);
    a->mShowClickedLink = enable;

    a->asyncCmdSetShowClickedLink(enable);

    return NULL;
}

/*
* @brief requests from webkit array of rectangles that cover an interactive
*        node at given coordinates
*
*/
const char* BrowserAdapter::js_addElementHighlight(AdapterBase *adapter, const NPVariant *args, uint32_t argCount, NPVariant *result)
{
    /* FIXME: RR
        if (argCount !=2
    || !IsIntegerVariant(args[0])
    || !IsIntegerVariant(args[1])) {
            return "BrowserAdapter::js_addElementHighlight(double, double): Bad arguments.";
    }

        BrowserAdapter* a = GetAndInitAdapter(adapter);
        if (!a->mBrowserServerConnected) {
            return "BrowserAdapter::connectBrowserServer Cannot connect to server.";
        }

        int rawX = VariantToInteger(args[0]);
        int rawY = VariantToInteger(args[1]);

        int scaledX = (rawX - a->mJsScrollX) / a->mZoomLevel;
        int scaledY = (rawY - a->mJsScrollY) / a->mZoomLevel;

    TRACEF("%d, %d (jsscroll %d, %d) %f\n",
    rawX, rawY, a->mJsScrollX, a->mJsScrollY, a->mZoomLevel);

        // BS will get rectangles from webkit and will send them to BA as a json array
        a->asyncCmdGetInteractiveNodeRects(scaledX, scaledY);
        a->mShowHighlight = true;
    */
    return NULL;
}



/**
 * @brief calls removeHighlight() to clear vector of highlight rects and mark
 *        rect regions as dirty.
 *
 */
const char* BrowserAdapter::js_removeElementHighlight(AdapterBase *adapter, const NPVariant *args, uint32_t argCount, NPVariant *result)
{
    TRACE;
    BrowserAdapter* a = GetAndInitAdapter(adapter);
    if (!a->mBrowserServerConnected) {
        return "BrowserAdapter::connectBrowserServer Cannot connect to server.";
    }

    a->removeHighlight();

    return NULL;
}

const char* BrowserAdapter::js_enableSelectionMode(AdapterBase *adapter, const NPVariant *args, uint32_t argCount, NPVariant *result)
{
    /* FIXME: RR
        if (argCount !=2
            || !IsIntegerVariant(args[0])
            || !IsIntegerVariant(args[1])) {
            return "BrowserAdapter::js_enableSelectionMode(double, double): Bad arguments.";
        }

        BrowserAdapter* a = GetAndInitAdapter(adapter);
        if (!a->mBrowserServerConnected) {
            return "BrowserAdapter::connectBrowserServer Cannot connect to server.";
        }

        int rawX = VariantToInteger(args[0]);
        int rawY = VariantToInteger(args[1]);

        int scaledX = (rawX - a->mJsScrollX) / a->mZoomLevel;
        int scaledY = (rawY - a->mJsScrollY) / a->mZoomLevel;

    TRACEF("%d, %d (jsscroll %d, %d) %f\n",
    rawX, rawY, a->mJsScrollX, a->mJsScrollY, a->mZoomLevel);

        a->mSelectionReticle.x = scaledX;
        a->mSelectionReticle.y = scaledY;
        a->mSelectionReticle.show = true;
        a->invalidateSelectionReticleRegion();

        a->asyncCmdEnableSelection(scaledX, scaledY);
    */
    return NULL;
}

const char* BrowserAdapter::js_disableSelectionMode(AdapterBase *adapter, const NPVariant *args, uint32_t argCount, NPVariant *result)
{
    BrowserAdapter* a = GetAndInitAdapter(adapter);
    if (!a->mBrowserServerConnected) {
        return "BrowserAdapter::disableSelectionMode Cannot connect to server.";
    }

    TRACE;
    a->asyncCmdDisableSelection();

    return NULL;
}

const char* BrowserAdapter::js_selectAll(AdapterBase *adapter, const NPVariant *args, uint32_t argCount, NPVariant *result)
{
    BrowserAdapter* a = GetAndInitAdapter(adapter);
    if (!a->mBrowserServerConnected) {
        return "BrowserAdapter::selectAll Cannot connect to server.";
    }

    TRACE;
    a->asyncCmdSelectAll();

    return NULL;
}

const char* BrowserAdapter::js_cut(AdapterBase *adapter, const NPVariant *args, uint32_t argCount, NPVariant *result)
{
    BrowserAdapter* a = GetAndInitAdapter(adapter);
    if (!a->mBrowserServerConnected) {
        return "BrowserAdapter::cut Cannot connect to server.";
    }

    a->asyncCmdCut();

    return NULL;
}

const char* BrowserAdapter::js_copy(AdapterBase *adapter, const NPVariant *args, uint32_t argCount, NPVariant *result)
{
    BrowserAdapter* proxy = GetAndInitAdapter(adapter);
    if (!proxy->mBrowserServerConnected) {
        return "BrowserAdapter::copy Cannot connect to server.";
    }

    // if we have one argument, it must be a callback
    if (argCount > 1 || (argCount == 1 && !NPVARIANT_IS_OBJECT(args[0]))) {
        INT32_TO_NPVARIANT( kInvalidParam, *result );
        return "BrowserAdapter::js_copy(callback): Bad arguments.";
    }

    // allow the user to not pass a callback, though we must still use the
    // callback query mechanism. we will just use a null NPObject
    // that is ignored by BA when BS sends it a copy complete message

    int queryNum = proxy->mBsQueryNum;
    proxy->mBsQueryNum++;

    NPObject* callback = NULL;

    if (argCount == 1) {
        callback = NPVARIANT_TO_OBJECT(args[0]);
    }

    CopySuccessCallbackArgs* callbackArgs = new CopySuccessCallbackArgs(callback, queryNum);
    proxy->m_copySuccessCallbackArgs.add(callbackArgs);
    proxy->asyncCmdCopy(queryNum);

    return NULL;
}

const char* BrowserAdapter::js_paste(AdapterBase *adapter, const NPVariant *args, uint32_t argCount, NPVariant *result)
{
    BrowserAdapter* a = GetAndInitAdapter(adapter);
    if (!a->mBrowserServerConnected) {
        return "BrowserAdapter::paste Cannot connect to server.";
    }

    a->asyncCmdPaste();

    return NULL;
}

const char* BrowserAdapter::js_setMouseMode(AdapterBase *adapter, const NPVariant *args, uint32_t argCount, NPVariant *result)
{
    if (argCount != 1 || !IsIntegerVariant(&args[0]) ) {
        return "setMouseMode(number): Bad arguments.";
    }

    int mode = VariantToInteger(&args[0]);
    if (mode < 0 || mode > 1) {
        return "invalid mode.";
    }

    BrowserAdapter* a = GetAndInitAdapter(adapter);
    if (!a->mBrowserServerConnected) {
        return "Cannot connect to server.";
    }

    TRACEF("%d\n", mode);

    a->asyncCmdSetMouseMode(mode);
    a->mMouseMode = mode;

    return NULL;
}

const char* BrowserAdapter::js_clearSelection(AdapterBase *adapter, const NPVariant *args, uint32_t argCount, NPVariant *result)
{
    BrowserAdapter* a = GetAndInitAdapter(adapter);
    if (!a->mBrowserServerConnected) {
        return "BrowserAdapter::clearSelection Cannot connect to server.";
    }

    TRACE;

    a->asyncCmdClearSelection();

    return NULL;
}


const char* BrowserAdapter::js_mouseEvent(AdapterBase *adapter, const NPVariant *args, uint32_t argCount, NPVariant *result)
{
    if (argCount != 4 ||
            !IsIntegerVariant(args[0]) ||
            !IsIntegerVariant(args[1]) ||
            !IsIntegerVariant(args[2]) ||
            !IsIntegerVariant(args[3])) {
        return "BrowserAdapter::js_mouseEvent(number, number, number, number): Bad arguments.";
    }

    BrowserAdapter *a = GetAndInitAdapter(adapter);

    TRACEF("%d, %d, %d, %d, %f\n",
           VariantToInteger(args[0]),
           VariantToInteger(args[1]),
           VariantToInteger(args[2]),
           VariantToInteger(args[3]), a->mZoomLevel);

    gettimeofday(&a->m_lastPassedEventTime, NULL);
    a->asyncCmdMouseEvent(VariantToInteger(args[0]),  // up, down, drag
                          VariantToInteger(args[1]) / a->mZoomLevel,
                          VariantToInteger(args[2]) / a->mZoomLevel,
                          VariantToInteger(args[3]));

    return NULL;
}

/**
 * @brief Called by app when it believes BA is not connected to BS
 */
const char* BrowserAdapter::js_connectBrowserServer(AdapterBase *adapter, const NPVariant *args, uint32_t argCount, NPVariant *result)
{
    TRACE;
    BrowserAdapter *a = GetAndInitAdapter(adapter);

    if (!a->mBrowserServerConnected) {
        return "BrowserAdapter::connectBrowserServer Cannot connect to server.";
    }

    return NULL;  // success
}

/**
 * @brief Called by our host when they want the adapter to disconnect from the BrowserServer.
 *
 * @note Does not fire the browserServerDisconnected event.
 */
const char* BrowserAdapter::js_disconnectBrowserServer(AdapterBase *adapter, const NPVariant *args, uint32_t argCount, NPVariant *result)
{
    BrowserAdapter *a = GetAndInitAdapter(adapter);
    TRACE;

    a->asyncCmdDisconnect();

    return NULL;
}

/**
 * Prints the specified frame.  WebKit will render the frame to image file(s) and
 * notify the Print Manager Luna Service to send them to the printer.
 *
 * @param args[0] string
 *   The name of the frame to print.  If not specified, WebKit will render the main frame.
 * @param args[1] int
 *   The job ID associated with this print
 * @param args[2] int
 *   The printable width of the page in pixels
 * @param args[3] int
 *   The printable height of the page in pixels
 * @param args[4] int
 *   The print Dpi
 * @param args[5] bool
 *   The orientation of the page, TRUE=landscape, FALSE=portrait
 * @param args[6] bool
 *   The order to print pages, TRUE=last-to-first, FALSE=first-to-last
 *
 */
const char* BrowserAdapter::js_printFrame(AdapterBase *adapter, const NPVariant *args, uint32_t argCount, NPVariant *result)
{
    if (argCount != 7) {
        return "BrowserAdapter::printFrame: Bad arguments.";
    }

    BrowserAdapter *a = GetAndInitAdapter(adapter);
    if (!a->mBrowserServerConnected) {
        return "Not connected to server.";
    }

    char* frameName = NPStringToString(NPVARIANT_TO_STRING(args[0]));
    int lpsJobId = VariantToInteger(args[1]);
    int width = VariantToInteger(args[2]);
    int height = VariantToInteger(args[3]);
    int dpi = VariantToInteger(args[4]);
    bool landscape = VariantToBoolean(args[5]);
    bool reverseOrder = VariantToBoolean(args[6]);

    a->asyncCmdPrintFrame(frameName,lpsJobId,width,height,dpi,landscape,reverseOrder);
    ::free(frameName);

    return NULL;
}

const char* BrowserAdapter::js_skipPaintHack(AdapterBase *adapter, const NPVariant *args, uint32_t argCount, NPVariant *result)
{
    return NULL;
}

const char* BrowserAdapter::js_findInPage(AdapterBase *adapter, const NPVariant *args, uint32_t argCount, NPVariant *result)
{
    if ((argCount != 1) || NPVARIANT_IS_STRING(args[0]) == false) {
        return "BrowserAdapter::findInPage(string): Bad arguments.";
    }

    BrowserAdapter *a = GetAndInitAdapter(adapter);
    if (a->mBrowserServerConnected) {
        char *str = NPStringToString(NPVARIANT_TO_STRING(args[0]));
        TRACEF("%s\n", str);
        a->asyncCmdFindString(str, true);
        ::free(str);

        return NULL;
    }
    else {
        return "BrowserAdapter::openURL: Disconnected from server";
    }
}

const char* BrowserAdapter::js_mouseHoldAt(AdapterBase *adapter, const NPVariant *args, uint32_t argCount, NPVariant *result)
{
    if (argCount != 2 || IsIntegerVariant(args[0]) == false ||
            IsIntegerVariant(args[1]) == false) {
        return "BrowserAdapter::js_mouseHoldAt(int, int): Bad arguments.";
    }

    BrowserAdapter *a = GetAndInitAdapter(adapter);

    if (a->m_passInputEvents || a->m_spotlightHandle) {
        return NULL;
    }

    Point docPt(VariantToInteger(args[0]) / a->mZoomLevel,
                VariantToInteger(args[1]) / a->mZoomLevel);

    if (a->flashRectContainsPoint(docPt)) {
        return NULL;
    }

    a->m_sentMouseHoldEvent.set(docPt.x, docPt.y);
    a->asyncCmdHoldAt(docPt.x, docPt.y);

    return NULL;
}

const char* BrowserAdapter::js_handleFlick(AdapterBase *adapter, const NPVariant *args, uint32_t argCount, NPVariant *result)
{
    if (argCount != 2 || !IsIntegerVariant(args[0]) || !IsIntegerVariant(args[1])) {
        INT32_TO_NPVARIANT( kInvalidParam, *result );
        return "BrowserAdapter::handleFlick(): Bad arguments.";
    }

    BrowserAdapter *proxy = GetAndInitAdapter(adapter);
    if (proxy && !proxy->mScrollableLayerScrollSession.isActive)
        proxy->mScroller->handleMouseFlick(VariantToInteger(args[0]),
                                           VariantToInteger(args[1]));

    return NULL;
}

const char* BrowserAdapter::js_setVisibleSize(AdapterBase *adapter, const NPVariant *args, uint32_t argCount, NPVariant *result)
{
    if (argCount != 2 || !IsIntegerVariant(args[0]) || !IsIntegerVariant(args[1])) {
        INT32_TO_NPVARIANT(kInvalidParam, *result);
        return "BrowserAdapter::setVisibleSize(int, int): Bad arguments.";
    }

    BrowserAdapter *proxy = GetAndInitAdapter(adapter);

    int oldwidth = proxy->mViewportWidth;
    int oldheight = proxy->mViewportHeight;

    proxy->mViewportWidth = VariantToInteger(args[0]);
    proxy->mViewportHeight = VariantToInteger(args[1]);

    TRACEF("new size %dx%d", proxy->mViewportWidth, proxy->mViewportHeight);

    proxy->asyncCmdSetWindowSize(proxy->mViewportWidth, proxy->mViewportHeight);
    proxy->mScroller->setViewportDimensions(proxy->mViewportWidth, proxy->mViewportHeight);

    proxy->scrollCaretIntoViewAfterResize(oldwidth, oldheight,
                                          proxy->mZoomLevel);
    return NULL;
}

void BrowserAdapter::scrollCaretIntoViewAfterResize(int oldwidth, int oldheight, double oldZoomLevel)
{
    //int oldcaretx = m_textCaretRect.x() * oldZoomLevel;
    int oldcarety = m_textCaretRect.y() * oldZoomLevel;
    //int newcaretx = m_textCaretRect.x() * mZoomLevel;
    int newcarety = m_textCaretRect.y() * mZoomLevel;
    /*
    TRACEF("caret %d,%d %d,%d old %dx%d vp %dx%d scroll %d,%d zoom %f %f",
    oldcaretx, oldcarety, newcaretx, newcarety,
    oldwidth, oldheight, mViewportWidth, mViewportHeight,
    mScrollPos.x, mScrollPos.y, mZoomLevel, oldZoomLevel);
    */
    if ((m_textCaretRect.x() != 0 || m_textCaretRect.y() != 0)
            && oldheight != mViewportHeight
            && oldcarety > mScrollPos.y && oldcarety < mScrollPos.y + oldheight
            && (newcarety > mScrollPos.y + mViewportHeight
                || newcarety < mScrollPos.y)) {
        mScrollPos.y += (oldheight - mViewportHeight) + (newcarety - oldcarety);
        TRACEF("scrolling to %d %d", mScrollPos.x, mScrollPos.y);
        mScroller->scrollTo(-mScrollPos.x, -mScrollPos.y, false);
        invalidate();
    }
}

/**
 * Invalidate the entire page forcing it to be redrawn.
 */
void BrowserAdapter::invalidate()
{
    NPRect dirty;
    dirty.top = 0;
    dirty.left = 0;
    dirty.bottom = mWindow.height;
    dirty.right = mWindow.width;
    NPN_InvalidateRect(&dirty);
}

/**
 * Smart zoom the adapter view to indicated origin in page coordinates.
 *
 * @deprecated
 *
 * @param pt The point (in document coordinates) to calculate the smart zoom rectangle for.
 */
bool BrowserAdapter::prvSmartZoom(const Point& pt)
{
    init();

    // no double tap zoom if meta viewport sets user-scalable=false
    if (mMetaViewport && !mMetaViewport->userScalable) {
        return false;
    }

    asyncCmdZoomSmartCalculateRequest(pt.x, pt.y);

    return true;
}


/*
Yapper connection related callbacks
*/

void BrowserAdapter::serverConnected()
{
    TRACE;

    if (mServerConnectedInvoking) return;

    mBrowserServerConnected = true;
    mNotifiedOfBrowserServerDisconnect = false;

    if (mBrowserServerConnected && !mServerConnectedInvoked) {
        mServerConnectedInvoking = true; // prevent reentry
        mServerConnectedInvoked = InvokeEventListener(gServerConnectedHandler, NULL, 0, NULL);
        mServerConnectedInvoking = false;
    }
}

void BrowserAdapter::serverDisconnected()
{
    TRACE;
    m_useFastScaling = false;
    bEditorFocused = false;
    if (!mNotifiedOfBrowserServerDisconnect) { // notify once
        TRACEF("%s: no connection with BrowserServer", __FUNCTION__);
        if (InvokeEventListener(gBrowserServerDisconnectHandler, NULL, 0, NULL)) {
            mNotifiedOfBrowserServerDisconnect = true;
        }
        else {
            TRACEF("%s: Error informing WebView of disconnect!", __FUNCTION__);
        }
    }
    mBrowserServerConnected = false;
    mServerConnectedInvoked = false;
    mSendFinishDocumentLoadNotification = false;
}

/*
Server Message Handlers
*/

/**
 * The browser server just painted into it's buffer and we need to respond accordingly.
 *
 * All coordinates are scaled document with the origin at the TL of the page.
 *
 * @param rectX Left of painted rectangle.
 * @param rectY Top of painted rectangle.
 * @param rectWidth Width of painted rectangle.
 * @param rectHeight Height of painted rectangle.
 */
void BrowserAdapter::msgPainted(int32_t sharedBufferKey)
{
    VERBOSE_TRACE("BrowserAdapter::msgPaintedBuffer key: %d",
                  sharedBufferKey);

    if (mFrozen) {
        if (m_bufferLock)
            sem_post(m_bufferLock);
        return;
    }

    if (mFrozenSurface) {
        mFrozenSurface->releaseRef();
        mFrozenSurface = NULL;
    }

    int receivedBuffer = -1;
    if (mOffscreen0->ipcBuffer()->key() == sharedBufferKey)
        receivedBuffer = 0;
    else if (mOffscreen1->ipcBuffer()->key() == sharedBufferKey)
        receivedBuffer = 1;

    if (receivedBuffer < 0) {
        g_warning("Received shared buffer key is not ours: %d", sharedBufferKey);
        if (m_bufferLock)
            sem_post(m_bufferLock);
        return;
    }

    if (mOffscreenCurrent) {
        assert(mOffscreenCurrent->ipcBuffer()->key() != sharedBufferKey);
        asyncCmdReturnBuffer(mOffscreenCurrent->key());
        mOffscreenCurrent = 0;
    }

    mOffscreenCurrent = receivedBuffer == 0 ? mOffscreen0 : mOffscreen1;
    invalidate();

    if (m_bufferLock)
        sem_post(m_bufferLock);

    if (!mFirstPaintComplete) {
        InvokeEventListener(gFirstPaintCompleteHandler, NULL, 0, NULL);
        mFirstPaintComplete = true;
    }
}

void BrowserAdapter::msgReportError(const char* url, int32_t code, const char* msg)
{
    TRACEF("ReportErrorMsg: Url: %s, Code: %d Msg: %s", url, code, msg);

    NPVariant args[3];
    STRINGZ_TO_NPVARIANT(url, args[0]);
    INT32_TO_NPVARIANT(code, args[1]);
    STRINGZ_TO_NPVARIANT(msg, args[2]);

    InvokeEventListener(gReportErrorHandler, args, G_N_ELEMENTS(args), 0 );
}

void BrowserAdapter::msgEditorFocused(bool focused, int32_t fieldType, int32_t fieldActions)
{
    TRACEF("EditorFocused: %c", focused ? 'T' : 'F');
    bEditorFocused = focused;
    NPVariant args[3];
    BOOLEAN_TO_NPVARIANT(focused, args[0]);
    INT32_TO_NPVARIANT(fieldType, args[1]);
    INT32_TO_NPVARIANT(fieldActions, args[2]);
    InvokeEventListener(gEditorFocusedHandler, args, G_N_ELEMENTS(args), 0 );
}

void BrowserAdapter::msgFailedLoad(const char* domain, int errorCode,
                                   const char* failingURL, const char* localizedDescription)
{
    if(domain==NULL)
        domain="";
    if(failingURL ==NULL)
        failingURL="";
    if(localizedDescription==NULL)
        localizedDescription="";
    TRACEF("%s, %d, %s, %s", domain, errorCode, failingURL, localizedDescription);
    NPVariant args[4];
    STRINGZ_TO_NPVARIANT(domain, args[0]);
    INT32_TO_NPVARIANT(errorCode, args[1]);
    STRINGZ_TO_NPVARIANT(failingURL, args[2]);
    STRINGZ_TO_NPVARIANT(localizedDescription, args[3]);

    InvokeEventListener(gFailedLoadHandler, args, G_N_ELEMENTS(args), 0 );
}

void BrowserAdapter::msgSetMainDocumentError(const char* domain, int errorCode,
        const char* failingURL, const char* localizedDescription)
{
    TRACEF("%s, %d, %s, %s", domain, errorCode, failingURL, localizedDescription);
    NPVariant args[4];
    STRINGZ_TO_NPVARIANT(domain, args[0]);
    INT32_TO_NPVARIANT(errorCode, args[1]);
    STRINGZ_TO_NPVARIANT(failingURL, args[2]);
    STRINGZ_TO_NPVARIANT(localizedDescription, args[3]);

    InvokeEventListener(gMainDocumentErrorHandler, args, G_N_ELEMENTS(args), 0 );
}

/**
 * The size of the contents (page) changed.
 *
 * @param width Scaled page width.
 * @param height Scaled page height.
 */
void BrowserAdapter::msgContentsSizeChanged(int32_t width, int32_t height)
{
    TRACEF("%s: Content size: %dx%d zoom %f", __FUNCTION__, width, height, mZoomLevel);

    if (mPageWidth == width && mPageHeight == height)
        return;

    if (width == 0 && height == 0) {
        // First content size after a page load. Zoom fit
        delete mMetaViewport;
        mMetaViewport = 0;

        resetScrollableLayerScrollSession();
        mScrollableLayers.clear();

        mZoomFit = true;
        mZoomLevel = 1.0;
        mScroller->scrollTo(0, 0, false);
        mScrollPos.x = 0;
        mScrollPos.y = 0;

        // return any buffers we own, since they are stale at this point
        if (mOffscreenCurrent) {
            asyncCmdReturnBuffer(mOffscreenCurrent->key());
            mOffscreenCurrent = 0;
        }
    }

    if (mZoomFit && mWindow.width != 0 && width != 0) {
        mZoomLevel = mWindow.width * 1.0 / width;
        asyncCmdSetZoomAndScroll(mZoomLevel, mScrollPos.x, mScrollPos.y);
    }

    mPageWidth = width;
    mPageHeight = height;
    mContentWidth = ::round(mZoomLevel * width);
    mContentHeight = ::round(mZoomLevel * height);

    if (mMetaViewport && !mMetaViewport->userScalable
            && mContentHeight > mViewportHeight && mContentHeight <= (mViewportHeight + 2)) {
        // In testing, it was observed that for some sites, like maps.google.com, the mContentHeight
        // would initially equal the mViewportHeight and then slightly increase by 2 pixels.  When
        // mContentHeight > mViewportHeight, input events are not passed (see shouldPassInputEvents()),
        // along with a variety of other state changes within BrowserAdapter that negatively effected
        // sites that use touch events.
        mContentHeight = mViewportHeight;
    }

    mScroller->setContentDimensions(mContentWidth, mContentHeight + m_headerHeight);

    if (mShowHighlight) {
        removeHighlight();
    }

    // FIXME: RR this should not be needed anymore

    // Let listeners know about the size change
    NPVariant args[2];
    INT32_TO_NPVARIANT(width, args[0]);
    INT32_TO_NPVARIANT(height, args[1]);
    InvokeEventListener(gPageDimensionsHandler, args, G_N_ELEMENTS(args), NULL);
}

void BrowserAdapter::msgSmartZoomCalculateResponseSimple(int32_t inCenterX, int32_t inCenterY,
        int32_t inLeft, int32_t inTop,
        int32_t inRight, int32_t inBottom,
        int inSpotlightHandle)
{
    if (mPageWidth == 0 || mPageHeight == 0 ||
            mWindow.width == 0 || mWindow.height == 0)
        return;

    int viewportWidth = mWindow.width;
    int viewportHeight = mWindow.height;

    const int kMargin = 10;

    // fit by width
    double zoom = viewportWidth * 1.0 / (inRight - inLeft + kMargin * 2);

    // Special handling for flash double tap zoom. We want to fit the entire content
    // and so may need to adjust the zoom
    if (inSpotlightHandle) {
        double zoom2 = viewportHeight * 1.0 / (inBottom - inTop + kMargin * 2);
        if (zoom > zoom2) {
            zoom = zoom2;
        }
    }

    // zoom cap
    double maxZoom = mMetaViewport ? mMetaViewport->maximumScale : kMaxZoom;
    double minZoom = mMetaViewport ? mMetaViewport->minimumScale : 0.0;
    double fitZoom = viewportWidth * 1.0 / mPageWidth;
    minZoom = MAX(minZoom, fitZoom);

    if (zoom < minZoom)
        zoom = minZoom;

    if (zoom > maxZoom)
        zoom = maxZoom;

    int left = 0;
    int top = 0;

    if (PrvIsEqual(zoom, mZoomLevel)) {
        // zoom back to fit width
        zoom = fitZoom;
        left = 0;
        top = inCenterY * zoom - viewportHeight / 2;
    }
    else {

        // Center the rectangle

        // Horizontal center is in middle of rectangle
        int centerX = ((inLeft + inRight) / 2) * zoom;

        // Vertical center is at centerY
        int centerY = inCenterY * zoom;

        // But if the rectangle will fit height-wise within the viewport Height
        // we center in vertically in the middle of the rectangle
        if ((inBottom - inTop) * zoom < viewportHeight) {
            centerY = ((inTop + inBottom) / 2) * zoom;
        }

        left = centerX - viewportWidth / 2;
        top = centerY - viewportHeight / 2;
    }

    // Clamp on edges
    int maxLeft = mPageWidth * zoom - viewportWidth;
    int maxTop = mPageHeight * zoom - viewportHeight;

    left = MIN(maxLeft, left);
    top = MIN(maxTop, top);

    left = MAX(left, 0);
    top = MAX(top, 0);

    startZoomAnimation(zoom, left, top);
}

void BrowserAdapter::msgMakePointVisible(int32_t x, int32_t y)
{
    TRACEF("%d %d", x, y);
    /* FIXME: RR
        int scaledX = x * mZoomLevel + mJsScrollX;
        int scaledY = y * mZoomLevel + mJsScrollY;

    TRACEF("MSG: MakePointVisible %d, %d\n", x, y);

        int scrollAlongX = 0;
        if (scaledX < 0) {
            scrollAlongX = scaledX; // negative number that will make us scroll left
        } else if (scaledX > mViewportWidth) {
            scrollAlongX = scaledX - mViewportWidth; // positive number to scroll right
        }

        int scrollAlongY = 0;
        if (scaledY < 0) {
            scrollAlongY = scaledY;  // negative will move us up
        } else if (scaledY > (mViewportHeight - 25)) {  // 25 gives us some breathing room
            scrollAlongY = scaledY - (mViewportHeight - 25);
        }


        if (scrollAlongX != 0 || scrollAlongY != 0) {
            //printf("%s: I want to scroll along X: %d, along Y: %d\n", __FUNCTION__,
            //        scrollAlongX, scrollAlongY);

            mWkScrollX += scrollAlongX;
            mWkScrollY += scrollAlongY;

            msgScrolledTo(mWkScrollX, mWkScrollY);
        }
    */
}

/**
 * The browser just scrolled to a new position. Will notify the offscreen
 * buffer of the scroll change and invalidate the screen to get it to redraw.
 *
 * @param scrollx The scaled left coordinate.
 * @param scrolly The scaled top coordinate.
 */
void BrowserAdapter::msgScrolledTo(int32_t scrollx, int32_t scrolly)
{
    mScrollPos.x = MIN(::round(scrollx * mZoomLevel), MAX(mContentWidth - mViewportWidth, 0));;
    mScrollPos.y = MIN(::round(scrolly * mZoomLevel), MAX(mContentHeight - mViewportHeight, 0));
    TRACEF("(%d, %d) (%d, %d) c %dx%d vp %dx%d", scrollx, scrolly, mScrollPos.x, mScrollPos.y, mContentWidth, mContentHeight, mViewportWidth, mViewportHeight);
    mScroller->scrollTo(-mScrollPos.x, -mScrollPos.y, false);
    /* FIXME: RR
    TRACEF("(%d, %d)",scrollx, scrolly);

    mWkScrollX = scrollx;
    mWkScrollY = scrolly;

    scrollx = lround(mWkScrollX * mZoomLevel);
    scrolly = lround(mWkScrollY * mZoomLevel);

    if( gScrollToHandler )
    {
    NPVariant args[2];
    INT32_TO_NPVARIANT(scrollx,args[0]);
    INT32_TO_NPVARIANT(scrolly,args[1]);
    InvokeEventListener(gScrollToHandler, args, G_N_ELEMENTS(args), 0 );
    }
    */
}

void BrowserAdapter::msgLoadStarted()
{
    TRACE;

    mFirstPaintComplete = false;

    if ( NULL != gLoadStartedHandler ) {
        InvokeEventListener(gLoadStartedHandler, NULL, 0, NULL);
    }
}

void BrowserAdapter::msgLoadStopped()
{
    TRACE;

    if ( NULL != gLoadStoppedHandler ) {
        InvokeEventListener(gLoadStoppedHandler, NULL, 0, NULL);
    }

}

void BrowserAdapter::jsonToRects(const char* rectsArrayJson)
{
    if (!rectsArrayJson) {
        TRACEF("%s: invoked with invalid parameter, rectsArrayJson = %s\n.", __FUNCTION__, rectsArrayJson);
        return;
    }

    int numRects = 0;

    // parse out rectangle coordinates
    pbnjson::JValue rectsArray;
    pbnjson::JDomParser parser(NULL);
    pbnjson::JSchemaFile schema("/etc/palm/browser/InteractiveWidgetRect.schema");

    if (!parser.parse(rectsArrayJson, schema, NULL)) {
        TRACEF("%s: unable to parse string '%s'\n", __FUNCTION__, rectsArrayJson);
        goto Done;
    }

    rectsArray = parser.getDom();
    numRects = (int) rectsArray.arraySize();

    for (int i = 0; i < numRects; ++i)
    {
        pbnjson::JValue rectObj = rectsArray[i];
        int left, top, right, bottom;
        uintptr_t id;
        InteractiveRectType type;

        id = (uintptr_t) rectObj["id"].asNumber<int64_t>();
        left = rectObj["left"].asNumber<int>();
        top = rectObj["top"].asNumber<int>();
        right = rectObj["right"].asNumber<int>();
        bottom = rectObj["bottom"].asNumber<int>();
        type = (InteractiveRectType) rectObj["type"].asNumber<int>();

        BrowserRect rect(left, top, right-left, bottom-top);

        switch (type) {
        case InteractiveRectDefault:
            mDefaultInteractiveRects.insert(RectMapItem(id, rect));
            TRACEF("inserting rect: type: %d, id: %d, left: %d, top: %d, width: %d, height: %d, new count: %d",
                   (int)type, id, rect.x(), rect.y(), rect.w(), rect.h(), mDefaultInteractiveRects.size());
            break;
        case InteractiveRectPlugin:
            mFlashRects.insert(RectMapItem(id, rect));
            TRACEF("inserting rect: type: %d, id: %d, left: %d, top: %d, width: %d, height: %d, new count: %d",
                   (int)type, id, rect.x(), rect.y(), rect.w(), rect.h(), mFlashRects.size());
            break;
        default:
            g_debug("Unrecognized rect type: %d", type);
            break;
        }
    }

Done:
    return;
}

void BrowserAdapter::msgAddFlashRects(const char* rectsArrayJson)
{
    TRACEF("ADD RECTS!: %s", rectsArrayJson);
    jsonToRects(rectsArrayJson);
}

void BrowserAdapter::msgRemoveFlashRects(const char* rectIdJson)
{
    TRACEF("REMOVE RECTS!: %s", rectIdJson);

    // numeric id and type are passed
    pbnjson::JValue rectId;
    pbnjson::JDomParser parser(NULL);
    pbnjson::JSchemaFragment schema("{}");

    if (!parser.parse(rectIdJson, schema, NULL)) {
        TRACEF("%s: unable to parse string '%s'\n", __FUNCTION__, rectIdJson);
        return;
    }

    rectId = parser.getDom();
    uintptr_t id = (uintptr_t) rectId["id"].asNumber<int64_t>();
    InteractiveRectType type = (InteractiveRectType) rectId["type"].asNumber<int>();

    switch (type) {
    case InteractiveRectDefault:
        mDefaultInteractiveRects.erase(id);
        TRACEF("Removing rect type: %d, id: %d, new count: %d", type, id, mDefaultInteractiveRects.size());
        break;
    case InteractiveRectPlugin:
        mFlashRects.erase(id);
        TRACEF("Removing rect type: %d, id: %d, new count: %d", type, id, mFlashRects.size());
        break;
    default:
        g_debug("Unrecognized rect type: %d", type);
        break;
    }

    return;
}

void BrowserAdapter::msgShowPrintDialog()
{
    InvokeEventListener(gShowPrintDialogHandler, NULL, 0, NULL);
}

void BrowserAdapter::msgGetTextCaretBoundsResponse(int32_t queryNum, int32_t left, int32_t top, int32_t right, int32_t bottom)
{
    TRACEF("%d %d %d %d", left, top, right, bottom);
    if (left && top && right && bottom
            && left != -1 && top != -1 && right != -1 && bottom != -1) {
        m_textCaretRect.setX(left);
        m_textCaretRect.setY(top);
        m_textCaretRect.setWidth(right - left);
        m_textCaretRect.setHeight(bottom - top);
    } else {
        m_textCaretRect.setX(0);
        m_textCaretRect.setY(0);
        m_textCaretRect.setWidth(0);
        m_textCaretRect.setHeight(0);
    }
}

void BrowserAdapter::msgUpdateGlobalHistory(const char* url, bool reload)
{
    NPVariant args[2];
    STRINGZ_TO_NPVARIANT(url, args[0]);
    BOOLEAN_TO_NPVARIANT(reload,args[1]);
    InvokeEventListener(gUpdateGlobalHistoryHandler, args, G_N_ELEMENTS(args), 0 );
}

void BrowserAdapter::msgDidFinishDocumentLoad()
{
    TRACE;

    InvokeEventListener(gDidFinishDocumentLoadHandler, NULL, 0, NULL);
    mSendFinishDocumentLoadNotification = false;
}

void BrowserAdapter::msgLoadProgress(int32_t progress)
{
    NPVariant arg;
    INT32_TO_NPVARIANT(progress, arg);

    if ( NULL != gLoadProgressHandler ) {
        InvokeEventListener(gLoadProgressHandler, &arg, 1, NULL);
    }
}

void BrowserAdapter::msgLocationChanged(const char* uri, bool canGoBack, bool canGoForward)
{
    TRACEF("%s", uri);

    NPVariant args[3];

    STRINGZ_TO_NPVARIANT(uri, args[0]);
    BOOLEAN_TO_NPVARIANT(canGoBack, args[1]);
    BOOLEAN_TO_NPVARIANT(canGoForward, args[2]);

    InvokeEventListener(gUrlChangeHandler, args, G_N_ELEMENTS(args), NULL);
}

void BrowserAdapter::msgTitleChanged(const char* title)
{
    TRACEF("%s", title);

    NPVariant args[1];

    STRINGZ_TO_NPVARIANT(title, args[0]);

    InvokeEventListener(gTitleChangeHandler, args, G_N_ELEMENTS(args), NULL);
}

void BrowserAdapter::msgTitleAndUrlChanged(const char* title, const char* url, bool canGoBack, bool canGoForward)
{
    TRACEF("Title&Url Change: %s, %s", title, url);

    NPVariant args[4];

    if( !url )
        url = "";

    if( !title )
        title = "";

    STRINGZ_TO_NPVARIANT(title, args[0]);
    STRINGZ_TO_NPVARIANT(url, args[1]);
    BOOLEAN_TO_NPVARIANT(canGoBack, args[2]);
    BOOLEAN_TO_NPVARIANT(canGoForward, args[3]);

    InvokeEventListener(gTitleURLChangeHandler, args, G_N_ELEMENTS(args), NULL);
}

void BrowserAdapter::msgDialogAlert(const char* syncPipePath, const char* msg)
{
    TRACEF("Password Dialog: %s", msg);

    if (mLogAlertMessages) {
        FILE* file = fopen("/tmp/browser-alert_message.txt", "w+");
        if (file != NULL) {
            if (msg == NULL)
                fputs("<NULL>", file);// just to be safe.
            else
                fputs(msg, file);
            fclose(file);
        }
    }

    strncpy(gDialogResponsePipe, syncPipePath, G_N_ELEMENTS(gDialogResponsePipe));
    gDialogResponsePipe[G_N_ELEMENTS(gDialogResponsePipe)-1] = '\0';

    NPVariant args[1];
    STRINGZ_TO_NPVARIANT(msg, args[0]);

    InvokeEventListener(gDialogAlertHandler, args, G_N_ELEMENTS(args), NULL);
}

void BrowserAdapter::msgDialogConfirm(const char* syncPipePath, const char* msg)
{
    TRACEF("Confirm Dialog: %s", msg);
    strncpy(gDialogResponsePipe, syncPipePath, G_N_ELEMENTS(gDialogResponsePipe));
    gDialogResponsePipe[G_N_ELEMENTS(gDialogResponsePipe)-1] = '\0';

    NPVariant args[1];
    STRINGZ_TO_NPVARIANT(msg, args[0]);

    InvokeEventListener(gDialogConfirmHandler, args, G_N_ELEMENTS(args), NULL);
}

void BrowserAdapter::msgDialogSSLConfirm(const char* syncPipePath, const char* host,int32_t code, const char* certFile)
{
    TRACEF("Confirm SSL Dialog: host:%s, code:%d, certFile:%s", host, code, certFile);
    strncpy(gDialogResponsePipe, syncPipePath, G_N_ELEMENTS(gDialogResponsePipe));
    gDialogResponsePipe[G_N_ELEMENTS(gDialogResponsePipe)-1] = '\0';

    NPVariant args[3];
    STRINGZ_TO_NPVARIANT(host, args[0]);
    INT32_TO_NPVARIANT(code,args[1]);
    STRINGZ_TO_NPVARIANT(certFile, args[2]);

    InvokeEventListener(gDialogConfirmSSLHandler, args, G_N_ELEMENTS(args), NULL);
}

void BrowserAdapter::msgDialogPrompt(const char* syncPipePath, const char* msg, const char* defaultValue)
{
    TRACEF("Prompt Dialog: %s (%s)", msg, defaultValue);
    strncpy(gDialogResponsePipe, syncPipePath, G_N_ELEMENTS(gDialogResponsePipe));
    gDialogResponsePipe[G_N_ELEMENTS(gDialogResponsePipe)-1] = '\0';

    NPVariant args[2];
    STRINGZ_TO_NPVARIANT(msg, args[0]);
    STRINGZ_TO_NPVARIANT(defaultValue, args[1]);

    InvokeEventListener(gDialogPromptHandler, args, G_N_ELEMENTS(args), NULL);
}

void BrowserAdapter::msgDialogUserPassword(const char* syncPipePath, const char* msg)
{
    TRACEF("Password Dialog: %s", msg);
    strncpy(gDialogResponsePipe, syncPipePath, G_N_ELEMENTS(gDialogResponsePipe));
    gDialogResponsePipe[G_N_ELEMENTS(gDialogResponsePipe)-1] = '\0';

    NPVariant args[1];
    STRINGZ_TO_NPVARIANT(msg, args[0]);

    InvokeEventListener(gDialogUserPasswordHandler, args, G_N_ELEMENTS(args), NULL);
}

/**
 * Sent by the browser server when the client needs to display a popoup menu.
 */
void BrowserAdapter::msgPopupMenuShow(const char* menuId, const char* menuDataFileName)
{
    TRACEF("showPopupMenu: %s", menuId);

    FILE* file = ::fopen(menuDataFileName, "rb");
    if (NULL != file) {

        ::fseek(file, 0, SEEK_END);
        long fsize = ::ftell(file);
        ::fseek(file, 0, SEEK_SET);
        char* menuData = new char[fsize+1];
        if (NULL != menuData) {
            if (static_cast<size_t>(fsize) == ::fread(menuData, sizeof(char), fsize, file)) {
                menuData[fsize] = '\0';

                NPVariant args[2];
                STRINGZ_TO_NPVARIANT(menuId, args[0]);
                STRINGZ_TO_NPVARIANT(menuData, args[1]);

                InvokeEventListener(gPopupMenuShowHandler, args, G_N_ELEMENTS(args), NULL);
            }
            else {
                TRACEF("ERROR reading menu data from file.");
                // If this fails then it's either a program error or something horrible going
                // wrong on the device. Not responding to this message won't cause WebKit to hang.
            }
            delete [] menuData;
        }

        ::fclose(file);
        ::unlink(menuDataFileName);
    }
}

/**
 * Sent by the browser server when the client needs to hide a popoup menu.
 */
void BrowserAdapter::msgPopupMenuHide(const char* menuId)
{
    TRACEF("hidePopupMenu: %s", menuId);

    NPVariant args[1];
    STRINGZ_TO_NPVARIANT(menuId, args[0]);

    InvokeEventListener(gPopupMenuHideHandler, args, G_N_ELEMENTS(args), NULL);
}

void BrowserAdapter::msgActionData(const char* dataType, const char* data)
{
    NPVariant args[2];

    TRACEF("ActionData: dataType: %s data: %s", dataType, data);
    STRINGZ_TO_NPVARIANT(dataType, args[0]);
    STRINGZ_TO_NPVARIANT(data, args[1]);

    InvokeEventListener(gActionDataHandler, args, G_N_ELEMENTS(args), NULL);
}

void BrowserAdapter::msgDownloadStart(const char* url)
{
    TRACEF("DownloadStartMsg: %s", url);

    NPVariant args[1];

    STRINGZ_TO_NPVARIANT(url, args[0]);

    InvokeEventListener(gDownloadStartHandler, args, G_N_ELEMENTS(args), NULL);
}

void BrowserAdapter::msgDownloadProgress(const char* url, int32_t totalSizeSoFar, int32_t totalSize)
{
    TRACEF("DownloadProgressMsg: Url: %s totalSizeSoFar %d totalSize %d",
           url, totalSizeSoFar, totalSize);

    NPVariant args[3];

    STRINGZ_TO_NPVARIANT(url, args[0]);
    INT32_TO_NPVARIANT(totalSizeSoFar, args[1]);
    INT32_TO_NPVARIANT(totalSize, args[2]);

    InvokeEventListener(gDownloadProgressHandler, args, G_N_ELEMENTS(args), NULL);
}

void BrowserAdapter::msgDownloadError(const char* url, const char* errorMsg)
{
    TRACEF("DownloadErrorMsg: Url: %s errorMsg: %s",
           url, errorMsg);

    NPVariant args[2];

    STRINGZ_TO_NPVARIANT(url, args[0]);
    STRINGZ_TO_NPVARIANT(errorMsg, args[1]);

    InvokeEventListener(gDownloadErrorHandler, args, G_N_ELEMENTS(args), NULL);
}

/**
 * @brief creates vector of BrowserAdapterRectangleS that cover an interactive
 *        element that will be highlighted at the next paint.
 *
 */
void BrowserAdapter::msgHighlightRects(const char* rectsArrayJson)
{
    TRACEF("%s: rects array: %s", __FUNCTION__, rectsArrayJson);

    // clear out previous rectangles
    removeHighlight();

    if (!rectsArrayJson)
    {
        // ignore the message if the parameter is invalid. This may happen if the list
        // of json objects was too long to fit into the Yap buffers, like when selecting a very
        // long URL (displayed in several lines) on the screen.
        TRACEF("%s: invoked with invalid parameter, rectsArrayJson = %s\n.", __FUNCTION__, rectsArrayJson);
        return;
    }

    bool success = false;
    int numRects = 0;

    // parse out rectangle coordinates
    json_object* rectsArrayRoot = json_tokener_parse(rectsArrayJson);
    array_list* rectsArray;


    if (!rectsArrayRoot || is_error(rectsArrayRoot)) {
        rectsArrayRoot = NULL;
        TRACEF("%s: unable to parse string '%s'\n", __FUNCTION__, rectsArrayJson);
        goto Done;
    }

    rectsArray = json_object_get_array(rectsArrayRoot);
    if (!rectsArray || is_error(rectsArray)) {
        TRACEF("%s: unable to get array from '%s'\n", __FUNCTION__, json_object_get_string(rectsArrayRoot));
        goto Done;
    }

    numRects = array_list_length(rectsArray);
    for (int i = 0; i < numRects; ++i)
    {
        json_object* rectJson = (json_object*) array_list_get_idx(rectsArray, i);
        int left, top, right, bottom;

        json_object* coord = json_object_object_get(rectJson, "left");
        left = json_object_get_int(coord);

        coord = json_object_object_get(rectJson, "top");
        if (!coord || is_error(coord)) {
            goto Done;
        }
        top = json_object_get_int(coord);

        coord = json_object_object_get(rectJson, "right");
        if (!coord || is_error(coord)) {
            goto Done;
        }
        right = json_object_get_int(coord);

        coord = json_object_object_get(rectJson, "bottom");
        if (!coord || is_error(coord)) {
            goto Done;
        }
        bottom = json_object_get_int(coord);

        BrowserRect rect(left, top, right-left, bottom-top);
        mHighlightRects.push_back(rect);

    }
    success = true;

Done:
    if (rectsArrayRoot) {
        json_object_put(rectsArrayRoot);
    }

    if (success) {
        for (std::vector<BrowserRect>::const_iterator rect_iter = mHighlightRects.begin();
                rect_iter != mHighlightRects.end();
                ++rect_iter)
        {
            TRACEF("%s: will draw rect with left: %d, top: %d, right: %d, bottom: %d\n",
                   __FUNCTION__, (*rect_iter).x(), (*rect_iter).y(), (*rect_iter).r(), (*rect_iter).b());
        }

        invalidateHighlightRectsRegion();  // force area under new rectangles to be redrawn

        mShowHighlight = true;

    } else {
        TRACEF("%s: some json failure\n", __FUNCTION__);
        removeHighlight();
    }
}

void BrowserAdapter::msgLinkClicked(const char* url)
{
    TRACEF("LinkClickedMsg: Url: %s", url);
    NPVariant args[1];

    STRINGZ_TO_NPVARIANT(url, args[0]);

    InvokeEventListener(gLinkClickedHandler, args, G_N_ELEMENTS(args), NULL);
}

void BrowserAdapter::msgDownloadFinished(const char* url, const char* mimeType, const char* tmpFilePath)
{
    TRACEF("DownloadFinishedMsg: Url: %s  mimeType: %s  tmpFilePath: %s",url, mimeType, tmpFilePath);

    NPVariant args[3];

    STRINGZ_TO_NPVARIANT(url, args[0]);
    STRINGZ_TO_NPVARIANT(mimeType, args[1]);
    STRINGZ_TO_NPVARIANT(tmpFilePath, args[2]);

    InvokeEventListener(gDownloadFinishedHandler, args, G_N_ELEMENTS(args), NULL);
}

void BrowserAdapter::msgMimeHandoffUrl(const char* mimeType, const char* url)
{
    TRACEF("MimeHandoffUrlMsg: mimeType: %s  Url: %s", mimeType, url);

    NPVariant args[2];

    STRINGZ_TO_NPVARIANT(mimeType, args[0]);
    STRINGZ_TO_NPVARIANT(url, args[1]);

    InvokeEventListener(gMimeHandoffUrlHandler, args, G_N_ELEMENTS(args), NULL);
}

void BrowserAdapter::msgMimeNotSupported(const char* mimeType, const char* url)
{
    TRACEF("MimeNotSupportedMsg: mimeType: %s  Url: %s", mimeType, url);

    NPVariant args[2];

    STRINGZ_TO_NPVARIANT(mimeType, args[0]);
    STRINGZ_TO_NPVARIANT(url, args[1]);

    InvokeEventListener(gMimeNotSupportedHandler, args, G_N_ELEMENTS(args), NULL);
}

void BrowserAdapter::msgCreatePage(int32_t identifier)
{
    TRACEF("CreatePageMsg: identifier: %d\n", identifier);

    char idBuff[24];  // 32-bit %d will not be longer than 11 decimal places, 64-bit < 21 places

    ::snprintf(idBuff, G_N_ELEMENTS(idBuff), "%d", identifier);

    NPVariant args[1];

    STRINGZ_TO_NPVARIANT(idBuff, args[0]);

    InvokeEventListener(gCreatePageHandler, args, G_N_ELEMENTS(args), NULL);
}


void BrowserAdapter::msgClickRejected(int32_t counter)
{
    TRACEF("ClickRejected: %d", counter);

    NPVariant args[1];

    INT32_TO_NPVARIANT(counter, args[0]);

    InvokeEventListener(gClickRejectedHandler, args, G_N_ELEMENTS(args), NULL);
}

void BrowserAdapter::scale(double zoom)
{
    TRACEF("Scale: %g", zoom);
    mZoomLevel = zoom;

}

void BrowserAdapter::scaleAndScrollTo(double zoom, int x, int y)
{
    // I believe this can be safely deprectated. Should do so after Browser's WebView
    // widget is merged into the framework.
}

/**
 * BrowserServer has requested that we purge this page. We're expected to do
 * so if this page is not focused.
 */
void BrowserAdapter::msgPurgePage()
{
    bool active = isActivated();

    g_message("Requested to purge this adapter. Active=%c", active ? 'Y' : 'N');

    if (active) {
        g_warning("Not disconnecting because adapter is active.");
        // Sending down a focused message will raise the page priority and then another
        // inactive adapter will be told to purge its DOM next time.
        asyncCmdPageFocused(true);
    }
    else {
        g_warning("Disconnecting as requested");
        asyncCmdDisconnect();
    }
}

void BrowserAdapter::setPageIdentifier(int32_t identifier)
{
    mPageIdentifier = identifier;
}

void BrowserAdapter::enableFastScaling(bool enable)
{
    if (enable != m_useFastScaling) {
        m_useFastScaling = enable;

        // trigger a repaint when we disable fast scaling
        if (!m_useFastScaling) {
            // FIXME: RR
        } else {
            m_startingZoomLevel = mZoomLevel;
        }
    }
}

void BrowserAdapter::msgInspectUrlAtPointResponse(int32_t queryNum, bool succeeded,
        const char* url, const char* desc, int32_t rectWidth,
        int32_t rectHeight, int32_t rectX, int32_t rectY)
{
    TRACEF("Got inspectUrlAtPoint response qn=%d success=%c url='%s' desc='%s' (%d, %d) %d x %d.\n",
           queryNum, succeeded ? 'T' : 'F', url, desc, rectX, rectY, rectWidth, rectHeight);

    std::auto_ptr<InspectUrlAtPointArgs> args( m_inspectUrlArgs.get(queryNum) );

    if (NULL != args.get()) {
        NPVariant jsCallResult, jsCallArgs;

        NPObject* info = NPN_CreateObject(&UrlInfo::sUrlInfoClass);
        if (info) {
            OBJECT_TO_NPVARIANT(info, jsCallArgs);
            static_cast<UrlInfo*>(info)->initialize(succeeded, url, desc, rectX, rectY, rectX + rectWidth, rectY + rectHeight);
            if (NPN_InvokeDefault(args->m_successCb, &jsCallArgs, 1, &jsCallResult))
                TRACEF("inspectUrlAtPoint response call success.");
            else
                TRACEF("inspectUrlAtPoint response call FAILED.");

            AdapterBase::NPN_ReleaseVariantValue(&jsCallArgs);
        }
        else {
            TRACEF("InspectUrlAtPointResponse: response obj NOT created.");
        }
    }
    else {
        TRACEF("Can't find args");
    }
}

void BrowserAdapter::msgUrlRedirected(const char* url, const char* userData)
{
    TRACEF("Got URL redirect: '%s' -> '%s'\n", url, userData);
    NPVariant args[2];
    STRINGZ_TO_NPVARIANT(url, args[0]);
    STRINGZ_TO_NPVARIANT(userData, args[1]);
    InvokeEventListener(gUrlRedirectedHandler, args, G_N_ELEMENTS(args), NULL);
}

void BrowserAdapter::msgIsEditing(int32_t queryNum, bool isEditing)
{
    TRACEF("is editable field in focus? %s\n", isEditing? "True" : "False");

    std::auto_ptr<IsEditingArgs> args(m_isEditingArgs.get(queryNum));

    if (!args.get()) {
        TRACEF("Args requests are overlapping");
        return;
    }

    NPVariant jsCallResult;
    NPVariant jsCallArgs[1];

    BOOLEAN_TO_NPVARIANT(isEditing, jsCallArgs[0]);

    if (NPN_InvokeDefault(args->m_callback, jsCallArgs, G_N_ELEMENTS(jsCallArgs), &jsCallResult)) {
        TRACEF("isEditing response call success.");
    }
    else {
        TRACEF("isEditing response call FAILED.");
    }
}

void BrowserAdapter::msgGetHistoryStateResponse(int32_t queryNum, bool canGoBack, bool canGoForward)
{
    TRACEF("Got historyState response back=%c, forward=%c\n", canGoBack ? 'Y' : 'N',
           canGoForward ? 'Y' : 'N' );

    std::auto_ptr<GetHistoryStateArgs> args( m_historyStateArgs.get(queryNum) );

    if (NULL != args.get()) {
        NPVariant jsCallResult;
        NPVariant jsCallArgs[2];

        BOOLEAN_TO_NPVARIANT(canGoBack, jsCallArgs[0]);
        BOOLEAN_TO_NPVARIANT(canGoForward, jsCallArgs[1]);
        if (NPN_InvokeDefault(args->m_successCb, jsCallArgs, G_N_ELEMENTS(jsCallArgs), &jsCallResult))
            TRACEF("getHistoryState response call success.");
        else
            TRACEF("getHistoryState response call FAILED.");
    }
    else {
        TRACEF("Can't find args");
    }
}


void BrowserAdapter::msgMetaViewportSet(double initialScale, double minimumScale, double maximumScale,
                                        int32_t width, int32_t height, bool userScalable)
{
    TRACEF("Got metaViewportSet: scale: %g, %g, %g, dimensions: %d, %d, userScalable: %d\n",
           initialScale, minimumScale, maximumScale, width, height, userScalable);

    if (!mMetaViewport)
        mMetaViewport = new BrowserMetaViewport;

    mMetaViewport->initialScale = initialScale;
    mMetaViewport->minimumScale = minimumScale;
    mMetaViewport->maximumScale = maximumScale;
    mMetaViewport->userScalable = userScalable;

    NPVariant args[6];
    DOUBLE_TO_NPVARIANT(initialScale, args[0]);
    DOUBLE_TO_NPVARIANT(minimumScale, args[1]);
    DOUBLE_TO_NPVARIANT(maximumScale, args[2]);
    INT32_TO_NPVARIANT(width, args[3]);
    INT32_TO_NPVARIANT(height, args[4]);
    BOOLEAN_TO_NPVARIANT(userScalable, args[5]);

    InvokeEventListener(gMetaViewportSetHandler, args, G_N_ELEMENTS(args), NULL);
}

void BrowserAdapter::msgCopySuccessResponse(int32_t queryNum, bool isEditing)
{
    std::auto_ptr<CopySuccessCallbackArgs> args(m_copySuccessCallbackArgs.get(queryNum));

    if (!args.get()) {
        TRACEF("Args requests are overlapping");
        return;
    }

    NPVariant jsCallResult;
    NPVariant jsCallArgs[1];

    BOOLEAN_TO_NPVARIANT(isEditing, jsCallArgs[0]);

    if (args->m_callback) {  // a callback was provided
        if (NPN_InvokeDefault(args->m_callback, jsCallArgs, G_N_ELEMENTS(jsCallArgs), &jsCallResult)) {
            TRACEF("copy response call success.");
        }
        else {
            TRACEF("copy response call FAILED.");
        }
    }
}
bool BrowserAdapter::isFrozen()
{
    return mFrozen;
}

void BrowserAdapter::freeze()
{
    const char* identifier = (const char*) NPN_GetValue((NPNVariable) npPalmApplicationIdentifier);
    g_message("%s: %p: %s", __PRETTY_FUNCTION__, this, identifier);

    if (mFrozen)
        return;

    mFrozen = true;

    if (mFrozenSurface) {
        mFrozenSurface->releaseRef();
        mFrozenSurface = 0;
    }

    if (mOffscreenCurrent && mOffscreenCurrent->surface()) {

        BrowserOffscreenInfo* info = mOffscreenCurrent->header();
        PGSurface* offscreenSurf = mOffscreenCurrent->surface();

        int width = offscreenSurf->width() * kFrozenSurfaceScale;
        int height = offscreenSurf->height() * kFrozenSurfaceScale;

        mFrozenSurface = PGSurface::create(width, height);

        PGContext* ctxt = PGContext::create();
        ctxt->setSurface(mFrozenSurface);

        ctxt->bitblt(offscreenSurf,
                     0, 0, (int) offscreenSurf->width(), (int) offscreenSurf->height(),
                     0, 0, width, height);

        ctxt->releaseRef();

        mFrozenRenderPos.x = info->renderedX;
        mFrozenRenderPos.y = info->renderedY;
        mFrozenRenderWidth = info->renderedWidth;
        mFrozenRenderHeight = info->renderedHeight;
        mFrozenZoomFactor = info->contentZoom;
    }


    // ---------------------------------------------------------------

    delete mOffscreen0;
    mOffscreen0 = 0;

    delete mOffscreen1;
    mOffscreen1 = 0;

    mOffscreenCurrent = 0;

    asyncCmdFreeze();
}

void BrowserAdapter::thaw()
{
    const char* identifier = (const char*) NPN_GetValue((NPNVariable) npPalmApplicationIdentifier);
    g_message("%s: %p: %s", __PRETTY_FUNCTION__, this, identifier);

    if (!mFrozen)
        return;

    TRACEF("BrowserAdapter::thaw %p\n", this);

    mFrozen = false;

    if (!init()) {
        g_critical("%s: failed to initialize adapter", __PRETTY_FUNCTION__);
        return;
    }

    asyncCmdThaw(mOffscreen0->key(), mOffscreen1->key(),
                 mOffscreen0->size());

    // don't release frozen at this point, wait msgPainted event coming back!
}

void BrowserAdapter::handlePaintInFrozenState(NpPalmDrawEvent* event)
{
    if (!mFrozenSurface) {
        TRACEF("Frozen surface null");
        return;
    }

    PGContext* gc = (PGContext*) event->graphicsContext;
    gc->push();

    gc->translate(mWindow.x, mWindow.y);
    gc->addClipRect(0, 0, mWindow.width, mWindow.height);
    gc->translate(-mScrollPos.x, -mScrollPos.y);

    // -- Checkerboard ---------------------------------------------------

    BrowserRect windowRect(mScrollPos.x,
                           mScrollPos.y + m_headerHeight,
                           mWindow.width,
                           mWindow.height - m_headerHeight);

    int pw = mDirtyPattern->width();
    int ph = mDirtyPattern->height();

    int dstX = windowRect.x() % pw - pw;
    int dstY = windowRect.y() % ph - ph;
    int dstR = windowRect.r() % pw + pw + windowRect.w();
    int dstB = windowRect.b() % ph + ph + windowRect.h();

    gc->drawPattern(mDirtyPattern,
                    windowRect.x() % pw,
                    windowRect.y() % ph,
                    dstX, dstY, dstR, dstB);

    // -- Scaled draw of frozen surface ----------------------------------


    int centerOfSurfX = mFrozenRenderPos.x + mFrozenRenderWidth / 2;
    int centerOfSurfY = mFrozenRenderPos.y + mFrozenRenderHeight / 2;

    float zoomFactor = mZoomLevel / mFrozenZoomFactor;

    centerOfSurfX *= zoomFactor;
    centerOfSurfY *= zoomFactor;

    gc->translate(centerOfSurfX, centerOfSurfY);
    gc->scale(zoomFactor, zoomFactor);

    gc->bitblt(mFrozenSurface,
               - mFrozenRenderWidth / 2,
               - mFrozenRenderHeight / 2,
               mFrozenRenderWidth / 2,
               mFrozenRenderHeight / 2);

    gc->pop();

    if (mShowHighlight) {
        showHighlight(gc);
    }
}

/**
 * @brief invalidates highlight rectangle regions and clears vector of rectangles
 *
 */
void BrowserAdapter::removeHighlight()
{
    invalidateHighlightRectsRegion();

    mHighlightRects.clear();
    mShowHighlight = false;

}

void BrowserAdapter::invalidateHighlightRectsRegion()
{
    // remove rectangles by invalidating their areas
    /*
      for (std::vector<BrowserAdapterRectangle>::const_iterator rect_iter = mHighlightRects.begin();
             rect_iter != mHighlightRects.end();
             ++rect_iter)
      {

          BrowserAdapterRectangle r = *rect_iter;
          NPRect dirty;

          dirty.left = r.left() * mZoomLevel + mJsScrollX;
          dirty.top = r.top() * mZoomLevel + mJsScrollY;
          dirty.right = r.right() * mZoomLevel + mJsScrollX;
          dirty.bottom = r.bottom() * mZoomLevel + mJsScrollY;

          NPN_InvalidateRect(&dirty);
      }
    */

    invalidate();
}


int BrowserAdapter::showSpotlight(PGContext* gc)
{
    /* FIXME: RR
        BrowserRect scaledSpotlightRect(ceil(m_spotlightRect.x()*mZoomLevel) + mJsScrollX,
        ceil(m_spotlightRect.y()*mZoomLevel) + mJsScrollY,
        floor(m_spotlightRect.r()*mZoomLevel) - ceil(m_spotlightRect.x()*mZoomLevel),
                floor(m_spotlightRect.b()*mZoomLevel) - ceil(m_spotlightRect.y()*mZoomLevel) );

    //draw a grey drop shadow
    if(!m_spotlightHandle)
    return 0;
    //draw a grey drop shadow
    gc->push();
    //gc->setStrokeColor(PColor32(0x69, 0x69, 0x69, 0x69));
       // gc->setFillColor(PColor32(0x69, 0x69, 0x69, 0x69));
        int x1=scaledSpotlightRect.x();
        int y1=scaledSpotlightRect.y();
        int x2=scaledSpotlightRect.r();
        int y2=scaledSpotlightRect.b();


    PGSurface* imgSurfaceTop=PGSurface::createFromPNGFile(kImgTopFile);
    PGSurface* imgSurfaceBot=PGSurface::createFromPNGFile(kImgBotFile);

    if(m_spotlightHandle==2) //when drop shadow is completely around the rect
    {

    //draw completely around the flash window
    PGSurface* imgSurfaceLeft=PGSurface::createFromPNGFile(kImgLeftFile);
    PGSurface* imgSurfaceLeftBotCorner=PGSurface::createFromPNGFile(kImgBotLeftFile);

    PGSurface* imgSurfaceRightBotCorner=PGSurface::createFromPNGFile(kImgBotRightFile);
    PGSurface* imgSurfaceRight=PGSurface::createFromPNGFile(kImgRightFile);

    PGSurface* imgSurfaceRightTop=PGSurface::createFromPNGFile(kImgTopLeftFile);
    PGSurface* imgSurfaceLeftTop=PGSurface::createFromPNGFile(kImgTopRightFile);




    //draw the left
    if(imgSurfaceLeft!=NULL)
    {

    gc->bitblt(imgSurfaceLeft,
    0, 0,
    imgSurfaceLeft->width()-1, imgSurfaceLeft->height()-1,
    x1-20, y1, x1, y2);
    }
    //draw left bottom corner
    if(imgSurfaceLeftBotCorner!=NULL)
    {

    gc->bitblt(imgSurfaceLeftBotCorner,
    0, 0,
    imgSurfaceLeftBotCorner->width()-1, imgSurfaceLeftBotCorner->height()-1,
    x1-20, y2-20, x1, y2);
    }
    //draw the bottom
    if(imgSurfaceBot!=NULL)
    {

    gc->bitblt(imgSurfaceBot,
    0, 0,
    imgSurfaceBot->width()-1, imgSurfaceBot->height()-1,
    x1, y2, x2, y2+20);
    }
    //draw the bottom right
    if(imgSurfaceRightBotCorner!=NULL)
    {

    gc->bitblt(imgSurfaceRightBotCorner,
    0, 0,
    imgSurfaceRightBotCorner->width()-1, imgSurfaceRightBotCorner->height()-1,
    x2, y2, x2+20, y2+20);
    }
    //draw the right
    if(imgSurfaceRight!=NULL)
    {
    //TRACE("drawDropShadow:imgSurfaceRight");
    gc->bitblt(imgSurfaceRight,
    0, 0,
    imgSurfaceRight->width()-1, imgSurfaceRight->height()-1,
    x2, y1, x2+20, y2);
    }
    //draw the right top
    if(imgSurfaceRightTop!=NULL)
    {

    gc->bitblt(imgSurfaceRightTop,
    0, 0,
    imgSurfaceRightTop->width()-1, imgSurfaceRightTop->height()-1,
    x2, y1-20, x2+20, y1);
    }

    //draw the left top

    if(imgSurfaceLeftTop!=NULL)
    {

    gc->bitblt(imgSurfaceLeftTop,
    0, 0,
    imgSurfaceLeftTop->width()-1, imgSurfaceLeftTop->height()-1,
    x2, y2, x1+20, y1-20);
    }
    //draw the top

    if(imgSurfaceTop!=NULL)
    {

    gc->bitblt(imgSurfaceTop,
    0, 0,
    imgSurfaceTop->width()-1, imgSurfaceTop->height()-1,
    x1, y1-20, x2, y1);
    }

    }
    else if(m_spotlightHandle==1) //only when dropshadow on top and bottom of rect
    {

     //draw only below the flash window and above flash window
    if(imgSurfaceBot != NULL)
    {

    //draw it around the flash area
    gc->bitblt(imgSurfaceBot,
    0, 0,
    imgSurfaceBot->width()-1, imgSurfaceBot->height()-1,
    x1, y2, x2, y2+20);
    }

    if(imgSurfaceTop!=NULL)
    {

    gc->bitblt(imgSurfaceTop,
    0, 0,
    imgSurfaceTop->width()-1, imgSurfaceTop->height()-1,
    x1, y1-20, x2, y1);
    }


    }

    gc->pop();
    */
    return 1;
}

/**
 * @brief draws highlight rectangles in current context
 *
 *
 */
int BrowserAdapter::showHighlight(PGContext* gc)
{
    gc->push();
    gc->setStrokeColor(PColor32(0, 0, 0, 0));  // no border around rects
    gc->setFillColor(PColor32(0, 0, 0, 60));

    gc->translate(mWindow.x, mWindow.y);
    gc->addClipRect(0, 0, mWindow.width, mWindow.height);
    gc->translate(-mScrollPos.x, m_headerHeight-mScrollPos.y);


    for (std::vector<BrowserRect>::const_iterator rect_iter = mHighlightRects.begin();
            rect_iter != mHighlightRects.end();
            ++rect_iter)
    {
        BrowserRect r = *rect_iter;
        BrowserRect d[4];
        int count = 0;
        for (RectMap::const_iterator i = mFlashRects.begin();
                i != mFlashRects.end();
                ++i)
        {
            BrowserRect f = i->second;
            if (r.intersects(f)) {
                // just do one for now
                /*
                TRACEF("rect %d,%d,%d,%d intersects with flash rect %d,%d,%d,%d", r.x(), r.y(), r.r(), r.b(), f.x(), f.y(), f.r(), f.b());
                */
                count = r.subtract(f, d);
                break;
            }
        }
        for (RectMap::const_iterator i = mDefaultInteractiveRects.begin();
                i != mDefaultInteractiveRects.end();
                ++i)
        {
            BrowserRect f = i->second;
            if (r.intersects(f)) {
                // just do one for now
                /*
                TRACEF("rect %d,%d,%d,%d intersects with irect %d,%d,%d,%d", r.x(), r.y(), r.r(), r.b(), f.x(), f.y(), f.r(), f.b());
                */
                count = r.subtract(f, d);
                break;
            }
        }


        if (count == 0) {
            gc->drawRect(r.x() * mZoomLevel,
                         r.y() * mZoomLevel,
                         r.r() * mZoomLevel,
                         r.b() * mZoomLevel);
        } else {
            for (int j = 0; j < count; j++) {
                gc->drawRect(d[j].x() * mZoomLevel,
                             d[j].y() * mZoomLevel,
                             d[j].r() * mZoomLevel,
                             d[j].b() * mZoomLevel);
            }
        }

        /*
        TRACEF("DREW rect with left: %d, top: %d, right: %d, bottom: %d",
        int(d.x() * mZoomLevel + mJsScrollX),
        int(d.y() * mZoomLevel + mJsScrollY),
        int(d.r() * mZoomLevel + mJsScrollX),
        int(d.b() * mZoomLevel + mJsScrollY));
        */
    }

    gc->pop();

    return mHighlightRects.size();
}

void BrowserAdapter::updateMouseInFlashStatus(bool inFlashRect)
{
    if (inFlashRect != mMouseInFlashRect) {
        mMouseInFlashRect = inFlashRect;

        // let BA know of change
        TRACEF("Updating mouseInFlash status to : %d\n", inFlashRect);
        NPVariant arg;
        BOOLEAN_TO_NPVARIANT(inFlashRect, arg);
        InvokeEventListener(gMouseInFlashChangeHandler, &arg, 1, 0);
    }
}

void BrowserAdapter::updateMouseInInteractiveStatus(bool inInteractiveRect)
{
    if (inInteractiveRect != mMouseInInteractiveRect) {
        mMouseInInteractiveRect = inInteractiveRect;

        // let BA know of change
        TRACEF("Updating mouseInInteractive status to : %d\n", inInteractiveRect);
        NPVariant arg;
        BOOLEAN_TO_NPVARIANT(inInteractiveRect, arg);
        InvokeEventListener(gMouseInInteractiveChangeHandler, &arg, 1, 0);
    }
}

void BrowserAdapter::updateFlashGestureLockStatus(bool gestureLockEnabled)
{
    if (gestureLockEnabled != mFlashGestureLock) {
        mFlashGestureLock = gestureLockEnabled;

        // Send a pen up if necessary since we get a penDown before gestureStart
        if (!gestureLockEnabled && mLastPointSentToFlash.x != -1 && mLastPointSentToFlash.y != -1) {
            gettimeofday(&m_lastPassedEventTime, NULL);
            asyncCmdMouseEvent(1 /*mouseup */, mLastPointSentToFlash.x, mLastPointSentToFlash.y, 1);
            mLastPointSentToFlash = Point(-1, -1);
        }

        // let BA know of change
        TRACEF("Updating flashGestureLock status to : %d\n", gestureLockEnabled);
        NPVariant arg;
        BOOLEAN_TO_NPVARIANT(gestureLockEnabled, arg);
        InvokeEventListener(gFlashGestureLockHandler, &arg, 1, 0);
    }
}

bool BrowserAdapter::flashRectContainsPoint(const Point& pt)
{
    bool ret = rectContainsPoint(mFlashRects, pt);

    if (ret) {
        TRACEF("point x: %d, y: %d in flash rect\n", pt.x, pt.y);
    } else {
        TRACEF("point x: %d, y: %d NOT in rect\n", pt.x, pt.y);
    }

    return ret;
}

bool BrowserAdapter::interactiveRectContainsPoint(const Point& pt)
{
    bool ret = rectContainsPoint(mDefaultInteractiveRects, pt);

    if (ret) {
        TRACEF("point x: %d, y: %d in interactive rect\n", pt.x, pt.y);
    } else {
        TRACEF("point x: %d, y: %d NOT in interactive rect\n", pt.x, pt.y);
    }

    return ret;
}

bool BrowserAdapter::rectContainsPoint(RectMap &rectMap, const Point& pt)
{
    if (rectMap.size() == 0) {
        return false;
    }

    BrowserRect point(pt.x, pt.y, 1, 1);
    for (RectMap::const_iterator i = rectMap.begin();
            i != rectMap.end();
            ++i)
    {
        BrowserRect r = i->second;

        if (r.overlaps(point)) {
            return true;
        }
    }
    return false;
}

const char* BrowserAdapter::js_saveImageAtPoint(AdapterBase *adapter, const NPVariant *args, uint32_t argCount, NPVariant *result)
{
    if ( argCount != 4
            || !IsIntegerVariant(args[0])
            || !IsIntegerVariant(args[1])
            || !NPVARIANT_IS_STRING(args[2])
            || !NPVARIANT_IS_OBJECT(args[3])
       ) {
        INT32_TO_NPVARIANT( kInvalidParam, *result );
        return "BrowserAdapter::saveImageAtPoint(x, y, outDir, cb): Bad arguments.";
    }

    BrowserAdapter *proxy = GetAndInitAdapter(adapter);
    if (proxy->mBrowserServerConnected) {

        int x = VariantToInteger(args[0]);
        int y = VariantToInteger(args[1]);

        char* outDir = NPStringToString(NPVARIANT_TO_STRING(args[2]));

        int nQueryNum = proxy->mBsQueryNum++;
        SaveImageAtPointArgs*  callArgs = new SaveImageAtPointArgs(x, y,
                NPVARIANT_TO_OBJECT(args[3]), nQueryNum);
        proxy->m_saveImageAtPointArgs.add(callArgs);

        x = (x + proxy->mScrollPos.x) / proxy->mZoomLevel;
        y = (y + proxy->mScrollPos.y) / proxy->mZoomLevel;

        proxy->asyncCmdSaveImageAtPoint(nQueryNum, x, y, outDir);
        ::free(outDir);
        return NULL;
    }
    else {
        return "BrowserAdapter::saveImageAtPoint() - Not connected to server";
    }
}

void
BrowserAdapter::msgSaveImageAtPointResponse(int32_t queryNum, bool succeeded, const char* filepath)
{
    TRACEF("SaveImageAtPointResponse: %c, '%s'", succeeded ? 'T' : 'F', filepath);
    std::auto_ptr<SaveImageAtPointArgs> args(m_saveImageAtPointArgs.get(queryNum));
    if (!args.get()) {
        TRACEF("Can't find response for this call.");
        return;
    }

    NPVariant jsCallResult;
    NPVariant jsCallArgs[2];

    BOOLEAN_TO_NPVARIANT(succeeded, jsCallArgs[0]);
    if (filepath == NULL)
        filepath = "";
    STRINGZ_TO_NPVARIANT(filepath, jsCallArgs[1]);

    if (NPN_InvokeDefault(args->m_callback, jsCallArgs, G_N_ELEMENTS(jsCallArgs), &jsCallResult)) {
        TRACEF("saveImageAtPoint response call success.");
    }
    else {
        TRACEF("saveImageAtPoint response call FAILED.");
    }
}

void
BrowserAdapter::msgCopiedToClipboard()
{
    NPVariant jsCallResult, jsCallArgs;

    STRINGZ_TO_NPVARIANT("PalmSystem.copiedToClipboard()", jsCallArgs);
    InvokeAdapter(gEval, &jsCallArgs, 1, &jsCallResult);
}

void
BrowserAdapter::msgPastedFromClipboard()
{
    // stub for sending notifications when paste succeeds
}

void
BrowserAdapter::initSelectionReticleSurface()
{
    mSelectionReticle.surface = NULL;
    mSelectionReticle.timeoutSource = NULL;
    mSelectionReticle.x = 0;
    mSelectionReticle.y = 0;
    mSelectionReticle.centerOffsetX = 0;
    mSelectionReticle.centerOffsetY = 0;
    mSelectionReticle.show = false;

    mSelectionReticle.surface = PGSurface::createFromPNGFile(kSelectionReticleFile);
    if (mSelectionReticle.surface) {
        mSelectionReticle.centerOffsetX = mSelectionReticle.surface->width()/2;
        mSelectionReticle.centerOffsetY = mSelectionReticle.surface->height()/2;
    }
}

void
BrowserAdapter::showSelectionReticle(PGContext* gc)
{
    if (!mSelectionReticle.surface) {
        return;
    }

    /* FIXME: RR
        gc->bitblt(mSelectionReticle.surface,
                   mSelectionReticle.x * mZoomLevel + mJsScrollX - mSelectionReticle.centerOffsetX,
                   mSelectionReticle.y * mZoomLevel + mJsScrollY - mSelectionReticle.centerOffsetY,
                   mSelectionReticle.x * mZoomLevel + mJsScrollX + mSelectionReticle.centerOffsetX,
                   mSelectionReticle.y * mZoomLevel + mJsScrollY + mSelectionReticle.centerOffsetY);
    */
}


void BrowserAdapter::invalidateSelectionReticleRegion()
{
    if (!mSelectionReticle.surface) {
        return;
    }

    invalidate();
}


void
BrowserAdapter::msgRemoveSelectionReticle()
{

    if (mSelectionReticle.timeoutSource) {
        return;
    }

    mSelectionReticle.timeoutSource = g_timeout_source_new(250); // in ms
    g_source_set_callback(mSelectionReticle.timeoutSource, &BrowserAdapter::removeSelectionReticleCallback, this, NULL);

    g_source_attach(mSelectionReticle.timeoutSource, g_main_loop_get_context(mMainLoop));
}

gboolean
BrowserAdapter::removeSelectionReticleCallback(gpointer arg)
{
    BrowserAdapter* adapter = (BrowserAdapter*)arg;

    adapter->mSelectionReticle.show = false;
    adapter->invalidateSelectionReticleRegion();

    if (adapter->mSelectionReticle.timeoutSource != NULL) {
        g_source_destroy(adapter->mSelectionReticle.timeoutSource);
        g_source_unref(adapter->mSelectionReticle.timeoutSource);
        adapter->mSelectionReticle.timeoutSource = NULL;
    }

    return FALSE;
}

void
BrowserAdapter::msgPluginFullscreenSpotlightCreate(int32_t spotlightHandle,
        int32_t rectX, int32_t rectY, int32_t rectWidth, int32_t rectHeight)
{
    NPVariant args[6];
    DOUBLE_TO_NPVARIANT((double)rectX, args[0]);
    DOUBLE_TO_NPVARIANT((double)rectY, args[1]);
    DOUBLE_TO_NPVARIANT((double)rectX+rectWidth, args[2]);
    DOUBLE_TO_NPVARIANT((double)rectY+rectHeight, args[3]);
    STRINGZ_TO_NPVARIANT("fullscreen", args[4]);
    BOOLEAN_TO_NPVARIANT(true, args[5]);
    InvokeEventListener(gPluginSpotlightCreate, args, G_N_ELEMENTS(args), NULL);
}

void
BrowserAdapter::msgPluginFullscreenSpotlightRemove()
{
    NPVariant args[0];
    InvokeEventListener(gPluginSpotlightRemove, args, G_N_ELEMENTS(args), NULL);
}


void
BrowserAdapter::msgSpellingWidgetVisibleRectUpdate(int32_t rectX, int32_t rectY, int32_t rectWidth, int32_t rectHeight)
{
    TRACEF("**** BrowserAdapter msgSpellingWidgetVisibleRectUpdate %d,%d,%d,%d",rectX,rectY,rectWidth,rectHeight);
    NPVariant args[4];

    DOUBLE_TO_NPVARIANT((double)rectX, args[0]);
    DOUBLE_TO_NPVARIANT((double)rectY, args[1]);
    DOUBLE_TO_NPVARIANT((double)rectWidth, args[2]);
    DOUBLE_TO_NPVARIANT((double)rectHeight, args[3]);
    InvokeEventListener(gMsgSpellingWidgetVisibleRectUpdate, args, G_N_ELEMENTS(args), NULL);
}

void BrowserAdapter::msgHitTestResponse(int32_t queryNum,
                                        const char *hitTestResultJson)
{
    TRACEF("json: %s", hitTestResultJson);
    std::auto_ptr<HitTestArgs> args(m_hitTestArgs.get(queryNum));
    if ((args.get() != NULL)) {
        NPVariant jsCallResult, jsCallArgs[2];
        NPObject *event = NPN_CreateObject(&NPObjectEvent::sNPObjectEventClass);
        if (event) {
            NPObject *hitTest =
                NPN_CreateObject(&JsonNPObject::sJsonNPObjectClass);
            if (hitTest) {
                OBJECT_TO_NPVARIANT(event, jsCallArgs[0]);
                OBJECT_TO_NPVARIANT(hitTest, jsCallArgs[1]);

                static_cast<NPObjectEvent*>(event)->initialize(args->m_type,
                        args->m_pt.x * mZoomLevel - mScrollPos.x,
                        args->m_pt.y * mZoomLevel - mScrollPos.y, args->m_modifiers);
                static_cast<JsonNPObject *>(hitTest)->initialize(
                    m_hitTestSchema, hitTestResultJson);

                InvokeEventListener(gEventFiredHandler, jsCallArgs, 2,
                                    &jsCallResult);
                TRACEF("result %d %s", !VariantToBoolean(jsCallResult), args->m_type);
                if (!VariantToBoolean(jsCallResult)) {
                    if (!strcmp(args->m_type, "click")) {
                        asyncCmdClickAt(args->m_pt.x, args->m_pt.y, 1, 1);
                    } else if (!strcmp(args->m_type, "mousehold")) {
                        m_didHold = true;
                        asyncCmdHoldAt(args->m_pt.x, args->m_pt.y);
                    }
                }

                AdapterBase::NPN_ReleaseVariantValue(&jsCallArgs[0]);
                AdapterBase::NPN_ReleaseVariantValue(&jsCallArgs[1]);
            }
        }
    }
}

bool BrowserAdapter::shouldPassInputEvents()
{
    return m_passInputEvents
           || (mContentWidth <= mViewportWidth
               && mContentHeight <= mViewportHeight
               && mMetaViewport && !mMetaViewport->userScalable);
}

// Touch events can be safely passed even if the content dimensions exceed the viewport dimensions
bool BrowserAdapter::shouldPassTouchEvents()
{
    return m_passInputEvents || (mMetaViewport && !mMetaViewport->userScalable);
}

void BrowserAdapter::fireScrolledToEvent(int x, int y)
{
    if( gScrolledToHandler ) {
        NPVariant args[2];
        INT32_TO_NPVARIANT(x, args[0]);
        INT32_TO_NPVARIANT(y, args[1]);
        InvokeEventListener(gScrolledToHandler, args, G_N_ELEMENTS(args), 0 );
    }

}

void BrowserAdapter::scrollTo(int x, int y)
{
    // ignore any callbacks from kinetic scroller if we are in the middle
    // of a zoom animation or in a gesture change
    if (m_zoomTimerSource || mInGestureChange)
        return;

    mScrollPos.x = (mContentWidth > (int) mWindow.width) ? -x : 0;
    mScrollPos.y = -y;

    asyncCmdSetScrollPosition(mScrollPos.x, mScrollPos.y,
                              mScrollPos.x + mWindow.width,
                              mScrollPos.y + mWindow.height);
    invalidate();

    fireScrolledToEvent(x, y);
}

gboolean BrowserAdapter::clickTimeoutCb(gpointer arg)
{
    BrowserAdapter *a = (BrowserAdapter *)arg;
    if (a->mBrowserServerConnected) {
        int q = a->mBsQueryNum++;
        HitTestArgs *callArgs = new HitTestArgs("click", a->m_clickPt, 0, q);
        a->m_hitTestArgs.add(callArgs);
        TRACEF("x: %d, y: %d, q: %d", a->m_clickPt.x, a->m_clickPt.y, q);
        a->asyncCmdHitTest(q, a->m_clickPt.x, a->m_clickPt.y);
    }
    a->stopClickTimer();
    return FALSE;
}

void BrowserAdapter::startClickTimer()
{
    if (m_clickTimerSource)
        stopClickTimer();

    m_clickTimerSource = g_timeout_source_new(kDoubleClickInterval);
    g_source_set_callback(m_clickTimerSource, clickTimeoutCb, this /*data*/, NULL);
    g_source_attach(m_clickTimerSource, g_main_loop_get_context(mMainLoop));
}

void BrowserAdapter::stopClickTimer()
{
    if (m_clickTimerSource) {
        g_source_destroy(m_clickTimerSource);
        g_source_unref(m_clickTimerSource);
        m_clickTimerSource = NULL;
    }
}

gboolean BrowserAdapter::mouseHoldTimeoutCb(gpointer arg)
{
    BrowserAdapter *a = (BrowserAdapter *)arg;
    if (a->mBrowserServerConnected
            && !a->flashRectContainsPoint(a->m_penDownDoc)) {
        int q = a->mBsQueryNum++;
        HitTestArgs *callArgs = new HitTestArgs("mousehold", a->m_penDownDoc, 0, q);
        a->m_hitTestArgs.add(callArgs);
        TRACEF("x: %d, y: %d, q: %d", a->m_penDownDoc.x, a->m_penDownDoc.y, q);
        a->asyncCmdHitTest(q, a->m_penDownDoc.x, a->m_penDownDoc.y);
    }
    a->stopMouseHoldTimer();
    return FALSE;
}

void BrowserAdapter::startMouseHoldTimer()
{
    if (m_mouseHoldTimerSource)
        stopMouseHoldTimer();

    m_mouseHoldTimerSource = g_timeout_source_new(kMouseHoldInterval);
    g_source_set_callback(m_mouseHoldTimerSource, mouseHoldTimeoutCb, this /*data*/, NULL);
    g_source_attach(m_mouseHoldTimerSource, g_main_loop_get_context(mMainLoop));
}

void BrowserAdapter::stopMouseHoldTimer()
{
    if (m_mouseHoldTimerSource) {
        g_source_destroy(m_mouseHoldTimerSource);
        g_source_unref(m_mouseHoldTimerSource);
        m_mouseHoldTimerSource = NULL;
    }
}

gboolean BrowserAdapter::zoomTimerTimeoutCb(gpointer arg)
{
    BrowserAdapter *a = (BrowserAdapter *)arg;
    return a->animateZoom();
}

bool BrowserAdapter::animateZoom()
{
    if (PrvIsEqual(mZoomLevel, m_zoomTarget)) {
        if (mPageWidth) {
            double fitZoom = mWindow.width * 1.0 / mPageWidth;
            if (PrvIsEqual(mZoomLevel, fitZoom))
                mZoomFit = true;
            else
                mZoomFit = false;
        }
        asyncCmdSetZoomAndScroll(mZoomLevel, mScrollPos.x, mScrollPos.y);
        stopZoomAnimation();
    } else {
        if (::fabs(m_zoomTarget - mZoomLevel) < ::fabs(m_zoomLevelInterval)) {
            mZoomLevel = m_zoomTarget;
            mContentWidth = ::round(mZoomLevel * mPageWidth);
            mContentHeight = ::round(mZoomLevel * mPageHeight);
            mScrollPos.x = m_zoomPt.x;
            mScrollPos.y = m_zoomPt.y;
        } else {
            mZoomLevel += m_zoomLevelInterval;
            mContentWidth = ::round(mZoomLevel * mPageWidth);
            mContentHeight = ::round(mZoomLevel * mPageHeight);
            mScrollPos.x += m_zoomXInterval;
            mScrollPos.y += m_zoomYInterval;
        }

        mScroller->setContentDimensions(mContentWidth, mContentHeight + m_headerHeight);
        mScroller->scrollTo(-mScrollPos.x, -mScrollPos.y, false);
        invalidate();
    }

    return TRUE;
}

void BrowserAdapter::startZoomAnimation(double zoom, int x, int y)
{
    if (m_zoomTimerSource) {
        stopZoomAnimation();
    }

    m_zoomTarget = zoom;
    m_zoomPt.set(x, y);
    m_zoomXInterval = (x - mScrollPos.x) / kZoomAnimationSteps;
    m_zoomYInterval = (y - mScrollPos.y) / kZoomAnimationSteps;
    m_zoomLevelInterval = (zoom - mZoomLevel) / kZoomAnimationSteps;

    m_zoomTimerSource = g_timeout_source_new(kTimerInterval);
    g_source_set_callback(m_zoomTimerSource, zoomTimerTimeoutCb, this /*data*/, NULL);
    g_source_attach(m_zoomTimerSource, g_main_loop_get_context(mMainLoop));
}

void BrowserAdapter::stopZoomAnimation()
{
    if (m_zoomTimerSource) {
        g_source_destroy(m_zoomTimerSource);
        g_source_unref(m_zoomTimerSource);
        m_zoomTimerSource = NULL;
    }
}

void BrowserAdapter::msgUpdateScrollableLayers(const char* json)
{
    if (!json)
        return;

    mScrollableLayers.clear();
    resetScrollableLayerScrollSession();

    json_object* root = json_tokener_parse(json);
    if (!root || is_error(root)) {
        return;
    }

    array_list* array = json_object_get_array(root);
    if (!array || is_error(array)) {
        json_object_put(root);
        return;
    }

    for (int i = 0; i < array_list_length(array); i++) {

        BrowserScrollableLayer layer;
        json_object* obj = (json_object*) array_list_get_idx(array, i);
        json_object* l = 0;

        int left, right, top, bottom;

        l = json_object_object_get(obj, "id");
        layer.id = json_object_get_int(l);

        l = json_object_object_get(obj, "left");
        left = json_object_get_int(l);

        l = json_object_object_get(obj, "right");
        right = json_object_get_int(l);

        l = json_object_object_get(obj, "top");
        top = json_object_get_int(l);

        l = json_object_object_get(obj, "bottom");
        bottom = json_object_get_int(l);

        layer.bounds.set(left, top, right, bottom);

        l = json_object_object_get(obj, "contentX");
        layer.contentX = json_object_get_int(l);

        l = json_object_object_get(obj, "contentY");
        layer.contentY = json_object_get_int(l);

        l = json_object_object_get(obj, "contentWidth");
        layer.contentWidth = json_object_get_int(l);

        l = json_object_object_get(obj, "contentHeight");
        layer.contentHeight = json_object_get_int(l);

        mScrollableLayers[layer.id] = layer;
    }

    json_object_put(root);
}

bool BrowserAdapter::detectScrollableLayerUnderMouseDown(const Point& pagePt, const Point& mousePt)
{
    resetScrollableLayerScrollSession();

    for (BrowserScrollableLayerMap::iterator it = mScrollableLayers.begin();
            it != mScrollableLayers.end(); ++it) {

        BrowserRect pt(pagePt.x, pagePt.y, 1, 1);
        if (it->second.bounds.overlaps(pt)) {
            mScrollableLayerScrollSession.layer = it->first;
            mScrollableLayerScrollSession.contentXAtMouseDown = it->second.contentX;
            mScrollableLayerScrollSession.contentYAtMouseDown = it->second.contentY;
            mScrollableLayerScrollSession.mouseDownX = mousePt.x;
            mScrollableLayerScrollSession.mouseDownY = mousePt.y;
            break;
        }
    }

    return (mScrollableLayerScrollSession.layer != 0);
}

bool BrowserAdapter::scrollLayerUnderMouse(const Point& currentMouseDoc)
{
    if (mScrollableLayerScrollSession.layer == 0)
        return false;

    BrowserScrollableLayerMap::iterator it = mScrollableLayers.find(mScrollableLayerScrollSession.layer);
    if (it == mScrollableLayers.end()) {
        resetScrollableLayerScrollSession();
        return false;
    }

    BrowserScrollableLayer& layer = it->second;

    int dx = - (currentMouseDoc.x - mScrollableLayerScrollSession.mouseDownX);
    int dy = - (currentMouseDoc.y - mScrollableLayerScrollSession.mouseDownY);

    int contentXMin = 0;
    int contentXMax = layer.contentWidth - (layer.bounds.r() - layer.bounds.x());

    int contentYMin = 0;
    int contentYMax = layer.contentHeight - (layer.bounds.b() - layer.bounds.y());

    if (contentXMax <= contentXMin)
        dx = 0;

    if (contentYMax <= contentYMin)
        dy = 0;

    // Not scrollable
    if (dx == 0 && dy == 0) {
        resetScrollableLayerScrollSession();
        return false;
    }

    int newContentX = mScrollableLayerScrollSession.contentXAtMouseDown + dx;
    int newContentY = mScrollableLayerScrollSession.contentYAtMouseDown + dy;

    newContentX = MAX(contentXMin, newContentX);
    newContentX = MIN(contentXMax, newContentX);

    newContentY = MAX(contentYMin, newContentY);
    newContentY = MIN(contentYMax, newContentY);

    if (newContentX == layer.contentX && newContentY == layer.contentY)
        return false;

    mScrollableLayerScrollSession.isActive = true;

    dx = newContentX - layer.contentX;
    dy = newContentY - layer.contentY;

    layer.contentX = newContentX;
    layer.contentY = newContentY;

    asyncCmdScrollLayer(mScrollableLayerScrollSession.layer, dx, dy);

    return true;
}

void BrowserAdapter::resetScrollableLayerScrollSession()
{
    mScrollableLayerScrollSession.isActive = false;
    mScrollableLayerScrollSession.layer = 0;
    mScrollableLayerScrollSession.contentXAtMouseDown = 0;
    mScrollableLayerScrollSession.contentYAtMouseDown = 0;
    mScrollableLayerScrollSession.mouseDownX = 0;
    mScrollableLayerScrollSession.mouseDownY = 0;
}

void BrowserAdapter::showActiveScrollableLayer(PGContext* gc)
{
    if (!mScrollableLayerScrollSession.isActive ||
            !mScrollableLayerScrollSession.layer)
        return;

    BrowserScrollableLayerMap::const_iterator it =
        mScrollableLayers.find(mScrollableLayerScrollSession.layer);
    if (it == mScrollableLayers.end()) {
        resetScrollableLayerScrollSession();
        return;
    }

    const BrowserScrollableLayer& layer = it->second;

    gc->push();
    gc->setFillColor(PColor32(0, 0, 0, 0));
    gc->setStrokeColor(PColor32(0x88, 0x88, 0x88, 0x80));

    gc->translate(mWindow.x, mWindow.y);
    gc->addClipRect(0, 0, mWindow.width, mWindow.height);
    gc->translate(-mScrollPos.x, -mScrollPos.y);

    gc->drawRect(layer.bounds.x() * mZoomLevel,
                 layer.bounds.y() * mZoomLevel,
                 layer.bounds.r() * mZoomLevel,
                 layer.bounds.b() * mZoomLevel);
    gc->pop();
}

void BrowserAdapter::recordGesture(float s, float r, int cX, int cY)
{
    m_recordedGestures.push_back(RecordedGestureEntry(s, r, cX, cY));
    if (m_recordedGestures.size() > (size_t) kNumRecordedGestures)
        m_recordedGestures.pop_front();
}

BrowserAdapter::RecordedGestureEntry BrowserAdapter::getAveragedGesture() const
{
    RecordedGestureEntry avg(0, 0, 0, 0);

    if (m_recordedGestures.empty()) {
        avg.scale = 1.0;
        return avg;
    }

    int index = 0;
    int denom = 0;
    int weight = 0;
    for (std::list<RecordedGestureEntry>::const_iterator it = m_recordedGestures.begin();
            it != m_recordedGestures.end(); ++it, ++index) {

        const RecordedGestureEntry& e = (*it);
        weight = s_recordedGestureAvgWeights[index];

        avg.scale   += e.scale   * weight;
        avg.rotate  += e.rotate  * weight;
        avg.centerX += e.centerX * weight;
        avg.centerY += e.centerY * weight;

        denom += weight;
    }

    avg.scale   /= denom;
    avg.rotate  /= denom;
    avg.centerX /= denom;
    avg.centerY /= denom;

    return avg;
}

bool BrowserAdapter::destroyBufferLock()
{
    bool result(false);

    if (m_bufferLock) {
        sem_post(m_bufferLock);
        sem_close(m_bufferLock);
        sem_unlink(m_bufferLockName);
        delete []m_bufferLockName;
        m_bufferLockName = 0;
        m_bufferLock = 0;

        result = true;
    }

    return result;
}

bool BrowserAdapter::createBufferLock()
{
    if (destroyBufferLock())
        incrementPostfix();

    m_bufferLockName = createBufferLockName(postfix());

    if (m_bufferLockName)
        m_bufferLock = sem_open(m_bufferLockName, O_CREAT, S_IRUSR | S_IWUSR, 0);

    return m_bufferLock != 0;
}

void BrowserAdapter::showScrollbar()
{
    mScrollbarOpacity = 0xFF * 2;
    stopFadeScrollbar();
}

void BrowserAdapter::startFadeScrollbar()
{
    if (mScrollbarFadeSource)
        return;

    if (mScrollbarOpacity <= 0)
        return;

    mScrollbarFadeSource = g_timeout_source_new(40);
    g_source_set_callback(mScrollbarFadeSource, &BrowserAdapter::scrollbarFadeoutCb, this, NULL);
    g_source_attach(mScrollbarFadeSource, g_main_loop_get_context(mMainLoop));
}

void BrowserAdapter::stopFadeScrollbar()
{
    if (mScrollbarFadeSource) {
        g_source_destroy(mScrollbarFadeSource);
        g_source_unref(mScrollbarFadeSource);
        mScrollbarFadeSource = NULL;
    }
}

#define SCALE_DELTA(val, div)   \
    if (val > 0)                \
        val = MAX(val/div, 1);  \
    else if (val < 0)           \
        val = MIN(val/div, -1);

gboolean BrowserAdapter::scrollbarFadeoutCb(gpointer data)
{
    BrowserAdapter *a = (BrowserAdapter*)data;

    a->invalidate();

    int delta = a->mScrollbarOpacity;

    SCALE_DELTA(delta, 2);
    a->mScrollbarOpacity -= delta;

    if (a->mScrollbarOpacity <= 0) {
        a->mScrollbarOpacity = 0;
        a->stopFadeScrollbar();
        return FALSE;
    }

    return TRUE;
}

void BrowserAdapter::startedAnimating()
{
    showScrollbar();
}

void BrowserAdapter::stoppedAnimating()
{
    startFadeScrollbar();
}
