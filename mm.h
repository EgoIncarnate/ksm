/*
 * ksm - a really simple and fast x64 hypervisor
 * Copyright (C) 2016 Ahmed Samy <f.fallen45@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef __MM_H
#define __MM_H

#ifdef __linux__
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/highmem.h>
#include <linux/vmalloc.h>
#include <linux/module.h>
#include <linux/sched.h>
#endif

#ifndef PXI_SHIFT
#define PXI_SHIFT		39
#endif

#ifndef PPI_SHIFT
#define PPI_SHIFT		30
#endif

#ifndef PDI_SHIFT
#define PDI_SHIFT		21
#endif

#ifndef PTI_SHIFT
#define PTI_SHIFT		12
#endif

#ifndef PTE_SHIFT
#define PTE_SHIFT		3
#endif

#define VA_BITS			48
#define VA_MASK			((1ULL << VA_BITS) - 1)
#define VA_SHIFT		16

#ifndef PTX_MASK
#define PTX_MASK		0x1FF
#endif

#ifndef PPI_MASK
#define PPI_MASK		0x3FFFF
#endif

#ifndef PDI_MASK
#define PDI_MASK		0x7FFFFFF
#endif

#ifndef PTI_MASK
#define PTI_MASK		0xFFFFFFFFF
#endif

#ifndef __linux__
extern uintptr_t pxe_base;
extern uintptr_t ppe_base;
extern uintptr_t pde_base;
extern uintptr_t pte_base;
#endif

#define PAGE_PRESENT		0x1
#define PAGE_WRITE		0x2
#define PAGE_USER		0x4
#define PAGE_WRITETHRU		0x8
#define PAGE_CACHEDISABLE	0x10
#define PAGE_ACCESSED		0x20
#define PAGE_DIRTY		0x40
#define PAGE_LARGE		0x80
#define PAGE_GLOBAL		0x100
#define PAGE_COPYONWRITE	0x200
#define PAGE_PROTOTYPE		0x400
#define PAGE_TRANSIT		0x800
#define PAGE_PA_MASK		(0xFFFFFFFFFULL << PAGE_SHIFT)
#define PAGE_PA(page)		((page) & PAGE_PA_MASK)
#define PAGE_FN(page)		(((page) >> PTI_SHIFT) & PTI_MASK)
#define PAGE_SOFT_WS_IDX_SHIFT	52
#define PAGE_SOFT_WS_IDX_MASK	0xFFF
#define PAGE_NX			0x8000000000000000
#define PAGE_LPRESENT		(PAGE_PRESENT | PAGE_LARGE)

#define PGF_PRESENT		0x1	/* present fault  */
#define PGF_WRITE		0x2	/* write fault  */
#define PGF_SP			0x4	/* supervisor fault (SMEP, SMAP)  */
#define PGF_RSVD		0x8	/* reserved bit was set fault  */
#define PGF_FETCH		0x10	/* fetch fault  */
#define PGF_PK			0x20	/* Protection key fault  */
#define PGF_SGX			0x40	/* SGX induced fault  */

#define __pxe_idx(addr)		(((addr) >> PXI_SHIFT) & PTX_MASK)
#define __ppe_idx(addr)		(((addr) >> PPI_SHIFT) & PTX_MASK)
#define __pde_idx(addr)		(((addr) >> PDI_SHIFT) & PTX_MASK)
#define __pte_idx(addr)		(((addr) >> PTI_SHIFT) & PTX_MASK)

#ifndef __linux__
#define __pa(va)	\
	MmGetPhysicalAddress((void *)(va)).QuadPart
#define __va(pa)	\
	(uintptr_t *)MmGetVirtualForPhysical((PHYSICAL_ADDRESS) { .QuadPart = (uintptr_t)(pa) })
#endif

#define page_align(addr)	((uintptr_t)(addr) & ~(PAGE_SIZE - 1))
static inline bool page_aligned(uintptr_t addr)
{
	return (addr & (PAGE_SIZE - 1)) == 0;
}

static inline size_t round_to_pages(size_t size)
{
	return (size >> PAGE_SHIFT) + ((size & (PAGE_SIZE - 1)) != 0);
}

static inline u16 addr_offset(uintptr_t addr)
{
	/* Get the lower 12 bits which represent the offset  */
	return addr & (PAGE_SIZE - 1);
}

