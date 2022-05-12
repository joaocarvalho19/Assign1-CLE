#include <stdio.h>
#include <stdlib.h>
#include <locale.h>
#include <time.h>
#include <pthread.h>


#include "shared_region.h"

static void *worker(void *par);
void init();
void swap_cols(double *matrix, int *x, int *y, int size);
void formula(double *kj, double ki, double ii, double ij);
void free_memory();

static double **determinants;
static int nFiles = 0;
static int numberOfThreads = 0;
static pthread_t *workers;
static chunk_structure chunk;

/**
 * \brief Main Function 
 * 
 * \param argc 
 * \param argv 
 * \return int 
 */
int main(int argc, char *argv[])
{
    /* time limits */
    struct timespec start, finish;                                            /* time limits */
    clock_gettime (CLOCK_MONOTONIC_RAW, &start);                              /* begin of measurement */


    /* number of workers to create */
    numberOfThreads = atoi(argv[1]);

    /* file names */
    nFiles = argc - 2;
    static char *fileNames[10];


    for (int i = 0; i < nFiles; i++)
    {
        fileNames[i] = argv[i +2];
    }

    init();

    int matrices_size[nFiles];                                              //Initialization matrices info
    int matrices_order[nFiles];

    
    for (int worker_index = 0; worker_index < numberOfThreads; worker_index++)      //Create threads
    {
       if (pthread_create(&workers[worker_index], NULL, worker, NULL) != 0)
        {
            perror("error on creating thread worker");
            exit(EXIT_FAILURE);
        }
    }

    

    for (int file_id = 0; file_id < nFiles; file_id++)              //Loop for files
    {
        FILE *file = fopen(fileNames[file_id], "rb");               //Open file
        
        if (file == NULL)                                           // Check file
        {
            perror("File could not be read");
            exit(1);
        }

        int number_of_matrices, matrix_order;                       //Initialization variables for info files

        if (fread(&number_of_matrices, sizeof(int), 1, file) != 1)  //Read number of matrices
        {
            perror("Error reading number of matrices!");
            exit(1);
        }

        if (fread(&matrix_order, sizeof(int), 1, file) != 1)        //Read matrices orders
        {
            perror("Error reading matrices order!");
            exit(1);
        }

        determinants[file_id] = malloc(sizeof(double) * number_of_matrices);        //Allocation list of determinants  for file
        matrices_size[file_id] = number_of_matrices;                                //Save number of matrices for file
        matrices_order[file_id] = matrix_order;                                     //Save matrix orders for file

        for (int matrix_index = 0; matrix_index < number_of_matrices; matrix_index++)
        {
            matrix_structure *matrix = malloc(sizeof(matrix_structure));            //allocate space for matrix
            matrix->cells = malloc(sizeof(double) * matrix_order * matrix_order);   //allocate space for cell of matrix

            if (fread(matrix->cells, sizeof(double), matrix_order * matrix_order, file) != (matrix_order * matrix_order))
            {
                perror("Error reading values from matrix!\n");
                exit(2);
            }

            matrix->id = file_id;                                                   //Save infos for matrix
            matrix->matrix_id = matrix_index;
            matrix->order_matrix = matrix_order;                                    

            put_matrix(&chunk, matrix);                                             //Put matrix on chunk
        }

        fclose(file);
    }

    
    for (int i = 0; i < numberOfThreads; i++)                                       //Finish for all threads
    {
        matrix_structure *end = malloc(sizeof(matrix_structure));
        end->matrix_id = NO_MORE_DATA;
        put_matrix(&chunk, end);
    }

    
    for (int worker_index = 0; worker_index < numberOfThreads; worker_index++)      // Thread Join
    {
        if (pthread_join(workers[worker_index], NULL) != 0){
            perror("error on creating thread worker");
            exit(EXIT_FAILURE);
        }
    }
    
    for (int file = 0; file < nFiles; file++)                                       // print results
    {
        printf("Number of matrices to be read: %d\n", matrices_size[file]);
        printf("Matrices order: %d\n\n", matrices_order[file]);

        for (int mat_id = 0; mat_id < matrices_size[file]; mat_id++)
        {
            printf("Processing matrix %d\n", mat_id + 1);
            printf("The determinant is %.3e\n", determinants[file][mat_id]);
        }
        printf("\n\n");
    }

    printf("Terminated.\n");
    clock_gettime (CLOCK_MONOTONIC_RAW, &finish);                                /* end of measurement */
    printf ("\nElapsed tim = %.6f s\n",  (finish.tv_sec - start.tv_sec) / 1.0 + (finish.tv_nsec - start.tv_nsec) / 1000000000.0);

    free_memory();
    return 0;
}
/**
 * \brief Free memory to determinants and workers
 * 
 */
