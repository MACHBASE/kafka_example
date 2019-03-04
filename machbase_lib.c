#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "machbase_lib.h"

/* for logging */
#include "include/zlog.h"
zlog_category_t    *gLog;

int   gState = 0;
int   gSuccessCount = 0;
char *gDefaultDateFormatStr = "YYYY/MM/DD-HH24:MI:SS.mmm";

#define SERVER_IP_ENV      "MACHBASE_IP"
#define SERVER_PORT_ENV    "MACHBASE_PORT_NO"
#define SERVER_DATE_FORMAT "CSV_DATEFORMAT"
#define TAG_TABLE_NAME     "TAG"
#define ERROR_CHECK_COUNT	1000

#define CHECK_APPEND_RESULT(aRC, aEnv, aCon, aSTMT)             \
    if( !SQL_SUCCEEDED(aRC) )                                   \
    {                                                           \
        if( checkAppendError(aEnv, aCon, aSTMT) == SQL_ERROR )  \
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

    zlog_error(gLog, "Append Error : [%d][%s]\n[%s]\n\n", aErrorCode, sErrMsg, sRowMsg);
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
        sServerPort = "5656";
    }
    aHandle->mDateFormat = getenv(SERVER_DATE_FORMAT);
    if (aHandle->mDateFormat == NULL)
    {
        aHandle->mDateFormat = gDefaultDateFormatStr;
    }
    if (strcmp(aHandle->mDateFormat, "INTEGER") == 0)
    {
        aHandle->mIsDateInteger = 1;
    }
    else
    {
        aHandle->mIsDateInteger = 0;
    }
    sprintf(sConnStr, "DSN=%s;UID=SYS;PWD=MANAGER;CONNTYPE=1;PORT_NO=%s",
            sServerIP, sServerPort);
    zlog_info(gLog, "MachbaseConnectionStr = %s", sConnStr);
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
    zlog_info(gLog, "Machbase connected successfully\n");
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
    zlog_info(gLog, "Machbase appendopen success.\n");
    gState = 5;
    if (SQLSetStmtAppendInterval(aHandle->mStmt, 500) != SQL_SUCCESS)
    {
      goto err_return;
    }
    SQLAppendSetErrorCallback(aHandle->mStmt, dumpError);
    return SQL_SUCCESS;
err_return:
    switch (gState)
    {
        case 5:
            fprintf(stderr, "Machbase SetStmtAppendInterval failed.\n");
            break;
        case 4:
            fprintf(stderr, "Machbase AppendOpen failed.\n");
            break;
        case 3:
            fprintf(stderr, "Machbase Connection failed.\n");
            break;
        case 2:
            fprintf(stderr, "Machbase AllocConn failed.\n");
            break;
        case 1:
            fprintf(stderr, "Machbase AllocEnv failed.\n");
            break;
        default:
            fprintf(stderr, "Machbase Unknown error.\n");
            break;
    }
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
                zlog_info(gLog, "%ld rows appended successfully, %ld rows failed to append\n", sSuccessCnt, sFailureCnt);
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
    /* remove trailing quoute and space */
    if (n < 1)
        return 1;
    while (aSrc[n-1] == '"' || aSrc[n-1] == ' ')
    {
        n--;
    }
    if (n < 1)
        return 1;
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
    int   sRc;
    sTemp = strtok(aSrc, ",");
    if (sTemp == NULL) return 1;
    sRc = TrimQuoteSpace(sTemp, aField1);
    if (sRc > 0) return sRc;
    sTemp = strtok(NULL, ",");
    if (sTemp == NULL) return 1;
    sRc = TrimQuoteSpace(sTemp, aField2);
    if (sRc > 0) return sRc;
    sTemp = strtok(NULL, ",");
    if (sTemp == NULL) return 1;
    sRc = TrimQuoteSpace(sTemp, aField3);
    if (sRc > 0) return sRc;
    return 0;
}

int csv_to_tag(machbase_handle_t *aHandle,
               char              *aInput,
               int                aSize)
{
    /* assume "tagname", "tagtime", value */
    SQLRETURN sRc;
    int i;
    SQL_APPEND_PARAM sParm[3];
    char sTagName[200];
    char sTimeStr[200];
    char sValStr[200];
    long sTime;/* to be changed */
    if (GetCSVRow(aInput, sTagName, sTimeStr, sValStr) != 0)
    {
        fprintf (stderr, "CSV Parsing error : %s\n", aInput);
        return 1;
    }
    /* tagname */
    sParm[0].mVar.mData = sTagName;
    sParm[0].mVar.mLength = strlen(sTagName);
    /* time */
    if (aHandle->mIsDateInteger == 1)
    {
        sTime = atol(sTimeStr);
        sParm[1].mDateTime.mTime = sTime;
    }
    else
    {
        sParm[1].mDateTime.mTime = SQL_APPEND_DATETIME_STRING;
        sParm[1].mDateTime.mDateStr = sTimeStr;
        sParm[1].mDateTime.mFormatStr = aHandle->mDateFormat;
    }
    sscanf (sValStr, "%lf", &(sParm[2].mDouble));
    sRc = SQLAppendDataV2(aHandle->mStmt, sParm);
    CHECK_APPEND_RESULT(sRc, aHandle->mEnv, aHandle->mDBC, aHandle->mStmt);
    gSuccessCount++;

    if (gSuccessCount == ERROR_CHECK_COUNT)
    {
        gSuccessCount = 0;
        sRc = SQLAppendFlush(aHandle->mStmt);
        CHECK_APPEND_RESULT(sRc, aHandle->mEnv, aHandle->mDBC, aHandle->mStmt);
    }

    return 0;
}
