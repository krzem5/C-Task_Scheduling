#ifndef __TASK_SCHEDULING_H__
#define __TASK_SCHEDULING_H__ 1
#include <stdint.h>



#define TASK_OK 0
#define TASK_YIELD 1
#define TASK_START 2
#define TASK_WAIT 3
#define TASK_END 4



#define UNKNOWN_TASK_INDEX 0xffffffff



union __TASK_STATE;



typedef uint8_t task_return_t;



typedef uint32_t task_index_t;



typedef task_return_t (*task_function_t)(union __TASK_STATE* o);



typedef struct __TASK_START_DATA{
	task_function_t fn;
	task_index_t* id;
} task_start_data_t;



typedef union __TASK_STATE{
	task_start_data_t start;
	task_index_t wait;
} task_state_t;



void run_scheduler(task_function_t fn);



#endif
