#ifndef _COMM_0
#define _COMM_0

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <libspe2.h>
#include <pthread.h>


#define SPU_THREADS 8

extern spe_program_handle_t spu_worker;

typedef struct {
	unsigned int id;
	spe_context_ptr_t ctx;
} thread_arg_t;

spe_context_ptr_t ctx[SPU_THREADS];
pthread_t threads[SPU_THREADS];
thread_arg_t arg[SPU_THREADS];


spe_event_unit_t pevents[SPU_THREADS], events_received[SPU_THREADS]; 
spe_event_handler_ptr_t event_handler; 


void *ppu_pthread_function(void *thread_arg);
void init_spus();
void stop_spus();
#endif
