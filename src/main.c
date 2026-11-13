
#include <pspkernel.h>
#include <pspdisplay.h>
#include <pspgu.h>
#include <pspgum.h>
#include <pspctrl.h>
#include <math.h>
#include <stdio.h>

PSP_MODULE_INFO("FC26 PSP Edition",0,1,0);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER);

#define W 480
#define H 272
#define BW 512
#define FRAME_SIZE (BW*H*4)

static unsigned int __attribute__((aligned(16))) list[262144];
static unsigned int clockFrames=0;

typedef struct {
    unsigned int color;
    float x,y,z;
} Vertex;

static void quad(float x1,float y1,float z1,
                 float x2,float y2,float z2,
                 float x3,float y3,float z3,
                 float x4,float y4,float z4,
                 unsigned int c)
{
    Vertex v[4]={{c,x1,y1,z1},{c,x2,y2,z2},
                 {c,x3,y3,z3},{c,x4,y4,z4}};
    sceGumDrawArray(GU_TRIANGLE_STRIP,
        GU_COLOR_8888|GU_VERTEX_32BITF|GU_TRANSFORM_3D,
        4,0,v);
}

static void line(float x1,float y1,float z1,
                 float x2,float y2,float z2,unsigned int c)
{
    Vertex v[2]={{c,x1,y1,z1},{c,x2,y2,z2}};
    sceGumDrawArray(GU_LINES,
        GU_COLOR_8888|GU_VERTEX_32BITF|GU_TRANSFORM_3D,
        2,0,v);
}

static void triangle(float x1,float y1,float z1,
                     float x2,float y2,float z2,
                     float x3,float y3,float z3,unsigned int c)
{
    Vertex v[3]={{c,x1,y1,z1},{c,x2,y2,z2},{c,x3,y3,z3}};
    sceGumDrawArray(GU_TRIANGLES,
        GU_COLOR_8888|GU_VERTEX_32BITF|GU_TRANSFORM_3D,
        3,0,v);
}

/* STANDS */

static void drawStands(void)
{
    int s,i;
    for(s=-1;s<=1;s+=2) {
        float x=s*12.0f,z=s*18.5f;
        for(i=0;i<8;i++) {
            float y=.5f+i*.65f;
            unsigned int c=(i%2)?0xFF3030A0:0xFF4040C0;

            quad(x,y,-17.5f-i*.35f,
                 x,y,17.5f+i*.35f,
                 x,y+.55f,-17.5f-i*.35f,
                 x,y+.55f,17.5f+i*.35f,c);

            quad(-11.5f-i*.35f,y,z,
                 11.5f+i*.35f,y,z,
                 -11.5f-i*.35f,y+.55f,z,
                 11.5f+i*.35f,y+.55f,z,c);
        }
    }

    quad(-13,6.5f,-20,13,6.5f,-20,
         -13,6.5f,20,13,6.5f,20,0xFF555555);
    quad(-13,6.5f,-20,-13,6.5f,20,
         -13,6.5f,-15,-13,6.5f,15,0xFF555555);
    quad(13,6.5f,-20,13,6.5f,20,
         13,6.5f,-15,13,6.5f,15,0xFF555555);
}

/* CROWD */

static void drawCrowd(unsigned int t)
{
    int s,r,n;
    unsigned int colors[6]={
        0xFFE03030,0xFF3030D0,0xFFFFFFFF,
        0xFF20A040,0xFF20A0E0,0xFF303030
    };

    for(s=-1;s<=1;s+=2) {
        for(r=0;r<7;r++) {
            for(n=0;n<25;n++) {
                float z=-16.5f+n*1.35f;
                float y=.85f+r*.65f;
                float b=.07f*sinf(t*.004f+n*.8f+r);
                unsigned int c=colors[(n+r*3)%6];

                quad(s*12.12f,y+b,z,
                     s*12.12f,y+b,z+.48f,
                     s*12.12f,y+.35f+b,z,
                     s*12.12f,y+.35f+b,z+.48f,c);

                quad(s*12.12f,y+.35f+b,z+.12f,
                     s*12.12f,y+.35f+b,z+.34f,
                     s*12.12f,y+.52f+b,z+.12f,
                     s*12.12f,y+.52f+b,z+.34f,0xFFB08060);
            }
        }

        for(r=0;r<7;r++) {
            for(n=0;n<17;n++) {
                float x=-10.5f+n*1.3f;
                float y=.85f+r*.65f;
                float b=.07f*sinf(t*.004f+n+r);
                float z=s*18.62f;
                unsigned int c=colors[(n*2+r)%6];

                quad(x,y+b,z,x+.42f,y+b,z,
                     x,y+.35f+b,z,x+.42f,y+.35f+b,z,c);
                quad(x+.1f,y+.35f+b,z,x+.32f,y+.35f+b,z,
                     x+.1f,y+.52f+b,z,x+.32f,y+.52f+b,z,
                     0xFFB08060);
            }
        }
    }
}

