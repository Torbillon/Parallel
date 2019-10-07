#ifndef NABLA_H
#define NABLA_H
#include "nabla.h"
#endif

// global time variables
double comp_time = 0.0;
double comm_time = 0.0;
double comm_start_time = 0.0;
double comm_end_time = 0.0;
double io_time = 0.0;
double io_start_time = 0.0;
double io_end_time = 0.0;
double tot_time = 0.0;
double tot_start_time = 0.0;
double tot_end_time = 0.0;

// global byte variables
int total_bytes = 0, global_total_bytes = 0;
void pre_proc(int argc, char** argv, nabla_data_hand_t* nabla_data_hand, file_data_hand_t* file_data_hand, mpi_data_hand_t* mpi_data_hand, stack_tuple_hand_t* bio_data_hand) {
  
  if(argc != MAX_ARGV) {
    printf("Error: No input file or  is given\n");
    exit(1);
  }
   
  // TODO implement flags for this part
  char* input_file = argv[1];
  char* process_file = argv[2];
  int line_count = atoi(argv[3]);

  // total levels in hierarchy
  int total_level;

  // get array of file_hand_t
  int file_count;
  
  // array of (most significant) largest process rank per level
  // i'th index will have largest process rank for data compressor level i
  int* nproc_array;

  // array of array of leaders for each level
  // index i will contain all the leaders for that level
  int* leader_level_unroll;
  int** leader_level;

  // array of arrays of filenames (strings)
  // each array of filenames corresponds to one process on level 0 of the hierarchy
  file_hand_array_t file_hand_array;
  
  // stack tuple structure
  stack_tuple_hand_t bio_data;

  // mpi data structure
  mpi_data_hand_t mpi_data;
 
  // nabla data structure
  nabla_data_hand_t nabla_data;
  
  // file data structure
  file_data_hand_t file_data;
   
  // MPI variables
  int nproc, rank;

  // Begin Parrellizing
  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &nproc);


  /*             * 
   * START CLOCK *
   *             */
  tot_start_time = MPI_Wtime();

  
  // Time
  io_start_time = MPI_Wtime();
  file_hand_array = init_file_hand_array(input_file,&file_count, rank, nproc, line_count);
  io_end_time = MPI_Wtime();
  io_time = io_end_time - io_start_time;

  if(rank == 0) {
    // compute leaders per level
    nproc_array = compute_nproc_array(process_file,&total_level);
    leader_level_unroll = compute_process_leaders(nproc_array, nproc, total_level);
  }

  // Time
  // send total_level and nproc_array to other proc
  comm_start_time = MPI_Wtime();
  MPI_Bcast(&total_level, 1, MPI_INT, 0, MPI_COMM_WORLD);
  comm_end_time = MPI_Wtime();
  comm_time += (comm_end_time - comm_start_time);

  if(rank != 0) {
    nproc_array = (int *) malloc(sizeof(int) * total_level);
  }

  // Time
  comm_start_time = MPI_Wtime();
  MPI_Bcast(nproc_array, total_level, MPI_INT, 0, MPI_COMM_WORLD);
  comm_end_time = MPI_Wtime();
  comm_time += (comm_end_time - comm_start_time);

  int leader_level_size = sum(nproc_array,total_level) + total_level;
  if(rank != 0) {      
    leader_level_unroll = (int *) malloc(sizeof(int *) * (leader_level_size));
  }

  // send leader_level_unroll
  // Time
  comm_start_time = MPI_Wtime();
  MPI_Bcast(leader_level_unroll, leader_level_size, MPI_INT, 0, MPI_COMM_WORLD);
  comm_end_time = MPI_Wtime();
  comm_time += (comm_end_time - comm_start_time);

  // roll up leader_level_unroll
  leader_level = roll_array(leader_level_unroll, nproc_array, total_level);

  // stack tuple init
  bio_data = malloc(sizeof(stack_tuple_t));
  bio_data->buf_x = malloc(sizeof(char) * MEGABYTE);
  bio_data->buf_y = malloc(sizeof(char) * MEGABYTE);
  bio_data->buf_z = malloc(sizeof(char) * MEGABYTE);
  bio_data->buf_e = malloc(sizeof(char) * MEGABYTE);
  assert(bio_data && bio_data->buf_x && bio_data->buf_y && bio_data->buf_z && bio_data->buf_e);
  bio_data->top_x =  bio_data->top_y =  bio_data->top_z =  bio_data->top_e = 0;
  *bio_data_hand = bio_data;


  // mpi data init
  mpi_data = malloc(sizeof(mpi_data_t));
  mpi_data->global_rank = rank;
  mpi_data->global_nproc = nproc;
  mpi_data->split_count = 0;
  *mpi_data_hand = mpi_data;
 
  nabla_data = malloc(sizeof(nabla_data_t));
  nabla_data->nproc_array = nproc_array;
  nabla_data->leader_level = leader_level;
  nabla_data->total_level = total_level;
  *nabla_data_hand = nabla_data;


  file_data = malloc(sizeof(file_data_t));
  file_data->file_hand_array = file_hand_array;
  file_data->file_count = file_count;
  *file_data_hand = file_data;


  int global_total_files;
  //printf("Rank: %i .. file_count: %i\n", mpi_data->global_rank, file_data->file_count);
  MPI_Reduce(&file_data->file_count, &global_total_files, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);
  if(rank == 0) {
    printf("global_total_files: %i\n", global_total_files);
  }
}
// get data from previous level
void get_data(stack_tuple_hand_t bio_data, mpi_data_hand_t mpi_data, int i) {
  if(!(mpi_data->local_nproc > 1)) {
    return;
  }
  
  char* buf_x = bio_data->buf_x, *buf_y = bio_data->buf_y, *buf_z = bio_data->buf_z, *buf_e =  bio_data->buf_e;
  int max, top_x = bio_data->top_x, top_y = bio_data->top_y, top_z = bio_data->top_z, top_e = bio_data->top_e;
  int curr_rank = mpi_data->local_rank;
  int curr_comm_size = mpi_data->local_nproc;
  MPI_Comm curr_comm = mpi_data->curr_comm;
  int rank = mpi_data->global_rank;
  char* rec_buf;
  
  // Get X values

  int *rec_bytes = malloc(sizeof(int) * curr_comm_size);
  MPI_Gather(&top_x, 1, MPI_INT,rec_bytes, 1, MPI_INT, curr_rank, curr_comm);
  int z;
  //if(mpi_data->global_rank == 0) {
  //    for(z = 0; z < curr_comm_size; z++)
  //        printf("%d\n",rec_bytes[z]);
  //    printf("\n");
  //}

  int* displs = (int *) malloc(sizeof(int) * curr_comm_size);
  displs[0] = 0;
  int sum_bytes = rec_buf[0] + 1;
  for(z = 1; z < curr_comm_size; z++) {
    sum_bytes += rec_bytes[z] + 1;
    displs[z] = displs[z-1] + rec_buf[z - 1] + 1;
  }
  char* gather_x = (char *) malloc(sizeof(char) * sum_bytes);
  for(z = 1; z < curr_comm_size; z++) {
    gather_x[displs[z] - 1] = '\0';
  }
  if(mpi_data->global_rank == 0) {
    assert(rec_bytes && displs && buf_x);
    MPI_Gatherv(buf_x, top_x, MPI_CHAR, gather_x, rec_bytes, displs, MPI_CHAR, curr_rank, curr_comm);
  }
  
  comm_start_time = MPI_Wtime();
  MPI_Reduce(&top_x, &max, 1, MPI_INT, MPI_MAX, curr_rank, curr_comm);
  MPI_Bcast(&max, 1, MPI_INT, curr_rank, curr_comm);
  comm_end_time = MPI_Wtime();
  comm_time += (comm_end_time - comm_start_time);

  rec_buf = (char *) malloc(sizeof(char) * (curr_comm_size * max) + 1);
  assert(rec_buf);
  char* temp_buf = (char *) malloc(sizeof(char) * max + 1);
  memset(temp_buf, ' ', max);
  memcpy(temp_buf, buf_x, top_x);
  free(buf_x);
  buf_x = temp_buf; 

  // Time
  comm_start_time = MPI_Wtime();
  MPI_Gather(buf_x, max, MPI_CHAR, rec_buf, max, MPI_CHAR, curr_rank,curr_comm);
  comm_end_time = MPI_Wtime();
  comm_time += (comm_end_time - comm_start_time);        

  free(buf_x);
  buf_x = rec_buf;
  top_x = max * curr_comm_size;

  // Time
  // Get Y values
  comm_start_time = MPI_Wtime();
  MPI_Reduce(&top_y, &max, 1, MPI_INT, MPI_MAX, curr_rank, curr_comm);
  MPI_Bcast(&max, 1, MPI_INT, curr_rank, curr_comm);
  comm_end_time = MPI_Wtime();
  comm_time += (comm_end_time - comm_start_time);

  rec_buf = (char *) malloc(sizeof(char) * (curr_comm_size * max) + 1);
  assert(rec_buf);
  temp_buf = (char *) malloc(sizeof(char) * max + 1);
  memset(temp_buf, ' ', max);
  memcpy(temp_buf, buf_y, top_y);
  free(buf_y);
  buf_y = temp_buf; 

  // Time
  comm_start_time = MPI_Wtime();
  MPI_Gather(buf_y, max, MPI_CHAR, rec_buf, max, MPI_CHAR, curr_rank,curr_comm); 
  comm_end_time = MPI_Wtime();
  comm_time += (comm_end_time - comm_start_time);

  free(buf_y);
  buf_y = rec_buf;
  top_y = max * curr_comm_size;

  // Time
  // Get Z values
  comm_start_time;
  MPI_Reduce(&top_z, &max, 1, MPI_INT, MPI_MAX, curr_rank, curr_comm);
  MPI_Bcast(&max, 1, MPI_INT, curr_rank, curr_comm);
  comm_end_time = MPI_Wtime();
  comm_time += (comm_end_time - comm_start_time);

  rec_buf = (char *) malloc(sizeof(char) * (curr_comm_size * max) + 1);
  assert(rec_buf);
  temp_buf = (char *) malloc(sizeof(char) * max + 1);
  memset(temp_buf, ' ', max);
  memcpy(temp_buf, buf_z, top_z);
  free(buf_z);
  buf_z = temp_buf; 

  //Time
  comm_start_time = MPI_Wtime();
  MPI_Gather(buf_z, max, MPI_CHAR, rec_buf, max, MPI_CHAR, curr_rank,curr_comm);
  free(buf_z);
  comm_end_time = MPI_Wtime();
  comm_time += (comm_end_time - comm_start_time);

  buf_z = rec_buf;
  top_z = max * curr_comm_size;
      
  // Get Everything Else
  // Time
  comm_start_time = MPI_Wtime();
  MPI_Reduce(&top_e, &max, 1, MPI_INT, MPI_MAX, curr_rank, curr_comm);
  MPI_Bcast(&max, 1, MPI_INT, curr_rank, curr_comm);
  comm_end_time = MPI_Wtime();
  comm_time += (comm_end_time - comm_start_time);

  rec_buf = (char *) malloc(sizeof(char) * (curr_comm_size * max) + 1);
  assert(rec_buf);
  temp_buf = (char *) malloc(sizeof(char) * max + 1);
  memset(temp_buf, ' ', max);
  memcpy(temp_buf, buf_e, top_e);
  free(buf_e);
  buf_e = temp_buf;

  // Time
  comm_start_time = MPI_Wtime();
  MPI_Gather(buf_e, max, MPI_CHAR, rec_buf, max, MPI_CHAR, curr_rank,curr_comm);
  comm_end_time = MPI_Wtime();
  comm_time += (comm_end_time - comm_start_time);

  free(buf_e);
  buf_e = rec_buf;
  top_e = max * curr_comm_size;

  // Time
  comm_start_time = MPI_Wtime();
  MPI_Comm_free(&curr_comm);
  if(DEBUG_MODE) {
    //printf("Level %i: Rank %i received %i bytes from all ranks in group\n",i,rank,top_x+top_y+top_z+top_e);
  }
  comm_end_time = MPI_Wtime();
  comm_time += (comm_end_time - comm_start_time);
  
  bio_data->buf_x = buf_x;
  bio_data->buf_y = buf_y;
  bio_data->buf_z = buf_z;
  bio_data->buf_e = buf_e;
  bio_data->top_x = top_x;
  bio_data->top_y = top_y;
  bio_data->top_z = top_z;
  bio_data->top_e = top_e;
}

