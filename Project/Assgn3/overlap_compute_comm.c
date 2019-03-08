#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <mpi.h>
#include <string.h>
#include <assert.h>
#include <omp.h>
#define MAX_ARGV 2
#define MPI_ROOT_PROCESS 0
#define BUF_SIZE 1024
#define TEMP_BUF_SIZE 33
#define FILE_NAME_SIZE 64
#define ENUM_MIN 1
#define DELIMITER_CHARS " \n\r"
#define MPI_ERROR_CODE 1
#define NUM_TIME 3
// Message Types
enum msg_types {MPI_PREP_TAG = ENUM_MIN, MPI_AVG_TAG};
 
// Helper Functions
double compute_avg_parallel(int* buf, int buf_size);
void perturb_elements(int* int_reg, int curr_avg, int prev_avg);
void compute(int rank, int nproc);
void read_file(int rank, int nproc);
void get_info(char* file_name);
void clean_up();
int rem(int x, int y);
void broadcast_data();
void compute_time(int rank, int nproc);

// Global Variables
uint32_t num_steps = 0;
uint32_t large_file_size = 0;
uint32_t num_int_blk = 0; 
int* int_reg = NULL;
int nints = 0;
double comp_time = 0.0;
double comm_time = 0.0;
double io_time = 0.0;
double tot_time = 0.0;













int main(int argc, char* argv[]) {

  if(argc != MAX_ARGV) {
    printf("Error: Not enough arguments");
    exit(1);
  }

  int nproc, rank; 
  double start = 0.0, end = 0.0;
  MPI_Init(&argc, &argv);
  MPI_Comm_size(MPI_COMM_WORLD, &nproc);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  
  start = MPI_Wtime();
  if(rank == MPI_ROOT_PROCESS) {
    get_info(argv[1]);
  }
  
  // Get Data From Paramater Files
  broadcast_data();

  // Read File For Integer Array
  read_file(rank, nproc);

  // Do Loop Computation
  compute(rank, nproc);
  end = MPI_Wtime();
  tot_time += (end - start);

  // Compute Time Spent
  compute_time(rank,nproc);

  // Clean Up This God-Forsaken Mess
  clean_up();
  
  // FIN
  MPI_Finalize();
  return 0;
}














// Update Elements
void perturb_elements(int* buf, int curr_avg, int prev_avg) {
  int i;
  if(curr_avg > prev_avg) {
    #pragma omp parallel 
    {
      #pragma omp for
      for(i = 0; i < nints; i++) {
        buf[i] -= 1; 
      }
    }

  }
  else if(curr_avg < prev_avg) {
    #pragma omp parallel
    {
      #pragma omp for
      for(i = 0; i < nints; i++) {
        buf[i] += 1;
      }
    }
  }
}












// Compute Average Value From Array
double compute_avg_parallel(int* buf, int buf_size) {
  int i;
  double sum = 0.0;
  #pragma omp parallel for reduction (+: sum)
  for(i = 0; i < buf_size; i++) {
    sum += buf[i];
  }
  return sum / buf_size;
}













// Get num_steps, large_file_size, and num_int_blk to all processes
void broadcast_data() {
  double start = 0.0;
  double end = 0.0;
  start = MPI_Wtime();
  MPI_Bcast(&num_steps, 1, MPI_UNSIGNED, MPI_ROOT_PROCESS, MPI_COMM_WORLD);   
  MPI_Bcast(&large_file_size, 1, MPI_UNSIGNED, MPI_ROOT_PROCESS, MPI_COMM_WORLD);
  MPI_Bcast(&num_int_blk, 1, MPI_UNSIGNED, MPI_ROOT_PROCESS, MPI_COMM_WORLD);
  end = MPI_Wtime();
  comm_time += (end - start);
}









 



// Perform Cooperative Parallel I/O Read
void read_file(int rank,int nproc) {
  double start = 0.0;
  double end = 0.0;

  start = MPI_Wtime(); 
  // Make "datafile$(nproc)" String
  char data_file[FILE_NAME_SIZE];
  strcpy(data_file,"datafile");  
  char temp_buf[TEMP_BUF_SIZE];
  sprintf(temp_buf, "%i",nproc);
  strcat(data_file,temp_buf);

  // Variables
  MPI_Datatype filetype;
  MPI_File fh;
  MPI_Status stat;
  int buf_size = large_file_size / nproc;
  nints = buf_size/sizeof(int);
  int num_blocks = nints / num_int_blk;
  int stride = num_int_blk * nproc;
  int_reg = (int *) malloc(sizeof(int) * nints);
  assert(int_reg);

  // Create datatype
  MPI_Type_vector(num_blocks, num_int_blk, stride, MPI_INT, &filetype);
  MPI_Type_commit(&filetype);
 
  // Setup file view
  int disp = num_int_blk * rank * sizeof(int);
  MPI_File_open(MPI_COMM_WORLD, data_file, MPI_MODE_RDONLY, MPI_INFO_NULL, &fh);
  MPI_File_set_view(fh, disp, MPI_INT, filetype, "native", MPI_INFO_NULL);

  // Collective MPI-IO read
  MPI_File_read_all(fh, int_reg, nints, MPI_INT, &stat);
  MPI_File_close(&fh);
  MPI_Type_free(&filetype);  

  end = MPI_Wtime();
  io_time += (end-start);
}















