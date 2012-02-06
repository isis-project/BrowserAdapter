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
#include "ImageInfo.h"
#include "Rectangle.h"
#include "BrowserAdapter.h"


static const char* sPropertyIdNames[] = {
    "success",
    "baseUri",
    "src",
    "title",
    "altText",
    "width",
    "height",
    "mimeType"
};

static const int kNumProperties = G_N_ELEMENTS(sPropertyIdNames);
static NPIdentifier sPropertyIds[kNumProperties];

static bool sPropertyIdsInitialized = false;

enum PropertyId {
    PropertySuccess = 0,
    PropertyBaseUri,
    PropertySrc,
    PropertyTitle,
    PropertyAltText,
    PropertyWidth,
    PropertyHeight,
    PropertyMimeType,
};

/**
 * Defines methods for ImageInfo object.
 */
NPClass ImageInfo::sImageInfoClass = {
    NP_CLASS_STRUCT_VERSION_CTOR,
    ImageInfo::PrvObjAllocate,
    ImageInfo::PrvObjDeallocate,
    ImageInfo::PrvObjInvalidate,
    ImageInfo::PrvObjHasMethod,
    ImageInfo::PrvObjInvoke,
    ImageInfo::PrvObjInvokeDefault,
    ImageInfo::PrvObjHasProperty,
    ImageInfo::PrvObjGetProperty,
    ImageInfo::PrvObjSetProperty,
    ImageInfo::PrvObjRemoveProperty,
    ImageInfo::PrvObjEnumerate,
    ImageInfo::PrvObjConstruct
};

// ImageInfo object static hooks implementation
// that call actual instance methods
NPObject* ImageInfo::PrvObjAllocate(NPP npp, NPClass* klass)
{
    TRACE("Entered %s", __FUNCTION__);
    return new ImageInfo(static_cast<BrowserAdapter*>(npp->pdata));
}
void ImageInfo::PrvObjDeallocate(NPObject* obj)
{
    TRACE("Entered %s", __FUNCTION__);
    delete static_cast<ImageInfo*>(obj);
}
void ImageInfo::PrvObjInvalidate(NPObject* obj)
{
    TRACE("Entered %s", __FUNCTION__);
    static_cast<ImageInfo*>(obj)->invalidate();
}

bool ImageInfo::PrvObjHasMethod(NPObject* obj, NPIdentifier name)
{
    TRACE("Entered %s", __FUNCTION__);
    return static_cast<ImageInfo*>(obj)->hasMethod(name);
}
bool ImageInfo::PrvObjInvoke(NPObject *obj, NPIdentifier name,
                             const NPVariant *args, uint32_t argCount,
                             NPVariant *result)
{
    TRACE("Entered %s", __FUNCTION__);
    return static_cast<ImageInfo*>(obj)->invoke(name, args, argCount, result);
}

bool ImageInfo::PrvObjInvokeDefault(NPObject *obj, const NPVariant *args,
                                    uint32_t argCount, NPVariant *result)
{
    TRACE("Entered %s", __FUNCTION__);
    return static_cast<ImageInfo*>(obj)->invokeDefault(args, argCount, result);
}
bool ImageInfo::PrvObjHasProperty(NPObject *obj, NPIdentifier name)
{
    TRACE("Entered %s", __FUNCTION__);
    return static_cast<ImageInfo*>(obj)->hasProperty(name);
}
bool ImageInfo::PrvObjGetProperty(NPObject *obj, NPIdentifier name, NPVariant *result)
{
    TRACE("Entered %s", __FUNCTION__);
    return static_cast<ImageInfo*>(obj)->getProperty(name, result);
}
bool ImageInfo::PrvObjSetProperty(NPObject *obj, NPIdentifier name,
                                  const NPVariant *value)
{
    TRACE("Entered %s", __FUNCTION__);
    return static_cast<ImageInfo*>(obj)->setProperty(name, value);
}
bool ImageInfo::PrvObjRemoveProperty(NPObject *obj, NPIdentifier name)
{
    TRACE("Entered %s", __FUNCTION__);
    return static_cast<ImageInfo*>(obj)->removeProperty(name);
}
bool ImageInfo::PrvObjEnumerate(NPObject *obj, NPIdentifier **value,
                                uint32_t *count)
{
    TRACE("Entered %s", __FUNCTION__);
    return static_cast<ImageInfo*>(obj)->enumerate(value, count);
}
bool ImageInfo::PrvObjConstruct(NPObject *obj, const NPVariant *args,
                                uint32_t argCount, NPVariant *result)
{
    TRACE("Entered %s", __FUNCTION__);
    return static_cast<ImageInfo*>(obj)->construct(args, argCount, result);
}

