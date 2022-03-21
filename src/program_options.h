#ifndef __PROGRAM_OPTIONS_H__
#define __PROGRAM_OPTIONS_H__

#define PARSE_BUFFER_SIZE 512

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
    // ignore cache
    int no_caching;
    // access file randomly
    int random_access;
} ProgramOptions;

void print_program_help(void);
void print_program_options(ProgramOptions* opts);
void parse_program_options(int argc, char **argv, ProgramOptions* opts);
#endif