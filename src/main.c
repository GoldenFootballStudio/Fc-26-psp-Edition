
#include <pspkernel.h>
#include <pspdisplay.h>
#include <pspgu.h>
#include <pspgum.h>
#include <pspctrl.h>
#include <math.h>
#include <stdlib.h>

PSP_MODULE_INFO("FC26 PSP EDITION", 0, 1, 0);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER | THREAD_ATTR_VFPU);

#define W 480
#define H 272
#define PI 3.14159265f
#define BLUE 0
#define RED 1

typedef struct {
    float x, y;
    float hx, hy;
} Player;

typedef struct {
    float x, y;
    float vx, vy;
} Ball;

static unsigned int __attribute__((aligned(16))) list[262144];

static Player blue[11];
static Player red[11];
static Ball ball;

static int controlled = 9;
static int owner = 9;
static int passTarget = -1;
static int scoreBlue = 0;
static int scoreRed = 0;
static int matchTime = 0;
static int gameFrames = 0;
static int tackleCooldown = 0;
static int secondPress = 0;
static int lastButtons = 0;

static int clampInt(int v, int lo, int hi)
{
    if(v < lo) return lo;
    if(v > hi) return hi;
    return v;
}

static float clampFloat(float v, float lo, float hi)
{
    if(v < lo) return lo;
    if(v > hi) return hi;
    return v;
}

static float distance(float x1, float y1, float x2, float y2)
{
    float dx = x2 - x1;
    float dy = y2 - y1;
    return sqrtf(dx * dx + dy * dy);
}

static Player *getPlayer(int id)
{
    if(id < 11)
        return &blue[id];

    return &red[id - 11];
}

static float screenX(float x)
{
    return 240.0f + x * 10.5f;
}

static float screenY(float y)
{
    return 136.0f + y * 7.0f;
}

static void drawRect(float x, float y, float w, float h,
                     unsigned int color)
{
    struct Vertex {
        unsigned int color;
        short x, y, z;
    };

    Vertex *v = (Vertex *)sceGuGetMemory(sizeof(Vertex) * 4);

    v[0].color = color;
    v[0].x = (short)x;
    v[0].y = (short)y;
    v[0].z = 0;

    v[1].color = color;
    v[1].x = (short)(x + w);
    v[1].y = (short)y;
    v[1].z = 0;

    v[2].color = color;
    v[2].x = (short)x;
    v[2].y = (short)(y + h);
    v[2].z = 0;

    v[3].color = color;
    v[3].x = (short)(x + w);
    v[3].y = (short)(y + h);
    v[3].z = 0;

    sceGuDrawArray(GU_TRIANGLE_STRIP,
        GU_COLOR_8888 | GU_VERTEX_16BIT | GU_TRANSFORM_2D,
        4, 0, v);
}

static void drawCircle(float x, float y, float radius,
                       unsigned int color)
{
    struct Vertex {
        unsigned int color;
        short x, y, z;
    };

    int i;
    Vertex *v = (Vertex *)sceGuGetMemory(sizeof(Vertex) * 22);

    v[0].color = color;
    v[0].x = (short)x;
    v[0].y = (short)y;
    v[0].z = 0;

    for(i = 0; i <= 20; i++)
    {
        float a = (float)i * 2.0f * PI / 20.0f;

        v[i + 1].color = color;
        v[i + 1].x = (short)(x + cosf(a) * radius);
        v[i + 1].y = (short)(y + sinf(a) * radius);
        v[i + 1].z = 0;
    }

    sceGuDrawArray(GU_TRIANGLE_FAN,
        GU_COLOR_8888 | GU_VERTEX_16BIT | GU_TRANSFORM_2D,
        22, 0, v);
}

