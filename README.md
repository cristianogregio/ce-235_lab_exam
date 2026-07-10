# Laboratório Final — CE-235 

Scheduler periódico e shell serial (`SCHED_CE235>`).

**Placa:** FRDM-KL25Z · **Serial:** OpenSDA UART0 @ **115200** 8N1

---

## Tasks e prioridades

| Task  | Período | Prioridade | WCET (Ci) simulado      |
|-------|---------|------------|-------------------------|
| task1 | 20 ms   | HIGH       | 5 ms                    |
| task2 | 40 ms   | MEDIUM     | 8 ms (~10 ms no tick*)  |
| task3 | 100 ms  | REGULAR    | 15 ms                   |
| task4 | 500 ms  | LOW        | 50 ms                   |
| task5 | 1000 ms | LOWEST     | 80 ms                   |


**TCOMP** = tempo da **última** execução (ms), medido com `OSA_TimeGetMsec()` (inclui preempção).  
Não acumula desde o boot. Deve respeitar **Ci ≤ Ti**.

---

## Como compilar e gravar (KDS)

1. Abra o projeto `lab_exam` no **Kinetis Design Studio**.
2. **Project → Build Project** (configuração Debug).
3. Grave com o debugger **OpenSDA / PEMicro** (launch do projeto).
4. Feche o debugger se a COM ficar ocupada.
5. Abra o **PuTTY** na porta COM do OpenSDA.

---

## Como testar no PuTTY

1. Conecte a placa pelo cabo OpenSDA.
2. No PuTTY:
   - Connection type: **Serial**
   - Speed: **115200**
   - Data bits: **8**, Stop bits: **1**, Parity: **None**
   - **Local echo: Off** (o firmware já ecoa o que você digita)
3. Após o reset, deve aparecer:

```text
SCHED_CE235>
```

4. Deixe o scheduler rodar **alguns segundos** (para as tasks executarem e gravarem TCOMP).
5. Digite, um comando por linha:

```text
stop
state
resume
```

---

## Comandos

| Comando  | Efeito |
|----------|--------|
| `stop`   | Para o PIT (ativador periódico), espera o WCET máximo (~90 ms), captura o STATUS e congela as tasks |
| `state`  | Mostra a tabela de tasks (**só funciona com o scheduler parado**) |
| `resume` | Retoma as tasks e liga o PIT de novo |

### Saída esperada

```text
SCHED_CE235> stop
Scheduler stopped.
SCHED_CE235> state
TASK NAME | ENTRY POINT  | ID | PRIORITY | STATUS  | TCOMP(ms)
----------|--------------|----|----------|---------|---------
task1     | Task1_task   |  1 | HIGH     | blocked |        5
task2     | Task2_task   |  2 | MEDIUM   | blocked |       10
task3     | Task3_task   |  3 | REGULAR  | blocked |       15
task4     | Task4_task   |  4 | LOW      | blocked |       50
task5     | Task5_task   |  5 | LOWEST   | blocked |       80
SCHED_CE235> resume
Scheduler resumed.
```

### Como ler a tabela

| Campo | O que significa |
|-------|-----------------|
| **STATUS `blocked`** | Esperado após `stop`: as tasks estão em `OSA_SemaWait` e o PIT não libera mais os semáforos |
| **TCOMP(ms)** | Duração da última ativação (próxima de Ci; deve ser ≤ período) |
| **PRIORITY** | Menor período = maior prioridade (HIGH … LOWEST) |

Se digitar `state` com o scheduler ainda rodando, a shell pede para dar `stop` antes.
