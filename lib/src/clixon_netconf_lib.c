/*
 *
  ***** BEGIN LICENSE BLOCK *****
 
  Copyright (C) 2009-2018 Olof Hagsand and Benny Holmgren

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

 * Netconf library functions. See RFC6241
 */

#ifdef HAVE_CONFIG_H
#include "clixon_config.h" /* generated by config & autoconf */
#endif

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <limits.h>
#include <fnmatch.h>
#include <stdint.h>
#include <assert.h>

/* cligen */
#include <cligen/cligen.h>

/* clixon */
#include "clixon_queue.h"
#include "clixon_hash.h"
#include "clixon_string.h"
#include "clixon_err.h"
#include "clixon_handle.h"
#include "clixon_yang.h"
#include "clixon_log.h"
#include "clixon_xml.h"
#include "clixon_netconf_lib.h"

/*! Create Netconf in-use error XML tree according to RFC 6241 Appendix A
 *
 * The request requires a resource that already is in use.
 * @param[out] cb      CLIgen buf. Error XML is written in this buffer
 * @param[in]  type     Error type: "application" or "protocol"
 * @param[in]  message  Error message
 */
int
netconf_in_use(cbuf *cb,
	       char *type,
	       char *message)
{
    int   retval = -1;
    char *encstr = NULL;
    
    if (cprintf(cb, "<rpc-reply><rpc-error>"
		"<error-tag>in-use</error-tag>"
		"<error-type>%s</error-type>"
		"<error-severity>error</error-severity>",
		type) <0)
	goto err;
    if (message){
	if (xml_chardata_encode(message, &encstr) < 0)
	    goto done;
	if (cprintf(cb, "<error-message>%s</error-message>", encstr) < 0)
	    goto err;
    }
    if (cprintf(cb, "</rpc-error></rpc-reply>") <0)
	goto err;
    retval = 0;
 done:
    if (encstr)
	free(encstr);
    return retval;
 err:
    clicon_err(OE_XML, errno, "cprintf");
    goto done;
}

/*! Create Netconf invalid-value error XML tree according to RFC 6241 Appendix A
 *
 * The request specifies an unacceptable value for one or more parameters.
 * @param[out] cb      CLIgen buf. Error XML is written in this buffer
 * @param[in]  type    Error type: "application" or "protocol"
 * @param[in]  message Error message
 */
int
netconf_invalid_value(cbuf *cb,
		      char *type,
		      char *message)
{
    int   retval = -1;
    char *encstr = NULL;

    if (cprintf(cb, "<rpc-reply><rpc-error>"
		"<error-tag>invalid-value</error-tag>"
		"<error-type>%s</error-type>"
		"<error-severity>error</error-severity>",
		type) <0)
	goto err;
    if (message){
	if (xml_chardata_encode(message, &encstr) < 0)
	    goto done;
	if (cprintf(cb, "<error-message>%s</error-message>", encstr) < 0)
	    goto err;
    }
    if (cprintf(cb, "</rpc-error></rpc-reply>") <0)
	goto err;
    retval = 0;
 done:
    if (encstr)
	free(encstr);
    return retval;
 err:
    clicon_err(OE_XML, errno, "cprintf");
    goto done;
}

/*! Create Netconf too-big error XML tree according to RFC 6241 Appendix A
 *
 * The request or response (that would be generated) is 
 * too large for the implementation to handle.
 * @param[out] cb      CLIgen buf. Error XML is written in this buffer
 * @param[in]  type    Error type: "transport", "rpc", "application", "protocol"
 * @param[in]  message Error message
 */
int
netconf_too_big(cbuf *cb,
		char *type,
		char *message)
{
    int     retval = -1;
    char *encstr = NULL;

    if (cprintf(cb, "<rpc-reply><rpc-error>"
		"<error-tag>too-big</error-tag>"
		"<error-type>%s</error-type>"
		"<error-severity>error</error-severity>",
		type) <0)
	goto err;
    if (message){
	if (xml_chardata_encode(message, &encstr) < 0)
	    goto done;
	if (cprintf(cb, "<error-message>%s</error-message>", encstr) < 0)
	    goto err;
    }
    if (cprintf(cb, "</rpc-error></rpc-reply>") <0)
	goto err;
    retval = 0;
 done:
    if (encstr)
	free(encstr);
    return retval;
 err:
    clicon_err(OE_XML, errno, "cprintf");
    goto done;
}

