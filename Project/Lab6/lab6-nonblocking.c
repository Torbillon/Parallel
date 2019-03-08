#include <stdlib.h>
#include <stdio.h>
#include "mpi.h"
#define MAXPROC 100    /* Max number of procsses */

int main(int argc, char* argv[]) {
  int i, nproc, rank, index;
  int tag  = 42;    /* Tag value for communication */
  int root = 0;     /* Root process in broadcast */

  MPI_Status status;              /* Status object for non-blocing receive */
  MPI_Request recv_req[MAXPROC];  /* Request objects for non-blocking receive */
  
  char hostname[MAXPROC][MPI_MAX_PROCESSOR_NAME];  /* Received host names */
  char myname[MPI_MAX_PROCESSOR_NAME]; /*local host name string */
  int namelen; // Length of the name

  //Begin parallel region
  //Init
  MPI_Init(&argc, &argv);
  //Rank --> rank
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  //Size --> nproc
  MPI_Comm_size(MPI_COMM_WORLD, &nproc);
 
 
  // Get hostname
  //MPI_Get_processor_name --> myname, namelen
  MPI_Get_processor_name(myname,&namelen);
  
  /* Terminate received buffer with null byte */
  hostname[rank][namelen] = '\0';
  if (rank == 0) {    /* Process 0 does this */
   
    /* Broadcast a message containing the process id */
    MPI_Bcast(&root,1,MPI_INT,0,MPI_COMM_WORLD);
   
    /* Start non-blocking calls to receive messages from all other processes */
    for (i = 1; i < nproc; i++) {
	MPI_Irecv(hostname[i],MPI_MAX_PROCESSOR_NAME, MPI_CHAR,MPI_ANY_SOURCE,tag,MPI_COMM_WORLD,&recv_req[i]);
    }

    /* While the messages are delivered, we could do computations here */
    printf("I am a very busy professor.\n");	
    
    /* Iterate to receive messages from all other processes and print their hostnames*/
    for (i = 1; i < nproc; i++) {
    	MPI_Waitany(nproc-1,&recv_req[1],&index,&status);
        printf("Received a message from process %d on %s\n", status.MPI_SOURCE, hostname[index+1]);
    }
  } 
  else { /* all other processes do this */
    //printf("ay\n");
    /* Receive the broadcasted message from process 0 */
    MPI_Bcast(&root,1,MPI_INT,0,MPI_COMM_WORLD);
    
    /* Send local hostname to process 0 */
    MPI_Send(&hostname[rank],namelen,MPI_CHAR,root,tag,MPI_COMM_WORLD);
   
    
  }

  // Finish by finalizing the MPI library
  MPI_Finalize();
  exit(0);
}
