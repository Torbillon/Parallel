/* noncontiguous access with a single collective I/O function */
#include "mpi.h"
#include <stdlib.h>
#include <string.h>


#define FILESIZE      1048320
//#define FILESIZE 1024
#define INTS_PER_BLK  16

int main(int argc, char **argv)
{
  int *buf, rank, nprocs, nints, bufsize, num_blocks, stride, disp;
  MPI_File fh;
  MPI_Datatype filetype;
  MPI_Status status;

  // Initialize MPI
  MPI_Init(&argc, &argv);
  // Get rank
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  // Get number of processes
  MPI_Comm_size(MPI_COMM_WORLD, &nprocs);

  
  bufsize = FILESIZE/nprocs;
  buf = (int *) malloc(bufsize);
  nints = bufsize/sizeof(int);

  num_blocks = nints / INTS_PER_BLK;
  stride = INTS_PER_BLK * nprocs;

  memset(buf, 'A'+rank, nints * sizeof(int));

  // Create datatype
  MPI_Type_vector(num_blocks, INTS_PER_BLK, stride, MPI_INT, &filetype);
  MPI_Type_commit(&filetype);

  // Setup file view
  disp = INTS_PER_BLK * rank * sizeof(int);
  MPI_File_open(MPI_COMM_WORLD, "temp.txt", MPI_MODE_CREATE | MPI_MODE_WRONLY, MPI_INFO_NULL, &fh);
  MPI_File_set_view(fh, disp, MPI_INT, filetype, "native", MPI_INFO_NULL);

  // Collective MPI-IO write
  MPI_File_write_all(fh, buf, nints, MPI_INT, &status);

  // Close file
  MPI_File_close(&fh);
  MPI_Type_free(&filetype);

  //Implement collective file reading as an exercise.
  free(buf);
  
  //Finalize
  MPI_Finalize();

  return 0; 
}
