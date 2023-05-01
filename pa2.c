/**********************************************************************
 * Copyright (c) 2019-2023
 *  Sang-Hoon Kim <sanghoonkim@ajou.ac.kr>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTIABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 **********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "types.h"
#include "list_head.h"

/**
 * The process which is currently running
 */
#include "process.h"
extern struct process *current;

/**
 * List head to hold the processes ready to run
 */
extern struct list_head readyqueue;

/**
 * Resources in the system.
 */
#include "resource.h"
extern struct resource resources[NR_RESOURCES];

/**
 * Monotonically increasing ticks. Do not modify it
 */
extern unsigned int ticks;

/**
 * Quiet mode. True if the program was started with -q option
 */
extern bool quiet;

/***********************************************************************
 * Default FCFS resource acquision function
 *
 * DESCRIPTION
 *   This is the default resource acquision function which is called back
 *   whenever the current process is to acquire resource @resource_id.
 *   The current implementation serves the resource in the requesting order
 *   without considering the priority. See the comments in sched.h
 ***********************************************************************/
static bool fcfs_acquire(int resource_id)
{
	struct resource *r = resources + resource_id;

	if (!r->owner) {
		/* This resource is not owned by any one. Take it! */
		r->owner = current;
		return true;
	}

	/* OK, this resource is taken by @r->owner. */
	/* Update the current process state */
	current->status = PROCESS_BLOCKED;

	/* And append current to waitqueue */
	list_add_tail(&current->list, &r->waitqueue);

	/**
	 * And return false to indicate the resource is not available.
	 * The scheduler framework will soon call schedule() function to
	 * schedule out current and to pick the next process to run.
	 */
	return false;
}

/***********************************************************************
 * Default FCFS resource release function
 *
 * DESCRIPTION
 *   This is the default resource release function which is called back
 *   whenever the current process is to release resource @resource_id.
 *   The current implementation serves the resource in the requesting order
 *   without considering the priority. See the comments in sched.h
 ***********************************************************************/
static void fcfs_release(int resource_id)
{
	struct resource *r = resources + resource_id;

	/* Ensure that the owner process is releasing the resource */
	assert(r->owner == current);

	/* Un-own this resource */
	r->owner = NULL;

	/* Let's wake up ONE waiter (if exists) that came first */
	if (!list_empty(&r->waitqueue)) {
		struct process *waiter = list_first_entry(&r->waitqueue, struct process, list);

		/**
		 * Ensure the waiter is in the wait status
		 */
		assert(waiter->status == PROCESS_BLOCKED);

		/**
		 * Take out the waiter from the waiting queue. Note we use
		 * list_del_init() over list_del() to maintain the list head tidy
		 * (otherwise, the framework will complain on the list head
		 * when the process exits).
		 */
		list_del_init(&waiter->list);

		/* Update the process status */
		waiter->status = PROCESS_READY;

		/**
		 * Put the waiter process into ready queue. The framework will
		 * do the rest.
		 */
		list_add_tail(&waiter->list, &readyqueue);
	}
}

#include "sched.h"

/***********************************************************************
 * FIFO scheduler
 ***********************************************************************/
static int fifo_initialize(void)
{
	return 0;
}

static void fifo_finalize(void)
{
}

static struct process *fifo_schedule(void)
{
//dump_status();

	struct process *next = NULL;
	// dump_status();
	if (!current || current->status == PROCESS_BLOCKED) 
		goto pick_next;
	

	if (current->age < current->lifespan) {
	//	dump_status();
	//	current->age = current->age+1;
	//	printf("          remain %d\n", current->lifespan - current->age);
	//

		return current;

	}
	

pick_next:

	if (!list_empty(&readyqueue)) {
		next = list_first_entry(&readyqueue, struct process, list);
		list_del_init(&next->list);
	}
//	dump_status();

	return next;
}

