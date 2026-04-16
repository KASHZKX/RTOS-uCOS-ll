/*
*********************************************************************************************************
*                                                uC/OS-II
*                                          The Real-Time Kernel
*
*                           (c) Copyright 1992-2002, Jean J. Labrosse, Weston, FL
*                                           All Rights Reserved
*
*                                               EXAMPLE #1
*********************************************************************************************************
*/

#include "includes.h"

/*
*********************************************************************************************************
*                                               CONSTANTS
*********************************************************************************************************
*/

#define  TASK_STK_SIZE                1024       /* Size of each task's stacks (# of WORDs)            */
#define  MAX_TASKS                       5       /* Number of identical tasks                          */
#define  TASK_SET_NO                     1       /* No of test sets (1, 2)                             */
#define  LOG_STK_SIZE                 1024
#define  LOG_FIRST_LINE                  1
#define  LOG_LAST_LINE                  20
/*
*********************************************************************************************************
*                                               VARIABLES
*********************************************************************************************************
*/

OS_STK        TaskStk[MAX_TASKS][TASK_STK_SIZE];        /* Tasks stacks                                  */
OS_STK        TaskStartStk[TASK_STK_SIZE];
OS_STK        LogStk[LOG_STK_SIZE];
TASKCFG       TaskCFG[MAX_TASKS];                      /* Parameters to pass to each task               */
INT8U         Taskcount;
FILE *fp_log;
/*
*********************************************************************************************************
*                                           FUNCTION PROTOTYPES
*********************************************************************************************************
*/

        void  Task(void *data);                       /* Function prototypes of tasks                  */
        void  TaskStart(void *data);                  /* Function prototypes of Startup task           */
static  void  InitTestSet (void);
static  void  TaskStartCreateTasks(void);
static  void  TaskStartDispInit(void);
static  void  TaskStartDisp(void);
static  void  LogTask (void *pdata);
static  const  char  *LogEventName (INT8U event);
static  void  LogTaskName (INT8U taskId, char *name);

/*$PAGE*/
/*
*********************************************************************************************************
*                                                MAIN
*********************************************************************************************************
*/

void  main (void)
{
    PC_DispClrScr(DISP_FGND_WHITE + DISP_BGND_BLACK);      /* Clear the screen                         */

    OSInit();                                              /* Initialize uC/OS-II                      */

    PC_DOSSaveReturn();                                    /* Save environment to return to DOS        */
    PC_VectSet(uCOS, OSCtxSw);                             /* Install uC/OS-II's context switch vector */

    OSTaskCreate(TaskStart, (void *)0, &TaskStartStk[TASK_STK_SIZE - 1], 0);
    OSStart();                                             /* Start multitasking                       */
}

/*
*********************************************************************************************************
*                                              STARTUP TASK
*********************************************************************************************************
*/
void  TaskStart (void *pdata)
{
#if OS_CRITICAL_METHOD == 3                                /* Allocate storage for CPU status register */
    OS_CPU_SR  cpu_sr;
#endif
    char       s[100];
    INT8U  i;
    INT32U current;
    pdata = pdata;                                         /* Prevent compiler warning                 */
    StartSem = OSSemCreate(0);

    OS_ENTER_CRITICAL();
    PC_VectSet(0x08, OSTickISR);                           /* Install uC/OS-II's clock tick ISR        */
    PC_SetTickRate(OS_TICKS_PER_SEC);                      /* Reprogram tick rate                      */
    OS_EXIT_CRITICAL();

    InitTestSet();                                         /* Initialize test set                      */
    TaskStartCreateTasks();                                /* Create all the application tasks         */

    OS_ENTER_CRITICAL();
        current = OSTimeGet();
    OS_EXIT_CRITICAL();

    while (OSTimeGet() == current){
    } 

    TimeIsReset = TRUE;
    OSTimeSet(0); 

    OS_ENTER_CRITICAL();
    for (i = 0; i < Taskcount + 1; i++) { 
        OSSemPost(StartSem);
    }
    OS_EXIT_CRITICAL();

    OSTaskDel(OS_PRIO_SELF);

}

/*
*********************************************************************************************************
*                                              InitTestSet
*********************************************************************************************************
*/

void  InitTestSet (void){
#if TASK_SET_NO == 1
    Taskcount = 2;

    TaskCFG[0].TaskID = 1;
    TaskCFG[0].TaskCompTime = 1;
    TaskCFG[0].TaskPeriod = 3;

    TaskCFG[1].TaskID = 2;
    TaskCFG[1].TaskCompTime = 3;
    TaskCFG[1].TaskPeriod = 6;
#elif TASK_SET_NO == 2 
    Taskcount = 3;

    TaskCFG[0].TaskID = 1;
    TaskCFG[0].TaskCompTime = 1;
    TaskCFG[0].TaskPeriod = 3;

    TaskCFG[1].TaskID = 2;
    TaskCFG[1].TaskCompTime = 3;
    TaskCFG[1].TaskPeriod = 6;

    TaskCFG[2].TaskID = 3;
    TaskCFG[2].TaskCompTime = 4;  
    TaskCFG[2].TaskPeriod = 9;
#endif
}

