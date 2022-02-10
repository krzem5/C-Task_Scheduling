#include <task_scheduling.h>
#include <stdlib.h>
#include <stdio.h>



static mutex_t mtx;



static task_return_t print_task(void){
	static unsigned int tick=0;
	printf("[print-task-1]: tick %u\n",tick);
	tick++;
	if (tick>=20){
		return TASK_END;
	}
	return TASK_OK;
}



static task_return_t print_task2(void){
	static unsigned int tick=0;
	printf("[print-task-2]: tick %u\n",tick);
	tick++;
	if (tick>=20){
		return TASK_END;
	}
	return TASK_YIELD;
}



static task_return_t mutex_task(void){
	static unsigned int tick=0;
	printf("[mutex-task-1]: tick %u\n",tick);
	tick++;
	if (tick==10){
		printf("[mutex-task-1]: Waiting for mutex\n");
		return TASK_DATA(TASK_MTX,mtx);
	}
	if (tick==11){
		printf("[mutex-task-1]: Acquired mutex\n");
	}
	if (tick==40){
		release_mutex(mtx);
		printf("[mutex-task-1]: Released mutex\n");
		return TASK_END;
	}
	return TASK_OK;
}



static task_return_t mutex_task2(void){
	static unsigned int tick=0;
	printf("[mutex-task-2]: tick %u\n",tick);
	tick++;
	if (tick==16){
		printf("[mutex-task-2]: Waiting for mutex\n");
		return TASK_DATA(TASK_MTX,mtx);
	}
	if (tick==17){
		printf("[mutex-task-2]: Acquired mutex\n");
	}
	if (tick==18){
		release_mutex(mtx);
		printf("[mutex-task-2]: Released mutex\n");
		return TASK_END;
	}
	return TASK_OK;
}



static task_return_t main_task(void){
	static unsigned int tick=0;
	static task_index_t child[2];
	tick++;
	switch (tick){
		case 1:
			child[0]=create_task(print_task);
			child[1]=create_task(print_task2);
			printf("Child#1: %u, Child#2: %u\n",child[0],child[1]);
			return TASK_OK;
		case 2:
			return TASK_DATA(TASK_WAIT,child[0]);
		case 3:
			remove_task(child[0]);
			printf("Child#1 finished\n");
			return TASK_DATA(TASK_WAIT,child[1]);
		case 4:
			remove_task(child[1]);
			printf("Child#2 finished\n");
			mtx=create_mutex();
			child[0]=create_task(mutex_task);
			child[1]=create_task(mutex_task2);
			return TASK_DATA(TASK_WAIT,child[0]);
		case 5:
			remove_task(child[0]);
			return TASK_DATA(TASK_WAIT,child[1]);
		default:
			delete_mutex(mtx);
			return TASK_END;
	}
}



int main(int argc,const char** argv){
	run_scheduler(main_task);
	return 0;
}
