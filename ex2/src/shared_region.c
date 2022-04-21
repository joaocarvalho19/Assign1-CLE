#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdbool.h>
#include <string.h>
#include <locale.h>

typedef struct {
    int fileId;
    int numberOfMatrices;
    int orders;
    long double *determinants; /* results array */
    int nextMatrixToProcessed; /* when this value is equal numberOfMatrices, means thats end of file, in no more matrices*/
    bool done;
} INFOSFILE;

static long pos;
static INFOSFILE * infofile;
static pthread_mutex_t accessCR = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t store;
static pthread_cond_t process;
static bool stored;
static bool processed;
static pthread_once_t init = PTHREAD_ONCE_INIT;

static char ** fileNamesForRegion;
static int nFiles;
bool firstProcessing = true;
int fileInProcessing = 0;
int M;
int N;

void destroy_matrix(double** matrix);
void printMatrix(double** mat);

/** \brief initialize conditions for pthread */
static void initialization (void) {
    processed = false;
    stored = false;

    /* initialises the conditions */
    pthread_cond_init (&process, NULL);
    pthread_cond_init (&store, NULL);

    setlocale(LC_CTYPE, "");
}

/** \brief allocate files */
void allocFiles(int numberOfFiles, char *fileNames[]) {

    /* Enter monitor*/
    if ((pthread_mutex_lock (&accessCR)) != 0) {
        perror ("error on entering monitor(CF)");
        int status = EXIT_FAILURE;
        pthread_exit(&status);
    }
    /*update nFiles*/
    nFiles = numberOfFiles;
    /* memory allocation for files names*/
    fileNamesForRegion = malloc(nFiles * sizeof(char*));
    /* memory allocation for information */
    infofile = (INFOSFILE *)malloc(sizeof(INFOSFILE) * nFiles);

    /* allocate names of files */
    for (int i=0; i<numberOfFiles; i++) {
        fileNamesForRegion[i] = malloc((12) * sizeof(char));
        strcpy(fileNamesForRegion[i], fileNames[i]);
    }

    pthread_once (&init, initialization);

    /* Exit monitor */
    if ((pthread_mutex_unlock (&accessCR)) != 0) {
        perror ("error on exiting monitor(CF)");
        int status = EXIT_FAILURE;
        pthread_exit(&status);
    }

}

/** \brief return 0 if has new matrix to process, return 1 if hasn't more matrix on files */
int getMatrix(int threadId, long double*** buffer, INFOSFILE *partialInfo, int *size) {

    /* Enter monitor */
    if ((pthread_mutex_lock (&accessCR)) != 0) {
        perror ("error on entering monitor(CF)");
        int status = EXIT_FAILURE;
        pthread_exit(&status);
    }

    /* if no more data to process in current file */
    if (infofile[fileInProcessing].done) {
        /* if current file is the last file to be processed */
        if (fileInProcessing == nFiles - 1) {
            /* exit monitor */
            if ((pthread_mutex_unlock (&accessCR)) != 0) {
                perror ("error on exiting monitor(CF)");
                int status = EXIT_FAILURE;
                pthread_exit(&status);
            }
            /* finalize while worker */
            return 1;
        }
        /* if exist more files to process, change to next file*/
        /* next file to process */
        fileInProcessing++;
        firstProcessing = true;
    }

    /* no need to wait in the first processing as there is no values to be stored */
    if (firstProcessing == false) {
        /* wait if the previous partial info was no stored */
        while (stored==false) {
            if ((pthread_cond_wait (&store, &accessCR)) != 0) {
                perror ("error on waiting in fifoEmpty");
                int status = EXIT_FAILURE;
                pthread_exit (&status);
            }
        }
    }

    /* open binary file */
    FILE *f = fopen(fileNamesForRegion[fileInProcessing], "rb+");

    /* check file */
    if(f == 0) {
        perror("fopen");
        exit(1);
    }

    /* if first time processing the current file */
    if (firstProcessing) {
        fread(&M,sizeof(M),1,f);
        fread(&N,sizeof(N),1,f);
        pos = ftell(f);

        /* initialize struct with new file */
        infofile[fileInProcessing].fileId = fileInProcessing;
        infofile[fileInProcessing].nextMatrixToProcessed = 0;
        infofile[fileInProcessing].numberOfMatrices = M;
        infofile[fileInProcessing].orders = N;
        infofile[fileInProcessing].done = false;
        infofile[fileInProcessing].determinants = (long double *)calloc(M, sizeof(long double *));
        for (int j = 0; j<M; j++){
            infofile[fileInProcessing].determinants[j] = 0;
        }
        *size = N;
    }

    /*create matrix*/
    *buffer = malloc(sizeof(long double*)*N);
    for (size_t i = 0; i < (size_t) N; i++) {
        *(*buffer+i) = malloc(sizeof(long double)*N);
        memset(*(*buffer+i), 0, sizeof(long double)*N);
    }

    /* If is last matrix on the file, update done variable */
    if (infofile[fileInProcessing].nextMatrixToProcessed +1 == infofile[fileInProcessing].numberOfMatrices ){
        infofile[fileInProcessing].done = true;
    }

    /* go to last position readed on the file*/
    if (!firstProcessing) fseek(f, pos, SEEK_SET );
    /* If it is first processing change the value */
    if (firstProcessing) firstProcessing = false;

    /* Read line by line next matrix and update buffer */
    for (int i = 0; i < N; ++i) {
        fread(*(*buffer+i), sizeof(long double) *N, 1, f);
    }

    /* update pos to last position readed */
    pos = ftell(f);

    /* close opened file*/
    fclose(f);

    /* Update partial info */
    *partialInfo = infofile[fileInProcessing];

    processed = true;

    /* inform thread it is processed */
    if ((pthread_cond_signal (&process)) != 0) {
        perror ("error on waiting in fifoEmpty");
        int status = EXIT_FAILURE;
        pthread_exit (&status);
    }
    stored = false;

    /* Exit monitor */
    if ((pthread_mutex_unlock (&accessCR)) != 0) {
        perror ("error on exiting monitor(CF)");
        int status = EXIT_FAILURE;
        pthread_exit(&status);
    }

    return 0;
}

