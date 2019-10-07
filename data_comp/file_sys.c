#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <assert.h>
#include <ctype.h>
#include <mpi.h>
#include <float.h>
#define FILE_READER_BUF_SIZE 1024
#define ATOM "ATOM"
#define HETATM "HETATM"
#ifndef FILE_SYS_H
#define FILE_SYS_H
#include "file_sys.h"
#endif
#define NUM_CHAR_LINE 45
#define NUM_CHAR_STRING 9

file_hand_array_t init_file_hand_array(char* file_name, int* file_count, int rank, int nproc, int line_count) {

  assert(file_name);

  // Variables
  MPI_Datatype filetype;
  MPI_File fh;
  MPI_Status stat;
  MPI_Comm comm;
  int num_blocks = line_count / nproc;
  int rem = line_count % nproc;
  int nchar = (num_blocks) * NUM_CHAR_LINE;
  char* buf = (char *) malloc(sizeof(char) * nchar + 1);
  buf[0] = '\0';
  assert(buf);
  int disp;

  if(num_blocks != 0) {

    // Logic
    MPI_Type_vector(num_blocks, NUM_CHAR_LINE, NUM_CHAR_LINE * nproc, MPI_CHAR, &filetype);
    MPI_Type_commit(&filetype);
    disp = NUM_CHAR_LINE * rank;
    MPI_File_open(MPI_COMM_WORLD, file_name, MPI_MODE_RDONLY, MPI_INFO_NULL, &fh); 
    MPI_File_set_view(fh, disp, MPI_CHAR, filetype, "native", MPI_INFO_NULL);
    MPI_File_read_all(fh, buf, nchar, MPI_CHAR, &stat);
    MPI_File_close(&fh);
    MPI_Type_free(&filetype);
  }
  buf[nchar] = '\0';
  // Do Extra Work
  if(rank >= 0 && rank < rem) {
    MPI_Comm_split(MPI_COMM_WORLD, 1, rank, &comm);

    // Read Rest of File to Buffer  
    num_blocks = 1;
    char* buf2 = (char *) malloc(sizeof(char) * NUM_CHAR_LINE + 1); 
    MPI_Type_vector(num_blocks, NUM_CHAR_LINE, NUM_CHAR_LINE, MPI_CHAR, &filetype);
    MPI_Type_commit(&filetype);
    int disp = (nchar * nproc * sizeof(char)) + (NUM_CHAR_LINE * rank * sizeof(char));
    
    // MPI Logic
    MPI_File_open(comm, file_name, MPI_MODE_RDONLY, MPI_INFO_NULL, &fh); 
    MPI_File_set_view(fh, disp, MPI_CHAR, filetype, "native", MPI_INFO_NULL);
    MPI_File_read_all(fh, buf2, NUM_CHAR_LINE, MPI_CHAR, &stat);
    MPI_File_close(&fh);
    MPI_Type_free(&filetype);
    
    // Concatenate Buffers
    char* buf3 = (char *) malloc(sizeof(char) * (nchar + NUM_CHAR_LINE) + 1);
    buf2[NUM_CHAR_LINE] = '\0';
    buf3[0] = '\0';
    strcat(buf3,buf);
    strcat(buf3,buf2);
    free(buf);
    free(buf2);
    buf = buf3;
    nchar += NUM_CHAR_LINE;
    MPI_Comm_free(&comm);
  } else {
    MPI_Comm_split(MPI_COMM_WORLD, 0, rank, &comm);
    MPI_Comm_free(&comm);
  }

  int i;
  uint32_t token_count = 0;
  char* curr_token;
  curr_token = strtok(buf, DELIMITER_CHARS);
  while(curr_token != NULL) {
    token_count++;
    curr_token = strtok(NULL, DELIMITER_CHARS);
  }
  // Post Condition token_count == $(number of tokens)

  int global_total_files;
  //printf("Rank: %i .. file_count: %i\n", mpi_data->global_rank, file_data->file_count);
  MPI_Reduce(&token_count, &global_total_files, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);
  if(rank == 0)
    printf("tot_token_count: %i\n", global_total_files);
  // intialize file_hand_array
  file_hand_array_t file_hand_array = (file_hand_array_t) malloc(sizeof(file_hand_t) * token_count);
  curr_token = buf;
  for(i = 0; i < token_count; i++) {
    add_file_hand_array(file_hand_array, curr_token, i);
    curr_token += strlen(curr_token) + 1;
    curr_token += strspn(curr_token, DELIMITER_CHARS);
  }

    
  *file_count = token_count;
  free(buf);
  return file_hand_array;
}

