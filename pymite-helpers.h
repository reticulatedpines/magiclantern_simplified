/** \file
 * PyMite wrappers
 */
#ifndef _pymite_h_
#define _pymite_h_

#include "pm.h"

#define PM_ARG_INT(num) \
	( { \
		pPmObj_t pn = NATIVE_GET_LOCAL(num); \
		if( OBJ_GET_TYPE(pn) != OBJ_TYPE_INT ) \
		{ \
			PM_RAISE( retval, PM_RET_EX_TYPE ); \
			return retval; \
		} \
		((pPmInt_t)pn)->val; \
	} )
		
#endif
