#include "memory.h"
#include <linux/tty.h>
#include <linux/io.h>
#include <linux/mm.h>
#include <linux/memory.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/version.h>
#include <asm/page.h>
#include <asm/pgtable.h>
#include <linux/vmalloc.h>
#include <linux/mman.h>

uintptr_t get_module_base(pid_t pid, char *name, int vm_flag) {
    struct pid *pid_struct;
    struct task_struct *task;
    struct mm_struct *mm;
    struct vm_area_struct *vma;
    uintptr_t result;
    struct dentry *dentry;
    size_t name_len, dname_len;

    result = 0;
    name_len = strlen(name);
    if (name_len == 0) return 0;

    pid_struct = find_get_pid(pid);
    if (!pid_struct) return 0;
    task = get_pid_task(pid_struct, PIDTYPE_PID);
    put_pid(pid_struct);
    if (!task) return 0;
    mm = get_task_mm(task);
    put_task_struct(task);
    if (!mm) return 0;

    MM_READ_LOCK(mm)
    for (vma = mm->mmap; vma; vma = vma->vm_next) {
        if (vma->vm_file) {
            if (vm_flag && !(vma->vm_flags & vm_flag)) continue;
            dentry = vma->vm_file->f_path.dentry;
            dname_len = dentry->d_name.len;
            if (!memcmp(dentry->d_name.name, name, min(name_len, dname_len))) {
                result = vma->vm_start;
                break;
            }
        }
    }
    MM_READ_UNLOCK(mm)
    mmput(mm);
    return result;
}

uintptr_t get_module_base_bss(pid_t pid, char *name, int vm_flag) {
    return 0;
}

phys_addr_t vaddr_to_phy_addr(struct mm_struct *mm, uintptr_t va) {
    pte_t *ptep;
    phys_addr_t page_addr;
    uintptr_t page_offset;

    if (!mm) return 0;
    ptep = page_from_virt_user(mm, va);
    if (!ptep) return 0;
    if (!pte_present(*ptep)) return 0;

    page_offset = va & (PAGE_SIZE - 1);
#if defined(__pte_to_phys)
    page_addr = (phys_addr_t) __pte_to_phys(*ptep);
#else
    page_addr = (phys_addr_t) (pte_pfn(*ptep) << PAGE_SHIFT);
#endif
    if (page_addr == 0) return 0;
    return page_addr + page_offset;
}

static int pid_vaddr_to_phy(pid_t global_pid, void *addr, phys_addr_t* pa) {
    struct task_struct *task;
    struct mm_struct *mm;
    struct pid *pid_struct;

    pid_struct = find_get_pid(global_pid);
    if (!pid_struct) return -ESRCH;
    task = get_pid_task(pid_struct, PIDTYPE_PID);
    put_pid(pid_struct);
    if (!task) return -ESRCH;
    mm = get_task_mm(task);
    if (!mm) { put_task_struct(task); return -ESRCH; }

    MM_READ_LOCK(mm)
    *pa = vaddr_to_phy_addr(mm, (uintptr_t)addr);
    MM_READ_UNLOCK(mm)
    mmput(mm);
    put_task_struct(task);

    if (*pa == 0) return -EFAULT;
    return 0;
}

int read_process_memory_ioremap(pid_t pid, void __user*addr, void __user*dest, size_t size) {
    phys_addr_t phy_addr;
    int ret;
    void* mapped;

    if (!addr) return -EINVAL;
    if (!access_ok(VERIFY_WRITE, dest, size)) return -EACCES;

    ret = pid_vaddr_to_phy(pid, addr, &phy_addr);
    if (ret) return ret;
    if (!pfn_valid(__phys_to_pfn(phy_addr))) return -EFAULT;
    if (!IS_VALID_PHYS_ADDR_RANGE(phy_addr, size)) return -EFAULT;

    if (phy_addr) {
        mapped = ioremap_cache(phy_addr, size);
        if (!mapped) ret = -ENOMEM;
        else if (copy_to_user(dest, mapped, size)) ret = -EACCES;
        else ret = 0;
        if (mapped) iounmap(mapped);
    } else ret = -EFAULT;
    return ret;
}

int write_process_memory_ioremap(pid_t pid, void __user*addr, void __user*src, size_t size) {
    phys_addr_t pa;
    int ret;
    void* mapped;

    if (!addr) return -EINVAL;
    if (!access_ok(VERIFY_READ, src, size)) return -EACCES;

    ret = pid_vaddr_to_phy(pid, addr, &pa);
    if (ret) return ret;

    if (pa && pfn_valid(__phys_to_pfn(pa)) && IS_VALID_PHYS_ADDR_RANGE(pa, size)) {
        mapped = ioremap_cache(pa, size);
        if (!mapped) ret = -ENOMEM;
        else if (copy_from_user(mapped, src, size)) ret = -EACCES;
        else ret = 0;
        if (mapped) iounmap(mapped);
    }
    return ret;
}

int access_process_vm_by_pid(pid_t from, void __user*from_addr, pid_t to, void __user*to_addr, size_t size) {
    char *buf;
    int ret;
    struct task_struct *task;
    struct pid *pid_struct;

    pid_struct = find_get_pid(from);
    if (!pid_struct) return -ESRCH;
    task = get_pid_task(pid_struct, PIDTYPE_PID);
    put_pid(pid_struct);
    if (!task) return -ESRCH;

    buf = vmalloc(size);
    if (!buf) { put_task_struct(task); return -ENOMEM; }

    ret = access_process_vm(task, (unsigned long) from_addr, buf, (int) size, 0);
    put_task_struct(task);
    if (ret != size) { vfree(buf); return -EIO; }

    pid_struct = find_get_pid(to);
    if (!pid_struct) { vfree(buf); return -ESRCH; }
    task = get_pid_task(pid_struct, PIDTYPE_PID);
    put_pid(pid_struct);
    if (!task) { vfree(buf); return -ESRCH; }

    ret = access_process_vm(task, (unsigned long) to_addr, buf, (int) size, FOLL_WRITE);
    put_task_struct(task);
    if (ret != size) { vfree(buf); return -EIO; }

    vfree(buf);
    return 0;
}

int read_process_memory(pid_t pid, void *addr, void *dest, size_t size) {
    phys_addr_t pa;
    int ret;
    void* mapped;

    if (!addr) return -EINVAL;
    if (!access_ok(VERIFY_WRITE, dest, size)) return -EACCES;

    ret = pid_vaddr_to_phy(pid, addr, &pa);
    if (ret) return ret;

    if (pa && pfn_valid(__phys_to_pfn(pa)) && IS_VALID_PHYS_ADDR_RANGE(pa, size)) {
        mapped = phys_to_virt(pa);
        if (!mapped) ret = -ENOMEM;
        else if (copy_to_user(dest, mapped, size)) ret = -EACCES;
        else ret = 0;
    }
    return ret;
}

int write_process_memory(pid_t pid, void *addr, void *src, size_t size) {
    phys_addr_t pa;
    int ret;
    void* mapped;

    if (!addr) return -EINVAL;
    if (!access_ok(VERIFY_READ, src, size)) return -EACCES;

    ret = pid_vaddr_to_phy(pid, addr, &pa);
    if (ret) return ret;

    if (pa && pfn_valid(__phys_to_pfn(pa)) && IS_VALID_PHYS_ADDR_RANGE(pa, size)) {
        mapped = phys_to_virt(pa);
        if (!mapped) ret = -ENOMEM;
        else if (copy_from_user(mapped, src, size)) ret = -EACCES;
        else ret = 0;
    }
    return ret;
}