static void drawPitch(void)
{
    unsigned int grass = GU_RGBA(30, 115, 48, 255);
    unsigned int stripe = GU_RGBA(38, 130, 55, 255);
    unsigned int white = GU_RGBA(255, 255, 255, 255);

    int i;

    drawRect(0, 0, W, H, GU_RGBA(12, 25, 35, 255));

    /* Stadium seating */
    drawRect(0, 0, W, 35, GU_RGBA(45, 48, 65, 255));
    drawRect(0, 237, W, 35, GU_RGBA(45, 48, 65, 255));
    drawRect(0, 35, 27, 202, GU_RGBA(45, 48, 65, 255));
    drawRect(453, 35, 27, 202, GU_RGBA(45, 48, 65, 255));

    for(i = 0; i < 18; i++)
    {
        drawRect(4 + i * 27, 7, 18, 4,
                 GU_RGBA(160, 160, 175, 255));
        drawRect(4 + i * 27, 20, 18, 4,
                 GU_RGBA(160, 160, 175, 255));
        drawRect(4 + i * 27, 244, 18, 4,
                 GU_RGBA(160, 160, 175, 255));
        drawRect(4 + i * 27, 258, 18, 4,
                 GU_RGBA(160, 160, 175, 255));
    }

    /* Field */
    drawRect(27, 35, 426, 202, grass);

    for(i = 0; i < 6; i++)
    {
        if(i % 2 == 0)
            drawRect(27 + i * 71, 35, 71, 202, stripe);
    }

    /* Field boundaries */
    drawRect(27, 35, 426, 2, white);
    drawRect(27, 235, 426, 2, white);
    drawRect(27, 35, 2, 202, white);
    drawRect(451, 35, 2, 202, white);

    /* Halfway line */
    drawRect(239, 35, 2, 202, white);

    /* Centre circle */
    drawCircle(240, 136, 42, white);
    drawCircle(240, 136, 40, grass);
    drawCircle(240, 136, 3, white);

    /* Penalty boxes */
    drawRect(27, 91, 54, 2, white);
    drawRect(27, 181, 54, 2, white);
    drawRect(79, 91, 2, 92, white);

    drawRect(399, 91, 54, 2, white);
    drawRect(399, 181, 54, 2, white);
    drawRect(399, 91, 2, 92, white);

    /* Six-yard boxes */
    drawRect(27, 112, 22, 2, white);
    drawRect(27, 160, 22, 2, white);
    drawRect(47, 112, 2, 50, white);

    drawRect(431, 112, 22, 2, white);
    drawRect(431, 160, 22, 2, white);
    drawRect(431, 112, 2, 50, white);

    /* Goals */
    drawRect(10, 116, 17, 40, GU_RGBA(230, 230, 230, 255));
    drawRect(453, 116, 17, 40, GU_RGBA(230, 230, 230, 255));

    /* Corner flags */
    drawCircle(29, 37, 3, GU_RGBA(255, 220, 0, 255));
    drawCircle(451, 37, 3, GU_RGBA(255, 220, 0, 255));
    drawCircle(29, 235, 3, GU_RGBA(255, 220, 0, 255));
    drawCircle(451, 235, 3, GU_RGBA(255, 220, 0, 255));

    /* Advertising boards */
    drawRect(27, 30, 426, 4, GU_RGBA(230, 170, 20, 255));
    drawRect(27, 238, 426, 4, GU_RGBA(230, 170, 20, 255));

    /* Floodlights */
    drawCircle(12, 42, 5, GU_RGBA(255, 255, 170, 255));
    drawCircle(468, 42, 5, GU_RGBA(255, 255, 170, 255));
}

static void drawPlayer(Player *p, int team, int selected)
{
    float x = screenX(p->x);
    float y = screenY(p->y);

    unsigned int color;

    if(team == BLUE)
        color = GU_RGBA(25, 70, 245, 255);
    else
        color = GU_RGBA(225, 35, 35, 255);

    if(selected)
    {
        drawCircle(x, y + 1, 10,
                   GU_RGBA(255, 230, 0, 255));
    }

    drawCircle(x, y, 7, color);
    drawCircle(x - 2, y - 2, 2,
               GU_RGBA(255, 255, 255, 255));
}

