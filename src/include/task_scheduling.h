#ifndef __TASK_SCHEDULING_H__
#define __TASK_SCHEDULING_H__ 1
#include <stdint.h>



#define TASK_OK 0
#define TASK_YIELD 1
#define TASK_WAIT 2
#define TASK_MTX 3
#define TASK_SEM 4
#define TASK_BEQ 5
#define TASK_BGE 6
#define TASK_END 7

#define TASK_DATA(t,v) ((t)|((v)<<3))
#define TASK_DATA2(t,v,n) ((t)|(((task_return_t)(v))<<3)|(((task_return_t)(n))<<35))



union __TASK_STATE;



typedef uint32_t barrier_t;



typedef uint32_t mutex_t;



typedef uint32_t semaphore_t;



typedef uint32_t semaphore_counter_t;



typedef uint32_t task_index_t;



typedef uint64_t task_return_t;



typedef task_return_t (*task_function_t)(void);



barrier_t create_barrier(void);



mutex_t create_mutex(void);



semaphore_t create_semaphore(semaphore_counter_t n);



task_index_t create_task(task_function_t fn);



void delete_barrier(barrier_t b);



void delete_mutex(mutex_t m);



void delete_semaphore(semaphore_t s);



void increase_barrier(barrier_t b);



void release_mutex(mutex_t m);



void release_semaphore(semaphore_t s);



void remove_task(task_index_t id);



void reset_barrier(barrier_t b);



void run_scheduler(task_function_t fn);



#endif
