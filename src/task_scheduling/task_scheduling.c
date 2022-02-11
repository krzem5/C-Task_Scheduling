#include <task_scheduling.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>



#define TASK_UNUSED ((task_function_t)(void*)0)
#define TASK_TERMINATED ((task_function_t)(void*)1)

#define TASK_INSTRUCTION_COUNT 5

#define TASK_ALLOCATION_COUNT 256

#define QUEUE_ALLOCATION_COUNT 256

#define WAIT_LIST_ALLOCATION_COUNT 256
#define WAIT_LIST_ALLOCATION_SIZE(l) (((l)+255)&0xffffff00)

#define MUTEX_ALLOCATION_COUNT 256
#define MUTEX_ALLOCATION_SIZE(l) (((l)+255)&0xffffff00)

#define TASK_RETURN_GET_TYPE(rt) ((rt)&7)
#define TASK_RETURN_GET_DATA(rt) (((rt)>>3)&0xffffffff)
#define TASK_RETURN_GET_DATA2(rt) ((rt)>>35)

#define HANDLE_TYPE_TASK 0
#define HANDLE_TYPE_BARRIER_EQ 1
#define HANDLE_TYPE_BARRIER_GEQ 2
#define HANDLE_TYPE_MUTEX 3
#define HANDLE_TYPE_SEMAPHORE 4

#define CREATE_HANDLE(t,v) ((t)|((v)<<3))
#define CREATE_HANDLE2(t,v,n) ((t)|(((handle_t)(v))<<3)|(((handle_t)(n))<<35))

#define HANDLE_GET_TYPE(h) ((h)&7)
#define HANDLE_GET_DATA(h) (((h)>>3)&0xffffffff)
#define HANDLE_GET_DATA2(h) ((h)>>35)

#define UNKNOWN_HANDLE 0xffffffffffffffffull

#define UNKNOWN_BARRIER_OFFSET 0xffffffff
#define UNKNOWN_MUTEX_OFFSET 0xffffffff
#define UNKNOWN_SEMAPHORE_OFFSET 0xffffffff
#define UNKNOWN_TASK_INDEX 0xffffffff

#define EMPTY_MUTEX 0x7fffffff

#define BARRIER_FLAG_PTR_UNUSED 0x80000000
#define MUTEX_FLAG_PTR_UNUSED 0x80000000
#define SEMAPHORE_FLAG_PTR_UNUSED 0x80000000

#define BARRIER_USED(m) (!((m)&MUTEX_FLAG_PTR_UNUSED))
#define MUTEX_USED(m) (!((m)&MUTEX_FLAG_PTR_UNUSED))
#define SEMAPHORE_USED(m) (!((m)&SEMAPHORE_FLAG_PTR_UNUSED))



typedef uint32_t barrier_count_t;



typedef uint32_t barrier_counter_t;



typedef uint32_t mutex_count_t;



typedef uint32_t semaphore_count_t;



typedef uint32_t task_count_t;



typedef uint64_t handle_t;



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



typedef struct __MUTEX_LIST{
	task_index_t* dt;
	mutex_count_t max;
	mutex_count_t len;
	mutex_count_t idx;
} mutex_list_t;



typedef struct __SEMAPHORE_LIST{
	semaphore_counter_t* dt;
	semaphore_count_t max;
	semaphore_count_t len;
	semaphore_count_t idx;
} semaphore_list_t;



typedef struct __BARRIER_LIST{
	barrier_counter_t* dt;
	barrier_count_t max;
	barrier_count_t len;
	barrier_count_t idx;
} barrier_list_t;