static void drawBall(void)
{
    drawCircle(screenX(ball.x), screenY(ball.y), 4,
               GU_RGBA(255, 255, 255, 255));
    drawCircle(screenX(ball.x), screenY(ball.y), 1,
               GU_RGBA(20, 20, 20, 255));
}

static void drawScoreboard(void)
{
    drawRect(155, 3, 170, 25,
             GU_RGBA(10, 10, 20, 230));

    drawRect(158, 6, 164, 19,
             GU_RGBA(25, 35, 55, 255));

    /* Blue and red indicators */
    drawRect(163, 10, 12, 11,
             GU_RGBA(25, 70, 245, 255));

    drawRect(305, 10, 12, 11,
             GU_RGBA(225, 35, 35, 255));

    /* Score blocks */
    drawRect(183, 9, 35, 13,
             GU_RGBA(255, 255, 255, 255));
    drawRect(262, 9, 35, 13,
             GU_RGBA(255, 255, 255, 255));

    /* Simple score digits */
    drawRect(190, 12, 5, 7,
             GU_RGBA(10, 10, 10, 255));
    drawRect(270, 12, 5, 7,
             GU_RGBA(10, 10, 10, 255));

    /* Time indicator */
    drawRect(225, 10, 30, 10,
             GU_RGBA(30, 30, 40, 255));
    drawRect(237, 12, 5, 6,
             GU_RGBA(255, 255, 255, 255));
}

static void setupTeams(void)
{
    int i;

    float bx[11] = {
        0, -7, -3, 3, 7,
        -5, 0, 5,
        -6, 0, 6
    };

    float by[11] = {
        13, 9, 9, 9, 9,
        3, 4, 3,
        -5, -7, -5
    };

    for(i = 0; i < 11; i++)
    {
        blue[i].x = bx[i];
        blue[i].y = by[i];
        blue[i].hx = bx[i];
        blue[i].hy = by[i];

        red[i].x = -bx[i];
        red[i].y = -by[i];
        red[i].hx = -bx[i];
        red[i].hy = -by[i];
    }

    controlled = 9;
}

static void resetKickoff(int blueStarts)
{
    setupTeams();

    ball.x = 0;
    ball.y = 0;
    ball.vx = 0;
    ball.vy = 0;

    passTarget = -1;

    if(blueStarts)
        owner = 9;
    else
        owner = 11 + 9;

    controlled = 9;
}

static int nearestBlue(float x, float y)
{
    int i;
    int best = 0;
    float bestDist = 99999.0f;

    for(i = 0; i < 11; i++)
    {
        float d = distance(blue[i].x, blue[i].y, x, y);

        if(d < bestDist)
        {
            bestDist = d;
            best = i;
        }
    }

    return best;
}

static int nearestRed(float x, float y)
{
    int i;
    int best = 0;
    float bestDist = 99999.0f;

    for(i = 0; i < 11; i++)
    {
        float d = distance(red[i].x, red[i].y, x, y);

        if(d < bestDist)
        {
            bestDist = d;
            best = i;
        }
    }

    return best;
}

static int findPassTarget(void)
{
    int i;
    int best = -1;
    float bestScore = -99999.0f;

    for(i = 0; i < 11; i++)
    {
        float d;
        float score;

        if(i == controlled)
            continue;

        d = distance(blue[i].x, blue[i].y,
                     blue[controlled].x,
                     blue[controlled].y);

        score = -d + (-blue[i].y * 0.8f);

        if(score > bestScore)
        {
            bestScore = score;
            best = i;
        }
    }

    return best;
}

static void passBall(int through)
{
    int target = findPassTarget();
    float dx, dy, d;
    float power;

    if(target < 0)
        return;

    dx = blue[target].x - blue[controlled].x;
    dy = blue[target].y - blue[controlled].y;
    d = sqrtf(dx * dx + dy * dy);

    if(d < 0.1f)
        return;

    power = through ? 0.55f : 0.38f;

    ball.x = blue[controlled].x;
    ball.y = blue[controlled].y;

    ball.vx = dx / d * power;
    ball.vy = dy / d * power;

    owner = -1;
    passTarget = target;
}

