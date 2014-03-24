/* barrier/barrier.c */

#include <linux/kernel.h>
#include <linux/linkage.h>
#include <linux/fcntl.h>
#include <linux/slab.h> 
#include <linux/mutex.h>
#include "barrier.h"

#define INDEX_HASH(v, s) ((((v) < 0) ? (-v) : (v)) % (s))
#define UNLINK(ptr) {									\
					if (ptr->prev != NULL) 				\
						ptr->prev->next = ptr->next; 	\
					if (ptr->next != NULL) 				\
						ptr->next->prev = ptr->prev; 	\
					}
					
static int is_init = 0;
static unsigned int next_descriptor = 0;
static barrier *barrier_hash_table[BARRIER_HT_SIZE];
static key_hash_node *key_hash_table[KEY_HT_SIZE];
static DEFINE_MUTEX(global_vars);

static void init(void)
{
	int entry;
	
	for (entry = 0; entry < BARRIER_HT_SIZE; entry++)
		barrier_hash_table[entry] = NULL;
	
	for (entry = 0; entry < KEY_HT_SIZE; entry++)
		key_hash_table[entry] = NULL;
}

static barrier* get_barrier_from_descriptor(int bd)
{
	int index;
	barrier *barr;
	
	if (bd < 0)
		return NULL;
	
	index = INDEX_HASH(bd, BARRIER_HT_SIZE);
	barr = barrier_hash_table[index];
	
	while (barr != NULL) {
		if (barr->descriptor == bd)
			return barr;
		barr = barr->next;
	}
	
	return NULL;
}

static barrier* get_barrier_from_key(key_t key)
{
	int index;
	key_hash_node *node;
	
	index = INDEX_HASH(key, KEY_HT_SIZE);
	node = key_hash_table[index];
	
	while (node != NULL) {
		if (node->key == key)
			return node->barr;
		node = node->next;
	}
	
	return NULL;
}

static int create_barrier(key_t key)
{
	int b_index, k_index, tag;
	barrier *barr, *b_head;
	key_hash_node *node, *k_head;
	
	if (next_descriptor == INT_MAX) {
		printk(KERN_ERR "[barrier] Reached limit of barrier descriptor values \n");
		return -1;
	}
	
	barr = kmalloc(sizeof(barrier), GFP_KERNEL);
	if (barr == NULL)
		return -1;
		
	node = kmalloc(sizeof(key_hash_node), GFP_KERNEL);
	if (node == NULL) {
		kfree(barr);
		return -1;
	}
	
	barr->key = key;
	barr->descriptor = next_descriptor++;
	barr->count = 0;
	mutex_init(&barr->lock);
	barr->key_node = node;
	barr->next = NULL;
	barr->prev = NULL;
	
	for (tag = 0; tag < NUM_TAGS; tag++)
		barr->process_list[tag] = NULL;
	
	node->key = key;
	node->barr = barr;
	node->next = NULL;
	node->prev = NULL;
	
	// linking the new barrier and key_node to the hash tables
	b_index = INDEX_HASH(barr->descriptor, BARRIER_HT_SIZE);
	k_index = INDEX_HASH(key, KEY_HT_SIZE);
	
	b_head = barrier_hash_table[b_index];
	if (b_head != NULL) {
		barr->next = b_head;
		b_head->prev = barr;
	}
	barrier_hash_table[b_index] = barr;
	
	k_head = key_hash_table[k_index];
	if (k_head != NULL) {
		node->next = k_head;
		k_head->prev = node;
	}
	key_hash_table[k_index] = node;
	
	return barr->descriptor;
}

static int remove_barrier(int bd)
{
	int b_index, k_index;
	barrier *barr;
	key_hash_node *node;
	
	if (bd < 0)
		return -1;
	
	b_index = INDEX_HASH(bd, BARRIER_HT_SIZE);
	
	barr = get_barrier_from_descriptor(bd);
	
	if (barr != NULL){
		mutex_lock(&barr->lock);
		node = barr->key_node;
		
		if (barr->count > 0) {
			printk(KERN_WARNING "[barrier] Can't release barrier %d, there are %d processes sleeping on it", barr->descriptor, barr->count);
			mutex_unlock(&barr->lock);
			return -1;
		}
		
		if (node != NULL) {
			k_index = INDEX_HASH(node->key, KEY_HT_SIZE);
			
			UNLINK(node);
			if (node->prev == NULL)
				key_hash_table[k_index] = node->next;
				
			kfree(node);
		} else {
			mutex_unlock(&barr->lock);
			panic("[barrier] Missing key node for barrier %d", barr->descriptor);
		}
		
		UNLINK(barr);
		if (barr->prev == NULL)
			barrier_hash_table[b_index] = barr->next;
			
		mutex_unlock(&barr->lock);
		kfree(barr);
	} else {
		printk(KERN_ERR "[barrier] Can't release barrier %d, it doesn't exist", barr->descriptor);
		return -1;
	}
	
	return 0;
}

