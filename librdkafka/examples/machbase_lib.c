#include <stdio.h>
#include <string.h>
#include "machbase_lib.h"

int   gState = 0;
char *gDefaultDateFormatStr = "YYYY/MM/DD-HH:MM:SS.mmm";
char sTagName[200];
char sTimeStr[200];
char sValStr[200];

extern int quiet;
#define SERVER_IP_ENV      "MACBHASE_IP"
#define SERVER_PORT_ENV    "MACHBASE_PORT"
#define SERVER_DATE_FORMAT "DATEFORMAT"
#define TAG_TABLE_NAME     "TAG"
#define ERROR_CHECK_COUNT	100000

#define CHECK_APPEND_RESULT(aRC, aEnv, aCon, aSTMT)             \
    if( !SQL_SUCCEEDED(aRC) )                                   \
    {                                                           \
        if( checkAppendError(aEnv, aCon, aSTMT) == SQL_ERROR ) \
        {                                                       \
            ;                                                   \
        }                                                       \
    }

void dumpError(SQLHSTMT aStmtHandle,
               SQLINTEGER aErrorCode,
               SQLPOINTER aErrorMessage,
               SQLLEN aErrorBufLen,
               SQLPOINTER aRowBuf,
               SQLLEN aRowBufLen)
{
    char sErrMsg[1024] = {0, };
    char sRowMsg[32 * 1024] = {0, };
    
    if (aErrorMessage != NULL)
    {
        strncpy(sErrMsg, (char *)aErrorMessage, aErrorBufLen);
    }
    
    if (aRowBuf != NULL)
    {
        strncpy(sRowMsg, (char *)aRowBuf, aRowBufLen);
    }
    
    fprintf(stdout, "Append Error : [%d][%s]\n[%s]\n\n", aErrorCode, sErrMsg, sRowMsg);
    exit(1);
}

int checkAppendError(SQLHENV aEnv, SQLHDBC aCon, SQLHSTMT aStmt)
{
    SQLINTEGER      sNativeError;
    SQLCHAR         sErrorMsg[SQL_MAX_MESSAGE_LENGTH + 1];
    SQLCHAR         sSqlState[SQL_SQLSTATE_SIZE + 1];
    SQLSMALLINT     sMsgLength;

    if( SQLError(aEnv, aCon, aStmt, sSqlState, &sNativeError,
        sErrorMsg, SQL_MAX_MESSAGE_LENGTH, &sMsgLength) != SQL_SUCCESS )
    {
        return SQL_ERROR;
    }

    printf("SQLSTATE-[%s], Machbase-[%d][%s]\n", sSqlState, sNativeError, sErrorMsg);

    if( sNativeError != 9604 &&
        sNativeError != 9605 &&
        sNativeError != 9606 )
    {
        return SQL_ERROR;
    }

    return SQL_SUCCESS;
}
int init_machbase(machbase_handle_t *aHandle)
{
    char *sServerIP;
    char *sServerPort;
    char sConnStr[512];
    sServerIP = getenv(SERVER_IP_ENV);
    if (sServerIP == NULL)
    {
        sServerIP = "127.0.0.1";
    }
    sServerPort = getenv(SERVER_PORT_ENV);
    if (sServerPort == NULL)
    {
        sServerPort = "8086";
    }
    aHandle->mDateFormat = getenv(SERVER_DATE_FORMAT);
    if (aHandle->mDateFormat == NULL)
    {
        aHandle->mDateFormat = gDefaultDateFormatStr;
    }
    sprintf(sConnStr, "DSN=%s;UID=SYS;PWD=MANAGER;CONNTYPE=1;PORT_NO=%s",
            sServerIP, sServerPort);
    fprintf(stderr, "MachbaseConnectionStr = %s", sConnStr);
    if (SQLAllocEnv(&(aHandle->mEnv)) != SQL_SUCCESS)
    {
        goto err_return;
    }
    gState = 1;
    if (SQLAllocConnect(aHandle->mEnv, &(aHandle->mDBC)) != SQL_SUCCESS)
    {
        goto err_return;
    }
    gState = 2;
    if (SQLDriverConnect(aHandle->mDBC, NULL,
                         (SQLCHAR*) sConnStr,
                         SQL_NTS, NULL, 0, NULL,
                         SQL_DRIVER_NOPROMPT) != SQL_SUCCESS)
    {
        goto err_return;
    }
    fprintf(stderr, "Machbase connected successfully\n");
    gState = 3;
    if (SQLAllocStmt(aHandle->mDBC, &(aHandle->mStmt)) != SQL_SUCCESS)
    {
        goto err_return;
    }
    gState = 4;
    if (SQLAppendOpen(aHandle->mStmt, (SQLCHAR*)TAG_TABLE_NAME, ERROR_CHECK_COUNT)
        != SQL_SUCCESS)
    {
        goto err_return;
    }
    fprintf(stderr, "Machbase appendopen success.\n");
    gState = 5;
/*    if (SQLSetStmtAppendInterval(aHandle->mStmt, 500) != SQL_SUCCESS)
    {
        goto err_return;
    }
    SQLAppendSetErrorCallback(aHandle->mStmt, dumpError);
*/ 
    return SQL_SUCCESS;
err_return:
    fini_machbase(aHandle);
    exit(1);
    return SQL_ERROR;
}

