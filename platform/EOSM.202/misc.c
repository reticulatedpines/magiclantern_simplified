// misc functions specific to 60D/109

#include <dryos.h>
#include <property.h>
#include <bmp.h>
#include <config.h>
#include <consts.h>
#include <lens.h>


int new_LiveViewApp_handler = 0xff123456;



// dummy stubs
int handle_af_patterns(struct event * event) { return 1; }
