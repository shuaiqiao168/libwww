/*								      CSParse.c
**	PICS CONFIGURATION MANAGER FOR CLIENTS AND SERVERS
**
**	(c) COPYRIGHT MIT 1995.
**	Please first read the full copyright statement in the file COPYRIGHT.
**
**	This module converts application/xpics streams (files or network) to PICS_ class data
**
** History:
**	 4 Dec 95  EGP  start
**
** BUGS: no code yet; doesn't actually do anything
*/

/* Library include files */
#include "WWWLib.h"
#include "HTProxy.h"
#include "CSLUtils.h"
#include "CSMR.h"
#include "CSUser.h"
#include "CSLL.h"
#include "CSLApp.h"

struct _HTStream {
    const HTStreamClass * isa;
    HTRequest *		  request;
    HTStream *		  target;
    CSParse_t *		  pCSParse;
};

PRIVATE int CSParse_put_block (HTStream * me, const char * b, int l)
{
    if (CSParse_parseChunk(me->pCSParse, b, l, 0) == CSDoMore_error)
        return HT_ERROR;
    return HT_OK;
}

PRIVATE int CSParse_put_character (HTStream * me, char c)
{
    return CSParse_put_block(me, &c, 1);
}

PRIVATE int CSParse_put_string (HTStream * me, const char * s)
{
    return CSParse_put_block(me, s, (int) strlen(s));
}

PRIVATE int CSParse_flush (HTStream * me)
{
    return (*me->target->isa->flush)(me->target);
}

PRIVATE int CSParse_free (HTStream * me)
{
    int status = HT_OK;
    if (me->target) {
	if ((status = (*me->target->isa->_free)(me->target)) == HT_WOULD_BLOCK)
	    return HT_WOULD_BLOCK;
    }
    if (APP_TRACE)
	HTTrace("Parser....... FREEING....\n");
    CSParse_delete(me->pCSParse);
    return status;
}
#if 0
PRIVATE int CSParse_abort (HTStream * me, HTList * e)
{
    int status = HT_ERROR;
    if (me->target) status = (*me->target->isa->abort)(me->target, e);
    if (APP_TRACE) HTTrace("Rules....... ABORTING...\n");
    HT_FREE(me);
    return status;
}

PRIVATE const HTStreamClass CSParseClass =
{		
    "PICSParser",
    CSParse_flush,
    CSParse_free,
    CSParse_abort,
    CSParse_put_character,
    CSParse_put_string,
    CSParse_put_block
};
#endif
PRIVATE HTStream * CSParseConverter_new (HTRequest *	request,
			                             HTStream *	output_stream)
{
    HTStream * me;
    if ((me = (HTStream *) HT_CALLOC(1, sizeof(HTStream))) == NULL)
        HT_OUTOFMEM("CSParse");
    me->request = request;
    me->target = output_stream;
    return me;
}

PRIVATE int CSParseMachRead_free (HTStream * me)
{
    int status = CSParse_free(me);
	CSParse_deleteMachReadState(me->pCSParse);
    return status;
}

PRIVATE int CSParseMachRead_abort (HTStream * me, HTList * e)
{
    int status = CSParse_free(me);
        CSParse_deleteMachReadState(me->pCSParse);
    return status;
}

PRIVATE const HTStreamClass CSParseClass_machRead =
{		
    "PICSParser",
    CSParse_flush,
    CSParseMachRead_free,
    CSParseMachRead_abort,
    CSParse_put_character,
    CSParse_put_string,
    CSParse_put_block
};

PUBLIC HTStream * CSParseMachRead (HTRequest *	request,
				   void *	param,
				   HTFormat  	input_format,
				   HTFormat     output_format,
				   HTStream *	output_stream)
{
    HTStream * me = CSParseConverter_new(request, output_stream);
    me->isa = &CSParseClass_machRead;
    me->pCSParse = CSParse_newMachReadState();
    return me;
}

PRIVATE int CSParseUser_free (HTStream * me)
{
/*    CSParse_deleteUser(me->pCSParse); */
    CSLoadedUser_add(CSParse_getUser(me->pCSParse), 
		      HTAnchor_address((HTAnchor *)HTRequest_anchor(me->request)));
    CSParse_free(me);
    return HT_OK;
}

PRIVATE int CSParseUser_abort (HTStream * me, HTList * e)
{
    CSParse_deleteUser(me->pCSParse);
    HT_FREE(me);
    return HT_OK;
}

PRIVATE const HTStreamClass CSParseClass_user =
{		
    "PICSParser",
    CSParse_flush,
    CSParseUser_free,
    CSParseUser_abort,
    CSParse_put_character,
    CSParse_put_string,
    CSParse_put_block
};

PUBLIC HTStream * CSParseUser (HTRequest *  request,
			       void *	    param,
			       HTFormat     input_format,
			       HTFormat	    output_format,
			       HTStream *   output_stream)
{
    HTStream * me = CSParseConverter_new(request, output_stream);
    me->isa = &CSParseClass_user;
    me->pCSParse = CSParse_newUser();
    me->request = request;
/*
    if (HTRequest_context(request))
        *((CSUser_t **)HTRequest_context(request)) = CSParse_getUser(me->pCSParse);
*/
    return me;
}

PRIVATE int CSParseLabel_free (HTStream * me)
{
    CSApp_label(me->request, CSParse_getLabel(me->pCSParse));
/*    CSParse_deleteLabel(me->pCSParse); keep it around and let CSApp free it */
    return CSParse_free(me);
}

PRIVATE int CSParseLabel_abort (HTStream * me, HTList * e)
{
    CSParse_deleteLabel(me->pCSParse);
    return CSParse_free(me);
}

PRIVATE const HTStreamClass CSParseClass_label =
{		
    "PICSParser",
    CSParse_flush,
    CSParseLabel_free,
    CSParseLabel_abort,
    CSParse_put_character,
    CSParse_put_string,
    CSParse_put_block
};

PUBLIC HTStream * CSParseLabel (HTRequest *	request,
           			            void *	    param,
			                    HTFormat  	input_format,
            			        HTFormat	    output_format,
			                    HTStream *	output_stream)
{
    HTStream * me = CSParseConverter_new(request, output_stream);
    me->isa = &CSParseClass_label;
	me->pCSParse = CSParse_newLabel(0, 0);
    return me;
}
