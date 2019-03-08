#include <stdio.h>
#include <omp.h>
#define MAX 24

int main() {
        int nthread, tid, i;

        #pragma omp parallel private(tid) 
        {
                tid = omp_get_thread_num();
                printf("HelloWorld from thread = %d\n\n",tid);

                if(tid ==0) {
                        nthread = omp_get_num_threads();
                        printf("Number of threads = %d\n", nthread);
                }


                #pragma omp for


                        for (i = 0; i < MAX; i++) {
                                printf("%d does %d\n", tid, i);
                        }

        }

}
