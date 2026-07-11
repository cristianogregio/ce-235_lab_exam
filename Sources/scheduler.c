#include "scheduler.h"
#include "Events.h"
#include "clockMan1.h"
#include "fsl_interrupt_manager.h"
#include "MKL25Z4.h"
#include "task.h"
#include <string.h>

// tick base de interrup??o: 20ms = menor periodo (task1)
#define SCHED_BASE_TICK_MS   20U
#define SCHED_PIT_CHANNEL    0U

static volatile uint32_t s_tick_count = 0U;
static volatile bool s_scheduler_running = false;
static bool s_tasks_suspended = false;
static bool s_auto_start_done = false;  /* so liga o PIT uma vez; stop nao pode ser desfeito pelas tasks */

/* tabela que o shell usa no comando state */
static sched_task_info_t s_tasks[SCHED_NUM_TASKS] =
{
  { "task1", "Task1_task", 1U, SCHED_PRIO_HIGH,    "HIGH",    20U,   5U, NULL, "blocked", 0U },
  { "task2", "Task2_task", 2U, SCHED_PRIO_MEDIUM,  "MEDIUM",  40U,   8U, NULL, "blocked", 0U },
  { "task3", "Task3_task", 3U, SCHED_PRIO_REGULAR, "REGULAR", 100U, 15U, NULL, "blocked", 0U },
  { "task4", "Task4_task", 4U, SCHED_PRIO_LOW,     "LOW",     500U, 50U, NULL, "blocked", 0U },
  { "task5", "Task5_task", 5U, SCHED_PRIO_LOWEST,  "LOWEST",  1000U, 80U, NULL, "blocked", 0U }
};

// configura PIT pra disparar a cada 20ms (timer do enunciado)
static void scheduler_pit_init(void) {
	uint32_t pit_freq;
	uint32_t load_value;

	CLOCK_SYS_EnablePitClock(0U);
	pit_freq = CLOCK_SYS_GetPitFreq(0U);
	load_value = (pit_freq / 1000U) * SCHED_BASE_TICK_MS;

	if (load_value > 0U) {
		load_value--; // Interrupcao conta de LDVAL ate 0
	}

	PIT->MCR = 0U;
	PIT->CHANNEL[SCHED_PIT_CHANNEL].LDVAL = load_value;
	PIT->CHANNEL[SCHED_PIT_CHANNEL].TCTRL = PIT_TCTRL_TIE(1U);
	PIT->CHANNEL[SCHED_PIT_CHANNEL].TFLG = PIT_TFLG_TIF(1U);

	NVIC_SetPriority(PIT_IRQn, 3U); /* prioridade baixa = pode chamar OSA na ISR */
	INT_SYS_EnableIRQ(PIT_IRQn);
}

static void scheduler_pit_enable(bool enable) {
	if (enable) {
		PIT->CHANNEL[SCHED_PIT_CHANNEL].TFLG = PIT_TFLG_TIF(1U);
		PIT->CHANNEL[SCHED_PIT_CHANNEL].TCTRL |= PIT_TCTRL_TEN(1U);
	} else {
		PIT->CHANNEL[SCHED_PIT_CHANNEL].TCTRL &=
				(uint32_t) (~PIT_TCTRL_TEN_MASK);
	}
}

/* libera semaforo so quando o tick bate o periodo da task */
static void scheduler_post_if_due(uint8_t task_index, uint32_t period_ticks) {
	if ((s_tick_count % period_ticks) == 0U) {
		switch (task_index) {
		case 0U:
			(void) OSA_SemaPost(&task1Sem);
			break;  //T1=20ms  -> todo tick
		case 1U:
			(void) OSA_SemaPost(&task2Sem);
			break;  // T2=40ms  -> a cada 2
		case 2U:
			(void) OSA_SemaPost(&task3Sem);
			break;  // T3=100ms -> a cada 5
		case 3U:
			(void) OSA_SemaPost(&task4Sem);
			break;  // T4=500ms -> a cada 25
		case 4U:
			(void) OSA_SemaPost(&task5Sem);
			break;  // T 5=1000ms-> a cada 50
		default:
			break;
		}
	}
}

// Interrupção do ISR
void PIT_IRQHandler(void) {
	if ((PIT->CHANNEL[SCHED_PIT_CHANNEL].TFLG & PIT_TFLG_TIF_MASK) != 0U) {
		PIT->CHANNEL[SCHED_PIT_CHANNEL].TFLG = PIT_TFLG_TIF(1U);

		if (s_scheduler_running) {
			s_tick_count++;
			scheduler_post_if_due(0U, 1U);
			scheduler_post_if_due(1U, 2U);
			scheduler_post_if_due(2U, 5U);
			scheduler_post_if_due(3U, 25U);
			scheduler_post_if_due(4U, 50U);
		}
	}
}

