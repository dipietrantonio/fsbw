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


void parse_program_options(int argc, char **argv, ProgramOptions* opts){
    const char *options = "S:B:c:p:f:";
    // TODO: add random option.
    int current_opt;
    // Set default options
    opts->filename = NULL;
    opts->block_size = parse_bytespec("4K");
    opts->file_size = parse_bytespec("512M");
    opts->read_prob = 0.5;
    opts->block_count = 0;

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
    if(opts->block_count == 0)
        opts->block_count = opts->file_size / opts->block_size;
}
