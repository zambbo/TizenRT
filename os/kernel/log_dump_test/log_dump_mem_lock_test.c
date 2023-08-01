#include "log_dump_test/log_dump_test.h"
#include <tinyara/mm/mm.h>



static int count = 0;

int log_dump_mem_lock_test(int argc, char *argv[]) {
	sleep(2);
	int loop = 5;
	for (int j=0; j < loop; j++) {
		mm_takesemaphore(&g_kmmheap[0]);
		for(int i = count; i < count + 30; i++) {
			printf("mem lock: %d\n", i);
		}
		count += 30;
		mm_givesemaphore(&g_kmmheap[0]);

		sleep(1);
	}
	
	return 0;
}