void free_memory()
{
    for (int i = 0; i < nFiles; i++)
    {
        free(determinants[i]);
    }
    free(workers);
    free(determinants);
}
/**
 * \brief  Allocation to workers and determinants results, and initialize shared region
 * 
 */
void init()
{
    workers = malloc(sizeof(pthread_t) * numberOfThreads);
    determinants = malloc(sizeof(double *) * nFiles);
    init_shared_region(&chunk);
}

/**
 * \brief Worker to calculate determinants
 * 
 * \param par 
 * \return void* 
 */
static void *worker(void *par)
{
    while (1)
    {
        
        matrix_structure *matrix = get_matrix(&chunk);                  // get next matrix

        if (matrix->matrix_id == NO_MORE_DATA)                          //If no more data, free matrix
        {
            free(matrix);
            break;
        }

        //determinant result initialization
        double result = 1;                                              // don't start with zero because the multiplication will wrong


        for (int x = 0; x < matrix->order_matrix; x++)                  //loop for lines
        {
            int diagonal = matrix->order_matrix * x + x;                // get diagonal to x line
            if (matrix->cells[diagonal] == 0)                           //check if diagonal is zero
            {
                for (int y = x + 1; y < matrix->order_matrix; y++)      //this next loop, will swap cols until diagonal not be zero
                {
                    if (matrix->cells[diagonal] != 0)
                    {
                        swap_cols(matrix->cells, &x, &y, matrix->order_matrix);      //Swap col x<->y
                        break;
                    }
                }
            }

            for (int y = matrix->order_matrix - 1; y > x - 1; y--)      //Apply  gauss elimination
            {
                for (int k = x + 1; k < matrix->order_matrix; k++)
                {
                    //Apply formula gauss elimination
                    formula(&matrix->cells[matrix->order_matrix * k + y], matrix->cells[matrix->order_matrix * k + x], matrix->cells[matrix->order_matrix * x + x], matrix->cells[matrix->order_matrix * x + y]);
                }
            }

            /*if after exchanging col and applying formula, the diagonal value is still zero,
            it is because the matrix is ​​a singular matrix and the determinant is zero*/
            if (matrix->cells[matrix->order_matrix * x + x] == 0)
                return 0;

            // do multiplication diagonal to determine determinant
            result *= matrix->cells[matrix->order_matrix * x + x];
        }


        determinants[matrix->id][matrix->matrix_id] = result;           //Save determinant result
        free(matrix->cells);                                            //Free cells of actual matrix
        free(matrix);                                                   //free matrix
    }
    return 0;
}
/**
 * @brief Swap columns x<->y of 'm', with 'size' order 
 * 
 * \param m 
 * \param x 
 * \param y 
 * \param size 
 */
void swap_cols(double *m, int *x, int *y, int size)
{
    //Swap both cols 
    // for all lines
    for (int i = 0; i < size; i++)
    {
        double aux = m[size * i + (*x)];                        // get first value
        m[size * i + (*x)] = m[size * i + (*y)];                // update next cell value with previous cell value
        m[size * i + (*y)] = aux;                               // update previous with next
    }
}

/**
 * @brief Gauss Elimination Formula  
 * 
 * \param kj 
 * \param ki 
 * \param ii 
 * \param ij 
 */
void formula(double *kj, double ki, double ii, double ij)
{
    // gauss elimination formula
    *kj -= ((ki / ii) * ij);
}