/*! Create Netconf missing-attribute error XML tree according to RFC 6241 App A
 *
 * An expected attribute is missing.
 * @param[out] cb      CLIgen buf. Error XML is written in this buffer
 * @param[in]  type    Error type: "rpc", "application" or "protocol"
 * @param[in]  info    bad-attribute or bad-element xml
 * @param[in]  message Error message
 */
int
netconf_missing_attribute(cbuf *cb,
			  char *type,
			  char *info,
			  char *message)
{
    int   retval = -1;
    char *encstr = NULL;

    if (cprintf(cb, "<rpc-reply><rpc-error>"
		"<error-tag>missing-attribute</error-tag>"
		"<error-type>%s</error-type>"
		"<error-info>%s</error-info>"
		"<error-severity>error</error-severity>",
		type, info) <0)
	goto err;
    if (message){
	if (xml_chardata_encode(message, &encstr) < 0)
	    goto done;
	if (cprintf(cb, "<error-message>%s</error-message>", encstr) < 0)
	    goto err;
    }
    if (cprintf(cb, "</rpc-error></rpc-reply>") <0)
	goto err;
    retval = 0;
 done:
    return retval;
 err:
    clicon_err(OE_XML, errno, "cprintf");
    goto done;
}

/*! Create Netconf bad-attribute error XML tree according to RFC 6241 App A
 *
 * An attribute value is not correct; e.g., wrong type,
 * out of range, pattern mismatch.
 * @param[out] cb      CLIgen buf. Error XML is written in this buffer
 * @param[in]  type    Error type: "rpc", "application" or "protocol"
 * @param[in]  info    bad-attribute or bad-element xml
 * @param[in]  message Error message
 */
int
netconf_bad_attribute(cbuf *cb,
		      char *type,
		      char *info,
		      char *message)
{
    int   retval = -1;
    char *encstr = NULL;

    if (cprintf(cb, "<rpc-reply><rpc-error>"
		"<error-tag>bad-attribute</error-tag>"
		"<error-type>%s</error-type>"
		"<error-info>%s</error-info>"
		"<error-severity>error</error-severity>",
		type, info) <0)
	goto err;
    if (message){
	if (xml_chardata_encode(message, &encstr) < 0)
	    goto done;
	if (cprintf(cb, "<error-message>%s</error-message>", encstr) < 0)
	    goto err;
    }
    if (cprintf(cb, "</rpc-error></rpc-reply>") <0)
	goto err;
    retval = 0;
 done:
    if (encstr)
	free(encstr);
    return retval;
 err:
    clicon_err(OE_XML, errno, "cprintf");
    goto done;
}

/*! Create Netconf unknwon-attribute error XML tree according to RFC 6241 App A
 *
 * An unexpected attribute is present.
 * @param[out] cb      CLIgen buf. Error XML is written in this buffer
 * @param[in]  type    Error type: "rpc", "application" or "protocol"
 * @param[in]  info    bad-attribute or bad-element xml
 * @param[in]  message Error message
 */
int
netconf_unknown_attribute(cbuf *cb,
			  char *type,
			  char *info,
			  char *message)
{
    int   retval = -1;
    char *encstr = NULL;

    if (cprintf(cb, "<rpc-reply><rpc-error>"
		"<error-tag>unknown-attribute</error-tag>"
		"<error-type>%s</error-type>"
		"<error-info>%s</error-info>"
		"<error-severity>error</error-severity>",
		type, info) <0)
	goto err;
    if (message){
	if (xml_chardata_encode(message, &encstr) < 0)
	    goto done;
	if (cprintf(cb, "<error-message>%s</error-message>", encstr) < 0)
	    goto err;
    }
    if (cprintf(cb, "</rpc-error></rpc-reply>") <0)
	goto err;
    retval = 0;
 done:
    if (encstr)
	free(encstr);
    return retval;
 err:
    clicon_err(OE_XML, errno, "cprintf");
    goto done;
}

/*! Create Netconf missing-element error XML tree according to RFC 6241 App A
 *
 * An expected element is missing.
 * @param[out] cb      CLIgen buf. Error XML is written in this buffer
 * @param[in]  type    Error type: "application" or "protocol"
 * @param[in]  info    bad-element xml
 * @param[in]  message Error message
 */