/*$PAGE*/
/*
*********************************************************************************************************
*                                             CREATE TASKS
*********************************************************************************************************
*/

static  void  TaskStartCreateTasks (void)
{
    INT8U  i;


    for (i = 0; i < Taskcount; i++) {                        /* Create N_TASKS identical tasks           */
        OSTaskCreateExt(
            Task,
            (void *)&TaskCFG[i],
            &TaskStk[i][TASK_STK_SIZE - 1],
            i + 1,
            i + 1,
            &TaskStk[i][0],
            TASK_STK_SIZE,
            (void *)&TaskCFG[i],
            0);
    }

    OSTaskCreateExt(
        LogTask,
        (void *)0,
        &LogStk[LOG_STK_SIZE - 1],
        OS_LOWEST_PRIO - 2,
        OS_LOWEST_PRIO - 2,
        &LogStk[0],
        LOG_STK_SIZE,
        (void *)0,
        0);
}

/*
*********************************************************************************************************
*                                                  TASKS
*********************************************************************************************************
*/

void  Task (void *pdata)
{
    TASKCFG curTask;
    INT32U start;
    INT32U end;
    INT32S toDelay;
    char s[64];
    INT8U err;

    curTask = *(TASKCFG *)pdata;

    OSSemPend(StartSem, 0, &err);
    sprintf(s,"%5u %5lu", OSTCBCur->compTime, OSTCBCur->deadLine);
    PC_DispStr(0, curTask.TaskID,(INT8U *)s, DISP_FGND_RED + DISP_BGND_BLACK);
    start = 0;

    for (;;) {
        OS_ENTER_CRITICAL();
        OSTCBCur->compTime = curTask.TaskCompTime;
        OS_EXIT_CRITICAL();

        while(OSTCBCur->compTime > 0){
        }
        
        end = OSTimeGet();
        toDelay = (INT32S)curTask.TaskPeriod - (INT32S)(end - start);
        
        if (toDelay < 0) {
            sprintf(s, "deadline violation #%u", curTask.TaskID);
            PC_DispStr(0, 24, (INT8U *)s, DISP_FGND_RED + DISP_BGND_BLACK);

            OSTaskDel(OS_PRIO_SELF);
        } else if (toDelay == 0){  
            toDelay = 1;
        }  
        start = start + curTask.TaskPeriod;

        OSTimeDly((INT16U)toDelay);  
    }
}


void  LogTask (void *pdata){
    OS_LOG prec;
    INT16S     key;
    INT8U line = LOG_FIRST_LINE;
    char s[80];
    char from[16];
    char to[16];
    int     log_count;
    INT8U err;
    
    pdata = pdata; 


    fp_log = fopen("NEW_LOG.txt", "w");
    if (fp_log == NULL) {
        PC_DispStr(0, 24, "File Open Error!", DISP_FGND_RED);
    }

    OSSemPend(StartSem, 0, &err);

    for(;;){   

        if (PC_GetKey(&key) == TRUE) {                     /* See if key has been pressed              */
            sprintf(s, "Key: 0x%04X", (unsigned int)key);
            PC_DispStr(60, 24, (INT8U *)s, DISP_FGND_YELLOW + DISP_BGND_BLUE);

            if ((key & 0x00FF) == 0x1B) {                             /* Yes, see if it's the ESCAPE key          */
                if (fp_log != NULL) fclose(fp_log);
                PC_DOSReturn();
            }
        }

        log_count = 0;

        while( log_count < 10 && OSGetLOG(&prec) == TRUE){
            LogTaskName(prec.fromTaskID, from);
            LogTaskName(prec.toTaskID, to);
            sprintf(s, "%5u  %-8s  %-8s -> %-8s   ", 
                prec.time, 
                LogEventName(prec.switchEvent), 
                from, 
                to);
            
            // PC_DispStr(5, line, (INT8U *)s, DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
            if (fp_log != NULL) {
                fprintf(fp_log, "%s\n", s);
            }

            line++;
            log_count++;

            if(line >= LOG_LAST_LINE){
                line = LOG_FIRST_LINE; 
            }
        }
        if (fp_log != NULL) {
            fflush(fp_log); 
        }
        OSTimeDly(5);        
    }
}

static  const  char  *LogEventName (INT8U event)
{
    switch (event) {
        case OS_LOG_EVT_COMPLETE:
             return ("Complete");

        case OS_LOG_EVT_PREEMPT:
             return ("Preempt");

        default:
             return ("Unknown");
    }
}

static  void  LogTaskName (INT8U taskId, char *name)
{
    if (taskId == 63) {
        strcpy(name, "idletask");
    } else {
        sprintf(name, "task%-2u", taskId);
    }
}
