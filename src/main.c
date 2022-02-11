#include <task_scheduling.h>
#include <stdlib.h>
#include <stdio.h>



static mutex_t mtx;
static semaphore_t sem;
static barrier_t bar;



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
	if (tick>=40){
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
	if (tick>=18){
		release_mutex(mtx);
		printf("[mutex-task-2]: Released mutex\n");
		return TASK_END;
	}
	return TASK_OK;
}



static task_return_t semaphore_task(void){
	static unsigned int tick=0;
	printf("[semaphore-task-1]: tick %u\n",tick);
	tick++;
	if (tick==10){
		printf("[semaphore-task-1]: Waiting for semaphore\n");
		return TASK_DATA(TASK_SEM,sem);
	}
	if (tick==11){
		printf("[semaphore-task-1]: Acquired semaphore\n");
	}
	if (tick>=40){
		release_semaphore(sem);
		printf("[semaphore-task-1]: Released semaphore\n");
		return TASK_END;
	}
	return TASK_OK;
}



static task_return_t semaphore_task2(void){
	static unsigned int tick=0;
	printf("[semaphore-task-2]: tick %u\n",tick);
	tick++;
	if (tick==12){
		printf("[semaphore-task-2]: Waiting for semaphore\n");
		return TASK_DATA(TASK_SEM,sem);
	}
	if (tick==13){
		printf("[semaphore-task-2]: Acquired semaphore\n");
	}
	if (tick>=30){
		release_semaphore(sem);
		printf("[semaphore-task-2]: Released semaphore\n");
		return TASK_END;
	}
	return TASK_OK;
}



static task_return_t semaphore_task3(void){
	static unsigned int tick=0;
	printf("[semaphore-task-3]: tick %u\n",tick);
	tick++;
	if (tick==20){
		printf("[semaphore-task-3]: Waiting for semaphore\n");
		return TASK_DATA(TASK_SEM,sem);
	}
	if (tick==21){
		printf("[semaphore-task-3]: Acquired semaphore\n");
	}
	if (tick>=22){
		release_semaphore(sem);
		printf("[semaphore-task-3]: Released semaphore\n");
		return TASK_END;
	}
	return TASK_OK;
}



static task_return_t barrier_task(void){
	static unsigned int tick=0;
	printf("[barrier-task-1]: tick %u\n",tick);
	tick++;
	if (tick==21){
		printf("[barrier-task-1]: Increasing barrier counter\n");
		increase_barrier(bar);
		return TASK_END;
	}
	return TASK_OK;
}



static task_return_t barrier_task2(void){
	static unsigned int tick=0;
	printf("[barrier-task-2]: tick %u\n",tick);
	tick++;
	if (tick==12){
		printf("[barrier-task-2]: Waiting for barrier >=1\n");
		return TASK_DATA2(TASK_BGE,bar,1);
	}
	if (tick==13){
		printf("[barrier-task-2]: Barrier broken\n");
		reset_barrier(bar);
		return TASK_END;
	}
	return TASK_OK;
}



static task_return_t main_task(void){
	static unsigned int tick=0;
	static task_index_t child[3];
	tick++;
	switch (tick){
		case 1:
			child[0]=create_task(print_task);
			child[1]=create_task(print_task2);
			printf("Child#1: %u, Child#2: %u\n",child[0],child[1]);
			return TASK_DATA(TASK_WAIT,child[0]);
		case 2:
			remove_task(child[0]);
			printf("Child#1 finished\n");
			return TASK_DATA(TASK_WAIT,child[1]);
		case 3:
			remove_task(child[1]);
			printf("Child#2 finished\n");
			mtx=create_mutex();
			child[0]=create_task(mutex_task);
			child[1]=create_task(mutex_task2);
			return TASK_DATA(TASK_WAIT,child[0]);
		case 4:
			remove_task(child[0]);
			return TASK_DATA(TASK_WAIT,child[1]);
		case 5:
			remove_task(child[1]);
			delete_mutex(mtx);
			sem=create_semaphore(1);
			child[0]=create_task(semaphore_task);
			child[1]=create_task(semaphore_task2);
			child[2]=create_task(semaphore_task3);
			return TASK_DATA(TASK_WAIT,child[0]);
		case 6:
			remove_task(child[0]);
			return TASK_DATA(TASK_WAIT,child[1]);
		case 7:
			remove_task(child[1]);
			return TASK_DATA(TASK_WAIT,child[2]);
		case 8:
			remove_task(child[2]);
			delete_semaphore(sem);
			bar=create_barrier();
			child[0]=create_task(barrier_task);
			child[1]=create_task(barrier_task2);
			return TASK_DATA(TASK_WAIT,child[0]);
		case 9:
			remove_task(child[0]);
			return TASK_DATA(TASK_WAIT,child[1]);
		default:
			remove_task(child[1]);
			delete_barrier(bar);
			return TASK_END;
	}
}



int main(int argc,const char** argv){
	run_scheduler(main_task);
	return 0;
}
