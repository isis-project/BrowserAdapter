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

#ifndef BROWSERSCROLLABLELAYER_H
#define BROWSERSCROLLABLELAYER_H

#include <stdint.h>
#include <list>

#include "BrowserRect.h"

struct BrowserScrollableLayer
{
    // id of the layer
    uintptr_t id;

    // absolute positioning of the rectangle in page coordinates
    BrowserRect bounds;

    // Content dimensions and scroll position inside the layer
    int contentX;
    int contentY;
    int contentWidth;
    int contentHeight;
};

typedef std::map<uintptr_t, BrowserScrollableLayer> BrowserScrollableLayerMap;

struct BrowserScrollableLayerScrollSession
{
    bool isActive;
    uintptr_t layer;
    int contentXAtMouseDown;
    int contentYAtMouseDown;
    int mouseDownX;
    int mouseDownY;
};

#endif /* BROWSERSCROLLABLELAYER_H */
