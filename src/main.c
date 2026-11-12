
#include <pspkernel.h>
#include <pspdisplay.h>
#include <pspgu.h>
#include <pspgum.h>

PSP_MODULE_INFO("FC26 Stadium Upgrade", 0, 1, 0);
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

static void quad3D(
    float x1, float y1, float z1,
    float x2, float y2, float z2,
    float x3, float y3, float z3,
    float x4, float y4, float z4,
    unsigned int color)
{
    Vertex *v = sceGuGetMemory(4 * sizeof(Vertex));

    v[0] = (Vertex){color, x1, y1, z1};
    v[1] = (Vertex){color, x2, y2, z2};
    v[2] = (Vertex){color, x3, y3, z3};
    v[3] = (Vertex){color, x4, y4, z4};

    sceGumDrawArray(
        GU_TRIANGLE_STRIP,
        GU_COLOR_8888 | GU_VERTEX_32BITF | GU_TRANSFORM_3D,
        4, 0, v
    );
}

static void line3D(
    float x1, float y1, float z1,
    float x2, float y2, float z2,
    unsigned int color)
{
    Vertex *v = sceGuGetMemory(2 * sizeof(Vertex));

    v[0] = (Vertex){color, x1, y1, z1};
    v[1] = (Vertex){color, x2, y2, z2};

    sceGumDrawArray(
        GU_LINES,
        GU_COLOR_8888 | GU_VERTEX_32BITF | GU_TRANSFORM_3D,
        2, 0, v
    );
}

static void drawSideStand(int side)
{
    unsigned int seats[] = {
        GU_RGBA(35,45,105,255),
        GU_RGBA(180,35,35,255),
        GU_RGBA(220,190,35,255),
        GU_RGBA(35,105,65,255)
    };

    for (int i = 0; i < 8; i++) {
        float inner = 10.8f + i * 0.8f;
        float outer = inner + 0.9f;
        float bottom = 0.35f + i * 0.62f;
        float top = bottom + 0.55f;

        float x1 = side * inner;
        float x2 = side * outer;

        quad3D(
            x1, bottom, -19.5f,
            x2, top, -19.5f,
            x1, bottom, 19.5f,
            x2, top, 19.5f,
            seats[i % 4]
        );
    }
}

static void drawEndStand(int side)
{
    unsigned int seats[] = {
        GU_RGBA(35,45,105,255),
        GU_RGBA(180,35,35,255),
        GU_RGBA(220,190,35,255),
        GU_RGBA(35,105,65,255)
    };

    for (int i = 0; i < 8; i++) {
        float inner = 16.8f + i * 0.8f;
        float outer = inner + 0.9f;
        float bottom = 0.35f + i * 0.62f;
        float top = bottom + 0.55f;

        float z1 = side * inner;
        float z2 = side * outer;

        quad3D(
            -12.0f, bottom, z1,
            12.0f, bottom, z1,
            -12.0f, top, z2,
            12.0f, top, z2,
            seats[i % 4]
        );
    }
}

static void drawRoofs(void)
{
    unsigned int roof = GU_RGBA(45,52,68,255);

    quad3D(
        -14.0f, 7.0f, -21.0f,
        -18.0f, 8.0f, -21.0f,
        -14.0f, 7.0f, 21.0f,
        -18.0f, 8.0f, 21.0f,
        roof
    );

    quad3D(
        14.0f, 7.0f, -21.0f,
        18.0f, 8.0f, -21.0f,
        14.0f, 7.0f, 21.0f,
        18.0f, 8.0f, 21.0f,
        roof
    );

    quad3D(
        -13.0f, 7.0f, -22.0f,
        13.0f, 7.0f, -22.0f,
        -13.0f, 8.0f, -26.0f,
        13.0f, 8.0f, -26.0f,
        roof
    );

    quad3D(
        -13.0f, 7.0f, 22.0f,
        13.0f, 7.0f, 22.0f,
        -13.0f, 8.0f, 26.0f,
        13.0f, 8.0f, 26.0f,
        roof
    );
}

static void drawStadium(void)
{
    drawEndStand(-1);
    drawEndStand(1);

    drawSideStand(-1);
    drawSideStand(1);

    drawRoofs();
}

