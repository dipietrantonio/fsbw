#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include "program_options.h"
#include "common.h"

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
            default : {
                return -1;
            }
        }
    }
    return result;
}



void print_program_options(ProgramOptions* opts){
    printf("Program options:\n----------------\n\t");
    printf("Filename: %s\n\t", opts->filename);
    printf("File size: %ld bytes (%.2f MB)\n\t",  opts->file_size, opts->file_size / (1024.0f * 1024));
    printf("Block size: %d bytes (%.4f MB)\n\t", opts->block_size, opts->block_size / (1024.0f * 1024));
    printf("Block count: %d\n\t", opts->block_count);
    printf("Read probability: %0.2f\n\t", opts->read_prob);
    printf("Avoid caching: %s\n\t", opts->no_caching ? "yes" : "no");
    printf("Random access: %s\n\n", opts->random_access ? "yes" : "no");
}


void print_program_help(void){
    printf("Program help:\n----------------\n");
    printf("\t-f <filename>: path to the file to be read/written (if it does not exist, will be created).\n");
    printf("\t-B <block size>: size in bytes of each read/write operation. Can use K/M/G postfixes for Kilo/Mega/Giga multiplier, e.g. 32M.\n");
    printf("\t-c <block count>: number of blocks to read/write.\n");
    printf("\t-S <file size>: if the file does not exist, a file of size S bytes will be created.\n\t\tCan use K/M/G postfixes for Kilo/Mega/Giga multiplier, e.g. 32M.\n");
    printf("\t-p <read probability>: number in the [0, 1] interval indicating the probability the next operation to perform is a read operation.\n");
    printf("\t-r: enable random access to the file (default: file read/written sequentially).\n");
    printf("\t-n: avoid caching at all costs.\n");
}



void parse_program_options(int argc, char **argv, ProgramOptions* opts){
    const char *options = "S:B:c:p:f:nr";
    // TODO: add random option.
    int current_opt;
    // Set default options
    opts->filename = NULL;
    opts->block_size = parse_bytespec("4K");
    opts->file_size = parse_bytespec("512M");
    opts->read_prob = 0.5;
    opts->block_count = 0;
    opts->no_caching = 0;
    opts->random_access = 0;

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
            case 'n':{
                opts->no_caching = 1;
                break;
            }
            case 'r':{
                opts->random_access = 1;
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
    if(opts->block_count == 0)
        opts->block_count = opts->file_size / opts->block_size;
}
