Linux Kernel Barrier
=====================

Linux kernel extension that provides a synchronization system based on a barrier with 32 levels of synchronization. Threads/processes can go  to sleep on the barrier and are awaken when the awake system call is invoked.

Installation
------------

Copy the barrier/ folder within the kernel source code and compile it.

See the test client for how to use the new system calls once the kernel is in use.

System calls
------------

- `int sys_get_barrier (key_t key, int flags)`: installs a new barrier with specified flags and returns its barrier descriptor
- `int sys_release_barrier (int bd)`: uninstalls the barrier with the given descriptor
- `int sys_sleep_on_barrier (int bd, int tag)`: puts the calling process to sleep on a barrier on the given tag (level)
- `int sys_awake_barrier (int bd, int tag)`: awakes all processing sleeping on the barrier on the given tag (level)
