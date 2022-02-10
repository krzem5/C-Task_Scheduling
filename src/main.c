#include <task_scheduling.h>
#include <stdlib.h>
#include <stdio.h>



static task_return_t print_task(task_state_t* o){
	static unsigned int tick=0;
	printf("[print-task-1]: tick %u\n",tick);
	tick++;
	if (tick>=40){
		return TASK_END;
	}
	return TASK_OK;
}



static task_return_t print_task2(task_state_t* o){
	static unsigned int tick=0;
	printf("[print-task-2]: tick %u\n",tick);
	tick++;
	if (tick>=40){
		return TASK_END;
	}
	return TASK_YIELD;
}



static task_return_t main_task(task_state_t* o){
	static unsigned int tick=0;
	static task_index_t* child=NULL;
	tick++;
	switch (tick){
		case 1:
			child=malloc(2*sizeof(task_index_t));
			o->start.fn=print_task;
			o->start.id=child;
			return TASK_START;
		case 2:
			o->start.fn=print_task2;
			o->start.id=child+1;
			return TASK_START;
		case 3:
			o->wait=*child;
			printf("Child#1: %u, Child#2: %u\n",*child,*(child+1));
			return TASK_WAIT;
		case 4:
			o->wait=*(child+1);
			printf("Child#1 finished\n");
			return TASK_WAIT;
	}
	printf("Child#2 finished\n");
	free(child);
	return TASK_END;
}



int main(int argc,const char** argv){
	run_scheduler(main_task);
	return 0;
}
