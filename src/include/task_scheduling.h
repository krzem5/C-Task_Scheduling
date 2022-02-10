#ifndef __TASK_SCHEDULING_H__
#define __TASK_SCHEDULING_H__ 1
#include <stdint.h>



#define TASK_OK 0
#define TASK_YIELD 1
#define TASK_WAIT 2
#define TASK_MTX 3
#define TASK_END 4

#define TASK_DATA(t,v) ((t)|((v)<<3))



union __TASK_STATE;



typedef uint32_t mutex_t;



typedef uint32_t task_index_t;



typedef uint64_t task_return_t;



typedef task_return_t (*task_function_t)(void);



mutex_t create_mutex(void);



task_index_t create_task(task_function_t fn);



void delete_mutex(mutex_t m);



void release_mutex(mutex_t m);



void remove_task(task_index_t id);



void run_scheduler(task_function_t fn);



#endif
