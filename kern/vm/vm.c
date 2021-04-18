#include <types.h>
#include <kern/errno.h>
#include <lib.h>
#include <thread.h>
#include <addrspace.h>
#include <vm.h>
#include <machine/tlb.h>

/* Place your page table functions here */


void vm_ptecp(paddr_t *** old, paddr_t *** new){
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

void vm_bootstrap(void)
{
    /* Initialise any global components of your VM sub-system here.  
     *  
     * You may or may not need to add anything here depending what's
     * provided or required by the assignment spec.
     */
}

int
vm_fault(int faulttype, vaddr_t faultaddress)
{
    (void) faulttype;
    (void) faultaddress;

    panic("vm_fault hasn't been written yet\n");

    return EFAULT;
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
