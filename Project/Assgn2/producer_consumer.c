#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <stdint.h>
#include <assert.h>
#include "mpi.h"



#define MIN 1
#define BROKER_RANK 0
// circular buffer
struct circ_buf {
  int head;
  int tail;
  int *buf;
  uint32_t size;
};

// type definitions for cicular buffer
typedef struct circ_buf circ_buf;
typedef circ_buf* circ_buf_hand;




// function prototypes
double time_elapsed(clock_t start);
circ_buf_hand circ_buf_init(uint32_t size);
void circ_buf_free(circ_buf_hand buf);
int is_free(circ_buf_hand buf);
int is_empty(circ_buf_hand buf);
void insert_tail(circ_buf_hand buf, int num);
int get_head(circ_buf_hand buf);




// enumerator for message types
enum msg_types {WORK = MIN, ACK, REQ, ABORT};



int main(int argc, char* argv[]) {

  
  int nproc, rank, ncons, nprod;
  double x;
  int num_msg;
  clock_t start;
  enum msg_types response_type;


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


  // number of producers
  ncons = nproc / 2 - 1;


  // number of consumers
  nprod = nproc - ncons - 1;

  
  

  // beging clock
  start = clock();

















  // Process is broker
  /* BROKER */
  if(rank == BROKER_RANK) {
    

    // circular buffer
    circ_buf_hand buf_handler = circ_buf_init(2*nprod);
    // process queue
    circ_buf_hand proc_q = circ_buf_init(nprod);



    int num, msg_type, source;
    int rec_tag = MPI_ANY_TAG;
    

    /* while the timer has not expired */
    while(time_elapsed(start) < x) {
      

      // Receive message (blocking call)
      MPI_Recv(&num, 1, MPI_INT, MPI_ANY_SOURCE, rec_tag, MPI_COMM_WORLD, &status);
      rec_tag = MPI_ANY_TAG;

      
      // Get MPI tag
      msg_type = status.MPI_TAG;


      // Get source ID number
      source = status.MPI_SOURCE;


      // Check if the elapsed time is >= the simulation window.
      // If yes, response_type = "ABORT"
      if(time_elapsed(start) >= x) {


	       response_type = ABORT;
      }


      // Else, response_type = -1
      else {


        response_type = -1;
      }


      /* if "Work" message from the producer */
      if(msg_type == WORK) {


 	      /* if free space in the buffer */
      	if(is_free(buf_handler)) {


          // Insert the message (random number) at the end of the buffer
          insert_tail(buf_handler, num);

          
	        // Send ACK message to the producer
	        MPI_Send(&num, 1, MPI_INT, source, ACK, MPI_COMM_WORLD);
          
	      }


	      /* else if buffer is full */
	      else {


	        // if response_type is ABORT
	        if(response_type == ABORT) {

            
	          // SEND ABORT message to the producer instead of an ACK message
 	          MPI_Send(&num, 1, MPI_INT, source, ABORT, MPI_COMM_WORLD);
            

	        }


          else {


	           // Save information for processes that need to be acknowledged in an Outstanding queue.
             insert_tail(proc_q, source);
	        }


        }


      }


      // Else if "Work request" message from the consumer"
      else if(msg_type == REQ) {


        // If response type is ABORT
	      if(response_type == ABORT) {

          
	        // Send ABORT message to the consumer instead of a valid work
	        MPI_Send(&num, 1, MPI_INT, source, ABORT, MPI_COMM_WORLD);
          
	      }


	      // Else if there is message in the buffer
	      else if(!is_empty(buf_handler)) {

          
	        // Remove the first element from the buffer and send the message to the consumer
        	num = get_head(buf_handler);
        	MPI_Send(&num, 1, MPI_INT, source, REQ, MPI_COMM_WORLD);
          


      	  // If there is any producers waiting for ACK from processor 0
          while(!is_empty(proc_q)) {


            // Send ACK to that producer
            int proc_id = get_head(proc_q);

            
            MPI_Send(&num, 1, MPI_INT, proc_id, ACK, MPI_COMM_WORLD);
            
          }


	     }


  	    // Else there is no message in buffer
  	    else {

         
      	  // Send "no work for you" message to the CONSUMER
      	  MPI_Send(&num, 1, MPI_INT, source, ACK, MPI_COMM_WORLD);
          

      	  // Handle the case so that the broker accepts only messages from producers in the next round
          rec_tag = WORK;


  	    }


      }


    }

    int i;
    for(i = 1; i < nproc; i++) {
      MPI_Send(&num, 1, MPI_INT, i, ABORT, MPI_COMM_WORLD);
    }

    int consumed_count = 0;
    MPI_Reduce(&consumed_count,&num_msg,1,MPI_INT,MPI_SUM,0,MPI_COMM_WORLD);

    circ_buf_free(buf_handler);
    circ_buf_free(proc_q);
    
  }


























  // Process is producer
  /* PRODUCER */


  else if(rank < nprod + 1) {


    int num, msg_type;


    // generate random number
    num = rand();


    while(1) {

      // Send "Work" message to the broker using a non-blocking call
      
      MPI_Isend(&num, 1, MPI_INT, BROKER_RANK, WORK, MPI_COMM_WORLD, &request);
      MPI_Wait(&request, &status);
      

      // Wait till ACK is received (blocking wait call)
      MPI_Recv(&num, 1, MPI_INT, BROKER_RANK, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
      
      msg_type = status.MPI_TAG;


      // If message type is ACK(NOWLEDGED)
      if(msg_type == ACK) {


        continue;
      }


      // Else if message type is ABORT
      else if(msg_type == ABORT) {
        int consumed_count = 0;
	MPI_Reduce(&consumed_count, &num_msg, 1, MPI_INT, MPI_SUM, BROKER_RANK, MPI_COMM_WORLD);


	       break;
      }


    }


  }






















  // Process is consumer
  /* CONSUMER */
  else {


    // number of consumed messages
    int num, consumed_count, msg_type;


    while(1) {

      
      // Send "Request Work" message to the borker using a non-blocking call
      MPI_Isend(&num, 1, MPI_INT, BROKER_RANK, REQ, MPI_COMM_WORLD, &request);
      MPI_Wait(&request, &status);

      
      // Wait till you receive a message from the broker.
      MPI_Recv(&num, 1, MPI_INT, BROKER_RANK, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
      

      // Get message type
      msg_type = status.MPI_TAG;


      // If it gets work from the broker then increment the consumed_count variable
      if(msg_type == REQ) {
	       consumed_count++;
      }


      // Else if it gets an ABORT message from the broker
      else if(msg_type == ABORT) {
        MPI_Reduce(&consumed_count, &num_msg, 1, MPI_INT, MPI_SUM, BROKER_RANK, MPI_COMM_WORLD);
        

	      // Exit the Loop
        break;
      }


      // Else continue with the loop
    }



  }


  if(rank == 0)
    printf("%d message have been consumed\n", num_msg);

  MPI_Finalize();
}





















// compute time elapsed starting at clock_t start
double time_elapsed(clock_t start) {
  clock_t end = clock(); 
  return (double) (end - start) / CLOCKS_PER_SEC;
}


// initialize circular buffer
circ_buf_hand circ_buf_init(uint32_t size) {
  assert(size);

  circ_buf_hand buf_handler = (circ_buf_hand) malloc(sizeof(circ_buf));

  assert(buf_handler);

  buf_handler->buf = (int *) malloc(sizeof(int) * size + 1);
  buf_handler->size = size + 1;

  // set to empty state
  buf_handler->head = 0;
  buf_handler->tail = 0;

}


// free buffer and buffer_handler
void circ_buf_free(circ_buf_hand buf_handler) {
  free(buf_handler->buf);
  free(buf_handler);
}



// return 1 if free spot in buffer, 0 otherwise
int is_free(circ_buf_hand buf_handler) {


  // if full, return 0
  if (((buf_handler->tail) + 1) % buf_handler->size == buf_handler->head) {
      return 0;
  }

  return 1;
}


// return 1 if buffer is empty, 0 otherwise
int is_empty(circ_buf_hand buf_handler) {
  if (buf_handler->head == buf_handler->tail) {
    return 1;
  }

  return 0;
}


void insert_tail(circ_buf_hand buf_handler, int num) {
  buf_handler->buf[buf_handler->tail] = num;
  buf_handler->tail = (buf_handler->tail + 1) % buf_handler->size;
}

int get_head(circ_buf_hand buf_handler) {
  int x = buf_handler->buf[buf_handler->head];
  buf_handler->head = (buf_handler->head + 1) % buf_handler->size;
  return x;
}
