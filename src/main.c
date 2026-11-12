
#include <pspkernel.h>
#include <pspdisplay.h>
#include <pspgu.h>
#include <pspgum.h>
#include <math.h>
#include <string.h>

PSP_MODULE_INFO("FC26 PSP Edition", 0, 1, 0);
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

static Vertex vertices[4096];
static int vertexCount = 0;

#define GREEN1 0xFF287A20
#define GREEN2 0xFF328C25
#define WHITE  0xFFFFFFFF
#define RED    0xFF2020E8
#define YELLOW 0xFF20D8FF
#define BLACK  0xFF181818

static void quad3D(
    float x1,float y1,float z1,
    float x2,float y2,float z2,
    float x3,float y3,float z3,
    float x4,float y4,float z4,
    unsigned int color)
{
    Vertex v[4] = {
        {color,x1,y1,z1},
        {color,x2,y2,z2},
        {color,x3,y3,z3},
        {color,x4,y4,z4}
    };

    sceGumDrawArray(
        GU_TRIANGLE_STRIP,
        GU_COLOR_8888 | GU_VERTEX_32BITF | GU_TRANSFORM_3D,
        4,0,v
    );
}

static void line3D(
    float x1,float y1,float z1,
    float x2,float y2,float z2,
    unsigned int color)
{
    Vertex v[2] = {
        {color,x1,y1,z1},
        {color,x2,y2,z2}
    };

    sceGumDrawArray(
        GU_LINES,
        GU_COLOR_8888 | GU_VERTEX_32BITF | GU_TRANSFORM_3D,
        2,0,v
    );
}

static void triangle3D(
    float x1,float y1,float z1,
    float x2,float y2,float z2,
    float x3,float y3,float z3,
    unsigned int color)
{
    Vertex v[3] = {
        {color,x1,y1,z1},
        {color,x2,y2,z2},
        {color,x3,y3,z3}
    };

    sceGumDrawArray(
        GU_TRIANGLES,
        GU_COLOR_8888 | GU_VERTEX_32BITF | GU_TRANSFORM_3D,
        3,0,v
    );
}

static void drawSideStand(int side)
{
    int i;
    float x = side * 12.0f;

    for(i=0;i<8;i++) {
        float y = 0.5f + i * 0.65f;
        float z1 = -17.5f - i * 0.35f;
        float z2 =  17.5f + i * 0.35f;

        unsigned int c = (i%2) ? 0xFF3030A0 : 0xFF4040C0;

        quad3D(
            x,y,z1,
            x,y,z2,
            x,y+0.55f,z1,
            x,y+0.55f,z2,c
        );
    }
}

static void drawEndStand(int side)
{
    int i;
    float z = side * 18.5f;

    for(i=0;i<8;i++) {
        float y = 0.5f + i * 0.65f;
        float x1 = -11.5f - i * 0.35f;
        float x2 =  11.5f + i * 0.35f;

        unsigned int c = (i%2) ? 0xFF3030A0 : 0xFF4040C0;

        quad3D(
            x1,y,z,
            x2,y,z,
            x1,y+0.55f,z,
            x2,y+0.55f,z,c
        );
    }
}