static inline bool same_page(uintptr_t a1, uintptr_t a2)
{
	return page_align(a1) == page_align(a2);
}

static inline bool is_canonical_addr(u64 addr)
{
	return (s64)addr >> 47 == (s64)addr >> 63;
}

static inline u64 *page_addr(u64 *pte)
{
	if (!pte || !*pte)
		return 0;

	return __va(PAGE_PA(*pte));
}

static inline int pte_soft_ws_idx(u64 *pte)
{
	return (*pte >> PAGE_SOFT_WS_IDX_SHIFT) & PAGE_SOFT_WS_IDX_MASK;
}

static inline bool pte_large(u64 *pte)
{
	return *pte & PAGE_LARGE;
}

#ifndef __linux__
static inline bool pte_present(u64 pte)
{
	return pte & PAGE_PRESENT;
}

static inline bool pte_trans(u64 pte)
{
	return pte & PAGE_TRANSIT;
}

static inline bool pte_prototype(u64 pte)
{
	return pte & PAGE_PROTOTYPE;
}

static inline bool pte_large_present(u64 pte)
{
	return (pte & PAGE_LPRESENT) == PAGE_LPRESENT;
}

static inline bool pte_swapper(u64 pte)
{
	if (!pte_present(pte))
		return false;

	return pte_trans(pte) && !pte_prototype(pte);
}

static inline u64 *va_to_pxe(uintptr_t va)
{
	uintptr_t off = (va >> PXI_SHIFT) & PTX_MASK;
	return (u64 *)pxe_base + off;
}

static inline u64 *va_to_ppe(uintptr_t va)
{
	uintptr_t off = (va >> PPI_SHIFT) & PPI_MASK;
	return (u64 *)ppe_base + off;
}

static inline u64 *va_to_pde(uintptr_t va)
{
	uintptr_t off = (va >> PDI_SHIFT) & PDI_MASK;
	return (u64 *)pde_base + off;
}

static inline u64 *va_to_pte(uintptr_t va)
{
	uintptr_t off = (va >> PTI_SHIFT) & PTI_MASK;
	return (u64 *)pte_base + off;
}

static inline uintptr_t __pte_to_va(u64 pte)
{
	return (((pte - pte_base) << (PAGE_SHIFT + VA_SHIFT - PTE_SHIFT)) >> VA_SHIFT);
}

static inline u64 va_to_pa(uintptr_t va)
{
	uintptr_t *pte = va_to_pde(va);
	if (!pte_large(pte))
		pte = va_to_pte(va);

	if (!(*pte & PAGE_PRESENT))
		return 0;

	return PAGE_PA(*pte) | addr_offset(va);
}

static inline u64 *__cr3_resolve_va(u64 cr3, u64 va)
{
	/* NB: You can also use va_to_pte / va_to_pde, etc.  */
	u64 *pml4 = __va(cr3 & PAGE_PA_MASK);
	u64 *pdpt = page_addr(&pml4[__pxe_idx(va)]);
	if (!pdpt)
		return 0;

	u64 *pdt = page_addr(&pdpt[__ppe_idx(va)]);
	if (!pdt)
		return 0;

	u64 *pdte = &pdt[__pde_idx(va)];
	if (!(*pdte & PAGE_PRESENT))
		return 0;

	if (pte_large(pdte))
		return pdte;

	u64 *pt = page_addr(pdte);
	if (pt)
		return &pt[__pte_idx(va)];

	return 0;
}

static inline bool consult_vad(u64 va)
{
	return !(*va_to_pde(va) & PAGE_PRESENT) || *va_to_pte(va) == 0;
}

static inline bool is_software_pte(u64 pte)
{
	return !pte_trans(pte) && !pte_prototype(pte);
}

static inline bool is_subsection_pte(u64 pte)
{
	return !pte_present(pte) && pte_prototype(pte);
}

static inline bool is_demandzero_pte(u64 pte)
{
	return !pte_present(pte) && !pte_prototype(pte) && !pte_trans(pte);
}

static inline bool is_phys(uintptr_t va)
{
	return (*va_to_pxe(va) & PAGE_PRESENT) && (*va_to_ppe(va) & PAGE_PRESENT) &&
		(pte_large_present(*va_to_pde(va)) || (pte_present(*va_to_pte(va))));
}

/* Transitition page  (Unique defines only...)  */
#define PTT_PROTECTION_SHIFT	5
#define PTT_PROTECTION_MASK	0x1F