struct scheduler fifo_scheduler = {
	.name = "FIFO",
	.acquire = fcfs_acquire,
	.release = fcfs_release,
	.initialize = fifo_initialize,
	.finalize = fifo_finalize,
	.schedule = fifo_schedule,
};

/***********************************************************************
 * SJF scheduler
 ***********************************************************************/
static struct process *sjf_schedule(void)
{
	struct process *next = NULL;
	if (!current || current->status == PROCESS_BLOCKED) {
		goto pick_next;
	}
	if (current->age < current->lifespan) {
	//	printf("current->age : %d\n", current->age);
		return current;
	}

pick_next:
	if (!list_empty(&readyqueue)) {
		struct process *iter;
		list_for_each_entry(iter, &readyqueue, list) 
			if (!next || iter->lifespan < next->lifespan) 
				next = iter;
			
		list_del_init(&next->list);
		
	}
	return next;
}
struct scheduler sjf_scheduler = {
	.name = "Shortest-Job First",
	.acquire = fcfs_acquire,	/* Use the default FCFS acquire() */
	.release = fcfs_release,	/* Use the default FCFS release() */
	.schedule = sjf_schedule,			/* TODO: Assign your schedule function  
								   to this function pointer to activate
								   SJF in the simulation system */
};

/***********************************************************************
 * STCF scheduler
 ***********************************************************************/

int remain_time(struct process *tmp)
{
	return (tmp->lifespan - tmp->age);
}

struct process* get_shortest(void)
{
	struct process *tmp;
	struct process *shortest = list_first_entry(&readyqueue, struct process, list);
	list_for_each_entry(tmp, &readyqueue, list){
		if(remain_time(tmp) < remain_time(shortest))
			shortest = tmp;
		
	}
	return shortest;
	

}	

static struct process *stcf_schedule(void) {
	struct process *next = NULL;
	if (!current || current->status == PROCESS_BLOCKED) 
		goto pick_next;
	
	if(current->age < current->lifespan){
	//	dump_status();
		if (get_shortest() != current){
	//		if(remain_time(current) > 0 ){
				list_add(&current->list, &readyqueue);
	//		}
		goto pick_next;	
		}	
		else
			return current;
	}
	

pick_next:
	if(!list_empty(&readyqueue)){
		next = get_shortest();
		list_del_init(&next->list);
	}
//	dump_status();
//	if(list_empty(&readyqueue))
//		printf("list_empty(&readyqueue)");


	
	return next;



}


struct scheduler stcf_scheduler = {
	.name = "Shortest Time-to-Complete First",
	.acquire = fcfs_acquire, /* Use the default FCFS acquire() */
	.release = fcfs_release, /* Use the default FCFS release() */
	.schedule = stcf_schedule,
	/* You need to check the newly created processes to implement STCF.
	 * Have a look at @forked() callback.
	 */

	/* Obviously, you should implement stcf_schedule() and attach it here */
};

/***********************************************************************
 * Round-robin scheduler
 ***********************************************************************/
LIST_HEAD(rr_stack);

static struct process* rr_schedule(void) {
	struct process* next = NULL;

	if (!current || current->status == PROCESS_BLOCKED)
		goto pick_next;

	if (current->age < current->lifespan) {
		if (remain_time(current) > 0) {
			struct process* new = current;
			list_add(&new->list, &rr_stack);
			goto pick_next;
		}	
	}


pick_next:
	if (!(list_empty(&readyqueue) && list_empty(&rr_stack))) {
		if (!list_empty(&readyqueue)) {
			next = list_first_entry(&readyqueue, struct process, list);
			list_del_init(&next->list);
		}
		else if (list_empty(&readyqueue)) {
			if (!list_empty(&rr_stack)) {
				struct process* pos, * tmp;
				list_for_each_entry_safe(pos, tmp, &rr_stack, list) {
					list_del(&pos->list);
					list_add(&pos->list, &readyqueue);
				}

			}
			next = list_first_entry(&readyqueue, struct process,list);
			list_del_init(&next->list);
		}
	}
	return next;
}