/**
 * Constructor.
 */
ImageInfo::ImageInfo(BrowserAdapter* adapter) :
    m_success(false), m_width(0), m_height(0)
{
    TRACE("Entered %s", __FUNCTION__);
    if (!sPropertyIdsInitialized) {
        AdapterBase::NPN_GetStringIdentifiers(sPropertyIdNames, kNumProperties, sPropertyIds);
        sPropertyIdsInitialized = true;
        TRACE("ImageInfo sPropertyIdsInitialized == true");
    }
}

ImageInfo::~ImageInfo()
{
    TRACE("Entered %s", __FUNCTION__);
}

void ImageInfo::initialize(bool success, const char* baseUri, const char* src, const char* title,
                           const char* altText, int32_t width, int32_t height, const char* mimeType)
{
    m_success = success;
    if (NULL != baseUri) {
        m_baseUri = baseUri;
    }
    m_src     = src;
    if (NULL != title) {
        m_title   = title;
    }
    if (NULL != altText) {
        m_altText = altText;
    }
    m_width   = width;
    m_height  = height;
    if (NULL != mimeType) {
        m_mimeType= mimeType;
    }
}

void ImageInfo::invalidate()
{
    TRACE("Entered %s", __FUNCTION__);
}

bool ImageInfo::hasMethod(NPIdentifier name)
{
    TRACE("Entered %s", __FUNCTION__);
    return false;
}

bool ImageInfo::invoke(NPIdentifier name, const NPVariant *args,
                       uint32_t argCount, NPVariant *rm_latitudeesult)
{
    TRACE("Entered %s", __FUNCTION__);
    return false;
}

bool ImageInfo::invokeDefault(const NPVariant *args,
                              uint32_t argCount, NPVariant *result)
{
    TRACE("Entered %s", __FUNCTION__);
    return false;
}

bool ImageInfo::hasProperty(NPIdentifier name)
{
    TRACE("Entered %s", __FUNCTION__);
    for (int i = 0; i < kNumProperties; i++) {
        if (sPropertyIds[i] == name) {
            return true;
        }
    }
    return false;
}

