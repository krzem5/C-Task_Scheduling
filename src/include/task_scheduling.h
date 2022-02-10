#ifndef __TASK_SCHEDULING_H__
#define __TASK_SCHEDULING_H__ 1
#include <stdint.h>



#define TASK_OK 0
#define TASK_YIELD 1
#define TASK_WAIT 2
#define TASK_END 3

#define TASK_DATA(t,v) (((v)<<2)|(t))



union __TASK_STATE;



typedef uint64_t task_return_t;



typedef uint32_t task_index_t;



typedef task_return_t (*task_function_t)(void);



task_index_t create_task(task_function_t fn);



void remove_task(task_index_t id);



void run_scheduler(task_function_t fn);



#endif