static inline u8 ptt_protection(uintptr_t *pte)
{
	return (*pte >> PTT_PROTECTION_SHIFT) & PTT_PROTECTION_MASK;
}

/* Prototype PTE  (Unique defines only...)  */
#define PRT_PROTECTION_SHIFT		11
#define PRT_PROTECTION_MASK		0x3F
#define PRT_PROTO_ADDRESS_SHIFT		VA_SHIFT
#define PRT_PROTO_ADDRESS_MASK		VA_MASK
#define PRT_READONLY			0x100

static inline u8 prt_prot(u64 pte)
{
	return (pte >> PRT_PROTECTION_SHIFT) & PRT_PROTECTION_MASK;
}

static inline uintptr_t prt_addr(u64 pte)
{
	return (pte >> PRT_PROTO_ADDRESS_SHIFT) & PRT_PROTO_ADDRESS_MASK;
}

static inline bool prt_ro(u64 pte)
{
	return pte & PRT_READONLY;
}

static inline bool prt_is_vad(u64 pte)
{
	return prt_addr(pte) == 0xFFFFFFFF0000;
}

/* Software PTE  */
#define SPTE_PF_LO_SHIFT	1			/* Number of page file (up to 16) */
#define SPTE_PF_LO_MASK		0x1F
#define SPTE_PF_HI_SHIFT	32			/* Page file offset (multiple of PAGE_SIZE)  */
#define SPTE_PF_HI_MASK		0xFFFFFFFF
#define SPTE_IN_STORE_MASK	0x400000
#define SPTE_PROTECTION_SHIFT	5
#define SPTE_PROTECTION_MASK	0x1F

static inline bool spte_in_store(u64 spte)
{
	return spte & SPTE_IN_STORE_MASK;
}

static inline bool spte_prot(u64 spte)
{
	return (spte >> SPTE_PROTECTION_SHIFT) & SPTE_PROTECTION_MASK;
}

static inline u32 spte_pg_hi(u64 spte)
{
	return (spte >> SPTE_PF_HI_SHIFT) & SPTE_PF_HI_MASK;
}

static inline u32 spte_pg_lo(u64 spte)
{
	return (spte >> SPTE_PF_LO_SHIFT) & SPTE_PF_LO_MASK;
}
#else
static inline u64 *va_to_pxe(uintptr_t va)
{
	return (u64 *)pgd_offset(current->mm, va);
}

static inline u64 *va_to_ppe(uintptr_t va)
{
	return (u64 *)pud_offset((pgd_t *)va_to_pxe(va), va);
}

static inline u64 *va_to_pde(uintptr_t va)
{
	return (u64 *)pmd_offset((pud_t *)va_to_ppe(va), va);
}

static inline u64 *va_to_pte(uintptr_t va)
{
	return (u64 *)pte_offset_kernel((pmd_t *)va_to_pde(va), va);
}

static inline uintptr_t __pte_to_va(u64 pte)
{
	struct page *page = pfn_to_page(PAGE_FN(pte));
	if (!page)
		return 0;

	return (uintptr_t)page_address(page);
}

static inline u64 *__cr3_resolve_va(uintptr_t cr3, uintptr_t va)
{
	pgd_t *pgd;
	pud_t *pud;
	pmd_t *pmd;
	pte_t *pte;

	pgd = pgd_offset(current->mm, va);
	if (pgd_none(*pgd) || pgd_bad(*pgd))
		return 0;

	pud = pud_offset(pgd, va);
	if (pud_none(*pud) || pud_bad(*pud))
		return 0;

	pmd = pmd_offset(pud, va);
	if (pmd_none(*pmd) || pmd_bad(*pmd))
		return 0;

	pte = pte_offset_kernel(pmd, va);
	return (u64 *)pte;
}

static inline u64 va_to_pa(uintptr_t va)
{
	u64 *pte = __cr3_resolve_va(0, va);
	if (!pte || !pte_present(*(pte_t *)pte))
		return 0;

	return PAGE_PA(*pte) | addr_offset(va);
}

static inline void __stosq(unsigned long long *a, unsigned long x, unsigned long count)
{
	/* Generates stosq anyway...  */
	memset(a, x, count << 3);
}
#endif

static inline u64 cr3_resolve_va(uintptr_t cr3, uintptr_t va)
{
	u64 *page = __cr3_resolve_va(cr3, va);
	if (*page & PAGE_PRESENT)
		return 0;

	return PAGE_PA(*page) | addr_offset(va);
}

