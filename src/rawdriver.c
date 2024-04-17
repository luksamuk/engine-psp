#include <pspkernel.h>
#include <pspdebug.h>
#include <pspctrl.h>
#include <pspgu.h>
#include <pspdisplay.h>
#include <pspaudio.h>
#include <pspmp3.h>
#include <psputility.h>
#include <stdio.h>
#include <flecs.h>

PSP_MODULE_INFO("Luksamuk PSP Engine", 0, 1, 0);
PSP_MAIN_THREAD_ATTR(PSP_THREAD_ATTR_VFPU | PSP_THREAD_ATTR_USER);
PSP_HEAP_SIZE_KB(-1024);

#define BUFFER_WIDTH  512
#define BUFFER_HEIGHT 272
#define SCREEN_WIDTH  480
#define SCREEN_HEIGHT BUFFER_HEIGHT
#define MUSIC_PATH "assets/walkin.mp3"
#define DESIRED_FPS 60

#define FPSCONVERT(x, dt) (x * dt * DESIRED_FPS)

char      list[0x20000]        __attribute__((aligned(64)));
char      mp3Buf[16*1024]      __attribute__((aligned(64)));
SceUChar8 pcmBuf[16*(1152/2)]  __attribute__((aligned(64)));

typedef struct {
    float x, y;
} vec2;

typedef struct {
    unsigned short u, v;
    short x, y, z;
} vertex;

typedef vec2 Position;
typedef vec2 Velocity;

static int RUNNING = 1;

static int press_up = 0;
static int press_down = 0;
static int press_left = 0;
static int press_right = 0;

const float speed = 0.4f;
const float decel = speed * 0.6f;
const float max_speed = 6.0f;
const float bounce_back = 2.0f;



#define printf  pspDebugScreenPrintf
#define ERRORMSG(...) { char msg[128]; sprintf(msg,__VA_ARGS__); error(msg); }
void error( char* msg )
{
    SceCtrlData pad;
    pspDebugScreenClear();
    pspDebugScreenSetXY(0, 0);
    printf(msg);
    printf("Press X to quit.\n");
    while(RUNNING) {
        sceCtrlReadBufferPositive(&pad, 1);
        if (pad.Buttons & PSP_CTRL_CROSS)
            break;
        sceDisplayWaitVblankStart();
    }
    sceKernelExitGame();
}


int  callback_thread(SceSize, void *);
int  exit_cb(int, int, void *);
void drawRect(float, float, float, float);
int  fillAudioStreamBuffer(int, int);
void MoveBody(ecs_iter_t *);
void ControlVelocity(ecs_iter_t *);
void RenderBody(ecs_iter_t *);

