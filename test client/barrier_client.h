#include <linux/unistd.h>
#include <unistd.h>
#include <fcntl.h>

#define __NR_get_barrier 17
#define __NR_sleep_on_barrier 31
#define __NR_awake_barrier 32
#define __NR_release_barrier 35

int get_barrier(key_t key, int flags) {
	return syscall(__NR_get_barrier, key, flags);
}

int sleep_on_barrier(int bd, int tag) {
	return syscall(__NR_sleep_on_barrier, bd, tag);
}

int awake_barrier(int bd, int tag) {
	return syscall(__NR_awake_barrier, bd, tag);
}

int release_barrier(int bd) {
	return syscall(__NR_release_barrier, bd);
}
