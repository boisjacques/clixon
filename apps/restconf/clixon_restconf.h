/*
 *
  ***** BEGIN LICENSE BLOCK *****
 
  Copyright (C) 2009-2019 Olof Hagsand

  This file is part of CLIXON.

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  Alternatively, the contents of this file may be used under the terms of
  the GNU General Public License Version 3 or later (the "GPL"),
  in which case the provisions of the GPL are applicable instead
  of those above. If you wish to allow use of your version of this file only
  under the terms of the GPL, and not to allow others to
  use your version of this file under the terms of Apache License version 2, 
  indicate your decision by deleting the provisions above and replace them with
  the  notice and other provisions required by the GPL. If you do not delete
  the provisions above, a recipient may use your version of this file under
  the terms of any one of the Apache License version 2 or the GPL.

  ***** END LICENSE BLOCK *****

 * The exported interface to plugins. External apps (eg frontend restconf plugins)
 * should only include this file (not the restconf_*.h)
 */

#ifndef _CLIXON_RESTCONF_H_
#define _CLIXON_RESTCONF_H_

/*
 * Types (also in restconf_lib.h)
 */
enum restconf_media{
    YANG_DATA_JSON,  /* "application/yang-data+json" */
    YANG_DATA_XML,   /* "application/yang-data+xml" */
    YANG_PATCH_JSON, /* "application/yang-patch+json" */
    YANG_PATCH_XML   /* "application/yang-patch+xml" */
};
typedef enum restconf_media restconf_media;

/*
 * Prototypes (also in restconf_lib.h)
 */
int restconf_err2code(char *tag);
const char *restconf_code2reason(int code);

int badrequest(FCGX_Request *r);
int unauthorized(FCGX_Request *r);
int forbidden(FCGX_Request *r);
int notfound(FCGX_Request *r);
int conflict(FCGX_Request *r);
int internal_server_error(FCGX_Request *r);
int notimplemented(FCGX_Request *r);
int restconf_test(FCGX_Request *r, int dbg);
cbuf *readdata(FCGX_Request *r);
int get_user_cookie(char *cookiestr, char  *attribute, char **val);
int api_return_err(clicon_handle h, FCGX_Request *r, cxobj *xerr,
		   int pretty, restconf_media media, int code);


#endif /* _CLIXON_RESTCONF_H_ */