/* FLOODLIGHTS */

static void drawLights(void)
{
    int s;
    for(s=-1;s<=1;s+=2) {
        float x=s*14.0f;
        line(x,0,-18,x,9,-18,0xFFAAAAAA);
        line(x,0,18,x,9,18,0xFFAAAAAA);

        quad(x-.8f,9,-19,x+.8f,9,-19,
             x-.8f,9.6f,-19,x+.8f,9.6f,-19,0xFFFFFFDD);
        quad(x-.8f,9,17,x+.8f,9,17,
             x-.8f,9.6f,17,x+.8f,9.6f,17,0xFFFFFFDD);
    }
}

/* PITCH */

static void drawPitch(void)
{
    int i;
    for(i=0;i<8;i++) {
        float z=-16+i*4;
        unsigned int c=(i%2)?0xFF287A20:0xFF328C25;
        quad(-10,.02f,z,10,.02f,z,-10,.02f,z+4,10,.02f,z+4,c);
    }

    line(-10,.06f,-16,10,.06f,-16,0xFFFFFFFF);
    line(-10,.06f,16,10,.06f,16,0xFFFFFFFF);
    line(-10,.06f,-16,-10,.06f,16,0xFFFFFFFF);
    line(10,.06f,-16,10,.06f,16,0xFFFFFFFF);
    line(-10,.06f,0,10,.06f,0,0xFFFFFFFF);

    for(i=0;i<64;i++) {
        float a=i*6.2831853f/64;
        float b=(i+1)*6.2831853f/64;
        line(2.5f*cosf(a),.06f,2.5f*sinf(a),
             2.5f*cosf(b),.06f,2.5f*sinf(b),0xFFFFFFFF);
    }

    line(-5.5f,.06f,-16,5.5f,.06f,-16,0xFFFFFFFF);
    line(-5.5f,.06f,-16,-5.5f,.06f,-10,0xFFFFFFFF);
    line(5.5f,.06f,-16,5.5f,.06f,-10,0xFFFFFFFF);
    line(-5.5f,.06f,-10,5.5f,.06f,-10,0xFFFFFFFF);

    line(-5.5f,.06f,16,5.5f,.06f,16,0xFFFFFFFF);
    line(-5.5f,.06f,16,-5.5f,.06f,10,0xFFFFFFFF);
    line(5.5f,.06f,16,5.5f,.06f,10,0xFFFFFFFF);
    line(-5.5f,.06f,10,5.5f,.06f,10,0xFFFFFFFF);

    line(-3,.07f,-16,3,.07f,-16,0xFFFFFFFF);
    line(-3,.07f,-16,-3,.07f,-13,0xFFFFFFFF);
    line(3,.07f,-16,3,.07f,-13,0xFFFFFFFF);
    line(-3,.07f,-13,3,.07f,-13,0xFFFFFFFF);

    line(-3,.07f,16,3,.07f,16,0xFFFFFFFF);
    line(-3,.07f,16,-3,.07f,13,0xFFFFFFFF);
    line(3,.07f,16,3,.07f,13,0xFFFFFFFF);
    line(-3,.07f,13,3,.07f,13,0xFFFFFFFF);
}

/* GOALS */

static void drawGoals(void)
{
    int i;
    for(i=-1;i<=1;i+=2) {
        float z=i*16.0f;
        line(-3.66f,.05f,z,3.66f,.05f,z,WHITE);
        line(-3.66f,.05f,z,-3.66f,2.44f,z,WHITE);
        line(3.66f,.05f,z,3.66f,2.44f,z,WHITE);
        line(-3.66f,2.44f,z,3.66f,2.44f,z,WHITE);

        for(int n=0;n<=6;n++) {
            float x=-3.66f+n*1.22f;
            line(x,.05f,z,x,2.44f,z+i*1.5f,WHITE);
        }
        for(int n=0;n<=4;n++) {
            float y=n*.61f;
            line(-3.66f,y,z,3.66f,y,z,WHITE);
        }
    }
}

