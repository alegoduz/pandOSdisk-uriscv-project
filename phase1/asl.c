// SPDX-FileCopyrightText: 2022 Luca Bassi, Gaia Clerici, Mirco Dondi, Fabio Gaiba
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "./headers/asl.h"

static semd_t semd_table[MAXPROC];
static struct list_head semdFree_h;
static struct list_head semd_h;

// Initialize the lists "semdFree_h" and "semd_h" and add all the elements of "semd_table" to "semdFree_h"
void initASL() {
	INIT_LIST_HEAD(&semdFree_h);
	INIT_LIST_HEAD(&semd_h);
	int i;
	for (i = 0; i < MAXPROC; ++i) {
		INIT_LIST_HEAD(&(semd_table[i].s_procq));
		list_add_tail(&(semd_table[i].s_link), &semdFree_h);
	}
}

// Add the PCB "p" to the semaphore with key "semAdd"
int insertBlocked(int* semAdd, pcb_t* p) {
	semd_t* iter;
	list_for_each_entry(iter, &(semd_h), s_link) {
		if (iter->s_key == semAdd) {
			// If there is a semaphore with key "semAdd" add "p" to its PCBs list
			list_add_tail(&(p->p_list), &(iter->s_procq));
			// Set the semaphore key in the PCB
			p->p_semAdd = semAdd;
			// Return FALSE
			return 0;
		}
	}
	if (list_empty(&(semdFree_h))) {
		// Return TRUE if there's not a free semaphore to allocate
		return 1;
	} else {
		// Get the first free semaphore
		struct list_head* ptr = list_next(&(semdFree_h));
		semd_t* semd = container_of(ptr, semd_t, s_link);
		// Remove it from the list "semdFree_h"
		list_del(ptr);
		// Allocate the new semaphore with key "semAdd" and add the PCB to the semaphore PCBs list
		semd->s_key = semAdd;
		list_add_tail(&(p->p_list), &(semd->s_procq));
		// Set the semaphore key in the PCB
		p->p_semAdd = semAdd;
		// Add the semaphore to the ASL
		list_add_tail(&(semd->s_link), &semd_h);
		// Return FALSE
		return 0;
	}
}

// Remove the first PCB from the semaphore with key "semAdd"
pcb_t* removeBlocked(int* semAdd) {
	semd_t* iter;
	list_for_each_entry(iter, &(semd_h), s_link) {
		if (iter->s_key == semAdd) {
			// Get the first process of semaphore with key "semAdd"
			pcb_t* pcb = container_of(list_next(&(iter->s_procq)), pcb_t, p_list);
			// Delete the PCB from the semaphore PCBs list
			list_del(&(pcb->p_list));
			// If the semaphore PCBs list become empty, add the semaphore back to the list "semdFree_h"
			if (list_empty(&(iter->s_procq))) {
				iter->s_key = NULL;
				list_del(&(iter->s_link));
				list_add_tail(&(iter->s_link), &semdFree_h);
			}
			// Unset the PCB semaphore key
			pcb->p_semAdd = NULL;
			return pcb;
		}
	}
	// Return NULL in case of error
	return NULL;
}

// Remove PCB with PID "pid" from its semaphore
pcb_t* outBlockedPid(int pid) {
	semd_t* iter;
	list_for_each_entry(iter, &(semd_h), s_link) {
		pcb_t* p;
		list_for_each_entry(p, &(iter->s_procq), p_list) {
			if (p->p_pid == pid) {
				// Remove the PCB from the semaphore PCBs list
				list_del(&(p->p_list));
				// If the semaphore PCBs list become empty, add the semaphore back to the list "semdFree_h"
				if (list_empty(&(iter->s_procq))) {
					iter->s_key = NULL;
					list_del(&(iter->s_link));
					list_add_tail(&(iter->s_link), &semdFree_h);
				}
				// Unset the PCB semaphore key
				p->p_semAdd = NULL;
				return p;
			}
		}
	}
	// Return NULL in case of error
	return NULL;
}

// Remove PCB "p" from its semaphore
pcb_t* outBlocked(pcb_t* p) {
	semd_t* iter;
	list_for_each_entry(iter, &(semd_h), s_link) {
		if (iter->s_key == p->p_semAdd) {
			// Remove the PCB from the semaphore PCBs list
			list_del(&(p->p_list));
			// If the semaphore PCBs list become empty, add the semaphore back to the list "semdFree_h"
			if (list_empty(&(iter->s_procq))) {
				iter->s_key = NULL;
				list_del(&(iter->s_link));
				list_add_tail(&(iter->s_link), &semdFree_h);
			}
			// Unset the PCB semaphore key
			p->p_semAdd = NULL;
			return p;
		}
	}
	// Return NULL in case of error
	return NULL;
}

// Return the first blocked PCB of the semaphore with key "semAdd"
pcb_t* headBlocked(int* semAdd) {
	semd_t* iter;
	list_for_each_entry(iter, &(semd_h), s_link) {
		if (iter->s_key == semAdd) {
			// If the PCBs list of semaphore with key "semAdd" is not empty, return the first blocked PCB
			if (!list_empty(&(iter->s_procq))) {
				return container_of(list_next(&(iter->s_procq)), pcb_t, p_list);
			}
		}
	}
	// Return NULL in case of error
	return NULL;
}
