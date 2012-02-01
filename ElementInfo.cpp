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
#include "ElementInfo.h"
#include "Rectangle.h"
#include "BrowserAdapter.h"


static const char* sPropertyIdNames[] = {
    "success",
    "element",
    "id",
    "name",
    "cname",
	"type",
    "bounds",
	"x",
	"y",
	"isEditable",
};

static const int kNumProperties = G_N_ELEMENTS(sPropertyIdNames);
static NPIdentifier sPropertyIds[kNumProperties];

static bool sPropertyIdsInitialized = false;

enum PropertyId {
    PropertySuccess = 0,
    PropertyElement,
    PropertyId,
    PropertyName,
    PropertyClassName,
    PropertyType,
    PropertyBounds,
    PropertyX,
    PropertyY,
    PropertyIsEditable,
};

/**
 * Defines methods for ElementInfo object.
 */
NPClass ElementInfo::sElementInfoClass = {
    NP_CLASS_STRUCT_VERSION_CTOR,
    ElementInfo::PrvObjAllocate,
    ElementInfo::PrvObjDeallocate,
    ElementInfo::PrvObjInvalidate,
    ElementInfo::PrvObjHasMethod,
    ElementInfo::PrvObjInvoke,
    ElementInfo::PrvObjInvokeDefault,
    ElementInfo::PrvObjHasProperty,
    ElementInfo::PrvObjGetProperty,
    ElementInfo::PrvObjSetProperty,
    ElementInfo::PrvObjRemoveProperty,
    ElementInfo::PrvObjEnumerate,
    ElementInfo::PrvObjConstruct
};

// ElementInfo object static hooks implementation
// that call actual instance methods 
NPObject* ElementInfo::PrvObjAllocate(NPP npp, NPClass* klass)
{
	TRACE("Entered %s", __FUNCTION__);
    return new ElementInfo(static_cast<BrowserAdapter*>(npp->pdata));
}
void ElementInfo::PrvObjDeallocate(NPObject* obj)
{
	TRACE("Entered %s", __FUNCTION__);
    delete static_cast<ElementInfo*>(obj); 
}
void ElementInfo::PrvObjInvalidate(NPObject* obj)
{
	TRACE("Entered %s", __FUNCTION__);
    static_cast<ElementInfo*>(obj)->invalidate();
}

bool ElementInfo::PrvObjHasMethod(NPObject* obj, NPIdentifier name)
{
	TRACE("Entered %s", __FUNCTION__);
    return static_cast<ElementInfo*>(obj)->hasMethod(name);
}
bool ElementInfo::PrvObjInvoke(NPObject *obj, NPIdentifier name,
                         const NPVariant *args, uint32_t argCount,
                         NPVariant *result)
{
	TRACE("Entered %s", __FUNCTION__);
    return static_cast<ElementInfo*>(obj)->invoke(name, args, argCount, result);
}

bool ElementInfo::PrvObjInvokeDefault(NPObject *obj, const NPVariant *args, 
                                   uint32_t argCount, NPVariant *result)
{
	TRACE("Entered %s", __FUNCTION__);
    return static_cast<ElementInfo*>(obj)->invokeDefault(args, argCount, result);
}
bool ElementInfo::PrvObjHasProperty(NPObject *obj, NPIdentifier name)
{
	TRACE("Entered %s", __FUNCTION__);
    return static_cast<ElementInfo*>(obj)->hasProperty(name);
}
bool ElementInfo::PrvObjGetProperty(NPObject *obj, NPIdentifier name, NPVariant *result)
{
	TRACE("Entered %s", __FUNCTION__);
    return static_cast<ElementInfo*>(obj)->getProperty(name, result);
}
bool ElementInfo::PrvObjSetProperty(NPObject *obj, NPIdentifier name,
                              const NPVariant *value)
{
	TRACE("Entered %s", __FUNCTION__);
    return static_cast<ElementInfo*>(obj)->setProperty(name, value);
}
bool ElementInfo::PrvObjRemoveProperty(NPObject *obj, NPIdentifier name)
{
	TRACE("Entered %s", __FUNCTION__);
    return static_cast<ElementInfo*>(obj)->removeProperty(name);
}
bool ElementInfo::PrvObjEnumerate(NPObject *obj, NPIdentifier **value,
                            uint32_t *count)
{
	TRACE("Entered %s", __FUNCTION__);
    return static_cast<ElementInfo*>(obj)->enumerate(value, count);
}
bool ElementInfo::PrvObjConstruct(NPObject *obj, const NPVariant *args,
                            uint32_t argCount, NPVariant *result)
{
	TRACE("Entered %s", __FUNCTION__);
    return static_cast<ElementInfo*>(obj)->construct(args, argCount, result);
}

