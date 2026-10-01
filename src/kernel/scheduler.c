#include "scheduler.h"
#include "idt.h"
#include "../../include/mm.h"
#include "../../include/panic.h"

extern void task_trampoline(void);

#define MAX_TASKS 16
#define STACK_SIZE 8192
#define NAME_SIZE 24
#define MS_PER_TICK (1000 / TICK_HZ)
#define MAX_WAIT_TICKS 0x7FFFFFF0u

enum task_state {
    TASK_UNUSED,
    TASK_READY,
    TASK_RUNNING,
    TASK_BLOCKED,
    TASK_TERMINATED
};

struct task {
    unsigned int esp;
    unsigned char *stack;
    enum task_state state;
    unsigned int id;
    char name[NAME_SIZE];
    struct wait_queue *queue;
    struct task *wait_next;
    unsigned int deadline;
    int has_deadline;
    int wake_result;
};

static struct task tasks[MAX_TASKS];
static struct task idle_task;
static struct task *current = &tasks[0];
static unsigned int slot_count = 0;
static unsigned int last_slot = 0;
static unsigned int next_id = 1;
static int running_flag = 0;
static volatile unsigned int ticks = 0;

static inline unsigned int irq_save(void) {
    unsigned int flags;
    asm volatile("pushfl; popl %0; cli" : "=r"(flags) : : "memory");
    return flags;
}

static inline void irq_restore(unsigned int flags) {
    asm volatile("pushl %0; popfl" : : "r"(flags) : "memory", "cc");
}

static void copy_name(char *dest, const char *src) {
    unsigned int i = 0;

    if (src) {
        while (src[i] != '\0' && i < NAME_SIZE - 1) {
            dest[i] = src[i];
            i++;
        }
    }
    dest[i] = '\0';
}

