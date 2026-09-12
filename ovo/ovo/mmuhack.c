#include "mmuhack.h"
#include <linux/kallsyms.h>
#include <asm/tlbflush.h>
#include <asm/uaccess.h>
#include <asm/pgtable.h>
#include "kkit.h"
#include <linux/ftrace.h>
#include <asm/unistd.h>
#include <linux/unistd.h>
#include <linux/mm.h>
#include <linux/version.h>
#include <linux/moduleloader.h>
#include <linux/stop_machine.h>

static struct mm_struct *init_mm_ptr = NULL;

pte_t *page_from_virt_kernel(unsigned long addr) {
    pgd_t *pgd;
    pud_t *pudp, pud;
    pmd_t *pmdp, pmd;
    pte_t *ptep;

    if (addr & PAGE_SIZE - 1) addr = addr + PAGE_SIZE & ~(PAGE_SIZE - 1);
    if (!init_mm_ptr) init_mm_ptr = (struct mm_struct *) ovo_kallsyms_lookup_name("init_mm");

    pgd = pgd_offset(init_mm_ptr, addr);
    if (pgd_none(*pgd) || pgd_bad(*pgd)) return NULL;

    pudp = pud_offset(pgd, addr);
    pud = READ_ONCE(*pudp);
    if (pud_none(pud) || pud_bad(pud)) return NULL;

    pmdp = pmd_offset(pudp, addr);
    pmd = READ_ONCE(*pmdp);
    if (pmd_none(pmd) || pmd_bad(pmd)) return NULL;

    ptep = pte_offset_kernel(pmdp, addr);
    return ptep;
}

pte_t *page_from_virt_user(struct mm_struct *mm, unsigned long addr) {
    pgd_t *pgd;
    pud_t *pudp, pud;
    pmd_t *pmdp, pmd;
    pte_t *ptep;

    pgd = pgd_offset(mm, addr);
    if (pgd_none(*pgd) || pgd_bad(*pgd)) return NULL;

    pudp = pud_offset(pgd, addr);
    pud = READ_ONCE(*pudp);
    if (pud_none(pud) || pud_bad(pud)) return NULL;

    pmdp = pmd_offset(pudp, addr);
    pmd = READ_ONCE(*pmdp);
    if (pmd_none(pmd) || pmd_bad(pmd)) return NULL;

    ptep = pte_offset_kernel(pmdp, addr);
    return ptep;
}

int protect_rodata_memory(unsigned nr) {
    pte_t pte;
    pte_t* ptep;
    uintptr_t addr;

    addr = (uintptr_t)((uintptr_t) ovo_find_syscall_table() + nr & PAGE_MASK);
    ptep = page_from_virt_kernel(addr);
    if (!pte_valid(READ_ONCE(*ptep))) return -2;

    pte = READ_ONCE(*ptep);
    pte = pte_wrprotect(pte);
    set_pte_at(init_mm_ptr, addr, ptep, pte);
    flush_tlb_kernel_range(addr, addr + PAGE_SIZE);
    return 0;
}

int unprotect_rodata_memory(unsigned nr) {
    pte_t pte;
    pte_t* ptep;
    uintptr_t addr;

    addr = (uintptr_t)((uintptr_t) ovo_find_syscall_table() + nr & PAGE_MASK);
    ptep = page_from_virt_kernel(addr);
    if (!pte_valid(READ_ONCE(*ptep))) return -2;

    pte = READ_ONCE(*ptep);
    pte = pte_mkwrite(pte);
    set_pte_at(init_mm_ptr, addr, ptep, pte);
    flush_tlb_kernel_range(addr, addr + PAGE_SIZE);
    return 0;
}