static inline void *pte_to_va(u64 pte)
{
	return (void *)__pte_to_va(pte);
}

static inline void *mm_alloc_page(void)
{
#ifndef __linux__
	void *v = ExAllocatePool(NonPagedPool, PAGE_SIZE);
	if (v)
		__stosq(v, 0, PAGE_SIZE >> 3);

	return v;
#else
	return (void *)get_zeroed_page(GFP_KERNEL);
#endif
}

static inline void __mm_free_page(void *v)
{
#ifndef __linux__
	ExFreePool(v);
#else
	free_page((unsigned long)v);
#endif
}

static inline void mm_free_page(void *v)
{
	__stosq(v, 0, PAGE_SIZE >> 3);
	__mm_free_page(v);
}

static inline void *mm_alloc_pool(size_t size)
{
#ifndef __linux__
	void *v = ExAllocatePool(NonPagedPool, size);
	if (v)
		__stosq(v, 0, size >> 3);

	return v;
#else
	return kmalloc(size, GFP_KERNEL | __GFP_ZERO);
#endif
}

static inline void mm_free_pool(void *v, size_t size)
{
	if (size)
		__stosq(v, 0, size >> 3);

#ifdef __linux__
	kfree(v);
#else
	ExFreePool(v);
#endif
}

#ifndef __linux__
static inline void *kmap_iomem(u64 addr, size_t size)
{
	return MmMapIoSpace((PHYSICAL_ADDRESS) { .QuadPart = addr }, size, MmNonCached);
}

static inline void kunmap_iomem(void *addr, size_t size)
{
	return MmUnmapIoSpace(addr, size);
}
#else
static inline void *kmap_iomem(unsigned long addr, unsigned long size)
{
	struct page *page = pfn_to_page(addr >> PAGE_SHIFT);
	if (!page)
		return NULL;

	/* should we use kmap() and kunmap() down below?  */
	return page_address(page);
}

static inline void kunmap_iomem(void *addr, unsigned long size)
{
	/* Do nothing  */
}

/*
 * From KSplice:
 *
 *  Original:
 *	 * map_writable creates a shadow page mapping of the range
 *	 [addr, addr + len) so that we can write to code mapped read-only.
 *
 *	 It is similar to a generalized version of x86's text_poke.  But
 *	 because one cannot use vmalloc/vfree() inside stop_machine, we use
 *	 map_writable to map the pages before stop_machine, then use the
 *	 mapping inside stop_machine, and unmap the pages afterwards.
 *
 *	https://github.com/jirislaby/ksplice/tree/master
 *	kmodsrc/ksplice.c
 *
 *  Copyright (C) 2007-2009  Ksplice, Inc.
 *  Authors: Jeff Arnold, Anders Kaseorg, Tim Abbott
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License, version 2.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street - Fifth Floor, Boston, MA
 *  02110-1301, USA.
 */
static void *kmap_virt(void *addr, size_t len, pgprot_t prot)
{
	int i;
	void *vaddr;
	int nr_pages = DIV_ROUND_UP(offset_in_page(addr) + len, PAGE_SIZE);
	struct page **pages = kmalloc(nr_pages * sizeof(*pages), GFP_KERNEL);
	void *page_addr = (void *)((unsigned long)addr & PAGE_MASK);

	if (!pages)
		return NULL;

	for (i = 0; i < nr_pages; ++i) {
		if (!__module_address((unsigned long)page_addr)) {
			pages[i] = virt_to_page(page_addr);
			WARN_ON(!PageReserved(pages[i]));
		} else {
			/* Modules are allocated via vmalloc() which is
			 * non-contiguous.  */
			pages[i] = vmalloc_to_page(page_addr);
		}

		if (!pages[i]) {
			kfree(pages);
			return NULL;
		}

		page_addr += PAGE_SIZE;
	}

	vaddr = vmap(pages, nr_pages, VM_MAP, prot);
	kfree(pages);
	if (!vaddr)
		return NULL;

	return vaddr + offset_in_page(addr);
}

static inline void *kmap_exec(void *addr, size_t len)
{
	return kmap_virt(addr, len, PAGE_KERNEL_EXEC);
}

static inline void *kmap_write(void *addr, size_t len)
{
	return kmap_virt(addr, len, PAGE_KERNEL);
}
#endif
#endif