// Do Loop Computation
void compute(int rank, int nproc) {
  // Variables
  MPI_Status stat;
  MPI_Request req1;
  MPI_Request req2;
  int next_proc = rem(rank + 1, nproc);
  int prev_proc = rem(rank - 1, nproc);
  double curr_avg = 0.0;
  double prev_avg = 0.0;
  double start = 0.0;
  double end = 0.0;
  int i;
 
  for(i = 0; i < num_steps; i++) {

    start = MPI_Wtime();
    // Send curr_avg To next_proc And Receive prev_avg From prev_proc
    MPI_Isend(&curr_avg, 1, MPI_DOUBLE, next_proc, MPI_AVG_TAG, MPI_COMM_WORLD, &req1);
    MPI_Irecv(&prev_avg, 1, MPI_DOUBLE, prev_proc, MPI_AVG_TAG, MPI_COMM_WORLD, &req2);
    end = MPI_Wtime();
    comm_time += (end - start);    

    // Compute Average
    curr_avg = compute_avg_parallel(int_reg, nints);

    start = MPI_Wtime();
    // Wait For Receive
    MPI_Wait(&req2,&stat);
    end = MPI_Wtime();
    comm_time += (end - start);

    // Update Elements
    perturb_elements(int_reg, curr_avg, prev_avg);
    

    start = MPI_Wtime();
    // Wait For Send 
    MPI_Wait(&req1,&stat);
    end = MPI_Wtime();
    comm_time += (end - start);
  }
}












// Compute Time Spent
void compute_time(int rank, int nproc) {

  // Boring Computation
  comp_time = tot_time - comm_time - io_time;
  double* time_array = (double *) malloc(sizeof(double) * NUM_TIME);
  assert(time_array);
  time_array[0] = comp_time;
  time_array[1] = comm_time;
  time_array[2] = io_time;

  int tot_dub = nproc * NUM_TIME;
  double *time_data;
  if (rank == MPI_ROOT_PROCESS) {
    time_data = (double *) malloc(sizeof(double) * tot_dub);
    assert(time_data);
  }

  // Aggregate data
  MPI_Gather(time_array, NUM_TIME, MPI_DOUBLE, time_data, NUM_TIME, MPI_DOUBLE, MPI_ROOT_PROCESS, MPI_COMM_WORLD);
  
  // If Process Is Root
  if(rank == MPI_ROOT_PROCESS) {
    double tot_comp_time = 0.0;
    double tot_comm_time = 0.0;
    double tot_io_time = 0.0;
    double start  = 0.0;
    double end = 0.0;
    int i;

    // Compute Total Time
    start = MPI_Wtime();
    for(i = 0; i < nproc; i++) {
      int j = NUM_TIME * i; 
      tot_comp_time += time_data[j];
      tot_comm_time += time_data[j + 1];
      tot_io_time += time_data[j + 2];
    }
    end = MPI_Wtime();
    tot_comp_time += (end - start);
    
    // Compute Time Distribution  
    double avg_comp_time = tot_comp_time / nproc;
    double avg_comm_time = tot_comm_time / nproc;
    double avg_io_time = tot_io_time / nproc;
    double avg_tot_time = avg_comp_time + avg_comm_time + avg_io_time;
    
    // Print Distribution
    printf("Compute: %.2f Comm: %.2f IO: %.2f\n", avg_comp_time / avg_tot_time * 100, avg_comm_time / avg_tot_time * 100, avg_io_time / avg_tot_time * 100);
  }

  // Clean Up
  free(time_array);
}












// Get Information From Paramater File
void get_info(char* file_name) {
    double start = 0.0;
    double end = 0.0;

    start = MPI_Wtime();
    // Read Parameter File
    FILE* param_file = fopen(file_name, "r");  
    if(param_file == NULL) {
      printf("Error: File cannot be opened\n");
      MPI_Abort(MPI_COMM_WORLD, MPI_ERROR_CODE);
    }

    // Get First Line From File
    char buf[BUF_SIZE];
    char* curr_token;    
    fgets(buf, BUF_SIZE, param_file);

    // Get num_steps, large_file_size, num_int_blk
    curr_token = strtok(buf, DELIMITER_CHARS);
    num_steps = atoi(curr_token);

    
    fgets(buf, BUF_SIZE, param_file);
    curr_token = strtok(buf, DELIMITER_CHARS);
    large_file_size = atoi(curr_token);
    fgets(buf, BUF_SIZE, param_file); 
    curr_token = strtok(buf, DELIMITER_CHARS);
    num_int_blk = atoi(curr_token);
    
    // Clean Up
    fclose(param_file);
    end = MPI_Wtime();
    io_time += (end - start);
}















// Return Remainder
int rem(int x, int y) {
  return (x + y) % y;
}















// Clean Up
void clean_up() {
  free(int_reg);
}
