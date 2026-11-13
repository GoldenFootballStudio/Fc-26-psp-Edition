
#include <pspkernel.h>
#include <pspdisplay.h>
#include <pspgu.h>
#include <pspgum.h>
#include <pspctrl.h>
#include <math.h>

PSP_MODULE_INFO("FC26 PSP Football", 0, 1, 0);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER);

#define W 480
#define H 272
#define BW 512
#define FRAME_SIZE (BW * H * 4)
#define WHITE 0xFFFFFFFF
#define BLUE  0xFF2020D8
#define RED   0xFFD02020
#define PI 3.14159265f

static unsigned int __attribute__((aligned(16))) list[262144];
static unsigned int frameCount;

typedef struct {
    unsigned int color;
    float x,y,z;
} Vertex;

typedef struct {
    float x,z,hx,hz;
} Player;

static Player blue[11], red[11];

static float ballX, ballZ, ballVX, ballVZ;

static int owner=0;
static int moving=0;
static int controlled=0;
static int passTarget=-1;
static int blueScore=0;
static int redScore=0;
static int restartTimer=0;

static unsigned int oldButtons=0;

static float clampf(float v,float a,float b) {
    return v<a?a:(v>b?b:v);
}

static float dist2(float x1,float z1,float x2,float z2) {
    float x=x1-x2;
    float z=z1-z2;
    return x*x+z*z;
}

static void quad(
    float x1,float y1,float z1,
    float x2,float y2,float z2,
    float x3,float y3,float z3,
    float x4,float y4,float z4,
    unsigned int c
) {
    Vertex v[4]={
        {c,x1,y1,z1},
        {c,x2,y2,z2},
        {c,x3,y3,z3},
        {c,x4,y4,z4}
    };

    sceGumDrawArray(
        GU_TRIANGLE_STRIP,
        GU_COLOR_8888|GU_VERTEX_32BITF|GU_TRANSFORM_3D,
        4,0,v
    );
}

static void line(
    float x1,float y1,float z1,
    float x2,float y2,float z2,
    unsigned int c
) {
    Vertex v[2]={
        {c,x1,y1,z1},
        {c,x2,y2,z2}
    };

    sceGumDrawArray(
        GU_LINES,
        GU_COLOR_8888|GU_VERTEX_32BITF|GU_TRANSFORM_3D,
        2,0,v
    );
}

static void triangle(
    float x1,float y1,float z1,
    float x2,float y2,float z2,
    float x3,float y3,float z3,
    unsigned int c
) {
    Vertex v[3]={
        {c,x1,y1,z1},
        {c,x2,y2,z2},
        {c,x3,y3,z3}
    };

    sceGumDrawArray(
        GU_TRIANGLES,
        GU_COLOR_8888|GU_VERTEX_32BITF|GU_TRANSFORM_3D,
        3,0,v
    );
}

/* STADIUM */

static void drawPitch(void) {
    int i;

    for(i=0;i<8;i++) {
        float z=-16+i*4;

        quad(
            -10,.02f,z,
            10,.02f,z,
            -10,.02f,z+4,
            10,.02f,z+4,
            (i&1)?0xFF287A20:0xFF328C25
        );
    }

    line(-10,.06f,-16,10,.06f,-16,WHITE);
    line(-10,.06f,16,10,.06f,16,WHITE);
    line(-10,.06f,-16,-10,.06f,16,WHITE);
    line(10,.06f,-16,10,.06f,16,WHITE);
    line(-10,.06f,0,10,.06f,0,WHITE);

    for(i=0;i<64;i++) {
        float a=i*2*PI/64;
        float b=(i+1)*2*PI/64;

        line(
            2.5f*cosf(a),.06f,2.5f*sinf(a),
            2.5f*cosf(b),.06f,2.5f*sinf(b),
            WHITE
        );
    }

    line(-5.5f,.06f,-16,5.5f,.06f,-16,WHITE);
    line(-5.5f,.06f,-16,-5.5f,.06f,-10,WHITE);
    line(5.5f,.06f,-16,5.5f,.06f,-10,WHITE);
    line(-5.5f,.06f,-10,5.5f,.06f,-10,WHITE);

    line(-5.5f,.06f,16,5.5f,.06f,16,WHITE);
    line(-5.5f,.06f,16,-5.5f,.06f,10,WHITE);
    line(5.5f,.06f,16,5.5f,.06f,10,WHITE);
    line(-5.5f,.06f,10,5.5f,.06f,10,WHITE);
}

