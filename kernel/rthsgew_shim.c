// SPDX-License-Identifier: GPL-2.0
/*
 * rthsgew_shim.c
 *   - exports symbols required by the prebuilt rthsgew.ko
 *   - adds /proc/rthsgew_peek to dump the module's own .bss
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/sched/task.h>
#include <linux/pid.h>
#include <linux/rcupdate.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/kallsyms.h>
#include <linux/string.h>

/* ---- required by rthsgew.ko ---- */
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

/* ---- helpers ---- */

static unsigned long rth_lookup(const char *name)
{
    return kallsyms_lookup_name(name);
}

static int rthsgew_peek_show(struct seq_file *m, void *v)
{
    unsigned long addr, p;
    char namebuf[64];
    unsigned int devt;

    seq_puts(m, "=== rthsgew peek ===\n");

    addr = rth_lookup("get_rand_str.string");
    seq_printf(m, "get_rand_str.string @ %016lx\n", addr);
    if (addr) {
        memset(namebuf, 0, sizeof(namebuf));
        memcpy(namebuf, (void *)addr, 16);
        seq_printf(m, "  raw  : %s\n", namebuf);
    }

    addr = rth_lookup("devicename");
    seq_printf(m, "devicename         @ %016lx\n", addr);
    if (addr) {
        p = *(unsigned long *)addr;
        seq_printf(m, "  *ptr = %016lx\n", p);
        if (p > 0xffff000000000000UL) {
            memset(namebuf, 0, sizeof(namebuf));
            memcpy(namebuf, (void *)p, 32);
            seq_printf(m, "  name = %s\n", namebuf);
        }
    }

    addr = rth_lookup("mem_tool_dev_t");
    seq_printf(m, "mem_tool_dev_t     @ %016lx\n", addr);
    if (addr) {
        devt = *(unsigned int *)addr;
        seq_printf(m, "  dev_t = %u (major=%u minor=%u)\n",
                   devt, (devt >> 20) & 0xfff, devt & 0xfffff);
    }

    addr = rth_lookup("mem_tool_class");
    seq_printf(m, "mem_tool_class     @ %016lx\n", addr);
    if (addr) {
        p = *(unsigned long *)addr;
        seq_printf(m, "  class = %016lx\n", p);
    }

    addr = rth_lookup("memdev");
    seq_printf(m, "memdev             @ %016lx\n", addr);

    return 0;
}

static int rthsgew_peek_open(struct inode *inode, struct file *file)
{
    return single_open(file, rthsgew_peek_show, NULL);
}

static const struct file_operations rthsgew_peek_fops = {
    .owner   = THIS_MODULE,
    .open    = rthsgew_peek_open,
    .read    = seq_read,
    .llseek  = seq_lseek,
    .release = single_release,
};

static int __init rthsgew_peek_init(void)
{
    proc_create("rthsgew_peek", 0444, NULL, &rthsgew_peek_fops);
    pr_info("rthsgew_peek: /proc/rthsgew_peek registered\n");
    return 0;
}
late_initcall(rthsgew_peek_init);
