#include <stdio.h>
#include <stdlib.h>
#include <locale.h>
#include <time.h>
#include <pthread.h>
#include <stdbool.h>
#include <math.h>

#include "shared_region.h"

int numberOfThreads = 0;
int MIN_ORDER = 256;


int gaussianElimination( double **mat);
void processMatrix( double **buffer, INFOSFILE *partialInfo);





static void *worker(void *par) {
    /* worker id */
    unsigned int idthread = *((unsigned int *) par);

    double **buffer;
    INFOSFILE infos;

    /* while has matrices to process */
    while (getMatrix(idthread, &buffer, &infos, &MIN_ORDER) != 1) {
        processMatrix(buffer, &infos);
        saveResult(idthread, buffer, &infos);
    }

    int status = EXIT_SUCCESS;
    /* end thread*/
    pthread_exit(&status);
}

/** \brief main function */
int main(int argc, char *argv[]) {

    /* time limits */
    double t0, t1, tf;
    t0 = ((double) clock()) / CLOCKS_PER_SEC;

    /* number of workers to create */
    int numberOfThreads = atoi(argv[1]);

    /* file names */
    char *fileNames[argc - 2];

    for (int i = 0; i < argc; i++) fileNames[i] = argv[i + 2];

    pthread_t tIdWork[numberOfThreads];
    unsigned int works[numberOfThreads];

    int *status_p;

    for (int t = 0; t < numberOfThreads; t++)
        works[t] = t;

    /* allocate files */
    allocFiles(argc - 2, fileNames);

    printf("File names stored in the shared region.\n");

    /* create threads workers */
    for (int t = 0; t < numberOfThreads; t++) {
        if (pthread_create(&tIdWork[t], NULL, worker, &works[t]) != 0)
        {
            perror("error on creating thread worker");
            exit(EXIT_FAILURE);
        }
    }

    for (int t = 0; t < numberOfThreads; t++) {
        if (pthread_join(tIdWork[t], (void *) &status_p) != 0)
        {
            perror("error on waiting for thread worker");
            exit(EXIT_FAILURE);
        }
        printf("thread worker, with id %u, has terminated. \n", t);
    }
    /* print results */
    printResults();

    printf("Terminated.\n");

    t1 = ((double) clock()) / CLOCKS_PER_SEC;
    tf = t1 - t0;
    printf("\nElapsed time = %.6f s\n", tf);
    return 0;
}

/** \brief swap row i to j and vice-versa  */
void swap_row(double **mat, int i, int j) {
    for (int k = 0; k <= N; k++) {
        double temp = mat[i][k];
        mat[i][k] = mat[j][k];
        mat[j][k] = temp;
    }
}

/** \brief gaussian elimination process return { 1-singular matrix; 0-upper triangular} */
int gaussianElimination( double **mat) {
    for (int k = 0; k < N; k++) {
        int index_pivot = k;
        int value_pivot = mat[index_pivot][k];

        // update pivot if exists any bigger
        for (int i = k + 1; i < N; i++)
            if (fabs(mat[i][k]) > value_pivot) {
                value_pivot = mat[i][k];
                index_pivot = i;
            }

        //check if diagonal is zero (singular matrix)
        if (!mat[k][index_pivot])
            return 1;
        // swap rows
        if (index_pivot != k)
            swap_row(mat, k, index_pivot);

        // apply formula
        for (int i = k + 1; i < N; i++) {
             double aux = 0;
            if (mat[k][k] != 0){
                aux = mat[i][k] / mat[k][k];
            }
            for (int j = k; j < N; j++)
                mat[i][j] -= aux * mat[k][j];
        }
    }
    return 0;
}

void processMatrix( double **buffer, INFOSFILE *infos) {
    float res = 1;
    // gaussian elimination process
    int singular_matrix = gaussianElimination(buffer);

    // if matrix is singular
    if (singular_matrix)
        (*infos).determinants[(*infos).nextMatrixToProcessed] = 0;

    for (int i = 0; i < N; ++i) {
        res *= buffer[i][i];
    }
    (*infos).determinants[(*infos).nextMatrixToProcessed] = res;
    (*infos).nextMatrixToProcessed++;
}



