#include <stdio.h>
#include <stdlib.h>
#ifndef FILE_SYS_H
#define FILE_SYS_H
#include "file_sys.h"
#endif

/*
 * compute nproc_array
 */
int* compute_nproc_array(char* process_file, int* token_count);

/* 
 * compute process_leaders
 */
int* compute_process_leaders(int* nproc_array, int nproc, int total_level);

/* 
 * distribute work evenly amongst $(nproc) processes
 */
file_hand_array_array_t distribute_work(file_hand_array_t file_array_hand_t, int size, int nproc, int rank);

/*
 * compute sum of all elements in buf
 */
int sum(int* buf, int size);
