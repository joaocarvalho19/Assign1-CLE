#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>

#include "shared_region.h"


/**
 * \brief Initialize chunk values and thread conditions 
 * 
 * \param chunk 
 */
void init_shared_region(chunk_structure *chunk)
{
  chunk->next_enter = 0;
  chunk->next_process = 0;
  chunk->counter = 0;

  pthread_mutex_init(&chunk->monitor, NULL);
  pthread_cond_init(&chunk->NotEmpty, NULL);
  pthread_cond_init(&chunk->NotFull, NULL);
}




/**
 * \brief Add matrix on chunk
 * 
 * \param chunk 
 * \param mat 
 */
void put_matrix(chunk_structure *chunk, matrix_structure *mat)
{
  if (pthread_mutex_lock(&chunk->monitor))                        //Enter monitor
  {
    perror("Failed to enter monitor mode");
    pthread_exit(NULL);
  }

  while (chunk->counter == CHUNK_SIZE)                            //Wait for chunk to have space, if chunk is full
  {
    if (pthread_cond_wait(&chunk->NotFull, &chunk->monitor))
    {
      perror("Failed to wait is not full condition");
      pthread_exit (NULL);
    }
  }

  unsigned int next = chunk->next_enter;                            //get index to add matrix on chunk
  chunk->matrices[next] = mat;                                      //Save matrix on chunk
  chunk->next_enter = (chunk->next_enter + 1) % CHUNK_SIZE;         //Update index to next matrix to enter
  chunk->counter++;                                                 //Add to counter

  if (pthread_cond_broadcast(&chunk->NotEmpty))                     //Inform that chunk is not empty
  {
    perror("Failed to broadcast is not empty condition");
    pthread_exit(NULL);
  }

  if (pthread_mutex_unlock(&chunk->monitor))                        //Exit monitor
  {
    perror("Failed to exit monitor mode");
    pthread_exit(NULL);
  }
}


/**
 * \brief Get the matrix object
 * 
 * \param chunk 
 * \return matrix_structure* 
 */
matrix_structure *get_matrix(chunk_structure *chunk)
{
  if (pthread_mutex_lock(&chunk->monitor))                            //Enter monitor
  {
    perror("Failed to enter monitor mode");
    pthread_exit(NULL);
  }

  while (chunk->counter == 0 )                                        //Wait for any matrix if chunk is empty
  {
    if (pthread_cond_wait(&chunk->NotEmpty, &chunk->monitor))
    {
      perror("Failed to wait is not empty condition");
      pthread_exit(NULL);
    }
  }

  matrix_structure *result = chunk->matrices[chunk->next_process];    //get matrix on chunk
  chunk->matrices[chunk->next_process] = NULL;                        //clean chunk matrix space
  chunk->next_process = (chunk->next_process + 1) % CHUNK_SIZE;       //update index to next matrix to exit/process
  chunk->counter--;                                                   //Subtract counter

  if (pthread_cond_broadcast(&chunk->NotFull))                        //Inform that chunk is not full
  {
    perror("Failed to broadcast is not full condition");
    pthread_exit(NULL);
  }

  if (pthread_mutex_unlock(&chunk->monitor))                          //Exit monitor
  {
    perror("Failed to exit monitor mode");
    pthread_exit(NULL);
  }

  return result;
}