// Entrypoint
int
main(void)
{
    pspDebugScreenInit();
    pspDebugScreenClear();
    
    // Setup callbacks
    int cb_tid = sceKernelCreateThread("callback_thread", callback_thread, 0x11, 0xFA0, 0, 0);
    if(cb_tid >= 0) sceKernelStartThread(cb_tid, 0, 0);

    // Load modules
    int status = sceUtilityLoadModule(PSP_MODULE_AV_AVCODEC);
    if(status < 0) {
        ERRORMSG("ERROR: sceUtilityLoadModule(PSP_MODULE_AV_AVCODEC) returned 0x%08X\n", status);
    }

    status = sceUtilityLoadModule(PSP_MODULE_AV_MP3);
    if(status < 0) {
        ERRORMSG("ERROR: sceUtilityLoadModule(PSP_MODULE_AV_MP3) returned 0x%08X\n", status);
    }

    // Open audio file
    int fd = sceIoOpen(MUSIC_PATH, PSP_O_RDONLY, 0777);
    if(fd < 0) {
        ERRORMSG("ERROR: Could not open file '%s' - 0x%08X\n", MUSIC_PATH, fd);
    }

    // Init MP3 resources
    status = sceMp3InitResource();
    if(status < 0) {
        ERRORMSG("ERROR: sceMp3InitResource returned 0x%08X\n", status);
    }

    // Reserve an MP3 handle for playback
    SceMp3InitArg mp3Init;
    mp3Init.mp3StreamStart = 0;
    mp3Init.mp3StreamEnd = sceIoLseek32(fd, 0, SEEK_END);
    mp3Init.mp3Buf = mp3Buf;
    mp3Init.mp3BufSize = sizeof(mp3Buf);
    mp3Init.pcmBuf = pcmBuf;
    mp3Init.pcmBufSize = sizeof(pcmBuf);

    int handle = sceMp3ReserveMp3Handle(&mp3Init);
    if(handle < 0) {
        ERRORMSG("ERROR: sceMp3ReserveMp3Handle returned 0x%08X\n", handle);
    }

    // Fill stream buffer with some data for a start
    fillAudioStreamBuffer(fd, handle);
    status = sceMp3Init(handle);
    if(status < 0) {
        ERRORMSG("ERROR: sceMp3Init returned 0x%08X\n", status);
    }

    /* Playback variables */
    int channel = -1;
    int samplingRate = sceMp3GetSamplingRate(handle);
    int numChannels = sceMp3GetMp3ChannelNum(handle);
    int lastDecoded = 0;
    int volume = PSP_AUDIO_VOLUME_MAX;
    int numPlayed = 0;
    //int paused = 0;

    sceMp3SetLoopNum(handle, -1);
    sceMp3ResetPlayPosition(handle);
    
    // Initialize GU
    sceGuInit();
    sceGuStart(GU_DIRECT, list);
    sceGuDrawBuffer(GU_PSM_8888, (void*)0, BUFFER_WIDTH);
    sceGuDispBuffer(SCREEN_WIDTH, SCREEN_HEIGHT, (void*)0x88000, BUFFER_WIDTH);
    sceGuDepthBuffer((void*)0x110000, BUFFER_WIDTH);
    sceGuOffset(2048 - (SCREEN_WIDTH / 2), 2048 - (SCREEN_HEIGHT / 2));
    sceGuViewport(2048, 2048, SCREEN_WIDTH, SCREEN_HEIGHT);
    sceGuEnable(GU_SCISSOR_TEST);
    sceGuScissor(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
    sceGuDepthRange(65535, 0);
    sceGuDepthFunc(GU_GEQUAL);
    sceGuEnable(GU_DEPTH_TEST);
    sceGuFinish();
    sceGuDisplay(GU_TRUE);

    // Initialize controller
    SceCtrlData pad;
    sceCtrlSetSamplingCycle(0);
    sceCtrlSetSamplingMode(PSP_CTRL_MODE_ANALOG);

    // Initialize ECS
    ecs_world_t *world = ecs_init();
    ECS_COMPONENT(world, Position);
    ECS_COMPONENT(world, Velocity);
    ECS_SYSTEM(world, MoveBody, EcsOnUpdate, Position, Velocity);
    ECS_SYSTEM(world, ControlVelocity, EcsOnUpdate, Velocity);
    ECS_SYSTEM(world, RenderBody, EcsPostUpdate, Position);

    ecs_entity_t e = ecs_new_id(world);
    ecs_set(world, e, Position, {32, 32});
    ecs_set(world, e, Velocity, {0, 0});

    while(RUNNING) {
        /* Update audio */
        // Get data if needed
        if(sceMp3CheckStreamDataNeeded(handle) > 0) {
            fillAudioStreamBuffer(fd, handle);
        }

        // Decode audio samples
        short *buf;
        int bytesDecoded;
        int retries = 0;
        for(; retries < 1; retries++) {
            bytesDecoded = sceMp3Decode(handle, &buf);
            
            if(bytesDecoded > 0)
                break;

            if(sceMp3CheckStreamDataNeeded(handle) <= 0)
                break;

            if(!fillAudioStreamBuffer(fd, handle)) {
                numPlayed = 0;
            }
        }

        if(bytesDecoded < 0 && bytesDecoded != 0x80671103) {
            ERRORMSG("ERROR: sceMp3Decode returned 0x%08X\n", bytesDecoded);
        }

        if(bytesDecoded == 0 || (bytesDecoded == 0x80671103)) {
            //paused = 1;
            sceMp3ResetPlayPosition(handle);
            numPlayed = 0;
        } else {
            // Reserve audio channel for our output if not yet done
            if(channel < 0 || lastDecoded != bytesDecoded) {
                if(channel >= 0) sceAudioSRCChRelease();
                channel = sceAudioSRCChReserve(bytesDecoded / (2 * numChannels), samplingRate, numChannels);
            }

            // Output decoded samples and accumulate the number of played samples
            // to get the playtime
            numPlayed += sceAudioSRCOutputBlocking(volume, buf);
        }
        

        /* Controls */
        // Read controls
        sceCtrlReadBufferPositive(&pad, 1);
        press_up = (pad.Buttons & PSP_CTRL_UP);
        press_down = (pad.Buttons & PSP_CTRL_DOWN);
        press_left = (pad.Buttons & PSP_CTRL_LEFT);
        press_right = (pad.Buttons & PSP_CTRL_RIGHT);

        if(pad.Buttons & PSP_CTRL_CIRCLE) {
            RUNNING = 0;
        }

        /* Rendering */
        sceGuStart(GU_DIRECT, list);
        sceGuClearColor(0xff000000); // ABGR
        sceGuClear(GU_COLOR_BUFFER_BIT);

        ecs_progress(world, 0);

        sceGuFinish();
        sceGuSync(0, 0);
        sceDisplayWaitVblankStart();
        sceGuSwapBuffers();
    }

    // Cleanup
    if(channel >= 0) sceAudioSRCChRelease();
    status = sceMp3ReleaseMp3Handle(handle);
    status = sceMp3TermResource();
    status = sceIoClose(fd);

    sceGuDisplay(GU_FALSE);
    sceGuTerm();

    sceKernelExitGame();
    return 0;
}

// =========

int
callback_thread(SceSize args, void *argp)
{
    // Install exit callback
    int cbid = sceKernelCreateCallback("Exit Callback", exit_cb, NULL);
    sceKernelRegisterExitCallback(cbid);
    sceKernelSleepThreadCB();
    return 0;
}

int
exit_cb(int arg1, int arg2, void *common)
{
    RUNNING = 0;
    return 0;
}

void
drawRect(float x, float y, float w, float h)
{
    // This is not permanent memory allocation. The memory will be invalid
    // as soon as the same display list starts being filled again, so no
    // need to deallocate stuff here
    vertex *vertices = (vertex*)sceGuGetMemory(2 * sizeof(vertex));

    vertices[0].x = x;
    vertices[0].y = y;
    vertices[1].x = x + w;
    vertices[1].y = y + h;

    sceGuColor(0xff0000ff); // red (ABGR)
    sceGuDrawArray(GU_SPRITES, GU_TEXTURE_16BIT | GU_VERTEX_16BIT | GU_TRANSFORM_2D, 2, 0, vertices);
}

int
fillAudioStreamBuffer(int fd, int handle)
{
    SceUChar8 *dst;
    SceInt32 write;
    SceInt32 pos;

    // Get stream info (where to fill, how much to fill, where to fill from)
    int status = sceMp3GetInfoToAddStreamData(handle, &dst, &write, &pos);
    if(status < 0) {
        ERRORMSG("ERROR: sceMp3GetInfoToAddStreamData returned 0x%08X\n", status);
    }

    // Seek file to requested position
    status = sceIoLseek32(fd, pos, SEEK_SET);
    if(status < 0) {
        ERRORMSG("ERROR: sceIoLseek32 returned 0x%08X\n", status);
    }

    // Read data
    int amount_read = sceIoRead(fd, dst, write);
    if(amount_read < 0) {
        ERRORMSG("ERROR: Could not read from file - 0x%08X\n", amount_read);
    }

    if(amount_read == 0) return 0; // EOF

    // Notify MP3 library about how much we really wrote to the stream buffer
    status = sceMp3NotifyAddStreamData(handle, amount_read);
    if(status < 0) {
        ERRORMSG("ERROR: sceMp3NotifyAddStreamData returned 0x%08X\n", status);
    }

    return (pos > 0);
}


void
MoveBody(ecs_iter_t *it)
{
    Position *p = ecs_field(it, Position, 1);
    Velocity *v = ecs_field(it, Velocity, 2);

    int i;
    for(i = 0; i < it->count; i++) {
        p[i].x += v[i].x;
        p[i].y += v[i].y;

        if(p[i].x > SCREEN_WIDTH - 32) {
            p[i].x = SCREEN_WIDTH - 32;
            v[i].x = -bounce_back;
        }

        if(p[i].x < 0.0f) {
            p[i].x = 0.0f;
            v[i].x = bounce_back;
        }

        if(p[i].y > SCREEN_HEIGHT - 32) {
            p[i].y = SCREEN_HEIGHT - 32;
            v[i].y = -bounce_back;
        }

        if(p[i].y < 0.0f) {
            p[i].y = 0.0f;
            v[i].y = bounce_back;
        }
    }
}

void
ControlVelocity(ecs_iter_t *it)
{
    Velocity *v = ecs_field(it, Velocity, 1);

    float speed_dt = FPSCONVERT(speed, it->delta_time);
    float decel_dt = FPSCONVERT(decel, it->delta_time);

    int i;
    for(i = 0; i < it->count; i++) {
        if(press_up) v[i].y -= speed_dt;
        if(press_down) v[i].y += speed_dt;
        if(press_left) v[i].x -= speed_dt;
        if(press_right) v[i].x += speed_dt;

        if(!press_left && !press_right) {
            if(v[i].x > decel) v[i].x -= decel_dt;
            else if(v[i].x < -decel) v[i].x += decel_dt;
            else v[i].x = 0.0f;
        }

        if(!press_up && !press_down) {
            if(v[i].y > decel) v[i].y -= decel_dt;
            else if(v[i].y < -decel) v[i].y += decel_dt;
            else v[i].y = 0.0f;
        }


        if(v[i].x > max_speed) v[i].x = max_speed;
        if(v[i].x < -max_speed) v[i].x = -max_speed;

        if(v[i].y > max_speed) v[i].y = max_speed;
        if(v[i].y < -max_speed) v[i].y = -max_speed;
    }
}

void
RenderBody(ecs_iter_t *it)
{
    Position *p = ecs_field(it, Position, 1);

    int i;
    for(i = 0; i < it->count; i++) {
        drawRect(p[i].x, p[i].y, 32, 32);
    }
}