int fini_machbase(machbase_handle_t *aHandle)
{
    switch (gState)
    {
        case 5:
            {
                unsigned long sSuccessCnt = 0;
                unsigned long sFailureCnt = 0;
                SQLAppendClose(aHandle->mStmt, &sSuccessCnt,
                    &sFailureCnt);
                fprintf(stderr, "%ld rows appended successfully, %ld rows failed to append\n", sSuccessCnt, sFailureCnt);
            }
        case 4:
            SQLFreeStmt(aHandle->mStmt, SQL_DROP);
        case 3:
            SQLDisconnect(aHandle->mDBC);
        case 2:
            SQLFreeConnect(aHandle->mDBC);
        case 1:
            SQLFreeEnv(aHandle->mEnv);
        default:
            break;
    }
    gState = 0;
}


int TrimQuoteSpace(char *aSrc,
                   char *aRet)
{
    int n = 0;
    int i;
    /* trim heading quote and space */
    while (*aSrc == '"' || *aSrc == ' ')
    {
        aSrc++;
    }
    n = strlen(aSrc);
    while (aSrc[n-1] == '"' || aSrc[n-1] == ' ')
    {
        n--;
    }
    for (i = 0; i < n; i++)
    {
        aRet[i] = aSrc[i];
    }
    aRet[i] = '\0';
    return 0;
}

int GetCSVRow(char *aSrc,
              char *aField1,
              char *aField2,
              char *aField3)
{
    char *sTemp;
    sTemp = strtok(aSrc, ",");
    TrimQuoteSpace(sTemp, aField1);
    sTemp = strtok(NULL, ",");
    TrimQuoteSpace(sTemp, aField2);
    sTemp = strtok(NULL, ",");
    TrimQuoteSpace(sTemp, aField3);
    return 0;
}

int csv_to_tag(machbase_handle_t *aHandle,
               char              *aInput,
               int                aSize)
{
    /* assume "tagname", "tagtime", value */
    char **sRowFields;
    SQLRETURN sRc;
    int i;
    SQL_APPEND_PARAM sParm[3];

    long sTime;/* to be changed */
    GetCSVRow(aInput, sTagName, sTimeStr, sValStr);

    /* tagname */
    sParm[0].mVar.mData = sTagName;
    sParm[0].mVar.mLength = strlen(sTagName);
    /* time */
    sTime = atol(sTimeStr);
    sParm[1].mDateTime.mTime = sTime;
    sscanf (sValStr, "%lf", &(sParm[2].mDouble));
    sRc = SQLAppendDataV2(aHandle->mStmt, sParm);
    CHECK_APPEND_RESULT(sRc, aHandle->mEnv, aHandle->mDBC, aHandle->mStmt);
    return 0;
}
