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
#define SOURCE_PROC 0
#define MAX_ARGV 4
#define MEGABYTE 100000000
#define DEBUG_MODE 1

int next_level_leader(int* leader_level, int rank, int size);
int next_leader_rank(int* level_array, int rank, int size);
int get_index(int* level_array, int rank, int size);
int** roll_array(int* level_array_unroll, int* nproc_array, int total_level);
int sum_bytes(char* x, int s);
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
int main(int argc, char* argv[]) {

  if(argc != MAX_ARGV) {
    printf("Error: No input file or  is given\n");
    exit(1);
  }

  // TODO implement flags for this part
  char* input_file = argv[1];
  char* process_file = argv[2];
  int line_count = atoi(argv[3]);

  // current level of process in hierarchy, and total levels
  int level, total_level;

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
  MPI_Status status;
  MPI_Request request;
  int nproc, rank;

  // Begin Parrellizing
  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &nproc);


  /*             * 
   * START CLOCK *
   *             */
  tot_start_time = MPI_Wtime();

  // get array of file_hand_t
  int file_count;
  
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

  if(DEBUG_MODE && rank == 0) {

  printf("//////////  //////////  //////////\n//      //  //      //  //        \n//     //   //     //   //        \n\n");
  int z;
  for(z = 0; z < total_level; z++) {
      int x = 0;
      if(z > 0)
        x = nproc_array[0] - nproc_array[z];
      int t;
      for(t = 0; t < x / 2; t++) {
        printf("   ");
      }
    
      int y;
      for(y = 0; y < nproc_array[z]; y++) {
        printf("[%i]",leader_level[z][y]);
      }
      printf("\n");
    }
  }

  char* x_coordinates = malloc(sizeof(char) * MEGABYTE);
  char* y_coordinates = malloc(sizeof(char) * MEGABYTE);
  char* z_coordinates = malloc(sizeof(char) * MEGABYTE);
  char* everything_else = malloc(sizeof(char) * MEGABYTE);
  assert(x_coordinates && y_coordinates && z_coordinates && everything_else);


  
  int total_bytes = 0;
  int global_total_bytes = 0;
  int x_start = 0, y_start = 0, z_start = 0, ev_start = 0;



  // do hierarchy here
  int num;
  int gather_count = 0;
  MPI_Comm curr_comm = MPI_COMM_WORLD;
  MPI_Comm new_comm;
  int curr_comm_size;
  int curr_rank;
  int* n_elements;
  int* disp;
  char* rec_buf;
  int i, nproc_prev, nproc_curr, nproc_ratio, nproc_rem, total, max;
  for(i = 0; i < total_level && file_count > 0; i++) {
    
    // if not level 0, get files from previous level
    if(i > 0) {
      
      gather_count++;
      int color = rank;

      // Time
      comm_start_time = MPI_Wtime();
      MPI_Comm_split(MPI_COMM_WORLD, color, rank, &new_comm);
      curr_comm = new_comm;
      MPI_Comm_size(curr_comm, &curr_comm_size);
      MPI_Comm_rank(curr_comm, &curr_rank);
      comm_end_time = MPI_Wtime();
      comm_time += (comm_end_time - comm_start_time);

      // current communicator size
      if(curr_comm_size > 1) {
      
        // Get X values
        comm_start_time = MPI_Wtime();
        MPI_Reduce(&x_start, &max, 1, MPI_INT, MPI_MAX, curr_rank, curr_comm);
        MPI_Bcast(&max, 1, MPI_INT, curr_rank, curr_comm);
        comm_end_time = MPI_Wtime();
        comm_time += (comm_end_time - comm_start_time);

        rec_buf = (char *) malloc(sizeof(char) * (curr_comm_size * max) + 1);
        assert(rec_buf);
        char* temp_buf = (char *) malloc(sizeof(char) * max + 1);
        memset(temp_buf, ' ', max);
        memcpy(temp_buf, x_coordinates, x_start);
        free(x_coordinates);
        x_coordinates = temp_buf; 

        // Time
        comm_start_time = MPI_Wtime();
        MPI_Gather(x_coordinates, max, MPI_CHAR, rec_buf, max, MPI_CHAR, curr_rank,curr_comm);
        comm_end_time = MPI_Wtime();
        comm_time += (comm_end_time - comm_start_time);        

        free(x_coordinates);
        x_coordinates = rec_buf;
        x_start = max * curr_comm_size;

        // Time
        // Get Y values
        comm_start_time = MPI_Wtime();
        MPI_Reduce(&y_start, &max, 1, MPI_INT, MPI_MAX, curr_rank, curr_comm);
        MPI_Bcast(&max, 1, MPI_INT, curr_rank, curr_comm);
        comm_end_time = MPI_Wtime();
        comm_time += (comm_end_time - comm_start_time);

        rec_buf = (char *) malloc(sizeof(char) * (curr_comm_size * max) + 1);
        assert(rec_buf);
        temp_buf = (char *) malloc(sizeof(char) * max + 1);
        memset(temp_buf, ' ', max);
        memcpy(temp_buf, y_coordinates, y_start);
        free(y_coordinates);
        y_coordinates = temp_buf; 

        // Time
        comm_start_time = MPI_Wtime();
        MPI_Gather(y_coordinates, max, MPI_CHAR, rec_buf, max, MPI_CHAR, curr_rank,curr_comm); 
        comm_end_time = MPI_Wtime();
        comm_time += (comm_end_time - comm_start_time);

        free(y_coordinates);
        y_coordinates = rec_buf;
        y_start = max * curr_comm_size;
  
        // Time
        // Get Z values
        comm_start_time;
        MPI_Reduce(&z_start, &max, 1, MPI_INT, MPI_MAX, curr_rank, curr_comm);
        MPI_Bcast(&max, 1, MPI_INT, curr_rank, curr_comm);
        comm_end_time = MPI_Wtime();
        comm_time += (comm_end_time - comm_start_time);

        rec_buf = (char *) malloc(sizeof(char) * (curr_comm_size * max) + 1);
        assert(rec_buf);
        temp_buf = (char *) malloc(sizeof(char) * max + 1);
        memset(temp_buf, ' ', max);
        memcpy(temp_buf, z_coordinates, z_start);
        free(z_coordinates);
        z_coordinates = temp_buf; 
  
        //Time
        comm_start_time = MPI_Wtime();
        MPI_Gather(z_coordinates, max, MPI_CHAR, rec_buf, max, MPI_CHAR, curr_rank,curr_comm);
        free(z_coordinates);
        comm_end_time = MPI_Wtime();
        comm_time += (comm_end_time - comm_start_time);
        
        z_coordinates = rec_buf;
        z_start = max * curr_comm_size;
            
        // Get Everything Else
        // Time
        comm_start_time = MPI_Wtime();
        MPI_Reduce(&ev_start, &max, 1, MPI_INT, MPI_MAX, curr_rank, curr_comm);
        MPI_Bcast(&max, 1, MPI_INT, curr_rank, curr_comm);
        comm_end_time = MPI_Wtime();
        comm_time += (comm_end_time - comm_start_time);
        
        rec_buf = (char *) malloc(sizeof(char) * (curr_comm_size * max) + 1);
        assert(rec_buf);
        temp_buf = (char *) malloc(sizeof(char) * max + 1);
        memset(temp_buf, ' ', max);
        memcpy(temp_buf, everything_else, ev_start);
        free(everything_else);
        everything_else = temp_buf;
  
        // Time
        comm_start_time = MPI_Wtime();
        MPI_Gather(everything_else, max, MPI_CHAR, rec_buf, max, MPI_CHAR, curr_rank,curr_comm);
        comm_end_time = MPI_Wtime();
        comm_time += (comm_end_time - comm_start_time);

        free(everything_else);
        everything_else = rec_buf;
        ev_start = max * curr_comm_size;

        // Time
        comm_start_time = MPI_Wtime();
        MPI_Comm_free(&curr_comm);
        printf("Level %i: Rank %i received %i bytes from all ranks in group\n",i,rank,x_start+y_start+z_start+ev_start);
        comm_end_time = MPI_Wtime();
        comm_time += (comm_end_time - comm_start_time);
      }
      
      // if no longer leader for next level
      if(i != total_level - 1 && next_level_leader(leader_level[i+1], rank, nproc_array[i+1]) == -1) {
        gather_count++;
        int leader_rank = next_leader_rank(leader_level[i+1],rank, nproc_array[i+1]);
        int color = leader_rank;

        comm_start_time = MPI_Wtime();
        MPI_Comm_split(MPI_COMM_WORLD, color, rank, &new_comm);
        comm_end_time = MPI_Wtime();
        comm_time += (comm_end_time - comm_start_time);
        
        curr_comm = new_comm;
      
        // Testing
        int temp_lead = leader_rank;
        if(DEBUG_MODE) {
          printf("Level %i: Rank %i will send data to Rank %i\n", i, rank, temp_lead);
        }
        leader_rank = 0;

        
        // X
        // Time
        comm_start_time = MPI_Wtime();
        MPI_Reduce(&x_start, &max, 1, MPI_INT, MPI_MAX, leader_rank, curr_comm); 
        MPI_Bcast(&max, 1, MPI_INT, leader_rank, curr_comm);
        comm_end_time = MPI_Wtime();
        comm_time += (comm_end_time - comm_start_time);
    
        char* temp_buf = (char *) malloc(sizeof(char) * max + 1);
        memset(temp_buf, ' ', max);
        memcpy(temp_buf, x_coordinates, x_start);
        free(x_coordinates);
        x_coordinates = temp_buf;
        
        // Time
        comm_start_time = MPI_Wtime(); 
        MPI_Gather(x_coordinates, max, MPI_CHAR, temp_buf, max, MPI_CHAR, leader_rank,curr_comm);
        comm_end_time = MPI_Wtime();
        comm_time += (comm_end_time - comm_start_time);
        x_start = max;
     
        // Y
        // Time
        comm_start_time = MPI_Wtime();
        MPI_Reduce(&y_start, &max, 1, MPI_INT, MPI_MAX, leader_rank, curr_comm);
        MPI_Bcast(&max, 1, MPI_INT, leader_rank, curr_comm);
        comm_end_time = MPI_Wtime();
        comm_time += (comm_end_time - comm_start_time);
        
        temp_buf = (char *) malloc(sizeof(char) * max + 1);
        memset(temp_buf, ' ', max);
        memcpy(temp_buf, y_coordinates, y_start);
        free(y_coordinates);
        y_coordinates = temp_buf;
   
        // Time
        comm_start_time = MPI_Wtime();
        MPI_Gather(y_coordinates, max, MPI_CHAR, temp_buf, max, MPI_CHAR, leader_rank,curr_comm);
        comm_end_time = MPI_Wtime();
        comm_time += (comm_end_time - comm_start_time);
        y_start = max;

        // Z
        // Time
        comm_start_time = MPI_Wtime();
        MPI_Reduce(&z_start, &max, 1, MPI_INT, MPI_MAX, leader_rank, curr_comm);
        MPI_Bcast(&max, 1, MPI_INT, leader_rank, curr_comm);
        comm_end_time = MPI_Wtime();
        comm_time += (comm_end_time - comm_start_time);
        
        temp_buf = (char *) malloc(sizeof(char) * max + 1);
        memset(temp_buf, ' ', max);
        memcpy(temp_buf, z_coordinates, z_start);
        free(z_coordinates);
        z_coordinates = temp_buf; 
  
        // Time
        comm_start_time = MPI_Wtime();
        MPI_Gather(z_coordinates, max, MPI_CHAR, temp_buf, max, MPI_CHAR, leader_rank,curr_comm);
        comm_end_time = MPI_Wtime();
        comm_time += (comm_end_time - comm_start_time);
        z_start = max;

        // E
        // Time
        comm_start_time = MPI_Wtime();
        MPI_Reduce(&ev_start, &max, 1, MPI_INT, MPI_MAX, leader_rank, curr_comm);
        MPI_Bcast(&max, 1, MPI_INT, leader_rank, curr_comm);
        comm_end_time = MPI_Wtime();
        comm_time += (comm_end_time - comm_start_time);
        
        temp_buf = (char *) malloc(sizeof(char) * max + 1);
        memset(temp_buf, ' ', max);
        memcpy(temp_buf, everything_else, ev_start);
        free(everything_else);
        everything_else = temp_buf; 

        // Time
        comm_start_time = MPI_Wtime();
        MPI_Gather(everything_else, max, MPI_CHAR, temp_buf, max, MPI_CHAR, leader_rank,curr_comm);
        comm_end_time = MPI_Wtime();
        comm_time += (comm_end_time - comm_start_time);
           
        ev_start = max;

        if(DEBUG_MODE) {
          printf("Level %i: Rank %i sent %i bytes to Rank %i\n", i, rank, x_start + y_start + z_start + ev_start, temp_lead);
        }

        comm_start_time = MPI_Wtime();
        MPI_Comm_free(&curr_comm);
        comm_end_time = MPI_Wtime();
        comm_time += (comm_end_time - comm_start_time);
        break;
      }

    } else {


      // do compression/aggregation
      aggregate_files(x_coordinates, y_coordinates, z_coordinates, everything_else, file_hand_array, file_count, &x_start, &y_start, &z_start, &ev_start);
      total_bytes = sum_bytes(x_coordinates,x_start) + sum_bytes(y_coordinates, y_start) + sum_bytes(z_coordinates, z_start) + sum_bytes(everything_else, ev_start);
      if(DEBUG_MODE) {
        printf("Rank %i: x_start: %i y_start: %i z_start: %i ev_start: %i total: %i\nRank %i:  x_byte: %i  y_byte: %i  z_byte: %i  ev_byte: %i total: %i\n", rank, x_start, y_start, z_start, ev_start, x_start + y_start + z_start + ev_start,rank, sum_bytes(x_coordinates,x_start), sum_bytes(y_coordinates,y_start), sum_bytes(z_coordinates,z_start), sum_bytes(everything_else,ev_start),total_bytes);
      }


      // if not last level and this process is not leader for next level
      if(i != total_level - 1 && next_level_leader(leader_level[i+1], rank, nproc_array[i+1]) == -1) {
        gather_count++;
        int leader_rank = next_leader_rank(leader_level[i+1],rank, nproc_array[i+1]);
        int color = leader_rank;

        // Time
        comm_start_time = MPI_Wtime();
        MPI_Comm_split(MPI_COMM_WORLD, color, rank, &new_comm);
        comm_end_time = MPI_Wtime();
        comm_time += (comm_end_time - comm_start_time);
        
        curr_comm = new_comm;
      
        // Testing
        int temp_lead = leader_rank;
        if(DEBUG_MODE) {
          printf("Level %i: Rank %i will send data to Rank %i\n", i, rank, temp_lead);
        }
        leader_rank = 0;

        // X
        // Time
        comm_start_time = MPI_Wtime();
        MPI_Reduce(&x_start, &max, 1, MPI_INT, MPI_MAX, leader_rank, curr_comm); 
        MPI_Bcast(&max, 1, MPI_INT, leader_rank, curr_comm);
        comm_end_time = MPI_Wtime();
        comm_time += (comm_end_time - comm_start_time);
        
        char* temp_buf = (char *) malloc(sizeof(char) * max + 1);
        memset(temp_buf, ' ', max);
        memcpy(temp_buf, x_coordinates, x_start);
        free(x_coordinates);
        x_coordinates = temp_buf; 

        comm_start_time = MPI_Wtime();
        MPI_Gather(x_coordinates, max, MPI_CHAR, temp_buf, max, MPI_CHAR, leader_rank,curr_comm);
        comm_end_time = MPI_Wtime();
        comm_time += (comm_end_time - comm_start_time);
        
        x_start = max;
     
        // Y
        // Time
        comm_start_time = MPI_Wtime();
        MPI_Reduce(&y_start, &max, 1, MPI_INT, MPI_MAX, leader_rank, curr_comm);
        MPI_Bcast(&max, 1, MPI_INT, leader_rank, curr_comm);
        comm_end_time = MPI_Wtime();
        comm_time += (comm_end_time - comm_start_time);
        
        temp_buf = (char *) malloc(sizeof(char) * max + 1);
        memset(temp_buf, ' ', max);
        memcpy(temp_buf, y_coordinates, y_start);
        free(y_coordinates);
        y_coordinates = temp_buf;
        
        // Time
        comm_start_time = MPI_Wtime(); 
        MPI_Gather(y_coordinates, max, MPI_CHAR, temp_buf, max, MPI_CHAR, leader_rank,curr_comm);
        comm_end_time = MPI_Wtime();
        comm_time += (comm_end_time - comm_start_time);
        
        y_start = max;

        // Z
        // Time
        comm_start_time = MPI_Wtime();
        MPI_Reduce(&z_start, &max, 1, MPI_INT, MPI_MAX, leader_rank, curr_comm);
        MPI_Bcast(&max, 1, MPI_INT, leader_rank, curr_comm);
        comm_end_time = MPI_Wtime();
        comm_time += (comm_end_time - comm_start_time);

        temp_buf = (char *) malloc(sizeof(char) * max + 1);
        memset(temp_buf, ' ', max);
        memcpy(temp_buf, z_coordinates, z_start);
        free(z_coordinates);
        z_coordinates = temp_buf; 
    
        // Time
        comm_start_time = MPI_Wtime();
        MPI_Gather(z_coordinates, max, MPI_CHAR, temp_buf, max, MPI_CHAR, leader_rank,curr_comm);
        comm_end_time = MPI_Wtime();
        comm_time += (comm_end_time - comm_start_time);

        z_start = max;

        // E
        // Time
        comm_start_time = MPI_Wtime();
        MPI_Reduce(&ev_start, &max, 1, MPI_INT, MPI_MAX, leader_rank, curr_comm);
        MPI_Bcast(&max, 1, MPI_INT, leader_rank, curr_comm);
        comm_end_time = MPI_Wtime();
        comm_time += (comm_end_time - comm_start_time);        
                
        temp_buf = (char *) malloc(sizeof(char) * max + 1);
        memset(temp_buf, ' ', max);
        memcpy(temp_buf, everything_else, ev_start);
        free(everything_else);
        everything_else = temp_buf;
        
        // Time
        comm_start_time = MPI_Wtime(); 
        MPI_Gather(everything_else, max, MPI_CHAR, temp_buf, max, MPI_CHAR, leader_rank,curr_comm);
        comm_end_time = MPI_Wtime();
        comm_time += (comm_end_time - comm_start_time);
        
        ev_start = max;

        if(DEBUG_MODE) {
          printf("Level %i: Rank %i sent %i bytes to Rank %i\n", i, rank, x_start + y_start + z_start + ev_start, temp_lead);
        }
        MPI_Comm_free(&curr_comm);
        break;
      }
    } 
  }

  // other proccesses do extra splits for process still in loop
  for(; gather_count < total_level - 1; gather_count++) {
    MPI_Comm_split(MPI_COMM_WORLD, nproc * total_level + 1, rank, &curr_comm);
    MPI_Comm_free(&curr_comm);
  }

  // Time
  comm_start_time = MPI_Wtime();
  MPI_Reduce(&total_bytes, &global_total_bytes, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);
  comm_end_time = MPI_Wtime();
  comm_time += (comm_end_time - comm_start_time);
  
  if(DEBUG_MODE && rank == 0) {
    printf("sum_bytes %i expected_sum_bytes %i\n", sum_bytes(x_coordinates,x_start) + sum_bytes(y_coordinates, y_start) + sum_bytes(z_coordinates, z_start) + sum_bytes(everything_else, ev_start),global_total_bytes);
  }

  // clean up
  free(x_coordinates);
  free(y_coordinates);
  free(z_coordinates);
  free(everything_else); 
  

  /* END CLOCK */
  tot_end_time = MPI_Wtime();
  tot_time = tot_end_time - tot_start_time;
  comp_time = tot_time - io_time - comm_time;


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
  MPI_Finalize();


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

// true iff $(rank) is leader in $(level_array)
int next_level_leader(int* level_array, int rank, int size) {
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
