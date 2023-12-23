#include "word.h"
#include "code.h"
#include <stdio.h>
#include <stdlib.h>

// Constructor for a new word
Word *word_create(uint8_t *syms, uint32_t len) {

    Word *newWord = (Word *) calloc(1, sizeof(Word)); // allocate memory to new Word
    newWord->syms = (uint8_t *) calloc(len, sizeof(uint8_t)); // allocate memory to sym array

    newWord->len = len;

    // copy ASCII value symbols from parameter onto new word
    for (uint32_t i = 0; i < len; i++)
        newWord->syms[i] = syms[i];

    if (newWord == NULL)
        return NULL;

    return newWord;
}

// Add parameter sym to the end of word
Word *word_append_sym(Word *w, uint8_t sym) {
    uint8_t *newTempSym = (uint8_t *) calloc(w->len + 1, sizeof(uint8_t));

    // copy ASCII value symbols from old word onto new word
    for (uint32_t i = 0; i < w->len; i++)
        newTempSym[i] = w->syms[i];

    newTempSym[w->len] = sym; // append by placing sym into length index - 1

    Word *wordCopy = word_create(newTempSym, (w->len + 1)); // create a newWord copy with len+1
    free(newTempSym);

    return wordCopy;
}

// Destructor for a Word
void word_delete(Word *w) {
    // set all values to 0 and null, then free their pointers
    w->len = 0;
    free(w->syms);
    w->syms = NULL;

    free(w);
    w = NULL;
}

// Constructor for WordTable
WordTable *wt_create(void) {
    WordTable *words = (WordTable *) calloc(MAX_CODE, sizeof(Word)); // initalize new word table
    words[EMPTY_CODE] = word_create(NULL, 0); // index EMPTY_CODE is empty word

    return words;
}

// Resets Wordtable to only contain empty word
void wt_reset(WordTable *wt) {
    // iterate from index after EMPTY_CODE
    for (int i = EMPTY_CODE + 1; i < MAX_CODE; i++) {
        // if word exists
        if (wt[i] != NULL) {
            word_delete(wt[i]);
            wt[i] = NULL;
        }
    }
}

// Destructor for WordTable
void wt_delete(WordTable *wt) {
    // delete all words and free memory
    for (int i = 0; i < MAX_CODE; i++) {
        if (wt[i] != NULL) {
            word_delete(wt[i]);
            wt[i] = NULL;
        }
    }

    free(wt);
}
