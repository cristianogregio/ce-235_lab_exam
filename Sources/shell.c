#include "shell.h"
#include "scheduler.h"
#include "fsl_debug_console.h"
#include "osa1.h"
#include <string.h>

#define SHELL_CMD_MAX_LEN 16U
#define SHELL_STACK_SIZE  1024U
#define SHELL_PRIORITY    25U
#define SHELL_PROMPT      "SCHED_CE235> "

static OSA_TASK_DEFINE(Shell, SHELL_STACK_SIZE);
static task_handler_t s_shell_handler;

static void shell_read_command(char *cmd, uint8_t max_len)
{
  uint8_t index = 0U;
  int ch;

  cmd[0] = '\0';

  while (index < (max_len - 1U))
  {
    ch = GETCHAR();

    if ((ch == '\r') || (ch == '\n'))
    {
      if ((index == 0U) && (ch == '\n'))
      {
        continue;
      }

      PUTCHAR('\r');
      PUTCHAR('\n');
      cmd[index] = '\0';
      return;
    }

    if ((ch == '\b') || (ch == 127))
    {
      if (index > 0U)
      {
        index--;
        PUTCHAR('\b');
        PUTCHAR(' ');
        PUTCHAR('\b');
      }
      continue;
    }

    cmd[index] = (char)ch;
    PUTCHAR(ch);
    index++;
  }

  cmd[index] = '\0';
}

static void shell_show_task_states(void)
{
  uint8_t i;
  const sched_task_info_t *info;

  PRINTF("%-9s | %-12s | %-2s | %-8s | %-7s | %6s\r\n",
         "TASK NAME", "ENTRY POINT", "ID", "PRIORITY", "STATUS", "TCOMP");
  PRINTF("----------|--------------|----|----------|---------|-------\r\n");

  for (i = 0U; i < scheduler_get_task_count(); i++)
  {
    info = scheduler_get_task_info(i);
    if (info == NULL)
    {
      continue;
    }

    PRINTF("%-9s | %-12s | %2u | %-8s | %-7s | %6u\r\n",
           info->name,
           info->entry,
           (unsigned int)info->id,
           info->priority_name,
           info->status,
           (unsigned int)info->tcomp_ms);
  }
}

static void shell_task(void *param)
{
  char cmd[SHELL_CMD_MAX_LEN];
  (void)param;

  while (1)
  {
    PRINTF(SHELL_PROMPT);
    shell_read_command(cmd, SHELL_CMD_MAX_LEN);

    if (strcmp(cmd, "stop") == 0)
    {
      scheduler_stop();
      PRINTF("Scheduler stopped.\r\n");
    }
    else if (strcmp(cmd, "state") == 0)
    {
      if (scheduler_is_running())
      {
        PRINTF("Stop the scheduler before showing task states.\r\n");
      }
      else
      {
        shell_show_task_states();
      }
    }
    else if (strcmp(cmd, "resume") == 0)
    {
      scheduler_resume();
      PRINTF("Scheduler resumed.\r\n");
    }
    else if (cmd[0] != '\0')
    {
      PRINTF("Unknown command. Use: stop, state, resume\r\n");
    }
  }
}

void shell_init(void)
{
  if (OSA_TaskCreate((task_t)shell_task,
                     (uint8_t *)"shell",
                     SHELL_STACK_SIZE,
                     Shell_stack,
                     SHELL_PRIORITY,
                     (task_param_t)NULL,
                     false,
                     &s_shell_handler) != kStatus_OSA_Success)
  {
    for (;;)
    {
    }
  }
}