/**
 * Constructor.
 */
ElementInfo::ElementInfo(BrowserAdapter* adapter) :
	 m_success(false)
	,m_bounds( adapter->NPN_CreateObject(&Rectangle::sRectangleClass) )
	,m_tapX(0)
	,m_tapY(0)
	,m_isEditable(false)
{
	TRACE("Entered %s", __FUNCTION__);
    if (!sPropertyIdsInitialized) {
        AdapterBase::NPN_GetStringIdentifiers(sPropertyIdNames, kNumProperties, sPropertyIds);
        sPropertyIdsInitialized = true;
        TRACE("ElementInfo sPropertyIdsInitialized == true");
    }
}

ElementInfo::~ElementInfo()
{
	TRACE("Entered %s", __FUNCTION__);
	AdapterBase::NPN_ReleaseObject( m_bounds );
}

void ElementInfo::initialize(bool success, const char* element, const char* id, const char* name, const char* cname,
			const char* type, int left, int top, int right, int bottom, int tapX, int tapY, bool isEditable)
{
	m_success = success;
	if (element != NULL)
		m_element = element;
	if (id != NULL)
		m_id = id;
	if (name != NULL)
		m_name = name;
	if (cname != NULL)
		m_cname = cname;
	if (type != NULL)
		m_type = type;
	static_cast<Rectangle*>(m_bounds)->initialize(left, top, right, bottom);
	m_tapX = tapX;
	m_tapY = tapY;
	m_isEditable = isEditable;
}

void ElementInfo::invalidate()
{
	TRACE("Entered %s", __FUNCTION__);
}

bool ElementInfo::hasMethod(NPIdentifier name)
{
	TRACE("Entered %s", __FUNCTION__);
    return false;
}

bool ElementInfo::invoke(NPIdentifier name, const NPVariant *args,
                      uint32_t argCount, NPVariant *rm_latitudeesult)
{
	TRACE("Entered %s", __FUNCTION__);
    return false;
}

bool ElementInfo::invokeDefault(const NPVariant *args,
                             uint32_t argCount, NPVariant *result)
{
	TRACE("Entered %s", __FUNCTION__);
    return false;
}

bool ElementInfo::hasProperty(NPIdentifier name)
{
	TRACE("Entered %s", __FUNCTION__);
    for (int i = 0; i < kNumProperties; i++) {
        if (sPropertyIds[i] == name) {
            return true;
        }
    }
    return false;
}

bool ElementInfo::getProperty(NPIdentifier name, NPVariant *result)
{
	TRACE("Entered %s", __FUNCTION__);
    if (NULL == result){
        return false;
    }
     
    if (name == sPropertyIds[PropertySuccess]) {
        BOOLEAN_TO_NPVARIANT(m_success, *result);
        return true;
    }
    else if (name == sPropertyIds[PropertyElement]) {
		if (m_element.empty())
       	 	NULL_TO_NPVARIANT(*result);
		else
       	 	STRINGZ_TO_NPVARIANT(strdup(m_element.c_str()), *result);
        return true;
    }
    else if (name == sPropertyIds[PropertyId]) {
		if (m_id.empty())
       	 	NULL_TO_NPVARIANT(*result);
		else
        	STRINGZ_TO_NPVARIANT(strdup(m_id.c_str()), *result);
        return true;
    }
	else if (name == sPropertyIds[PropertyName]) {
		if (m_name.empty())
       	 	NULL_TO_NPVARIANT(*result);
		else
        	STRINGZ_TO_NPVARIANT(strdup(m_name.c_str()), *result);
        return true;
    }
	else if (name == sPropertyIds[PropertyClassName]) {
		if (m_cname.empty())
       	 	NULL_TO_NPVARIANT(*result);
		else
        	STRINGZ_TO_NPVARIANT(strdup(m_cname.c_str()), *result);
        return true;
	}
	else if (name == sPropertyIds[PropertyType]) {
		if (m_type.empty())
       	 	NULL_TO_NPVARIANT(*result);
		else
        	STRINGZ_TO_NPVARIANT(strdup(m_type.c_str()), *result);
        return true;
    }
	else if (name == sPropertyIds[PropertyBounds]) {
		OBJECT_TO_NPVARIANT(m_bounds, *result);
		AdapterBase::NPN_RetainObject(m_bounds);
        return true;
    }
	else if (name == sPropertyIds[PropertyX]) {
		double val(m_tapX);
		DOUBLE_TO_NPVARIANT(val, *result);
        return true;
    }
	else if (name == sPropertyIds[PropertyY]) {
		double val(m_tapY);
		DOUBLE_TO_NPVARIANT(val, *result);
        return true;
    }
	else if (name == sPropertyIds[PropertyIsEditable]) {
		BOOLEAN_TO_NPVARIANT(m_isEditable, *result);
        return true;
    }
    
    return false;
}

