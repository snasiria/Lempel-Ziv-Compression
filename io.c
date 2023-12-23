#include "io.h"
#include "code.h"
#include "endian.h"
#include <stdio.h>
#include <unistd.h>
#include <assert.h>
#include <math.h>

#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>

uint8_t symBuffer[BLOCK], pairBuffer[BLOCK];
uint16_t symIndex, symIndexSize, pairIndex, pairSize;
uint64_t total_syms, total_bits;

// Reads infile to buffer
int read_bytes(int infile, uint8_t *buf, int to_read) {

    ssize_t bytesRead, totalBytesRead;
    bytesRead = 0;
    totalBytesRead = 0;

    do {
        bytesRead = read(infile, buf, to_read);

        if (bytesRead == 0) // no more bytes to read
            break;
        totalBytesRead += bytesRead;
        to_read -= bytesRead;
    } while (to_read != 0);

    return (int) totalBytesRead;
}

// Writes buffer to outfile
int write_bytes(int outfile, uint8_t *buf, int to_write) {

    ssize_t bytesWritten, totalBytesWritten;
    bytesWritten = 0;
    totalBytesWritten = 0;

    do {
        bytesWritten = write(outfile, buf, to_write);

        if (bytesWritten == 0) // no more bytes to write
            break;

        totalBytesWritten += bytesWritten;
        to_write -= bytesWritten;
    } while (to_write != 0);

    return (int) (totalBytesWritten);
}

// Reads header file from buffer
void read_header(int infile, FileHeader *header) {
    uint8_t *buffer = (uint8_t *) header; // create a pointer of type uint8_t that points to header
    read_bytes(infile, buffer, sizeof(FileHeader)); // read bytes into header

    // make sure endianness of fields match
    if (big_endian()) {
        header->magic = swap32(header->magic);
        header->protection = swap16(header->protection);
    }

    assert(header->magic == MAGIC); // make sure header magic number is equal to magic
}

// Writes header file from buffer
void write_header(int outfile, FileHeader *header) {
    // make sure endianness of fields match
    if (big_endian()) {
        header->magic = swap32(header->magic);
        header->protection = swap16(header->protection);
    }

    uint8_t *buffer = (uint8_t *) header; // create a pointer of type uint8_t that points to header
    write_bytes(outfile, buffer, sizeof(FileHeader)); // write bytes into fileheader
}

// Read one symbol from buffer, refill buffer if full
bool read_sym(int infile, uint8_t *sym) {

    // fill up buffer for the first time
    if (symIndex == 0)
        symIndexSize = read_bytes(infile, symBuffer, BLOCK);

    // if the buffer position is at the end, refill buffer
    if (symIndex == symIndexSize) {

        // clear the buffer by setting values to null or 0
        for (int i = 0; i < symIndexSize; i++)
            symBuffer[i] = 0;

        symIndexSize = read_bytes(infile, symBuffer, BLOCK);
        symIndex = 0;
    }

    if (symIndexSize == 0) // if we reached EOF, return false
        return false;

    sym[0] = symBuffer[symIndex]; // fill sym

    // increment global fields
    total_syms++;
    symIndex++;

    return true;
}

// Writes LSB to MSB version of byte, considers only index number of indicies
uint64_t byte_flipper(uint64_t byte, uint8_t index) {

    uint64_t byteArr[index];
    uint64_t byte_copy = byte;

    // shift bit values LSB to MSB and calculate each value
    for (int i = 0; i < index; i++) {
        byte_copy = (1 & byte >> (index - 1 - i)); // checks if shifted bit is 1 or 0
        byteArr[i] = (byte_copy * pow(2, i)); // calculates its value based on reverse position
    }

    // add all the bit values
    uint64_t flippedByteValue = 0;
    for (int i = 0; i < index; i++)
        flippedByteValue += byteArr[i];

    return flippedByteValue;
}

