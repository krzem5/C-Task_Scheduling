#include <task_scheduling.h>
#include <stdint.h>
#include <stdlib.h>



#define TIME_QUANTUM 5



typedef struct __TASK{
	task_function_t fn;
} task_t;



void run_scheduler(task_function_t fn){
	uint32_t q_len=0;
	uint32_t q_idx=0;
	task_t* q=NULL;
	task_t task={
		fn
	};
	while (1){
		uint32_t i=0;
		while (i<TIME_QUANTUM){
			i++;
			task_return_t st=task.fn();
			if (st==TASK_OK){
				continue;
			}
			i=0;
			if (st==TASK_END){
				if (!q_len){
					free(q);
					return;
				}
				task=*(q+q_idx);
				for (uint32_t j=q_idx+1;j<q_len;j++){
					*(q+j-1)=*(q+j);
				}
				q_len--;
				if (!q_len){
					q_idx=0;
					free(q);
					q=NULL;
				}
				else{
					if (!q_idx){
						q_idx=q_len;
					}
					q_idx--;
					q=realloc(q,q_len*sizeof(task_t));
				}
			}
			else if (st==TASK_YIELD){
				i=TIME_QUANTUM;
			}
			else{
				q_len++;
				q=realloc(q,q_len*sizeof(task_t));
				*(q+q_len-1)=task;
				task.fn=st;
			}
			break;
		}
		if (i==TIME_QUANTUM&&q_len){
			task_t tmp=*(q+q_idx);
			*(q+q_idx)=task;
			task=tmp;
			if (!q_idx){
				q_idx=q_len;
			}
			q_idx--;
		}
	}
}
