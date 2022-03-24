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



float timed_linux_read_write(int block_index, ProgramOptions *opts, short is_read){
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




float timed_posix_read_write(int block_index, ProgramOptions *opts, short is_read){
    struct timespec begin, end;
    FILE *fp;
    if(is_read)
        fp = fopen(opts->filename, "r");
    else
        fp = fopen(opts->filename, "r+");
    if(!fp){
        fprintf(stderr, "timed_posix_read_write: error while opening the file.\n");
        exit(IO_ERROR);
    }
    if(fseek(fp, (long)opts->block_size * block_index, SEEK_SET) != 0){
        fprintf(stderr, "timed_posix_read_write: error while moving within the file.\n");
        fclose(fp);
        exit(IO_ERROR);
    }
    char *buffer = malloc(sizeof(char) * opts->block_size);
    if(!buffer){
        fprintf(stderr, "timed_posix_read_write: error while allocating memory.\n");
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
        fprintf(stderr, "timed_posix_read_write: error while writing/reading portion of the file.\n");
        exit(IO_ERROR);
    }
    free(buffer);
    fclose(fp);
    // compute elapsed time in seconds
    float elapsed_time = (end.tv_nsec - begin.tv_nsec) / 1000000000.0 + (end.tv_sec  - begin.tv_sec);
    return elapsed_time;
}



float run_experiment(ProgramOptions *opts){
    float (*io_func) (int, ProgramOptions*, short);
    if(opts->no_caching)
        io_func = timed_linux_read_write;
    else
        io_func = timed_posix_read_write;
    // arrays where to save timings
    float *read_bw = (float*) malloc(sizeof(float) * opts->block_count);
    float *write_bw = (float*) malloc(sizeof(float) * opts->block_count);
    int num_writes = 0, num_reads = 0;
    if(!read_bw || !write_bw){
        fprintf(stderr, "Error while allocating memory.\n");
        exit(MEMORY_ERROR);
    }
    int current_block;
    const int blocks_in_file = opts->file_size / opts->block_size;
    float block_size_in_mb = opts->block_size / (1024.0f * 1024);
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
            read_bw[num_reads++] =  block_size_in_mb/io_func(current_block, opts, READ);
        }else{
            // it is a write op
            write_bw[num_writes++] = block_size_in_mb/io_func(current_block, opts, WRITE);
        }
    }

    // print stats
    if(opts->json_output) printf("{");
    
    if(num_reads){
        float avg_read_bandwidth = mean(read_bw, num_reads);
        float rd_std = stddev(read_bw, num_reads);
        float min_read_bw = min(read_bw, num_reads);
        float max_read_bw = max(read_bw, num_reads);
        float nops = (float) num_reads / (num_reads + num_writes) * 100;
        if(opts->json_output){
            printf("\"reads\" : { \"min\" : %f, \"max\" : %f,  \"mean\" : %f, \"stddev\" : %f, \"nops\" : %f} %c", 
                min_read_bw, max_read_bw, avg_read_bandwidth, rd_std, nops, num_writes ? ',' : ' ');
        }else{
            printf("Read bandwidth - min: %.3f MB/s, max: %.3fMB/s, mean: %.3f MB/s, stddev: %.3f. (N. read ops: %.2f%%).\n", \
                min_read_bw, max_read_bw, avg_read_bandwidth, rd_std, nops);
        }
        
    }

    if(num_writes){
        float avg_write_bandwidth = mean(write_bw, num_writes);
        float wr_std = stddev(write_bw, num_writes);
        float min_write_bw = min(write_bw, num_reads);
        float max_write_bw = max(write_bw, num_reads);
        float nops = (float) num_writes / (num_reads + num_writes) * 100;
        if(opts->json_output){
            printf("\"writes\" : { \"min\" : %f, \"max\" : %f,  \"mean\" : %f, \"stddev\" : %f, \"nops\" : %f}", 
                min_write_bw, max_write_bw, avg_write_bandwidth, wr_std, nops);
        }else{
            printf("Write bandwidth - min: %.3f MB/s, max: %.3fMB/s, mean: %.3f MB/s, stddev: %.3f. (N. write ops: %.2f%%).\n", \
                min_write_bw, max_write_bw, avg_write_bandwidth, wr_std, nops);
        }
    }
    if(opts->json_output) printf("}\n");
    free(read_bw);
    free(write_bw);
}


