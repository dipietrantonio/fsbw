// Access File program
// Author: Cristian Di Pietrantonio (cdipietrantonio@pawsey.org.au)
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include "common.h"
#include "program_options.h"
#include "experiments.h"

void create_file_if_not_exist(ProgramOptions *opts);



int main(int argc, char **argv){
    if(argc < 2){
        print_program_help();
        exit(0);
    }
    ProgramOptions opts;
    parse_program_options(argc, argv, &opts);
    create_file_if_not_exist(&opts);
    // option validation
    if(opts.block_size > opts.file_size){
        fprintf(stderr, "Block size can't be larger than file size.\n");
        exit(OPTION_PARSING_ERROR);
    }
    if(opts.block_count == 0)
        opts.block_count = opts.file_size / opts.block_size;
    
    if(opts.interpret_as_max){
        unsigned int blocks_in_file = opts.file_size / opts.block_size;
        if(blocks_in_file < opts.block_count) opts.block_count = blocks_in_file;
    }
    if(!opts.json_output)
        print_program_options(&opts);
    run_experiment(&opts);
    return 0;
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
        fclose(f);
    }
}