static void shootBall(int chip)
{
    float power = chip ? 0.52f : 0.78f;

    ball.x = blue[controlled].x;
    ball.y = blue[controlled].y;

    ball.vx = 0;
    ball.vy = -power;

    if(chip)
        ball.vx = 0.10f;

    owner = -1;
    passTarget = -1;
}

static void crossBall(void)
{
    ball.x = blue[controlled].x;
    ball.y = blue[controlled].y;

    ball.vx = 0.35f;
    ball.vy = -0.52f;

    owner = -1;
    passTarget = -1;
}

static void moveControlled(float dx, float dy, float speed)
{
    Player *p = &blue[controlled];

    p->x += dx * speed;
    p->y += dy * speed;

    p->x = clampFloat(p->x, -9.0f, 9.0f);
    p->y = clampFloat(p->y, -14.0f, 14.0f);

    if(owner == controlled)
    {
        ball.x = p->x;
        ball.y = p->y;
    }
}

static void switchPlayer(void)
{
    int i;
    int best = controlled;
    float bestDist = 99999.0f;

    for(i = 0; i < 11; i++)
    {
        float d = distance(blue[i].x, blue[i].y,
                           ball.x, ball.y);

        if(d < bestDist)
        {
            bestDist = d;
            best = i;
        }
    }

    controlled = best;
}

static void switchDirectional(float dx, float dy)
{
    int i;
    int best = controlled;
    float bestScore = -99999.0f;

    for(i = 0; i < 11; i++)
    {
        float vx = blue[i].x - blue[controlled].x;
        float vy = blue[i].y - blue[controlled].y;
        float d = sqrtf(vx * vx + vy * vy);
        float score;

        if(d < 0.1f)
            continue;

        score = (vx * dx + vy * dy) / d;

        if(score > bestScore)
        {
            bestScore = score;
            best = i;
        }
    }

    controlled = best;
}

/* CIRCLE = STANDING TACKLE
   SQUARE = SLIDING TACKLE */
static void attemptTackle(int sliding)
{
    int enemy;
    Player *p;
    Player *e;
    float dx, dy, d;
    float range;
    float step;

    if(owner < 11 || owner >= 22)
        return;

    if(tackleCooldown > 0)
        return;

    p = &blue[controlled];
    e = &red[owner - 11];

    dx = e->x - p->x;
    dy = e->y - p->y;
    d = sqrtf(dx * dx + dy * dy);

    if(d > 0.01f)
    {
        step = sliding ? 0.55f : 0.38f;

        p->x += dx / d * step;
        p->y += dy / d * step;
    }

    p->x = clampFloat(p->x, -9.0f, 9.0f);
    p->y = clampFloat(p->y, -14.0f, 14.0f);

    d = distance(p->x, p->y, e->x, e->y);

    range = sliding ? 3.5f : 3.0f;

    if(d <= range)
    {
        enemy = owner;

        owner = controlled;
        ball.x = p->x;
        ball.y = p->y;
        ball.vx = 0;
        ball.vy = 0;
        passTarget = -1;

        tackleCooldown = sliding ? 35 : 18;
    }
    else
    {
        /* Keep the button responsive even when far away. */
        tackleCooldown = 3;
    }
}

static void goalkeeperRush(void)
{
    int keeper = 0;
    Player *g = &blue[keeper];

    if(owner >= 11 || owner == -1)
    {
        float dx = ball.x - g->x;
        float dy = ball.y - g->y;
        float d = sqrtf(dx * dx + dy * dy);

        if(d > 0.1f)
        {
            g->x += dx / d * 0.65f;
            g->y += dy / d * 0.65f;
        }

        if(d < 2.8f)
        {
            owner = 0;
            ball.x = g->x;
            ball.y = g->y;
            ball.vx = 0;
            ball.vy = 0;
        }
    }
}

