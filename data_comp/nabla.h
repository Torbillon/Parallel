#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <ctype.h>
#include <mpi.h>
#include <string.h>
#ifndef PRE_COMP_H
#define PRE_COMP_H
#include "preprocessor_comp.h"
#endif
#ifndef FILE_SYS_H
#define FILE_SYS_H
#include "file_sys.h"
#endif
#define TAG 1
#define MPI_FILE_TAG 2
#define MAX_ARGV 4
#define MEGABYTE 100000000
#define DEBUG_MODE 1
#define GLOBAL_ROOT_RANK 0

struct mpi_data {
  MPI_Comm curr_comm;
  int global_rank;
  int global_nproc;
  int local_rank;
  int local_nproc;
  int local_color;
  int split_count;
  int leader_rank;
};

typedef struct mpi_data mpi_data_t;
typedef mpi_data_t* mpi_data_hand_t;

struct nabla_data {
  int** leader_level;
  int* nproc_array;
  int total_level;
};

typedef struct nabla_data nabla_data_t;
typedef nabla_data_t* nabla_data_hand_t;

// prototypes
void pre_proc(int argc, char** argv, nabla_data_hand_t* nabla_data_hand, file_data_hand_t* file_data_hand, mpi_data_hand_t* mpi_data_hand, stack_tuple_hand_t* bio_data_hand);
void get_data(stack_tuple_hand_t bio_data, mpi_data_hand_t mpi_data, int i);
void send_data(nabla_data_hand_t nabla_data, stack_tuple_hand_t bio_data, mpi_data_hand_t mpi_data, int i);
void split_comm(mpi_data_hand_t mpi_data);
void get_total_bytes(stack_tuple_hand_t bio_data);
void wait_proc(mpi_data_hand_t mpi_data, nabla_data_hand_t nabla_data);
void data_display(mpi_data_hand_t mpi_data, stack_tuple_hand_t bio_data);
void clean_up(stack_tuple_hand_t bio_data, mpi_data_hand_t mpi_data);
void end_clock();
void time_display(int rank);
int is_leader(nabla_data_hand_t nabla_data, mpi_data_hand_t mpi_data, int i);
int next_level_leader(nabla_data_hand_t nalba_data, mpi_data_hand_t mpi_data, int level);
int next_leader_rank(int* level_array, int rank, int size);
int get_index(int* level_array, int rank, int size);
int** roll_array(int* level_array_unroll, int* nproc_array, int total_level);
int sum_bytes(char* x, int s);
int sum_ints(int* x, int s);
void data_aware_comp(stack_tuple_hand_t bio_data, mpi_data_hand_t mpi_data);
void finalize(mpi_data_hand_t mpi_data, nabla_data_hand_t nabla_data, stack_tuple_hand_t bio_data);

