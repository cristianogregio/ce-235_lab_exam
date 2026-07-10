#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "osa1.h"
#include <stdint.h>
#include <stdbool.h>

#define SCHED_NUM_TASKS 5U

#define SCHED_PRIO_HIGH     11U
#define SCHED_PRIO_MEDIUM   12U
#define SCHED_PRIO_REGULAR  13U
#define SCHED_PRIO_LOW      14U
#define SCHED_PRIO_LOWEST   15U

typedef struct
{
  const char *name;
  const char *entry;
  uint8_t id;
  uint16_t osa_priority;
  const char *priority_name;
  uint16_t period_ms;
  uint32_t wcet_ms;
  task_handler_t handler;
  char status[12];
  uint32_t tcomp_ms;
} sched_task_info_t;

void scheduler_init(void);
void scheduler_ensure_started(void);
void scheduler_start(void);
void scheduler_stop(void);
void scheduler_resume(void);
bool scheduler_is_running(void);

void scheduler_register_task(uint8_t task_index, task_handler_t handler);
void scheduler_set_task_status(uint8_t task_index, const char *status);
void scheduler_add_tcomp(uint8_t task_index, uint32_t elapsed_ms);

const sched_task_info_t *scheduler_get_task_info(uint8_t task_index);
uint8_t scheduler_get_task_count(void);

#endif
