#include "log_dump_test/log_dump_test.h"
#include <tinyara/sched.h>



static int count = 0;

int log_dump_sched_lock_test(int argc, char *argv[]) {
	sleep(2);
	int loop = 5;
	for (int j=0; j < loop; j++) {
		sched_lock();
		for(int i = count; i < count + 1000; i++) {
			printf("sched lock: %d\n", i);
		}
		count += 1000;
		sched_unlock();
		sleep(1);
	}
	
	return 0;
}
