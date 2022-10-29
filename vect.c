/**
 * Vector implementation.
 *
 * - Implement each of the functions to create a working growable array (vector).
 * - Do not change any of the structs
 * - When submitting, You should not have any 'printf' statements in your vector 
 *   functions.
 *
 * IMPORTANT: The initial capacity and the vector's growth factor should be 
 * expressed in terms of the configuration constants in vect.h
 */
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "vect.h"

/** Main data structure for the vector. */
struct vect {
  char **data;             /* Array containing the actual data. */
  unsigned int size;       /* Number of items currently in the vector. */
  unsigned int capacity;   /* Maximum number of items the vector can hold before growing. */
};

/** Construct a new empty vector. */
vect_t *vect_new() {

  vect_t *v = malloc(sizeof(vect_t));
  (*v).data = malloc(sizeof(long) * VECT_INITIAL_CAPACITY);
  (*v).size = 0;
  (*v).capacity = VECT_INITIAL_CAPACITY;

  return v;
}

/** Delete the vector, freeing all memory it occupies. */
void vect_delete(vect_t *v) {
    // freeing each string first
    int index;
    for(index = 0; index < (*v).size; index++) {
      free((*v).data[index]);
    }
    // then freeing array pointer
    free((*v).data);
    // finally freeing vector struct
    free(v);
}

/** Get the element at the given index. */
const char *vect_get(vect_t *v, unsigned int idx) {
  assert(v != NULL);
  assert(idx < v->size);

  return (*v).data[idx];
}

/** Get a copy of the element at the given index. The caller is responsible
 *  for freeing the memory occupied by the copy. */
char *vect_get_copy(vect_t *v, unsigned int idx) {
  assert(v != NULL);
  assert(idx < v->size);

  char *stringCopy = malloc(sizeof(char) * (strlen((*v).data[idx]) + 1));
  strcpy(stringCopy, (*v).data[idx]);
  return stringCopy;
}

/** Set the element at the given index. */
void vect_set(vect_t *v, unsigned int idx, const char *elt) {
  assert(v != NULL);
  assert(idx < v->size);
  
  char *elementCopy = malloc(sizeof(char) * (strlen(elt) + 1));
  strcpy(elementCopy, elt);
  (*v).data[idx] = elementCopy;
}

/** Add an element to the back of the vector. */
void vect_add(vect_t *v, const char *elt) {
  assert(v != NULL);

  // if the size equals the capacity, then we need to use realloc to obtain a new array
  if ((*v).size == (*v).capacity) {
    (*v).data = realloc((*v).data, ((*v).capacity * sizeof(long)) * VECT_GROWTH_FACTOR);
    (*v).capacity = (*v).capacity * VECT_GROWTH_FACTOR;
  }

  char *elementCopy = malloc(sizeof(char) * (strlen(elt) + 1));
  strcpy(elementCopy, elt);
  (*v).data[(*v).size] = elementCopy;
  (*v).size += 1;
}

/** Remove the last element from the vector. */
void vect_remove_last(vect_t *v) {
  assert(v != NULL);

  free((*v).data[(*v).size - 1]);
  (*v).size -= 1;
}

/** The number of items currently in the vector. */
unsigned int vect_size(vect_t *v) {
  assert(v != NULL);

  return (*v).size;
}

/** The maximum number of items the vector can hold before it has to grow. */
unsigned int vect_current_capacity(vect_t *v) {
  assert(v != NULL);

  return (*v).capacity;
}
