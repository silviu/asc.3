#include "comm.h"

void *ppu_pthread_function(void *thread_arg) 
{
	thread_arg_t *arg = (thread_arg_t *) thread_arg;
	unsigned int entry = SPE_DEFAULT_ENTRY;

	if (spe_context_run(arg->ctx, &entry, 0, (void*) arg->id, NULL, NULL) < 0) 
	{
		perror("PPU:Failed running context");
		exit(1);
	}

   pthread_exit(NULL);
}

void init_spus()
{
	unsigned int i;

	event_handler = spe_event_handler_create();

	for (i = 0; i < SPU_THREADS; i++) {
		if ((ctx[i] = spe_context_create(SPE_EVENTS_ENABLE, NULL)) == NULL) {
			perror ("Failed creating context");
			exit(1);
		}
		pevents[i].events = SPE_EVENT_OUT_INTR_MBOX; 
		pevents[i].spe = ctx[i];
		pevents[i].data.u32 = i;
		spe_event_handler_register(event_handler, &pevents[i]);


		if (spe_program_load(ctx[i], &spu_worker)) {
			perror ("Failed loading program");
			exit(1);
		}

		arg[i].id   = i;
		arg[i].ctx  = ctx[i];
		
		printf("PPU: CREATING SPU %d\n", i);

		if (pthread_create(&threads[i], NULL, &ppu_pthread_function, &arg[i])) {
			perror("Failed creating thread");
			exit(1);
		}
	}
}

void stop_spus()
{
	int i;
	for (i = 0; i < SPU_THREADS; i++) {
		if (pthread_join(threads[i], NULL)) {
			perror("Failed pthread_join");
			exit(1);
		}
		if (spe_context_destroy(ctx[i]) != 0) {
			perror("Failed destroying context");
			exit(1);
		}
		printf("PPU: SPU %d STOPPED\n", i);
	}
}
