
#include <pspkernel.h>
#include <pspdisplay.h>
#include <pspgu.h>
#include <pspgum.h>

PSP_MODULE_INFO("FC26 3D Test", 0, 1, 0);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER);

#define W 480
#define H 272
#define BW 512
#define FRAME_SIZE (BW * H * 4)

static unsigned int __attribute__((aligned(16))) list[262144];

typedef struct {
    unsigned int color;
    float x, y, z;
} Vertex;

int main(void)
{
    sceGuInit();

    sceGuStart(GU_DIRECT, list);

    sceGuDrawBuffer(GU_PSM_8888, (void *)0, BW);
    sceGuDispBuffer(W, H, (void *)FRAME_SIZE, BW);
    sceGuDepthBuffer((void *)(FRAME_SIZE * 2), BW);

    sceGuOffset(2048 - W / 2, 2048 - H / 2);
    sceGuViewport(2048, 2048, W, H);
    sceGuScissor(0, 0, W, H);
    sceGuEnable(GU_SCISSOR_TEST);

    sceGuDisable(GU_DEPTH_TEST);
    sceGuDisable(GU_CULL_FACE);

    sceGuFinish();
    sceGuSync(GU_SYNC_FINISH, GU_SYNC_WHAT_DONE);

    sceDisplayWaitVblankStart();
    sceGuDisplay(GU_TRUE);

    ScePspFVector3 camera = {0.0f, 0.0f, 5.0f};
    ScePspFVector3 target = {0.0f, 0.0f, 0.0f};
    ScePspFVector3 up = {0.0f, 1.0f, 0.0f};

    while (1) {
        sceGuStart(GU_DIRECT, list);

        sceGuClearColor(GU_RGBA(20, 40, 90, 255));
        sceGuClear(GU_COLOR_BUFFER_BIT);

        sceGumMatrixMode(GU_PROJECTION);
        sceGumLoadIdentity();
        sceGumPerspective(
            60.0f, (float)W / H, 0.5f, 100.0f
        );

        sceGumMatrixMode(GU_VIEW);
        sceGumLoadIdentity();
        sceGumLookAt(&camera, &target, &up);

        sceGumMatrixMode(GU_MODEL);
        sceGumLoadIdentity();

        Vertex *v = sceGuGetMemory(3 * sizeof(Vertex));

        v[0] = (Vertex){GU_RGBA(255, 170, 20, 255), -1.5f, -1.0f, 0.0f};
        v[1] = (Vertex){GU_RGBA(255, 170, 20, 255),  1.5f, -1.0f, 0.0f};
        v[2] = (Vertex){GU_RGBA(255, 170, 20, 255),  0.0f,  1.5f, 0.0f};

        sceGumDrawArray(
            GU_TRIANGLES,
            GU_COLOR_8888 | GU_VERTEX_32BITF | GU_TRANSFORM_3D,
            3, 0, v
        );

        sceGuFinish();
        sceGuSync(GU_SYNC_FINISH, GU_SYNC_WHAT_DONE);

        sceDisplayWaitVblankStart();
        sceGuSwapBuffers();
    }

    return 0;
}
