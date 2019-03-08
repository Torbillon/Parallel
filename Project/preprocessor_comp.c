#include <stdio.h>
#include <string.h>
#include <mpi.h>
#ifndef PRE_COMP_H
#define PRE_COMP_H
#include "preprocessor_comp.h"
#endif
#ifndef FILE_SYS_H
#define FILE_SYS_H
#include "file_sys.h"
#endif

int* compute_nproc_array(char* file_name, int* token_count) {
  
  FILE *open_file = fopen(file_name,"r");
  if(open_file == NULL) {
    printf("Error: File cannot be opened\n");
    MPI_Abort(MPI_COMM_WORLD, 1);
  }

  // get_token_count and other variables
  *token_count = get_token_count(file_name);
  int* nproc_array = (int *) malloc(sizeof(int) * *token_count);
  char buf[FILE_READER_BUF_SIZE];
  char* curr_token;
  int nlevel = 0;

  // for each line
  while(fgets(buf, FILE_READER_BUF_SIZE, open_file) != NULL) {

    // tokenize
    curr_token = strtok(buf, DELIMITER_CHARS);
    while(curr_token != NULL) {
      nproc_array[nlevel] = atoi(curr_token);
      nlevel++;
      curr_token = strtok(NULL, DELIMITER_CHARS);
    }
  }

  fclose(open_file);
  return nproc_array;  
}


int* compute_process_leaders(int* nproc_array, int nproc, int total_level) {
  int* proc_leaders = (int *) malloc(sizeof(int) * (sum(nproc_array, total_level) + total_level));
  
  // compute leaders for level 0
  int i;
  for(i = 0; i < nproc; i++) {
    proc_leaders[i] = i;
  }
  proc_leaders[nproc] = -1; 
  int num_prev, num_curr, stride, curr_sum = 0, prev_sum = 0;

  // compute leaders for rest of levels
  for(i = 1; i < total_level; i++) {

    prev_sum = curr_sum;
    curr_sum += nproc_array[i-1] + 1;
    num_prev = nproc_array[i-1];
    num_curr = nproc_array[i];
    stride = num_prev/num_curr;
    int j, k = 0;
    for(j = 0; j < num_curr; j++) {
      proc_leaders[curr_sum + j] = proc_leaders[prev_sum + k];
      k += stride;
    }
    proc_leaders[curr_sum + num_curr] = -1;
  }
  return proc_leaders;
} 

int sum(int* buf, int size) {
  int i;
  long sum = 0;
  for(i = 0; i < size; i++) {
    sum += buf[i];
  }
  return sum;
}

// distribution algorithm
file_hand_array_array_t distribute_work(file_hand_array_t file_hand_array, int size, int nproc, int rank) {

  // allocate $(file_number) of arrays
  file_hand_array_array_t file_hand_array_array = (file_hand_array_array_t) malloc(sizeof(file_hand_array_t) * nproc);
  int div = size / nproc;
  int rem = size % nproc;
  int total = div;
  int i;
  
  // distribute
  for(i = 0; i < nproc; i++) {
    if(rank == nproc - 1 && rem != 0) {
      total += rem;
    }
    file_hand_array_t temp = (file_hand_array_t) malloc(sizeof(file_hand_t) * total + 1);
    int start_idx = div * i;
    int j;
    for(j = 0; j < total; j++) {
      temp[j] = file_hand_array[start_idx + j];
    }
    temp[total] = NULL;
    file_hand_array_array[rank] = temp;
  }
  return file_hand_array_array;
}
