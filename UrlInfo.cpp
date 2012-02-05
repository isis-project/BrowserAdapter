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

#include <stdlib.h>
#include <stdio.h>
#include <glib.h>
#include <npupp.h>
#include <npruntime.h>
#include <AdapterBase.h>
#include "UrlInfo.h"
#include "Rectangle.h"
#include "BrowserAdapter.h"


static const char* sPropertyIdNames[] = {
    "success",
    "url",
    "desc",
    "bounds"
};

static const int kNumProperties = G_N_ELEMENTS(sPropertyIdNames);
static NPIdentifier sPropertyIds[kNumProperties];

static bool sPropertyIdsInitialized = false;

enum PropertyId {
    PropertySuccess = 0,
    PropertyUrl,
    PropertyDescription,
    PropertyBounds
};

/**
 * Defines methods for UrlInfo object.
 */
NPClass UrlInfo::sUrlInfoClass = {
    NP_CLASS_STRUCT_VERSION_CTOR,
    UrlInfo::PrvObjAllocate,
    UrlInfo::PrvObjDeallocate,
    UrlInfo::PrvObjInvalidate,
    UrlInfo::PrvObjHasMethod,
    UrlInfo::PrvObjInvoke,
    UrlInfo::PrvObjInvokeDefault,
    UrlInfo::PrvObjHasProperty,
    UrlInfo::PrvObjGetProperty,
    UrlInfo::PrvObjSetProperty,
    UrlInfo::PrvObjRemoveProperty,
    UrlInfo::PrvObjEnumerate,
    UrlInfo::PrvObjConstruct
};

// UrlInfo object static hooks implementation
// that call actual instance methods
NPObject* UrlInfo::PrvObjAllocate(NPP npp, NPClass* klass)
{
    TRACE("Entered %s", __FUNCTION__);
    return new UrlInfo(static_cast<BrowserAdapter*>(npp->pdata));
}
void UrlInfo::PrvObjDeallocate(NPObject* obj)
{
    TRACE("Entered %s", __FUNCTION__);
    delete static_cast<UrlInfo*>(obj);
}
void UrlInfo::PrvObjInvalidate(NPObject* obj)
{
    TRACE("Entered %s", __FUNCTION__);
    static_cast<UrlInfo*>(obj)->invalidate();
}

bool UrlInfo::PrvObjHasMethod(NPObject* obj, NPIdentifier name)
{
    TRACE("Entered %s", __FUNCTION__);
    return static_cast<UrlInfo*>(obj)->hasMethod(name);
}
bool UrlInfo::PrvObjInvoke(NPObject *obj, NPIdentifier name,
                           const NPVariant *args, uint32_t argCount,
                           NPVariant *result)
{
    TRACE("Entered %s", __FUNCTION__);
    return static_cast<UrlInfo*>(obj)->invoke(name, args, argCount, result);
}

bool UrlInfo::PrvObjInvokeDefault(NPObject *obj, const NPVariant *args,
                                  uint32_t argCount, NPVariant *result)
{
    TRACE("Entered %s", __FUNCTION__);
    return static_cast<UrlInfo*>(obj)->invokeDefault(args, argCount, result);
}
bool UrlInfo::PrvObjHasProperty(NPObject *obj, NPIdentifier name)
{
    TRACE("Entered %s", __FUNCTION__);
    return static_cast<UrlInfo*>(obj)->hasProperty(name);
}
bool UrlInfo::PrvObjGetProperty(NPObject *obj, NPIdentifier name, NPVariant *result)
{
    TRACE("Entered %s", __FUNCTION__);
    return static_cast<UrlInfo*>(obj)->getProperty(name, result);
}
bool UrlInfo::PrvObjSetProperty(NPObject *obj, NPIdentifier name,
                                const NPVariant *value)
{
    TRACE("Entered %s", __FUNCTION__);
    return static_cast<UrlInfo*>(obj)->setProperty(name, value);
}
bool UrlInfo::PrvObjRemoveProperty(NPObject *obj, NPIdentifier name)
{
    TRACE("Entered %s", __FUNCTION__);
    return static_cast<UrlInfo*>(obj)->removeProperty(name);
}
bool UrlInfo::PrvObjEnumerate(NPObject *obj, NPIdentifier **value,
                              uint32_t *count)
{
    TRACE("Entered %s", __FUNCTION__);
    return static_cast<UrlInfo*>(obj)->enumerate(value, count);
}
bool UrlInfo::PrvObjConstruct(NPObject *obj, const NPVariant *args,
                              uint32_t argCount, NPVariant *result)
{
    TRACE("Entered %s", __FUNCTION__);
    return static_cast<UrlInfo*>(obj)->construct(args, argCount, result);
}

