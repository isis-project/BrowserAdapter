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

#ifndef BROWSERADAPTER_H
#define BROWSERADAPTER_H

#include "BrowserClientBase.h"
#include "AdapterBase.h"
#include "KineticScroller.h"

#include <glib.h>
#include <string>
#include <list>
#include <vector>
#include <map>
#include <PGContext.h>
#include <PGSurface.h>
#include <pbnjson.hpp>
#include <semaphore.h>

#include <BrowserRect.h>
#include "BrowserScrollableLayer.h"

struct PluginType;
class BrowserOffscreen;
struct BrowserAdapterData;
class BrowserSyncReplyPipe;
class BrowserAdapterData;
class BrowserCenteredZoom;
struct BrowserMetaViewport;

/**
 * This is the NPAPI browser plugin to our browser. This object communicates with
 * the browser server which renders a browser page into this adapter.
 * 
 * All js_* methods are the implementations of the exposed JavaScript routines
 * callable by a JavaScript program.
 * 
 * The msg* methods are unsolicited messages sent by the browser server when it
 * needs to inform this adapter of something.
 * 
 * The handle* methods are called by the browser via the AdapterBase::PrvNPP_HandleEvent
 * NPAPI plugin callback event handler.
 */
class BrowserAdapter : public BrowserClientBase
					 , public AdapterBase
					 , public KineticScrollerListener
{
public:

	BrowserAdapter(NPP instance, GMainContext *ctxt, int16_t argc, char* argn[], char* argv[]);
	virtual ~BrowserAdapter();

	void freeze();
	void thaw();
	bool isFrozen();

	bool flashGestureLock() const { return mFlashGestureLock; }

	// Methods exposed to JavaScript:
	static const char* js_addUrlRedirect(AdapterBase *adapter, const NPVariant *args, uint32_t argCount, NPVariant *result);
	static const char* js_clearHistory(AdapterBase *adapter, const NPVariant *args, uint32_t argCount, NPVariant *result);
	static const char* js_clickAt(AdapterBase *adapter, const NPVariant *args, uint32_t argCount, NPVariant *result);
	static const char* js_deleteImage(AdapterBase *adapter, const NPVariant *args, uint32_t argCount, NPVariant *result);
	static const char* js_generateIconFromFile(AdapterBase *adapter, const NPVariant *args, uint32_t argCount, NPVariant *result);
	static const char* js_getHistoryState(AdapterBase *adapter, const NPVariant *args, uint32_t argCount, NPVariant *result);
	static const char* js_goBack(AdapterBase *adapter, const NPVariant *args, uint32_t argCount, NPVariant *result);
	static const char* js_goForward(AdapterBase *adapter, const NPVariant *args, uint32_t argCount, NPVariant *result);
	static const char* js_interrogateClicks(AdapterBase *adapter, const NPVariant *args, uint32_t argCount, NPVariant *result);
	static const char* js_openURL(AdapterBase *adapter, const NPVariant *args, uint32_t argCount, NPVariant *result);
	static const char* js_pageScaleAndScroll(AdapterBase *adapter, const NPVariant *args, uint32_t argCount, NPVariant *result);
	static const char* js_reloadPage(AdapterBase *adapter, const NPVariant *args, uint32_t argCount, NPVariant *result);
	static const char* js_resizeImage(AdapterBase *adapter, const NPVariant *args, uint32_t argCount, NPVariant *result);
	static const char* js_saveViewToFile(AdapterBase *adapter, const NPVariant *args, uint32_t argCount, NPVariant *result);
	static const char* js_selectPopupMenuItem(AdapterBase *adapter, const NPVariant *args, uint32_t argCount, NPVariant *result);
	static const char* js_sendDialogResponse(AdapterBase *adapter, const NPVariant *args, uint32_t argCount, NPVariant *result);
	static const char* js_setMagnification(AdapterBase *adapter, const NPVariant *args, uint32_t argCount, NPVariant *result);
	static const char* js_setMinFontSize(AdapterBase *adapter, const NPVariant *args, uint32_t argCount, NPVariant *result);
	static const char* js_setPageIdentifier(AdapterBase *adapter, const NPVariant *args, uint32_t argCount, NPVariant *result);
	static const char* js_smartZoom(AdapterBase *adapter, const NPVariant *args, uint32_t argCount, NPVariant *result);
	static const char* js_setViewportSize(AdapterBase *adapter, const NPVariant *args, uint32_t argCount, NPVariant *result);
	static const char* js_stopLoad(AdapterBase *adapter, const NPVariant *args, uint32_t argCount, NPVariant *result);
	static const char* js_gestureStart(AdapterBase *adapter, const NPVariant *args, uint32_t argCount, NPVariant *result);
	static const char* js_gestureChange(AdapterBase *adapter, const NPVariant *args, uint32_t argCount, NPVariant *result);
	static const char* js_gestureEnd(AdapterBase *adapter, const NPVariant *args, uint32_t argCount, NPVariant *result);
	static const char* js_scrollStarting(AdapterBase *adapter, const NPVariant *args, uint32_t argCount, NPVariant *result);
	static const char* js_scrollEnding(AdapterBase *adapter, const NPVariant *args, uint32_t argCount, NPVariant *result);
	static const char* js_setEnableJavaScript(AdapterBase *adapter, const NPVariant *args, uint32_t argCount, NPVariant *result);
	static const char* js_setBlockPopups(AdapterBase *adapter, const NPVariant *args, uint32_t argCount, NPVariant *result);
	static const char* js_setHeaderHeight(AdapterBase *adapter, const NPVariant *args, uint32_t argCount, NPVariant *result);
	static const char* js_setAcceptCookies(AdapterBase *adapter, const NPVariant *args, uint32_t argCount, NPVariant *result);
    static const char* js_setShowClickedLink(AdapterBase *adapter, const NPVariant *args, uint32_t argCount, NPVariant *result);
	static const char* js_mouseEvent(AdapterBase* adapter, const NPVariant* args, uint32_t argCount, NPVariant* result);
	static const char* js_addElementHighlight(AdapterBase* adapter, const NPVariant* args, uint32_t argCount, NPVariant* result);
	static const char* js_removeElementHighlight(AdapterBase* adapter, const NPVariant* args, uint32_t argCount, NPVariant* result);
	static const char* js_connectBrowserServer(AdapterBase *adapter, const NPVariant *args, uint32_t argCount, NPVariant *result);
	static const char* js_disconnectBrowserServer(AdapterBase *adapter, const NPVariant *args, uint32_t argCount, NPVariant *result);
	static const char* js_pageFocused(AdapterBase *adapter, const NPVariant *args, uint32_t argCount, NPVariant *result);
	static const char* js_inspectUrlAtPoint(AdapterBase *adapter, const NPVariant *args, uint32_t argCount, NPVariant *result);
    static const char* js_isEditing(AdapterBase *adapter, const NPVariant *args, uint32_t argCount, NPVariant *result);
    static const char* js_insertStringAtCursor(AdapterBase *adapter, const NPVariant *args, uint32_t argCount, NPVariant *result);
    static const char* js_enableSelectionMode(AdapterBase *adapter, const NPVariant *args, uint32_t argCount, NPVariant *result);
    static const char* js_disableSelectionMode(AdapterBase *adapter, const NPVariant *args, uint32_t argCount, NPVariant *result);
    static const char* js_clearSelection(AdapterBase *adapter, const NPVariant *args, uint32_t argCount, NPVariant *result);
	static const char* js_saveImageAtPoint(AdapterBase *adapter, const NPVariant *args, uint32_t argCount, NPVariant *result);
	static const char* js_getImageInfoAtPoint(AdapterBase *adapter, const NPVariant *args, uint32_t argCount, NPVariant *result);
	static const char* js_selectAll(AdapterBase *adapter, const NPVariant *args, uint32_t argCount, NPVariant *result);
	static const char* js_cut(AdapterBase *adapter, const NPVariant *args, uint32_t argCount, NPVariant *result);
    static const char* js_copy(AdapterBase *adapter, const NPVariant *args, uint32_t argCount, NPVariant *result);
	static const char* js_paste(AdapterBase *adapter, const NPVariant *args, uint32_t argCount, NPVariant *result);
	static const char* js_interactiveAtPoint(AdapterBase *adapter, const NPVariant *args, uint32_t argCount, NPVariant *result);
	static const char* js_getElementInfoAtPoint(AdapterBase *adapter, const NPVariant *args, uint32_t argCount, NPVariant *result);
	static const char* js_setMouseMode(AdapterBase *adapter, const NPVariant *args, uint32_t argCount, NPVariant *result);
	static const char* js_startDeferWindowChange(AdapterBase *adapter, const NPVariant *args, uint32_t argCount, NPVariant *result);
	static const char* js_stopDeferWindowChange(AdapterBase *adapter, const NPVariant *args, uint32_t argCount, NPVariant *result);

	static const char* js_setSpotlight(AdapterBase *adapter, const NPVariant *args, uint32_t argCount, NPVariant *result);
	static const char* js_removeSpotlight(AdapterBase *adapter, const NPVariant *args, uint32_t argCount, NPVariant *result);
    static const char* js_setHTML(AdapterBase *adapter, const NPVariant *args, uint32_t argCount, NPVariant *result);
	static const char* js_disableEnhancedViewport(AdapterBase *adapter, const NPVariant *args, uint32_t argCount, NPVariant *result);
	static const char* js_ignoreMetaTags(AdapterBase *adapter, const NPVariant *args, uint32_t argCount, NPVariant *result);
	static const char* js_setNetworkInterface(AdapterBase *adapter, const NPVariant *args, uint32_t argCount, NPVariant *result);	
	static const char* js_enableFastScaling(AdapterBase *adapter, const NPVariant *args, uint32_t argCount, NPVariant *result);
	static const char* js_setDefaultLayoutWidth(AdapterBase *adapter, const NPVariant *args, uint32_t argCount, NPVariant *result);
    static const char* js_printFrame(AdapterBase *adapter, const NPVariant *args, uint32_t argCount, NPVariant *result);
	static const char* js_skipPaintHack(AdapterBase *adapter, const NPVariant *args, uint32_t argCount, NPVariant *result);
	static const char* js_findInPage(AdapterBase *adapter, const NPVariant *args, uint32_t argCount, NPVariant *result);
	static const char *js_mouseHoldAt(AdapterBase *adapter, const NPVariant *args, uint32_t argCount, NPVariant *result);
	static const char* js_handleFlick(AdapterBase *adapter, const NPVariant *args, uint32_t argCount, NPVariant *result);
	static const char* js_setVisibleSize(AdapterBase *adapter, const NPVariant *args, uint32_t argCount, NPVariant *result);
	static const char* js_setDNSServers(AdapterBase *adapter, const NPVariant *args, uint32_t argCount, NPVariant *result);	
    static const int kRecordBufferEmptyError = -10;
	
	static const int kExceptionMessageLength = 128;

protected:
	
	// AdapterBase overrides:
	virtual bool handlePenDown(NpPalmPenEvent *event);
	virtual bool handlePenUp(NpPalmPenEvent *event);
	virtual bool handlePenMove(NpPalmPenEvent *event);
	virtual bool handlePenClick(NpPalmPenEvent *event);
	virtual bool handlePenDoubleClick(NpPalmPenEvent *event);
	
	virtual bool handleKeyDown(NpPalmKeyEvent *event);
	virtual bool handleKeyUp(NpPalmKeyEvent *event);

	virtual bool handleTouchStart(NpPalmTouchEvent *event);
	virtual bool handleTouchEnd(NpPalmTouchEvent *event);
	virtual bool handleTouchMove(NpPalmTouchEvent *event);
	virtual bool handleTouchCancelled(NpPalmTouchEvent *event);

	virtual bool handleGesture(NpPalmGestureEvent *event);
	
	virtual void handlePaint(NpPalmDrawEvent* event);
	virtual void handleWindowChange(NPWindow* window);

	virtual bool handleFocus(bool val);

	// KineticScrollerListener overrides:
	virtual void scrollTo(int x, int y);
	virtual void startedAnimating();
	virtual void stoppedAnimating();
	
	// BrowserClientBase overrides:
	virtual void serverConnected();
	virtual void serverDisconnected();

	// Async message handlers inherited from BrowserClientBase:
	virtual void msgPainted(int32_t sharedBufferKey);
	virtual void msgReportError(const char* url, int32_t code, const char* msg);
	virtual void msgContentsSizeChanged(int32_t width, int32_t height);
	virtual void msgScrolledTo(int32_t contentsX, int32_t contentsY);
	virtual void msgLoadStarted();
	virtual void msgLoadStopped();
	virtual void msgLoadProgress(int32_t progress);
	virtual void msgLocationChanged(const char* uri, bool canGoBack, bool canGoForward);
	virtual void msgTitleChanged(const char* title);
	virtual void msgTitleAndUrlChanged(const char* title, const char* url, bool canGoBack, bool canGoForward);
	virtual void msgDialogAlert(const char* syncPipePath, const char* msg);
	virtual void msgDialogConfirm(const char* syncPipePath, const char* msg);
	virtual void msgDialogSSLConfirm(const char* syncPipePath, const char* host,int32_t code, const char* certFile);
	virtual void msgDialogPrompt(const char* syncPipePath, const char* msg, const char* defaultValue);
	virtual void msgDialogUserPassword(const char* syncPipePath, const char* msg);
	virtual void msgActionData(const char* dataType, const char* data);
	virtual void msgDownloadStart(const char* url);
	virtual void msgDownloadProgress(const char* url, int32_t totalSizeSoFar, int32_t totalSize);
	virtual void msgDownloadError(const char* url, const char* errorMsg);
	virtual void msgDownloadFinished(const char* url, const char* mimeType, const char* tmpFilePath);
	virtual void msgLinkClicked(const char* url);
	virtual void msgMimeHandoffUrl(const char* mimeType, const char* url);
	virtual void msgMimeNotSupported(const char* mimeType, const char* url);
	virtual void msgCreatePage(int32_t identifier);
	virtual void msgClickRejected(int32_t counter);
	virtual void msgPopupMenuShow(const char* identifier, const char* menuDataFileName);
	virtual void msgPopupMenuHide(const char* identifier);
	virtual void msgSmartZoomCalculateResponseSimple(int32_t pointX, int32_t pointY, int32_t left, int32_t top, int32_t right, int32_t bottom, int fullscreenSpotlightHandle);
	virtual void msgFailedLoad(const char* domain, int32_t errorCode, const char* failingURL, const char* localizedDescription);
	virtual void msgSetMainDocumentError(const char* domain, int32_t errorCode, const char* failingURL, const char* localizedDescription);
	virtual void msgEditorFocused(bool focused, int32_t fieldType, int32_t fieldActions);
	virtual void msgDidFinishDocumentLoad();
	virtual void msgUpdateGlobalHistory(const char* url, bool reload);
    virtual void msgPurgePage();
	virtual void msgInspectUrlAtPointResponse(int32_t queryNum, bool succeeded, const char* url, const char* desc, int32_t rectWidth, int32_t rectHeight, int32_t rectX, int32_t rectY);
	virtual void msgGetHistoryStateResponse(int32_t queryNum, bool canGoBack, bool canGoForward);
	virtual void msgIsEditing(int32_t queryNum, bool isEditing);
	virtual void msgUrlRedirected(const char* url, const char* userData);
	virtual void msgMetaViewportSet(double initialScale, double minimumScale, double maximumScale, int32_t width, int32_t height, bool userScalable);	
	virtual void msgHighlightRects(const char* rectsArrayJson);
	virtual void msgSaveImageAtPointResponse(int32_t queryNum, bool succeeded, const char* filepath);
	virtual void msgGetImageInfoAtPointResponse(int32_t queryNum, bool succeeded, const char* baseUri, const char* src,
			const char* title, const char* altText, int32_t width, int32_t height, const char* mimeType);
	virtual void msgGetElementInfoAtPointResponse(int32_t queryNum, bool succeeded, const char* element, const char* id,
			const char* name, const char* cname, const char* type, int32_t left, int32_t top, int32_t right, int32_t bottom, bool isEditable);
	virtual void msgMakePointVisible(int32_t x, int32_t y);
	virtual void msgIsInteractiveAtPointResponse(int32_t queryNum, bool interactive);
	virtual void msgCopiedToClipboard();
	virtual void msgPastedFromClipboard();
	virtual void msgRemoveSelectionReticle();
	virtual void msgCopySuccessResponse(int32_t queryNum, bool success);
	virtual void msgPluginFullscreenSpotlightCreate(int32_t spotlightHandle, int32_t rectX, int32_t rectY, int32_t rectWidth, int32_t rectHeight);
	virtual void msgPluginFullscreenSpotlightRemove();
	virtual void msgSpellingWidgetVisibleRectUpdate(int32_t rectX, int32_t rectY, int32_t rectWidth, int32_t rectHeight);
	virtual void msgHitTestResponse(int32_t queryNum, const char *hitTestResultJson);
    virtual void msgAddFlashRects(const char* rectsArrayJson);
    virtual void msgRemoveFlashRects(const char* rectsArrayJson);
	virtual void msgShowPrintDialog();
	virtual void msgGetTextCaretBoundsResponse(int32_t queryNum, int32_t left, int32_t top, int32_t right, int32_t bottom);
	virtual void msgUpdateScrollableLayers(const char* json);
	
private:
    /* TODO: We should get this from the webkit headers */
    enum InteractiveRectType {
        InteractiveRectDefault,
        InteractiveRectPlugin   //< Interactive rect is a plugin rect
    };

    struct Point {
        Point(int x, int y) { this->x = x; this->y = y; }
        Point(const Point& rhs) : x(rhs.x), y(rhs.y) {}
        void set(int x, int y) { this->x = x; this->y = y; }
        bool operator==(const Point& rhs) const {
            return x == rhs.x && y == rhs.y;
        }
        Point& operator=(const Point& rhs) {
            x = rhs.x;
            y = rhs.y;
            return *this;
        }
        int x;
        int y;
    };

	struct BrowserServerCallArgs {
		BrowserServerCallArgs(int queryNum) : m_queryNum(queryNum) {}
        virtual ~BrowserServerCallArgs() {}
		int			m_queryNum;		///< The query number used to associate the reply with the correct call.
	};

	/**
	 * We save arguments to inspectUrlAtPoint in this object so that when we receive
	 * the response we can handle it properly.
	 */
	struct InspectUrlAtPointArgs : public BrowserServerCallArgs {
		int			m_x; 			///< The orginally supplied X coordinate.
		int			m_y;			///< The originally supplied Y coordinate.
		NPObject*	m_successCb; 	///< The success callback function.
		
		InspectUrlAtPointArgs(int x, int y, NPObject* cb, int queryNum);
		virtual ~InspectUrlAtPointArgs ();
	};

	struct GetImageInfoAtPointArgs : public BrowserServerCallArgs {
		int			m_x; 			///< The orginally supplied X coordinate.
		int			m_y;			///< The originally supplied Y coordinate.
		NPObject*	m_callback; 	///< The success callback function.
		
		GetImageInfoAtPointArgs(int x, int y, NPObject* cb, int queryNum);
		virtual ~GetImageInfoAtPointArgs ();
	};

	struct IsInteractiveAtPointArgs : public BrowserServerCallArgs {
		int			m_x; 			///< The orginally supplied X coordinate.
		int			m_y;			///< The originally supplied Y coordinate.
		NPObject*	m_callback; 	///< The success callback function.
		
		IsInteractiveAtPointArgs(int x, int y, NPObject* cb, int queryNum);
		virtual ~IsInteractiveAtPointArgs ();
	};

	struct GetElementInfoAtPointArgs : public BrowserServerCallArgs {
		int			m_x; 			///< The orginally supplied X coordinate.
		int			m_y;			///< The originally supplied Y coordinate.
		NPObject*	m_callback; 	///< The success callback function.
		
		GetElementInfoAtPointArgs(int x, int y, NPObject* cb, int queryNum);
		virtual ~GetElementInfoAtPointArgs ();
	};

	struct GetHistoryStateArgs : public BrowserServerCallArgs {
		NPObject*	m_successCb; 	///< The success callback function.
		
		GetHistoryStateArgs(NPObject* cb, int queryNum);
		virtual ~GetHistoryStateArgs ();
	};
	
	struct IsEditingArgs : public BrowserServerCallArgs {
	    NPObject* m_callback;       ///< js callback fired when reply is ready
	    
	    IsEditingArgs(NPObject* callback, int queryNum);
	    virtual ~IsEditingArgs();
	};

    struct CopySuccessCallbackArgs : public BrowserServerCallArgs {
        NPObject* m_callback;       ///< js callback fired when reply is ready

        CopySuccessCallbackArgs(NPObject* callback, int queryNum);
        virtual ~CopySuccessCallbackArgs();
    };

    struct SaveImageAtPointArgs : public BrowserServerCallArgs {
		int			m_x; 			///< The orginally supplied X coordinate.
		int			m_y;			///< The originally supplied Y coordinate.
	    NPObject* m_callback;       ///< js callback fired when reply is ready
	    
	    SaveImageAtPointArgs(int x, int y, NPObject* cb, int queryNum);
	    virtual ~SaveImageAtPointArgs();
	};

	struct HitTestArgs : public BrowserServerCallArgs {
		const char *m_type;
        Point m_pt;
		int m_modifiers;

		HitTestArgs(const char *type, const Point& pt, int modifiers, int queryNum);
		virtual ~HitTestArgs();
	};

	struct GetTextCaretArgs : public BrowserServerCallArgs {
		NPObject *m_callback;

		GetTextCaretArgs(NPObject *cb, int queryNum);
		virtual ~GetTextCaretArgs();
	};

	struct UrlRedirectInfo {
		std::string re;
		bool redir;
		std::string udata;
		int type;
		
		UrlRedirectInfo( const char* urlRe, bool redirect, const char* userData, int redirectType ) :
			re(urlRe), redir(redirect), udata(userData), type(redirectType) {}
	};
	
	struct SelectionReticleInfo {
	    PGSurface* surface;
	    GSource* timeoutSource;
	    int x;
	    int y;
	    int centerOffsetX;
	    int centerOffsetY;
        bool show;
	};

	/**
	 * A collection of arguments that have been used for a call into BrowserServer that we're waiting
	 * for a reply on.
	 */
	template<typename T>
	class ArgList
	{
        private:
        typedef std::list<T*>  myArgList;
		public:
		~ArgList() {
			typename myArgList::const_iterator i;
			for (i = m_args.begin(); i != m_args.end(); ++i) {
				delete *i;
			}
		}

		/**
		 * Add an argument object to this list. We now own the pointer.
		 */
		void add(T* arg) {
			m_args.push_back(arg);
		}

		/**
		 * Find an argument object whose query number matches the one provided.
		 * <strong>The caller now owns the argument object.</strong>
		 */
		T* get(int queryNum) {
			typename myArgList::iterator i;
			for (i = m_args.begin(); i != m_args.end(); ++i) {
				T* args = *i;
				if (args->m_queryNum == queryNum) {
					m_args.erase(i);	// Caller now owns this pointer
					return args;
				}
			}
			return NULL;	// Not found
		};

		private:
		myArgList	m_args;
	};

	KineticScroller* mScroller;
	PGSurface* mDirtyPattern;


    Point m_pageOffset; // Position of top left corner relative to a Netscape page
    Point m_touchPtDoc; // last touch press or touch move point in document coordinates


	// viewport size
	int mViewportWidth;
	int mViewportHeight;

	double mZoomLevel;
	bool mZoomFit;
	int mPageWidth;
	int mPageHeight;
	int mContentWidth;
	int mContentHeight;
    Point mScrollPos;
	bool mInGestureChange;

	BrowserMetaViewport* mMetaViewport;
	BrowserCenteredZoom* mCenteredZoom;

	BrowserOffscreen* mOffscreen0;
	BrowserOffscreen* mOffscreen1;
	BrowserOffscreen* mOffscreenCurrent;

	PGSurface* mFrozenSurface;
	bool mFrozen;
    Point mFrozenRenderPos;
	int mFrozenRenderWidth;
	int mFrozenRenderHeight;
	float mFrozenZoomFactor;
	
	bool m_passInputEvents;
	bool mFirstPaintComplete; ///< Set once the first non-empty paint has been completed.
	bool mEnableJavaScript;
	bool mBlockPopups;
	bool mAcceptCookies;
	bool mShowClickedLink;

	int m_defaultLayoutWidth;

	bool mBrowserServerConnected;   ///< Is this adapter currently connected to the BrowserServer?
	bool mNotifiedOfBrowserServerDisconnect;    ///< Has our owner been notified of a BrowserServer disconnect?
	
	bool mSendFinishDocumentLoadNotification;  ///< True when load is complete but didFinishDocumentLoad not sent

	std::list<UrlRedirectInfo*> m_urlRedirects; ///< an ordered list of URL's to redirect back to the app
	
	int16_t 			mArgc;		///< Saved copy of argc when adapter is initialized
	char**			mArgn;		///< Saved copy of argn when adapter is initialized
	char** 			mArgv;		///< Saved copy of argv when adapter is initialized
	bool			mPaintEnabled;
	bool            mInScroll;
	bool			bEditorFocused;	///< Is the current page focused.

	bool m_useFastScaling;
	double m_startingZoomLevel;

    int32_t         mPageIdentifier;

	ArgList<InspectUrlAtPointArgs>	m_inspectUrlArgs;
	ArgList<GetHistoryStateArgs>	m_historyStateArgs;
	ArgList<IsEditingArgs>        	m_isEditingArgs;
	ArgList<GetImageInfoAtPointArgs>m_getImageInfoAtPointArgs;
	ArgList<IsInteractiveAtPointArgs>m_isInteractiveAtPointArgs;
	ArgList<SaveImageAtPointArgs>	m_saveImageAtPointArgs;
	ArgList<GetElementInfoAtPointArgs>	m_getElementInfoAtPointArgs;
	ArgList<CopySuccessCallbackArgs> m_copySuccessCallbackArgs;
	ArgList<HitTestArgs> m_hitTestArgs;
	ArgList<GetTextCaretArgs> m_getTextCaretArgs;
	
	int				mBsQueryNum;
	bool            m_interrogateClicks;
	int				mMouseMode;
	bool			mLogAlertMessages;

	bool mPageFocused; ///< Is the onwer of this adapter focused?
    bool mServerConnectedInvoked; // has successfully invoked serverConnected
    bool mServerConnectedInvoking; // in calling serverConnected

    bool mShowHighlight;
    std::vector<BrowserRect> mHighlightRects;

    Point mLastPointSentToFlash;
    bool mMouseInFlashRect;
    bool mFlashGestureLock;
    typedef std::map<uintptr_t, BrowserRect> RectMap;
    typedef std::pair<uintptr_t, BrowserRect> RectMapItem;
    RectMap mFlashRects;

    bool mMouseInInteractiveRect;
    RectMap mDefaultInteractiveRects;

	BrowserScrollableLayerMap mScrollableLayers;
	BrowserScrollableLayerScrollSession mScrollableLayerScrollSession;
    
    SelectionReticleInfo mSelectionReticle;
    
    static gboolean removeSelectionReticleCallback(gpointer arg);

	// the highlight area (mouse, gesture are directly send to it)
	int m_spotlightHandle;
	int m_spotlightAlpha; // 0~ 255
	BrowserRect m_spotlightRect;

	pbnjson::JSchemaFile m_hitTestSchema;
	
	bool init();
	bool initializeIpcBuffer();
	void setDefaultViewportSize();
	void sendStateToServer();

	// gesture handling
	void doGestureStart(int cx, int cy, float scale, float rotate, int center_x, int center_y);
	void doGestureChange(int cx, int cy, float scale, float rotate, int center_x, int center_y, bool isGestureEnd=false);
	void doGestureEnd(int cx, int cy, float scale, float rotate, int center_x, int center_y);

	void sendGestureStart(int cx, int cy, float scale, float rotate, int center_x, int center_y);
	void sendGestureChange(int cx, int cy, float scale, float rotate, int center_x, int center_y);
	void sendGestureEnd(int cx, int cy, float scale, float rotate, int center_x, int center_y);
	void sendSingleTap(int cx, int cy, int modifiers);

	// touch handling
	bool doTouchEvent(int32_t type, NpPalmTouchEvent *event);
	
	static BrowserAdapter* GetAndInitAdapter( AdapterBase* adapter );
	static void addJsonProperty(struct json_value* root, const char* name, const char* value);
	static void addJsonProperty(struct json_value* root, const char* name, int32_t value);
	static void addJsonProperty(struct json_value* root, const char* name, bool value);
	static int readPngFile(const char* pszFileName, uint32_t* &pPixelData, int &nImageWidth, int &nImageHeight);
	static int readPngFile(const char* pszFileName, PContext2D& context, PPixmap* &pPixmap, 
										   int &nImageWidth, int &nImageHeight);
	static int preScaleImage(PPixmap* pPixmap, int nImageWidth, int nImageHeight);
	static int writePngFile(const char* pszFileName, const uint32_t* pPixelData, int nImageWidth, int nImageHeight);
	static bool isSafeDir(const char* pszFileName);
	bool prvSmartZoom(const Point& pt);
	void invalidate(void);
	void handlePaintInFrozenState(NpPalmDrawEvent* event);

	void scale(double zoom);
	void scaleAndScrollTo(double zoom, int x, int y);
    void fireScrolledToEvent(int x, int y);
	bool isActivated();

    void setPageIdentifier(int32_t identifier);

	void enableFastScaling(bool enable);

	int showHighlight(PGContext* gc);
	void removeHighlight();
	void invalidateHighlightRectsRegion();

	void updateMouseInFlashStatus(bool inFlashRect);
	void updateFlashGestureLockStatus(bool gestureLockEnabled);
	bool flashRectContainsPoint(const Point& pt);
	bool rectContainsPoint(RectMap &rectMap, const Point& pt);
	void updateMouseInInteractiveStatus(bool inInteractiveRect);
	bool interactiveRectContainsPoint(const Point& pt);
	void jsonToRects(const char* rectsArrayJson);

	bool detectScrollableLayerUnderMouseDown(const Point& pagePt, const Point& mousePt);
	bool scrollLayerUnderMouse(const Point& currentMousePtDoc);
	void resetScrollableLayerScrollSession();
	void showActiveScrollableLayer(PGContext* gc);

	// used to create a scrim around a plugin rectangle "spotlight"
	int showSpotlight(PGContext* gc);

	void initSelectionReticleSurface();
	void showSelectionReticle(PGContext* gc);
	void invalidateSelectionReticleRegion();

	bool sendRequestOffscreenChange(bool sync, int asyncDelayMillis);

    bool isPointInList(const Point &point, const NpPalmTouchList &list);

	bool shouldPassInputEvents();
	bool shouldPassTouchEvents();

	struct timeval m_lastPassedEventTime;

	unsigned long long m_ft;

	void scrollCaretIntoViewAfterResize(int oldwidth, int oldheight, double oldZoomLevel);
	BrowserRect m_textCaretRect;

    Point m_clickPt;    ///< Point where mouse clicked (minus header)
    Point m_penDownDoc;    ///< Mouse down location in scaled document coordinates

	bool m_dragging;
	bool m_didDrag;
	bool m_didHold;
	bool m_sentToBS;

	GSource *m_clickTimerSource;
	static gboolean clickTimeoutCb(gpointer data);
	void startClickTimer();
	void stopClickTimer();

	GSource *m_mouseHoldTimerSource;
	static gboolean mouseHoldTimeoutCb(gpointer data);
	void startMouseHoldTimer();
	void stopMouseHoldTimer();

	struct SentMouseHoldEvent {
	    SentMouseHoldEvent() : pt(0,0), sent(false) {}
	    void reset() { sent = false; }
	    void set(int cx, int cy)
	    {
            pt.set(cx, cy);
	        sent = true;
	    }

        Point pt;
	    bool sent;
	};
	SentMouseHoldEvent m_sentMouseHoldEvent;

	GSource *m_zoomTimerSource;
	static gboolean zoomTimerTimeoutCb(gpointer data);
	bool animateZoom();
	void startZoomAnimation(double zoom, int x, int y);
	void stopZoomAnimation();
    Point m_zoomPt;
	double m_zoomTarget;
	int m_zoomXInterval;
	int m_zoomYInterval;
	double m_zoomLevelInterval;
    int m_headerHeight; ///< Height (in pixels) of the header which is a space above the page we don't draw.

    void showScrollbar();
    void startFadeScrollbar();
    void stopFadeScrollbar();
    static gboolean scrollbarFadeoutCb(gpointer data);

    int mScrollbarOpacity;
    GSource* mScrollbarFadeSource;

	struct RecordedGestureEntry {
		RecordedGestureEntry(float s, float r, int cX, int cY)
			: scale(s)
			, rotate(r)
			, centerX(cX)
			, centerY(cY) {
		}

		float scale;
		float rotate;
		int centerX;
		int centerY;
	};

	std::list<RecordedGestureEntry> m_recordedGestures;

	void recordGesture(float s, float r, int cX, int cY);
	RecordedGestureEntry getAveragedGesture() const;
    bool destroyBufferLock();
    bool createBufferLock();
    sem_t* m_bufferLock;
    char* m_bufferLockName;

	friend class BrowserAdapterData;
};


#endif /* BROWSERADAPTER_H */
