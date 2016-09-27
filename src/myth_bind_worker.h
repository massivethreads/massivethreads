/* 
 * myth_bind_worker.h
 */
#pragma once
#ifndef MYTH_BIND_WORKER_H_
#define MYTH_BIND_WORKER_H_

void myth_get_available_cpus(void);
int  myth_get_n_available_cpus(void);
void myth_bind_worker(int rank);

#endif	/* MYTH_BIND_WORKER_H_ */