/* CORNER FLAGS AND ARCS */

static void drawCorners(void)
{
    int sx,sz,i;
    for(sx=-1;sx<=1;sx+=2) {
        for(sz=-1;sz<=1;sz+=2) {
            float x=sx*9.8f,z=sz*15.8f;
            line(x,.05f,z,x,1.8f,z,WHITE);
            triangle(x,1.8f,z,x+sx*.8f,1.55f,z,
                     x,1.25f,z,0xFF2020E8);

            for(i=0;i<16;i++) {
                float a=i*1.5707963f/16;
                float b=(i+1)*1.5707963f/16;
                line(sx*(10-.8f*cosf(a)),.07f,
                     sz*(16-.8f*sinf(a)),
                     sx*(10-.8f*cosf(b)),.07f,
                     sz*(16-.8f*sinf(b)),WHITE);
            }
        }
    }
}

/* ADVERTISING BOARDS */

static void drawBoards(void)
{
    int i;
    for(i=0;i<10;i++) {
        float z=-16.5f+i*3.3f;
        unsigned int c=(i%2)?0xFF00A0E0:0xFFE05020;

        quad(-11,.1f,z,-11,.1f,z+3,
             -11,1,z,-11,1,z+3,c);
        quad(11,.1f,z,11,.1f,z+3,
             11,1,z,11,1,z+3,c);
    }

    for(i=0;i<7;i++) {
        float x=-10+i*3;
        unsigned int c=(i%2)?0xFF00A0E0:0xFFE05020;
        quad(x,.1f,-17.2f,x+2.7f,.1f,-17.2f,
             x,1,-17.2f,x+2.7f,1,-17.2f,c);
        quad(x,.1f,17.2f,x+2.7f,.1f,17.2f,
             x,1,17.2f,x+2.7f,1,17.2f,c);
    }
}

/* TECHNICAL AREAS */

static void drawTechnical(void)
{
    int s;
    for(s=-1;s<=1;s+=2) {
        float x=s*12.5f;
        line(x,.08f,-5,x,.08f,5,WHITE);
        line(x,.08f,-5,x+s,-.0f,-5,WHITE);
        line(x,.08f,5,x+s,.08f,5,WHITE);

        quad(x+s*.2f,1.8f,-4,x+s*.2f,1.8f,4,
             x+s*2,1.8f,-4,x+s*2,1.8f,4,0xFF505050);

        line(x+s*.3f,.1f,-3.8f,x+s*.3f,1.8f,-3.8f,WHITE);
        line(x+s*.3f,.1f,3.8f,x+s*.3f,1.8f,3.8f,WHITE);
        line(x+s*1.9f,.1f,-3.8f,x+s*1.9f,1.8f,-3.8f,WHITE);
        line(x+s*1.9f,.1f,3.8f,x+s*1.9f,1.8f,3.8f,WHITE);
    }
}

/* 2D SCOREBOARD */

static void rect2D(int x,int y,int w,int h,unsigned int c)
{
    sceGuDisable(GU_DEPTH_TEST);
    sceGuDisable(GU_TEXTURE_2D);
    sceGuDrawBuffer(GU_PSM_8888,(void*)0,BW);

    sceGuDrawArray(GU_SPRITES,
        GU_COLOR_8888|GU_VERTEX_16BIT|GU_TRANSFORM_2D,
        2,0,0);
}

/* Simple 2D pixel rectangles */
static void pixelRect(int x,int y,int w,int h,unsigned int c)
{
    Vertex v[4]={
        {c,(float)x,(float)y,0},
        {c,(float)(x+w),(float)y,0},
        {c,(float)x,(float)(y+h),0},
        {c,(float)(x+w),(float)(y+h),0}
    };
    sceGumDrawArray(GU_TRIANGLE_STRIP,
        GU_COLOR_8888|GU_VERTEX_32BITF|GU_TRANSFORM_2D,
        4,0,v);
}

