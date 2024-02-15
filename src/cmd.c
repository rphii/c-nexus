#include "cmd.h"

void cmd_run(Str *cmd)
{
    ASSERT(cmd, ERR_NULL_ARG);
    Str cmd_fmt = {0};
    if(str_length(cmd)) {
        TRY(str_fmt(&cmd_fmt, "%.*s", STR_F(cmd)), ERR_STR_FMT);
        system(cmd_fmt.s);
    }
error:
    str_free(&cmd_fmt);
}