/**
 * Constructor.
 */
UrlInfo::UrlInfo(BrowserAdapter* adapter) :
    m_success(false)
    ,m_bounds( adapter->NPN_CreateObject(&Rectangle::sRectangleClass) )
{
    TRACE("Entered %s", __FUNCTION__);
    if (!sPropertyIdsInitialized) {
        AdapterBase::NPN_GetStringIdentifiers(sPropertyIdNames, kNumProperties, sPropertyIds);
        sPropertyIdsInitialized = true;
        TRACE("UrlInfo sPropertyIdsInitialized == true");
    }
}

UrlInfo::~UrlInfo()
{
    TRACE("Entered %s", __FUNCTION__);
    AdapterBase::NPN_ReleaseObject( m_bounds );
}

void UrlInfo::initialize(bool success, const char* url, const char* desc,
                         int left, int top, int right, int bottom)
{
    m_success = success;
    m_url = url;
    m_desc = desc;
    static_cast<Rectangle*>(m_bounds)->initialize(left, top, right, bottom);
}

void UrlInfo::invalidate()
{
    TRACE("Entered %s", __FUNCTION__);
}

bool UrlInfo::hasMethod(NPIdentifier name)
{
    TRACE("Entered %s", __FUNCTION__);
    return false;
}

bool UrlInfo::invoke(NPIdentifier name, const NPVariant *args,
                     uint32_t argCount, NPVariant *rm_latitudeesult)
{
    TRACE("Entered %s", __FUNCTION__);
    return false;
}

bool UrlInfo::invokeDefault(const NPVariant *args,
                            uint32_t argCount, NPVariant *result)
{
    TRACE("Entered %s", __FUNCTION__);
    return false;
}

bool UrlInfo::hasProperty(NPIdentifier name)
{
    TRACE("Entered %s", __FUNCTION__);
    for (int i = 0; i < kNumProperties; i++) {
        if (sPropertyIds[i] == name) {
            return true;
        }
    }
    return false;
}

bool UrlInfo::getProperty(NPIdentifier name, NPVariant *result)
{
    TRACE("Entered %s", __FUNCTION__);
    if (NULL == result) {
        return false;
    }

    if (name == sPropertyIds[PropertySuccess]) {
        BOOLEAN_TO_NPVARIANT(m_success, *result);
        return true;
    }
    else if (name == sPropertyIds[PropertyUrl]) {
        if (m_url.empty())
            NULL_TO_NPVARIANT(*result);
        else
            STRINGZ_TO_NPVARIANT(strdup(m_url.c_str()), *result);
        return true;
    }
    else if (name == sPropertyIds[PropertyDescription]) {
        if (m_desc.empty())
            NULL_TO_NPVARIANT(*result);
        else
            STRINGZ_TO_NPVARIANT(strdup(m_desc.c_str()), *result);
        return true;
    }
    else if (name == sPropertyIds[PropertyBounds]) {
        OBJECT_TO_NPVARIANT(m_bounds, *result);
        AdapterBase::NPN_RetainObject(m_bounds);
        return true;
    }

    return false;
}

bool UrlInfo::setProperty(NPIdentifier name, const NPVariant *value)
{
    TRACE("Entered %s", __FUNCTION__);
    if (name == sPropertyIds[PropertySuccess]) {
        if (NPVARIANT_IS_BOOLEAN(*value)) {
            m_success = NPVARIANT_TO_DOUBLE(*value);
            return true;
        }
    }
    else if (name == sPropertyIds[PropertyUrl]) {
        if (NPVARIANT_IS_STRING(*value)) {
            char* szval = AdapterBase::NPStringToString(NPVARIANT_TO_STRING(*value));
            m_url = szval;
            ::free(szval);
            return true;
        }
    }
    else if (name == sPropertyIds[PropertyDescription]) {
        if (NPVARIANT_IS_STRING(*value)) {
            char* szval = AdapterBase::NPStringToString(NPVARIANT_TO_STRING(*value));
            m_desc = szval;
            ::free(szval);
            return true;
        }
    }
    return false;
}

bool UrlInfo::removeProperty(NPIdentifier name)
{
    TRACE("Entered %s", __FUNCTION__);
    return false;
}

bool UrlInfo::enumerate(NPIdentifier **value, uint32_t *count)
{
    TRACE("Entered %s", __FUNCTION__);
    return false;
}

bool UrlInfo::construct(const NPVariant *args, int32_t argCount,
                        NPVariant *result)
{
    TRACE("Entered %s", __FUNCTION__);
    return false;
}