void send_data(nabla_data_hand_t nabla_data, stack_tuple_hand_t bio_data, mpi_data_hand_t mpi_data, int i) { 
  mpi_data->split_count++;

  mpi_data->leader_rank = next_leader_rank(nabla_data->leader_level[i+1], mpi_data->global_rank, nabla_data->nproc_array[i+1]);
  mpi_data->local_color = mpi_data->leader_rank;

  char* buf_x = bio_data->buf_x, *buf_y = bio_data->buf_y, *buf_z = bio_data->buf_z, *buf_e =  bio_data->buf_e;
  int max, top_x = bio_data->top_x, top_y = bio_data->top_y, top_z = bio_data->top_z, top_e = bio_data->top_e;
  //if(mpi_data->leader_rank == 0) {
  //  printf("%d: leader_rank is 0; and sending top_x = %d\n", mpi_data->global_rank, bio_data->top_x);
  //}
  
  int curr_rank = mpi_data->local_rank;
  int curr_comm_size = mpi_data->local_nproc;
  MPI_Comm curr_comm = mpi_data->curr_comm;
  int rank = mpi_data->global_rank;
  int leader_rank = mpi_data->leader_rank;
  int color = mpi_data->local_color;
  comm_start_time = MPI_Wtime();
  MPI_Comm_split(MPI_COMM_WORLD, color, rank, &curr_comm);
  comm_end_time = MPI_Wtime();
  comm_time += (comm_end_time - comm_start_time);
  

  // Testing
  int temp_lead = leader_rank;
  if(DEBUG_MODE) {
    //printf("Level %i: Rank %i will send data to Rank %i\n", i, rank, temp_lead);
  }
  leader_rank = 0;

  
  // X
  // Time
 
  int *rec_bytes;
  MPI_Gather(&top_x, 1, MPI_INT, rec_bytes, 1, MPI_INT, leader_rank, curr_comm);

  int* displs;
  char* gather_x;
  if(temp_lead == 0) {
    assert(rec_bytes && displs && buf_x);
    MPI_Gatherv(buf_x, top_x, MPI_CHAR, gather_x, rec_bytes, displs, MPI_CHAR, leader_rank, curr_comm);
  }
  
  comm_start_time = MPI_Wtime();
  MPI_Reduce(&top_x, &max, 1, MPI_INT, MPI_MAX, leader_rank, curr_comm); 
  MPI_Bcast(&max, 1, MPI_INT, leader_rank, curr_comm);
  comm_end_time = MPI_Wtime();
  comm_time += (comm_end_time - comm_start_time);

  char* temp_buf = (char *) malloc(sizeof(char) * max + 1);
  memset(temp_buf, ' ', max);
  memcpy(temp_buf, buf_x, top_x);
  free(buf_x);
  buf_x = temp_buf;
  
  // Time
  comm_start_time = MPI_Wtime(); 
  MPI_Gather(buf_x, max, MPI_CHAR, temp_buf, max, MPI_CHAR, leader_rank,curr_comm);
  comm_end_time = MPI_Wtime();
  comm_time += (comm_end_time - comm_start_time);
  top_x = max;

  // Y
  // Time
  comm_start_time = MPI_Wtime();
  MPI_Reduce(&top_y, &max, 1, MPI_INT, MPI_MAX, leader_rank, curr_comm);
  MPI_Bcast(&max, 1, MPI_INT, leader_rank, curr_comm);
  comm_end_time = MPI_Wtime();
  comm_time += (comm_end_time - comm_start_time);
  
  temp_buf = (char *) malloc(sizeof(char) * max + 1);
  memset(temp_buf, ' ', max);
  memcpy(temp_buf, buf_y, top_y);
  free(buf_y);
  buf_y = temp_buf;

  // Time
  comm_start_time = MPI_Wtime();
  MPI_Gather(buf_y, max, MPI_CHAR, temp_buf, max, MPI_CHAR, leader_rank,curr_comm);
  comm_end_time = MPI_Wtime();
  comm_time += (comm_end_time - comm_start_time);
  top_y = max;

  // Z
  // Time
  comm_start_time = MPI_Wtime();
  MPI_Reduce(&top_z, &max, 1, MPI_INT, MPI_MAX, leader_rank, curr_comm);
  MPI_Bcast(&max, 1, MPI_INT, leader_rank, curr_comm);
  comm_end_time = MPI_Wtime();
  comm_time += (comm_end_time - comm_start_time);
  
  temp_buf = (char *) malloc(sizeof(char) * max + 1);
  memset(temp_buf, ' ', max);
  memcpy(temp_buf, buf_z, top_z);
  free(buf_z);
  buf_z = temp_buf; 

  // Time
  comm_start_time = MPI_Wtime();
  MPI_Gather(buf_z, max, MPI_CHAR, temp_buf, max, MPI_CHAR, leader_rank,curr_comm);
  comm_end_time = MPI_Wtime();
  comm_time += (comm_end_time - comm_start_time);
  top_z = max;

  // E
  // Time
  comm_start_time = MPI_Wtime();
  MPI_Reduce(&top_e, &max, 1, MPI_INT, MPI_MAX, leader_rank, curr_comm);
  MPI_Bcast(&max, 1, MPI_INT, leader_rank, curr_comm);
  comm_end_time = MPI_Wtime();
  comm_time += (comm_end_time - comm_start_time);
  
  temp_buf = (char *) malloc(sizeof(char) * max + 1);
  memset(temp_buf, ' ', max);
  memcpy(temp_buf, buf_e, top_e);
  free(buf_e);
  buf_e = temp_buf; 

  // Time
  comm_start_time = MPI_Wtime();
  MPI_Gather(buf_e, max, MPI_CHAR, temp_buf, max, MPI_CHAR, leader_rank,curr_comm);
  comm_end_time = MPI_Wtime();
  comm_time += (comm_end_time - comm_start_time);
     
  top_e = max;

  if(DEBUG_MODE) {
    //printf("Level %i: Rank %i sent %i bytes to Rank %i\n", i, rank, top_x + top_y + top_z + top_e, temp_lead);
  }

  comm_start_time = MPI_Wtime();
  MPI_Comm_free(&curr_comm);
  comm_end_time = MPI_Wtime();
  comm_time += (comm_end_time - comm_start_time);
  
  bio_data->buf_x = buf_x;
  bio_data->buf_y = buf_y;
  bio_data->buf_z = buf_z;
  bio_data->buf_e = buf_e;
  bio_data->top_x = top_x;
  bio_data->top_y = top_y;
  bio_data->top_z = top_z;
  bio_data->top_e = top_e;
}

