#include <task_scheduling.h>
#include <stdint.h>
#include <stdlib.h>



#define TASK_UNUSED ((task_function_t)(void*)0)
#define TASK_TERMINATED ((task_function_t)(void*)1)

#define TASK_INSTRUCTION_COUNT 5

#define TASK_ALLOCATION_COUNT 256

#define QUEUE_ALLOCATION_COUNT 256

#define WAIT_LIST_ALLOCATION_COUNT 256
#define WAIT_LIST_ALLOCATION_SIZE(l) (((l)+255)&0xffffff00)

#define TASK_RETURN_GET_TYPE(rt) ((rt)&3)
#define TASK_RETURN_GET_DATA(rt) ((rt)>>2)

#define UNKNOWN_HANDLE 0xffffffff

#define UNKNOWN_TASK_INDEX 0xffffffff



typedef uint32_t task_count_t;



typedef uint32_t handle_t;



typedef struct __TASK{
	task_function_t fn;
	handle_t w;
} task_t;



typedef struct __TASK_LIST{
	task_t* dt;
	task_count_t max;
	task_count_t len;
	task_count_t idx;
} task_list_t;



typedef struct __QUEUE{
	task_index_t* dt;
	task_count_t max;
	task_count_t len;
	task_count_t idx;
} queue_t;



typedef struct __WAIT_LIST{
	task_index_t* dt;
	task_count_t max;
	task_count_t len;
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
		if ((_scheduler_data->tl.dt+o)->fn==TASK_UNUSED){
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
	(_scheduler_data->tl.dt+o)->w=UNKNOWN_HANDLE;
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
	for (task_count_t j=_scheduler_data->q.idx+1;j<_scheduler_data->q.len;j++){
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



static task_index_t _remove_wait_tasks(handle_t id){
	task_index_t o=UNKNOWN_TASK_INDEX;
	task_count_t i=0;
	for (task_count_t j=0;j<_scheduler_data->wl.len;j++){
		task_index_t k=*(_scheduler_data->wl.dt+j);
		if ((_scheduler_data->tl.dt+k)->w==id){
			(_scheduler_data->tl.dt+k)->w=UNKNOWN_HANDLE;
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
	task_count_t sz=WAIT_LIST_ALLOCATION_SIZE(_scheduler_data->wl.len);
	if (sz&&sz<_scheduler_data->wl.max){
		_scheduler_data->wl.max=sz;
		_scheduler_data->wl.dt=realloc(_scheduler_data->wl.dt,_scheduler_data->wl.max*sizeof(task_index_t));
	}
	return o;
}



task_index_t create_task(task_function_t fn){
	task_index_t o=_create_task(fn);
	_queue_task(o);
	return o;
}



void remove_task(task_index_t id){
	if (id>=_scheduler_data->tl.len||(_scheduler_data->tl.dt+id)->fn!=TASK_TERMINATED){
		return;
	}
	(_scheduler_data->tl.dt+id)->fn=TASK_UNUSED;
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
			task_return_t rt=(dt.tl.dt+task)->fn();
			if (TASK_RETURN_GET_TYPE(rt)==TASK_OK){
				continue;
			}
			if (TASK_RETURN_GET_TYPE(rt)==TASK_WAIT){
				task_index_t w_id=(task_index_t)TASK_RETURN_GET_DATA(rt);
				if (w_id!=task&&w_id<dt.tl.len&&(dt.tl.dt+w_id)->fn!=TASK_UNUSED&&(dt.tl.dt+w_id)->fn!=TASK_TERMINATED){
					(dt.tl.dt+task)->w=w_id;
					_add_wait_task(task);
					task=_remove_queue_task();
				}
				else{
					continue;
				}
			}
			else if (TASK_RETURN_GET_TYPE(rt)!=TASK_YIELD){
				if (!dt.q.len&&!dt.wl.len){
					free(dt.tl.dt);
					free(dt.q.dt);
					free(dt.wl.dt);
					_scheduler_data=NULL;
					return;
				}
				(dt.tl.dt+task)->fn=TASK_TERMINATED;
				task=_remove_wait_tasks(task);
				if (task==UNKNOWN_TASK_INDEX){
					task=_remove_queue_task();
				}
			}
			i=(TASK_RETURN_GET_TYPE(rt)==TASK_YIELD?TASK_INSTRUCTION_COUNT:0);
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
