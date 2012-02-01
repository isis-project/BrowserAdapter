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
#include <AdapterBase.h>
#include "Rectangle.h"


static const char* sPropertyIdNames[] = {
    "left",
    "top",
    "right",
    "bottom",
    "width",
    "height"
};

static const int kNumProperties = G_N_ELEMENTS(sPropertyIdNames);
static NPIdentifier sPropertyIds[kNumProperties];

static bool sPropertyIdsInitialized = false;

enum PropertyId {
    PropertyLeft = 0,
    PropertyTop,
    PropertyRight,
    PropertyBottom,
	PropertyWidth,
	PropertyHeight
};

/// structure that defines methods for Rectangle object
NPClass Rectangle::sRectangleClass = {
    NP_CLASS_STRUCT_VERSION_CTOR,
    Rectangle::PrvObjAllocate,
    Rectangle::PrvObjDeallocate,
    Rectangle::PrvObjInvalidate,
    Rectangle::PrvObjHasMethod,
    Rectangle::PrvObjInvoke,
    Rectangle::PrvObjInvokeDefault,
    Rectangle::PrvObjHasProperty,
    Rectangle::PrvObjGetProperty,
    Rectangle::PrvObjSetProperty,
    Rectangle::PrvObjRemoveProperty,
    Rectangle::PrvObjEnumerate,
    Rectangle::PrvObjConstruct
};

// Rectangle object static hooks implementation
// that call actual instance methods 
NPObject* Rectangle::PrvObjAllocate(NPP npp, NPClass* klass)
{
	TRACE("Entered %s", __FUNCTION__);
    return new Rectangle(static_cast<AdapterBase*>(npp->pdata));
}
void Rectangle::PrvObjDeallocate(NPObject* obj)
{
	TRACE("Entered %s", __FUNCTION__);
    delete static_cast<Rectangle*>(obj); 
}
void Rectangle::PrvObjInvalidate(NPObject* obj)
{
	TRACE("Entered %s", __FUNCTION__);
    static_cast<Rectangle*>(obj)->invalidate();
}

bool Rectangle::PrvObjHasMethod(NPObject* obj, NPIdentifier name)
{
	TRACE("Entered %s", __FUNCTION__);
    return static_cast<Rectangle*>(obj)->hasMethod(name);
}
bool Rectangle::PrvObjInvoke(NPObject *obj, NPIdentifier name,
                         const NPVariant *args, uint32_t argCount,
                         NPVariant *result)
{
	TRACE("Entered %s", __FUNCTION__);
    return static_cast<Rectangle*>(obj)->invoke(name, args, argCount, result);
}

bool Rectangle::PrvObjInvokeDefault(NPObject *obj, const NPVariant *args, 
                                   uint32_t argCount, NPVariant *result)
{
	TRACE("Entered %s", __FUNCTION__);
    return static_cast<Rectangle*>(obj)->invokeDefault(args, argCount, result);
}
bool Rectangle::PrvObjHasProperty(NPObject *obj, NPIdentifier name)
{
	TRACE("Entered %s", __FUNCTION__);
    return static_cast<Rectangle*>(obj)->hasProperty(name);
}
bool Rectangle::PrvObjGetProperty(NPObject *obj, NPIdentifier name, NPVariant *result)
{
	TRACE("Entered %s", __FUNCTION__);
    return static_cast<Rectangle*>(obj)->getProperty(name, result);
}
bool Rectangle::PrvObjSetProperty(NPObject *obj, NPIdentifier name,
                              const NPVariant *value)
{
	TRACE("Entered %s", __FUNCTION__);
    return static_cast<Rectangle*>(obj)->setProperty(name, value);
}
bool Rectangle::PrvObjRemoveProperty(NPObject *obj, NPIdentifier name)
{
	TRACE("Entered %s", __FUNCTION__);
    return static_cast<Rectangle*>(obj)->removeProperty(name);
}
bool Rectangle::PrvObjEnumerate(NPObject *obj, NPIdentifier **value,
                            uint32_t *count)
{
	TRACE("Entered %s", __FUNCTION__);
    return static_cast<Rectangle*>(obj)->enumerate(value, count);
}
bool Rectangle::PrvObjConstruct(NPObject *obj, const NPVariant *args,
                            uint32_t argCount, NPVariant *result)
{
	TRACE("Entered %s", __FUNCTION__);
    return static_cast<Rectangle*>(obj)->construct(args, argCount, result);
}

