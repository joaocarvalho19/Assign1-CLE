#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>

#include "shared_region.h"



void init_shared_region(chunk_structure *chunk)
{
  chunk->next_enter = 0;
  chunk->next_process = 0;
  chunk->counter = 0;
  pthread_mutex_init(&chunk->mutex, NULL);
  pthread_cond_init(&chunk->NotEmpty, NULL);
  pthread_cond_init(&chunk->NotFull, NULL);
}




//called from main function
void put_matrix(chunk_structure *chunk, matrix_structure *mat)
{
  if (pthread_mutex_lock(&chunk->mutex))
  {
    perror("Failed to enter monitor mode");
    pthread_exit(NULL);
  }

  while (chunk->counter == CHUNK_SIZE)
  {
    if (pthread_cond_wait(&chunk->NotFull, &chunk->mutex))
    {
      perror("Failed to wait is not full condition");
    }
  }

  unsigned int idx = chunk->next_enter;
  chunk->matrices[idx] = mat;
  chunk->next_enter = (chunk->next_enter + 1) % CHUNK_SIZE;
  chunk->counter++;

  if (pthread_cond_broadcast(&chunk->NotEmpty))
  {
    perror("Failed to broadcast is not empty condition");
    pthread_exit(NULL);
  }

  if (pthread_mutex_unlock(&chunk->mutex))
  {
    perror("Failed to exit monitor mode");
    pthread_exit(NULL);
  }
}


//called from worker

matrix_structure *get_matrix(chunk_structure *chunk)
{
  if (pthread_mutex_lock(&chunk->mutex))
  {
    perror("Failed to enter monitor mode");
    pthread_exit(NULL);
  }

  while (chunk->counter == 0 )
  {
    if (pthread_cond_wait(&chunk->NotEmpty, &chunk->mutex))
    {
      perror("Failed to wait is not empty condition");
      pthread_exit(NULL);
    }
  }

  matrix_structure *result = chunk->matrices[chunk->next_process];
  chunk->matrices[chunk->next_process] = NULL;
  chunk->next_process = (chunk->next_process + 1) % CHUNK_SIZE;
  chunk->counter--;

  if (pthread_cond_broadcast(&chunk->NotFull))
  {
    perror("Failed to broadcast is not full condition");
    pthread_exit(NULL);
  }

  if (pthread_mutex_unlock(&chunk->mutex))
  {
    perror("Failed to exit monitor mode");
    pthread_exit(NULL);
  }

  return result;
}