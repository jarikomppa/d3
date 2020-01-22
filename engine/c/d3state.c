/*
Example implementation of state.
When integrating with a game engine, it's possible
that some kind of state system already exists,
in which case a custom implementation is needed.
Note that only the following need to be implemented:

void d3state_set(void *s, char *symbol);
void d3state_clear(void *s, char *symbol);
int d3state_get(void *s, char *symbol);
void d3state_setvalue(void *s, char *symbol, int value);
int d3state_getvalue(void *s, char *symbol);
*/
#include "d3.h"

/*
 * Hashmap implementation based on 
 * https://github.com/petewarden/c_hashmap/blob/master/hashmap.h
 * (public domain / cc0) by Elliot C Back and Pete Warden, simplified
 * for d3 use.
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define INITIAL_SIZE (256)
#define MAX_CHAIN_LENGTH (8)

 /* We need to keep keys and values */
typedef struct _hashmap_element {
	char* key;
	int in_use;
	int data;
} hashmap_element;

/* A hashmap has some maximum size and current size,
 * as well as the data to hold. */
typedef struct _hashmap_map {
	int table_size;
	int size;
	hashmap_element *data;
} hashmap_map;

/*
 * Return an empty hashmap, or NULL on failure.
 */
static void* hashmap_new() {
	hashmap_map* m = (hashmap_map*)malloc(sizeof(hashmap_map));

	m->data = (hashmap_element*)calloc(INITIAL_SIZE, sizeof(hashmap_element));

	m->table_size = INITIAL_SIZE;
	m->size = 0;

	return m;
}

/*
 * Hashing function for a string
 */
static unsigned int hashmap_hash_int(hashmap_map * m, char* keystring) {

	unsigned long hash = 5381;
	unsigned int c;
	while (*keystring)
	{
		c = *keystring;
		hash = (hash << 5) + hash + c;
		keystring++;
	}

	return hash % m->table_size;
}

/*
 * Return the integer of the location in data
 * to store the point to the item, or MAP_FULL.
 */
static int hashmap_hash(void* in, char* key) {
	int curr;
	int i;

	/* Cast the hashmap */
	hashmap_map* m = (hashmap_map *)in;

	/* If full, return immediately */
	if (m->size >= (m->table_size / 2)) return -1;

	/* Find the best index */
	curr = hashmap_hash_int(m, key);

	/* Linear probing */
	for (i = 0; i < MAX_CHAIN_LENGTH; i++) {
		if (m->data[curr].in_use == 0)
			return curr;

		if (m->data[curr].in_use == 1 && (strcmp(m->data[curr].key, key) == 0))
			return curr;

		curr = (curr + 1) % m->table_size;
	}

	return -1;
}

void hashmap_put(void* in, char* key, int value);

/*
 * Doubles the size of the hashmap, and rehashes all the elements
 */
static void hashmap_rehash(void* in) {
	int i;
	int old_size;
	hashmap_element* curr;

	/* Setup the new elements */
	hashmap_map *m = (hashmap_map *)in;
	hashmap_element* temp = (hashmap_element *)
		calloc(2 * m->table_size, sizeof(hashmap_element));

	/* Update the array */
	curr = m->data;
	m->data = temp;

	/* Update the size */
	old_size = m->table_size;
	m->table_size = 2 * m->table_size;
	m->size = 0;

	/* Rehash the elements */
	for (i = 0; i < old_size; i++) {
		if (curr[i].in_use == 0)
			continue;

		hashmap_put(m, curr[i].key, curr[i].data);
	}
	free(curr);
}

/*
 * Add data to the hashmap with some key
 */
static void hashmap_put(void* in, char* key, int value) {
	int index;
	hashmap_map* m;

	/* Cast the hashmap */
	m = (hashmap_map *)in;

	/* Find a place to put our value */
	index = hashmap_hash(in, key);
	while (index < 0) {
		hashmap_rehash(in);
		index = hashmap_hash(in, key);
	}

	/* Set the data */
	m->data[index].data = value;
	m->data[index].key = key;
	m->data[index].in_use = 1;
	m->size++;
}

/*
 * Get your data out of the hashmap with a key
 */
static int hashmap_get(void* in, char* key) {
	int curr;
	int i;
	hashmap_map* m;

	/* Cast the hashmap */
	m = (hashmap_map *)in;

	/* Find data location */
	curr = hashmap_hash_int(m, key);

	/* Linear probing, if necessary */
	for (i = 0; i < MAX_CHAIN_LENGTH; i++) {

		int in_use = m->data[curr].in_use;
		if (in_use == 1) {
			if (strcmp(m->data[curr].key, key) == 0) {
				return (m->data[curr].data);
			}
		}

		curr = (curr + 1) % m->table_size;
	}

	return 0;
}

/*
 * Remove an element with that key from the map
 */
static void hashmap_remove(void *in, char* key) {
	int i;
	int curr;
	hashmap_map* m;

	/* Cast the hashmap */
	m = (hashmap_map *)in;

	/* Find key */
	curr = hashmap_hash_int(m, key);

	/* Linear probing, if necessary */
	for (i = 0; i < MAX_CHAIN_LENGTH; i++) {

		int in_use = m->data[curr].in_use;
		if (in_use == 1) {
			if (strcmp(m->data[curr].key, key) == 0) {
				/* Blank out the fields */
				m->data[curr].in_use = 0;
				m->data[curr].data = 0;
				m->data[curr].key = NULL;

				/* Reduce the size */
				m->size--;
				return;
			}
		}
		curr = (curr + 1) % m->table_size;
	}
}

/* Deallocate the hashmap */
static void hashmap_free(void* in) {
	hashmap_map* m = (hashmap_map*)in;
	free(m->data);
	free(m);
}

/******************************************************************/
/******************************************************************/
/******************************************************************/
/******************************************************************/


void* d3state_alloc()
{
	return hashmap_new();
}

void d3state_free(void *s)
{
	hashmap_free(s);
}

int d3state_serialize(void *s, char *aFilename)
{
	return 0;
}

int d3state_deserialize(void *s, char *aFilename)
{
	return 0;
}

int d3state_serialize_size(void *s)
{
	return 0;
}

int d3state_serialize_mem(void *s, char *mem, int size)
{
	return 0;
}

int d3state_deserialize_mem(void *s, char *mem, int size)
{
	return 0;
}


void d3state_set(void *s, char *symbol)
{
	hashmap_put(s, symbol, 1);
}

void d3state_clear(void *s, char *symbol)
{
	hashmap_remove(s, symbol);
}

int d3state_get(void *s, char *symbol)
{
	return hashmap_get(s, symbol);
}

void d3state_setvalue(void *s, char *symbol, int value)
{
	hashmap_put(s, symbol, value);
}

int d3state_getvalue(void *s, char *symbol)
{
	return hashmap_get(s, symbol);
}