bool ImageInfo::getProperty(NPIdentifier name, NPVariant *result)
{
    TRACE("Entered %s", __FUNCTION__);
    if (NULL == result) {
        return false;
    }

    if (name == sPropertyIds[PropertySuccess]) {
        BOOLEAN_TO_NPVARIANT(m_success, *result);
        return true;
    }
    else if (name == sPropertyIds[PropertyBaseUri]) {
        if (m_baseUri.empty())
            NULL_TO_NPVARIANT(*result);
        else
            STRINGZ_TO_NPVARIANT(strdup(m_baseUri.c_str()), *result);
        return true;
    }
    else if (name == sPropertyIds[PropertySrc]) {
        if (m_src.empty())
            NULL_TO_NPVARIANT(*result);
        else
            STRINGZ_TO_NPVARIANT(strdup(m_src.c_str()), *result);
        return true;
    }
    else if (name == sPropertyIds[PropertyAltText]) {
        if (m_altText.empty())
            NULL_TO_NPVARIANT(*result);
        else
            STRINGZ_TO_NPVARIANT(strdup(m_altText.c_str()), *result);
        return true;
    }
    else if (name == sPropertyIds[PropertyMimeType]) {
        if (m_mimeType.empty())
            NULL_TO_NPVARIANT(*result);
        else
            STRINGZ_TO_NPVARIANT(strdup(m_mimeType.c_str()), *result);
        return true;
    }
    else if (name == sPropertyIds[PropertyTitle]) {
        if (m_altText.empty())
            NULL_TO_NPVARIANT(*result);
        else
            STRINGZ_TO_NPVARIANT(strdup(m_title.c_str()), *result);
        return true;
    }
    else if (name == sPropertyIds[PropertyWidth]) {
        DOUBLE_TO_NPVARIANT(m_width, *result);
        return true;
    }
    else if (name == sPropertyIds[PropertyHeight]) {
        DOUBLE_TO_NPVARIANT(m_height, *result);
        return true;
    }

    return false;
}

bool ImageInfo::setProperty(NPIdentifier name, const NPVariant *value)
{
    TRACE("Entered %s", __FUNCTION__);
    if (name == sPropertyIds[PropertySuccess]) {
        if (NPVARIANT_IS_BOOLEAN(*value)) {
            m_success = NPVARIANT_TO_BOOLEAN(*value);
            return true;
        }
    }
    else if (name == sPropertyIds[PropertyBaseUri]) {
        if (NPVARIANT_IS_STRING(*value)) {
            char* szval = AdapterBase::NPStringToString(NPVARIANT_TO_STRING(*value));
            m_baseUri = szval;
            ::free(szval);
            return true;
        }
    }
    else if (name == sPropertyIds[PropertySrc]) {
        if (NPVARIANT_IS_STRING(*value)) {
            char* szval = AdapterBase::NPStringToString(NPVARIANT_TO_STRING(*value));
            m_src = szval;
            ::free(szval);
            return true;
        }
    }
    else if (name == sPropertyIds[PropertyAltText]) {
        if (NPVARIANT_IS_STRING(*value)) {
            char* szval = AdapterBase::NPStringToString(NPVARIANT_TO_STRING(*value));
            m_altText = szval;
            ::free(szval);
            return true;
        }
    }
    else if (name == sPropertyIds[PropertyMimeType]) {
        if (NPVARIANT_IS_STRING(*value)) {
            char* szval = AdapterBase::NPStringToString(NPVARIANT_TO_STRING(*value));
            m_mimeType = szval;
            ::free(szval);
            return true;
        }
    }
    else if (name == sPropertyIds[PropertyTitle]) {
        if (NPVARIANT_IS_STRING(*value)) {
            char* szval = AdapterBase::NPStringToString(NPVARIANT_TO_STRING(*value));
            m_title = szval;
            ::free(szval);
            return true;
        }
    }
    else if (name == sPropertyIds[PropertyWidth]) {
        if (NPVARIANT_IS_DOUBLE(*value)) {
            m_width = NPVARIANT_TO_DOUBLE(*value);
            return true;
        }
    }
    else if (name == sPropertyIds[PropertyHeight]) {
        if (NPVARIANT_IS_DOUBLE(*value)) {
            m_height = NPVARIANT_TO_DOUBLE(*value);
            return true;
        }
    }
    return false;
}

bool ImageInfo::removeProperty(NPIdentifier name)
{
    TRACE("Entered %s", __FUNCTION__);
    return false;
}

bool ImageInfo::enumerate(NPIdentifier **value, uint32_t *count)
{
    TRACE("Entered %s", __FUNCTION__);
    return false;
}

bool ImageInfo::construct(const NPVariant *args, int32_t argCount,
                          NPVariant *result)
{
    TRACE("Entered %s", __FUNCTION__);
    return false;
}

