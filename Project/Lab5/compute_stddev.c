#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <math.h>
#include <assert.h>

// Creates an array of random numbers. Each number has a value from 0 - 1
float *rand_num_gen(int num_elements) {
  float *rand_nums = (float *)malloc(sizeof(float) * num_elements);
  assert(rand_nums != NULL);
  int i;
  for (i = 0; i < num_elements; i++) {
    rand_nums[i] = (rand() / (float)RAND_MAX);
  }
  return rand_nums;
}


int main(int argc, char** argv) {
  if (argc != 2) {
    fprintf(stderr, "Usage: avg num_elements_per_proc\n");
    exit(1);
  }


  int nproc, rank;
  MPI_Init(&argc, &argv);

  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &nproc);

  int num_elements_per_proc = atoi(argv[1]);


  // Create a random array of elements on all processes.
  srand(123456*nproc); // Seed the random number generator of processes uniquely
  float *rand_nums = NULL;
  rand_nums = rand_num_gen(num_elements_per_proc);
  // Sum the numbers locally
  float local_sum = 0.0;
  int i;
  for(i = 0; i < num_elements_per_proc; i++) {
    local_sum += rand_nums[i];
  }
  // Reduce all of the local sums into the global sum in order to
  float global_sum;
  MPI_Allreduce(&local_sum,&global_sum,1,MPI_FLOAT,MPI_SUM,MPI_COMM_WORLD);
  // calculate the mean
  float global_mean;

  global_mean = global_sum / nproc;
  // Compute the local sum of the squared differences from the mean
  float local_square_diff = 0.0, temp = 0.0;
  for(i = 0; i < num_elements_per_proc; i++) {
    temp = rand_nums[i] - global_mean;
    temp *= temp;
    local_square_diff += temp - global_mean;
    temp = 0.0;
  }
  // Reduce the global sum of the squared differences to the root process
  // and print off the answer
  float global_square_diff;
  MPI_Reduce(&local_square_diff,&global_square_diff,1,MPI_FLOAT,MPI_SUM,0,MPI_COMM_WORLD);
  // The standard deviation is the square root of the mean of the squared
  // differences.
  if(rank == 0) {
    int global_num_elements = nproc * num_elements_per_proc;
    float var = global_square_diff / global_num_elements;
    double std_dev = sqrt(var);
    printf("%f\n",std_dev);
  }
  // Clean up
  free(rand_nums);
  
}
                                                                               
