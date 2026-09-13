#include "scheduler.h"
#include "idt.h"
#include "../../include/mm.h"
#include "../../include/panic.h"

extern void task_trampoline(void);

#define MAX_TASKS 16
#define STACK_SIZE 8192
#define NAME_SIZE 24

enum task_state {
    TASK_UNUSED,
    TASK_READY,
    TASK_RUNNING,
    TASK_TERMINATED
};

struct task {
    unsigned int esp;
    unsigned char *stack;
    enum task_state state;
    unsigned int id;
    char name[NAME_SIZE];
};

static struct task tasks[MAX_TASKS];
static unsigned int slot_count = 0;
static unsigned int current = 0;
static unsigned int next_id = 1;
static int running_flag = 0;

static void copy_name(char *dest, const char *src) {
    unsigned int i = 0;
    while (src[i] != '\0' && i < NAME_SIZE - 1) {
        dest[i] = src[i];
        i++;
    }
    dest[i] = '\0';
}

static unsigned int build_stack(unsigned char *top, void (*entry)(void)) {
    unsigned short selector;
    unsigned int *sp;

    asm volatile("mov %%cs, %0" : "=r"(selector));

    sp = (unsigned int *)top;

    *(--sp) = 0x202;
    *(--sp) = selector;
    *(--sp) = (unsigned int)task_trampoline;

    *(--sp) = 0;
    *(--sp) = 0;
    *(--sp) = 0;
    *(--sp) = 0;
    *(--sp) = 0;
    *(--sp) = 0;
    *(--sp) = 0;
    *(--sp) = (unsigned int)entry;

    return (unsigned int)sp;
}

void task_exit(void) {
    tasks[current].state = TASK_TERMINATED;

    while (1) {
        asm volatile("int $0x20");
    }
}

unsigned int scheduler_tick(unsigned int old_esp) {
    unsigned int start;
    unsigned int probe;

    if (!running_flag || slot_count == 0) {
        return old_esp;
    }

    tasks[current].esp = old_esp;
    if (tasks[current].state == TASK_RUNNING) {
        tasks[current].state = TASK_READY;
    }

    start = current;
    probe = start;

    while (1) {
        probe = (probe + 1) % slot_count;

        if (tasks[probe].state == TASK_READY) {
            break;
        }

        if (probe == start) {
            break;
        }
    }

    current = probe;
    tasks[current].state = TASK_RUNNING;

    return tasks[current].esp;
}

void scheduler_init(void) {
    struct task *main_task = &tasks[0];

    main_task->stack = 0;
    main_task->esp = 0;
    main_task->state = TASK_RUNNING;
    main_task->id = next_id++;
    copy_name(main_task->name, "main");

    slot_count = 1;
    current = 0;
    running_flag = 0;
}

void scheduler_start(void) {
    idt_init();
    pic_remap();
    pit_init(100);
    running_flag = 1;
    asm volatile("sti");
}

int task_create(void (*entry)(void), const char *name) {
    unsigned char *stack;
    struct task *t;

    if (slot_count >= MAX_TASKS) {
        return -1;
    }

    stack = (unsigned char *)kmalloc(STACK_SIZE);
    if (!stack) {
        panic("scheduler: out of memory for task stack");
    }

    t = &tasks[slot_count];
    t->stack = stack;
    t->esp = build_stack(stack + STACK_SIZE, entry);
    t->state = TASK_READY;
    t->id = next_id++;
    copy_name(t->name, name);

    return (int)(slot_count++);
}

void task_yield(void) {
    asm volatile("int $0x20");
}

unsigned int task_current_id(void) {
    return tasks[current].id;
}

unsigned int task_count(void) {
    return slot_count;
}

const char *task_current_name(void) {
    return tasks[current].name;
}
