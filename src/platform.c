#include "platform.h"
#include "colorprint.h"

/******************************************************************************/
/* getch **********************************************************************/
/******************************************************************************/

#if defined(PLATFORM_WINDOWS)
#include <conio.h>
#else
#include <unistd.h>
#include <termios.h>
#include <stdio.h>
#include <stdlib.h>
#endif

int platform_getch(void)
{
    //printf(F("[press any key] ", IT FG_CY_B));
    fflush(stdout);

#if defined(PLATFORM_WINDOWS)

    return _getch();

#else

    int buf = 0;
    struct termios old = {0};
    fflush(stdout);
    if(tcgetattr(0, &old) < 0) {
        perror("tcsetattr()");
    }
    old.c_lflag &= (tcflag_t)~ICANON;
    old.c_lflag &= (tcflag_t)~ECHO;
    old.c_cc[VMIN] = 1;
    old.c_cc[VTIME] = 0;
    if(tcsetattr(0, TCSANOW, &old) < 0) {
        perror("tcsetattr ICANON");
    }
    if(read(0, &buf, 1) < 0) {
        perror("read()");
    }
    old.c_lflag |= ICANON;
    old.c_lflag |= ECHO;
    if(tcsetattr(0, TCSADRAIN, &old) < 0) {
        perror("tcsetattr ~ICANON");
    }
    //printf("%c", buf);
    return buf;

#endif
}

/******************************************************************************/
/* clear - terminal clearing **************************************************/
/******************************************************************************/

void platform_clear(void)
{
    printf("\033[H\033[J""\033[H\033[J");
}