void split_comm(mpi_data_hand_t mpi_data) {
  mpi_data->split_count++;
  mpi_data->local_color = mpi_data->global_rank;
  int rank = mpi_data->global_rank;
  int color = mpi_data->local_color;

  // Time
  comm_start_time = MPI_Wtime();
  MPI_Comm_split(MPI_COMM_WORLD, color, rank, &mpi_data->curr_comm);
  MPI_Comm_size(mpi_data->curr_comm, &mpi_data->local_nproc);
  MPI_Comm_rank(mpi_data->curr_comm, &mpi_data->local_rank);
  comm_end_time = MPI_Wtime();
  comm_time += (comm_end_time - comm_start_time);
}

void get_total_bytes(stack_tuple_hand_t bio_data) {
  
  if(DEBUG_MODE) {
    total_bytes = sum_bytes(bio_data->buf_x,bio_data->top_x) + sum_bytes(bio_data->buf_y, bio_data->top_y) + sum_bytes(bio_data->buf_z, bio_data->top_z) + sum_bytes(bio_data->buf_e, bio_data->top_e);
  }
}

void wait_proc(mpi_data_hand_t mpi_data, nabla_data_hand_t nabla_data) {
  MPI_Comm comm;
  int split_count = mpi_data->split_count;
  int nproc = mpi_data->global_nproc;
  int rank = mpi_data->global_rank;
  int total_level = nabla_data->total_level;
  // other processes do extra splits for process still in loop
  for(; split_count < total_level - 1; split_count++) {
    MPI_Comm_split(MPI_COMM_WORLD, nproc * total_level + 1, rank, &comm);
    MPI_Comm_free(&comm);
  }
}