void add_file_hand_array(file_hand_array_t file_hand_array, char* file_name, int index) {
  
  file_hand_t file_handler = (file_hand_t) malloc(sizeof(file_t));
  // add file name to new file handler object
  // + 1 for null terminator

  file_handler->file_name = malloc(sizeof(char) * strlen(file_name) + 1);
  strcpy(file_handler->file_name, file_name);
  
  // add number of bytes to new file handler object
  struct stat st;
  if(stat(file_name, &st) == -1) {
    printf("Error stat: File \"%s\" could not be opened\n",file_name);
    MPI_Abort(MPI_COMM_WORLD, 1);
  }
  file_handler->file_size = st.st_size;

  // add new file_handler to file_handler_array
  file_hand_array[index] = file_handler;
}

int get_token_count(char* file_name) {

  // suppose $(file_name) is not null
  assert(file_name);
  FILE *open_file = fopen(file_name,"r");
  if(open_file == NULL) {
    printf("Error: File cannot be opened\n");
    exit(1);
  }

  // count tokens
  uint32_t token_count = 0;
  char buf[FILE_READER_BUF_SIZE+1];
  char* curr_token;
  while(fgets(buf, FILE_READER_BUF_SIZE, open_file) != NULL) {
    curr_token = strtok(buf, DELIMITER_CHARS);
    while(curr_token != NULL) {
      token_count++;
      curr_token = strtok(NULL, DELIMITER_CHARS);
    }
  }

  // Clean up
  fclose(open_file);
  return token_count;
}

