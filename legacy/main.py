# PyMite interface to the Magic Lantern firmware

"""__NATIVE__
#include "bmp.h"
#include "pymite-helpers.h"
"""

def bmp_puts(x,y,s):
	"""__NATIVE__
	PmReturn_t retval = PM_RET_OK;

	if( NATIVE_GET_NUM_ARGS() != 3 )
	{
		PM_RAISE( retval, PM_RET_EX_TYPE );
		return retval;
	}

	int x = PM_ARG_INT(0);
	int y = PM_ARG_INT(1);

	pPmObj_t pn = NATIVE_GET_LOCAL(2);
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

def bmp_fill(color,x,y,w,h):
	"""__NATIVE__
	PmReturn_t retval = PM_RET_OK;

	if( NATIVE_GET_NUM_ARGS() != 5 )
	{
		PM_RAISE( retval, PM_RET_EX_TYPE );
		return retval;
	}

	unsigned color = PM_ARG_INT(0);
	unsigned x = PM_ARG_INT(1);
	unsigned y = PM_ARG_INT(2);
	unsigned w = PM_ARG_INT(3);
	unsigned h = PM_ARG_INT(4);
	bmp_fill( color, x, y, w, h );
	NATIVE_SET_TOS(PM_NONE);
	return retval;
	"""
	pass

#print "Hello, world from %s!" % "Pymite"


