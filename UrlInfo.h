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

#ifndef URLINFO_H
#define URLINFO_H

#include <npupp.h>
#include <npapi.h>
#include <string>

class BrowserAdapter;

class UrlInfo : public NPObject
{
public:
    UrlInfo(BrowserAdapter *adapter);
    ~UrlInfo();

    static NPClass sUrlInfoClass;

    void initialize(bool success, const char* url, const char* desc,
                    int left, int top, int right, int bottom);
    void invalidate();
    bool hasMethod(NPIdentifier name);
    bool invoke(NPIdentifier name, const NPVariant *args, uint32_t argCount, NPVariant *result);
    bool invokeDefault(const NPVariant *args, uint32_t argCount, NPVariant *result);
    bool hasProperty(NPIdentifier name);
    bool getProperty(NPIdentifier name, NPVariant *result);
    bool setProperty(NPIdentifier name, const NPVariant *value);
    bool removeProperty(NPIdentifier name);
    bool enumerate(NPIdentifier **value, uint32_t *count);
    bool construct(const NPVariant *args, int32_t argCount, NPVariant *result);


    // Private NP Object callbacks, used for JavaScript integration.
    // (these translate into appropriate non-static method calls).
    static NPObject* PrvObjAllocate(NPP npp, NPClass* klass);
    static void PrvObjDeallocate(NPObject* obj);
    static void PrvObjInvalidate(NPObject* obj);
    static bool PrvObjHasMethod(NPObject* obj, NPIdentifier name);
    static bool PrvObjInvoke(NPObject *obj, NPIdentifier name, const NPVariant *args, uint32_t argCount, NPVariant *result);
    static bool PrvObjInvokeDefault(NPObject *obj, const NPVariant *args, uint32_t argCount, NPVariant *result);
    static bool PrvObjHasProperty(NPObject *obj, NPIdentifier name);
    static bool PrvObjGetProperty(NPObject *obj, NPIdentifier name, NPVariant *result);
    static bool PrvObjSetProperty(NPObject *obj, NPIdentifier name, const NPVariant *value);
    static bool PrvObjRemoveProperty(NPObject *obj, NPIdentifier name);
    static bool PrvObjEnumerate(NPObject *obj, NPIdentifier **value, uint32_t *count);
    static bool PrvObjConstruct(NPObject *obj, const NPVariant *args, uint32_t argCount, NPVariant *result);

private:
    bool m_success;
    std::string	m_url;
    std::string	m_desc;
    NPObject* m_bounds;
};

#endif



