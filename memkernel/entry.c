// MIT License
/*
 * Memory Operation driver for Linux Android
 * Original author:  Jiang-Night
 * Current maintainer: Poko
 * Adapted for ESPDXD-CODMV2.2.sh userspace compatibility.
 */

#include <linux/module.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/mm.h>
#include <linux/uaccess.h>
#include <linux/mutex.h>
#include "comm.h"
#include "memory.h"
#include "process.h"

#define DEVICE_NAME "phmeop"

static int dispatch_open(struct inode *node, struct file *file) {
        return 0;
}

static int dispatch_close(struct inode *node, struct file *file) {
        return 0;
}

static long dispatch_ioctl(struct file *const file, unsigned int const cmd, unsigned long const arg)
{
        struct CopyMemory cm;
        struct ModuleBase mb;
        char name[0x100] = {0};

        switch (cmd)
        {
        case OP_READ_MEM:
        {
                if (copy_from_user(&cm, (void __user *)arg, sizeof(cm)) != 0)
                        return -1;
                return readwrite_process_memory(cm.pid, cm.addr, cm.buffer, cm.size, false);
        }
        case OP_WRITE_MEM:
        {
                if (copy_from_user(&cm, (void __user *)arg, sizeof(cm)) != 0)
                        return -1;
                return readwrite_process_memory(cm.pid, cm.addr, cm.buffer, cm.size, true);
        }
        case OP_READ_MEM_APV:
        {
                if (copy_from_user(&cm, (void __user *)arg, sizeof(cm)) != 0)
                        return -1;
                return readwrite_process_memory_apv(cm.pid, cm.addr, cm.buffer, cm.size, false);
        }
        case OP_WRITE_MEM_APV:
        {
                if (copy_from_user(&cm, (void __user *)arg, sizeof(cm)) != 0)
                        return -1;
                return readwrite_process_memory_apv(cm.pid, cm.addr, cm.buffer, cm.size, true);
        }
        case OP_GET_PFN:
        {
                struct GetPfnReq req;
                long pfn;
                if (copy_from_user(&req, (void __user *)arg, sizeof(req)) != 0)
                        return -1;
                pfn = get_process_pfn(req.pid, req.addr);
                if (pfn < 0) {
                        req.status = -1;
                        req.pfn = 0;
                } else {
                        req.status = 0;
                        req.pfn = (uintptr_t)pfn;
                }
                if (copy_to_user((void __user *)arg, &req, sizeof(req)) != 0)
                        return -1;
                break;
        }
        case OP_MODULE_BASE:
        {
                if (copy_from_user(&mb, (void __user *)arg, sizeof(mb)) != 0)
                        return -1;
                if (copy_from_user(name, (void __user *)mb.name, sizeof(name) - 1) != 0)
                        return -1;
                mb.base = get_module_base(mb.pid, name);
                if (copy_to_user((void __user *)arg, &mb, sizeof(mb)) != 0)
                        return -1;
                break;
        }
        default:
                break;
        }
        return 0;
}

static int dispatch_mmap(struct file *file, struct vm_area_struct *vma)
{
    unsigned long pfn = vma->vm_pgoff;
    unsigned long size = vma->vm_end - vma->vm_start;

    if (size != PAGE_SIZE)
        return -EINVAL;
    if (!pfn_valid(pfn))
        return -EINVAL;
    if (remap_pfn_range(vma, vma->vm_start, pfn, size, vma->vm_page_prot))
        return -EAGAIN;
    return 0;
}

static struct file_operations dispatch_functions = {
        .owner = THIS_MODULE,
        .open = dispatch_open,
        .release = dispatch_close,
        .unlocked_ioctl = dispatch_ioctl,
        .mmap = dispatch_mmap,
};

static struct miscdevice misc = {
        .minor = MISC_DYNAMIC_MINOR,
        .name = DEVICE_NAME,
        .fops = &dispatch_functions,
};

#define ALIAS_COUNT 7
static const char *alias_names[ALIAS_COUNT] = {
    "dream", "dreamc", "driver", "hook", "hook_driver", "hookdrv", "rthook"
};
static struct miscdevice misc_aliases[ALIAS_COUNT];

int __init memkernel_entry(void)
{
        int ret;
        int i;

        printk("[+] memkernel_entry");

        ret = misc_register(&misc);
        if (ret) {
                printk("[-] memkernel: primary misc_register failed: %d\n", ret);
                return ret;
        }

        for (i = 0; i < ALIAS_COUNT; i++) {
                misc_aliases[i].minor = MISC_DYNAMIC_MINOR;
                misc_aliases[i].name  = alias_names[i];
                misc_aliases[i].fops  = &dispatch_functions;
                misc_aliases[i].mode  = 0666;
                ret = misc_register(&misc_aliases[i]);
                if (ret) {
                        printk("[-] memkernel: alias /dev/%s failed: %d\n",
                               alias_names[i], ret);
                        misc_aliases[i].name = NULL;
                } else {
                        printk("[+] memkernel: alias /dev/%s ready\n",
                               alias_names[i]);
                }
        }

        return 0;
}

void __exit memkernel_unload(void)
{
        int i;

        printk("[+] memkernel_unload");

        for (i = 0; i < ALIAS_COUNT; i++) {
                if (misc_aliases[i].name) {
                        misc_deregister(&misc_aliases[i]);
                        misc_aliases[i].name = NULL;
                }
        }
        misc_deregister(&misc);
}

module_init(memkernel_entry);
module_exit(memkernel_unload);

MODULE_DESCRIPTION("Linux Kernel.");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Linux");