struct scheduler rr_scheduler = {
	.name = "Round-Robin",
	.acquire = fcfs_acquire, /* Use the default FCFS acquire() */
	.release = fcfs_release, /* Use the default FCFS release() */
	.schedule = rr_schedule,
	/* Obviously, ... */
};

/***********************************************************************
 * Priority scheduler
 ***********************************************************************/
/*
static bool prio_acquire(int resource_id)
{
	struct resource*r = resources + resource_id;
	if(!r->owner) {
		r->owner = current;
		return true;
	}
	current->status = PROCESS_BLOCKED;
	list_add_tail(&current->list, &r->waitqueue);
	return false;
}

static void prio_release(int resource_id)
{
	struct resource* r = resources + resource_id;
	assert(r->owner == current);
	r->owner = NULL;
	if (!list_empty(&r->waitqueue)) {
		struct process* waiter = list_first_entry(&r->waitqueue, struct process, list);
		assert(waiter->status == PROCESS_BLOCKED);
		list_del_init(&waiter->list);
		waiter->status = PROCESS_READY;
		list_add_tail(&waiter->list, &readyqueue);
	}
}
*/
/*******************************************************/
/*
struct process* get_highest_prio(void) {
	struct process* tmp;
	struct process* highest = list_first_enty(&r->waitqueue, struct process, list);
	list_for_each_entry(tmp, &r->waitqueue, list) {
		if (tmp->prio > highest->prio)
			highest = tmp;
	}
	return highest;



}
*/

/***************************************************************/
static bool prio_acquire(int resource_id){
	struct resource *r = resources + resource_id;
	if(!r->owner){
		r->owner = current;
		return true;
	}
	
	current->status = PROCESS_BLOCKED;
	list_add_tail(&current->list, &r->waitqueue);
	return false;
}

struct process* get_highest_prio_wait(struct resource *r) {
	struct process* tmp;
	struct process* highest = list_first_entry(&r->waitqueue, struct process, list);
	list_for_each_entry(tmp, &r->waitqueue, list)
		if(tmp->prio > highest->prio)
			highest = tmp;
	return highest;
}

static void prio_release(int resource_id){
	struct resource *r = resources + resource_id;
	assert(r->owner == current);
	r->owner = NULL;

	if(!list_empty(&r->waitqueue)){
		struct process* waiter = get_highest_prio_wait(r);
	
	assert(waiter->status == PROCESS_BLOCKED);
	list_del_init(&waiter->list);

	waiter->status = PROCESS_READY;

	list_add_tail(&waiter->list, &readyqueue);
	}

}

LIST_HEAD(prio_stack);

struct process* get_same_prio(struct process* curr) {
	struct process* tmp;
	struct process* same_prio = curr;
	list_for_each_entry(tmp, &readyqueue, list) 
		if(tmp->prio == same_prio->prio)
			return same_prio;
	return NULL;
}

struct process* get_highest_prio(void){

//	if(!list_empty(&readyqueue)){
//		return 
//	}
	struct process* tmp;
	struct process* highest = list_first_entry(&readyqueue, struct process, list);
	list_for_each_entry(tmp, &readyqueue, list) 
		if(tmp->prio > highest->prio)
			highest = tmp;
	
	return highest;
}

unsigned int get_stack_prio(void){
	return list_first_entry(&prio_stack, struct process, list)->prio;
}

static struct process* prio_schedule(void){
	struct process* next = NULL;


	/*
	if (current != NULL){
	printf("origin_prio == %d\n",current->prio_orig);
	printf("prio==%d\n", current->prio++);
	}*/
//	dump_status();
	if(!current||current->status == PROCESS_BLOCKED)
		goto pick_next;

