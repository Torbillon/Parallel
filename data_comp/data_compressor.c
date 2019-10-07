#ifndef NABLA_H

#define NABLA_H
#include "nabla.h"
#endif


int main(int argc, char* argv[]) {

  file_data_hand_t file_data;
  nabla_data_hand_t nabla_data;
  stack_tuple_hand_t bio_data;
  mpi_data_hand_t mpi_data;

  // preprocess
  pre_proc(argc, argv, &nabla_data, &file_data, &mpi_data, &bio_data);

  // for each level
  int i;
  for(i = 0; i < nabla_data->total_level && file_data->file_count > 0; i++) {

    // if not level 0
    if(i > 0) {
      split_comm(mpi_data);
      get_data(bio_data, mpi_data, i);

      // if not last level and no longer leader for next level
      if(is_leader(nabla_data, mpi_data, i)) {
        send_data(nabla_data, bio_data, mpi_data, i);
        break;
      }
    }

    // else level is 0
    else {
      // do compression/aggregation
      aggregate_files(bio_data, file_data);
      data_aware_comp(bio_data, mpi_data);
      get_total_bytes(bio_data);

      // if this process is not leader for next level, send data to leader
      if(is_leader(nabla_data, mpi_data, i)) {
        send_data(nabla_data, bio_data, mpi_data, i);
        break;
      }
    }
  }
  finalize(mpi_data, nabla_data, bio_data);
}
