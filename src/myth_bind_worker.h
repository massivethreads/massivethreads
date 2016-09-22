/* 
 * myth_bind_worker.h
 */
#pragma once
#ifndef MYTH_BIND_WORKER_H_
#define MYTH_BIND_WORKER_H_

void myth_read_available_cpu_list(void);
int myth_get_cpu_num(void);
void myth_bind_worker(int rank);

#endif	/* MYTH_BIND_WORKER_H_ */
