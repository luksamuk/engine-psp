#ifndef ENGINE_GRAPHICS_H
#define ENGINE_GRAPHICS_H

#define BUFFER_WIDTH  512
#define BUFFER_HEIGHT 272
#define SCREEN_WIDTH  480
#define SCREEN_HEIGHT BUFFER_HEIGHT
#define DESIRED_FPS 60

#define FPSCONVERT(x, dt) (x * dt * DESIRED_FPS)

void initGu(void);
void disposeGu(void);
void guRenderStart(void);
void guRenderEnd(void);

void drawRect(float x, float y, float w, float h);

#endif
