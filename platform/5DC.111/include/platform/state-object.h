#ifndef __platform_state_object_h
#define __platform_state_object_h

// we need to detect halfshutter press from EMState.
#define EMState (*(struct state_object **)0x4f24)

#endif // __platform_state_object_h