/** \brief Save partial info */
void saveResult(int threadId, double** buffer, INFOSFILE *infos) {
    /* Enter monitor */
    if ((pthread_mutex_lock (&accessCR)) != 0) {
        perror ("error on entering monitor(CF)");
        int status = EXIT_FAILURE;
        pthread_exit(&status);
    }

    /* Wait for process */
    while (processed == false) {
        if ((pthread_cond_wait (&process, &accessCR)) != 0) {
            perror ("error on waiting in fifoEmpty");
            int status = EXIT_FAILURE;
            pthread_exit (&status);
        }
    }

    /* Save Result */
    infofile[fileInProcessing] = *infos;

    /* destroy buffer */
    destroy_matrix(buffer);

    stored = true;

    /* Inform thread it is stored */
    if ((pthread_cond_signal (&store)) != 0) {
        perror ("error on waiting in fifoEmpty");
        int status = EXIT_FAILURE;
        pthread_exit (&status);
    }
    processed = false;

    /* Exit monitor */
    if ((pthread_mutex_unlock (&accessCR)) != 0) {
        perror ("error on exiting monitor(CF)");
        int status = EXIT_FAILURE;
        pthread_exit(&status);
    }

}

/** \brief Show matrix */
void printMatrix(double** mat) {
    for (int i = 0; i < N; ++i) {
        for (int j = 0; j < N; ++j) {
            printf("%f\t", mat[i][j]);
        }
        printf("\n");
    }
}

/** \brief Free matrix */
void destroy_matrix(double** matrix) {
    for (size_t i = 0; i < (size_t) N; i++)
        free(matrix[i]);
    free(matrix);
}

/** \brief Show all final results */
void printResults() {
    /* enter monitor */
    if ((pthread_mutex_lock (&accessCR)) != 0) {
        perror ("error on entering monitor(CF)");
        int status = EXIT_FAILURE;
        pthread_exit(&status);
    }

    printf("\n\nFiles counter: %d", nFiles);
    /* for each partial file info */
    for (int i=0; i<nFiles; i++) {
        printf("\nFile name: %s\n", fileNamesForRegion[i]);
        printf("File ID = %d\n", infofile[i].fileId);
        printf("Matrizes processadas = %d\n", infofile[i].nextMatrixToProcessed);
        printf("Número de matrizes originais = %d\n", infofile[i].numberOfMatrices);
        printf("Ordem da matriz = %d\n", infofile[i].orders);
        for (int j = 0; j < infofile[i].numberOfMatrices; ++j) {
            printf("\tDeterminante da Matriz %d : %LE\n", j, infofile[i].determinants[j]);
        }
    }

    /* exit monitor */
    if ((pthread_mutex_unlock (&accessCR)) != 0) {
        perror ("error on exiting monitor(CF)");
        int status = EXIT_FAILURE;
        pthread_exit(&status);
    }

}