// aggregate array of files
void aggregate_files(stack_tuple_hand_t bio_data, file_data_hand_t file_data) {
  
  char* buf_x = bio_data->buf_x, *buf_y = bio_data->buf_y, *buf_z = bio_data->buf_z, *buf_e =  bio_data->buf_e;
  int max, top_x = bio_data->top_x, top_y = bio_data->top_y, top_z = bio_data->top_z, top_e = bio_data->top_e;
  file_hand_array_t file_hand_array = file_data->file_hand_array;
  int file_count = file_data->file_count;
  int i;
  file_hand_t current_file_hand;
  char* file_name;
  FILE* open_file;
  char *delimiter_characters = " ";
  char buf[FILE_READER_BUF_SIZE];
  char* curr_token;

  // for each file, do aggregation
  for(i = 0; i < file_count; i++) {
    current_file_hand = file_hand_array[i];
    file_name = current_file_hand->file_name;
    open_file = fopen(file_name, "r");
    
    if(open_file == NULL) {
      printf("Error: File cannot be opened - %s\n",file_name);
      MPI_Abort(MPI_COMM_WORLD, 1);
    }
  
    while(fgets(buf, FILE_READER_BUF_SIZE, open_file) != NULL) {
      curr_token = strtok(buf, delimiter_characters);

      if(strcmp(curr_token, ATOM) == 0) {
        top_e += sprintf(buf_e + top_e, "%s ", curr_token);
          
        curr_token = strtok(NULL, delimiter_characters);
        top_e += sprintf(buf_e + top_e, "%s ", curr_token);

        curr_token = strtok(NULL, delimiter_characters);
        top_e += sprintf(buf_e + top_e, "%s ", curr_token);

        curr_token = strtok(NULL, delimiter_characters);
        top_e += sprintf(buf_e + top_e, "%s ", curr_token);
        
        curr_token = strtok(NULL, delimiter_characters);
        top_e += sprintf(buf_e + top_e, "%s ", curr_token);
        
        curr_token = strtok(NULL, delimiter_characters);
        top_e += sprintf(buf_e + top_e, "%s ", curr_token);

        // X        
        curr_token = strtok(NULL, delimiter_characters);
        top_x += sprintf(buf_x + top_x, "%s\n", curr_token);
        
        // Y
        curr_token = strtok(NULL, delimiter_characters);
        top_y += sprintf(buf_y + top_y, "%s\n", curr_token);

        // Z
        curr_token = strtok(NULL, delimiter_characters);
        top_z += sprintf(buf_z + top_z, "%s\n", curr_token);

        curr_token = strtok(NULL, delimiter_characters);
        top_e += sprintf(buf_e + top_e, "%s ", curr_token);

        curr_token = strtok(NULL, delimiter_characters);
        top_e += sprintf(buf_e + top_e, "%s ", curr_token);

        curr_token = strtok(NULL, delimiter_characters);
        top_e += sprintf(buf_e + top_e, "%s ", curr_token);
        
        curr_token = strtok(NULL, delimiter_characters);
        if(curr_token != NULL) {
          top_e += sprintf(buf_e + top_e, "%s ", curr_token);
        }

      // Same code for ATOM case, but might need later for aggregation
      } else if(strcmp(curr_token, HETATM) == 0) {
        
        top_e += sprintf(buf_e + top_e, "%s ", curr_token);
          
        curr_token = strtok(NULL, delimiter_characters);
        top_e += sprintf(buf_e + top_e, "%s ", curr_token);

        curr_token = strtok(NULL, delimiter_characters);
        top_e += sprintf(buf_e + top_e, "%s ", curr_token);

        curr_token = strtok(NULL, delimiter_characters);
        top_e += sprintf(buf_e + top_e, "%s ", curr_token);
        
        curr_token = strtok(NULL, delimiter_characters);
        top_e += sprintf(buf_e + top_e, "%s ", curr_token);
        
        curr_token = strtok(NULL, delimiter_characters);
        top_e += sprintf(buf_e + top_e, "%s ", curr_token);

        // X        
        curr_token = strtok(NULL, delimiter_characters);
        top_x += sprintf(buf_x + top_x, "%s\n", curr_token);
        
        // Y
        curr_token = strtok(NULL, delimiter_characters);
        top_y += sprintf(buf_y + top_y, "%s\n", curr_token);
        
        // Z
        curr_token = strtok(NULL, delimiter_characters);
        top_z += sprintf(buf_z + top_z, "%s\n", curr_token);

        curr_token = strtok(NULL, delimiter_characters);
        top_e += sprintf(buf_e + top_e, "%s ", curr_token);

        curr_token = strtok(NULL, delimiter_characters);
        top_e += sprintf(buf_e + top_e, "%s ", curr_token);
        
        curr_token = strtok(NULL, delimiter_characters);
        top_e += sprintf(buf_e + top_e, "%s ", curr_token);
        

        curr_token = strtok(NULL,delimiter_characters);
        if(curr_token != NULL) {
          top_e += sprintf(buf_e + top_e, "%s ", curr_token);
        }
        
      } else {
        while(curr_token != NULL) {
          top_e += sprintf(buf_e + top_e, "%s ", curr_token);
          curr_token = strtok(NULL,delimiter_characters);
        }
      }
    }
  }
  bio_data->buf_x = buf_x;
  bio_data->buf_y = buf_y;
  bio_data->buf_z = buf_z;
  bio_data->buf_e = buf_e;
  bio_data->top_x = top_x;
  bio_data->top_y = top_y;
  bio_data->top_z = top_z;
  bio_data->top_e = top_e;
}


long compute_max(file_hand_array_t file_hand_array, int size) {
  int i;
  long max = 0;
  for(i = 0; i < size; i++) {
    if(max < file_hand_array[i]->file_size) {
      max = file_hand_array[i]->file_size;
    }
  }
  return max;
}
