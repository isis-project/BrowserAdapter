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
#include "InteractiveInfo.h"
#include "Rectangle.h"
#include "BrowserAdapter.h"


static const char* sPropertyIdNames[] = {
    "interactive",
    "x",
    "y"
};

static const int kNumProperties = G_N_ELEMENTS(sPropertyIdNames);
static NPIdentifier sPropertyIds[kNumProperties];

static bool sPropertyIdsInitialized = false;

enum PropertyId {
    PropertyInteractive = 0,
    PropertyX,
    PropertyY,
};

/**
 * Defines methods for InteractiveInfo object.
 */
NPClass InteractiveInfo::sInteractiveInfoClass = {
    NP_CLASS_STRUCT_VERSION_CTOR,
    InteractiveInfo::PrvObjAllocate,
    InteractiveInfo::PrvObjDeallocate,
    InteractiveInfo::PrvObjInvalidate,
    InteractiveInfo::PrvObjHasMethod,
    InteractiveInfo::PrvObjInvoke,
    InteractiveInfo::PrvObjInvokeDefault,
    InteractiveInfo::PrvObjHasProperty,
    InteractiveInfo::PrvObjGetProperty,
    InteractiveInfo::PrvObjSetProperty,
    InteractiveInfo::PrvObjRemoveProperty,
    InteractiveInfo::PrvObjEnumerate,
    InteractiveInfo::PrvObjConstruct
};

// InteractiveInfo object static hooks implementation
// that call actual instance methods 
NPObject* InteractiveInfo::PrvObjAllocate(NPP npp, NPClass* klass)
{
	TRACE("Entered %s", __FUNCTION__);
    return new InteractiveInfo(static_cast<BrowserAdapter*>(npp->pdata));
}
void InteractiveInfo::PrvObjDeallocate(NPObject* obj)
{
	TRACE("Entered %s", __FUNCTION__);
    delete static_cast<InteractiveInfo*>(obj); 
}
void InteractiveInfo::PrvObjInvalidate(NPObject* obj)
{
	TRACE("Entered %s", __FUNCTION__);
    static_cast<InteractiveInfo*>(obj)->invalidate();
}

bool InteractiveInfo::PrvObjHasMethod(NPObject* obj, NPIdentifier name)
{
	TRACE("Entered %s", __FUNCTION__);
    return static_cast<InteractiveInfo*>(obj)->hasMethod(name);
}
bool InteractiveInfo::PrvObjInvoke(NPObject *obj, NPIdentifier name,
                         const NPVariant *args, uint32_t argCount,
                         NPVariant *result)
{
	TRACE("Entered %s", __FUNCTION__);
    return static_cast<InteractiveInfo*>(obj)->invoke(name, args, argCount, result);
}

bool InteractiveInfo::PrvObjInvokeDefault(NPObject *obj, const NPVariant *args, 
                                   uint32_t argCount, NPVariant *result)
{
	TRACE("Entered %s", __FUNCTION__);
    return static_cast<InteractiveInfo*>(obj)->invokeDefault(args, argCount, result);
}
bool InteractiveInfo::PrvObjHasProperty(NPObject *obj, NPIdentifier name)
{
	TRACE("Entered %s", __FUNCTION__);
    return static_cast<InteractiveInfo*>(obj)->hasProperty(name);
}
bool InteractiveInfo::PrvObjGetProperty(NPObject *obj, NPIdentifier name, NPVariant *result)
{
	TRACE("Entered %s", __FUNCTION__);
    return static_cast<InteractiveInfo*>(obj)->getProperty(name, result);
}
bool InteractiveInfo::PrvObjSetProperty(NPObject *obj, NPIdentifier name,
                              const NPVariant *value)
{
	TRACE("Entered %s", __FUNCTION__);
    return static_cast<InteractiveInfo*>(obj)->setProperty(name, value);
}
bool InteractiveInfo::PrvObjRemoveProperty(NPObject *obj, NPIdentifier name)
{
	TRACE("Entered %s", __FUNCTION__);
    return static_cast<InteractiveInfo*>(obj)->removeProperty(name);
}
bool InteractiveInfo::PrvObjEnumerate(NPObject *obj, NPIdentifier **value,
                            uint32_t *count)
{
	TRACE("Entered %s", __FUNCTION__);
    return static_cast<InteractiveInfo*>(obj)->enumerate(value, count);
}
bool InteractiveInfo::PrvObjConstruct(NPObject *obj, const NPVariant *args,
                            uint32_t argCount, NPVariant *result)
{
	TRACE("Entered %s", __FUNCTION__);
    return static_cast<InteractiveInfo*>(obj)->construct(args, argCount, result);
}