// Converts decimal value of byte to binary, puts binary values into uint8_t array passed
void decimalToBinary(uint8_t *binaryArr, uint64_t num, int bitlen) {

    uint8_t index = 0;
    uint64_t num_copy = byte_flipper(num, bitlen); // flip bytes first to read from LSB to MSB

    // loop will read shifted MSB (rightmost) bit and add to binary array
    while (num_copy > 0) {
        if (num_copy & 1)
            binaryArr[index] = 1;
        else
            binaryArr[index] = 0;

        num_copy >>= 1;
        index++;
    }
}

// Takes bits starting from start to end index of byte, place its result in binaryArr
void bitExtractor(uint8_t *binaryArr, uint8_t byte, uint8_t start, uint8_t end) {
    uint8_t byteArr[8];

    // initialize all values of array to 0
    for (int i = 0; i < 8; i++)
        byteArr[i] = 0;

    decimalToBinary(byteArr, byte, 8);

    // store select bits in resultant array
    for (int i = 0; i < (end - start + 1); i++)
        binaryArr[i] = byteArr[i + start];
}

// Writes (code, symbol) pair to outfile
void write_pair(int outfile, uint16_t code, uint8_t sym, int bitlen) {

    uint8_t indexPos = (total_bits % 8); // index position in a byte (0-7)
    uint8_t flipCodeArr[bitlen]; // array for flipped bits of code

    // make sure all values are initalized to 0
    for (int i = 0; i < bitlen; i++)
        flipCodeArr[i] = 0;

    decimalToBinary(flipCodeArr, byte_flipper(code, bitlen),
        bitlen); // create binary array of flipped value for code

    uint8_t previousByteArr[8];

    // initalize all array values to 0
    for (int i = 0; i < 8; i++)
        previousByteArr[i] = 0;

    decimalToBinary(previousByteArr, byte_flipper(pairBuffer[pairIndex], 8), 8);

    uint8_t flipSymArr[8]; // array for flipped bits of sym

    // initalize all array values to 0
    for (int i = 0; i < 8; i++)
        flipSymArr[i] = 0;

    sym = byte_flipper(sym, 8);
    decimalToBinary(flipSymArr, sym, 8);

    uint8_t bufferPairSize = bitlen + 8;
    uint8_t pairArr[bufferPairSize];

    // make sure all values are initalized to 0
    for (int i = 0; i < bufferPairSize; i++)
        pairArr[i] = 0;

    // fill pair array with code array first
    for (int i = 0; i < bitlen; i++)
        pairArr[i] = flipCodeArr[i];

    // fill pair array with symbol array next
    for (int i = bitlen; i < bufferPairSize; i++)
        pairArr[i] = flipSymArr[i - bitlen];

    uint8_t currentBytePair = 0;
    uint8_t indexRemainder = (8 - indexPos); // how many times you shift bits right

    // calculate value of bits going into current Byte
    for (int i = 0; i < indexRemainder; i++)
        currentBytePair += pow(2, 7 - indexPos - i) * pairArr[i];

    // flip current byte
    uint8_t remainingBits = (bufferPairSize - indexRemainder);

    // how many full bytes we can write to
    uint8_t loopCalls = 0;
    if (remainingBits % 8 != 0)
        loopCalls = (remainingBits / 8) + 1;
    else
        loopCalls = (remainingBits / 8);

    loopCalls -= 1;

    uint8_t currentByte = byte_flipper(pairBuffer[pairIndex], 8);
    uint8_t finalByteValue = currentByte + currentBytePair;

    // flip the combined byte and put it in buffer
    pairBuffer[pairIndex] = byte_flipper(finalByteValue, 8);
    pairIndex++;

    // check if out of bounds
    if (pairIndex == BLOCK)
        flush_pairs(outfile);

    // write to buffer loopCalls number of full bytes
    for (int k = 0; k < loopCalls; k++) {
        uint8_t nextByte = 0;

        // calculate value of next byte
        for (int i = 0; i < 8; i++)
            nextByte += pow(2, (7 - i)) * pairArr[i + indexRemainder];

        pairBuffer[pairIndex] = byte_flipper(nextByte, 8);

        uint8_t nextByteArr[8];

        // initalize all array values to 0
        for (int i = 0; i < 8; i++)
            nextByteArr[i] = 0;

        decimalToBinary(nextByteArr, finalByteValue, 8);

        remainingBits -= 8;
        pairIndex++;

        // check if out of bounds
        if (pairIndex == BLOCK)
            flush_pairs(outfile);

        indexRemainder += 8;
    }

    uint8_t nextByte = 0;

    // calculate remaining bits for next byte
    for (int i = 0; i < remainingBits; i++)
        nextByte += pow(2, (7 - i)) * pairArr[i + indexRemainder];

    pairBuffer[pairIndex] = byte_flipper(nextByte, 8); // flip byte back to LSB to MSB
    total_bits += bufferPairSize;

    if (remainingBits == 8) {
        pairIndex++;

        if (pairIndex == BLOCK)
            flush_pairs(outfile);
    }
}

