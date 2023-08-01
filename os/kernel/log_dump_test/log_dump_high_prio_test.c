#include "log_dump_test/log_dump_test.h"



static int count = 0;

int log_dump_high_prio_test(int argc, char *argv[]) {
	sleep(2);
	int loop = 5;
	for (int j=0; j < loop; j++) {
		for(int i = count; i < count + 30; i++) {
			printf("high prio: %d\n", i);
		}
		count += 30;

		sleep(1);
	}
	
	return 0;
}
