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

#include "NPObjectEvent.h"
#include "BrowserAdapter.h"
#include "Debug.h"

static const char *sPropertyIdNames[] = {
    "type",
    "pageX",
    "pageY",
    "altKey",
    "shiftKey",
    "ctrlKey",
    "metaKey"
};

static const int kNumProperties = G_N_ELEMENTS(sPropertyIdNames);
static NPIdentifier sPropertyIds[kNumProperties];

static bool sPropertyIdsInitialized = false;

enum PropertyId {
    PropertyType = 0,
    PropertyPageX,
    PropertyPageY,
    PropertyAltKey,
    PropertyShiftKey,
    PropertyCtrlKey,
    PropertyMetaKey
};

/**
 * Defines methods for NPObjectEvent object.
 */
NPClass NPObjectEvent::sNPObjectEventClass = {
    NP_CLASS_STRUCT_VERSION_CTOR,
    NPObjectEvent::PrvObjAllocate,
    NPObjectEvent::PrvObjDeallocate,
    NPObjectEvent::PrvObjInvalidate,
    NPObjectEvent::PrvObjHasMethod,
    NPObjectEvent::PrvObjInvoke,
    NPObjectEvent::PrvObjInvokeDefault,
    NPObjectEvent::PrvObjHasProperty,
    NPObjectEvent::PrvObjGetProperty,
    NPObjectEvent::PrvObjSetProperty,
    NPObjectEvent::PrvObjRemoveProperty,
    NPObjectEvent::PrvObjEnumerate,
    NPObjectEvent::PrvObjConstruct
};

// NPObjectEvent object static hooks implementation
// that call actual instance methods
NPObject* NPObjectEvent::PrvObjAllocate(NPP npp, NPClass* klass)
{
    //TRACEF("Entered %s", __FUNCTION__);
    return new NPObjectEvent(static_cast<BrowserAdapter*>(npp->pdata));
}
void NPObjectEvent::PrvObjDeallocate(NPObject* obj)
{
    //TRACEF("Entered %s", __FUNCTION__);
    delete static_cast<NPObjectEvent*>(obj);
}

void NPObjectEvent::PrvObjInvalidate(NPObject* obj)
{
    /* do nothing */
}

bool NPObjectEvent::PrvObjHasMethod(NPObject* obj, NPIdentifier name)
{
    /* unsupported */
    return false;
}
bool NPObjectEvent::PrvObjInvoke(NPObject *obj, NPIdentifier name,
                                 const NPVariant *args, uint32_t argCount,
                                 NPVariant *result)
{
    /* unsupported */
    return false;
}

bool NPObjectEvent::PrvObjInvokeDefault(NPObject *obj, const NPVariant *args,
                                        uint32_t argCount, NPVariant *result)
{
    /* unsupported */
    return false;
}

bool NPObjectEvent::PrvObjHasProperty(NPObject *obj, NPIdentifier name)
{
    //TRACEF("Entered %s", __FUNCTION__);
    return static_cast<NPObjectEvent*>(obj)->hasProperty(name);
}

bool NPObjectEvent::PrvObjGetProperty(NPObject *obj, NPIdentifier name, NPVariant *result)
{
    //TRACEF("Entered %s", __FUNCTION__);
    return static_cast<NPObjectEvent*>(obj)->getProperty(name, result);
}

bool NPObjectEvent::PrvObjSetProperty(NPObject *obj, NPIdentifier name,
                                      const NPVariant *value)
{
    /* unsupported */
    return false;
}

bool NPObjectEvent::PrvObjRemoveProperty(NPObject *obj, NPIdentifier name)
{
    /* unsupported */
    return false;
}

bool NPObjectEvent::PrvObjEnumerate(NPObject *obj, NPIdentifier **value,
                                    uint32_t *count)
{
    /* unsupported */
    return false;
}

bool NPObjectEvent::PrvObjConstruct(NPObject *obj, const NPVariant *args,
                                    uint32_t argCount, NPVariant *result)
{
    /* unsupported */
    return false;
}

NPObjectEvent::NPObjectEvent(BrowserAdapter *adapter)
    : m_pageX(0)
    , m_pageY(0)
    , m_modifiers(0)
{
    //TRACEF("Entered %s", __FUNCTION__);
    if (!sPropertyIdsInitialized) {
        AdapterBase::NPN_GetStringIdentifiers(sPropertyIdNames, kNumProperties,
                                              sPropertyIds);
        sPropertyIdsInitialized = true;
        TRACEF("NPObjectEvent sPropertyIdsInitialized == true");
    }
}

NPObjectEvent::~NPObjectEvent()
{
    //TRACEF("Entered %s", __FUNCTION__);
}

void NPObjectEvent::initialize(const char *type, int pageX, int pageY,
                               int modifiers)
{
    if (type != NULL) {
        m_type = type;
    }
    m_pageX = pageX;
    m_pageY = pageY;
    m_modifiers = modifiers;
}

bool NPObjectEvent::hasProperty(NPIdentifier name)
{
    //TRACEF("Entered %s", __FUNCTION__);
    for (int i = 0; i < kNumProperties; i++) {
        if (sPropertyIds[i] == name) {
            return true;
        }
    }
    return false;
}

bool NPObjectEvent::getProperty(NPIdentifier name, NPVariant *result)
{
    //TRACEF("Entered %s", __FUNCTION__);
    if (result == NULL) {
        return false;
    }

    if (sPropertyIds[PropertyType] == name) {
        if (m_type.empty()) {
            NULL_TO_NPVARIANT(*result);
        } else {
            STRINGZ_TO_NPVARIANT(strdup(m_type.c_str()), *result);
        }
        return true;

    } else if (sPropertyIds[PropertyPageX] == name) {
        double v(m_pageX);
        DOUBLE_TO_NPVARIANT(v, *result);
        return true;

    } else if (sPropertyIds[PropertyPageY] == name) {
        double v(m_pageY);
        DOUBLE_TO_NPVARIANT(v, *result);
        return true;
    } else if (sPropertyIds[PropertyAltKey] == name) {
        bool b(m_modifiers & npPalmAltKeyModifier);
        BOOLEAN_TO_NPVARIANT(b, *result);
        return true;
    } else if (sPropertyIds[PropertyShiftKey] == name) {
        bool b(m_modifiers & npPalmShiftKeyModifier);
        BOOLEAN_TO_NPVARIANT(b, *result);
        return true;
    } else if (sPropertyIds[PropertyCtrlKey] == name) {
        bool b(m_modifiers & npPalmCtrlKeyModifier);
        BOOLEAN_TO_NPVARIANT(b, *result);
        return true;
    } else if (sPropertyIds[PropertyMetaKey] == name) {
        bool b(m_modifiers & npPalmMetaKeyModifier);
        BOOLEAN_TO_NPVARIANT(b, *result);
        return true;
    }

    return false;
}