static void drawStadium(void) {
    int s,r,n;

    for(s=-1;s<=1;s+=2) {
        for(r=0;r<8;r++) {
            float a=11.3f+r*.8f;
            float b=a+.8f;
            float y=.45f+r*.55f;

            quad(
                s*a,y,-20,
                s*b,y,-20,
                s*a,y,20,
                s*b,y,20,
                (r&1)?0xFF3030B0:0xFF404080
            );
        }
    }

    for(s=-1;s<=1;s+=2) {
        for(r=0;r<7;r++) {
            float a=17.4f+r*.75f;
            float b=a+.75f;
            float y=.45f+r*.55f;

            quad(
                -12,y,s*a,
                12,y,s*a,
                -12,y,s*b,
                12,y,s*b,
                (r&1)?0xFF3030B0:0xFF404080
            );
        }
    }

    for(s=-1;s<=1;s+=2) {
        for(r=0;r<7;r++) {
            for(n=0;n<22;n++) {
                float z=-18+n*1.65f;
                float x=s*(11.7f+r*.8f);
                float y=.65f+r*.55f;
                float bob=.05f*sinf(frameCount*.04f+n+r);

                unsigned int c[5]={
                    0xFFE03030,
                    0xFF3030D0,
                    WHITE,
                    0xFF20A040,
                    0xFF20A0E0
                };

                quad(
                    x,y+bob,z,
                    x,y+bob,z+.3f,
                    x,y+.3f+bob,z,
                    x,y+.3f+bob,z+.3f,
                    c[(n+r)%5]
                );
            }
        }
    }

    for(s=-1;s<=1;s+=2) {
        for(r=-1;r<=1;r+=2) {
            float x=s*14.5f;
            float z=r*19.5f;

            line(x,0,z,x,9,z,0xFFAAAAAA);

            quad(
                x-.65f,9.2f,z-.3f,
                x+.65f,9.2f,z-.3f,
                x-.65f,9.8f,z+.3f,
                x+.65f,9.8f,z+.3f,
                0xFFFFFFDD
            );
        }
    }

    for(s=-1;s<=1;s+=2) {
        float z=s*16;

        line(-3.66f,.05f,z,3.66f,.05f,z,WHITE);
        line(-3.66f,.05f,z,-3.66f,2.44f,z,WHITE);
        line(3.66f,.05f,z,3.66f,2.44f,z,WHITE);
        line(-3.66f,2.44f,z,3.66f,2.44f,z,WHITE);

        for(n=0;n<=6;n++) {
            float x=-3.66f+n*1.22f;

            line(
                x,.05f,z,
                x,2.44f,z+s*1.5f,
                WHITE
            );
        }

        for(n=0;n<=4;n++) {
            line(
                -3.66f,n*.61f,z,
                3.66f,n*.61f,z,
                WHITE
            );
        }
    }

    for(s=-1;s<=1;s+=2) {
        for(r=-1;r<=1;r+=2) {
            float x=s*9.8f;
            float z=r*15.8f;

            line(x,.05f,z,x,1.8f,z,WHITE);

            triangle(
                x,1.8f,z,
                x+s*.8f,1.55f,z,
                x,1.25f,z,
                0xFF2020E8
            );
        }
    }

    for(n=0;n<10;n++) {
        float z=-16+n*3.2f;
        unsigned int c=(n&1)?0xFF00A0E0:0xFFE05020;

        quad(
            -10.8f,.1f,z,
            -10.8f,.1f,z+2.8f,
            -10.8f,1,z,
            -10.8f,1,z+2.8f,
            c
        );

        quad(
            10.8f,.1f,z,
            10.8f,.1f,z+2.8f,
            10.8f,1,z,
            10.8f,1,z+2.8f,
            c
        );
    }
}

