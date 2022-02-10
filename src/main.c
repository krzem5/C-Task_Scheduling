#include <task_scheduling.h>
#include <stdio.h>



static task_return_t print_task(void){
	static unsigned int tick=0;
	printf("[print-task-1]: tick %u\n",tick);
	tick++;
	if (tick>=200){
		return TASK_END;
	}
	return TASK_YIELD;
}



static task_return_t print_task2(void){
	static unsigned int tick=0;
	printf("[print-task-2]: tick %u\n",tick);
	tick++;
	if (tick>=200){
		return TASK_END;
	}
	return TASK_OK;
}



static task_return_t main_task(void){
	static unsigned int tick=0;
	tick++;
	if (tick==5){
		return print_task;
	}
	else if (tick==100){
		return print_task2;
	}
	else if (tick>=250){
		return TASK_END;
	}
	return TASK_OK;
}



int main(int argc,const char** argv){
	run_scheduler(main_task);
	return 0;
}
