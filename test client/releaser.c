#include <stdlib.h>
#include <stdio.h>
#include "barrier_client.h"

int main (int argc, char **argv) {
	int key, bd, ret;

	if (argc < 2) {
		printf("Usage: releaser <barrier_key> \n");
		return -1;
	}
	
	key = atoi(argv[1]);
	
	bd = get_barrier(key, 0);
	if (bd == -1) {
		printf("Error: the barrier doesn't exist \n");
		return -1;
	}
	
	ret = release_barrier(bd);
	
	if (ret == -1) {
		printf("Error: impossible to release the barrier\n");
		return -1;
	}
	
	printf("Barrier correctly released \n");
	
	return 0;
}
