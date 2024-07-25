#include "co.h"
#include <stdlib.h>
#include <stdint.h>
#include <setjmp.h>
#include <malloc.h>
#include <time.h>

#undef LOCAL_MACHINE
#ifdef LOCAL_MACHINE
    #define debug(...) printf(__VA_ARGS__)
#else
    #define debug(...)
#endif

#define STACK_SIZE  (64 * 1024)

struct list_head {
	struct list_head *next, *prev;
};

#define LIST_HEAD_INIT(name) { &(name), &(name) }

#define LIST_HEAD(name) \
	struct list_head name = LIST_HEAD_INIT(name)

#define INIT_LIST_HEAD(ptr) do { \
	(ptr)->next = (ptr); (ptr)->prev = (ptr); \
} while (0)

typedef enum co_status {
    CO_NEW = 1, // 创建未执行
    CO_RUNNING, // 已经执行过
    CO_WAITING, // 等待完成
    CO_DEAD,    // 已经结束, 没有释放资源
} co_status_t;

// typedef struct context {
//     uint64_t    rax, rbx, rcx, rdx;     // 通用寄存器
//     uint64_t    rsi, rdi;               // 字符串操作寄存器
//     uint64_t    rbp, rsp;               // 栈指针寄存器
//     uint64_t    r8, r9, r10, r11, 
//     uint64_t    r12, r13, r14, r15      // 扩展通用寄存器
//     uint64_t    rip;                    // PC 指针
//     uint64_t    rflags;                 // 状态寄存器
//     uint64_t    cr3;
// } co_context_t;

typedef jmp_buf co_context_t;

struct co {
    struct list_head    co_list;

    const char      *name;
    void           (*func)(void *);
    void            *arg;

    co_status_t     status;             // 协程状态
    struct co       *wait;              // 哪条协程在等待本协程
    co_context_t    context;            // 上下文
    uint8_t         stack[STACK_SIZE];  // 协程的堆栈
};

struct co         *co_current;
struct list_head  *co_list_head = NULL;
int                co_list_num  = 0;

/*
 * Insert a new entry between two known consecutive entries.
 *
 * This is only for internal list manipulation where we know
 * the prev/next entries already!
 */
static inline void __list_add(struct list_head *new,
			      struct list_head *prev,
			      struct list_head *next)
{
	next->prev = new;
	new->next = next;
	new->prev = prev;
	prev->next = new;
}

/**
 * list_add - add a new entry
 * @new: new entry to be added
 * @head: list head to add it after
 *
 * Insert a new entry after the specified head.
 * This is good for implementing stacks.
 */
static inline void list_add(struct list_head *new, struct list_head *head)
{
	__list_add(new, head, head->next);
}

/**
 * list_add_tail - add a new entry
 * @new: new entry to be added
 * @head: list head to add it before
 *
 * Insert a new entry before the specified head.
 * This is useful for implementing queues.
 */
static inline void list_add_tail(struct list_head *new, struct list_head *head)
{
	__list_add(new, head->prev, head);
}

/*
 * Delete a list entry by making the prev/next entries
 * point to each other.
 *
 * This is only for internal list manipulation where we know
 * the prev/next entries already!
 */
static inline void __list_del(struct list_head *prev, struct list_head *next)
{
	next->prev = prev;
	prev->next = next;
}

/**
 * list_del - deletes entry from list.
 * @entry: the element to delete from the list.
 * Note: list_empty on entry does not return true after this, the entry is in an undefined state.
 */
static inline void list_del(struct list_head *entry)
{
	__list_del(entry->prev, entry->next);
	entry->next = (void *) 0;
	entry->prev = (void *) 0;
}

/**
 * list_entry - get the struct for this entry
 * @ptr:	the &struct list_head pointer.
 * @type:	the type of the struct this is embedded in.
 * @member:	the name of the list_struct within the struct.
 */
#define list_entry(ptr, type, member) \
	((type *)((char *)(ptr)-(unsigned long)(&((type *)0)->member)))

/**
 * list_for_each	-	iterate over a list
 * @pos:	the &struct list_head to use as a loop counter.
 * @head:	the head for your list.
 */
#define list_for_each(pos, head) \
	for (pos = (head)->next; pos != (head); \
        	pos = pos->next)
/**
 * list_for_each_prev	-	iterate over a list backwards
 * @pos:	the &struct list_head to use as a loop counter.
 * @head:	the head for your list.
 */
#define list_for_each_prev(pos, head) \
	for (pos = (head)->prev; pos != (head); \
        	pos = pos->prev)

