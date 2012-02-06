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

#ifndef JSON_NPOBJECT_H
#define JSON_NPOBJECT_H

#include <npupp.h>
#include <npapi.h>

#include <string>
#include <map>

#include <pbnjson.hpp>

class BrowserAdapter;

/**
 * \brief This class implements a generic NPObject that is initialized
 * with Json.
 *
 * Only getting properties is supported. Methods, and modifying/deleting
 * properties are not supported.
 */

class JsonNPObject : public NPObject
{
public:
    JsonNPObject(BrowserAdapter *adapter);
    ~JsonNPObject();

    /**
     * Defines methods for the object.
     */
    static NPClass sJsonNPObjectClass;

    /**
     * Initializes the object with JSON.
     * \param json Json that represents the object.
     * \return True if successful.
     */
    bool initialize(pbnjson::JSchemaFile &schema, const char *json);

    /**
     * Initialize the object with a parsed DOM
     * \param dom Representation of the DOM.
     */
    bool initialize(pbnjson::JValue &dom);

private:
    /**
     * \name NPClass methods
     */
    /*@{*/
    static NPObject* PrvObjAllocate(NPP npp, NPClass* klass);
    static void PrvObjDeallocate(NPObject* obj);
    static void PrvObjInvalidate(NPObject* obj);
    static bool PrvObjHasMethod(NPObject* obj, NPIdentifier name);
    static bool PrvObjInvoke(NPObject *obj, NPIdentifier name,
                             const NPVariant *args, uint32_t argCount, NPVariant *result);
    static bool PrvObjInvokeDefault(NPObject *obj, const NPVariant *args,
                                    uint32_t argCount, NPVariant *result);
    static bool PrvObjHasProperty(NPObject *obj, NPIdentifier name);
    static bool PrvObjGetProperty(NPObject *obj, NPIdentifier name,
                                  NPVariant *result);
    static bool PrvObjSetProperty(NPObject *obj, NPIdentifier name,
                                  const NPVariant *value);
    static bool PrvObjRemoveProperty(NPObject *obj, NPIdentifier name);
    static bool PrvObjEnumerate(NPObject *obj, NPIdentifier **value,
                                uint32_t *count);
    static bool PrvObjConstruct(NPObject *obj, const NPVariant *args,
                                uint32_t argCount, NPVariant *result);
    /*@}*/

    /**
     * \name Instance methods that implement the NPClass methods.
     */
    /*@{*/
    bool hasProperty(NPIdentifier name);
    bool getProperty(NPIdentifier name, NPVariant *result);
    /*@}*/

private:
    /**
     * \brief DOM representation of the parsed Json input.
     */
    pbnjson::JValue m_dom;

    /**
     * \brief Map of this object's properties that are objects
     * themselves.
     *
     * This is populated lazily.
     */
    std::map<std::string, NPObject *> m_objects;

    /**
     * \brief A reference to the NPP object.
     */
    BrowserAdapter *m_adapter;
};

#endif