static void updateAI(void)
{
    int i;

    for(i = 0; i < 11; i++)
    {
        Player *p = &red[i];

        if(owner >= 11 && owner < 22)
        {
            if(owner == i + 11)
            {
                p->y += 0.025f;
                continue;
            }
        }

        if(owner >= 0 && owner < 11)
        {
            float dx = blue[owner].x - p->x;
            float dy = blue[owner].y - p->y;
            float d = sqrtf(dx * dx + dy * dy);

            if(d > 0.1f)
            {
                p->x += dx / d * 0.045f;
                p->y += dy / d * 0.045f;
            }
        }
        else
        {
            float dx = ball.x - p->x;
            float dy = ball.y - p->y;
            float d = sqrtf(dx * dx + dy * dy);

            if(d > 0.1f)
            {
                p->x += dx / d * 0.035f;
                p->y += dy / d * 0.035f;
            }
        }

        p->x = clampFloat(p->x, -9.0f, 9.0f);
        p->y = clampFloat(p->y, -14.0f, 14.0f);
    }

    /* Red player carrying the ball moves toward Blue's goal. */
    if(owner >= 11 && owner < 22)
    {
        Player *p = &red[owner - 11];

        p->y -= 0.045f;

        ball.x = p->x;
        ball.y = p->y;

        if(p->y < -10.0f && fabsf(p->x) < 5.0f)
        {
            ball.vy = -0.65f;
            ball.vx = (0.0f - p->x) * 0.025f;
            owner = -1;
        }
    }
}

static void updateBall(void)
{
    int i;
    int best = -1;
    float bestDist = 99999.0f;

    if(owner >= 0)
    {
        Player *p = getPlayer(owner);

        ball.x = p->x;
        ball.y = p->y;

        return;
    }

    ball.x += ball.vx;
    ball.y += ball.vy;

    ball.vx *= 0.985f;
    ball.vy *= 0.985f;

    if(fabsf(ball.vx) < 0.006f)
        ball.vx = 0;

    if(fabsf(ball.vy) < 0.006f)
        ball.vy = 0;

    /* Goal for Blue */
    if(ball.y < -15.0f && fabsf(ball.x) < 3.0f)
    {
        scoreBlue++;
        resetKickoff(0);
        return;
    }

    /* Goal for Red */
    if(ball.y > 15.0f && fabsf(ball.x) < 3.0f)
    {
        scoreRed++;
        resetKickoff(1);
        return;
    }

    if(ball.x < -10.0f || ball.x > 10.0f)
        ball.vx = -ball.vx * 0.65f;

    if(ball.y < -15.0f || ball.y > 15.0f)
        ball.vy = -ball.vy * 0.65f;

    ball.x = clampFloat(ball.x, -10.0f, 10.0f);
    ball.y = clampFloat(ball.y, -15.0f, 15.0f);

    /* Automatic pass receiver selection. */
    if(passTarget >= 0)
    {
        if(distance(ball.x, ball.y,
                    blue[passTarget].x,
                    blue[passTarget].y) < 1.7f)
        {
            owner = passTarget;
            controlled = passTarget;

            ball.vx = 0;
            ball.vy = 0;
            passTarget = -1;

            return;
        }
    }

    /* Find the nearest player to a loose ball. */
    for(i = 0; i < 11; i++)
    {
        float d = distance(ball.x, ball.y,
                           blue[i].x, blue[i].y);

        if(d < bestDist)
        {
            bestDist = d;
            best = i;
        }

        d = distance(ball.x, ball.y,
                     red[i].x, red[i].y);

        if(d < bestDist)
        {
            bestDist = d;
            best = i + 11;
        }
    }

    if(best >= 0 && bestDist < 0.9f)
    {
        owner = best;

        if(best < 11)
            controlled = best;

        ball.vx = 0;
        ball.vy = 0;
        passTarget = -1;
    }
}

