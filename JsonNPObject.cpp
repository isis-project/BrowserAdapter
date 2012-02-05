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

#include "JsonNPObject.h"
#include "BrowserAdapter.h"
#include "Debug.h"

JsonNPObject::JsonNPObject(BrowserAdapter *adapter) :
    m_adapter(adapter)
{
}

JsonNPObject::~JsonNPObject()
{
    for (std::map<std::string, NPObject *>::iterator i = m_objects.begin();
            i != m_objects.end(); m_objects.erase(i++)) {
        if (i->second != NULL) {
            m_adapter->NPN_ReleaseObject(i->second);
            i->second = NULL;
        }
    }
}

bool JsonNPObject::initialize(pbnjson::JSchemaFile &schema, const char *json)
{
    pbnjson::JDomParser parser(NULL);

    //TRACEF("%s", json);

    if (!parser.parse(json, schema, NULL)) {
        TRACEF("failed to parse JSON\n");
        return false;
    }

    m_dom = parser.getDom();

    return true;
}

bool JsonNPObject::initialize(pbnjson::JValue &dom)
{
    m_dom = dom;
    return true;
}

bool JsonNPObject::hasProperty(NPIdentifier name)
{
    //TRACEF("%s", m_adapter->NPN_UTF8FromIdentifier(name));
    return m_dom.hasKey(m_adapter->NPN_UTF8FromIdentifier(name));
}

bool JsonNPObject::getProperty(NPIdentifier name, NPVariant *result)
{
    if (!result) {
        return false;
    }

    std::string key = m_adapter->NPN_UTF8FromIdentifier(name);
    pbnjson::JValue val = m_dom[key];

    //TRACEF("%s", key.c_str());

    if (val.isNull()) {
        //TRACEF("result is null");
        NULL_TO_NPVARIANT(*result);

    } else if (val.isNumber()) {
        double d;
        if (val.asNumber<double>(d)) {
            NULL_TO_NPVARIANT(*result);
        } else {
            //TRACEF("result is double: %f", d);
            DOUBLE_TO_NPVARIANT(d, *result);
        }

    } else if (val.isString()) {
        std::string s;
        if (val.asString(s)) {
            NULL_TO_NPVARIANT(*result);
        } else {
            //TRACEF("result is string: %s", s.c_str());
            STRINGZ_TO_NPVARIANT(strdup(s.c_str()), *result);
        }

    } else if (val.isBoolean()) {
        bool b;
        if (val.asBool(b)) {
            NULL_TO_NPVARIANT(*result);
        } else {
            //TRACEF("result is bool: %d", b);
            BOOLEAN_TO_NPVARIANT(b, *result);
        }

    } else if (val.isObject()) {
        NPObject *obj = NULL;

        //TRACEF("result is object");

        std::map<std::string, NPObject *>::iterator i = m_objects.find(key);
        if (i != m_objects.end()) {
            /* we've accessed this object before */
            obj = i->second;

        } else {
            /* we need to construct an NPObject */
            JsonNPObject *jobj =
                static_cast<JsonNPObject *>(m_adapter->NPN_CreateObject(
                                                &JsonNPObject::sJsonNPObjectClass));
            jobj->initialize(val);

            /* add it into the cached objects */
            m_objects.insert(std::pair<std::string, NPObject *>(key, jobj));

            obj = jobj;
        }

        m_adapter->NPN_RetainObject(obj);
        OBJECT_TO_NPVARIANT(obj, *result);

    } else if (val.isArray()) {
        //TRACEF("array type not supported!\n");
        NULL_TO_NPVARIANT(*result);
        return false;
    }

    return true;
}

NPClass JsonNPObject::sJsonNPObjectClass = {
    NP_CLASS_STRUCT_VERSION_CTOR,
    JsonNPObject::PrvObjAllocate,
    JsonNPObject::PrvObjDeallocate,
    JsonNPObject::PrvObjInvalidate,
    JsonNPObject::PrvObjHasMethod,
    JsonNPObject::PrvObjInvoke,
    JsonNPObject::PrvObjInvokeDefault,
    JsonNPObject::PrvObjHasProperty,
    JsonNPObject::PrvObjGetProperty,
    JsonNPObject::PrvObjSetProperty,
    JsonNPObject::PrvObjRemoveProperty,
    JsonNPObject::PrvObjEnumerate,
    JsonNPObject::PrvObjConstruct,
};

NPObject* JsonNPObject::PrvObjAllocate(NPP npp, NPClass* klass)
{
    JsonNPObject *obj =
        new JsonNPObject(static_cast<BrowserAdapter *>(npp->pdata));
    //TRACEF("%p", obj);
    return obj;
}

void JsonNPObject::PrvObjDeallocate(NPObject* obj)
{
    //TRACEF("%p", obj);
    delete static_cast<JsonNPObject *>(obj);
}

void JsonNPObject::PrvObjInvalidate(NPObject* obj)
{
    /* do nothing */
}

bool JsonNPObject::PrvObjHasMethod(NPObject* obj, NPIdentifier name)
{
    /* unsupported */
    return false;
}

bool JsonNPObject::PrvObjInvoke(NPObject *obj, NPIdentifier name,
                                const NPVariant *args, uint32_t argCount, NPVariant *result)
{
    /* unsupported */
    return false;
}

bool JsonNPObject::PrvObjInvokeDefault(NPObject *obj, const NPVariant *args,
                                       uint32_t argCount, NPVariant *result)
{
    /* unsupported */
    return false;
}

bool JsonNPObject::PrvObjHasProperty(NPObject *obj, NPIdentifier name)
{
    return static_cast<JsonNPObject *>(obj)->hasProperty(name);
}

bool JsonNPObject::PrvObjGetProperty(NPObject *obj, NPIdentifier name,
                                     NPVariant *result)
{
    return static_cast<JsonNPObject *>(obj)->getProperty(name, result);
}

bool JsonNPObject::PrvObjSetProperty(NPObject *obj, NPIdentifier name,
                                     const NPVariant *value)
{
    /* unsupported */
    return false;
}

bool JsonNPObject::PrvObjRemoveProperty(NPObject *obj, NPIdentifier name)
{
    /* unsupported */
    return false;
}

bool JsonNPObject::PrvObjEnumerate(NPObject *obj, NPIdentifier **value,
                                   uint32_t *count)
{
    /* unsupported */
    return false;
}

bool JsonNPObject::PrvObjConstruct(NPObject *obj, const NPVariant *args,
                                   uint32_t argCount, NPVariant *result)
{
    /* unsupported */
    return false;
}
