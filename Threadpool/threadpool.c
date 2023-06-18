#include "threadpool.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include "array.h"

int poolSize;
int terminate;
pthread_t main_pid;
pthread_t *threads;
static pthread_mutex_t mutex;
static Future **tasks;

/* TODO: interne, globale Variablen hinzufügen */

/* TODO: interne Hilfsfunktionen hinzufügen */

Future *get_task(){
	//printf("Getting Task thread [%i]", pthread_self());
	pthread_mutex_lock(&mutex);
	if(arrayIsEmpty(tasks)){
		//printf("NoTask thread [%i]", pthread_self());
		pthread_mutex_unlock(&mutex);
		sched_yield();
		return NULL;
	}
	//printf("Size: %i\n", arrayLen(tasks));
	Future *task = arrayPop(tasks);
	task->finished = 0;
	pthread_mutex_unlock(&mutex);
	sched_yield();
	return task;
}

int try_execute_task() {
    // if Task Stack is empty get_task() will return NULL
    // in this case: also return from this function
    // to maybe get back to an await point to check if awaited future is done
    // (if try_execute_task() was called from worker(), it will be called again
    // immediately)
    Future *fut = get_task();
    if (fut == NULL) {
        return 0;
    }
    //  fut is first member in the corresponding func_future struct
    //  and fn==funcThunk takes func_future* as argument
	
    fut->fn(fut);
    fut->finished = 1;
    return 1;
}

void *worker()
{
    while (1) {
		//gucken ob neue Tasks da sind
		if(!arrayIsEmpty(tasks) && terminate == 0){
			// lock um poolSize und taskIndex um die beiden zu bearbeiten
			Future *task;
			task = get_task();
			if (task != NULL)
			{
				task->fn(task);
				task->finished = 1;
			}
		}

		if(terminate == 1){
			pthread_mutex_lock(&mutex);
			poolSize -= 1;
			pthread_mutex_unlock(&mutex);
			pthread_exit(NULL);
		}
    }
}

int tpInit(size_t size) {
	main_pid = pthread_self();
	arrayInit(tasks);

	threads = (pthread_t *)malloc(size * sizeof(pthread_t));
	terminate = 0;

	pthread_mutex_init(&mutex, NULL);

	for (size_t i = 0; i < size; i++)
	{
		pthread_mutex_lock(&mutex);
		poolSize++;
		pthread_mutex_unlock(&mutex);

		pthread_create(&threads[i], NULL, &worker, NULL);
	}
	
	return 0;
}

void tpRelease(void) {

	terminate = 1;
	
	for (int i = 0; i < poolSize; i++)
	{
		pthread_cancel(threads[i]);
		pthread_join(threads[i], NULL);
	}
	arrayRelease(tasks);
}

void tpAsync(Future *future) {

	pthread_mutex_lock(&mutex);
	arrayPush(tasks) = future;
	pthread_mutex_unlock(&mutex);
	sched_yield();

}

void tpAwait(Future *future) {

	pthread_t id = pthread_self();
	if (id == main_pid) {
		//printf("main Thread here");
        while (!future->finished){
			//printf("within loop");
            nanosleep((struct timespec[]){{0, 10000000L}}, NULL);
		}
        return;
    }

    while (1) {
        if (future->finished) {
            return;
        }
		//printf("Size %li\n", arrayLen(tasks));
        try_execute_task();
    }

}