/* PLAYERS */

static void drawPlayer(
    float x,
    float z,
    unsigned int kit,
    int selected
) {
    float b=.035f*sinf(frameCount*.18f);

    line(x-.22f,.12f,z,x-.25f,.78f+b,z,0xFF101010);
    line(x+.22f,.12f,z,x+.25f,.78f+b,z,0xFF101010);

    line(x-.25f,.78f+b,z,x-.18f,1.12f+b,z,0xFF202020);
    line(x+.25f,.78f+b,z,x+.18f,1.12f+b,z,0xFF202020);

    quad(
        x-.34f,1.05f+b,z,
        x+.34f,1.05f+b,z,
        x-.29f,1.95f+b,z,
        x+.29f,1.95f+b,z,
        kit
    );

    line(x-.30f,1.82f+b,z,x-.48f,1.18f+b,z,0xFFB87945);
    line(x+.30f,1.82f+b,z,x+.48f,1.18f+b,z,0xFFB87945);

    quad(
        x-.18f,1.95f+b,z,
        x+.18f,1.95f+b,z,
        x-.18f,2.28f+b,z,
        x+.18f,2.28f+b,z,
        0xFFB87945
    );

    if(selected) {
        line(
            x-.65f,2.5f+b,z,
            x+.65f,2.5f+b,z,
            0xFFFFFF00
        );
    }
}

static void drawBall(void) {
    float y=.18f+.035f*sinf(frameCount*.25f);

    triangle(
        ballX,y+.16f,ballZ,
        ballX-.16f,y,ballZ,
        ballX,y,ballZ-.16f,
        WHITE
    );

    triangle(
        ballX,y+.16f,ballZ,
        ballX,y,ballZ-.16f,
        ballX+.16f,y,ballZ,
        WHITE
    );

    triangle(
        ballX,y+.16f,ballZ,
        ballX+.16f,y,ballZ,
        ballX,y,ballZ+.16f,
        0xFF202020
    );

    triangle(
        ballX,y+.16f,ballZ,
        ballX,y,ballZ+.16f,
        ballX-.16f,y,ballZ,
        WHITE
    );
}

static void drawScoreboard(void) {
    quad(
        -5.5f,9,-22,
        5.5f,9,-22,
        -5.5f,12.5f,-22,
        5.5f,12.5f,-22,
        0xFF101010
    );

    quad(
        -5.1f,9.5f,-21.9f,
        -2.5f,9.5f,-21.9f,
        -5.1f,12,-21.9f,
        -2.5f,12,-21.9f,
        BLUE
    );

    quad(
        2.5f,9.5f,-21.9f,
        5.1f,9.5f,-21.9f,
        2.5f,12,-21.9f,
        5.1f,12,-21.9f,
        WHITE
    );

    line(
        -1.7f,10.8f,-21.8f,
        -1.7f+(blueScore%5)*.35f,10.8f,-21.8f,
        WHITE
    );

    line(
        .7f,10.8f,-21.8f,
        .7f+(redScore%5)*.35f,10.8f,-21.8f,
        RED
    );
}

/* FORMATIONS */

static void setupTeams(void) {
    int i;

    float bx[11]={
        0,0,-7,-2.3f,2.3f,7,
        -5,5,-6,0,6
    };

    float bz[11]={
        2,14.3f,10,11,11,10,
        5,5,-3,-5,-3
    };

    float rz[11]={
        -14.3f,-10,-11,-11,-10,
        -4,-3,-4,2,4,2
    };

    for(i=0;i<11;i++) {
        blue[i].hx=bx[i];
        blue[i].hz=bz[i];
        blue[i].x=bx[i];
        blue[i].z=bz[i];

        red[i].hx=bx[i];
        red[i].hz=rz[i];
        red[i].x=bx[i];
        red[i].z=rz[i];
    }

    controlled=0;
    owner=0;
    moving=0;
    passTarget=-1;

    ballX=blue[0].x;
    ballZ=blue[0].z-.4f;
    ballVX=ballVZ=0;
}

