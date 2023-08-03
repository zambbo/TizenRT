#include "log_dump_test/log_dump_test.h"
#include <tinyara/mm/mm.h>



static int count = 0;

int log_dump_mem_lock_test(int argc, char *argv[]) {
	sleep(2);
	int loop = 1;
	for (int j=0; j < loop; j++) {
		mm_takesemaphore(&g_kmmheap[0]);
		for (int i=count; i<count +3000; i+=10)
			printf("mem_lock: %d %d %d %d %d %d %d %d %d %d\n", i, i+1, i+2, i+3, i+4, i+5, i+6, i+7, i+8, i+9);	

		count += 3000;
		mm_givesemaphore(&g_kmmheap[0]);

		sleep(10);
	}
	
	return 0;
}