/* Tiny 3x5 font for scoreboard text */
static const unsigned char font[11][5]={
    {7,5,5,5,7}, /* 0 */
    {2,6,2,2,7}, /* 1 */
    {7,1,7,4,7}, /* 2 */
    {7,1,7,1,7}, /* 3 */
    {5,5,7,1,1}, /* 4 */
    {7,4,7,1,7}, /* 5 */
    {7,4,7,5,7}, /* 6 */
    {7,1,1,1,1}, /* 7 */
    {7,5,7,5,7}, /* 8 */
    {7,5,7,1,7}, /* 9 */
    {0,2,0,2,0}  /* : */
};

static void drawChar(int x,int y,int ch,int scale,unsigned int c)
{
    int r,col,index;
    if(ch==':') index=10;
    else if(ch>='0'&&ch<='9') index=ch-'0';
    else return;

    for(r=0;r<5;r++) {
        for(col=0;col<3;col++) {
            if(font[index][r]&(1<<(2-col))) {
                Vertex v[4]={
                    {c,x+col*scale,y+r*scale,0},
                    {c,x+(col+1)*scale,y+r*scale,0},
                    {c,x+col*scale,y+(r+1)*scale,0},
                    {c,x+(col+1)*scale,y+(r+1)*scale,0}
                };
                sceGumDrawArray(GU_TRIANGLE_STRIP,
                    GU_COLOR_8888|GU_VERTEX_32BITF|GU_TRANSFORM_2D,
                    4,0,v);
            }
        }
    }
}

static void drawScoreboard(unsigned int seconds)
{
    char clockText[8];
    int minutes=(seconds/60)%100;
    int secs=seconds%60;

    sprintf(clockText,"%02d:%02d",minutes,secs);

    sceGuDisable(GU_DEPTH_TEST);
    sceGumMatrixMode(GU_PROJECTION);
    sceGumLoadIdentity();
    sceGumOrtho(0,W,H,0,-1,1);

    sceGumMatrixMode(GU_VIEW);
    sceGumLoadIdentity();
    sceGumMatrixMode(GU_MODEL);
    sceGumLoadIdentity();

    /* Main dark broadcast panel */
    pixelRect(130,8,220,36,0xE0101010);
    pixelRect(130,8,4,36,0xFF2020E8);
    pixelRect(346,8,4,36,0xFFFFFFFF);

    /* Team colour panels */
    pixelRect(139,14,53,24,0xFF2020E8);
    pixelRect(288,14,53,24,0xFFFFFFFF);

    /* Score */
    drawChar(204,15,0,4,WHITE);
    drawChar(221,15,0,4,WHITE);
    drawChar(238,15,0,4,WHITE);

    /* Timer panel */
    pixelRect(195,49,90,19,0xDD101010);
    drawChar(214,55,clockText[0],2,WHITE);
    drawChar(222,55,clockText[1],2,WHITE);
    drawChar(230,55,':',2,WHITE);
    drawChar(238,55,clockText[3],2,WHITE);
    drawChar(246,55,clockText[4],2,WHITE);

    sceGumUpdateMatrix();
}

/* MAIN */

int main(void)
{
    ScePspFVector3 camera={0.0f,30.0f,43.0f};
    ScePspFVector3 target={0.0f,2.0f,0.0f};
    ScePspFVector3 up={0.0f,1.0f,0.0f};

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
        SceCtrlData pad;
        sceCtrlReadBufferPositive(&pad,1);

        sceGuStart(GU_DIRECT,list);
        sceGuClearColor(0xFF202020);
        sceGuClear(GU_COLOR_BUFFER_BIT);

        sceGuEnable(GU_DEPTH_TEST);
        sceGumMatrixMode(GU_PROJECTION);
        sceGumLoadIdentity();
        sceGumPerspective(45.0f,(float)W/H,1.0f,200.0f);

        sceGumMatrixMode(GU_VIEW);
        sceGumLoadIdentity();
        sceGumLookAt(&camera,&target,&up);

        sceGumMatrixMode(GU_MODEL);
        sceGumLoadIdentity();

        drawStands();
        drawCrowd(clockFrames);
        drawLights();
        drawPitch();
        drawGoals();
        drawCorners();
        drawBoards();
        drawTechnical();

        sceGuFinish();
        sceGuSync(0,0);

        sceGuStart(GU_DIRECT,list);
        sceGuDisable(GU_DEPTH_TEST);

        clockFrames++;
        drawScoreboard(clockFrames/60);

        sceGuFinish();
        sceGuSync(0,0);
        sceDisplayWaitVblankStart();
        sceGuSwapBuffers();
    }

    sceGuTerm();
    sceKernelExitGame();
    return 0;
}