/**
 * Constructor.
 */
Rectangle::Rectangle(AdapterBase* adapter) :
	 m_left(0)
	,m_top(0)
	,m_right(0)
	,m_bottom(0)
{
	TRACE("Entered %s", __FUNCTION__);
    if (!sPropertyIdsInitialized) {
		AdapterBase::NPN_GetStringIdentifiers(sPropertyIdNames, kNumProperties, sPropertyIds);
        sPropertyIdsInitialized = true;
        TRACE("Rectangle sPropertyIdsInitialized == true");
    }
}

Rectangle::~Rectangle()
{
	TRACE("Entered %s", __FUNCTION__);
}

void Rectangle::initialize(int left, int top, int right, int bottom)
{
	m_left = left;
	m_top = top;
	m_right = right;
	m_bottom = bottom;
}

void Rectangle::set(const Rectangle* r)
{
	if (NULL != r) {
		m_left = r->m_left;
		m_top = r->m_top;
		m_right = r->m_right;
		m_bottom = r->m_bottom;
	}
}

void Rectangle::invalidate()
{
	TRACE("Entered %s", __FUNCTION__);
}

bool Rectangle::hasMethod(NPIdentifier name)
{
	TRACE("Entered %s", __FUNCTION__);
    return false;
}

bool Rectangle::invoke(NPIdentifier name, const NPVariant *args,
                      uint32_t argCount, NPVariant *rm_latitudeesult)
{
	TRACE("Entered %s", __FUNCTION__);
    return false;
}

bool Rectangle::invokeDefault(const NPVariant *args,
                             uint32_t argCount, NPVariant *result)
{
	TRACE("Entered %s", __FUNCTION__);
    return false;
}

bool Rectangle::hasProperty(NPIdentifier name)
{
	TRACE("Entered %s", __FUNCTION__);
    for (int i = 0; i < kNumProperties; i++) {
        if (sPropertyIds[i] == name) {
            return true;
        }
    }
    return false;
}
    
bool Rectangle::getProperty(NPIdentifier name, NPVariant *result)
{
	TRACE("Entered %s", __FUNCTION__);
    if (NULL == result){
        return false;
    }
     
    if (name == sPropertyIds[PropertyLeft]){
        INT32_TO_NPVARIANT(m_left, *result);
        return true;
    }
    else if (name == sPropertyIds[PropertyTop]){
        INT32_TO_NPVARIANT(m_top, *result);
        return true;
    }
    else if (name == sPropertyIds[PropertyRight]){
        INT32_TO_NPVARIANT(m_right, *result);
        return true;
    }
    else if (name == sPropertyIds[PropertyBottom]){
        INT32_TO_NPVARIANT(m_bottom, *result);
        return true;
	}
    else if (name == sPropertyIds[PropertyWidth]){
        INT32_TO_NPVARIANT(m_right - m_left, *result);
        return true;
	}
    else if (name == sPropertyIds[PropertyHeight]){
        INT32_TO_NPVARIANT(m_bottom - m_top, *result);
        return true;
	}
    
    return false;
}

bool Rectangle::setProperty(NPIdentifier name, const NPVariant *value)
{
	TRACE("Entered %s", __FUNCTION__);
	if (name == sPropertyIds[PropertyLeft] && AdapterBase::IsIntegerVariant(value)) {
		m_left = AdapterBase::VariantToInteger(value);
       	return true;
    }
    else if (name == sPropertyIds[PropertyTop] && AdapterBase::IsIntegerVariant(value)) {
		m_top = AdapterBase::VariantToInteger(value);
		return true;
    }
    else if (name == sPropertyIds[PropertyRight] && AdapterBase::IsIntegerVariant(value)) {
		m_right = AdapterBase::VariantToInteger(value);
		return true;
    }
    else if (name == sPropertyIds[PropertyBottom] && AdapterBase::IsIntegerVariant(value)) {
		m_bottom = AdapterBase::VariantToInteger(value);
		return true;
    }

    return false;
}

bool Rectangle::removeProperty(NPIdentifier name)
{
	TRACE("Entered %s", __FUNCTION__);
    return false;
}

bool Rectangle::enumerate(NPIdentifier **value, uint32_t *count)
{
	TRACE("Entered %s", __FUNCTION__);
    return false;
}

bool Rectangle::construct(const NPVariant *args, int32_t argCount,
                         NPVariant *result)
{
	TRACE("Entered %s", __FUNCTION__);
    return false;
}


