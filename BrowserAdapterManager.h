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

#ifndef BROWSERADAPTERMANAGER_H
#define BROWSERADAPTERMANAGER_H

#include <glib.h>

class BrowserAdapter;

class BrowserAdapterManager
{
public:

	static BrowserAdapterManager* instance();

	void registerAdapter(BrowserAdapter* adapter);
	void unregisterAdapter(BrowserAdapter* adapter);
	void adapterActivated(BrowserAdapter* adapter, bool activated);
	void inactiveAdaptersActivate();
	
private:

	BrowserAdapterManager();
	~BrowserAdapterManager();

private:
	
	GList* m_adapterList;
};


#endif /* BROWSERADAPTERMANAGER_H */
