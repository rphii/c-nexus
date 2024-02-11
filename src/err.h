#ifndef ERR_H

#include <stddef.h>
#include <stdio.h>
#include <assert.h>

#include "colorprint.h"
#include "attr.h"

#ifndef DEBUG
#define DEBUG           0
#define DEBUG_GETCH     0
#define DEBUG_GETCHAR   0
#define DEBUG_INSPECT   0
#define DEBUG_DISABLE_ERR_MESSAGES  0
#endif

#define SIZE_IS_NEG(x)      (((SIZE_MAX >> 1) + 1) & (x))

/* use this when declaring function that can error */
#define ErrDecl             ATTR_NODISCARD int
#define ErrDeclStatic       ATTR_NODISCARD static inline int

#define SIZE_ARRAY(x)       (sizeof(x)/sizeof(*x))

#define ERR_STRINGIFY(S)    #S
#define ERR_CLEAN           do { err = -1; goto clean; } while(0)

/* general error messages */
#define ERR_MALLOC          "failed to malloc"
#define ERR_CREATE_POINTER  "expected pointer to Create"
#define ERR_SIZE_T_POINTER  "expected pointer to size_t"
#define ERR_BOOL_POINTER    "expected pointer to bool"
#define ERR_CSTR_POINTER    "expected pointer to c-string"
#define ERR_CSTR_INVALID    "encountered unexpected null character in c-string"
#define ERR_UNHANDLED_ID    "unhandled id"
#define ERR_UNREACHABLE     "unreachable error"
#define ERR_UNIMPLEMENTED   "unimplemented"
#define ERR_NULL_ARG        "unexpected null pointer argument received"
#define ERR_SYSTEM          "failed executing system command"
#define ERR_POPEN           ERR_SYSTEM


#define ERR_FILE_STREAM     stderr

#if DEBUG_DISABLE_ERR_MESSAGES
#define ERR_PRINTF(fmt, ...)    {}
#else
#define ERR_PRINTF(fmt, ...)    do { fflush(stdout); /* for whatever reason I have to do this fflush... */ fprintf(ERR_FILE_STREAM, fmt, ##__VA_ARGS__); } while(0)
#endif

/* macros */

#define THROW(fmt, ...)      do { \
    ERR_PRINTF(F("[ERROR]", BOLD FG_RD_B)" "F("%s:%d:%s", FG_WT_B)" "fmt"\n", __FILE__, __LINE__, __func__, ##__VA_ARGS__); \
    goto error; } while(0)

#define ABORT(fmt, ...)      do { \
    ERR_PRINTF(F("[ABORT]", BOLD FG_BK BG_RD_B)" "F("%s:%d:%s (end of trace)", FG_WT_B)" "fmt"\n", __FILE__, __LINE__, __func__, ##__VA_ARGS__); exit(-1); } while(0)

#define INFO(fmt, ...)       do { if(DEBUG) \
    ERR_PRINTF(F("[INFO]", BOLD FG_YL_B)" "F("%s:%d:%s", FG_WT_B)" "fmt"\n", __FILE__, __LINE__, __func__, ##__VA_ARGS__); \
} while(0)

#define TRY(stmt, fmt, ...)  if (stmt) { THROW(fmt, ##__VA_ARGS__); }
#define ASSERT_ERROR(x)      assert(0 && (x))
#define ASSERT(stmt, fmt, ...)   if (!stmt) { ABORT("assertion of '" ERR_STRINGIFY(stmt) "' failed... " fmt, ##__VA_ARGS__); }


#define ERR_H
#endif