asmlinkage int sys_get_barrier(key_t key, int flags)
{
	int ret;
	barrier *barr;
	
	mutex_lock(&global_vars);
	
	if (!is_init) {
		printk(KERN_INFO "[barrier] Data structures initialization \n");
		init();
		is_init = 1;
	}
	
	barr = get_barrier_from_key(key);
	
	if (barr == NULL) {
		if (flags == O_CREAT || flags == (O_CREAT|O_EXCL)) {
			ret = create_barrier(key);
			if (ret == -1) ret = -ENOMEM;
		} else if (flags == 0) {
			ret = -ENOENT;
		} else {
			printk(KERN_WARNING "[barrier] Undefined flags");
			ret = -EINVAL;
		}
	} else {
		if (flags == 0 || flags == O_CREAT) {
			ret = barr->descriptor;
		} else if (flags == (O_CREAT|O_EXCL)) {
			ret = -EEXIST;
		} else {
			printk(KERN_WARNING "[barrier] Undefined flags");
			ret = -EINVAL;
		}
	}
	
	mutex_unlock(&global_vars);
	
	return ret;
}

asmlinkage int sys_sleep_on_barrier(int bd, int tag)
{
	barrier *barr;
	process_record *record, *head;
	DECLARE_WAIT_QUEUE_HEAD(my_wait_queue);
	DEFINE_WAIT(my_wait);
	
	if (tag < 0 || tag > NUM_TAGS - 1) {
		printk (KERN_ERR "[barrier] Invalid tag");
		goto out_error;
	}
	
	if (bd < 0) {
		printk (KERN_ERR "[barrier] Invalid descriptor \n");
		goto out_error;
	}
	
	mutex_lock(&global_vars);
	
	if (!is_init)
		goto out_error_unlock;
	
	barr = get_barrier_from_descriptor(bd);
	if (barr == NULL) {
		printk (KERN_ERR "[barrier] The barrier %d doesn't exist, can't sleep on it \n", barr->descriptor);
		goto out_error_unlock;
	}
	
	mutex_lock(&barr->lock);
	mutex_unlock(&global_vars);
	
	record = kmalloc(sizeof(process_record), GFP_KERNEL);
	if (record == NULL) {
		mutex_unlock(&barr->lock);
		return -ENOMEM;
	}
	
	record->pcb = current;
	record->barr = barr;
	record->next = NULL;
	record->prev = NULL;
	
	head = barr->process_list[tag];
	if (head != NULL)
		record->next = head;
	
	barr->process_list[tag] = record;
	barr->count++;
	
	prepare_to_wait(&my_wait_queue, &my_wait, TASK_INTERRUPTIBLE);
	
	mutex_unlock(&barr->lock);
	
	if (barr == NULL) {
		finish_wait(&my_wait_queue, &my_wait);
		return -EINVAL;
	} else if (barr->process_list[tag] != NULL) {
		schedule();
	}
	
	mutex_lock(&barr->lock);
	
	finish_wait(&my_wait_queue, &my_wait);
	
	// signal handling
	if (signal_pending(current)) {
		printk(KERN_INFO "[barrier] Process sleeping on barrier %d was awakened by a signal \n", barr->descriptor);
		
		UNLINK(record);
		if (record->prev == NULL)
			barr->process_list[tag] = record->next;
		
		kfree(record);
		barr->count--;
		
		mutex_unlock(&barr->lock);
		return -ERESTARTSYS;
	}
	
	mutex_unlock(&barr->lock);
	
	return 0;
	
out_error_unlock:
	mutex_unlock(&global_vars);
out_error:
	return -EINVAL;
}

asmlinkage int sys_awake_barrier(int bd, int tag)
{
	barrier *barr;
	process_record *record, *temp;
	struct task_struct *pcb;
	
	if (tag < 0 || tag > NUM_TAGS-1) {
		printk (KERN_ERR "[barrier] Invalid tag");
		goto out_error;
	}
	
	if (bd < 0) {
		printk (KERN_ERR "[barrier] Invalid descriptor \n");
		goto out_error;
	}
	
	mutex_lock(&global_vars);
	
	if (!is_init)
		goto out_error_unlock;

	barr = get_barrier_from_descriptor(bd);
	
	if (barr == NULL) {
		printk (KERN_ERR "[barrier] The barrier %d doesn't exist, can't awake processes \n", barr->descriptor);
		goto out_error_unlock;
	}
	
	mutex_lock(&barr->lock);
	mutex_unlock(&global_vars);

	record = barr->process_list[tag];
	
	// for every process_record in the list, wake up the process and remove the record
	while (record != NULL) {
		temp = record;
		
		UNLINK(record);
		if (record->prev == NULL)
			barr->process_list[tag] = record->next;
			
		record = record->next;
		
		pcb = temp->pcb;
		if (pcb == NULL)
			panic("[barrier] Malformed process record in barrier %d \n", barr->descriptor);
		
		if (pcb->state != TASK_INTERRUPTIBLE)
			printk(KERN_WARNING "[barrier] Process with pid %d is not sleeping \n", pcb->pid);
		else
			wake_up_process(pcb);
		
		kfree(temp);
		barr->count--; 
	}
	
	mutex_unlock(&barr->lock);
	
	return 0;
	
out_error_unlock:
	mutex_unlock(&global_vars);
out_error:
	return -EINVAL;
}

asmlinkage int sys_release_barrier(int bd)
{
	int ret;
	
	if (bd < 0) {
		printk (KERN_ERR "[barrier] Invalid descriptor \n");
		goto out_error;
	}
	
	mutex_lock(&global_vars);
	
	if (!is_init)
		goto out_error_unlock;
	
	ret = remove_barrier(bd);
	if (ret == -1)
		goto out_error_unlock;
	
	mutex_unlock(&global_vars);
	
	return 0;

out_error_unlock:
	mutex_unlock(&global_vars);
out_error:
	return -EINVAL;
}
