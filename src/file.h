#ifndef FILE_H

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include "err.h"
#include "str.h"
#include "vector.h"

/******************************************************************************/
/* PUBLIC FUNCTION PROTOTYPES *************************************************/
/******************************************************************************/

int file_is_dir(Str *filename);

#define ERR_FILE_STR_READ "failed to read file"
#define file_str_read_ERR(filename, content) "failed reading file '%.*s'", STR_F(filename)
ErrDecl file_str_read(Str *filename, Str *content);

ErrDecl file_str_write(Str *filename, Str *content);

#define ERR_FILE_FP_READ "failed to read file"
#define file_fp_read_ERR(file, content) "failed reading file pointer '%p'", file
ErrDecl file_fp_read(FILE *file, Str *content);

#define file_dir_read_ERR(dirname, files) "failed reading directory"
ErrDecl file_dir_read(Str *dirname, VStr *files);

#define FILE_H
#endif
