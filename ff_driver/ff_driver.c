#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/sched.h>
#include <linux/pid.h>
#include <linux/mm.h>
#include <linux/atomic.h>

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
    struct task_struct *task;
    int ret = 0;

    if (copy_from_user(&req, (void __user *)arg, sizeof(req)))
        return -EFAULT;

    rcu_read_lock();
    task = find_task_by_vpid(req.pid);
    if (!task) {
        rcu_read_unlock();
        return -ESRCH;
    }
    
    // Manually increment the reference count so the task doesn't get freed
    // after we drop the rcu_read_lock. (Replaces get_task_struct)
    atomic_inc(&task->usage);
    rcu_read_unlock();

    // Now we are safe to sleep in access_process_vm!
    switch (cmd) {
        case IOCTL_READ_MEM:
            ret = access_process_vm(task, req.address, req.buffer, (int)req.size, FOLL_FORCE);
            if (ret != (int)req.size) ret = -EIO;
            else ret = 0;
            break;
        case IOCTL_WRITE_MEM:
            ret = access_process_vm(task, req.address, req.buffer, (int)req.size, FOLL_FORCE | FOLL_WRITE);
            if (ret != (int)req.size) ret = -EIO;
            else ret = 0;
            break;
        default:
            ret = -EINVAL;
    }

    // Decrement the reference count (Replaces put_task_struct)
    // If it hits 0 it won't be freed immediately, but that's fine for us.
    atomic_dec(&task->usage);

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