static void giveBlue(int i) {
    owner=i;
    moving=0;
    passTarget=-1;
    ballVX=ballVZ=0;
    controlled=i;

    ballX=blue[i].x;
    ballZ=blue[i].z-.4f;
}

static void kickoff(int blueStarts) {
    int i;

    for(i=0;i<11;i++) {
        blue[i].x=blue[i].hx;
        blue[i].z=blue[i].hz;

        red[i].x=red[i].hx;
        red[i].z=red[i].hz;
    }

    owner=blueStarts?0:11;
    moving=0;
    passTarget=-1;

    ballVX=ballVZ=0;
    ballX=0;
    ballZ=0;

    controlled=0;
    restartTimer=45;
}

static int closestBlue(float x,float z) {
    int i,best=0;
    float bd=99999;

    for(i=0;i<11;i++) {
        float d=dist2(blue[i].x,blue[i].z,x,z);

        if(d<bd) {
            bd=d;
            best=i;
        }
    }

    return best;
}

static int closestRed(float x,float z) {
    int i,best=1;
    float bd=99999;

    for(i=1;i<11;i++) {
        float d=dist2(red[i].x,red[i].z,x,z);

        if(d<bd) {
            bd=d;
            best=i;
        }
    }

    return best;
}

static int findPassTarget(int from,int through) {
    int i,best=-1;
    float scoreBest=99999;

    for(i=0;i<11;i++) {
        if(i!=from) {
            float dx=blue[i].x-blue[from].x;
            float dz=blue[i].z-blue[from].z;
            float d=sqrtf(dx*dx+dz*dz);
            float score=d;

            if(d<1.0f || d>17.0f) continue;

            if(dz>=0) score+=5.0f;
            if(through && dz<0) score-=4.0f;

            if(score<scoreBest) {
                scoreBest=score;
                best=i;
            }
        }
    }

    return best;
}

static void passBall(int from,int target,float power) {
    float dx=blue[target].x-blue[from].x;
    float dz=blue[target].z-blue[from].z;
    float d=sqrtf(dx*dx+dz*dz);

    if(d<.1f) return;

    ballX=blue[from].x;
    ballZ=blue[from].z-.35f;

    ballVX=dx/d*power;
    ballVZ=dz/d*power;

    owner=-1;
    moving=1;
    passTarget=target;
}

/* TEAM AI */

static void updateAI(void) {
    int i;

    float bx=ballX;
    float bz=ballZ;

    for(i=0;i<11;i++) {
        float tx=blue[i].hx+clampf(bx*.20f,-2.2f,2.2f);
        float tz=blue[i].hz+clampf(bz*.22f,-3.5f,3.5f);

        if(i==controlled) continue;

        if(owner>=0 && owner<11) {
            if(i==8 || i==9 || i==10) tz-=2.0f;
            if(i==6 || i==7) tz-=1.0f;
        }

        blue[i].x+=(clampf(tx,-9,9)-blue[i].x)*.025f;
        blue[i].z+=(clampf(tz,-14.5f,14.5f)-blue[i].z)*.025f;
    }

    for(i=1;i<11;i++) {
        float tx=red[i].hx;
        float tz=red[i].hz;

        if(owner>=0 && owner<11) {
            if(i==closestRed(bx,bz)) {
                tx=bx;
                tz=bz;
            } else {
                tx+=clampf(bx*.10f,-1.5f,1.5f);
                tz+=clampf(bz*.12f,-2,2);
            }
        } else if(owner>=11) {
            if(i==closestRed(bx,bz)) {
                tx=bx;
                tz=bz;
            }
        } else {
            if(i==closestRed(bx,bz)) {
                tx=bx;
                tz=bz;
            }
        }

        red[i].x+=(clampf(tx,-9,9)-red[i].x)*.025f;
        red[i].z+=(clampf(tz,-14.5f,14.5f)-red[i].z)*.025f;
    }

    red[0].x+=(clampf(bx,-2.5f,2.5f)-red[0].x)*.035f;
    red[0].z+=(-14.2f-red[0].z)*.035f;

    blue[1].x+=(clampf(bx,-2.5f,2.5f)-blue[1].x)*.025f;
    blue[1].z+=(14.2f-blue[1].z)*.025f;
}

