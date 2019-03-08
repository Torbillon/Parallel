#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <stdint.h>
#include <assert.h>
#include "mpi.h"

#define MIN 1
double time_elapsed(clock_t start);

enum msg_types {WORK = MIN, ACK};

int main(int argc, char* argv[]) {

  int nproc, rank, global_count;
  double x;
  clock_t start;
  enum msg_types response_type, msg_type;

  MPI_Status status;
  MPI_Request request;

  if(argc != 2) {
    printf("Error: number of seconds, X, not given\n");
    exit(1);
  }


  // number of seconds to execute program
  x = (double) atoi(argv[1]);


  // Begin parallel region
  // Init
  MPI_Init(&argc, &argv);


  // Get rank and number of processes
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &nproc);


  // begin clock
  start = clock();


  // number of message consumed
  int consumed_count = 0;


  // messsage buffer and destination for message
  int num, dest, source;


  while(time_elapsed(start) < x) {

    // Produce an Item and send it to a random processor
    num = rand();
    dest = rand() % nproc;
    MPI_Send(&num, 1, MPI_INT, dest, WORK, MPI_COMM_WORLD);

    MPI_Recv(&num, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
    msg_type = status.MPI_TAG;
    source = status.MPI_SOURCE;


    // If Work is received
    if(msg_type == WORK) {


      // Send ACK to the sender
      MPI_Send(&num, 1, MPI_INT, source, ACK, MPI_COMM_WORLD);


      // Increment the consumed_count
      consumed_count++;
    }

    // Else ACK is received, so continue
  }
  MPI_Reduce(&consumed_count, &global_count, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);

  if(rank == 0)
    printf("%d message have been consumed\n",global_count);

  MPI_Finalize();
}


// compute the time elapsed in seconds
double time_elapsed(clock_t start) {


  clock_t end = clock();
  return (double) (end - start) / CLOCKS_PER_SEC;
}
