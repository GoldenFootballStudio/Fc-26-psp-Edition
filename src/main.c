
#include <pspkernel.h>
#include <pspdebug.h>
#include <pspdisplay.h>

PSP_MODULE_INFO("FC26 PSP Edition", 0, 1, 0);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER);

int main(void)
{
    pspDebugScreenInit();

    pspDebugScreenSetTextColor(0xFFFFFFFF);
    pspDebugScreenPrintf("FC 26 PSP EDITION\n\n");

    pspDebugScreenSetTextColor(0xFF00FF00);
    pspDebugScreenPrintf("Welcome to the game!\n\n");

    pspDebugScreenSetTextColor(0xFFFFFFFF);
    pspDebugScreenPrintf("Golden Football Studio\n");
    pspDebugScreenPrintf("First playable prototype\n\n");
    pspDebugScreenPrintf("Stay Golden!");

    while (1)
    {
        sceDisplayWaitVblankStart();
    }

    return 0;
}