void data_display(mpi_data_hand_t mpi_data, stack_tuple_hand_t bio_data) {
  char* buf_x = bio_data->buf_x, *buf_y = bio_data->buf_y, *buf_z = bio_data->buf_z, *buf_e =  bio_data->buf_e;
  int top_x = bio_data->top_x, top_y = bio_data->top_y, top_z = bio_data->top_z, top_e = bio_data->top_e;
  int rank = mpi_data->global_rank;
  if(DEBUG_MODE) {
    // Time
    comm_start_time = MPI_Wtime();
    MPI_Reduce(&total_bytes, &global_total_bytes, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);
    comm_end_time = MPI_Wtime();
    comm_time += (comm_end_time - comm_start_time);
    
    if(rank == 0) {
      printf("sum_bytes %i expected_sum_bytes %i\n", sum_bytes(buf_x,top_x) + sum_bytes(buf_y, top_y) + sum_bytes(buf_z, top_z) + sum_bytes(buf_e, top_e),global_total_bytes);
    }
  }
}

void clean_up(stack_tuple_hand_t bio_data, mpi_data_hand_t mpi_data) {
  free(bio_data->buf_x);
  free(bio_data->buf_y);
  free(bio_data->buf_z);
  free(bio_data->buf_e);
  free(bio_data);

  free(mpi_data);
}

