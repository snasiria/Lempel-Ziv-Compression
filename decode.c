#include "code.h"
#include "trie.h"
#include "word.h"
#include "io.h"

#include <inttypes.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <math.h>

#define OPTIONS "vhi:o:"

int main(int argc, char **argv) {
    int opt;
    bool verbose = false;
    bool help = false;

    char *input_file, *output_file;
    input_file = NULL;
    output_file = NULL;

    int optInd = optind + 1;

    // manages user inputs
    while ((opt = getopt(argc, argv, OPTIONS)) != -1) {
        switch (opt) {
        case 'v': {
            verbose = true;
            break;
        }

        case 'h': {
            help = true;
            break;
        }

        case 'i': {
            input_file = argv[optInd];
            break;
        }

        case 'o': {
            output_file = argv[optInd];
            break;
        }

        default: {
            help = true;
            break;
        }
        }
        optInd = optind + 1;
    }

    // usage message
    if (help == true) {
        printf("SYNOPSIS:\n   Decompresses files with the LZ78 decompression algorithm.\n   Used "
               "with files compressed with the corresponding encoder.\n\nUSAGE\n   ./decode [-vh] "
               "[-i input] [-o output]\n\nOPTIONS\n  -h\t\t\tDisplay program help and usage.\n  "
               "-v\t\t\tDisplay decompression statistics.\n  -i input\t\tSpecify input to "
               "decompress (stdin by default)\n  -o output\t\tSpecify output of decompressed input "
               "(stdout by default)\n");
        return 0;
    }

    // file containing compressed data
    int infileFD = 0; // defaults to stdin file descriptor

    // if input file provided, use that instead of stdin
    if (input_file != NULL) {
        infileFD = open(input_file, O_RDONLY, 0664); // open to read

        // if file doesn't exist in directory
        if (infileFD == -1) {
            printf("%s: No such file or directory\n", input_file);
            return 0;
        }
    }

    // file that will contain decompressed file
    int outfileFD = 1; // defaults to stdout file descriptor

    // if output file provided, use that instead of stdout
    if (output_file != NULL) {
        outfileFD = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0666); // open to write

        // if file doesn't exist in directory
        if (outfileFD == -1) {
            printf("%s: No such file or directory\n", output_file);
            return 0;
        }
    }

    struct stat header_stats;
    fstat(infileFD, &header_stats);

    FileHeader *head = (FileHeader *) calloc(1, sizeof(FileHeader));
    head->magic = MAGIC;
    head->protection = header_stats.st_mode;

    read_header(infileFD, head);
    free(head);

    WordTable *table = wt_create();
    uint8_t curr_sym = 0;
    uint16_t curr_code = 0;
    uint16_t next_code = START_CODE;
    uint8_t bitLen = 0;

    if (next_code >= 1)
        bitLen = log2(next_code) + 1;
    else
        bitLen = 1;

    // while there are more pairs to read, add to WordTable
    while (read_pair(infileFD, &curr_code, &curr_sym, bitLen)) {
        table[next_code] = word_append_sym(table[curr_code], curr_sym);
        write_word(outfileFD, table[next_code]);
        next_code++;

        // reset Wordtable if full
        if (next_code == MAX_CODE) {
            wt_reset(table);
            next_code = START_CODE;
        }
        bitLen = log2(next_code) + 1;
    }
    flush_words(outfileFD); // write out words that didn't completely fill buffer

    // verbose statistics for compression
    if (verbose) {
        uint64_t compressed_file_size = 0;
        uint64_t uncompressed_file_size = 0;

        if (total_bits % 8 == 0)
            compressed_file_size = ((total_bits / 8) + 9);
        else
            compressed_file_size = ((total_bits / 8) + 10);
        uncompressed_file_size = total_syms;

        double space_saving = (double) compressed_file_size / uncompressed_file_size;
        space_saving = 1 - space_saving;
        space_saving *= 100;

        printf("Compressed file size: %" PRIu64 " bytes\n", compressed_file_size);
        printf("Uncompressed file size: %" PRIu64 " bytes\n", uncompressed_file_size);
        printf("Compression ratio: %.2f%%\n", space_saving);
    }

    close(infileFD);
    close(outfileFD);
    wt_delete(table); // free memory by deleting wordtable

    return 0;
}
