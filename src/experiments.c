#include <time.h>

#include "common.h"
#include "program_options.h"
#include "experiments.h"
#include "stats.h"


static inline float timed_read_write(int block_index, ProgramOptions *opts, short is_read){
    struct timespec begin, end;
    FILE *fp;
    if(is_read)
        fp = fopen(opts->filename, "r");
    else
        fp = fopen(opts->filename, "r+");
    if(!fp){
        fprintf(stderr, "Error while opening the file.\n");
        exit(IO_ERROR);
    }
    int wrapped_block_index = block_index % (opts->file_size / opts->block_size);
    if(fseek(fp, opts->block_size * wrapped_block_index, SEEK_SET) != 0){
        fprintf(stderr, "Error while moving within the file.\n");
        fclose(fp);
        exit(IO_ERROR);
    }
    char *buffer = malloc(sizeof(char) * opts->block_size);
    if(!buffer){
        fprintf(stderr, "Error while allocating memory.\n");
        fclose(fp);
        exit(MEMORY_ERROR);
    }
    size_t ret_val;
    clock_gettime(CLOCK_MONOTONIC_RAW, &begin);
    if(is_read)
        ret_val = fread(buffer, sizeof(char) * opts->block_size, 1, fp);
    else
        ret_val = fwrite(buffer, sizeof(char) * opts->block_size, 1, fp);
    clock_gettime(CLOCK_MONOTONIC_RAW, &end);
    if(ret_val < 1){
        fprintf(stderr, "timed_read_write: error while writing/reading portion of the file.\n");
        exit(IO_ERROR);
    }
    free(buffer);
    fclose(fp);
    // compute elapsed time in seconds
    float elapsed_time = (end.tv_nsec - begin.tv_nsec) / 1000000000.0 + (end.tv_sec  - begin.tv_sec);
    return elapsed_time;
}



float run_sequential_experiment(ProgramOptions *opts){
    float *read_times = (float*) malloc(sizeof(float) * opts->block_count);
    float *write_times = (float*) malloc(sizeof(float) * opts->block_count);
    int num_writes = 0, num_reads = 0;
    if(!read_times || !write_times){
        fprintf(stderr, "Error while allocating memory.\n");
        exit(MEMORY_ERROR);
    }
    // TODO: possible parallelisation here
    for(int current_block = 0; current_block < opts->block_count; current_block++){
        // choose op: read or write?
        // TODO: set seed
        if(((float)rand())/RAND_MAX <= opts->read_prob){
            // it is a read op
            read_times[num_reads++] = timed_read_write(current_block, opts, 1);
        }else{
            // it is a write op
            write_times[num_writes++]= timed_read_write(current_block, opts, 0);
        }
    }

    // stats
    float block_size_in_mb = opts->block_size / (1024.0f * 1024);
    float avg_read_bandwidth = block_size_in_mb / mean(read_times, num_reads);
    float avg_write_bandwidth = block_size_in_mb / mean(write_times, num_writes);
    printf("Average read bandwidth: %.3f MB/s.\n", avg_read_bandwidth);
    printf("Average write bandwidth: %.3f MB/s.\n", avg_write_bandwidth);
}


