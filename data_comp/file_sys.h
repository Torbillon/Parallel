#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#ifdef FILE_SYS_H
#define FILE_SYS_H
#define FILE_READER_BUF_SIZE 1024
#define DELIMITER_CHARS " \f\n\r\t\v"
#endif

struct file_t {
  char* file_name;
  uint32_t file_size;
};


typedef struct file_t file_t;
// used to point to file "object"
typedef file_t* file_hand_t;

// used to represent array of file_hand_t (each element is pointer to file object)
typedef file_hand_t* file_hand_array_t;

struct file_data {
  file_hand_array_t file_hand_array;
  int file_count;
};

typedef struct file_data file_data_t;
typedef file_data_t* file_data_hand_t;
// used to represent array of file_hand_array_t (each element is an array of pointers to file objects);
typedef file_hand_array_t* file_hand_array_array_t;

struct stack_tuple {
  char* buf_x;
  char* buf_y;
  char* buf_z;
  char* buf_e;
  int top_x;
  int top_y;
  int top_z;
  int top_e;
};

typedef struct stack_tuple stack_tuple_t;
typedef stack_tuple_t* stack_tuple_hand_t;
/* 
 * file_hand_array_hand: handler of array of pointers to file objects
 * char* file_name: file containing all of the file names to evenly distribute
 *
 * Purpose: This function reads from  file_name  , checks whether each string token from
 * this file is a valid file
 *
 */
file_hand_array_t init_file_hand_array(char* file_name, int* file_count, int rank, int nproc, int line_count);

/*
 *
 * file_hand_array: array of file_hand_t
 * last_token
 *
 */
void add_file_hand_array(file_hand_array_t file_hand_array, char* file_name, int index);

/*
 *
 * char* file_name: file containing all of the file names
 *
 */
file_hand_array_t get_file_array(char* file_name, int file_count);

/*
 *
 * file_name: file containing all of the file names
 *
 * returns the number of tokens contained within $(file_name) on success
 * returns -1 upon failure
 * assumes $(file_name) is not null.
 *
 */
int get_token_count(char* file_name);

/* 
 * aggregate files and store into respective buffers
 * and store top of "stacks" (last element index of buffer)
 *
 */
void aggregate_files(stack_tuple_hand_t bio_data, file_data_hand_t file_data);

/* 
 * compute maximum file_size from file_t array
 */
long compute_max(file_hand_array_t file_hand_array, int size);