static void updateMatch(unsigned int buttons, unsigned int pressed)
{
    float dx = 0;
    float dy = 0;
    float speed = 0.17f;

    Player *p = &blue[controlled];

    if(tackleCooldown > 0)
        tackleCooldown--;

    if(buttons & PSP_CTRL_UP)
        dy = -1;

    if(buttons & PSP_CTRL_DOWN)
        dy = 1;

    if(buttons & PSP_CTRL_LEFT)
        dx = -1;

    if(buttons & PSP_CTRL_RIGHT)
        dx = 1;

    if(buttons & PSP_CTRL_RTRIGGER)
        speed = 0.29f;

    /* The player controlled by the user is ALWAYS Blue. */
    if(dx != 0 || dy != 0)
    {
        float length = sqrtf(dx * dx + dy * dy);

        if(length > 0)
        {
            dx /= length;
            dy /= length;
        }

        moveControlled(dx, dy, speed);
    }

    /* L = switch to nearest Blue player. */
    if(pressed & PSP_CTRL_LTRIGGER)
        switchPlayer();

    /* Defensive controls when Red has possession. */
    if(owner >= 11 && owner < 22)
    {
        if(pressed & PSP_CTRL_CIRCLE)
            attemptTackle(0);

        if(pressed & PSP_CTRL_SQUARE)
            attemptTackle(1);

        if(pressed & PSP_CTRL_TRIANGLE)
            goalkeeperRush();

        if(buttons & PSP_CTRL_X)
        {
            float d = distance(p->x, p->y,
                               ball.x, ball.y);

            if(d > 1.0f)
            {
                float vx = ball.x - p->x;
                float vy = ball.y - p->y;
                float len = sqrtf(vx * vx + vy * vy);

                if(len > 0.1f)
                {
                    p->x += vx / len * 0.12f;
                    p->y += vy / len * 0.12f;
                }
            }
            else
            {
                attemptTackle(0);
            }
        }

        if((buttons & PSP_CTRL_LTRIGGER) &&
           (buttons & PSP_CTRL_RTRIGGER))
        {
            secondPress = 1;
        }
        else
        {
            secondPress = 0;
        }
    }
    else
    {
        /* Attacking controls when Blue has possession. */
        if(owner == controlled)
        {
            if(pressed & PSP_CTRL_X)
                passBall(0);

            if(pressed & PSP_CTRL_TRIANGLE)
                passBall(1);

            if(pressed & PSP_CTRL_SQUARE)
            {
                if(buttons & PSP_CTRL_RTRIGGER)
                    shootBall(0);
                else if(buttons & PSP_CTRL_TRIANGLE)
                    shootBall(1);
                else
                    shootBall(0);
            }

            if(pressed & PSP_CTRL_CIRCLE)
                crossBall();

            if((buttons & PSP_CTRL_X) &&
               (buttons & PSP_CTRL_RTRIGGER))
            {
                moveControlled(0, -1, 0.25f);
            }
        }
        else if(owner == -1)
        {
            /* Loose ball: move toward it and collect it. */
            if(pressed & PSP_CTRL_CIRCLE)
                attemptTackle(0);
        }
    }

    /* Keep the controlled index within the Blue team. */
    controlled = clampInt(controlled, 0, 10);

    updateAI();
    updateBall();

    gameFrames++;

    if(gameFrames % 60 == 0)
        matchTime++;
}

static void drawGame(void)
{
    int i;

    sceGuStart(GU_DIRECT, list);

    sceGuClearColor(GU_RGBA(5, 10, 15, 255));
    sceGuClear(GU_COLOR_BUFFER_BIT);

    drawPitch();

    /* Red team */
    for(i = 0; i < 11; i++)
        drawPlayer(&red[i], RED, 0);

    /* Blue team */
    for(i = 0; i < 11; i++)
        drawPlayer(&blue[i], BLUE, i == controlled);

    drawBall();
    drawScoreboard();

    sceGuFinish();
    sceGuSync(0, 0);

    sceDisplayWaitVblankStart();
    sceGuSwapBuffers();
}

int main(void)
{
    SceCtrlData pad;
    unsigned int buttons;
    unsigned int pressed;

    sceGuInit();

    sceGu