	if(current->age < current->lifespan){
	//	current->prio = current->prio_orig;
		if (( get_highest_prio()->prio == current->prio)){
			if(list_empty(&readyqueue)){
				if(list_empty(&prio_stack)){
					return current;
				}
			}
			struct process* new = current;
			list_add(&new->list, &prio_stack);
			goto pick_next;

		}
		else{
			if(get_stack_prio() == current->prio){
				
				struct process* last = current;
				list_add(&last->list, &prio_stack);
				goto pick_next;
			}
			else{
			struct process* new = current;
			list_add(&new->list, &readyqueue);
			goto pick_next;
			}
		}
	}
	

pick_next:
	if (!(list_empty(&readyqueue) && list_empty(&prio_stack))) {
		if(list_empty(&readyqueue)){
			if(!list_empty(&prio_stack)){
				struct process* pos, *tmp;
				list_for_each_entry_safe(pos, tmp, &prio_stack, list){
					list_del(&pos->list);
					list_add(&pos->list, &readyqueue);
				}

			}

		}





		next = get_highest_prio();
		list_del_init(&next->list);
		
		if(!list_empty(&prio_stack)){
			
			if(get_stack_prio() != next->prio){
				
				struct process* pos, * tmp;
				list_for_each_entry_safe(pos, tmp, &prio_stack, list) {
					list_del(&pos->list);
					list_add(&pos->list, &readyqueue);
				}
				list_add(&next->list, &readyqueue);	
				next = get_highest_prio();
				list_del_init(&next->list);

			}
		}

	}
	return next;
}






struct scheduler prio_scheduler = {
	.name = "Priority",
	.acquire = prio_acquire,
	.release = prio_release,
	.schedule = prio_schedule,
	//.schedule = prio_schedule,
	/**
	 * Implement your own acqure/release function to make the priority
	 * scheduler correct.
	 */

	/* Implement your own prio_schedule() and attach it here */
};

/***********************************************************************
 * Priority scheduler with aging
 ***********************************************************************/

LIST_HEAD(pa_stack);


void prio_reset(struct process* curr){
	curr->prio = curr->prio_orig;
}

void prio_boost(void){
	struct process *tmp;
	list_for_each_entry(tmp, &readyqueue, list){
		tmp->prio++;
		if(tmp->prio == MAX_PRIO)
			tmp->prio = MAX_PRIO;
	}
	
	struct process *prio_tmp;
	list_for_each_entry(prio_tmp, &pa_stack, list){
		prio_tmp->prio++;
		if(prio_tmp->prio == MAX_PRIO)
			prio_tmp->prio = MAX_PRIO;
	}
}

unsigned int get_pastack_prio(void) {
	return list_first_entry(&pa_stack, struct process, list)->prio;
}

static struct process* pa_schedule(void) {
	struct process* next = NULL;
	if(current != NULL)
	{	
	//	printf("current->prio == %d\n", current->prio);
		prio_reset(current);
	//	printf("after reset\ncurrent->prio == %d\n", current->prio);
		prio_boost();
	//	struct process* tmp;
	//	printf("readyqueue->prio == \n");
	//	list_for_each_entry(tmp, &readyqueue, list)
	//		printf("%d	", tmp->prio);
	//	printf("\n");
	}
//		dump_status();
	if (!current || current->status == PROCESS_BLOCKED)
		goto pick_next;