static void drawPitch(void)
{
    unsigned int grass1 = GU_RGBA(30,115,42,255);
    unsigned int grass2 = GU_RGBA(43,145,55,255);
    unsigned int white = GU_RGBA(255,255,255,255);

    for (int i = 0; i < 8; i++) {
        float z1 = -16.0f + i * 4.0f;
        float z2 = z1 + 4.0f;

        quad3D(
            -10, 0, z1,
            10, 0, z1,
            -10, 0, z2,
            10, 0, z2,
            (i % 2) ? grass1 : grass2
        );
    }

    // Pitch boundary
    line3D(-10,0.04f,-16, 10,0.04f,-16,white);
    line3D(-10,0.04f,16, 10,0.04f,16,white);
    line3D(-10,0.04f,-16,-10,0.04f,16,white);
    line3D(10,0.04f,-16,10,0.04f,16,white);

    // Halfway line
    line3D(-10,0.04f,0,10,0.04f,0,white);

    // Centre circle
    for (int i = 0; i < 64; i++) {
        float a = i * 6.2831853f / 64.0f;
        float b = (i + 1) * 6.2831853f / 64.0f;

        line3D(
            2.2f * __builtin_cosf(a), 0.04f,
            2.2f * __builtin_sinf(a),
            2.2f * __builtin_cosf(b), 0.04f,
            2.2f * __builtin_sinf(b),
            white
        );
    }

    // Penalty areas
    line3D(-7,0.04f,-16,-7,0.04f,-12,white);
    line3D(7,0.04f,-16,7,0.04f,-12,white);
    line3D(-7,0.04f,-12,7,0.04f,-12,white);

    line3D(-7,0.04f,16,-7,0.04f,12,white);
    line3D(7,0.04f,16,7,0.04f,12,white);
    line3D(-7,0.04f,12,7,0.04f,12,white);

    // Goal areas
    line3D(-3,0.04f,-16,-3,0.04f,-14,white);
    line3D(3,0.04f,-16,3,0.04f,-14,white);
    line3D(-3,0.04f,-14,3,0.04f,-14,white);

    line3D(-3,0.04f,16,-3,0.04f,14,white);
    line3D(3,0.04f,16,3,0.04f,14,white);
    line3D(-3,0.04f,14,3,0.04f,14,white);
}

static void drawGoals(void)
{
    unsigned int white = GU_RGBA(245,245,245,255);
    unsigned int net = GU_RGBA(210,220,225,255);

    // Goal frames
    for (int side = -1; side <= 1; side += 2) {
        float z = side * 16.1f;
        float back = side * 18.4f;

        line3D(-3,0.05f,z, -3,2.2f,z,white);
        line3D(3,0.05f,z, 3,2.2f,z,white);
        line3D(-3,2.2f,z, 3,2.2f,z,white);

        // Net roof and back
        line3D(-3,2.2f,z, -3,2.0f,back,net);
        line3D(3,2.2f,z, 3,2.0f,back,net);
        line3D(-3,0.05f,z, -3,0.05f,back,net);
        line3D(3,0.05f,z, 3,0.05f,back,net);
        line3D(-3,2.0f,back, 3,2.0f,back,net);
        line3D(-3,0.05f,back, 3,0.05f,back,net);

        // Net mesh
        for (int i = 1; i < 6; i++) {
            float x = -3.0f + i;
            line3D(x,0.05f,z, x,2.1f,back,net);
        }

        for (int i = 1; i < 4; i++) {
            float y = i * 0.5f;
            line3D(-3,y,back,3,y,back,net);
        }
    }
}

int main(void)
{
    sceGuInit();

    sceGuStart(GU_DIRECT, list);

    sceGuDrawBuffer(GU_PSM_8888, (void *)0, BW);
    sceGuDispBuffer(W,H,(void *)FRAME_SIZE,BW);
    sceGuDepthBuffer((void *)(FRAME_SIZE * 2),BW);

    sceGuOffset(2048-W/2,2048-H/2);
    sceGuViewport(2048,2048,W,H);
    sceGuScissor(0,0,W,H);
    sceGuEnable(GU_SCISSOR_TEST);

    sceGuDisable(GU_DEPTH_TEST);
    sceGuDisable(GU_CULL_FACE);

    sceGuFinish();
    sceGuSync(GU_SYNC_FINISH,GU_SYNC_WHAT_DONE);

    sceDisplayWaitVblankStart();
    sceGuDisplay(GU_TRUE);

    ScePspFVector3 camera = {0.0f,27.0f,38.0f};
    ScePspFVector3 target = {0.0f,2.0f,0.0f};
    ScePspFVector3 up = {0.0f,1.0f,0.0f};

    while (1) {
        sceGuStart(GU_DIRECT,list);

        sceGuClearColor(GU_RGBA(18,30,48,255));
        sceGuClear(GU_COLOR_BUFFER_BIT);

        sceGumMatrixMode(GU_PROJECTION);
        sceGumLoadIdentity();
        sceGumPerspective(48.0f,(float)W/H,0.5f,100.0f);

        sceGumMatrixMode(GU_VIEW);
        sceGumLoadIdentity();
        sceGumLookAt(&camera,&target,&up);

        sceGumMatrixMode(GU_MODEL);
        sceGumLoadIdentity();

        drawStadium();
        drawPitch();
        drawGoals();

        sceGuFinish();
        sceGuSync(GU_SYNC_FINISH,GU_SYNC_WHAT_DONE);

        sceDisplayWaitVblankStart();
        sceGuSwapBuffers();
    }

    return 0;
}
