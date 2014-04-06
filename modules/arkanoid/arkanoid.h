#ifndef _arkanoid_h_
#define _arkanoid_h_

typedef struct element
{
    int type;
    
    float x;
    float old_x;
    float y;
    float old_y;
    
    int w;
    int h;
    
    float deltaX;
    float deltaY;
    float speed;
    
    int color;
    
    int fade;
    int fade_delta;
    
    int c1;
    int deleted;
	
    struct element * next;
    struct element * prev;
} element;

#endif
