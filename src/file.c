#include <errno.h>
#include "file.h"
#include "platform.h"

/******************************************************************************/
/* PUBLIC FUNCTION IMPLEMENTATIONS ********************************************/
/******************************************************************************/

#if defined(PLATFORM_WINDOWS)
#else
#include <sys/stat.h>
#endif

int file_is_dir(Str *filename)
{
#if defined(PLATFORM_WINDOWS)
    ASSERT("not implemented");
#else
    struct stat s;
    char *path = str_cstr(filename);
    if(path) {
        int r = stat(path, &s);
        free(path);
        if(r) return 0;
        return S_ISDIR(s.st_mode);
    }
#endif
    return 0;
}

int file_fp_read(FILE *file, Str *content)
{
    if(!file) THROW("invalid filename");
    if(!content) THROW("invalid output buffer");

    /* get file length */
    fseek(file, 0, SEEK_END);
    size_t bytes_file = (size_t)ftell(file);
    fseek(file, 0, SEEK_SET);

    /* allocate memory */
    TRY(str_reserve(content, bytes_file + 1), "couldn't increase capacity");

    /* read file */
    size_t bytes_read = fread(content->s, 1, bytes_file, file);
    //if(bytes_file != bytes_read) THROW("mismatch read vs expected bytes");
    content->s[bytes_read] = 0;
    content->last = bytes_read;

    /* close file outside */
    return 0;
error:
    return -1;
}

int file_str_read(Str *filename, Str *content)
{
    int err = 0;
    FILE *file = 0;
    if(!filename) THROW("invalid filename");
    if(!content) THROW("invalid output buffer");

    /* open the file */
    errno = 0;
    if(filename->last && (
                filename->s[filename->last - 1] == PLATFORM_CH_SUBDIR ||
                filename->s[filename->last - 1] == '/')) {
        THROW("won't open directories");
    }
    file = fopen(filename->s, "r");
    if(!file || errno) THROW("failed to open file named '%s'", filename->s);

    TRY(file_fp_read(file, content), ERR_FILE_FP_READ);
    /* close file */
clean:
    if(file) fclose(file);
    return err;
error: ERR_CLEAN;
}

#if 0
int file_str_write(Str *filename, Str *content)
{
    ASSERT_ERROR("implementation missing");
    return 0;
}
#endif

#if defined(PLATFORM_WINDOWS)
#else
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#endif

ErrDecl file_dir_read(Str *dirname, VStr *files)
{
    int err = 0;
    DIR *dir = 0;
    struct dirent *dp = 0;
    if ((dir = opendir(dirname->s)) == NULL) {
        THROW("can't open directory '%s'", dirname->s);
    }
    while ((dp = readdir(dir)) != NULL)
    {
        Str filename = {0};
        if(!str_cmp(&STR_L(dp->d_name), &STR(".")) || !str_cmp(&STR_L(dp->d_name), &STR(".."))) continue;
        TRYF(str_fmt, &filename, "%s/%s", dirname->s, dp->d_name);
        //printf("FILE: %.*s\n", STR_F(&filename));
        TRY(vstr_push_back(files, &filename), ERR_VEC_PUSH_BACK);
    }
clean:
    if(dir) closedir(dir);
    return err;
error: ERR_CLEAN;
}