/**
 * list_for_each_safe	-	iterate over a list safe against removal of list entry
 * @pos:	the &struct list_head to use as a loop counter.
 * @n:		another &struct list_head to use as temporary storage
 * @head:	the head for your list.
 */
#define list_for_each_safe(pos, n, head) \
	for (pos = (head)->next, n = pos->next; pos != (head); \
		pos = n, n = pos->next)

/**
 * list_for_each_entry	-	iterate over list of given type
 * @pos:	the type * to use as a loop counter.
 * @head:	the head for your list.
 * @member:	the name of the list_struct within the struct.
 */
#define list_for_each_entry(pos, head, member)				\
	for (pos = list_entry((head)->next, typeof(*pos), member);	\
	     &pos->member != (head); 					\
	     pos = list_entry(pos->member.next, typeof(*pos), member))

/**
 * list_for_each_entry_safe - iterate over list of given type safe against removal of list entry
 * @pos:	the type * to use as a loop counter.
 * @n:		another type * to use as temporary storage
 * @head:	the head for your list.
 * @member:	the name of the list_struct within the struct.
 */
#define list_for_each_entry_safe(pos, n, head, member)			\
	for (pos = list_entry((head)->next, typeof(*pos), member),	\
		n = list_entry(pos->member.next, typeof(*pos), member);	\
	     &pos->member != (head); 					\
	     pos = n, n = list_entry(n->member.next, typeof(*n), member))

__attribute__((constructor)) 
void co_init (void)  {
    struct co *co_main = (struct co *)malloc(sizeof(struct co));

    co_main->name   = "main";
    co_main->func   = NULL;       // main
    co_main->arg    = NULL;

    co_main->status = CO_RUNNING;
    co_main->wait   = NULL;

    co_list_head    = &co_main->co_list;
    INIT_LIST_HEAD(co_list_head);
    co_list_num++;

    co_current = co_main;

    srand(time(NULL));
}

// 这里的对齐很关键, 这里的对齐会将协程的 rsp 栈顶指针进行对齐, 对齐之后后续执行才不会出错误
// __attribute__((force_align_arg_pointer))
void co_warpper (struct co *co) {
    co->func(co->arg);
    co->status = CO_DEAD;
    while (1) {
        co_yield();
    }
}

struct co *co_start(const char *name, void (*func)(void *), void *arg) {
    struct co *new = (struct co *)malloc(sizeof(struct co));

    new->name = name;
    new->func = func;
    new->arg  = arg;

    new->status = CO_NEW;
    new->wait   = NULL;

    debug("%p - %p\n", new->stack, new->stack + STACK_SIZE);

    list_add(&new->co_list, co_list_head);
    co_list_num++;

    debug("[%d]: %p\n", co_list_num, new);

    co_yield();

    return new;
}

void co_wait(struct co *co) {
    while (co->status != CO_DEAD) {   
        co_yield();
    }

    list_del(&co->co_list);
    free(co);
}

static inline void
stack_switch_call(void *sp, void *entry, uintptr_t arg);

void co_yield() {
    int next_num;
    int val = setjmp(co_current->context);
    struct co *co_next, *co_pos;

    if (val == 0) {
        // 选择下一个待运行的协程 (相当于修改 current), 并切换到这个协程运行
        next_num = (rand() % co_list_num) + 1;

        debug("next_num = %d\n", next_num);

        list_for_each_entry(co_pos, co_list_head, co_list) {
            if (!(--next_num)) {
                break;
            }
        }

        co_current = co_pos;
        debug("co_current = %s\n", co_current->name);

        if (co_pos->status == CO_NEW) {
            co_pos->status = CO_RUNNING;
            stack_switch_call(co_pos->stack + STACK_SIZE, co_warpper, (uintptr_t)co_pos);
        }
        longjmp(co_pos->context, 1);
    } else {
        // 协程切换以及完成, 继续运行切换后的程序
    }
}

static inline void
stack_switch_call(void *sp, void *entry, uintptr_t arg) {
    __asm__ volatile (
#if __x86_64__
        "movq %0, %%rsp; movq %2, %%rdi; jmp *%1"
          :
          : "b"((uintptr_t)sp),
            "d"(entry),
            "a"(arg)
          : "memory"
#else
        "movl %0, %%esp; movl %2, 4(%0); jmp *%1"
          :
          : "b"((uintptr_t)sp - 8),
            "d"(entry),
            "a"(arg)
          : "memory"
#endif
    );
}