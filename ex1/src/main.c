#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <wchar.h>
#include <locale.h>
#include <stdbool.h>
#include <libgen.h>
#include <getopt.h>
#include <time.h>
#include <pthread.h>

#include "shared_region.h"


int numberOfThreads = 0;
char *files_names[30];
int fileCounter = 0;
int MAX_SIZE_WORD = 50;
int MAX_BYTES_TO_READ = 12;

void processChar(char *buf, INFOSFILE *partialInfo);

/** \brief Worker */
static void *worker (void *par) {
    /* worker id */
    unsigned int idthread = *((unsigned int *) par);
    printf("Inside Worker ID: %d\n", idthread);

    char buffer[MAX_BYTES_TO_READ+MAX_SIZE_WORD];
    INFOSFILE infos;

    while (getChar(idthread, buffer, &infos) != 1) {
        processChar(buffer, &infos);
        saveResult(idthread, &infos);
    }
    int status = EXIT_SUCCESS;
    pthread_exit(&status);

}

/** \brief main function */
int main(int argc, char *argv[]){
    /* time limits */
    double t0, t1, tf;
    t0 = ((double) clock ()) / CLOCKS_PER_SEC;

    /* number of workers */
    int numberOfThreads = atoi(argv[1]);

    /* file names */
    char *fileNames[argc-2];

    /* get files names on arguments */
    for(int i=0; i<argc; i++) fileNames[i] = argv[i+2];


    pthread_t tIdWork[numberOfThreads];
    unsigned int works[numberOfThreads];

    int *status_p;

    for (int t=0; t<numberOfThreads; t++)
        works[t] = t;

    allocFiles(argc-2, fileNames);

    printf("File names stored in the shared region.\n");

    /* create threads  */
    for(int t=0; t<numberOfThreads; t++) {
        if (pthread_create (&tIdWork[t], NULL, worker, &works[t]) != 0)
        { perror ("error on creating thread worker");
            exit (EXIT_FAILURE);
        }
        printf("Thread Created: %d\n", t);
    }

    for(int t=0; t<numberOfThreads; t++)   {
        if (pthread_join (tIdWork[t], (void *) &status_p) != 0)
        { perror ("error on waiting for thread worker");
            exit (EXIT_FAILURE);
        }
        printf ("thread worker, with id %u, has terminated. \n", t);

    }

    /* print results */
    printProcessingResults();

    printf("Terminated.\n");

    t1 = ((double) clock ()) / CLOCKS_PER_SEC;
    tf = t1 - t0;
    printf ("\nElapsed time = %.6f s\n", tf);
    return 0;
}

/** \brief main process to chars, count all counters */
void processChar(char *buf, INFOSFILE *partialInfo) {

    int buf_size = 0;
    char c, last_char = ' ';

    /* get last char */
    while (buf[buf_size]!=NULL) {
        last_char = c;
        c = buf[buf_size];
        buf_size++;
    }

    /* if actual char is char to word */
    if (isCharToWord(c)) {
        //if it is out of Word
        if (!(*partialInfo).status_word) {
            (*partialInfo).status_word = true; // change to in word
            (*partialInfo).number_of_words++;
            if (isVogal(c)) {
                (*partialInfo).number_of_vowels_words++;
            }
        }
    } else {
        /* if char isn't char to word */
        /* if last char is char to word, means that is finish of word */
        if (isCharToWord(last_char)) {
            if (isConsonant(last_char))  (*partialInfo).number_of_consoants_words++;
        }
        /* change status to false (not in word)*/
        (*partialInfo).status_word = false;
    }
}

