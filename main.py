# PyMite interface to the Magic Lantern firmware

"""__NATIVE__
#include "bmp.h"
"""

def bmp_puts(x,y,s):
	"""__NATIVE__
	PmReturn_t retval = PM_RET_OK;

	if( NATIVE_GET_NUM_ARGS() != 3 )
	{
		PM_RAISE( retval, PM_RET_EX_TYPE );
		return retval;
	}

	pPmObj_t pn;
	pn = NATIVE_GET_LOCAL(0);
	if( OBJ_GET_TYPE(pn) != OBJ_TYPE_INT )
	{
		PM_RAISE( retval, PM_RET_EX_TYPE );
		return retval;
	}

	int x = ((pPmInt_t)pn)->val;

	pn = NATIVE_GET_LOCAL(1);
	if( OBJ_GET_TYPE(pn) != OBJ_TYPE_INT )
	{
		PM_RAISE( retval, PM_RET_EX_TYPE );
		return retval;
	}

	int y = ((pPmInt_t)pn)->val;

	pn = NATIVE_GET_LOCAL(2);
	if( OBJ_GET_TYPE(pn) == OBJ_TYPE_STR )
	{
		bmp_printf( FONT_LARGE, x, y, "%s", ((pPmString_t)pn)->val );
		NATIVE_SET_TOS(PM_NONE);
		return retval;
	}

	if( OBJ_GET_TYPE(pn) == OBJ_TYPE_INT )
	{
		bmp_printf( FONT_LARGE, x, y, "%d", (int) ((pPmInt_t)pn)->val );
		NATIVE_SET_TOS(PM_NONE);
		return retval;
	}

	PM_RAISE( retval, PM_RET_EX_TYPE );
	return retval;
	"""
	pass

#print "Hello, world from %s!" % "Pymite"


