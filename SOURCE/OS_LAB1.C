/*
*********************************************************************************************************
*                                                uC/OS-II
*                                         Lab1 Logging Extension
*
* File : OS_LAB1.C
* Author : Kash
*********************************************************************************************************
*/

#ifndef  OS_MASTER_FILE
#define  OS_GLOBALS
#include "includes.h"
#endif


#if OS_LAB1_EN > 0
/*$PAGE*/
/*
*********************************************************************************************************
*                                         INITIALIZE LAB1 MODULE
*********************************************************************************************************
*/

void  OSLab1Init (void)
{
    OSLogHead       = 0;
    OSLogTail       = 0;
    OSLogCount      = 0;
    OSLogOverflowCtr = 0L;
    OSLogPendEvent  = OS_LOG_EVT_NONE;
    TimeIsReset     = FALSE;
}


/*$PAGE*/
/*
*********************************************************************************************************
*                                         [Lab1] LOG Function
*********************************************************************************************************
*/

void  OSLogTaskSwCapture (void)
{
    OS_LOG *prec;


    if (OSLogPendEvent == OS_LOG_EVT_NONE || TimeIsReset == FALSE) {
        return;
    }

    prec             = &OSLOGTbl[OSLogHead];
#if OS_TIME_GET_SET_EN > 0
    prec->time       = OSTime;
#else
    prec->time       = 0;
#endif
    prec->switchEvent = OSLogPendEvent;
    prec->fromTaskID = OSTCBCur->OSTCBPrio;
    prec->toTaskID   = OSTCBHighRdy->OSTCBPrio;

    if (prec->fromTaskID == OS_LOWEST_PRIO - 2) {
            prec->fromTaskID = OS_LOWEST_PRIO;
        }
	if (prec->toTaskID == OS_LOWEST_PRIO - 2) {
		prec->toTaskID = OS_LOWEST_PRIO;
	}
	if (prec->fromTaskID == prec->toTaskID) {
	        return;
	}

    OSLogHead++;
    if (OSLogHead >= OS_LOG_BUF_SIZE) {
        OSLogHead = 0;
    }

    if (OSLogCount < OS_LOG_BUF_SIZE) {
        OSLogCount++;
    } else {
        OSLogOverflowCtr++;
        OSLogTail++;
        if (OSLogTail >= OS_LOG_BUF_SIZE) {
            OSLogTail = 0;
        }
    }
}


INT8U  OSGetLOG (OS_LOG *prec)
{
#if OS_CRITICAL_METHOD == 3                                /* Allocate storage for CPU status register */
    OS_CPU_SR  cpu_sr;
#endif
    INT8U      result;


    if (prec == (OS_LOG *)0) {
        return (FALSE);
    }

    result = FALSE;

    OS_ENTER_CRITICAL();
    if (OSLogCount > 0) {
        *prec = OSLOGTbl[OSLogTail];
        OSLogTail++;
        if (OSLogTail >= OS_LOG_BUF_SIZE) {
            OSLogTail = 0;
        }
        OSLogCount--;
        result = TRUE;
    }
    OS_EXIT_CRITICAL();
    return (result);
}
#endif