#ifndef __TASK_SCHEDULING_H__
#define __TASK_SCHEDULING_H__ 1
#include <stdint.h>



#define TASK_OK ((task_function_t)(void*)0)
#define TASK_YIELD ((task_function_t)(void*)1)
#define TASK_END ((task_function_t)(void*)2)



typedef void* task_return_t;



typedef task_return_t (*task_function_t)(void);



void run_scheduler(task_function_t fn);



#endif
