#include "cmd.h"

int cmd_run(Str *cmd)
{
    ASSERT(cmd, ERR_NULL_ARG);
    Str cmd_fmt = {0};
    int err = 0;
    if(str_length(cmd)) {
        TRY(str_fmt(&cmd_fmt, "%.*s", STR_F(cmd)), ERR_STR_FMT);
        err = system(cmd_fmt.s);
    }
clean:
    str_free(&cmd_fmt);
    return err;
error:
    ERR_CLEAN;
}
