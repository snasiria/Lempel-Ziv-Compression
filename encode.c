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
        printf("SYNOPSIS:\n   Compresses files using the LZ78 compression algorithm.\n   "
               "Compressed files are decompressed with the corresponding decoder.\n\nUSAGE\n   "
               "./encode [-vh] [-i input] [-o output]\n\nOPTIONS\n  -h\t\t\tDisplay program help "
               "and usage.\n  -v\t\t\tDisplay compression statistics.\n  -i input\t\tSpecify input "
               "to compress (stdin by default)\n  -o output\t\tSpecify output of compressed input "
               "(stdout by default)\n");
        return 0;
    }

    // file containing message
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

    // file that will contain compressed file
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

    fstat(outfileFD, &header_stats);

    FileHeader *head = (FileHeader *) calloc(1, sizeof(FileHeader));
    head->magic = MAGIC;
    head->protection = header_stats.st_mode;

    write_header(outfileFD, head);
    free(head);

    TrieNode *root = trie_create(); // create trie with root trie code equal to EMPTY_CODE
    TrieNode *curr_node = root;
    TrieNode *prev_node = NULL;
    TrieNode *next_node = NULL;

    uint16_t next_code = START_CODE;
    uint8_t curr_sym = 0;
    uint8_t prev_sym = 0;
    uint8_t bitLen = 0; // bit length for next code

    // while there are more symbols to read
    while (read_sym(infileFD, &curr_sym)) {
        next_node = trie_step(curr_node, curr_sym);

        // if next_node exists, increment
        if (next_node != NULL) {
            prev_node = curr_node;
            curr_node = next_node;
        }
        // once all syms are in trie
        else {
            if (next_code >= 1)
                bitLen = log2(next_code) + 1;
            else
                bitLen = 1;

            write_pair(outfileFD, curr_node->code, curr_sym, bitLen); // write pair to outfile
            curr_node->children[curr_sym] = trie_node_create(next_code);
            curr_node = root;
            next_code++;
        }

        if (next_code == MAX_CODE) {
            trie_reset(root);
            curr_node = root;
            next_code = START_CODE;
        }
        prev_sym = curr_sym;
    }

    if (curr_node != root) {
        if (next_code >= 1)
            bitLen = log2(next_code) + 1;
        else
            bitLen = 1;
        write_pair(outfileFD, prev_node->code, prev_sym, bitLen);
        next_code = (next_code + 1) % MAX_CODE;
    }

    if (next_code >= 1)
        bitLen = log2(next_code) + 1;
    else
        bitLen = 1;

    write_pair(outfileFD, STOP_CODE, 0, bitLen);
    flush_pairs(outfileFD);

    close(infileFD);
    close(outfileFD);

    // verbose statistics for compression
    if (verbose) {
        uint64_t compressed_file_size = 0;

        if (total_bits % 8 == 0)
            compressed_file_size = ((total_bits / 8) + 8);
        else
            compressed_file_size = ((total_bits / 8) + 9);

        double space_saving = (double) compressed_file_size / total_syms;
        space_saving = 1 - space_saving;
        space_saving *= 100;

        printf("Compressed file size: %" PRIu64 " bytes\n", compressed_file_size);
        printf("Uncompressed file size: %" PRIu64 " bytes\n", total_syms);
        printf("Compression ratio: %.2f%%\n", space_saving);
    }

    trie_delete(root); // free memory by deleting root
    return 0;
}
