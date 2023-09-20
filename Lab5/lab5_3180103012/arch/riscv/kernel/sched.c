/**
 * @file sched.c
 * @author Scott Chen
 * @brief the scheduler implementation of oslab4
 * @version 0.2
 * @date 2020-12-05
 * @ref https://gitee.com/zjuicsr/lab20fall-stu/wikis/lab4
 */
#include "sched.h"
#include "put.h"
#include "rand.h"
#include "syscall.h"
#include "vm.h"

#define write_csr(reg, val) ({ asm volatile("csrw " #reg ", %0" ::"rK"(val)); })
#define read_csr(__reg)                   \
    ({                                    \
        uint64 __tmp;                     \
        asm volatile("csrr t0, " #__reg); \
        asm("sd t0, %0" : : "m"(__tmp));  \
        __tmp;                            \
    })

struct task_struct *current;
struct task_struct *task[NR_TASKS];

/**
 * @brief init tasks, create 4 threads running dead-loop
 */
void task_init(void) {
    current = (struct task_struct *)TASK_BASE;
    for (int i = 0; i <= LAB_TEST_NUM; i++) {
        task[i]           = (struct task_struct *)(long)(TASK_BASE + TASK_SIZE * i);
        task[i]->state    = TASK_RUNNING;
        task[i]->counter  = i == 0 ? 0 : (PREEMPT_ENABLE ? 8 - i : rand());
        task[i]->priority = 5;
        task[i]->blocked  = 0;
        task[i]->pid      = i;

        task[i]->thread.sp = TASK_BASE + TASK_SIZE * (i + 1);
        asm("la t0, thread_init");
        asm("sd t0, %0" : : "m"(task[i]->thread.ra));

        task[i]->sscratch = (size_t)task[i]->thread.sp;

        task[i]->mm.rtpg_addr = user_paging_init();

        if (i != 0)
#if PREEMPT_ENABLE == 0  // SJF
            putf("[PID = %d] Process Create Successfully! counter = %d\n", task[i]->pid,
                task[i]->counter);
#else  // PRIORITY
            putf("[PID = %d] Process Create Successfully! counter = %d priority = %d\n",
                task[i]->pid, task[i]->counter, task[i]->priority);
#endif
    }
    asm("ld t0, %0" : : "m"(task[0]->sscratch));
    asm("csrw sscratch, t0");
    // write_csr(sscratch, task[0]->sscratch);
}

/**
 * @brief called by timer int
 */
void do_timer(void) {
#if PREEMPT_ENABLE == 0  // SJF
    // Print thread info for SJF
    // putf("[PID = %d] Context Calculation: counter = %d\n", current->pid, current->counter);

    // Decrease counter and schedule
    current->counter--;
    if (current->counter <= 0)
        schedule();

#else  // PRIORITY
    current->counter--;
    if (current->counter <= 0)
        current->counter = (current->pid == 0) ? 5 : 8 - current->pid;
    schedule();
#endif
}

/**
 * @brief context switch from current to next
 *
 * @param next
 */
void switch_to(struct task_struct *next) {
    if (current == next)
        return;

    asm("addi sp, sp, 32");  // Restore the stack of switch_to
    CONTEXT_SAVE(current);   // Do context save

    asm("csrr t0, sscratch");
    asm("sd t0, %0" : : "m"(current->sscratch));
    // current->sscratch = read_csr(sscratch);

    current = next;  // `next` in $s0(-O0), will be overwrite soon

    asm("ld t0, %0" : : "m"(current->sscratch));
    asm("csrw sscratch, t0");
    // write_csr(sscratch, current->sscratch);

    asm("ori t0, zero, 8");
    asm("sll t0, t0, 16");
    asm("ori t0, t0, 0");
    asm("sll t0, t0, 44");
    asm("ld t1, %0" : : "m"(current->mm.rtpg_addr));
    asm("srl t1, t1, 12");
    asm("or t0, t0, t1");
    asm("csrw satp, t0");
    asm("sfence.vma");

    CONTEXT_LOAD(current);  // This `current` is the argv `next`

    asm("ret");
}

/**
 * @brief schedule implementation
 */
void schedule(void) {
#if PREEMPT_ENABLE == 0               // SJF
    int i_min_cnt    = LAB_TEST_NUM;  // index of min but not zero counter
    _Bool all_zeroes = 1;
    for (int i = LAB_TEST_NUM; i > 0; i--)
        if (task[i]->state == TASK_RUNNING) {
            if (task[i]->counter > 0 && task[i]->counter < task[i_min_cnt]->counter ||
                task[i_min_cnt]->counter == 0)
                i_min_cnt = i;
            if (task[i]->counter > 0)  // In case of negative cnt
                all_zeroes = 0;
        }
    if (all_zeroes) {
        for (int i = 1; i <= LAB_TEST_NUM; i++)
            if (task[i]->state == TASK_RUNNING) {
                task[i]->counter = rand();
                // putf("[PID = %d] Reset counter = %d\n", task[i]->pid, task[i]->counter);
            }
        schedule();
    } else {
        putf("[!] Switch from task %d[%lx] to task %d[%lx], prio: %d, counter: %d\n",
            current->pid, (unsigned long)current, task[i_min_cnt]->pid,
            (unsigned long)task[i_min_cnt], task[i_min_cnt]->priority,
            task[i_min_cnt]->counter);
        switch_to(task[i_min_cnt]);
    }

#else  // PRIORITY
    int max_pri = __INT_MAX__, i_min_cnt = 1;
    for (int i = 1; i <= LAB_TEST_NUM; i++)
        if (task[i]->state == TASK_RUNNING)
            if (task[i]->priority < max_pri) {
                i_min_cnt = i;
                max_pri = task[i]->priority;
            } else if (task[i]->priority == max_pri &&
                       task[i]->counter < task[i_min_cnt]->counter && task[i]->counter > 0)
                i_min_cnt = i;

    // Must be printed here to meet demands, else the printed info is out-dated
    putf("[!] Switch from task %d[%lx] to task %d[%lx], prio: %d, counter: %d\n",
        current->pid, (unsigned long)current, task[i_min_cnt]->pid,
        (unsigned long)task[i_min_cnt], task[i_min_cnt]->priority, task[i_min_cnt]->counter);

    // Use another loop to update prio
    for (int i = 1; i <= LAB_TEST_NUM; i++)
        if (task[i]->state == TASK_RUNNING)
            task[i]->priority = rand();

    // Print all threads' info for PRIORITY
    // puts("tasks' priority changed\n");
    for (int i = 1; i <= LAB_TEST_NUM; i++)
        if (task[i]->state == TASK_RUNNING) {
            // putf("[PID = %d] counter = %d priority = %d\n", task[i]->pid, task[i]->counter,
            // task[i]->priority);
        }
    switch_to(task[i_min_cnt]);
#endif
}
