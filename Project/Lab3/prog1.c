#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
int main( int argc, char *argv[] ) {
  int nproc, rank, rank2;
  char buf[33];
  char buf2[33];
  MPI_Status Stat;

  /* get process i and j from user */
  int i = atoi(argv[1]);
  int j = atoi(argv[2]);

  MPI_Init (&argc,&argv); /* Initialize MPI */
  MPI_Comm_size(MPI_COMM_WORLD,&nproc); /* Get Comm Size - how many 
  ppl working*/
  MPI_Comm_rank(MPI_COMM_WORLD,&rank); /* Get rank -- what is my 
  position */

  sprintf(buf, "%d",rank);

  if (rank == i) {
    MPI_Send(&buf, 1, MPI_INT, j, 1, MPI_COMM_WORLD);
    printf("Process %d sent ID %s to process %d\n",i,buf,j);
  } else if (rank == j) {
    MPI_Recv(&buf2, 1, MPI_INT, i, 1, MPI_COMM_WORLD, &Stat);
    printf("Process %d received ID %s from process %d\n",j,buf2,i);
  }



  MPI_Finalize(); /* Finalize */

  
  return 0;
}
