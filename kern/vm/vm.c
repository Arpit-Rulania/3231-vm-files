#include <types.h>
#include <kern/errno.h>
#include <lib.h>
#include <thread.h>
#include <addrspace.h>
#include <vm.h>
#include <machine/tlb.h>

#include <current.h>
#include <proc.h>
#include <spl.h>

/* Place your page table functions here */
vaddr_t level_1_bits (vaddr_t addr) {
    return addr >> 24; //getting the top 8 bits of the 32 bit address.
}

vaddr_t level_2_bits (vaddr_t addr) {
    return (addr << 8) >> 26; //getting the middle 6 bits of the 32 bit address.
}

vaddr_t level_3_bits (vaddr_t addr) {
    return (addr << 14) >> 26; //getting the lower 6 bits of the 32 bit address.
}

int vm_ptecp(paddr_t *** old, paddr_t *** new){
    //first level
    for(int i = 0; i< 256; i++){
        if(old[i] == NULL){
            continue;
        }
        new[i] = kmalloc(sizeof(paddr_t **) * 64);
        if(new[i] == NULL){
            return ENOMEM;
        }
        for(int x = 0; x < 64; x ++){
            new[i][x] =NULL;
        }
        for(int j = 0; j < 64; j++){
            if(old[i][j] == NULL){
                continue;
            }
            new[i][j] = kmalloc(sizeof(paddr_t *) * 64);
            if(new[i][j] == NULL){
                return ENOMEM;
            }
            bzero((void *)new[i][j], 64);
            // do the memmove
            
            for(int k = 0; k < 64; k++){
                if(old[i][j][k] == 0){
                    new[i][j][k] = 0;
                }else{
                    
                    if(memmove((void *)new[i][j][k], (const void *)PADDR_TO_KVADDR(old[i][j][k] & PAGE_FRAME), PAGE_SIZE) == NULL){
                        //free the already malloced shit
                        return ENOMEM;
                    }
                    vaddr_t framecp = alloc_kpages(1);
                    bzero((void *)new[i][j], PAGE_SIZE);
                    //if(framecp == NULL){
                      //  return ENOMEM;
                    //}
                    new[i][j][k] = (KVADDR_TO_PADDR(framecp) & PAGE_FRAME) | (old[i][j][k] & TLBLO_DIRTY) | TLBLO_VALID;;

                }
            }
        }

    }
    return 0;
}

void freePTE(paddr_t ***tofree){
    for(int i = 0; i<256; i++){
        if(tofree[i] == NULL){
            continue;
        }
        for(int j = 0; j < 64; j++){
            if(tofree[i][j] == NULL){
                continue;
            }
            for(int k = 0; k < 64; k++){
                if(tofree[i][j][k] != 0){
                    free_kpages(PADDR_TO_KVADDR(tofree[i][j][k] & PAGE_FRAME));
                }
            }
            kfree(tofree[i][j]);
        }
        kfree(tofree[i]);
    }
    kfree(tofree);
    return;
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
        bzero((void *)as->pagetable[hbits][mbits], 64);
    } else if (as->pagetable[hbits][mbits] == NULL) {
        as->pagetable[hbits][mbits] = kmalloc(sizeof(paddr_t *) * 64);
        if (as->pagetable[hbits][mbits] == NULL) {
            return ENOMEM;
        }
        bzero((void *)as->pagetable[hbits][mbits], 64);
    } else {
        // Check if there is something in page already
        if (as->pagetable[hbits][mbits][lbits] != 0){
            return EFAULT; 
        }
    }
    
    as->pagetable[hbits][mbits][lbits] = paddr;
    return 0;
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
    if (faultaddress == 0) return EFAULT;
    if (curproc == NULL) return EFAULT;

    if (faulttype == VM_FAULT_READONLY) {
        return EFAULT;
    } else if (faulttype != VM_FAULT_WRITE && faulttype != VM_FAULT_READ) {
        return EINVAL;
    }

    // get the address space
    struct addrspace *as = proc_getas();
    if (as == NULL) return EFAULT;
    if (as->pagetable == NULL) return EFAULT;
    if (as->start_of_regions == NULL) return EFAULT;

    uint32_t hbits = level_1_bits(faultaddress);
    uint32_t mbits = level_2_bits(faultaddress);
    uint32_t lbits = level_3_bits(faultaddress);

    // if entry is there in the page table then load it if the region is valid.
    if (check_entry_exist(as, faultaddress) == 0)
    {
        if (check_region_exists(as, faultaddress, faulttype) == 0) {
            int sql = splhigh();
            tlb_random(faultaddress & TLBHI_VPAGE, as->pagetable[hbits][mbits][lbits]);
            splx(sql);
            return 0;
        } else {
            return EFAULT;
        }
    }

    // if the entry does not exist then check if the region is valid.
    if (check_region_exists(as, faultaddress, faulttype) != 0) {
        return check_region_exists(as, faultaddress, faulttype);
    }

    // if the region is valid then allocate a page and a frame for that entry trying to be accessed.
    vaddr_t nV = alloc_kpages(1);
    if (nV == 0) return ENOMEM;
    bzero((void *)nV, PAGE_SIZE);
    paddr_t pV = KVADDR_TO_PADDR(nV) & TLBHI_VPAGE;
    struct region *curr = as->start_of_regions;
    // get the current region...
    while (curr != NULL) {
        if ((faultaddress < (curr->start + curr->size)) && faultaddress >= curr->start) {
            break;
        } else {
            curr = curr->next;
        }
    }
    if (curr->write_flag) {
        pV = pV|TLBLO_DIRTY;
    }
    if (insert_page_table_entry(as, faultaddress, pV|TLBLO_VALID)) return ENOMEM;
    int sql = splhigh();
    tlb_random((faultaddress & TLBHI_VPAGE), as->pagetable[hbits][mbits][lbits]);
    splx(sql);
    return 0;  
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