	if (current->age < current->lifespan) {

		if ((get_highest_prio()->prio == current->prio)) {
			if (list_empty(&readyqueue)) {
				if (list_empty(&pa_stack)) {
				//	dump_status();
					return current;
				}
			
			}
			//printf("there is same prio\n");
			struct process* new = current;
			list_add(&new->list, &pa_stack);
			goto pick_next;

		}
		else {
			if (get_pastack_prio() == current->prio) {

				struct process* last = current;
				list_add(&last->list, &pa_stack);
				goto pick_next;
			}
			else {
				struct process* new = current;
				list_add_tail(&new->list, &readyqueue);
				goto pick_next;
			}
		}
	}


pick_next:
	if (!(list_empty(&readyqueue) && list_empty(&pa_stack))) {
		if (list_empty(&readyqueue)) {
			if (!list_empty(&pa_stack)) {
				struct process* pos, * tmp;
				list_for_each_entry_safe(pos, tmp, &pa_stack, list) {
					list_del(&pos->list);
					list_add_tail(&pos->list, &readyqueue);
				}

			}

		}





		next = get_highest_prio();
		list_del_init(&next->list);

		if (!list_empty(&pa_stack)) {
			//	list_add(&next->list, &readyqueue);
			if (get_pastack_prio() != next->prio) {
//	printf("hi");
				struct process* pos, * tmp;
				list_for_each_entry_safe(pos, tmp, &pa_stack, list) {
				////	printf("pid == %d\n", pos->pid);
					list_del(&pos->list);
					list_add_tail(&pos->list, &readyqueue);
				}
				list_add(&next->list, &readyqueue);
				next = get_highest_prio();
				list_del_init(&next->list);

			}
		}

	}
//	dump_status();
	return next;
}
struct scheduler pa_scheduler = {
	.name = "Priority + aging",
	.acquire = prio_acquire,
	.release = prio_release,
	.schedule = pa_schedule,
	/**
	 * Ditto
	 */
};

/***********************************************************************
 * Priority scheduler with priority ceiling protocol
 ***********************************************************************/

static bool pcp_acquire(int resource_id) {
	struct resource* r = resources + resource_id;
	if (!r->owner) {
		r->owner = current;
/*
		if(list_empty(&pcp_stack)){
			if(!list_empty(&readyqueue)){
				if(current->prio < get_highest_prio()->prio 
			}
		
		}*/
		current->prio = MAX_PRIO;
			
	//	dump_status();
		return true;
	}
	current->status = PROCESS_BLOCKED;
	list_add_tail(&current->list, &r->waitqueue);
	return false;
}


static void pcp_release(int resource_id) {
	struct resource* r = resources + resource_id;
	assert(r->owner == current);
	r->owner->prio = current->prio_orig;
//	dump_status();
	r->owner = NULL;

	if (!list_empty(&r->waitqueue)) {
		struct process* waiter = get_highest_prio_wait(r);

		assert(waiter->status == PROCESS_BLOCKED);
		list_del_init(&waiter->list);

		waiter->status = PROCESS_READY;

		list_add_tail(&waiter->list, &readyqueue);
	}

}









LIST_HEAD(pcp_stack);






unsigned int get_pcpstack_prio(void) {
	return list_first_entry(&pcp_stack, struct process, list)->prio;
}



static struct process* pcp_schedule(void) {
	struct process* next = NULL;
	//	dump_status();
	if (!current || current->status == PROCESS_BLOCKED)
		goto pick_next;

	if (current->age < current->lifespan) {

		if ((get_highest_prio()->prio == current->prio)) {
			if (list_empty(&readyqueue)) {
				if (list_empty(&pcp_stack)) {
				//	 dump_status();
					return current;
				}
			}
			struct process* new = current;
			list_add(&new->list, &pcp_stack);
			goto pick_next;

		}
		else {
			if (get_pcpstack_prio() == current->prio) {

				struct process* last = current;
				list_add(&last->list, &pcp_stack);
				goto pick_next;
			}
			else {
				struct process* new = current;
				list_add_tail(&new->list, &readyqueue);
				goto pick_next;
			}
		}
	}


pick_next:
	if (!(list_empty(&readyqueue) && list_empty(&pcp_stack))) {
		if (list_empty(&readyqueue)) {
			if (!list_empty(&pcp_stack)) {
				struct process* pos, * tmp;
				list_for_each_entry_safe(pos, tmp, &pcp_stack, list) {
					list_del(&pos->list);
					list_add(&pos->list, &readyqueue);
				}

			}

		}





		next = get_highest_prio();
		list_del_init(&next->list);

		if (!list_empty(&pcp_stack)) {

			if (get_pcpstack_prio() != next->prio) {

				struct process* pos, * tmp;
				list_for_each_entry_safe(pos, tmp, &pcp_stack, list) {
					list_del(&pos->list);
					list_add(&pos->list, &readyqueue);
				}
				list_add(&next->list, &readyqueue);
				next = get_highest_prio();
				list_del_init(&next->list);

			}
		}

	}
//	dump_status();
	return next;
}