/* semaforo + timer - roda antes do RTOS subir */
void scheduler_init(void) {
	(void) OSA_SemaCreate(&task1Sem, 0U);
	(void) OSA_SemaCreate(&task2Sem, 0U);
	(void) OSA_SemaCreate(&task3Sem, 0U);
	(void) OSA_SemaCreate(&task4Sem, 0U);
	(void) OSA_SemaCreate(&task5Sem, 0U);

	scheduler_pit_init();
}

void scheduler_start(void) {
	s_tick_count = 0U;
	s_scheduler_running = true;
	scheduler_pit_enable(true);
}

/*
 * Liga o PIT so na primeira vez.
 * Nao pode chamar scheduler_start() de novo apos stop ? senao, durante o
 * OSA_TimeDelay(90) do stop, alguma task periodica religa o scheduler e o
 * state continua pedindo stop.
 */
void scheduler_ensure_started(void) {
	if (!s_auto_start_done) {
		s_auto_start_done = true;
		scheduler_start();
	}
}

// congela as tasks pra tabela nao mudar entre stop e state
static void scheduler_suspend_periodic_tasks(void) {
	uint8_t i;

	for (i = 0U; i < SCHED_NUM_TASKS; i++) {
		if (s_tasks[i].handler != NULL) {
			vTaskSuspend(s_tasks[i].handler);
		}
	}

	s_tasks_suspended = true;
}

static void scheduler_resume_periodic_tasks(void) {
	uint8_t i;

	if (s_tasks_suspended) {
		for (i = 0U; i < SCHED_NUM_TASKS; i++) {
			if (s_tasks[i].handler != NULL) {
				vTaskResume(s_tasks[i].handler);
			}
		}

		s_tasks_suspended = false;
	}
}

// Conversão do enum
static const char *scheduler_state_to_string(eTaskState state) {
	switch (state) {
	case eRunning:
		return "running";
	case eReady:
		return "ready";
	case eBlocked:
		return "blocked";
	case eSuspended:
		return "suspended";
	case eDeleted:
		return "deleted";
	default:
		return "unknown";
	}
}

// le estado REAL do RTOS no instante do stop
static void scheduler_snapshot_task_states(void) {
	uint8_t i;

	for (i = 0U; i < SCHED_NUM_TASKS; i++) {
		if (s_tasks[i].handler != NULL) {
			scheduler_set_task_status(i, scheduler_state_to_string(eTaskGetState(s_tasks[i].handler)));
		}
	}
}

/*
    Stop: para o ativador, espera as tasks voltarem ao SemaWait (blocked), grava STATUS, depois suspende so pra congelar ate o resume.
    Ou seja:
    1 - A interrupção para: não libera mais os semáforos
    2 - As tasks terminam o que estavam fazendo e voltam para OSA_SemaWait
    3 - Sem novo SemaPost, o FreeRTOS as marca como blocked (esperando o semáforo)
    4 - O state mostra esse snapshot
 */
void scheduler_stop(void) {
	s_scheduler_running = false;
	scheduler_pit_enable(false);

	/* da tempo da task5 terminar (WCET max = 80ms) */
	OSA_TimeDelay(90U);

	/* snapshot ANTES do suspend -> STATUS = blocked (em SemaWait) */
	scheduler_snapshot_task_states();
	scheduler_suspend_periodic_tasks();
}

void scheduler_resume(void) {
	scheduler_resume_periodic_tasks();
	s_scheduler_running = true;
	scheduler_pit_enable(true);
}

bool scheduler_is_running(void) {
	return s_scheduler_running;
}

//   task se registra na 1a execucao e ganha prioridade certa
void scheduler_register_task(uint8_t task_index, task_handler_t handler) {
	if (task_index < SCHED_NUM_TASKS) {
		s_tasks[task_index].handler = handler;
		(void) OSA_TaskSetPriority(handler, s_tasks[task_index].osa_priority);
	}
}

void scheduler_set_task_status(uint8_t task_index, const char *status) {
	if ((task_index < SCHED_NUM_TASKS) && (status != NULL)) {
		(void) strncpy(s_tasks[task_index].status, status,
				sizeof(s_tasks[task_index].status) - 1U);
		s_tasks[task_index].status[sizeof(s_tasks[task_index].status) - 1U] = '\0';
	}
}

/* TCOMP = ultima execucao (nao acumula!) */
void scheduler_set_tcomp(uint8_t task_index, uint32_t elapsed_ms) {
	if (task_index < SCHED_NUM_TASKS) {
		s_tasks[task_index].tcomp_ms = elapsed_ms;
	}
}

const sched_task_info_t *scheduler_get_task_info(uint8_t task_index) {
	if (task_index < SCHED_NUM_TASKS) {
		return &s_tasks[task_index];
	}

	return NULL;
}

uint8_t scheduler_get_task_count(void) {
	return SCHED_NUM_TASKS;
}
