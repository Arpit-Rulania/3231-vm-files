/*
 * Copyright (c) 2000, 2001, 2002, 2003, 2004, 2005, 2008, 2009
 *	The President and Fellows of Harvard College.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE UNIVERSITY AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE UNIVERSITY OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <types.h>
#include <kern/errno.h>
#include <lib.h>
#include <spl.h>
#include <spinlock.h>
#include <current.h>
#include <mips/tlb.h>
#include <addrspace.h>
#include <vm.h>
#include <proc.h>
#include <synch.h>

/*
 * Note! If OPT_DUMBVM is set, as is the case until you start the VM
 * assignment, this file is not compiled or linked or in any way
 * used. The cheesy hack versions in dumbvm.c are used instead.
 *
 * UNSW: If you use ASST3 config as required, then this file forms
 * part of the VM subsystem.
 *
 */


struct addrspace *
as_create(void)
{
	struct addrspace *as;

	as = kmalloc(sizeof(struct addrspace));
	if (as == NULL) {
		return NULL;
	}
	// first level pagetable is 2^8 256
	as -> start_of_regions = NULL;
	as -> pagetable = kmalloc(sizeof(paddr_t**)* 256); ////////////////////////////////////////////////////////////////////////////////////////
	//as -> pagetable = kmalloc(4 * 256);
	for(int i = 0; i < (256); i++){
		as -> pagetable[i] = NULL;
	}
	as -> thelock = lock_create("thelock");
	if(as -> thelock == NULL){
		kfree(as -> pagetable);
		kfree(as);
		return NULL;
	}
	/*
	 * Initialize as needed.
	 */
	
	return as;
}

void
as_destroy(struct addrspace *as)
{
	/*
	 * Clean up as needed.
	 */
	lock_acquire(as -> thelock);
	struct region *tofree = as -> start_of_regions;
	struct region *temp;
	while(tofree != NULL){
		temp = tofree;
		tofree = tofree -> next;
		kfree(temp);
		
	}
	// need to free up the page table;
	lock_release(as -> thelock);
	lock_destroy(as -> thelock);
	freePTE(as -> pagetable);
	kfree(as);
}

int as_copy(struct addrspace *old, struct addrspace **ret) {
	struct addrspace *newas;

	newas = as_create();
	if (newas==NULL) {
		return ENOMEM;
	}

	/*
	 * Write this.
	 */
	lock_acquire(newas->thelock);
	lock_acquire(old->thelock);
	struct region *old_re = old -> start_of_regions;
	struct region *new = NULL;
	struct region *temp = NULL;
	if(old == NULL){
		return 0;
	}else{
		temp = kmalloc(sizeof(struct region));
		if(temp == NULL){
			as_destroy(newas);
			lock_release(old->thelock);
			lock_release(newas->thelock);
			return ENOMEM;
		}
		//vaddr_t addr = alloc_kpages(old_re -> size);
		//donno if we need to do this 
		//memcpy(addr, old_re -> start, old_re -> size);
		temp -> start = old_re -> start;
		temp -> size = old_re -> size;
		temp -> read_flag = old_re -> read_flag;
		temp -> write_flag = old_re -> write_flag;
		temp -> pre_write = old_re -> pre_write;
		temp -> pre_read = old_re -> pre_read;
		temp -> next = NULL;
		//as_define_region(newas, old_re -> start, old_re ->size, old_re -> read_flag, old_re ->read_flag, 1);
		old_re = old_re -> next;
		new = temp;
	}

	while(old_re != NULL){
		temp = kmalloc(sizeof(struct region));
		if(temp == NULL){
			as_destroy(newas);
			lock_release(old->thelock);
			lock_release(newas->thelock);
			return ENOMEM;
		}
		//vaddr_t addr = alloc_kpages(old_re -> size);
		//donno if we need to do this
		//memcpy(addr, old_re -> start, old_re -> size);
		temp -> start = old_re -> start;
		temp -> size = old_re -> size;
		temp -> read_flag = old_re -> read_flag;
		temp -> write_flag = old_re -> write_flag;
		temp -> pre_write = old_re -> pre_write;
		temp -> pre_read = old_re -> pre_read;
		temp -> next = NULL;
		new -> next = temp;
		new = new -> next;
		old_re = old_re -> next;

	}
	newas -> start_of_regions = new;
	//copy pagetable
	if(vm_ptecp(old -> pagetable, newas -> pagetable) != 0){
		lock_release(old->thelock);
		lock_release(newas->thelock);
		return ENOMEM;
	}
	//(void)old;

	lock_release(old->thelock);
	lock_release(newas->thelock);
	*ret = newas;
	return 0;
}



