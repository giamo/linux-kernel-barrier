#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <strings.h>
#include "barrier_client.h"

#define NUM_SLEEPERS 6
#define NUM_AWAKERS 3
#define KEY1 5
#define KEY2 -9

typedef struct _param_struct {
	int thread_id;
	key_t key;
	int tag;
} param_struct;

static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_t threads[NUM_SLEEPERS+NUM_AWAKERS];
static param_struct sleepers_params[NUM_SLEEPERS];
static param_struct awakers_params[NUM_AWAKERS];
static int sleeping = 0;

void init() {
	int sleepers_keys[NUM_SLEEPERS] = {KEY1, KEY1, KEY1, KEY2, KEY2, KEY2};
	int sleepers_tags[NUM_SLEEPERS] = {0, 0, 0, 5, 17, 17};
	int awakers_keys[NUM_AWAKERS] = {KEY1, KEY2, KEY2};
	int awakers_tags[NUM_AWAKERS] = {0, 17, 5};
	int i;
	
	for (i = 0; i < NUM_SLEEPERS; i++) {
		sleepers_params[i].key = sleepers_keys[i];
		sleepers_params[i].tag = sleepers_tags[i];
		sleepers_params[i].thread_id = i;
	}
	
	for (i = 0; i < NUM_AWAKERS; i++) {
		awakers_params[i].key = awakers_keys[i];
		awakers_params[i].tag = awakers_tags[i];
		awakers_params[i].thread_id = i;
	}
}

void go_to_sleep(param_struct *params) {
	int bd, pid;
	pid = getpid();
	
	bd = get_barrier(params->key, O_CREAT);
	if (bd == -1) {
		printf("[sleeper %d] Can't obtain barrier (ERROR) \n", params->thread_id);
		return;	
	}
	
	pthread_mutex_lock(&lock);
	sleeping++;
	pthread_mutex_unlock(&lock);
	
	printf("[sleeper %d] I'm going to sleep on barrier %d tag %d \n", params->thread_id, params->key, params->tag);
	sleep_on_barrier(bd, params->tag);
	printf("[sleeper %d] I'm awake! \n", params->thread_id);
}

void wake_up(param_struct *params) {
	int bd, ret, count;

	bd = get_barrier(params->key, O_CREAT);
	
	if (bd == -1) {
		printf("[awaker %d] Can't obtain barrier (ERROR) \n", params->thread_id);
		return;
	}
	
	while (1) {
		pthread_mutex_lock(&lock);
		count = sleeping;
		pthread_mutex_unlock(&lock);
		if (count >= NUM_SLEEPERS) break;
	}
	printf("[awaker %d] I'm going to awake processes on barrier %d tag %d \n", params->thread_id, params->key, params->tag);

	ret = awake_barrier(bd, params->tag);
	if (ret == -1) {
		printf("[awaker %d] Can't awake barrier %d tag %d (ERROR) \n", params->thread_id, params->key, params->tag);
		return;
	}
}

void release_all() {
	int bd1, bd2, ret1, ret2;
	
	bd1 = get_barrier(KEY1, 0);
	if (bd1 == -1) printf("[release] Can't obtain barrier with key %d (ERROR) \n", KEY1);
	else {
		ret1 = release_barrier(bd1);
		if (ret1 == -1) printf("[release] Can't release barrier with key %d (ERROR) \n", KEY1);
	}
	
	bd2 = get_barrier(KEY2, 0);
	if (bd2 == -1) printf("[release] Can't obtain barrier with key %d (ERROR) \n", KEY2);
	else {
		ret2 = release_barrier(bd2);
		if (ret2 == -1) printf("[release] Can't release barrier with key %d (ERROR) \n", KEY2);
	}

}

int main() {
	int i, j;
	bzero(threads, NUM_SLEEPERS+NUM_AWAKERS);
	
	init();
	
	for (i = 0; i < NUM_SLEEPERS; i++)
		pthread_create(&threads[i], NULL, (void*)go_to_sleep, &(sleepers_params[i]));
	
	sleep(2);
	
	for (i = NUM_SLEEPERS, j = 0; i < NUM_SLEEPERS+NUM_AWAKERS; i++, j++)
		pthread_create(&threads[i], NULL, (void*)wake_up, &(awakers_params[j]));

	for(i = 0; i < NUM_SLEEPERS+NUM_AWAKERS; i++)
		pthread_join(threads[i], NULL);
		
	release_all();
	
	return 0;
}