int
netconf_missing_element(cbuf      *cb, 
			char      *type,
			char      *info,
			char      *message)
{
    int   retval = -1;
    char *encstr = NULL;

    if (cprintf(cb, "<rpc-reply><rpc-error>"
		"<error-tag>missing-element</error-tag>"
		"<error-type>%s</error-type>"
		"<error-info>%s</error-info>"
		"<error-severity>error</error-severity>",
		type, info) <0)
	goto err;
    if (message){
	if (xml_chardata_encode(message, &encstr) < 0)
	    goto done;
	if (cprintf(cb, "<error-message>%s</error-message>", encstr) < 0)
	    goto err;
    }
    if (cprintf(cb, "</rpc-error></rpc-reply>") <0)
	goto err;
    retval = 0;
 done:
    if (encstr)
	free(encstr);
    return retval;
 err:
    clicon_err(OE_XML, errno, "cprintf");
    goto done;
}

/*! Create Netconf bad-element error XML tree according to RFC 6241 App A
 *
 * An element value is not correct; e.g., wrong type, out of range, 
 * pattern mismatch.
 * @param[out] cb      CLIgen buf. Error XML is written in this buffer
 * @param[in]  type    Error type: "application" or "protocol"
 * @param[in]  info    bad-element xml
 * @param[in]  message Error message
 */
int
netconf_bad_element(cbuf *cb,
		    char *type,
		    char *info,
		    char *message)
{
    int   retval = -1;
    char *encstr = NULL;

    if (cprintf(cb, "<rpc-reply><rpc-error>"
		"<error-tag>bad-element</error-tag>"
		"<error-type>%s</error-type>"
		"<error-info>%s</error-info>"
		"<error-severity>error</error-severity>",
		type, info) <0)
	goto err;
    if (message){
	if (xml_chardata_encode(message, &encstr) < 0)
	    goto done;
	if (cprintf(cb, "<error-message>%s</error-message>", encstr) < 0)
	    goto err;
    }
    if (cprintf(cb, "</rpc-error></rpc-reply>") <0)
	goto err;
    retval = 0;
 done:
    if (encstr)
	free(encstr);
    return retval;
 err:
    clicon_err(OE_XML, errno, "cprintf");
    goto done;
}

/*! Create Netconf unknown-element error XML tree according to RFC 6241 App A
 *
 * An unexpected element is present.
 * @param[out] cb      CLIgen buf. Error XML is written in this buffer
 * @param[in]  type    Error type: "application" or "protocol"
 * @param[in]  info    bad-element xml
 * @param[in]  message Error message
 */
int
netconf_unknown_element(cbuf *cb,
			char *type,
			char *info,
			char *message)
{
    int   retval = -1;
    char *encstr = NULL;

    if (cprintf(cb, "<rpc-reply><rpc-error>"
		"<error-tag>unknown-element</error-tag>"
		"<error-type>%s</error-type>"
		"<error-info>%s</error-info>"
		"<error-severity>error</error-severity>",
		type, info) <0)
	goto err;
    if (message){
	if (xml_chardata_encode(message, &encstr) < 0)
	    goto done;
	if (cprintf(cb, "<error-message>%s</error-message>", encstr) < 0)
	    goto err;
    }
    if (cprintf(cb, "</rpc-error></rpc-reply>") <0)
	goto err;
    retval = 0;
 done:
    if (encstr)
	free(encstr);
    return retval;
 err:
    clicon_err(OE_XML, errno, "cprintf");
    goto done;
}

/*! Create Netconf unknown-namespace error XML tree according to RFC 6241 App A
 *
 * An unexpected namespace is present.
 * @param[out] cb      CLIgen buf. Error XML is written in this buffer
 * @param[in]  type    Error type: "application" or "protocol"
 * @param[in]  info    bad-element or bad-namespace xml
 * @param[in]  message Error message
 */
int
netconf_unknown_namespace(cbuf *cb,
			  char *type,
			  char *info,
			  char *message)
{
    int   retval = -1;
    char *encstr = NULL;

    if (cprintf(cb, "<rpc-reply><rpc-error>"
		"<error-tag>unknown-namespace</error-tag>"
		"<error-type>%s</error-type>"
		"<error-info>%s</error-info>"
		"<error-severity>error</error-severity>",
		type, info) <0)
	goto err;
    if (message){
	if (xml_chardata_encode(message, &encstr) < 0)
	    goto done;
	if (cprintf(cb, "<error-message>%s</error-message>", encstr) < 0)
	    goto err;
    }
    if (cprintf(cb, "</rpc-error></rpc-reply>") <0)
	goto err;
    retval = 0;
 done:
    if (encstr)
	free(encstr);
    return retval;
 err:
    clicon_err(OE_XML, errno, "cprintf");
    goto done;
}

