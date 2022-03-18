// Access File program
// Author: Cristian Di Pietrantonio (cdipietrantonio@pawsey.org.au)

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include <math.h>


#define PARSE_BUFFER_SIZE 512
#define PARSE_BUFFER_ERROR -2
#define GENERIC_ERROR -1
#define MEMORY_ERROR -3
#define OPTION_PARSING_ERROR -4
#define IO_ERROR -5


typedef struct {
    // size in bytes of each I/O operation.
    int block_size;
    // number of I/O operations (read or writes) to execute.
    int block_count;
    // file to operate on.
    char *filename;
    // size of the file to operate on.
    long long file_size;
    // probabilities that the next op is read, respectively.
    float read_prob;
} ProgramOptions;


// function prototypes - defined somwhere below the `main` function
void parse_program_options(int argc, char **argv, ProgramOptions* opts);
void create_file_if_not_exist(ProgramOptions *opts);
float run_sequential_experiment(ProgramOptions *opts);
float stddev(float *values, unsigned int n);
float variance(float *values, unsigned int n);
float mean(float *values, unsigned int n);



int main(int argc, char **argv){
    ProgramOptions opts;
    parse_program_options(argc, argv, &opts);
    create_file_if_not_exist(&opts);
    run_sequential_experiment(&opts);
    return 0;
}



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
    // TODO wrap around the file
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



//
// parse_bytespec
// Returns the number of bytes represented by the string argument. The string can be a sequence
// of digits optionally followed by one of the following unit of measures (K)ilo, (M)ega, (G)iga.
// Example: 1K -> 1024
//
long long parse_bytespec(const char * const spec){
    static char buffer[PARSE_BUFFER_SIZE];
    long long result;
    int len = strlen(spec);
    if(len == 0) return GENERIC_ERROR;
    int i = 0;
    while(i < len && isdigit(spec[i])){
        buffer[i] = spec[i];
        i++;       
    }
    if(i >= PARSE_BUFFER_SIZE - 1) return PARSE_BUFFER_ERROR;
    buffer[i] = '\0';
    result = atoll(buffer);
    if(i < len){
        // more characters to process - may be a Unit?
        switch(spec[i]){
            case 'K':{
                result *= 1024;
                break;
            }
            case 'M':{
                result *= 1024 * 1024;
                break;
            }
            case 'G':{
                result *= 1024 * 1024 * 1024;
                break;
            }
        }
    }
    return result;
}



void create_file_if_not_exist(ProgramOptions *opts){
    if(access(opts->filename, F_OK) == -1){
        // if the file does not exist, create the file
        FILE *f = fopen(opts->filename, "w");
        if(!f){
            fprintf(stderr, "Error creating the file %s.\n", opts->filename);
            exit(GENERIC_ERROR);
        }
        // and set it to the desired size
        if(ftruncate(fileno(f), opts->file_size)){
            fprintf(stderr, "Error while truncating the file %s.\n", opts->filename);
            fclose(f);
            exit(GENERIC_ERROR);
        }
        fclose(f);
    }else if(!access(opts->filename, R_OK | W_OK)){
        // file exists and can be read and written
        // then, just grab the file size
        FILE *f = fopen(opts->filename, "r+");
        if(!f){
            fprintf(stderr, "Error opening the file %s.\n", opts->filename);
            exit(GENERIC_ERROR);
        }
        if(fseek(f, 0, SEEK_END) != 0 || (opts->file_size = ftell(f)) == -1L){
            fprintf(stderr, "Error while getting the size of the file %s.\n", opts->filename);
            fclose(f);
            exit(GENERIC_ERROR);
        }
        if(opts->block_size > opts->file_size){
            fprintf(stderr, "Block size can't be larger than file size.\n");
            fclose(f);
            exit(OPTION_PARSING_ERROR);
        }
        fclose(f);
    }
}




void parse_program_options(int argc, char **argv, ProgramOptions* opts){
    const char *options = "S:B:c:p:f:";
    // TODO: add random option.
    int current_opt;
    // Set default options
    opts->filename = NULL;
    opts->block_size = parse_bytespec("4K");
    opts->file_size = parse_bytespec("512M");
    opts->read_prob = 0.5;
    opts->block_count = (int) (opts->file_size / opts->block_size);

    while((current_opt = getopt(argc, argv, options)) != - 1){
        switch(current_opt){
            case 'S': {
                opts->file_size = parse_bytespec(optarg);
                if(opts->file_size <= 0){
                    fprintf(stderr, "Non positive file size specified.\n");
                    exit(OPTION_PARSING_ERROR);
                }
                break;
            }
            case 'B' : {
                opts->block_size = parse_bytespec(optarg);
                if(opts->block_size <= 0){
                    fprintf(stderr, "Non positive block size specified.\n");
                    exit(OPTION_PARSING_ERROR);
                }
                break;
            }
            case 'c': {
                opts->block_count = atoi(optarg);
                if(opts->block_count <= 0){
                    fprintf(stderr, "Non positive block count specified.\n");
                    exit(OPTION_PARSING_ERROR);
                }
                break;
            }
            case 'f': {
                opts->filename = optarg;
                break;
            }
            case 'p': {
                opts->read_prob = atof(optarg);
                if(opts->read_prob < 0 || opts->read_prob > 1){
                    fprintf(stderr, "The probability must be between 0 and 1.\n");
                    exit(GENERIC_ERROR);
                }
                break;
            }
            default : {
                fprintf(stderr, "Unrecognised option: '%c'\n", optopt);
                exit(GENERIC_ERROR);
            }
        }
    }
    // options validation
    if(opts->filename == NULL){
        fprintf(stderr, "No filename specified.\n");
        exit(OPTION_PARSING_ERROR);
    } 
}



// define mean and stddev
float mean(float *values, unsigned int n){
    if(n == 0) return 0;
    float mean = 0;
    for(unsigned int i = 0; i < n; i++){
        mean += values[i];
    }
    return mean / n;
}


float variance(float *values, unsigned int n){
    if(n == 0) return 0;
    float var = 0;
    float mean_val = mean(values, n);
    for(unsigned int i = 0; i < n; i++){
        var += powf(mean_val - values[i], 2);
    }
    return var / n;
}

float stddev(float *values, unsigned int n){
    if(n == 0) return 0;
    return sqrt(variance(values, n));
}