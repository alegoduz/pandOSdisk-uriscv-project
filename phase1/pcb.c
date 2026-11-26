// SPDX-FileCopyrightText: 2022 Luca Bassi, Gaia Clerici, Mirco Dondi, Fabio Gaiba
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "./headers/pcb.h"

static struct list_head pcbFree_h;
static pcb_t pcbFree_table[MAXPROC];
static int next_pid = 1;

// Initialize "pcbFree_h" and add elements of "pcbFree_table" to the list "pcbFree_h"
void initPcbs() {
	INIT_LIST_HEAD(&pcbFree_h);
	int i;
	for (i = 0; i < MAXPROC; ++i) {
		freePcb(&pcbFree_table[i]);
	}
}

// Add PCB "p" to the list "pcbFree_h"
void freePcb(pcb_t* p) {
	list_add_tail(&(p->p_list), &pcbFree_h);
}

// Allocate new PCB removing one from list "pcbFree_h" if possible
pcb_t* allocPcb() {
	// Check if there is a PCB in the list "pcbFree_h", if none return NULL (error)
	if (list_empty(&pcbFree_h)) {
		return NULL;
	} else {
		// Get the first PCB from list "pcbFree_h"
		struct list_head* ptr = list_next(&pcbFree_h);
		pcb_t* pcb = container_of(ptr, pcb_t, p_list);
		// Delete the PCB from list "pcbFree_h"
		list_del(ptr);
		// Initialize all the PCB fields
		INIT_LIST_HEAD(&(pcb->p_list));
		pcb->p_parent = NULL;
		INIT_LIST_HEAD(&(pcb->p_child));
		INIT_LIST_HEAD(&(pcb->p_sib));
		pcb->p_s.entry_hi = 0;
		pcb->p_s.cause = 0;
		pcb->p_s.status = 0;
		pcb->p_s.pc_epc = 0;
		pcb->p_s.mie = 0;
		for (int i = 0; i < STATE_GPR_LEN; ++i) {
			pcb->p_s.gpr[i] = 0;
		}
		pcb->p_time = 0;
		pcb->p_semAdd = NULL;
		pcb->p_supportStruct = NULL;
		pcb->p_prio = PROCESS_PRIO_LOW;
		pcb->p_pid = next_pid++;
		// Return the initialized PCB
		return pcb;
	}
}

// Create an empty PCB list
void mkEmptyProcQ(struct list_head* head) {
	INIT_LIST_HEAD(head);
}

// Check if the PCB list "head" is empty
int emptyProcQ(struct list_head* head) {
	return list_empty(head);
}

// Insert PCB "p" in the list "head"
void insertProcQ(struct list_head* head, pcb_t* p) {
	list_add_tail(&(p->p_list), head);
}

// Return the first PCB in the list "head" without removing it
pcb_t* headProcQ(struct list_head* head) {
	return container_of(list_next(head), pcb_t, p_list);
}

// Remove and return the first PCB in the list "head"
pcb_t* removeProcQ(struct list_head* head) {
	// Check if the list pointed by "head" is empty, return NULL (error)
	if (list_empty(head)) {
		return NULL;
	} else {
		// Get the first PCB from list "head"
		struct list_head* ptr = list_next(head);
		pcb_t* pcb = container_of(ptr, pcb_t, p_list);
		// Delete the PCB from the list
		list_del(ptr);
		// Return the PCB
		return pcb;
	}
}

// Remove PCB "p" from the list "head"
pcb_t* outProcQ(struct list_head* head, pcb_t* p) {
	pcb_t* iter;
	// Search the PCB in the list
	list_for_each_entry(iter, head, p_list) {
		if (iter == p) {
			// If found, delete from the list and return it
			list_del(&(iter->p_list));
			return iter;
		}
	}
	// Return NULL otherwise
	return NULL;
}

// Check if the PCB "p" has children
int emptyChild(pcb_t* p) {
	return list_empty(&(p->p_child));
}

// Insert the PCB child "p" into the PCB parent "prnt"
void insertChild(pcb_t* prnt, pcb_t* p) {
	// Check if the child is in the parent's children list (additional check)
	if (p->p_parent == prnt) {
		return;
	}
	// Add the new child to the parent child list
	list_add_tail(&(p->p_sib), &(prnt->p_child));
	// Set the child parent
	p->p_parent = prnt;
}

// Remove and return the first child of the PCB "p"
pcb_t* removeChild(pcb_t* p) {
	// Check if the children list pointed of "p" is empty, return NULL (error)
	if (list_empty(&(p->p_child))) {
		return NULL;
	} else {
		// Get the first child
		struct list_head* ptr = list_next(&(p->p_child));
		pcb_t* pcb = container_of(ptr, pcb_t, p_sib);
		// Remove child from the children list
		list_del(ptr);
		// Unset the parent of the PCB and return it
		pcb->p_parent = NULL;
		return pcb;
	}
}

// Remove and return the PCB "p" from the parent's children list
pcb_t* outChild(pcb_t* p) {
	// If the PCB "p" has a parent
	if (!(p->p_parent == NULL)) {
		pcb_t* iter;
		// Check if the child is in the parent's children list (additional check)
		list_for_each_entry(iter, &(p->p_parent->p_child), p_sib) {
			if (iter == p) {
				// Remove child from the children list
				list_del(&(iter->p_sib));
				// Unset the parent of the PCB and return it
				iter->p_parent = NULL;
				return iter;
			}
		}
	}
	// Return NULL otherwise
	return NULL;
}
