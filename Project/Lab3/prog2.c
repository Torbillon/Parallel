#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <string.h>
int main( int argc, char *argv[] ) {
  int nproc, rank, rank2;
  char buf[33];
  char buf2[33];
  MPI_Status Stat;


  MPI_Init (&argc,&argv); /* Initialize MPI */
  MPI_Comm_size(MPI_COMM_WORLD,&nproc); /* Get Comm Size - how many 
  ppl working*/
  MPI_Comm_rank(MPI_COMM_WORLD,&rank); /* Get rank -- what is my 
  position */
  int random = rand();
  sprintf(buf, "%d", random);
  buf[4] = '\0';  
  if (rank == 0) {
    MPI_Send(&buf, 1, MPI_INT, ((rank + 1) % nproc), 1, MPI_COMM_WORLD);
    printf("Process %d sent the number %s to process %d\n",rank,buf,((rank + 1) % nproc));
    MPI_Recv(&buf2, 1, MPI_INT, ((rank - 1) % nproc), 1, MPI_COMM_WORLD, &Stat);
    printf("Process %d received the number %s from process %d\n",rank,buf2,((rank - 1 + nproc) % nproc));
  } else {
    MPI_Recv(&buf2, 1, MPI_INT, (rank - 1) % nproc, 1, MPI_COMM_WORLD, &Stat);
    printf("Process %d received the number %s from process %d\n",rank,buf2,((rank - 1 + nproc) % nproc));
    MPI_Send(&buf2, 1, MPI_INT, (rank + 1) % nproc, 1, MPI_COMM_WORLD);
    printf("Process %d sent the number %s to process %d\n",rank,buf2,((rank + 1) % nproc));
  }



  MPI_Finalize(); /* Finalize */

 
  return 0;
}
