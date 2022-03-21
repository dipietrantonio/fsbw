#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include "common.h"
#include "program_options.h"
#include "stats.h"


#define READ 1
#define WRITE 0


static inline float timed_linux_read_write(int block_index, ProgramOptions *opts, short is_read){
    struct timespec begin, end;
    int fd = open(opts->filename, O_RDWR | O_FSYNC); // | __O_DIRECT);
    if(fd == -1){
        fprintf(stderr, "timed_linux_read_write: error while opening the file.\n");
        exit(IO_ERROR);
    }
    if(lseek(fd, opts->block_size * block_index, SEEK_SET) == -1){
        fprintf(stderr, "timed_linux_read_write: error while moving within the file.\n");
        close(fd);
        exit(IO_ERROR);
    }
    char *buffer = malloc(sizeof(char) * opts->block_size);
    if(!buffer){
        fprintf(stderr, "Error while allocating memory.\n");
        close(fd);
        exit(MEMORY_ERROR);
    }
    ssize_t ret_val;
    clock_gettime(CLOCK_MONOTONIC_RAW, &begin);
    if(is_read)
        ret_val = read(fd, buffer, sizeof(char) * opts->block_size);
    else
        ret_val = write(fd, buffer, sizeof(char) * opts->block_size);
    clock_gettime(CLOCK_MONOTONIC_RAW, &end);
    if(ret_val < 0){
        fprintf(stderr, "timed_linux_read_write: error while writing/reading portion of the file.\n");
        exit(IO_ERROR);
    }
    free(buffer);
    close(fd);
    // compute elapsed time in seconds
    float elapsed_time = (end.tv_nsec - begin.tv_nsec) / 1000000000.0 + (end.tv_sec  - begin.tv_sec);
    return elapsed_time;
}




static inline float timed_posix_read_write(int block_index, ProgramOptions *opts, short is_read){
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
    if(fseek(fp, opts->block_size * block_index, SEEK_SET) != 0){
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
    float (*io_func) (int, ProgramOptions*, short);
    if(opts->no_caching)
        io_func = timed_linux_read_write;
    else
        io_func = timed_posix_read_write;
    // arrays where to save timings
    float *read_times = (float*) malloc(sizeof(float) * opts->block_count);
    float *write_times = (float*) malloc(sizeof(float) * opts->block_count);
    int num_writes = 0, num_reads = 0;
    if(!read_times || !write_times){
        fprintf(stderr, "Error while allocating memory.\n");
        exit(MEMORY_ERROR);
    }
    int current_block;
    const int blocks_in_file = opts->file_size / opts->block_size;
    // TODO: possible parallelisation here
    for(int count = 0; count < opts->block_count; count++){
        if(opts->random_access){
            current_block = rand() % blocks_in_file;
        }else{
            current_block = count % blocks_in_file;
        }
        // choose op: read or write?
        // TODO: set seed
        if(((float)rand())/RAND_MAX <= opts->read_prob){
            // it is a read op
            read_times[num_reads++] = io_func(current_block, opts, READ);
        }else{
            // it is a write op
            write_times[num_writes++]= io_func(current_block, opts, WRITE);
        }
    }

    // stats
    float block_size_in_mb = opts->block_size / (1024.0f * 1024);
    float avg_read_bandwidth = block_size_in_mb / mean(read_times, num_reads);
    float avg_write_bandwidth = block_size_in_mb / mean(write_times, num_writes);
    printf("Average read bandwidth: %.3f MB/s.\n", avg_read_bandwidth);
    printf("Average write bandwidth: %.3f MB/s.\n", avg_write_bandwidth);
    free(read_times);
    free(write_times);
}


