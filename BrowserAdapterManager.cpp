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

#include "BrowserAdapterManager.h"

#include "BrowserAdapter.h"

BrowserAdapterManager* BrowserAdapterManager::instance()
{
    static BrowserAdapterManager* s_instance = 0;
    if (G_UNLIKELY(s_instance == 0)) {

        s_instance = new BrowserAdapterManager;
    }

    return s_instance;
}

BrowserAdapterManager::BrowserAdapterManager()
    : m_adapterList(0)
{
}

BrowserAdapterManager::~BrowserAdapterManager()
{
    g_list_free(m_adapterList);
}

void BrowserAdapterManager::registerAdapter(BrowserAdapter* adapter)
{
    m_adapterList = g_list_prepend(m_adapterList, adapter);
}

void BrowserAdapterManager::unregisterAdapter(BrowserAdapter* adapter)
{
    m_adapterList = g_list_remove(m_adapterList, adapter);
}

void BrowserAdapterManager::adapterActivated(BrowserAdapter* adapter, bool activated)
{
    if (activated) {

        GList* head = g_list_first(m_adapterList);
        if (head && head->data != adapter) {

            m_adapterList = g_list_remove(m_adapterList, adapter);
            m_adapterList = g_list_prepend(m_adapterList, adapter);
        }

        // Freeze all the other adapters and thaw this one
        for (GList* iter = g_list_first(m_adapterList); iter; iter = g_list_next(iter)) {

            BrowserAdapter* a = (BrowserAdapter*) iter->data;
            if (a == adapter)
                continue;

            a->freeze();
        }

        adapter->thaw();
    }
    /*
    // if we are really concerned about memory we should enable this
    else {
    adapter->freeze();
    }
    */
}
void BrowserAdapterManager::inactiveAdaptersActivate()
{

    for (GList* iter = g_list_first(m_adapterList); iter; iter = g_list_next(iter)) {

        BrowserAdapter* a = (BrowserAdapter*) iter->data;
        //thaw the first frozen adapter that is encountered
        if(a->isFrozen()) {

            a->thaw();
            break;
        }

    }
}