/* BALL PHYSICS */

static void updateBall(void) {
    int i;

    if(owner>=0 && owner<11 && !moving) {
        ballX=blue[owner].x;
        ballZ=blue[owner].z-.4f;
    }
    else if(owner>=11 && !moving) {
        int r=owner-11;

        ballX=red[r].x;
        ballZ=red[r].z+.4f;
    }

    if(moving) {
        ballX+=ballVX;
        ballZ+=ballVZ;

        ballVX*=.992f;
        ballVZ*=.992f;

        if(passTarget>=0 &&
           dist2(ballX,ballZ,blue[passTarget].x,blue[passTarget].z)<.8f) {
            giveBlue(passTarget);
        }
        else if(passTarget<0) {
            int b=closestBlue(ballX,ballZ);
            int r=closestRed(ballX,ballZ);

            if(dist2(ballX,ballZ,blue[b].x,blue[b].z)<.45f) {
                giveBlue(b);
            }
            else if(dist2(ballX,ballZ,red[r].x,red[r].z)<.45f) {
                owner=11+r;
                moving=0;
                ballVX=ballVZ=0;
                passTarget=-1;
            }
        }

        if(fabsf(ballVX)+fabsf(ballVZ)<.012f && moving) {
            moving=0;
            passTarget=-1;
            ballVX=ballVZ=0;
        }
    }

    /* GOALS */

    if(ballZ < -16.25f && fabsf(ballX)<3.66f) {
        blueScore++;
        kickoff(0);
    }
    else if(ballZ > 16.25f && fabsf(ballX)<3.66f) {
        redScore++;
        kickoff(1);
    }

    /* BALL OUT OF PLAY */

    else if(fabsf(ballX)>10.0f || fabsf(ballZ)>17.0f) {
        ballX=clampf(ballX,-9.5f,9.5f);
        ballZ=clampf(ballZ,-15.5f,15.5f);

        moving=0;
        passTarget=-1;
        ballVX=ballVZ=0;
        owner=-1;
    }

    if(owner==-1 && !moving) {
        int b=closestBlue(ballX,ballZ);
        int r=closestRed(ballX,ballZ);

        if(dist2(ballX,ballZ,blue[b].x,blue[b].z)<.55f) {
            giveBlue(b);
        }
        else if(dist2(ballX,ballZ,red[r].x,red[r].z)<.55f) {
            owner=11+r;
            ballX=red[r].x;
            ballZ=red[r].z+.4f;
        }
    }
}

/* CONTROLS */

