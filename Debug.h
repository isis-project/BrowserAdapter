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

#ifndef DEBUG_H
#define DEBUG_H

#include <syslog.h>

#undef TRACE
#undef TRACEF
#undef EVENT_TRACEF

#undef DEBUG
#undef DEBUG_EVENTS

//#define DEBUG 1
//#define DEBUG_EVENTS 1

#ifdef DEBUG

#define TRACE \
do { \
	openlog("browser-adapter", 0, LOG_USER); \
	syslog(LOG_DEBUG, "(%s:%d) %s\n", __FILE__, __LINE__, __FUNCTION__); \
	closelog(); \
} while (0)
#define TRACEF(fmt, args...) \
do { \
	openlog("browser-adapter", 0, LOG_USER); \
	syslog(LOG_DEBUG, "(%s:%d) %s: " fmt "\n", __FILE__, __LINE__, __FUNCTION__, ## args); \
	closelog(); \
} while (0)
#else
#define TRACE (void)0
#define TRACEF(fmt, args...) (void)0
#endif

#ifdef DEBUG_EVENTS
#define EVENT_TRACEF(fmt, args...) TRACEF(fmt, ## args)
#else
#define EVENT_TRACEF(fmt, args...) (void)0
#endif

#define LOG(fmt, args...) \
do { \
	openlog("browser-adapter", 0, LOG_USER); \
	syslog(LOG_DEBUG, "(%s:%d) %s: " fmt "\n", __FILE__, __LINE__, __FUNCTION__, ## args); \
	closelog(); \
} while (0)

#endif
