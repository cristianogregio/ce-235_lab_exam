/* ###################################################################
**     Filename    : os_tasks.c
**     Project     : lab_exam
**     Processor   : MKL25Z128VLK4
**     Component   : Events
**     Version     : Driver 01.00
**     Compiler    : GNU C Compiler
**     Date/Time   : 2026-07-10, 02:48, # CodeGen: 8
**     Abstract    :
**         This is user's event module.
**         Put your event handler code here.
**     Settings    :
**     Contents    :
**         Task5_task - void Task5_task(os_task_param_t task_init_data);
**         Task4_task - void Task4_task(os_task_param_t task_init_data);
**         Task3_task - void Task3_task(os_task_param_t task_init_data);
**         Task2_task - void Task2_task(os_task_param_t task_init_data);
**         Task1_task - void Task1_task(os_task_param_t task_init_data);
**
** ###################################################################*/
/*!
** @file os_tasks.c
** @version 01.00
** @brief
**         This is user's event module.
**         Put your event handler code here.
*/
/*!
**  @addtogroup os_tasks_module os_tasks module documentation
**  @{
*/
/* MODULE os_tasks */

#include "Cpu.h"
#include "Events.h"
#include "os_tasks.h"
#include "scheduler.h"

#ifdef __cplusplus
extern "C" {
#endif

/* User includes (#include below this line is not maintained by Processor Expert) */

static void periodic_task_body(uint8_t task_index, semaphore_t *sem, uint32_t wcet_ms)
{
  uint32_t start_ms;
  uint32_t end_ms;
  uint32_t elapsed_ms;
  bool registered = false;

#ifdef PEX_USE_RTOS
  while (1) {
#endif
    if (!registered)
    {
      scheduler_register_task(task_index, OSA_TaskGetHandler());
      registered = true;
    }

    scheduler_ensure_started();
    (void)OSA_SemaWait(sem, OSA_WAIT_FOREVER);

    start_ms = OSA_TimeGetMsec();
    OSA_TimeDelay(wcet_ms);
    end_ms = OSA_TimeGetMsec();

    elapsed_ms = end_ms - start_ms;
    if (elapsed_ms == 0U)
    {
      elapsed_ms = wcet_ms;
    }

    scheduler_set_tcomp(task_index, elapsed_ms);

#ifdef PEX_USE_RTOS
  }
#endif
}

/*
** ===================================================================
**     Event       :  Task5_task (module os_tasks)
**
**     Component   :  Task5 [OS_Task]
*/
/* ===================================================================*/
void Task5_task(os_task_param_t task_init_data)
{
  (void)task_init_data;
  periodic_task_body(4U, &task5Sem, 80U);
}

/*
** ===================================================================
**     Event       :  Task4_task (module os_tasks)
**
**     Component   :  Task4 [OS_Task]
*/
/* ===================================================================*/
void Task4_task(os_task_param_t task_init_data)
{
  (void)task_init_data;
  periodic_task_body(3U, &task4Sem, 50U);
}

/*
** ===================================================================
**     Event       :  Task3_task (module os_tasks)
**
**     Component   :  Task3 [OS_Task]
*/
/* ===================================================================*/
void Task3_task(os_task_param_t task_init_data)
{
  (void)task_init_data;
  periodic_task_body(2U, &task3Sem, 15U);
}

/*
** ===================================================================
**     Event       :  Task2_task (module os_tasks)
**
**     Component   :  Task2 [OS_Task]
*/
/* ===================================================================*/
void Task2_task(os_task_param_t task_init_data)
{
  (void)task_init_data;
  periodic_task_body(1U, &task2Sem, 8U);
}

/*
** ===================================================================
**     Event       :  Task1_task (module os_tasks)
**
**     Component   :  Task1 [OS_Task]
*/
/* ===================================================================*/
void Task1_task(os_task_param_t task_init_data)
{
  (void)task_init_data;
  periodic_task_body(0U, &task1Sem, 5U);
}

/* END os_tasks */

#ifdef __cplusplus
}  /* extern "C" */
#endif

/*!
** @}
*/
/*
** ###################################################################
**
**     This file was created by Processor Expert 10.5 [05.21]
**     for the Freescale Kinetis series of microcontrollers.
**
** ###################################################################
*/
