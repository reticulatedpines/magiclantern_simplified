#ifndef __VECTORSCOPE_H_
#define __VECTORSCOPE_H_
int vectorscope_should_draw();
void vectorscope_request_draw(int flag);
void vectorscope_start();
void vectorscope_pixel_step(int Y, int U, int V);
void vectorscope_redraw();
#endif