struct scheduler pcp_scheduler = {
	.name = "Priority + PCP Protocol",
	.acquire = pcp_acquire,
	.release = pcp_release,
	.schedule = pcp_schedule,
	/**
	 * Ditto
	 */
};

/***********************************************************************
 * Priority scheduler with priority inheritance protocol
 ***********************************************************************/



static bool pip_acquire(int resource_id) {
	//dump_status();
	struct resource* r = resources + resource_id;
	if (!r->owner) {
		r->owner = current;

		return true;
	}
	current->status = PROCESS_BLOCKED;
	list_add_tail(&current->list, &r->waitqueue);
	if(current->prio == get_highest_prio_wait(r)->prio){
		r->owner->prio = current->prio;
	}
	return false;
}


static void pip_release(int resource_id) {
	struct resource* r = resources + resource_id;
	assert(r->owner == current);
	r->owner->prio = current->prio_orig;
	r->owner = NULL;

	if (!list_empty(&r->waitqueue)) {
		struct process* waiter = get_highest_prio_wait(r);

		assert(waiter->status == PROCESS_BLOCKED);
		list_del_init(&waiter->list);

		waiter->status = PROCESS_READY;

		list_add_tail(&waiter->list, &readyqueue);
	}

}






LIST_HEAD(pip_stack);

unsigned int get_pipstack_prio(void) {
	return list_first_entry(&pip_stack, struct process, list)->prio;
}

static struct process* pip_schedule(void) {
	struct process* next = NULL;
	if (!current || current->status == PROCESS_BLOCKED)
		goto pick_next;

	if (current->age < current->lifespan) {

		if ((get_highest_prio()->prio == current->prio)) {
			if (list_empty(&readyqueue)) {
				if (list_empty(&pip_stack)) {
					return current;
				}
			}
			struct process* new = current;
			list_add(&new->list, &pip_stack);
			goto pick_next;

		}
		else {
			if (get_pipstack_prio() == current->prio) {

				struct process* last = current;
				list_add(&last->list, &pip_stack);
				goto pick_next;
			}
			else {
				struct process* new = current;
				list_add_tail(&new->list, &readyqueue);
				goto pick_next;
			}
		}
	}


pick_next:
	if (!(list_empty(&readyqueue) && list_empty(&pip_stack))) {
		if (list_empty(&readyqueue)) {
			if (!list_empty(&pip_stack)) {
				struct process* pos, * tmp;
				list_for_each_entry_safe(pos, tmp, &pip_stack, list) {
					list_del(&pos->list);
					list_add(&pos->list, &readyqueue);
				}

			}

		}





		next = get_highest_prio();
		list_del_init(&next->list);

		if (!list_empty(&pip_stack)) {

			if (get_pipstack_prio() != next->prio) {

				struct process* pos, * tmp;
				list_for_each_entry_safe(pos, tmp, &pcp_stack, list) {
					list_del(&pos->list);
					list_add(&pos->list, &readyqueue);
				}
				list_add(&next->list, &readyqueue);
				next = get_highest_prio();
				list_del_init(&next->list);

			}
		}

	}
	return next;
}





struct scheduler pip_scheduler = {
	.name = "Priority + PIP Protocol",
	.acquire = pip_acquire,
	.release = pip_release,
	.schedule = pip_schedule,
	/**
	 * Ditto
	 */
};
