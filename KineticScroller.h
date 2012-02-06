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

#ifndef KINETICSCROLLER_H
#define KINETICSCROLLER_H

#include <glib.h>

class KineticScrollerListener
{
public:

    KineticScrollerListener() {}
    virtual ~KineticScrollerListener() {}
    virtual void scrollTo(int x, int y) = 0;
    virtual void startedAnimating() = 0;
    virtual void stoppedAnimating() = 0;
};



class KineticScroller
{
public:

    KineticScroller(GMainContext* glibCtxt);
    ~KineticScroller();

    void setListener(KineticScrollerListener* listener);

    void setViewportDimensions(int width, int height);
    void setContentDimensions(int width, int height);

    void scrollTo(int x, int y, bool animate);

    void handleMouseDown(int x, int y);
    void handleMouseMove(int x, int y);
    void handleMouseUp(int x, int y);
    void handleMouseFlick(int xVel, int yVel);

private:

    double getTime();
    void constrain();
    void verlet();
    void friction(double& inEx, double& inEx0, double inCoeff);
    double simulate(double t);

    double boundaryDamping(double value, double aBoundary, double bBoundary, double coeff);
    double damping(double value, double origin, double coeff, int sign);

    double topBoundary() const;
    double bottomBoundary() const;
    double leftBoundary() const;
    double rightBoundary() const;

    void start();
    void stop();
    void animate();

    GMainContext* m_glibCtxt;
    KineticScrollerListener* m_listener;

    static gboolean timeoutCb(gpointer data);

    void trackScrollLock(int x, int y);

    double m_scrollX;
    double m_scrollY;
    double m_viewportWidth;
    double m_viewportHeight;
    double m_contentWidth;
    double m_contentHeight;
    double m_lastMouseX;
    double m_lastMouseY;

    GSource* m_timerSource;
    bool m_dragging;

    double m_t;
    double m_t0;

    double m_x0;
    double m_y0;

    double m_my;
    double m_py;
    double m_uy;

    double m_mx;
    double m_px;
    double m_ux;

private:

    enum ScrollLock {
        ScrollLockInvalid = 0,
        ScrollLockFree,
        ScrollLockHoriz,
        ScrollLockVert
    };

    ScrollLock m_scrollLock;
    int m_recordedMouseX;
    int m_recordedMouseY;
};


#endif /* KINETICSCROLLER_H */
