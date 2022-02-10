#include <task_scheduling.h>
#include <stdint.h>
#include <stdlib.h>



#define TIME_QUANTUM 5



typedef struct __TASK{
	task_function_t fn;
	task_index_t id;
	task_index_t w;
} task_t;



void run_scheduler(task_function_t fn){
	task_index_t q_len=0;
	task_index_t q_idx=0;
	task_t* q=NULL;
	task_t task={
		fn,
		0,
		UNKNOWN_TASK_INDEX
	};
	task_index_t n_id=1;
	while (1){
		uint32_t i=0;
		while (i<TIME_QUANTUM){
			i++;
			task_state_t st;
			task_return_t rt=task.fn(&st);
			if (rt==TASK_OK){
				continue;
			}
			i=0;
			if (rt==TASK_YIELD){
				i=TIME_QUANTUM;
			}
			else if (rt==TASK_START){
				q_len++;
				q=realloc(q,q_len*sizeof(task_t));
				*(q+q_len-1)=task;
				task.fn=st.start.fn;
				task.id=n_id;
				task.w=UNKNOWN_TASK_INDEX;
				if (st.start.id){
					*(st.start.id)=n_id;
				}
				n_id++;
			}
			else if (rt==TASK_WAIT){
				i=TIME_QUANTUM;
				if (st.wait==task.id){
					break;
				}
				task_index_t j=0;
				while (j<q_len){
					if ((q+j)->id==st.wait){
						break;
					}
					j++;
				}
				if (j==q_len){
					break;
				}
				task.w=st.wait;
			}
			else{
				if (!q_len){
					free(q);
					return;
				}
				task_index_t c_id=task.id;
				task_index_t j=0;
				while (j<q_idx+1){
					if ((q+j)->w==c_id){
						(q+j)->w=UNKNOWN_TASK_INDEX;
					}
					j++;
				}
				task=*(q+q_idx);
				while (j<q_len){
					*(q+j-1)=*(q+j);
					if ((q+j-1)->w==c_id){
						(q+j-1)->w=UNKNOWN_TASK_INDEX;
					}
					j++;
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
			break;
		}
		if ((i==TIME_QUANTUM||task.w!=UNKNOWN_TASK_INDEX)&&q_len){
			task_t tmp;
			do{
				tmp=*(q+q_idx);
				*(q+q_idx)=task;
				task=tmp;
				if (!q_idx){
					q_idx=q_len;
				}
				q_idx--;
			} while (task.w!=UNKNOWN_TASK_INDEX);
		}
		if (task.w!=UNKNOWN_TASK_INDEX){
			return;
		}
	}
}