// Writes out any remaining pairs in buffer to outfile
void flush_pairs(int outfile) {
    if (pairIndex == BLOCK)
        write_bytes(outfile, pairBuffer, pairIndex); // write file buffer to outfile
    else
        write_bytes(outfile, pairBuffer, pairIndex + 1); // add stop code

    // reset the buffer
    for (int i = 0; i < BLOCK; i++)
        pairBuffer[i] = 0;

    pairIndex = 0;
}

// Reads (code, symbol) pair to infile
bool read_pair(int infile, uint16_t *code, uint8_t *sym, int bitlen) {
    // fill up buffer for the first time
    if (pairIndex == 0 && total_bits == 0)
        pairSize = read_bytes(infile, pairBuffer, BLOCK);

    if (pairSize == 0) // if there is nothing in file, return false
        return false;

    uint8_t indexPos = (total_bits % 8); // index position in a byte
    uint8_t bitlen_copy = bitlen;
    uint8_t firstByteSize = 0;

    // calculate how much of current byte to calculate
    if ((8 - indexPos) < bitlen)
        firstByteSize = 8 - indexPos;
    else
        firstByteSize = bitlen_copy;

    uint8_t codeArrBinary[bitlen];
    uint8_t codeFirstByte[8];

    // initalize all array values to 0
    for (int i = 0; i < 8; i++)
        codeFirstByte[i] = 0;

    bitExtractor(
        codeFirstByte, byte_flipper(pairBuffer[pairIndex], 8), indexPos, 7); // extract first byte

    if (bitlen_copy >= 8 || (indexPos + firstByteSize) >= 8) {
        pairIndex++;

        if (pairIndex == BLOCK) {

            for (int i = 0; i < BLOCK; i++)
                pairBuffer[i] = 0;

            pairSize = read_bytes(infile, pairBuffer, BLOCK);
            pairIndex = 0;

            if (pairSize == 0) // if we reached EOF, return false
                return false;
        }
    }

    total_bits += firstByteSize;
    indexPos = (total_bits % 8);
    bitlen -= firstByteSize;

    // initalize all array values to 0
    for (int i = 0; i < firstByteSize; i++)
        codeArrBinary[i] = codeFirstByte[i];

    // if there are more code bits to be extracted in other bytes, extract them
    while (bitlen > 0) {
        // if there is at least another full byte that contains bits for code
        if (bitlen >= 8) {
            uint8_t codeNextByte[8];

            // initalize all array values to 0
            for (int i = 0; i < 8; i++)
                codeNextByte[i] = 0;

            decimalToBinary(codeNextByte, byte_flipper(pairBuffer[pairIndex], 8), 8);

            for (int i = 0; i < 8; i++)
                codeArrBinary[i + firstByteSize] = codeNextByte[i];

            total_bits += 8;
            indexPos = (total_bits % 8);

            pairIndex++;

            if (pairIndex == BLOCK) {
                for (int i = 0; i < pairSize; i++)
                    pairBuffer[i] = 0;

                pairSize = read_bytes(infile, pairBuffer, BLOCK);
                pairIndex = 0;

                if (pairSize == 0) // if we reached EOF, return false
                    return false;
            }

            bitlen -= 8;
            firstByteSize += 8;
        }

        // if there is a last byte that contains partial bits for code
        else if (bitlen < 8) {
            uint8_t lastByteArr[bitlen];

            // initalize all array values to 0
            for (int i = 0; i < bitlen; i++)
                lastByteArr[i] = 0;

            bitExtractor(lastByteArr, byte_flipper(pairBuffer[pairIndex], 8), 0, bitlen);

            // add last byte to total code array
            for (int i = 0; i < bitlen; i++)
                codeArrBinary[i + firstByteSize] = lastByteArr[i];

            if (bitlen + indexPos >= 8)
                pairIndex++;

            if (pairIndex == BLOCK) {
                for (int i = 0; i < pairSize; i++)
                    pairBuffer[i] = 0;
                pairSize = read_bytes(infile, pairBuffer, BLOCK);
                pairIndex = 0;

                if (pairSize == 0) // if we reached EOF, return false
                    return false;
            }

            total_bits += bitlen;
            indexPos = (total_bits % 8);
            bitlen = 0;
        }
    }

    uint16_t codeVal = 0;

    // convert binary value of code to decimal
    for (int i = 0; i < bitlen_copy; i++)
        codeVal += codeArrBinary[i] * pow(2, bitlen_copy - i - 1);

    codeVal = byte_flipper(codeVal, bitlen_copy);

    // if the code is STOP_CODE, return false
    if (codeVal == STOP_CODE)
        return false;

    // store code value into code pointer
    *code = codeVal;

    uint8_t symArrBinary[8];

    // initalize all array values to 0
    for (int i = 0; i < 8; i++)
        symArrBinary[i] = 0;

    firstByteSize = 8 - indexPos;
    uint8_t symFirstByte[firstByteSize];

    // initalize all array values to 0
    for (int i = 0; i < 8; i++)
        symFirstByte[i] = 0;

    bitExtractor(
        symFirstByte, byte_flipper(pairBuffer[pairIndex], 8), indexPos, 7); // extract first byte

    // update variable counters
    total_bits += firstByteSize;
    indexPos = (total_bits % 8);
    pairIndex++;

    if (pairIndex == BLOCK) {
        for (int i = 0; i < pairSize; i++)
            pairBuffer[i] = 0;
        pairSize = read_bytes(infile, pairBuffer, BLOCK);
        pairIndex = 0;

        if (pairSize == 0)
            return false;
    }

    // add first byte to total sym array
    for (int i = 0; i < firstByteSize; i++)
        symArrBinary[i] = symFirstByte[i];

    // if there are more sym bits in the next byte, extract it
    if (firstByteSize < 8) {
        uint8_t secondByteSize = 8 - firstByteSize;
        uint8_t symSecondByte[secondByteSize];

        // initalize all array values to 0
        for (int i = 0; i < secondByteSize; i++)
            symSecondByte[i] = 0;

        bitExtractor(symSecondByte, byte_flipper(pairBuffer[pairIndex], 8), 0, secondByteSize);

        // add second byte to total sym array
        for (int i = 0; i < secondByteSize; i++)
            symArrBinary[i + firstByteSize] = symSecondByte[i];

        total_bits += secondByteSize;
        indexPos = (total_bits % 8);
    }

    uint8_t symVal = 0;

    // calculate final sym value
    for (int i = 0; i < 8; i++)
        symVal += symArrBinary[i] * pow(2, i);

    *sym = symVal;

    return true;
}

// Writes word's symbol to outfile
void write_word(int outfile, Word *w) {
    // if this word's syms won't fit in this buffer, write to outfile and empty the buffer
    if ((w->len + symIndex + 1) > BLOCK)
        flush_words(outfile);

    // write word's syms to buffer
    for (uint32_t i = 0; i < w->len; i++) {
        symBuffer[symIndex] = w->syms[i];
        symIndex++;
        total_syms++;
    }
}

// Writes word's sym to outfile and resets buffer
void flush_words(int outfile) {

    write_bytes(outfile, symBuffer, symIndex);

    // reset buffer
    for (int i = 0; i < BLOCK; i++)
        symBuffer[i] = 0;

    symIndex = 0;
}
