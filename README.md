Linux Kernel Barrier
=====================

Linux kernel extension that provides a synchronization system based on a barrier with 32 levels of synchronization. Threads/processes can go  to sleep on the barrier and are awaken when the awake system call is invoked.

Installation
------------

Copy the barrier/ folder within the kernel source code and compile it.

See the test client for how to use the new system calls once the kernel is in use.

System calls
------------

_int sys\_get\_barrier (key\_t key, int flags)_: installs a new barrier with specified flags and returns its barrier descriptor
_int sys\_release\_barrier (int bd)_: uninstalls the barrier with the given descriptor
_int sys\_sleep\_on\_barrier (int bd, int tag)_: puts the calling process to sleep on a barrier on the given tag (level)
_int sys\_awake\_barrier (int bd, int tag)_: awakes all processing sleeping on the barrier on the given tag (level)