/*! Create Netconf access-denied error XML tree according to RFC 6241 App A
 *
 * An expected element is missing.
 * @param[out] cb      CLIgen buf. Error XML is written in this buffer
 * @param[in]  type    Error type: "application" or "protocol"
 * @param[in]  message Error message
 */
int
netconf_access_denied(cbuf *cb,
		      char *type,
		      char *message)
{
    int retval = -1;
    char *encstr = NULL;
    
    if (cprintf(cb, "<rpc-reply><rpc-error>"
		"<error-tag>access-denied</error-tag>"
		"<error-type>%s</error-type>"
		"<error-severity>error</error-severity>",
		type) <0)
	goto err;
    if (message){
	if (xml_chardata_encode(message, &encstr) < 0)
	    goto done;
	if (cprintf(cb, "<error-message>%s</error-message>", encstr) < 0)
	    goto err;
    }
    if (cprintf(cb, "</rpc-error></rpc-reply>") <0)
	goto err;
    retval = 0;
 done:
    if (encstr)
	free(encstr);
    return retval;
 err:
    clicon_err(OE_XML, errno, "cprintf");
    goto done;
}

/*! Create Netconf access-denied error XML tree according to RFC 6241 App A
 *
 * An expected element is missing.
 * @param[out] xret    Error XML tree 
 * @param[in]  type    Error type: "application" or "protocol"
 * @param[in]  message Error message
 */
int
netconf_access_denied_xml(cxobj **xret,
			  char   *type,
			  char   *message)
{
    int   retval =-1;
    cbuf *cbret = NULL;    

    if ((cbret = cbuf_new()) == NULL){
	clicon_err(OE_XML, errno, "cbuf_new");
	goto done;
    }
    if (netconf_access_denied(cbret, type, message) < 0)
	goto done;
    if (xml_parse_string(cbuf_get(cbret), NULL, xret) < 0)
	goto done;
    if (xml_rootchild(*xret, 0, xret) < 0)
	goto done;
    retval = 0;
 done:
    if (cbret)
	cbuf_free(cbret);
    return retval;
}

/*! Create Netconf lock-denied error XML tree according to RFC 6241 App A
 *
 * Access to the requested lock is denied because the lock is currently held
 * by another entity.
 * @param[out] cb      CLIgen buf. Error XML is written in this buffer
 * @param[in]  info    session-id xml
 * @param[in]  message Error message
 */
int
netconf_lock_denied(cbuf *cb,
		    char *info,
		    char *message)
{
    int   retval = -1;
    char *encstr = NULL;

    if (cprintf(cb, "<rpc-reply><rpc-error>"
		"<error-tag>lock-denied</error-tag>"
		"<error-type>protocol</error-type>"
		"<error-info>%s</error-info>"
		"<error-severity>error</error-severity>",
		info) <0)
	goto err;
    if (message){
	if (xml_chardata_encode(message, &encstr) < 0)
	    goto done;
	if (cprintf(cb, "<error-message>%s</error-message>", encstr) < 0)
	    goto err;
    }
    if (cprintf(cb, "</rpc-error></rpc-reply>") <0)
	goto err;
    retval = 0;
 done:
    if (encstr)
	free(encstr);
    return retval;
 err:
    clicon_err(OE_XML, errno, "cprintf");
    goto done;
}

/*! Create Netconf resource-denied error XML tree according to RFC 6241 App A
 *
 * An expected element is missing.
 * @param[out] cb      CLIgen buf. Error XML is written in this buffer
 * @param[in]  type    Error type: "transport, "rpc", "application", "protocol"
 * @param[in]  message Error message
 */
