#include <types.h>
#include <kern/errno.h>
#include <lib.h>
#include <thread.h>
#include <addrspace.h>
#include <vm.h>
#include <machine/tlb.h>

/* Place your page table functions here */


void vm_ptecp(paddr_t *** old, paddr_t *** new){
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
        for(int j = 0; J < 64; j++){
            if(old[i][j] == NULL){
                continue;
            }
            new[i][j] = kmalloc(sizeof(paddr_t *) * 64);
            if(new[i][j] == NULL){
                return ENOMEM;
            }
            bezero((void *)new[i][j], 64);
            // do the memmove
            if(memmove((void *)new[i][j], (const void *)PADDR_TO_KVADDR(old[i][j] & PAGE_FRAME), PAGE_SIZE) == NULL){

            }
            for(int k = 0; k < PAGE_SZIE; k++){

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
