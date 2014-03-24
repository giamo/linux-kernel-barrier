#include <stdlib.h>
#include <stdio.h>
#include "barrier_client.h"

int main(int argc, char **argv) {
	int key, tag, bd, ret;

	if (argc < 3) {
		printf("Usage: awaker <barrier_key> <tag> \n");
		return -1;
	}
	
	key = atoi(argv[1]);
	tag = atoi(argv[2]);
	
	bd = get_barrier(key, 0);
	if (bd == -1) {
		printf("Error: the barrier doesn't exist \n");
		return -1;
	}
	
	ret = awake_barrier(bd, tag);

	if (ret != -1)
		printf("Awekened processes sleeping on tag %d \n", tag);
	else printf("Invalid tag \n");
	
	return 0;
}
