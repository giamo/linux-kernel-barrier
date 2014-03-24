Linux Kernel Barrier
=================================

Linux kernel extension that provides a synchronization system based on a barrier with 32 levels of synchronization. Threads/processes can sleep on a given level and are awaken when the awake system call is invoked on that level.

Installation
------------

Copy the barrier/ folder within the kernel source code and compile it.

See the test client for how to use the new system calls once the kernel is in use.
