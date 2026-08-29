#ifndef MEMKERNEL_COMM_H
#define MEMKERNEL_COMM_H

struct CopyMemory
{
    pid_t pid;
    uintptr_t addr;
    void *buffer;
    size_t size;
};

struct ModuleBase
{
    pid_t pid;
    char *name;
    uintptr_t base;
    int index; // ADDED: To select which match to return (1st, 2nd, etc.)
};

enum Operations
{
    OP_READ_MEM = 0x801,
    OP_WRITE_MEM = 0x802,
    OP_MODULE_BASE = 0x803,
};

#endif
