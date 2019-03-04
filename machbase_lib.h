#ifndef MACHLIB_H
#define MACHLIB_H
#include <machbase_sqlcli.h>

typedef struct machbase_handle_s
{
    SQLHENV mEnv;
    SQLHDBC mDBC;
    SQLHSTMT mStmt;
    char   *mDateFormat;
    int     mIsDateInteger;
} machbase_handle_t;

int init_machbase(machbase_handle_t *aHandle);
int fini_machbase(machbase_handle_t *aHandle);

int csv_to_tag(machbase_handle_t *aHandle,
               char              *aInput,
               int                aSize);
#endif