typedef struct __SCHEDULER{
	task_list_t tl;
	queue_t q;
	wait_list_t wl;
	mutex_list_t ml;
	semaphore_list_t sl;
	barrier_list_t bl;
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
	if (!_scheduler_data->q.len){
		return UNKNOWN_TASK_INDEX;
	}
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



static void _add_wait(task_index_t t){
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



barrier_t create_barrier(void){
	barrier_t o;
	if (_scheduler_data->bl.idx==UNKNOWN_BARRIER_OFFSET){
		o=_scheduler_data->bl.len;
		_scheduler_data->bl.len++;
		if (_scheduler_data->bl.len>=_scheduler_data->bl.max){
			_scheduler_data->bl.max+=WAIT_LIST_ALLOCATION_COUNT;
			_scheduler_data->bl.dt=realloc(_scheduler_data->bl.dt,_scheduler_data->bl.max*sizeof(task_index_t));
		}
	}
	else{
		o=_scheduler_data->bl.idx;
		_scheduler_data->bl.idx=(*(_scheduler_data->bl.dt+o)==UNKNOWN_BARRIER_OFFSET?UNKNOWN_BARRIER_OFFSET:(*(_scheduler_data->bl.dt+o))&0x7fffffff);
	}
	*(_scheduler_data->bl.dt+o)=0;
	return o;
}



mutex_t create_mutex(void){
	mutex_t o;
	if (_scheduler_data->ml.idx==UNKNOWN_MUTEX_OFFSET){
		o=_scheduler_data->ml.len;
		_scheduler_data->ml.len++;
		if (_scheduler_data->ml.len>=_scheduler_data->ml.max){
			_scheduler_data->ml.max+=WAIT_LIST_ALLOCATION_COUNT;
			_scheduler_data->ml.dt=realloc(_scheduler_data->ml.dt,_scheduler_data->ml.max*sizeof(task_index_t));
		}
	}
	else{
		o=_scheduler_data->ml.idx;
		_scheduler_data->ml.idx=(*(_scheduler_data->ml.dt+o)==UNKNOWN_MUTEX_OFFSET?UNKNOWN_MUTEX_OFFSET:(*(_scheduler_data->ml.dt+o))&0x7fffffff);
	}
	*(_scheduler_data->ml.dt+o)=EMPTY_MUTEX;
	return o;
}



semaphore_t create_semaphore(semaphore_counter_t n){
	semaphore_t o;
	if (_scheduler_data->sl.idx==UNKNOWN_SEMAPHORE_OFFSET){
		o=_scheduler_data->sl.len;
		_scheduler_data->sl.len++;
		if (_scheduler_data->sl.len>=_scheduler_data->sl.max){
			_scheduler_data->sl.max+=WAIT_LIST_ALLOCATION_COUNT;
			_scheduler_data->sl.dt=realloc(_scheduler_data->sl.dt,_scheduler_data->sl.max*sizeof(semaphore_counter_t));
		}
	}
	else{
		o=_scheduler_data->sl.idx;
		_scheduler_data->sl.idx=(*(_scheduler_data->sl.dt+o)==UNKNOWN_SEMAPHORE_OFFSET?UNKNOWN_SEMAPHORE_OFFSET:(*(_scheduler_data->sl.dt+o))&0x7fffffff);
	}
	*(_scheduler_data->sl.dt+o)=n;
	return o;
}



task_index_t create_task(task_function_t fn){
	task_index_t o=_create_task(fn);
	_queue_task(o);
	return o;
}



void delete_barrier(barrier_t b){
	if (b>=_scheduler_data->bl.len||!BARRIER_USED(*(_scheduler_data->bl.dt+b))){
		return;
	}
	*(_scheduler_data->bl.dt+b)=_scheduler_data->bl.idx|BARRIER_FLAG_PTR_UNUSED;
	_scheduler_data->bl.idx=b;
}



void delete_mutex(mutex_t m){
	if (m>=_scheduler_data->ml.len||!MUTEX_USED(*(_scheduler_data->ml.dt+m))){
		return;
	}
	if (*(_scheduler_data->ml.dt+m)!=EMPTY_MUTEX){
		printf("Deleted non-empy mutex!\n");
	}
	*(_scheduler_data->ml.dt+m)=_scheduler_data->ml.idx|MUTEX_FLAG_PTR_UNUSED;
	_scheduler_data->ml.idx=m;
}



void delete_semaphore(semaphore_t s){
	if (s>=_scheduler_data->sl.len||!SEMAPHORE_USED(*(_scheduler_data->sl.dt+s))){
		return;
	}
	*(_scheduler_data->sl.dt+s)=_scheduler_data->sl.idx|SEMAPHORE_FLAG_PTR_UNUSED;
	_scheduler_data->sl.idx=s;
}



void increase_barrier(barrier_t b){
	if (b>=_scheduler_data->bl.len||!BARRIER_USED(*(_scheduler_data->bl.dt+b))){
		return;
	}
	(*(_scheduler_data->bl.dt+b))++;
	barrier_counter_t v=*(_scheduler_data->bl.dt+b);
	task_count_t i=0;
	for (task_count_t j=0;j<_scheduler_data->wl.len;j++){
		task_index_t k=*(_scheduler_data->wl.dt+j);
		handle_t h=(_scheduler_data->tl.dt+k)->w;
		if (HANDLE_GET_DATA(h)==b&&((HANDLE_GET_TYPE(h)==HANDLE_TYPE_BARRIER_EQ&&HANDLE_GET_DATA2(h)==v)||(HANDLE_GET_TYPE(h)==HANDLE_TYPE_BARRIER_GEQ&&HANDLE_GET_DATA2(h)<=v))){
			(_scheduler_data->tl.dt+k)->w=UNKNOWN_HANDLE;
			_queue_task(k);
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
}



void remove_task(task_index_t id){
	if (id>=_scheduler_data->tl.len||(_scheduler_data->tl.dt+id)->fn!=TASK_TERMINATED){
		return;
	}
	(_scheduler_data->tl.dt+id)->fn=TASK_UNUSED;
}



void release_mutex(mutex_t m){
	if (m>=_scheduler_data->ml.len||!MUTEX_USED(*(_scheduler_data->ml.dt+m))){
		return;
	}
	if (*(_scheduler_data->ml.dt+m)==EMPTY_MUTEX){
		printf("Released empy mutex!\n");
	}
	task_count_t i=0;
	while (i<_scheduler_data->wl.len){
		task_index_t j=*(_scheduler_data->wl.dt+i);
		if ((_scheduler_data->tl.dt+j)->w==CREATE_HANDLE(HANDLE_TYPE_MUTEX,m)){
			(_scheduler_data->tl.dt+j)->w=UNKNOWN_HANDLE;
			_queue_task(j);
			*(_scheduler_data->ml.dt+m)=j;
			break;
		}
		i++;
	}
	if (i==_scheduler_data->wl.len){
		*(_scheduler_data->ml.dt+m)=EMPTY_MUTEX;
		return;
	}
	i++;
	while (i<_scheduler_data->wl.len){
		*(_scheduler_data->wl.dt+i-1)=*(_scheduler_data->wl.dt+i);
		i++;
	}
	_scheduler_data->wl.len--;
	task_count_t sz=WAIT_LIST_ALLOCATION_SIZE(_scheduler_data->wl.len);
	if (sz&&sz<_scheduler_data->wl.max){
		_scheduler_data->wl.max=sz;
		_scheduler_data->wl.dt=realloc(_scheduler_data->wl.dt,_scheduler_data->wl.max*sizeof(task_index_t));
	}
}



void release_semaphore(semaphore_t s){
	if (s>=_scheduler_data->sl.len||!MUTEX_USED(*(_scheduler_data->sl.dt+s))){
		return;
	}
	task_count_t i=0;
	while (i<_scheduler_data->wl.len){
		task_index_t j=*(_scheduler_data->wl.dt+i);
		if ((_scheduler_data->tl.dt+j)->w==CREATE_HANDLE(HANDLE_TYPE_SEMAPHORE,s)){
			(_scheduler_data->tl.dt+j)->w=UNKNOWN_HANDLE;
			_queue_task(j);
			break;
		}
		i++;
	}
	if (i==_scheduler_data->wl.len){
		(*(_scheduler_data->sl.dt+s))++;
		return;
	}
	i++;
	while (i<_scheduler_data->wl.len){
		*(_scheduler_data->wl.dt+i-1)=*(_scheduler_data->wl.dt+i);
		i++;
	}
	_scheduler_data->wl.len--;
	task_count_t sz=WAIT_LIST_ALLOCATION_SIZE(_scheduler_data->wl.len);
	if (sz&&sz<_scheduler_data->wl.max){
		_scheduler_data->wl.max=sz;
		_scheduler_data->wl.dt=realloc(_scheduler_data->wl.dt,_scheduler_data->wl.max*sizeof(task_index_t));
	}
}



void reset_barrier(barrier_t b){
	if (b>=_scheduler_data->bl.len||!BARRIER_USED(*(_scheduler_data->bl.dt+b))){
		return;
	}
	*(_scheduler_data->bl.dt+b)=0;
	task_index_t t=_remove_wait_tasks(CREATE_HANDLE2(HANDLE_TYPE_BARRIER_EQ,b,0));
	if (t!=UNKNOWN_TASK_INDEX){
		_queue_task(t);
	}
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
		},
		{
			NULL,
			0,
			0,
			UNKNOWN_MUTEX_OFFSET
		},
		{
			NULL,
			0,
			0,
			UNKNOWN_SEMAPHORE_OFFSET
		},
		{
			NULL,
			0,
			0,
			UNKNOWN_BARRIER_OFFSET
		}
	};
	_scheduler_data=&dt;// lgtm [cpp/stack-address-escape]
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
					(dt.tl.dt+task)->w=CREATE_HANDLE(HANDLE_TYPE_TASK,w_id);
					_add_wait(task);
					task=_remove_queue_task();
					if (task==UNKNOWN_TASK_INDEX){
						goto _error;
					}
				}
				else{
					continue;
				}
			}
			else if (TASK_RETURN_GET_TYPE(rt)==TASK_MTX){
				mutex_t mtx=(mutex_t)TASK_RETURN_GET_DATA(rt);
				if (mtx<dt.ml.len&&MUTEX_USED(*(dt.ml.dt+mtx))){
					if (*(dt.ml.dt+mtx)==EMPTY_MUTEX){
						*(dt.ml.dt+mtx)=task;
						continue;
					}
					else{
						(dt.tl.dt+task)->w=CREATE_HANDLE(HANDLE_TYPE_MUTEX,mtx);
						_add_wait(task);
						task=_remove_queue_task();
						if (task==UNKNOWN_TASK_INDEX){
							goto _error;
						}
					}
				}
				else{
					continue;
				}
			}
			else if (TASK_RETURN_GET_TYPE(rt)==TASK_BEQ){
				barrier_t bar=(barrier_t)TASK_RETURN_GET_DATA(rt);
				barrier_counter_t bv=(barrier_counter_t)TASK_RETURN_GET_DATA2(rt);
				if (bar<dt.bl.len&&BARRIER_USED(*(dt.bl.dt+bar))){
					if (*(dt.bl.dt+bar)!=bv){
						(dt.tl.dt+task)->w=CREATE_HANDLE2(HANDLE_TYPE_BARRIER_EQ,bar,bv);
						_add_wait(task);
						task=_remove_queue_task();
						if (task==UNKNOWN_TASK_INDEX){
							goto _error;
						}
					}
				}
				else{
					continue;
				}
			}
			else if (TASK_RETURN_GET_TYPE(rt)==TASK_BGE){
				barrier_t bar=(barrier_t)TASK_RETURN_GET_DATA(rt);
				barrier_counter_t bv=(barrier_counter_t)TASK_RETURN_GET_DATA2(rt);
				if (bar<dt.bl.len&&BARRIER_USED(*(dt.bl.dt+bar))){
					if (*(dt.bl.dt+bar)<bv){
						(dt.tl.dt+task)->w=CREATE_HANDLE2(HANDLE_TYPE_BARRIER_GEQ,bar,bv);
						_add_wait(task);
						task=_remove_queue_task();
						if (task==UNKNOWN_TASK_INDEX){
							goto _error;
						}
					}
				}
				else{
					continue;
				}
			}
			else if (TASK_RETURN_GET_TYPE(rt)==TASK_SEM){
				semaphore_t sem=(semaphore_t)TASK_RETURN_GET_DATA(rt);
				if (sem<dt.sl.len&&SEMAPHORE_USED(*(dt.sl.dt+sem))){
					if (*(dt.sl.dt+sem)){
						(*(dt.sl.dt+sem))--;
						continue;
					}
					else{
						(dt.tl.dt+task)->w=CREATE_HANDLE(HANDLE_TYPE_SEMAPHORE,sem);
						_add_wait(task);
						task=_remove_queue_task();
						if (task==UNKNOWN_TASK_INDEX){
							goto _error;
						}
					}
				}
				else{
					continue;
				}
			}
			else if (TASK_RETURN_GET_TYPE(rt)!=TASK_YIELD){
				if (!dt.q.len&&!dt.wl.len){
					goto _cleanup;
				}
				(dt.tl.dt+task)->fn=TASK_TERMINATED;
				task=_remove_wait_tasks(CREATE_HANDLE(HANDLE_TYPE_TASK,task));
				if (task==UNKNOWN_TASK_INDEX){
					task=_remove_queue_task();
					if (task==UNKNOWN_TASK_INDEX){
						goto _error;
					}
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
_error:
	printf("Execution ended due to a deadlock.\n");
_cleanup:
	free(dt.tl.dt);
	free(dt.q.dt);
	free(dt.wl.dt);
	free(dt.ml.dt);
	free(dt.bl.dt);
	_scheduler_data=NULL;
	return;
}