void end_clock() {
  tot_end_time = MPI_Wtime();
  tot_time = tot_end_time - tot_start_time;
  comp_time = tot_time - io_time - comm_time;
}

void time_display(int rank) {
  double global_comp_time = 0.0;
  double global_comm_time = 0.0;
  double global_io_time = 0.0;
  double global_tot_time = 0.0;

  MPI_Reduce(&comp_time, &global_comp_time, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);  
  MPI_Reduce(&comm_time, &global_comm_time, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);  
  MPI_Reduce(&io_time, &global_io_time, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);  
  MPI_Reduce(&tot_time, &global_tot_time, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);  


  if(rank == 0) {
    printf("Comp: %.2f Comm: %.2f IO: %.2f\n",global_comp_time / global_tot_time, global_comm_time / global_tot_time, global_io_time / global_tot_time);
  }
}

// makes 1D array to 2D jagged array
int** roll_array(int* level_array_unroll, int* nproc_array, int total_level) {
  int** level_array = (int **) malloc(sizeof(int*) * total_level);
  int i, disp = 0;
  for(i = 0; i < total_level; i++) {
    level_array[i] = (int *) malloc(sizeof(int) * nproc_array[i]);
    int j;
    for(j = 0; j < nproc_array[i]; j++) {
      level_array[i][j] = level_array_unroll[disp + j];
    }
    disp += nproc_array[i] + 1;
  }
  return level_array;
}

