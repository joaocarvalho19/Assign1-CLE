#ifndef SHARED_REGION_H
#define SHARED_REGION_H

#define NO_MORE_DATA -1
#define CHUNK_SIZE 10

typedef struct matrix_structure
{
  int id;
  int matrix_id;
  double *cells;
  int order_matrix;
} matrix_structure;

typedef struct chunk_structure
{
  matrix_structure *matrices[CHUNK_SIZE];
  unsigned int next_enter;
  unsigned int next_process;
  unsigned int counter;
  pthread_mutex_t monitor;
  pthread_cond_t NotEmpty;
  pthread_cond_t NotFull;
} chunk_structure;



void init_shared_region(chunk_structure *chunk);
void put_matrix(chunk_structure *chunk, matrix_structure *mat);
matrix_structure *get_matrix(chunk_structure *chunk);

#endif