int
netconf_resource_denied(cbuf *cb,
			char *type,
			char *message)
{
    int   retval = -1;
    char *encstr = NULL;
    
    if (cprintf(cb, "<rpc-reply><rpc-error>"
		"<error-tag>resource-denied</error-tag>"
		"<error-type>%s</error-type>"
		"<error-severity>error</error-severity>",
		type) <0)
	goto err;
    if (message){
	if (xml_chardata_encode(message, &encstr) < 0)
	    goto done;
	if (cprintf(cb, "<error-message>%s</error-message>", encstr) < 0)
	    goto err;
    }
    if (cprintf(cb, "</rpc-error></rpc-reply>") <0)
	goto err;
    retval = 0;
 done:
    if (encstr)
	free(encstr);
    return retval;
 err:
    clicon_err(OE_XML, errno, "cprintf");
    goto done;
}

/*! Create Netconf rollback-failed error XML tree according to RFC 6241 App A
 *
 * Request to roll back some configuration change (via rollback-on-error or 
 * <discard-changes> operations) was not completed for some reason.
 * @param[out] cb      CLIgen buf. Error XML is written in this buffer
 * @param[in]  type    Error type: "application" or "protocol"
 * @param[in]  message Error message
 */
int
netconf_rollback_failed(cbuf *cb,
			char *type,
			char *message)
{
    int   retval = -1;
    char *encstr = NULL;

    if (cprintf(cb, "<rpc-reply><rpc-error>"
		"<error-tag>rollback-failed</error-tag>"
		"<error-type>%s</error-type>"
		"<error-severity>error</error-severity>",
		type) <0)
	goto err;
    if (message){
	if (xml_chardata_encode(message, &encstr) < 0)
	    goto done;
	if (cprintf(cb, "<error-message>%s</error-message>", encstr) < 0)
	    goto err;
    }
    if (cprintf(cb, "</rpc-error></rpc-reply>") <0)
	goto err;
    retval = 0;
 done:
    if (encstr)
	free(encstr);
    return retval;
 err:
    clicon_err(OE_XML, errno, "cprintf");
    goto done;
}

/*! Create Netconf data-exists error XML tree according to RFC 6241 Appendix A
 *
 * Request could not be completed because the relevant
 * data model content already exists.  For example,
 * a "create" operation was attempted on data that already exists.
 * @param[out] cb      CLIgen buf. Error XML is written in this buffer
 * @param[in]  message Error message
 */
int
netconf_data_exists(cbuf      *cb, 
		    char      *message)
{
    int   retval = -1;
    char *encstr = NULL;

    if (cprintf(cb, "<rpc-reply><rpc-error>"
		"<error-tag>data-exists</error-tag>"
		"<error-type>application</error-type>"
		"<error-severity>error</error-severity>") <0)
	goto err;
    if (message){
	if (xml_chardata_encode(message, &encstr) < 0)
	    goto done;
	if (cprintf(cb, "<error-message>%s</error-message>", encstr) < 0)
	    goto err;
    }
    if (cprintf(cb, "</rpc-error></rpc-reply>") <0)
	goto err;
    retval = 0;
 done:
    if (encstr)
	free(encstr);
    return retval;
 err:
    clicon_err(OE_XML, errno, "cprintf");
    goto done;
}

/*! Create Netconf data-missing error XML tree according to RFC 6241 App A
 *
 * Request could not be completed because the relevant data model content 
 * does not exist.  For example, a "delete" operation was attempted on
 * data that does not exist.
 * @param[out] cb      CLIgen buf. Error XML is written in this buffer
 * @param[in]  message Error message
 */
int
netconf_data_missing(cbuf *cb,
		     char *message)
{
    int   retval = -1;
    char *encstr = NULL;

    if (cprintf(cb, "<rpc-reply><rpc-error>"
		"<error-tag>data-missing</error-tag>"
		"<error-type>application</error-type>"
		"<error-severity>error</error-severity>") <0)
	goto err;
    if (message){
	if (xml_chardata_encode(message, &encstr) < 0)
	    goto done;
	if (cprintf(cb, "<error-message>%s</error-message>", encstr) < 0)
	    goto err;
    }
    if (cprintf(cb, "</rpc-error></rpc-reply>") <0)
	goto err;
    retval = 0;
 done:
    if (encstr)
	free(encstr);
    return retval;
 err:
    clicon_err(OE_XML, errno, "cprintf");
    goto done;
}

/*! Create Netconf operation-not-supported error XML according to RFC 6241 App A
 *
 * Request could not be completed because the requested operation is not
 * supported by this implementation.
 * @param[out] cb      CLIgen buf. Error XML is written in this buffer
 * @param[in]  type    Error type: "application" or "protocol"
 * @param[in]  message Error message
 */