void as_activate(void) {
	struct addrspace *as;

	as = proc_getas();
	if (as == NULL) {
		/*
		 * Kernel thread without an address space; leave the
		 * prior address space in place.
		 */
		return;
	}

	/*
	 * Write this.
	 */

	// this is from dumb vm.c
	int spl = splhigh();

	for (int i=0; i<NUM_TLB; i++) {
		tlb_write(TLBHI_INVALID(i), TLBLO_INVALID(), i);
	}

	splx(spl);
}

void as_deactivate(void) {
	as_activate();
}

int as_define_region(struct addrspace *as, vaddr_t vaddr, size_t memsize, 
					 int readable, int writeable, int executable) {
	/*
	 * Write this.
	 */

	//(void)as;
	//(void)vaddr;
	//(void)memsize;
	//(void)readable;
	//(void)writeable;
	(void)executable;

	// copied from dumb vm
	memsize +=  vaddr & ~(vaddr_t)PAGE_FRAME;
	vaddr &= PAGE_FRAME;

	memsize = (memsize + PAGE_SIZE -1) & PAGE_FRAME;
	// create the region

	struct region *new = kmalloc(sizeof(struct region));

	if (new == NULL){
		return ENOMEM;
	} 
	new -> start = vaddr;
	new -> size = memsize;
	new -> write_flag = writeable;
	new -> read_flag = readable;
	new -> pre_read = readable;
	new -> pre_write = writeable;

	struct region *tmp = as -> start_of_regions;
	new -> next = tmp;
	as -> start_of_regions = new;


	return 0; /* Unimplemented */
}

int
as_prepare_load(struct addrspace *as)
{
	/*
	 * Write this.
	 */
	struct region *temp = as -> start_of_regions;


	while(temp != NULL){
		temp -> pre_read = temp -> read_flag;
		temp -> pre_write = temp -> write_flag;
		temp -> read_flag = 1;
		temp -> write_flag = 1;
		temp = temp -> next;
	}


	//(void)as;
	return 0;
}

int
as_complete_load(struct addrspace *as)
{
	/*
	 * Write this.
	 */
	as_activate();
	lock_acquire(as->thelock);
	paddr_t*** table = as -> pagetable;

	for(int i = 0; i<256; i++){
		if(table[i] != NULL){
			for(int j = 0; j<64; j++){
				if(table[i][j] != NULL){
					for(int k = 0; k<64; k++){
						if(table[i][j][k] != 0){
							vaddr_t h_bit = i << 24;
							vaddr_t m_bit = j << 18;
							vaddr_t l_bit = k << 12;
							vaddr_t total = h_bit | m_bit | l_bit;
							struct region *curr = as->start_of_regions;
    						// get the current region...
							while (curr != NULL) {
								if ((total < (curr->start + curr->size)) && total >= curr -> start) {
									break;
								} else {
									curr = curr->next;
								}
								
							}
							if(curr != NULL){
								if(curr -> pre_write == 0){
									table[i][j][k] = (table[i][j][k] & PAGE_FRAME) | TLBLO_VALID;
								}
							}

						}
					}
				}
			}
		}
	}

	//(void)as;
	lock_release(as->thelock);
	struct region *tmp = as -> start_of_regions;
	while(tmp != NULL){
		tmp -> write_flag = tmp -> pre_write;
		tmp -> read_flag = tmp -> pre_read;
		tmp = tmp -> next;
	}
	return 0;
}

int
as_define_stack(struct addrspace *as, vaddr_t *stackptr)
{
	/*
	 * Write this.
	 */

	(void)as;
	int read = 1;
	int write = 1;
	int exc = 0;
	/* Initial user-level stack pointer */
	*stackptr = USERSTACK;
	int result = as_define_region(as, *stackptr - SPEC_STACK_SIZE, SPEC_STACK_SIZE, read, write, exc);
	return result;
}