bool ElementInfo::setProperty(NPIdentifier name, const NPVariant *value)
{
	TRACE("Entered %s", __FUNCTION__);
	if (name == sPropertyIds[PropertySuccess]) {
		if (NPVARIANT_IS_BOOLEAN(*value)) {
			m_success = NPVARIANT_TO_DOUBLE(*value);
        	return true;
		}
    }
    else if (name == sPropertyIds[PropertyElement]) {
        if (NPVARIANT_IS_STRING(*value)) {
			char* szval = AdapterBase::NPStringToString(NPVARIANT_TO_STRING(*value));
			m_element = szval;
			::free(szval);
        	return true;
		}
    }
    else if (name == sPropertyIds[PropertyId]) {
        if (NPVARIANT_IS_STRING(*value)) {
			char* szval = AdapterBase::NPStringToString(NPVARIANT_TO_STRING(*value));
			m_id = szval;
			::free(szval);
        	return true;
		}
    }
	else if (name == sPropertyIds[PropertyName]) {
        if (NPVARIANT_IS_STRING(*value)) {
			char* szval = AdapterBase::NPStringToString(NPVARIANT_TO_STRING(*value));
			m_name = szval;
			::free(szval);
        	return true;
		}
    }
	else if (name == sPropertyIds[PropertyClassName]) {
        if (NPVARIANT_IS_STRING(*value)) {
			char* szval = AdapterBase::NPStringToString(NPVARIANT_TO_STRING(*value));
			m_cname = szval;
			::free(szval);
        	return true;
		}
    }
	else if (name == sPropertyIds[PropertyType]) {
        if (NPVARIANT_IS_STRING(*value)) {
			char* szval = AdapterBase::NPStringToString(NPVARIANT_TO_STRING(*value));
			m_type = szval;
			::free(szval);
        	return true;
		}
    }
	else if (name == sPropertyIds[PropertyX]) {
		if (NPVARIANT_IS_DOUBLE(*value)) {
			m_tapX = NPVARIANT_TO_DOUBLE(*value);
        	return true;
		}
    }
	else if (name == sPropertyIds[PropertyY]) {
		if (NPVARIANT_IS_DOUBLE(*value)) {
			m_tapY = NPVARIANT_TO_DOUBLE(*value);
        	return true;
		}
    }
	else if (name == sPropertyIds[PropertyIsEditable]) {
		if (NPVARIANT_IS_BOOLEAN(*value)) {
			m_isEditable = NPVARIANT_TO_BOOLEAN(*value);
        	return true;
		}
    }
    return false;
}

bool ElementInfo::removeProperty(NPIdentifier name)
{
	TRACE("Entered %s", __FUNCTION__);
    return false;
}

bool ElementInfo::enumerate(NPIdentifier **value, uint32_t *count)
{
	TRACE("Entered %s", __FUNCTION__);
    return false;
}

bool ElementInfo::construct(const NPVariant *args, int32_t argCount,
                         NPVariant *result)
{
	TRACE("Entered %s", __FUNCTION__);
    return false;
}