static void updateMatch(void) {
    SceCtrlData pad;
    unsigned int pressed;

    float speed=.12f;
    float ax,ay;

    int i;

    sceCtrlPeekBufferPositive(&pad,1);

    pressed=pad.Buttons & ~oldButtons;

    if(restartTimer>0) restartTimer--;

    if(restartTimer==0) {
        if(owner>=0 && owner<11) {
            Player *p=&blue[controlled];

            if(pad.Buttons&PSP_CTRL_RTRIGGER)
                speed=.20f;

            if(pad.Buttons&PSP_CTRL_LEFT) p->x-=speed;
            if(pad.Buttons&PSP_CTRL_RIGHT) p->x+=speed;
            if(pad.Buttons&PSP_CTRL_UP) p->z-=speed;
            if(pad.Buttons&PSP_CTRL_DOWN) p->z+=speed;

            ax=((int)pad.Lx-128)/127.0f;
            ay=((int)pad.Ly-128)/127.0f;

            if(fabsf(ax)>.18f) p->x+=ax*speed;
            if(fabsf(ay)>.18f) p->z+=ay*speed;

            p->x=clampf(p->x,-9,9);
            p->z=clampf(p->z,-14.5f,14.5f);
        }

        if((pressed&PSP_CTRL_LTRIGGER) &&
           owner>=0 && owner<11) {
            float best=99999;
            int pick=controlled;

            for(i=0;i<11;i++) {
                if(i!=controlled) {
                    float d=dist2(
                        blue[i].x,blue[i].z,
                        ballX,ballZ
                    );

                    if(d<best) {
                        best=d;
                        pick=i;
                    }
                }
            }

            controlled=pick;
        }

        if(owner>=0 && owner<11 &&
           owner==controlled && !moving) {

            if(pressed&PSP_CTRL_CROSS) {
                int t=findPassTarget(controlled,0);

                if(t>=0) passBall(controlled,t,.34f);
            }

            if(pressed&PSP_CTRL_CIRCLE) {
                int t=findPassTarget(controlled,0);

                if(t>=0) passBall(controlled,t,.48f);
            }

            if(pressed&PSP_CTRL_TRIANGLE) {
                int t=findPassTarget(controlled,1);

                if(t>=0) passBall(controlled,t,.42f);
            }

            if(pressed&PSP_CTRL_SQUARE) {
                ballX=blue[controlled].x;
                ballZ=blue[controlled].z-.35f;

                ballVX=0;
                ballVZ=-.62f;

                owner=-1;
                moving=1;
                passTarget=-1;
            }
        }
    }

    updateAI();
    updateBall();

    oldButtons=pad.Buttons;
}

/* MAIN GAME */

int main(void) {
    ScePspFVector3 camera={0,29,44};
    ScePspFVector3 target={0,2,0};
    ScePspFVector3 up={0,1,0};

    int i;

    setupTeams();

    sceCtrlSetSamplingCycle(0);
    sceCtrlSetSamplingMode(PSP_CTRL_MODE_ANALOG);

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
    sceGuDisable(GU_CULL_FACE);

    sceGuEnable(GU_DEPTH_TEST);
    sceGuDepthFunc(GU_GEQUAL);

    sceGuClearColor(0xFF202020);
    sceGuClearDepth(0);

    sceGuClear(
        GU_COLOR_BUFFER_BIT |
        GU_DEPTH_BUFFER_BIT
    );

    sceGuFinish();
    sceGuSync(0,0);

    sceDisplayWaitVblankStart();
    sceGuDisplay(GU_TRUE);

    for(;;) {
        updateMatch();

        sceGuStart(GU_DIRECT,list);

        sceGuClearColor(0xFF202020);
        sceGuClearDepth(0);

        sceGuClear(
            GU_COLOR_BUFFER_BIT |
            GU_DEPTH_BUFFER_BIT
        );

        sceGuEnable(GU_DEPTH_TEST);

        sceGumMatrixMode(GU_PROJECTION);
        sceGumLoadIdentity();

        sceGumPerspective(
            48.0f,
            (float)W/H,
            1.0f,
            200.0f
        );

        sceGumMatrixMode(GU_VIEW);
        sceGumLoadIdentity();

        sceGumLookAt(&camera,&target,&up);

        sceGumMatrixMode(GU_MODEL);
        sceGumLoadIdentity();

        drawStadium();
        drawPitch();

        for(i=0;i<11;i++) {
            drawPlayer(
                blue[i].x,
                blue[i].z,
                BLUE,
                i==controlled
            );
        }

        for(i=0;i<11;i++) {
            drawPlayer(
                red[i].x,
                red[i].z,
                i==0?0xFF20A020:RED,
                0
            );
        }

        drawBall();
        drawScoreboard();

        sceGuFinish();
        sceGuSync(0,0);

        sceDisplayWaitVblankStart();
        sceGuSwapBuffers();

        frameCount++;
    }

    sceGuTerm();
    sceKernelExitGame();

    return 0;
}