/**
 * Constructor.
 */
InteractiveInfo::InteractiveInfo(BrowserAdapter* adapter) :
	 m_interactive(false), m_x(0), m_y(0)
{
	TRACE("Entered %s", __FUNCTION__);
    if (!sPropertyIdsInitialized) {
        AdapterBase::NPN_GetStringIdentifiers(sPropertyIdNames, kNumProperties, sPropertyIds);
        sPropertyIdsInitialized = true;
        TRACE("InteractiveInfo sPropertyIdsInitialized == true");
    }
}

InteractiveInfo::~InteractiveInfo()
{
	TRACE("Entered %s", __FUNCTION__);
}

void InteractiveInfo::initialize(bool interactive, int32_t x, int32_t y)
{
	m_interactive = interactive;
	m_x = x;
	m_y = y;
}

void InteractiveInfo::invalidate()
{
	TRACE("Entered %s", __FUNCTION__);
}

bool InteractiveInfo::hasMethod(NPIdentifier name)
{
	TRACE("Entered %s", __FUNCTION__);
    return false;
}

bool InteractiveInfo::invoke(NPIdentifier name, const NPVariant *args,
                      uint32_t argCount, NPVariant *rm_latitudeesult)
{
	TRACE("Entered %s", __FUNCTION__);
    return false;
}

bool InteractiveInfo::invokeDefault(const NPVariant *args,
                             uint32_t argCount, NPVariant *result)
{
	TRACE("Entered %s", __FUNCTION__);
    return false;
}

bool InteractiveInfo::hasProperty(NPIdentifier name)
{
	TRACE("Entered %s", __FUNCTION__);
    for (int i = 0; i < kNumProperties; i++) {
        if (sPropertyIds[i] == name) {
            return true;
        }
    }
    return false;
}

bool InteractiveInfo::getProperty(NPIdentifier name, NPVariant *result)
{
	TRACE("Entered %s", __FUNCTION__);
    if (NULL == result){
        return false;
    }
     
    if (name == sPropertyIds[PropertyInteractive]) {
        BOOLEAN_TO_NPVARIANT(m_interactive, *result);
        return true;
    }
	else if (name == sPropertyIds[PropertyX]) {
        DOUBLE_TO_NPVARIANT(m_x, *result);
        return true;
    }
	else if (name == sPropertyIds[PropertyY]) {
        DOUBLE_TO_NPVARIANT(m_y, *result);
        return true;
    }
    
    return false;
}

bool InteractiveInfo::setProperty(NPIdentifier name, const NPVariant *value)
{
	TRACE("Entered %s", __FUNCTION__);
	if (name == sPropertyIds[PropertyInteractive]) {
		if (NPVARIANT_IS_BOOLEAN(*value)) {
			m_interactive = NPVARIANT_TO_BOOLEAN(*value);
        	return true;
		}
    }
	else if (name == sPropertyIds[PropertyX]) {
		if (NPVARIANT_IS_DOUBLE(*value)) {
			m_x = NPVARIANT_TO_DOUBLE(*value);
        	return true;
		}
    }
	else if (name == sPropertyIds[PropertyY]) {
		if (NPVARIANT_IS_DOUBLE(*value)) {
			m_y = NPVARIANT_TO_DOUBLE(*value);
        	return true;
		}
    }
    return false;
}

bool InteractiveInfo::removeProperty(NPIdentifier name)
{
	TRACE("Entered %s", __FUNCTION__);
    return false;
}

bool InteractiveInfo::enumerate(NPIdentifier **value, uint32_t *count)
{
	TRACE("Entered %s", __FUNCTION__);
    return false;
}

bool InteractiveInfo::construct(const NPVariant *args, int32_t argCount,
                         NPVariant *result)
{
	TRACE("Entered %s", __FUNCTION__);
    return false;
}

