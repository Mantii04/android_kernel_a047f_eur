// SPDX-License-Identifier: GPL-2.0
/*
 * rthsgew_shim.c - provides symbols required by prebuilt rthsgew.ko
 * that are unique to a downstream kernel. All lockdep/RCU symbols
 * are provided by enabling CONFIG_PROVE_LOCKING etc. in a04s.config.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/sched/task.h>
#include <linux/pid.h>
#include <linux/rcupdate.h>

struct task_struct *my_get_task_struct_by_pid(int pid)
{
    struct task_struct *task;
    struct pid *p = find_vpid(pid);
    if (!p)
        return NULL;
    rcu_read_lock();
    task = pid_task(p, PIDTYPE_PID);
    if (task)
        get_task_struct(task);
    rcu_read_unlock();
    return task;
}
EXPORT_SYMBOL(my_get_task_struct_by_pid);