static void drawRoofs(void)
{
    quad3D(-13.0f,6.5f,-20.0f,
           13.0f,6.5f,-20.0f,
           -13.0f,6.5f,20.0f,
           13.0f,6.5f,20.0f,0xFF555555);

    quad3D(-13.0f,6.5f,-20.0f,
           -13.0f,6.5f,20.0f,
           -13.0f,6.5f,-15.0f,
           -13.0f,6.5f,15.0f,0xFF555555);

    quad3D(13.0f,6.5f,-20.0f,
           13.0f,6.5f,20.0f,
           13.0f,6.5f,-15.0f,
           13.0f,6.5f,15.0f,0xFF555555);
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
    int i;

    for(i=0;i<8;i++) {
        float z1 = -16.0f + i*4.0f;
        float z2 = z1 + 4.0f;
        unsigned int c = (i%2) ? GREEN1 : GREEN2;

        quad3D(-10.0f,0.02f,z1,
               10.0f,0.02f,z1,
               -10.0f,0.02f,z2,
               10.0f,0.02f,z2,c);
    }

    /* Pitch boundary */
    line3D(-10,0.06f,-16,10,0.06f,-16,WHITE);
    line3D(-10,0.06f,16,10,0.06f,16,WHITE);
    line3D(-10,0.06f,-16,-10,0.06f,16,WHITE);
    line3D(10,0.06f,-16,10,0.06f,16,WHITE);

    /* Halfway line */
    line3D(-10,0.06f,0,10,0.06f,0,WHITE);

    /* Centre circle */
    for(i=0;i<64;i++) {
        float a = i * 6.2831853f / 64.0f;
        float b = (i+1) * 6.2831853f / 64.0f;

        line3D(
            2.5f*cosf(a),0.06f,2.5f*sinf(a),
            2.5f*cosf(b),0.06f,2.5f*sinf(b),
            WHITE
        );
    }

    /* Penalty areas */
    line3D(-5.5f,0.06f,-16,5.5f,0.06f,-16,WHITE);
    line3D(-5.5f,0.06f,-16,-5.5f,0.06f,-10,WHITE);
    line3D(5.5f,0.06f,-16,5.5f,0.06f,-10,WHITE);
    line3D(-5.5f,0.06f,-10,5.5f,0.06f,-10,WHITE);

    line3D(-5.5f,0.06f,16,5.5f,0.06f,16,WHITE);
    line3D(-5.5f,0.06f,16,-5.5f,0.06f,10,WHITE);
    line3D(5.5f,0.06f,16,5.5f,0.06f,10,WHITE);
    line3D(-5.5f,0.06f,10,5.5f,0.06f,10,WHITE);

    /* Goal areas */
    line3D(-3.0f,0.07f,-16,3.0f,0.07f,-16,WHITE);
    line3D(-3.0f,0.07f,-16,-3.0f,0.07f,-13,WHITE);
    line3D(3.0f,0.07f,-16,3.0f,0.07f,-13,WHITE);
    line3D(-3.0f,0.07f,-13,3.0f,0.07f,-13,WHITE);

    line3D(-3.0f,0.07f,16,3.0f,0.07f,16,WHITE);
    line3D(-3.0f,0.07f,16,-3.0f,0.07f,13,WHITE);
    line3D(3.0f,0.07f,16,3.0f,0.07f,13,WHITE);
    line3D(-3.0f,0.07f,13,3.0f,0.07f,13,WHITE);
}

static void drawGoals(void)
{
    int i;

    /* Front goal frames */
    line3D(-3.66f,0.05f,-16,3.66f,0.05f,-16,WHITE);
    line3D(-3.66f,0.05f,-16,-3.66f,2.44f,-16,WHITE);
    line3D(3.66f,0.05f,-16,3.66f,2.44f,-16,WHITE);
    line3D(-3.66f,2.44f,-16,3.66f,2.44f,-16,WHITE);

    line3D(-3.66f,0.05f,16,3.66f,0.05f,16,WHITE);
    line3D(-3.66f,0.05f,16,-3.66f,2.44f,16,WHITE);
    line3D(3.66f,0.05f,16,3.66f,2.44f,16,WHITE);
    line3D(-3.66f,2.44f,16,3.66f,2.44f,16,WHITE);

    /* Goal net lines */
    for(i=0;i<=6;i++) {
        float x = -3.66f + i*1.22f;

        line3D(x,0.05f,-16,x,2.44f,-17.5f,WHITE);
        line3D(x,2.44f,-16,x,2.44f,-17.5f,WHITE);

        line3D(x,0.05f,16,x,0.05f,17.5f,WHITE);
        line3D(x,2.44f,16,x,2.44f,17.5f,WHITE);
    }

    for(i=0;i<=4;i++) {
        float y = i*0.61f;

        line3D(-3.66f,y,-16,3.66f,y,-16,WHITE);
        line3D(-3.66f,y,-17.5f,3.66f,y,-17.5f,WHITE);

        line3D(-3.66f,y,16,3.66f,y,16,WHITE);
        line3D(-3.66f,y,17.5f,3.66f,y,17.5f,WHITE);
    }
}

static void drawCornerFlags(void)
{
    int sx,sz;

    for(sx=-1;sx<=1;sx+=2) {
        for(sz=-1;sz<=1;sz+=2) {
            float x = sx*9.8f;
            float z = sz*15.8f;

            line3D(x,0.05f,z,x,1.8f,z,WHITE);

            triangle3D(
                x,1.8f,z,
                x+sx*0.8f,1.55f,z,
                x,1.25f,z,
                (sx==sz)?RED:YELLOW
            );
        }
    }
}

static void drawCornerArcs(void)
{
    int sx,sz,i;

    for(sx=-1;sx<=1;sx+=2) {
        for(sz=-1;sz<=1;sz+=2) {
            for(i=0;i<16;i++) {
                float a = i*1.5707963f/16.0f;
                float b = (i+1)*1.5707963f/16.0f;

                float x1 = sx*(10.0f-0.8f*cosf(a));
                float z1 = sz*(16.0f-0.8f*sinf(a));
                float x2 = sx*(10.0f-0.8f*cosf(b));
                float z2 = sz*(16.0f-0.8f*sinf(b));

                line3D(x1,0.07f,z1,x2,0.07f,z2,WHITE);
            }
        }
    }
}