static unsigned int build_stack(unsigned char *base, void (*entry)(void)) {
    unsigned short selector;
    unsigned int *sp;

    asm volatile("mov %%cs, %0" : "=r"(selector));

    sp = (unsigned int *)(((unsigned int)(base + STACK_SIZE)) & ~15u);

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

static void idle_entry(void) {
    while (1) {
        asm volatile("hlt");
    }
}

static unsigned int ms_to_ticks(unsigned int ms) {
    unsigned int n = ms / MS_PER_TICK;

    if (ms % MS_PER_TICK != 0) {
        n++;
    }
    if (n > MAX_WAIT_TICKS) {
        n = MAX_WAIT_TICKS;
    }
    return n;
}

static unsigned int deadline_after(unsigned int ms) {
    unsigned int n = ms_to_ticks(ms);

    return ticks + n + (n != 0 ? 1 : 0);
}

static int time_reached(unsigned int now, unsigned int deadline) {
    return (int)(now - deadline) >= 0;
}

static void queue_push(struct wait_queue *q, struct task *t) {
    t->wait_next = 0;
    if (q->tail) {
        q->tail->wait_next = t;
    } else {
        q->head = t;
    }
    q->tail = t;
}

static struct task *queue_pop(struct wait_queue *q) {
    struct task *t = q->head;

    if (!t) {
        return 0;
    }

    q->head = t->wait_next;
    if (!q->head) {
        q->tail = 0;
    }
    t->wait_next = 0;
    return t;
}

static void queue_remove(struct wait_queue *q, struct task *t) {
    struct task *prev = 0;
    struct task *it = q->head;

    while (it && it != t) {
        prev = it;
        it = it->wait_next;
    }

    if (!it) {
        return;
    }

    if (prev) {
        prev->wait_next = it->wait_next;
    } else {
        q->head = it->wait_next;
    }
    if (q->tail == it) {
        q->tail = prev;
    }
    it->wait_next = 0;
}

static void make_ready(struct task *t, int result) {
    t->state = TASK_READY;
    t->queue = 0;
    t->wait_next = 0;
    t->has_deadline = 0;
    t->wake_result = result;
}

static struct task *wake_first(struct wait_queue *q) {
    struct task *t = queue_pop(q);

    if (t) {
        make_ready(t, WAIT_OK);
    }
    return t;
}

static void wake_expired(void) {
    unsigned int i;

    for (i = 0; i < slot_count; i++) {
        struct task *t = &tasks[i];

        if (t->state != TASK_BLOCKED || !t->has_deadline) {
            continue;
        }
        if (!time_reached(ticks, t->deadline)) {
            continue;
        }
        if (t->queue) {
            queue_remove(t->queue, t);
        }
        make_ready(t, WAIT_TIMEOUT);
    }
}

static unsigned int schedule(unsigned int old_esp) {
    struct task *next = &idle_task;
    unsigned int probe = last_slot;
    unsigned int i;

    current->esp = old_esp;
    if (current->state == TASK_RUNNING) {
        current->state = TASK_READY;
    }

    for (i = 0; i < slot_count; i++) {
        probe = (probe + 1) % slot_count;
        if (tasks[probe].state == TASK_READY) {
            next = &tasks[probe];
            last_slot = probe;
            break;
        }
    }

    current = next;
    current->state = TASK_RUNNING;

    return current->esp;
}

static int block_current(struct wait_queue *q, int timed, unsigned int deadline) {
    struct task *t = current;

    if (!running_flag) {
        panic("scheduler: blocking call before scheduler_start");
    }
    if (t == &idle_task) {
        panic("scheduler: idle task must not block");
    }
    if (timed && time_reached(ticks, deadline)) {
        return WAIT_TIMEOUT;
    }

    if (q) {
        queue_push(q, t);
    }
    t->queue = q;
    t->has_deadline = timed;
    t->deadline = deadline;
    t->state = TASK_BLOCKED;

    asm volatile("int $0x81" : : : "memory");

    return t->wake_result;
}

void task_exit(void) {
    current->state = TASK_TERMINATED;

    while (1) {
        asm volatile("int $0x81" : : : "memory");
    }
}

unsigned int scheduler_tick(unsigned int old_esp) {
    if (!running_flag) {
        return old_esp;
    }

    ticks++;
    wake_expired();
    return schedule(old_esp);
}

unsigned int scheduler_switch(unsigned int old_esp) {
    if (!running_flag) {
        return old_esp;
    }

    return schedule(old_esp);
}

void scheduler_init(void) {
    struct task *main_task = &tasks[0];

    main_task->stack = 0;
    main_task->esp = 0;
    main_task->state = TASK_RUNNING;
    main_task->id = next_id++;
    copy_name(main_task->name, "main");

    idle_task.stack = (unsigned char *)kmalloc(STACK_SIZE);
    if (!idle_task.stack) {
        panic("scheduler: out of memory for idle stack");
    }
    idle_task.esp = build_stack(idle_task.stack, idle_entry);
    idle_task.state = TASK_READY;
    idle_task.id = 0;
    copy_name(idle_task.name, "idle");

    slot_count = 1;
    last_slot = 0;
    current = main_task;
    running_flag = 0;
}

void scheduler_start(void) {
    idt_init();
    pic_remap();
    pit_init(TICK_HZ);
    running_flag = 1;
    asm volatile("sti");
}

int task_create(void (*entry)(void), const char *name) {
    unsigned char *stack;
    struct task *t;
    unsigned int flags;
    int slot;

    if (!entry) {
        return -1;
    }

    flags = irq_save();

    if (slot_count >= MAX_TASKS) {
        irq_restore(flags);
        return -1;
    }

    stack = (unsigned char *)kmalloc(STACK_SIZE);
    if (!stack) {
        panic("scheduler: out of memory for task stack");
    }

    slot = (int)slot_count;
    t = &tasks[slot];
    t->stack = stack;
    t->esp = build_stack(stack, entry);
    t->state = TASK_READY;
    t->id = next_id++;
    t->queue = 0;
    t->wait_next = 0;
    t->has_deadline = 0;
    t->wake_result = WAIT_OK;
    copy_name(t->name, name);
    slot_count++;

    irq_restore(flags);
    return slot;
}

void task_yield(void) {
    if (!running_flag) {
        return;
    }
    asm volatile("int $0x81" : : : "memory");
}

void task_sleep(unsigned int ms) {
    unsigned int flags;

    if (ms == 0) {
        task_yield();
        return;
    }

    flags = irq_save();
    block_current(0, 1, deadline_after(ms));
    irq_restore(flags);
}

unsigned int task_current_id(void) {
    return current->id;
}

unsigned int task_count(void) {
    return slot_count;
}

const char *task_current_name(void) {
    return current->name;
}

unsigned int timer_ticks(void) {
    return ticks;
}

unsigned int uptime_ms(void) {
    return ticks * MS_PER_TICK;
}

void wait_queue_init(struct wait_queue *q) {
    q->head = 0;
    q->tail = 0;
}

int wait_queue_wait_until(struct wait_queue *q, int (*ready)(void *), void *arg, unsigned int timeout_ms) {
    unsigned int flags;
    unsigned int deadline;
    int timed = (timeout_ms != WAIT_FOREVER);
    int result;

    if (!q || !ready) {
        panic("scheduler: invalid wait_queue_wait_until arguments");
    }

    flags = irq_save();
    deadline = deadline_after(timeout_ms);

    while (1) {
        if (ready(arg)) {
            result = WAIT_OK;
            break;
        }

        result = block_current(q, timed, deadline);

        if (result == WAIT_TIMEOUT) {
            if (ready(arg)) {
                result = WAIT_OK;
            }
            break;
        }
    }

    irq_restore(flags);
    return result;
}

unsigned int wait_queue_wake_one(struct wait_queue *q) {
    unsigned int flags;
    unsigned int woken;

    if (!q) {
        panic("scheduler: null wait queue");
    }

    flags = irq_save();
    woken = wake_first(q) ? 1 : 0;
    irq_restore(flags);
    return woken;
}

unsigned int wait_queue_wake_all(struct wait_queue *q) {
    unsigned int flags;
    unsigned int woken = 0;

    if (!q) {
        panic("scheduler: null wait queue");
    }

    flags = irq_save();
    while (wake_first(q)) {
        woken++;
    }
    irq_restore(flags);
    return woken;
}

void sem_init(struct semaphore *s, unsigned int count) {
    s->count = count;
    wait_queue_init(&s->waiters);
}

int sem_wait_timeout(struct semaphore *s, unsigned int timeout_ms) {
    unsigned int flags;
    int result;

    if (!s) {
        panic("scheduler: null semaphore");
    }

    flags = irq_save();

    if (s->count > 0) {
        s->count--;
        irq_restore(flags);
        return WAIT_OK;
    }

    result = block_current(&s->waiters, timeout_ms != WAIT_FOREVER, deadline_after(timeout_ms));

    irq_restore(flags);
    return result;
}

void sem_wait(struct semaphore *s) {
    sem_wait_timeout(s, WAIT_FOREVER);
}

int sem_trywait(struct semaphore *s) {
    return sem_wait_timeout(s, 0);
}

void sem_post(struct semaphore *s) {
    unsigned int flags;

    if (!s) {
        panic("scheduler: null semaphore");
    }

    flags = irq_save();

    if (!wake_first(&s->waiters)) {
        if (s->count == 0xFFFFFFFFu) {
            panic("semaphore: count overflow");
        }
        s->count++;
    }

    irq_restore(flags);
}

void mutex_init(struct mutex *m) {
    m->owner = 0;
    wait_queue_init(&m->waiters);
}

int mutex_lock_timeout(struct mutex *m, unsigned int timeout_ms) {
    unsigned int flags;
    int result = WAIT_OK;

    if (!m) {
        panic("scheduler: null mutex");
    }

    flags = irq_save();

    if (m->owner == current) {
        panic("mutex: recursive lock");
    }

    if (!m->owner) {
        m->owner = current;
    } else {
        result = block_current(&m->waiters, timeout_ms != WAIT_FOREVER, deadline_after(timeout_ms));
    }

    irq_restore(flags);
    return result;
}

void mutex_lock(struct mutex *m) {
    mutex_lock_timeout(m, WAIT_FOREVER);
}

int mutex_trylock(struct mutex *m) {
    unsigned int flags;
    int result = WAIT_TIMEOUT;

    if (!m) {
        panic("scheduler: null mutex");
    }

    flags = irq_save();

    if (!m->owner) {
        m->owner = current;
        result = WAIT_OK;
    }

    irq_restore(flags);
    return result;
}

void mutex_unlock(struct mutex *m) {
    unsigned int flags;

    if (!m) {
        panic("scheduler: null mutex");
    }

    flags = irq_save();

    if (m->owner != current) {
        panic("mutex: unlock by non-owner");
    }

    m->owner = wake_first(&m->waiters);

    irq_restore(flags);
}
