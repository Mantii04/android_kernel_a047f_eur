#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/sched.h>
#include <linux/pid.h>
#include <linux/mm.h>

#define IOCTL_READ_MEM   _IOWR('f', 1, struct mem_request)
#define IOCTL_WRITE_MEM  _IOWR('f', 2, struct mem_request)

struct mem_request {
    int pid;
    unsigned long address;
    void *buffer;
    size_t size;
};

static long ff_ioctl(struct file *file, unsigned int cmd, unsigned long arg) {
    struct mem_request req;
    struct pid *pid_struct;
    struct task_struct *task;
    int ret = 0;

    if (copy_from_user(&req, (void __user *)arg, sizeof(req)))
        return -EFAULT;

    rcu_read_lock();
    pid_struct = find_vpid(req.pid);
    if (!pid_struct) {
        rcu_read_unlock();
        return -ESRCH;
    }
    task = pid_task(pid_struct, PIDTYPE_PID);
    if (!task) {
        rcu_read_unlock();
        return -ESRCH;
    }

    // In this kernel version, access_process_vm handles get_task_mm 
    // and down_read(&mm->mmap_sem) internally. We must NOT do it here.
    switch (cmd) {
        case IOCTL_READ_MEM:
            ret = access_process_vm(task, req.address, req.buffer, req.size, FOLL_FORCE);
            if (ret != req.size) ret = -EIO;
            else ret = 0;
            break;
        case IOCTL_WRITE_MEM:
            ret = access_process_vm(task, req.address, req.buffer, req.size, FOLL_FORCE | FOLL_WRITE);
            if (ret != req.size) ret = -EIO;
            else ret = 0;
            break;
        default:
            ret = -EINVAL;
    }

    rcu_read_unlock();

    return ret;
}

static int ff_open(struct inode *inode, struct file *file) { return 0; }
static int ff_release(struct inode *inode, struct file *file) { return 0; }

static struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = ff_open,
    .release = ff_release,
    .unlocked_ioctl = ff_ioctl,
};

static struct class *ff_class;
static int major_num;

static int __init ff_init(void) {
    major_num = register_chrdev(0, "ff_driver", &fops);
    if (major_num < 0) return major_num;
    
    ff_class = class_create(THIS_MODULE, "ff_driver_class");
    device_create(ff_class, NULL, MKDEV(major_num, 0), NULL, "/dev/ff_driver");
    
    return 0;
}

static void __exit ff_exit(void) {
    device_destroy(ff_class, MKDEV(major_num, 0));
    class_destroy(ff_class);
    unregister_chrdev(major_num, "ff_driver");
}

module_init(ff_init);
module_exit(ff_exit);
MODULE_LICENSE("GPL");
