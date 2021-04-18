#include <types.h>
#include <kern/errno.h>
#include <lib.h>
#include <thread.h>
#include <addrspace.h>
#include <vm.h>
#include <machine/tlb.h>

/* Place your page table functions here */


void vm_ptecp(paddr_t *** old, paddr_t *** new) {
    for(int i = 0; i< PAGE_SIZE; i++){
        if(old[i] == NULL){
            continue;
        }
        new[i] = kmalloc(sizeof(paddr_t **) * PAGE_SIZE);
        for(int j = 0; J < PAGE_SIZE; j++){
            if(old[i][j] = NULL){
                continue;
            }
        }

    }
}

int insert_page_table_entry (struct addrspace *as, vaddr_t vaddr, paddr_t paddr) {
    uint32_t hbits = level_1_bits(vaddr);
    uint32_t mbits = level_2_bits(vaddr);
    uint32_t lbits = level_3_bits(vaddr);

    if (hbits >= 256 || mbits >= 64 || lbits >= 64) {
        return EFAULT;
    }

    if (as->pagetable[hbits] == NULL) {
        as->pagetable[hbits] = kmalloc(sizeof(paddr_t **) * 64);
        if (as->pagetable[hbits] == NULL) {
            return ENOMEM;
        }
        for (int i = 0; i < 64; i++) {
            as->pagetable[hbits][i] = NULL;
        }
        as->pagetable[hbits][mbits] = kmalloc(sizeof(paddr_t *) * 64);
        if (as->pagetable[hbits][mbits] == NULL) {
            return ENOMEM;
        }
        bzero((void *)as->pagetable[hbits][mbits]);
    } else if (as->pagetable[hbits][mbits] == NULL) {
        as->pagetable[hbits][mbits] = kmalloc(sizeof(paddr_t *) * 64);
        if (as->pagetable[hbits][mbits] == NULL) {
            return ENOMEM;
        }
        bzero((void *)as->pagetable[hbits][mbits]);
    } else {
        // Check if there is something in page already
        if (as->pagetable[hbits][mbits][lbits] != 0){
            return EFAULT; 
        }
    }
    
    as->pagetable[hbits][mbits][lbits] = paddr;
    return 0;
}

int check_entry_exist(struct addrspace *as, vaddr_t vaddr) {
    uint32_t hbits = level_1_bits(vaddr);
    uint32_t mbits = level_2_bits(vaddr);
    uint32_t lbits = level_3_bits(vaddr);

    if (hbits >= 256 || mbits >= 64 || lbits >= 64) {
        return EFAULT;
    } else if (as->pagetable == NULL) {
        return -1;
    } else if (as->pagetable[hbits] == NULL) {
        return -1;
    } else if (as->pagetable[hbits][mbits] == NULL) {
        return -1;
    } else if (as->pagetable[hbits][mbits][lbits] == 0) {
        return -1;
    } else {
        return 0;
    }
}

int update_page_table_entry(struct addrspace *as, vaddr_t vaddr, paddr_t paddr) {
    int result = check_entry_exist(as, vaddr);
    if (result != 0) return result;
    uint32_t hbits = level_1_bits(vaddr);
    uint32_t mbits = level_2_bits(vaddr);
    uint32_t lbits = level_3_bits(vaddr);

    as->pagetable[hbits][mbits][lbits] = paddr;
    return 0;
}

void vm_bootstrap(void)
{
    /* Initialise any global components of your VM sub-system here.  
     *  
     * You may or may not need to add anything here depending what's
     * provided or required by the assignment spec.
     */
}

int vm_fault(int faulttype, vaddr_t faultaddress)
{
    (void) faulttype;
    (void) faultaddress;

    panic("vm_fault hasn't been written yet\n");

    return EFAULT;

    if (faulttype == VM_FAULT_READONLY) {
        return EFAULT;
    } else if (faulttype != VM_FAULT_WRITE && faulttype != VM_FAULT_READ) {
        return EINVAL
    }

    if (curproc == NULL) return EFAULT;

    struct addrspace *as = proc_getas();
    if (as == NULL) return EFAULT;
    if (as->pagetable == NULL) return EFAULT;

    uint32_t hbits = level_1_bits(faultaddress);
    uint32_t mbits = level_2_bits(faultaddress);
    uint32_t lbits = level_3_bits(faultaddress);

    if (check_entry_exist(as, faultaddress) == 0)
    {
        if (check_region_exists(as, faultaddress, faulttype) == 0) {
            load_tlb(faultaddress & PAGE_FRAME, pagetable[hbits][mbits][lbits]);
            return 0;
        }
        return EFAULT;
    }

}

/*
 * SMP-specific functions.  Unused in our UNSW configuration.
 */

void
vm_tlbshootdown(const struct tlbshootdown *ts)
{
	(void)ts;
	panic("vm tried to do tlb shootdown?!\n");
}

vaddr_t level_1_bits (vaddr_t addr) {
    return addr >> 24; //getting the top 8 bits of the 32 bit address.
}

vaddr_t level_2_bits (vaddr_t addr) {
    return (addr << 8) >> 26; //getting the middle 6 bits of the 32 bit address.
}

vaddr_t level_3_bits (vaddr_t addr) {
    return (addr << 14) >> 26; //getting the lower 6 bits of the 32 bit address.
}

int check_region_exists(struct addrspace *as, vaddr_t vaddr, int faulttype) {
    struct region *head = as->start_of_regions;
    while(head != NULL) {
        if ((vaddr < (head->start + head->size)) && vaddr >= head->start) {
            break;
        }
        head = head->next;
    }

    if (head == NULL) return EFAULT; 
    if (faulttype == VM_FAULT_WRITE) {
        if (head->write_flag == 0) return EPERM;
        else return 0;
    } else if (faulttype == VM_FAULT_READ) {
        if (head->read_flag == 0) return EPERM;
        else return 0;
    } else {
        return EINVAL;
    }
    return 0;
}

