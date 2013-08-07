#ifndef _arkanoid_h_
#define _arkanoid_h_

typedef struct
{
    int type;
    
    float x;
    float y;
    int z;
    
    int w;
    int h;
    
    float deltaX;
    float deltaY;
    float speed;
    
    int color;
    
    int fade;
    int fade_delta;
    
    int c1;
} element;

#endif