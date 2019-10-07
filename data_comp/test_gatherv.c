#include <stdio.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <ctype.h>
#include <mpi.h>
#define ROOT_PROC 0

int main(int argc, char* argv[]) {
    MPI_Comm comm;
    int nproc, send_int;
    int rook, rank, *r_buf;

    int i;

    // Being parrallelizing
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nproc);

    send_int = rank;

    if(rank == ROOT_PROC) {
        r_buf = (int *) malloc(sizeof(int) * nproc);
    }

    MPI_Gather(&send_int,1,MPI_INT,r_buf,1,MPI_INT,ROOT_PROC,MPI_COMM_WORLD);

    if(rank == ROOT_PROC) {
        printf("this is rank %d \n", rank);
        int len = nproc;
        for(i = 0; i < len; i++)
           printf("%d ",r_buf[i]);
    }
    MPI_Finalize();
}
