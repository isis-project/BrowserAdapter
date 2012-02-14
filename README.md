BrowserAdapter
==============

This is the [NPAPI](/isis-project/npapi-headers) browser plugin to [isis-browser](/isis-project/isis-browser). This plugin communicates with the [BrowserServer](/isis-project/BrowserServer) which renders a web page into this adapter.

* __js_*__ methods are the implementations of the exposed JavaScript routines callable by a JavaScript program.

* __msg*__ methods are unsolicited messages sent by the BrowserServer when it needs to inform this adapter of something.

* __handle*__ methods are called by the browser via the [AdapterBase](/isis-project/AdapterBase)::PrvNPP_HandleEvent NPAPI plugin callback event handler.

_No further development is planned on the component._

For current development news see [isis-project](http://isis-project.org).

License
-------
This code is released under the Apache 2.0 license.

#### Copyright and License Information

All content, including all source code files and documentation files in this repository are:
Copyright (c) 2012 Hewlett-Packard Development Company, L.P.

All content, including all source code files and documentation files in this repository are:
Licensed under the Apache License, Version 2.0 (the "License");
you may not use this content except in compliance with the License.
You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

#### end Copyright and License Information