static void drawAdvertisingBoards(void)
{
    int i;

    /* Boards along both touchlines */
    for(i=0;i<10;i++) {
        float z1 = -16.5f + i*3.3f;
        float z2 = z1 + 3.0f;

        quad3D(
            -11.0f,0.1f,z1,
            -11.0f,0.1f,z2,
            -11.0f,1.0f,z1,
            -11.0f,1.0f,z2,
            (i%2)?0xFF00A0E0:0xFFE05020
        );

        quad3D(
            11.0f,0.1f,z1,
            11.0f,0.1f,z2,
            11.0f,1.0f,z1,
            11.0f,1.0f,z2,
            (i%2)?0xFFE05020:0xFF00A0E0
        );
    }

    /* Boards behind both goals */
    for(i=0;i<7;i++) {
        float x1 = -10.0f + i*3.0f;
        float x2 = x1 + 2.7f;

        quad3D(
            x1,0.1f,-17.2f,
            x2,0.1f,-17.2f,
            x1,1.0f,-17.2f,
            x2,1.0f,-17.2f,
            (i%2)?0xFF00A0E0:0xFFE05020
        );

        quad3D(
            x1,0.1f,17.2f,
            x2,0.1f,17.2f,
            x1,1.0f,17.2f,
            x2,1.0f,17.2f,
            (i%2)?0xFFE05020:0xFF00A0E0
        );
    }
}

static void drawTechnicalAreas(void)
{
    int side;

    for(side=-1;side<=1;side+=2) {
        float x = side*12.5f;

        /* Technical area boundary */
        line3D(x,0.08f,-5.0f,x,0.08f,5.0f,WHITE);
        line3D(x,0.08f,-5.0f,x+side*1.0f,0.08f,-5.0f,WHITE);
        line3D(x,0.08f,5.0f,x+side*1.0f,0.08f,5.0f,WHITE);

        /* Team bench canopy */
        quad3D(
            x+side*0.2f,1.8f,-4.0f,
            x+side*0.2f,1.8f,4.0f,
            x+side*2.0f,1.8f,-4.0f,
            x+side*2.0f,1.8f,4.0f,
            0xFF505050
        );

        /* Bench supports */
        line3D(x+side*0.3f,0.1f,-3.8f,
               x+side*0.3f,1.8f,-3.8f,WHITE);
        line3D(x+side*0.3f,0.1f,3.8f,
               x+side*0.3f,1.8f,3.8f,WHITE);
        line3D(x+side*1.9f,0.1f,-3.8f,
               x+side*1.9f,1.8f,-3.8f,WHITE);
        line3D(x+side*1.9f,0.1f,3.8f,
               x+side*1.9f,1.8f,3.8f,WHITE);
    }
}

int main(void)
{
    ScePspFVector3 camera = {0.0f,27.0f,38.0f};
    ScePspFVector3 target = {0.0f,2.0f,0.0f};
    ScePspFVector3 up = {0.0f,1.0f,0.0f};

    sceGuInit();

    sceGuStart(GU_DIRECT,list);

    sceGuDrawBuffer(GU_PSM_8888,(void*)0,BW);
    sceGuDispBuffer(W,H,(void*)FRAME_SIZE,BW);
    sceGuDepthBuffer((void*)(2*FRAME_SIZE),BW);

    sceGuOffset(2048-W/2,2048-H/2);
    sceGuViewport(2048,2048,W,H);
    sceGuDepthRange(65535,0);
    sceGuScissor(0,0,W,H);

    sceGuEnable(GU_SCISSOR_TEST);
    sceGuDisable(GU_DEPTH_TEST);
    sceGuDisable(GU_CULL_FACE);

    sceGuClearColor(0xFF202020);
    sceGuClear(GU_COLOR_BUFFER_BIT);

    sceGuFinish();
    sceGuSync(0,0);

    sceDisplayWaitVblankStart();
    sceGuDisplay(GU_TRUE);

    for(;;) {
        sceGuStart(GU_DIRECT,list);

        sceGuClearColor(0xFF202020);
        sceGuClear(GU_COLOR_BUFFER_BIT);

        sceGumMatrixMode(GU_PROJECTION);
        sceGumLoadIdentity();
        sceGumPerspective(45.0f,(float)W/(float)H,1.0f,200.0f);

        sceGumMatrixMode(GU_VIEW);
        sceGumLoadIdentity();
        sceGumLookAt(&camera,&target,&up);

        sceGumMatrixMode(GU_MODEL);
        sceGumLoadIdentity();

        drawStadium();
        drawPitch();
        drawGoals();
        drawCornerFlags();
        drawCornerArcs();
        drawAdvertisingBoards();
        drawTechnicalAreas();

        sceGuFinish();
        sceGuSync(0,0);

        sceDisplayWaitVblankStart();
        sceGuSwapBuffers();
    }

    sceGuTerm();
    sceKernelExitGame();
    return 0;
}