int
netconf_operation_not_supported(cbuf *cb,
				char *type,
				char *message)
{
    int   retval = -1;
    char *encstr = NULL;

    if (cprintf(cb, "<rpc-reply><rpc-error>"
		"<error-tag>operation-not-supported</error-tag>"
		"<error-type>%s</error-type>"
		"<error-severity>error</error-severity>",
		type) <0)
	goto err;
    if (message){
	if (xml_chardata_encode(message, &encstr) < 0)
	    goto done;
	if (cprintf(cb, "<error-message>%s</error-message>", encstr) < 0)
	    goto err;
    }
    if (cprintf(cb, "</rpc-error></rpc-reply>") <0)
	goto err;
    retval = 0;
 done:
    if (encstr)
	free(encstr);
    return retval;
 err:
    clicon_err(OE_XML, errno, "cprintf");
    goto done;
}

/*! Create Netconf operation-failed error XML tree according to RFC 6241 App A
 *
 * Request could not be completed because the requested operation failed for
 * some reason not covered by any other error condition.
 * @param[out] cb      CLIgen buf. Error XML is written in this buffer
 * @param[in]  type    Error type: "rpc", "application" or "protocol"
 * @param[in]  message Error message
 */
int
netconf_operation_failed(cbuf  *cb,
			 char  *type,
			 char  *message)
{
    int   retval = -1;
    char *encstr = NULL;

    if (cprintf(cb, "<rpc-reply><rpc-error>"
		"<error-tag>operation-failed</error-tag>"
		"<error-type>%s</error-type>"
		"<error-severity>error</error-severity>",
		type) <0)
	goto err;
    if (message){
	if (xml_chardata_encode(message, &encstr) < 0)
	    goto done;
	if (cprintf(cb, "<error-message>%s</error-message>", encstr) < 0)
	    goto err;
    }
    if (cprintf(cb, "</rpc-error></rpc-reply>") < 0)
	goto err;
    retval = 0;
 done:
    if (encstr)
	free(encstr);
    return retval;
 err:
    clicon_err(OE_XML, errno, "cprintf");
    goto done;
}

/*! Create Netconf operation-failed error XML tree according to RFC 6241 App A
 *
 * Request could not be completed because the requested operation failed for
 * some reason not covered by any other error condition.
 * @param[out] xret    Error XML tree 
 * @param[in]  type    Error type: "rpc", "application" or "protocol"
 * @param[in]  message Error message
 */
int
netconf_operation_failed_xml(cxobj **xret,
			     char  *type,
			     char  *message)
{
    int   retval =-1;
    cbuf *cbret = NULL;    

    if ((cbret = cbuf_new()) == NULL){
	clicon_err(OE_XML, errno, "cbuf_new");
	goto done;
    }
    if (netconf_operation_failed(cbret, type, message) < 0)
	goto done;
    if (xml_parse_string(cbuf_get(cbret), NULL, xret) < 0)
	goto done;
    if (xml_rootchild(*xret, 0, xret) < 0)
	goto done;
    retval = 0;
 done:
    if (cbret)
	cbuf_free(cbret);
    return retval;
}

/*! Create Netconf malformed-message error XML tree according to RFC 6241 App A
 *
 * A message could not be handled because it failed to be parsed correctly.  
 * For example, the message is not well-formed XML or it uses an
 * invalid character set.
 * @param[out]  cb      CLIgen buf. Error XML is written in this buffer
 * @param[in]   message Error message
 * @note New in :base:1.1
 */
int
netconf_malformed_message(cbuf  *cb,
			  char  *message)
{
    int   retval = -1;
    char *encstr = NULL;

    if (cprintf(cb, "<rpc-reply><rpc-error>"
		"<error-tag>malformed-message</error-tag>"
		"<error-type>rpc</error-type>"
		"<error-severity>error</error-severity>") <0)
	goto err;
    if (message){
	if (xml_chardata_encode(message, &encstr) < 0)
	    goto done;
	if (cprintf(cb, "<error-message>%s</error-message>", encstr) < 0)
	    goto err;
    }
    if (cprintf(cb, "</rpc-error></rpc-reply>") <0)
	goto err;
    retval = 0;
 done:
    if (encstr)
	free(encstr);
    return retval;
 err:
    clicon_err(OE_XML, errno, "cprintf");
    goto done;
}
