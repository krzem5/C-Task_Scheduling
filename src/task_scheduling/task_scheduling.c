#include <task_scheduling.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>



#define TASK_INSTRUCTION_COUNT 50

#define TASK_ALLOCATION_COUNT 256

#define QUEUE_ALLOCATION_COUNT 256

#define WAIT_LIST_ALLOCATION_COUNT 256
#define WAIT_LIST_ALLOCATION_SIZE(l) (((l)+255)&0xffffff00)



typedef struct __TASK{
	task_function_t fn;
	task_index_t w;
} task_t;



typedef struct __TASK_LIST{
	task_t* dt;
	task_index_t max;
	task_index_t len;
	task_index_t idx;
} task_list_t;



typedef struct __QUEUE{
	task_index_t* dt;
	task_index_t max;
	task_index_t len;
	task_index_t idx;
} queue_t;



typedef struct __WAIT_LIST{
	task_index_t* dt;
	task_index_t max;
	task_index_t len;
} wait_list_t;



typedef struct __SCHEDULER{
	task_list_t tl;
	queue_t q;
	wait_list_t wl;
} scheduler_t;



static scheduler_t* _scheduler_data=NULL;



static task_index_t _create_task(task_function_t fn){
	task_index_t o=0;
	while (o<_scheduler_data->tl.len){
		if (!(_scheduler_data->tl.dt+o)->fn){
			break;
		}
		o++;
	}
	if (o==_scheduler_data->tl.len){
		_scheduler_data->tl.len++;
		if (_scheduler_data->tl.len>=_scheduler_data->tl.max){
			_scheduler_data->tl.max+=TASK_ALLOCATION_COUNT;
			_scheduler_data->tl.dt=realloc(_scheduler_data->tl.dt,_scheduler_data->tl.max*sizeof(task_t));
		}
	}
	(_scheduler_data->tl.dt+o)->fn=fn;
	(_scheduler_data->tl.dt+o)->w=UNKNOWN_TASK_INDEX;
	return o;
}



static void _queue_task(task_index_t t){
	_scheduler_data->q.len++;
	if (_scheduler_data->q.len==_scheduler_data->q.max){
		_scheduler_data->q.max+=QUEUE_ALLOCATION_COUNT;
		_scheduler_data->q.dt=realloc(_scheduler_data->q.dt,_scheduler_data->q.max*sizeof(task_index_t));
	}
	*(_scheduler_data->q.dt+_scheduler_data->q.len-1)=t;
}



static task_index_t _remove_queue_task(void){
	task_index_t o=*(_scheduler_data->q.dt+_scheduler_data->q.idx);
	for (task_index_t j=_scheduler_data->q.idx+1;j<_scheduler_data->q.len;j++){
		*(_scheduler_data->q.dt+j-1)=*(_scheduler_data->q.dt+j);
	}
	_scheduler_data->q.len--;
	if (!_scheduler_data->q.len){
		_scheduler_data->q.idx=0;
	}
	else{
		if (!_scheduler_data->q.idx){
			_scheduler_data->q.idx=_scheduler_data->q.len;
		}
		_scheduler_data->q.idx--;
		if (_scheduler_data->q.max>QUEUE_ALLOCATION_COUNT&&_scheduler_data->q.len<_scheduler_data->q.max-QUEUE_ALLOCATION_COUNT){
			_scheduler_data->q.max-=QUEUE_ALLOCATION_COUNT;
			_scheduler_data->q.dt=realloc(_scheduler_data->q.dt,_scheduler_data->q.max*sizeof(task_index_t));
		}
	}
	return o;
}



static void _add_wait_task(task_index_t t){
	_scheduler_data->wl.len++;
	if (_scheduler_data->wl.len==_scheduler_data->wl.max){
		_scheduler_data->wl.max+=WAIT_LIST_ALLOCATION_COUNT;
		_scheduler_data->wl.dt=realloc(_scheduler_data->wl.dt,_scheduler_data->wl.max*sizeof(task_index_t));
	}
	*(_scheduler_data->wl.dt+_scheduler_data->wl.len-1)=t;
}



static task_index_t _remove_wait_tasks(task_index_t id){
	task_index_t o=UNKNOWN_TASK_INDEX;
	task_index_t i=0;
	for (task_index_t j=0;j<_scheduler_data->wl.len;j++){
		task_index_t k=*(_scheduler_data->wl.dt+j);
		if ((_scheduler_data->tl.dt+k)->w==id){
			(_scheduler_data->tl.dt+k)->w=UNKNOWN_TASK_INDEX;
			if (o==UNKNOWN_TASK_INDEX){
				o=k;
			}
			else{
				_queue_task(k);
			}
		}
		else{
			if (i!=j){
				*(_scheduler_data->wl.dt+i)=*(_scheduler_data->wl.dt+j);
			}
			i++;
		}
	}
	_scheduler_data->wl.len=i;
	task_index_t sz=WAIT_LIST_ALLOCATION_SIZE(_scheduler_data->wl.len);
	if (sz&&sz<_scheduler_data->wl.max){
		_scheduler_data->wl.max=sz;
		_scheduler_data->wl.dt=realloc(_scheduler_data->wl.dt,_scheduler_data->wl.max*sizeof(task_index_t));
	}
	return o;
}



void run_scheduler(task_function_t fn){
	scheduler_t dt={
		{
			NULL,
			0,
			0,
			0
		},
		{
			malloc(QUEUE_ALLOCATION_COUNT*sizeof(task_index_t)),
			QUEUE_ALLOCATION_COUNT,
			0,
			0
		},
		{
			malloc(WAIT_LIST_ALLOCATION_COUNT*sizeof(task_index_t)),
			WAIT_LIST_ALLOCATION_COUNT,
			0
		}
	};
	_scheduler_data=&dt;
	task_index_t task=_create_task(fn);
	while (1){
		uint32_t i=0;
		while (i<TASK_INSTRUCTION_COUNT){
			i++;
			task_state_t st;
			task_return_t rt=(dt.tl.dt+task)->fn(&st);
			if (rt==TASK_OK){
				continue;
			}
			if (rt==TASK_START){
				task_index_t n_task=_create_task(st.start.fn);
				_queue_task(n_task);
				if (st.start.id){
					*(st.start.id)=n_task;
				}
				continue;
			}
			else if (rt==TASK_WAIT){
				if (st.wait!=task&&st.wait<dt.tl.len&&(dt.tl.dt+st.wait)->fn){
					(dt.tl.dt+task)->w=st.wait;
					_add_wait_task(task);
					task=_remove_queue_task();
				}
				else{
					continue;
				}
			}
			else if (rt!=TASK_YIELD){
				if (!dt.q.len&&!dt.wl.len){
					free(dt.tl.dt);
					free(dt.q.dt);
					free(dt.wl.dt);
					_scheduler_data=NULL;
					return;
				}
				(dt.tl.dt+task)->fn=NULL;
				task=_remove_wait_tasks(task);
				if (task==UNKNOWN_TASK_INDEX){
					task=_remove_queue_task();
				}
			}
			i=(rt==TASK_YIELD?TASK_INSTRUCTION_COUNT:0);
			break;
		}
		if (i==TASK_INSTRUCTION_COUNT&&dt.q.len){
			task_index_t tmp=*(dt.q.dt+dt.q.idx);
			*(dt.q.dt+dt.q.idx)=task;
			task=tmp;
			if (!dt.q.idx){
				dt.q.idx=dt.q.len;
			}
			dt.q.idx--;
		}
	}
}
