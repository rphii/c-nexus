#ifndef CMD_H

#include "str.h"
#include "colorprint.h"

#define CMD_FMT(x)      F(x, FG_BK_B)
#define CMD_IMG(x)      "imv " x " &"
#define CMD_SUDO(x)     "sudo " x " "

#define CMD_NONE        0

void cmd_run(Str *cmd);

#define CMD_H
#endif

