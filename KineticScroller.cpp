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

#include <stdio.h>
#include <sys/time.h>
#include <stdlib.h>
#include <math.h>

#include "KineticScroller.h"

/*
 * The basis for this code comes from the enyo framework ScrollMath.js file,
 * which is located in enyo in framework/source/base/base/scroller/ScrollMath.js.
 * A few of the variables are named slightly differently in this file:
 *  this.x0   ==> m_lastMouseX
 *  this.y0   ==> m_lastMouseY
 *  this.x    ==> m_scrollX
 *  this.y    ==> m_scrollY
 *  this.job  ==> m_timerSource
 */

//#define DEBUG
//#define TRACE_SIMULATION

#ifdef DEBUG
#define TRACEF(fmt, args...)                                      \
do {                                                              \
    fprintf(stderr, "(%s:%d) %s: "fmt"\n",                        \
                    __FILE__, __LINE__, __FUNCTION__, ##args);    \
} while(0)
#else
#define TRACEF(fmt, args...) (void)0
#endif

#ifdef TRACE_SIMULATION
#define TRACEF_SIMULATE(fmt, args...)                             \
do {                                                              \
    fprintf(stderr, "(%s:%d) %s: "fmt"\n",                        \
                    __FILE__, __LINE__, __FUNCTION__, ##args);    \
} while(0)
#else
#define TRACEF_SIMULATE(fmt, args...) (void)0
#endif

// Define this for use with systems that don't send flick events
#undef SIMULATED_FLICKS

#ifdef SIMULATED_FLICKS
static const double kSimulatedFlickVelocity = 0.5;
#endif

// 'spring' damping returns the scroll position to a value inside the boundaries (lower provides FASTER snapback)
static const double kSpringDamping = 0.93;

// 'drag' damping resists dragging the scroll position beyond the boundaries (lower provides MORE resistance)
static const double kDragDamping = 0.5;

// 'friction' damping reduces momentum over time (lower provides MORE friction)
// enyo scroller uses 0.98 as the value here, for browser we want the scrolling to be a little tighter
static const double kFrictionDamping = 0.95;

// Additional 'friction' damping applied when momentum carries the viewport into overscroll (lower provides MORE friction)
static const double kSnapFriction = 0.9;

// Scalar applied to 'flick' event velocity
static const double kFlickScalar = 0.02;

// the value used in friction() to determine if the deta (e.g. y - y0) is close enough to zero to consider as zero.
static const double kFrictionEpsilon = 1e-2;

// timer interval in ms
static const int kTimerInterval = 16;

static const int kFrame = 10;

KineticScroller::KineticScroller(GMainContext* glibCtxt)
    : m_glibCtxt(glibCtxt)
    , m_listener(0)
    , m_scrollX(0)
    , m_scrollY(0)
    , m_viewportWidth(0)
    , m_viewportHeight(0)
    , m_contentWidth(0)
    , m_contentHeight(0)
    , m_lastMouseX(0)
    , m_lastMouseY(0)
    , m_timerSource(0)
    , m_dragging(false)
    , m_t(0)
    , m_t0(0)
    , m_x0(0)
    , m_y0(0)
    , m_my(0)
    , m_py(0)
    , m_uy(0)
    , m_mx(0)
    , m_px(0)
    , m_ux(0)
    , m_scrollLock(ScrollLockInvalid)
{
}

KineticScroller::~KineticScroller()
{
    stop();
}

double KineticScroller::getTime()
{
    struct timespec time;
    clock_gettime(CLOCK_MONOTONIC, &time);
    return (double)time.tv_sec * 1000.0 + (double)time.tv_nsec / 1000000.0;
}

double KineticScroller::topBoundary() const
{
    return 0;
}

double KineticScroller::bottomBoundary() const
{
    return m_viewportHeight - m_contentHeight;
}

double KineticScroller::leftBoundary() const
{
    return 0;
}

double KineticScroller::rightBoundary() const
{
    return m_viewportWidth - m_contentWidth;
}

/**
    Simulation constraints (spring damping occurs here)
*/
void KineticScroller::constrain()
{
    double y = boundaryDamping(m_scrollY, topBoundary(), bottomBoundary(), kSpringDamping);
    if (y != m_scrollY) {
        // ensure snapping introduces no velocity, add additional friction
        m_lastMouseY = y - (m_scrollY - m_lastMouseY) * kSnapFriction;
        m_scrollY = y;
    }
    double x = boundaryDamping(m_scrollX, leftBoundary(), rightBoundary(), kSpringDamping);
    if (x != m_scrollX) {
        m_lastMouseX = x - (m_scrollX - m_lastMouseX) * kSnapFriction;
        m_scrollX = x;
    }
}

void KineticScroller::verlet()
{
    double x = m_scrollX;
    m_scrollX += x - m_lastMouseX;
    m_lastMouseX = x;

    double y = m_scrollY;
    m_scrollY += y - m_lastMouseY;
    m_lastMouseY = y;
}

/**
        The friction function
*/
void KineticScroller::friction(double& inEx, double& inEx0, double inCoeff)
{
    // implicit velocity
    double dp = inEx - inEx0;
    // let close-to-zero collapse to zero (smaller than epsilon is considered zero)
    double c = abs(dp) > kFrictionEpsilon ? inCoeff : 0;
    // reposition using damped velocity
    inEx = inEx0 + c * dp;
}

double KineticScroller::simulate(double t)
{
    while (t >= kFrame) {
        TRACEF_SIMULATE("simulate: t: %f\n", t);
        t -= kFrame;

        if (!m_dragging) {
            TRACEF_SIMULATE("before constrain (%f,%f) (%f,%f)\n", m_scrollX, m_scrollY, m_lastMouseX, m_lastMouseY);
            constrain();
            TRACEF_SIMULATE("after constrain (%f,%f) (%f,%f)\n", m_scrollX, m_scrollY, m_lastMouseX, m_lastMouseY);
        }

        TRACEF_SIMULATE("before verlet (%f,%f) (%f,%f)\n", m_scrollX, m_scrollY, m_lastMouseX, m_lastMouseY);
        verlet();
        TRACEF_SIMULATE("after verlet (%f,%f) (%f,%f)\n", m_scrollX, m_scrollY, m_lastMouseX, m_lastMouseY);
        TRACEF_SIMULATE("before friction (%f,%f) (%f,%f)\n", m_scrollX, m_scrollY, m_lastMouseX, m_lastMouseY);
        friction(m_scrollY, m_lastMouseY, kFrictionDamping);
        friction(m_scrollX, m_lastMouseX, kFrictionDamping);
        TRACEF_SIMULATE("after friction (%f,%f) (%f,%f)\n", m_scrollX, m_scrollY, m_lastMouseX, m_lastMouseY);
    }

    return t;
}

gboolean KineticScroller::timeoutCb(gpointer data)
{
    KineticScroller* scroller = (KineticScroller*)data;

    // schedule next frame
    //this.job = window.setTimeout(fn, this.interval);

    // wall-clock time
    double t1 = scroller->getTime();

    // delta from last wall clock time
    double dt = t1 - scroller->m_t0;

    TRACEF("dt: %f\n", dt);

    // record the time for next delta
    scroller->m_t0 = t1;

    //
    // Slop factor of up to 5ms is observed in Chrome, much higher on devices.
    // One theory is that the low-level timer fires as posts some kind of message
    // in a queue, and there is latency in queue processing.
    // So the timer may fire every 20ms, but if the first timer message is delayed
    // processing for 38ms, and the next message fires without delay, the perceived
    // wall-clock delta is 2ms. Overall however, the frequency is still 20ms (that is
    // two timer messages were processed in 40ms total).
    // We should ask Mark Lam to verify/dispel this theory, but it's relatively unimportant
    // as the animation algorithm can deal (although smoother timer processing will yield
    // smoother animation). There is a conditional below (t < this.frame) to handle this situation.
    //
    if (dt < kTimerInterval - 5) {
        TRACEF("wall-clock delta %f ms is significantly less than timer interval %d ms", dt, kTimerInterval);
    }

#if 0
    //
    // *********************************************
    // for fps tracking only
    ft += dt;
    // keep a moving average
    dtf.push(dt);
    if (++fs == 20) {
        fs--;
        ft -= dtf.shift();
    }
    // make this value available to something
    this.fps = (fs * 1000 / ft).toFixed(1) + " fps";
    // *********************************************
    //
#endif

    // user drags override animation
    if (scroller->m_dragging) {
        scroller->m_lastMouseY = scroller->m_scrollY = scroller->m_uy;
        scroller->m_lastMouseX = scroller->m_scrollX = scroller->m_ux;
    }

    //
    // frame-time accumulator
    scroller->m_t += dt;

    // we don't expect t to be less than frame, unless the wall-clock interval
    // was very much less than expected (which can occur, see note above)
    if (scroller->m_t < kFrame) {
        return TRUE;
    }
    // consume some t in simulation
    scroller->m_t = scroller->simulate(scroller->m_t);

    //
    // scroll if we have moved, otherwise the animation is stalled and we can stop
    if (scroller->m_y0 != scroller->m_scrollY || scroller->m_x0 != scroller->m_scrollX) {
        //this.scroll();
        TRACEF("scrolling scrollX: %f, scrollY: %f\n", scroller->m_scrollX, scroller->m_scrollY);
        scroller->m_listener->scrollTo(scroller->m_scrollX, scroller->m_scrollY);
    } else if (!scroller->m_dragging) {
        //stop(true);
        scroller->stop();

        //this.fps = "stopped";

        //this.scroll();
        TRACEF("stopping scrollX: %f, scrollY: %f\n", scroller->m_scrollX, scroller->m_scrollY);
        scroller->m_listener->scrollTo(scroller->m_scrollX, scroller->m_scrollY);
    }
    scroller->m_y0 = scroller->m_scrollY;
    scroller->m_x0 = scroller->m_scrollX;

    return TRUE;
}


void KineticScroller::setListener(KineticScrollerListener* listener)
{
    m_listener = listener;
}

void KineticScroller::setViewportDimensions(int width, int height)
{
    m_viewportWidth = width;
    m_viewportHeight = height;

    m_contentWidth = MAX(m_viewportWidth, m_contentWidth);
    m_contentHeight = MAX(m_viewportHeight, m_contentHeight);

    m_lastMouseX = m_scrollX;
    m_lastMouseY = m_scrollY;
    start();

    TRACEF("viewport dimensions (%f x %f), conent dimensions (%f x %f)\n", m_viewportWidth, m_viewportHeight, m_contentWidth, m_contentHeight);
}

void KineticScroller::setContentDimensions(int width, int height)
{
    m_contentWidth = width;
    m_contentHeight = height;

    m_contentWidth = MAX(m_viewportWidth, m_contentWidth);
    m_contentHeight = MAX(m_viewportHeight, m_contentHeight);

    TRACEF("viewport dimensions (%f x %f), conent dimensions (%f x %f)\n", m_viewportWidth, m_viewportHeight, m_contentWidth, m_contentHeight);

    // FIXME: Should adjust scroll offsets if neeeded
}

void KineticScroller::scrollTo(int x, int y, bool animate)
{
    if (animate) {
        m_scrollY = -1 * (-m_lastMouseY - (y + -m_lastMouseY) * (1 - kFrictionDamping));
        m_scrollX = -1 * (-m_lastMouseX - (x + -m_lastMouseX) * (1 - kFrictionDamping));
        start();
    } else {
        m_scrollY = m_lastMouseY = y;
        m_scrollX = m_lastMouseX = x;
        start();
    }
}

void KineticScroller::handleMouseDown(int x, int y)
{
    TRACEF("(%d, %d)\n", x, y);

    if (m_timerSource) {

        // still in a flick scroll
        switch (m_scrollLock) {
        case ScrollLockHoriz:
            y = m_recordedMouseY;
            break;
        case ScrollLockVert:
            x = m_recordedMouseX;
            break;
        default:
            m_scrollLock = ScrollLockInvalid;
            m_recordedMouseX = x;
            m_recordedMouseY = y;
            break;
        }
    }
    else {
        m_scrollLock = ScrollLockInvalid;
        m_recordedMouseX = x;
        m_recordedMouseY = y;
    }

    m_dragging = true;
    m_my = y;
    m_py = m_uy = m_scrollY;

    m_mx = x;
    m_px = m_ux = m_scrollX;
}

/**
        Boundary damping function.
        Return damped 'value' based on 'coeff' on one side of 'origin'.
*/
double KineticScroller::damping(double value, double origin, double coeff, int sign)
{
    double kEpsilon = 0.5;
    //
    // this is basically just value *= coeff (generally, coeff < 1)
    //
    // 'sign' and the conditional is to force the damping to only occur
    // on one side of the origin.
    //
    // Force close to zero to be zero
    if (abs(value-origin) < kEpsilon) {
        return origin;
    }
    return value*sign > origin*sign ? coeff * (value-origin) + origin : value;
}

/**
        Dual-boundary damping function.
        Return damped 'value' based on 'coeff' when exceeding either boundary.
*/
double KineticScroller::boundaryDamping(double value, double aBoundary, double bBoundary, double coeff)
{
    return damping(damping(value, aBoundary, coeff, 1), bBoundary, coeff, -1);
}

void KineticScroller::handleMouseMove(int x, int y)
{
    TRACEF("(%d, %d)\n", x, y);

    if (m_scrollLock == ScrollLockInvalid)
        trackScrollLock(x, y);

    switch (m_scrollLock) {
    case ScrollLockHoriz: {
        y = m_recordedMouseY;
        break;
    }
    case ScrollLockVert: {
        x = m_recordedMouseX;
        break;
    }
    case ScrollLockFree:
    default:
        break;
    }

    if (m_dragging) {
#if 0
        int dx = x - m_lastMouseX;
        int dy = y - m_lastMouseY;

        m_lastMouseX = x;
        m_lastMouseY = y;

        m_scrollX += dx;
        m_scrollY += dy;
#endif
        double dy = y - m_my; // TODO: vertical
        m_uy = dy + m_py;
        // provides resistance against dragging into overscroll
        m_uy = boundaryDamping(m_uy, topBoundary(), bottomBoundary(), kDragDamping);

        double dx = x - m_mx; // TODO: horizontal
        m_ux = dx + m_px;
        // provides resistance against dragging into overscroll
        m_ux = boundaryDamping(m_ux, leftBoundary(), rightBoundary(), kDragDamping);

        start();
    }
}

void KineticScroller::animate()
{
    stop();

    // time tracking
    m_t0 = getTime();
    m_t = 0;

#if 0
    // only for fps tracking
    var fs = 0, ft = 0, dtf = [], dt;

#endif
    // delta tracking
    m_x0 = 0;
    m_y0 = 0;

    // animation cadence
    m_timerSource = g_timeout_source_new(kTimerInterval);
    g_source_set_callback(m_timerSource, timeoutCb, this /*data*/, NULL);
    g_source_attach(m_timerSource, m_glibCtxt);
    g_source_unref(m_timerSource);

    m_listener->startedAnimating();
}

void KineticScroller::start()
{
    if (!m_timerSource) {
        animate();
        //doScrollStart();
    }
}

void KineticScroller::handleMouseUp(int x, int y)
{
    TRACEF("(%d, %d)\n", x, y);

    if (m_dragging) {

#ifdef SIMULATED_FLICKS
        m_scrollY = m_uy;
        m_lastMouseY = m_scrollY - (m_scrollY - m_lastMouseY) * kSimulatedFlickVelocity;
        m_scrollX = m_ux;
        m_lastMouseX = m_scrollX - (m_scrollX - m_lastMouseX) * kSimulatedFlickVelocity;

        m_listener->scrollTo(m_scrollX, m_scrollY);
#endif

        m_dragging = false;
    }

}

void KineticScroller::handleMouseFlick(int xVel, int yVel)
{
    TRACEF("velocity: (%d, %d)\n", xVel, yVel);

    if (m_dragging) {

        if (m_scrollLock == ScrollLockInvalid) {
            if (abs(xVel) > abs(yVel))
                m_scrollLock = ScrollLockHoriz;
            else
                m_scrollLock = ScrollLockVert;
        }

        switch (m_scrollLock) {
        case ScrollLockHoriz:
            yVel = 0;
            break;
        case ScrollLockVert:
            xVel = 0;
            break;
        default:
            break;
        }

        m_scrollY = m_lastMouseY + yVel * kFlickScalar;
        m_scrollX = m_lastMouseX + xVel * kFlickScalar;
        m_listener->scrollTo(m_scrollX, m_scrollY);
        m_dragging = false;
        start();
    }
}

void KineticScroller::stop()
{
    m_listener->stoppedAnimating();

    if (m_timerSource) {
        g_source_destroy(m_timerSource);
        //g_source_unref(m_timerSource);
        m_timerSource = NULL;
    }

    //inFireEvent && this.doScrollStop();
}

void KineticScroller::trackScrollLock(int x, int y)
{
    int dx = x - m_recordedMouseX;
    int dy = y - m_recordedMouseY;

    // dx / dy = tan(angle).
    // angle < 22.5 - horizontal lock, angle > 67.5 - vertical lock
    if (abs(dx) > 2.4142 * abs(dy))
        m_scrollLock = ScrollLockHoriz;
    else if (abs(dx) < 0.4142 * abs(dy))
        m_scrollLock = ScrollLockVert;
    else
        m_scrollLock = ScrollLockFree;
}