int is_leader(nabla_data_hand_t nabla_data, mpi_data_hand_t mpi_data, int i) {
  return (i != nabla_data->total_level - 1 && next_level_leader(nabla_data, mpi_data, i) == -1);
  }

// true iff $(rank) is leader in $(level_array)
int next_level_leader(nabla_data_hand_t nabla_data, mpi_data_hand_t mpi_data, int level) {
  int* level_array = nabla_data->leader_level[level + 1];
  int size = nabla_data->nproc_array[level + 1];
  int rank = mpi_data->global_rank;
  int i;
  for(i = 0; i < size; i++) {
    if(rank == level_array[i]) {
      return 1;
    }
  }
  return -1;
}

// return rank of next leader relative to $(rank)
int next_leader_rank(int* level_array, int rank, int size) {
  int i;
  for(i = 0; i < size; i++) {
    if(rank < level_array[i]) {
      return level_array[i - 1];
    }
  }
  return level_array[size - 1];
}

// get index of element in array
int get_index(int* array, int rank, int size) {
  int i;
  for(i = 0; i < size; i++) {
    if(rank == array[i]) {
      return i;
    }
  }
  return 0;
}

// sum up the bytes
int sum_bytes(char* x, int s) {
  int i, sum = 0;
  for(i = 0; i < s; i++) {
    char z = x[i];
    if(isspace(z) == 0) {
      sum+=z;
    }
  }
  return sum;
}

// sum up int(s)
int sum_ints(int* x, int s) {
  int i, sum = 0;
  for(i = 0; i < s; i++) {
    sum += x[i];
  }
  return sum;
}

void data_aware_comp(stack_tuple_hand_t bio_data, mpi_data_hand_t mpi_data) {
  
}
void finalize(mpi_data_hand_t mpi_data, nabla_data_hand_t nabla_data, stack_tuple_hand_t bio_data) {
  
  wait_proc(mpi_data, nabla_data);
  data_display(mpi_data, bio_data);
  clean_up(bio_data, mpi_data);
  end_clock();
  time_display(mpi_data->global_rank);
  MPI_Finalize();
  
}
