#include "co.h"
#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>
#include <stdint.h>
#include <assert.h>

#define STACK_SIZE 64 * 1024

enum {
  CO_RUNNING,
  CO_DEAD,
};

struct _co_arg {
  void (*func)(void *);
  void *arg;
  jmp_buf back;
};

struct node {
  struct node *next;
  struct co *co;
};

struct co {
  jmp_buf buf;
  int status;
  union {
    struct _co_arg arg;
    uint8_t stack[STACK_SIZE];
  };
} __attribute__((aligned(16)));

// the list {{{
struct list {
  int size;
  struct node *head;
};
static void list_init(struct list *list) {
  list->size = 0;
  list->head = NULL;
}
static void list_insert(struct list *list, struct co *co) {
  struct node *node = malloc(sizeof(struct node));
  node->co = co;
  node->next = list->head;
  list->head = node;
  list->size++;
}
static struct co *list_remove(struct list *list, struct co *co) {
  struct node *prev = NULL;
  struct node *node = list->head;
  while (node) {
    if (node->co == co) {
      if (prev) {
        prev->next = node->next;
      } else {
        list->head = node->next;
      }
      list->size--;
      free(node);
      return co;
    }
    prev = node;
    node = node->next;
  }
  return NULL;
}
static struct co *list_index(struct list *list, int index) {
  struct node *node = list->head;
  while (index-- && node) {
    node = node->next;
  }
  return node ? node->co : NULL;
}
// the list }}}

static struct list list;

static struct co *current = NULL;
static jmp_buf main_buf;

void __attribute__((constructor)) co_init() {
  list_init(&list);
}

void __attribute__((force_align_arg_pointer)) wrapper(void *arg) {
  struct co *co = arg;
  if (setjmp(co->buf) == 0) {
    co->status = CO_RUNNING;
    longjmp(co->arg.back, 1);
  } else {
    co->arg.func(co->arg.arg);
    co->status = CO_DEAD;
    while(1) co_yield();
  }
}

static inline void stack_switch_call(void *sp, void *entry, void *arg) {
  asm volatile(
#if __x86_64__
      "movq %0, %%rsp; movq %2, %%rdi; jmp *%1"
      :
      : "b"((uintptr_t)sp), "d"(entry), "a"((uintptr_t)arg)
      : "memory"
#else
      "movl %0, %%esp; movl %2, 4(%0); jmp *%1"
      :
      : "b"((uintptr_t)sp - 8), "d"(entry), "a"((uintptr_t)arg)
      : "memory"
#endif
  );
}

struct co *co_start(const char *name, void (*func)(void *), void *arg) {
  struct co *co = malloc(sizeof(struct co));
  co->status = CO_RUNNING;
  co->arg.func = func;
  co->arg.arg = arg;
  list_insert(&list, co);
  if (setjmp(co->arg.back) == 0) {
    stack_switch_call(co->stack + STACK_SIZE, wrapper, co);
    assert(0); // never reach here
  }
  return co;
}

void co_wait(struct co *co) {
  while (co->status != CO_DEAD) {
    co_yield();
  }
  free(list_remove(&list, co));
}

void co_yield () {
  if ((current == NULL ? setjmp(main_buf) : setjmp(current->buf)) == 0) {
    int next = rand() % (list.size + 1);
    current = next == list.size ? NULL : list_index(&list, next);
    if (current == NULL) {
      longjmp(main_buf, 1);
    } else {
      longjmp(current->buf, 1);
    }
    assert(0);
  }
}