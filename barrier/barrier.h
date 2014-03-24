/* barrier/barrier.h */
  
#ifndef __BARRIER_H__
#define __BARRIER_H__

#include <linux/sched.h>
#include <linux/wait.h>

#define NUM_TAGS		32
#define BARRIER_HT_SIZE	32
#define KEY_HT_SIZE		32

struct _barrier;

typedef struct _key_hash_node {
	key_t key;
	struct _barrier *barr;
	struct _key_hash_node *next;
	struct _key_hash_node *prev;
} key_hash_node;

typedef struct _process_record {
	struct task_struct *pcb;
	struct _barrier *barr;
	struct _process_record *next;
	struct _process_record *prev;
} process_record;

typedef struct _barrier {
	key_t key;
	int descriptor;
	int count;
	struct mutex lock;
	process_record *process_list[NUM_TAGS];
	key_hash_node *key_node;
	struct _barrier *next;
	struct _barrier *prev;
} barrier;

asmlinkage int sys_get_barrier (key_t key, int flags);
asmlinkage int sys_release_barrier (int bd);
asmlinkage int sys_sleep_on_barrier (int bd, int tag);
asmlinkage int sys_awake_barrier (int bd, int tag);

#endif
