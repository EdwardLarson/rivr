#include "hashtable.h"

#include <malloc.h>

Hash_Table* create_htable(int initial_size){
    return create_htable(initial_size, default_hash);
}

Hash_Table* create_htable(int initial_size, long int (*hash_function) (byte*, int)){
    Hash_Table* htable = calloc(1, sizeof(htable));
    
    if (htable){
        htable->size = initial_size;
        htable->n_elem = 0;
        
        htable>buckets = calloc(initial_size, sizeof(Hash_Table_Bucket));
        htable->hash = hash_function;
    }
    
    return htable;
}

// resize a hash table to a new size
// returns 1 on success, 0 on failure
int resize_htable(Hash_Table* htable, int new_size){
    // ensure new size has enough space to contain all elements
    if (new_size < htable->n_elem) return 0;
    
    int old_size = htable->size;
    Hash_Table_Bucket* old_buckets = htable->buckets;
    
    htable->size = new_size;
    htable->buckets = calloc(new_size, sizeof(Hash_Table_Bucket));
    
    int i;
    for (i = 0; i < old_size; i++){
        if (!old_buckets[i].key) continue;
        
        int new_position = get_htable_position(htable, old_buckets[i].key, old_buckets[i].key_size);
        htable->buckets[new_position] = old_buckets[i];
    }
}

void f(){
    
}

// TO-DO: reimplement with linked lists
// Because what if a key that collided with a later key is removed?
// How will get_htable_position know that even though there is no key in that bucket spot, it should keep hashing to find the actual spot?

int get_htable_position(const Hash_Table* htable, const byte* key, int key_size){
    long int hash_val = htable->hash(key, key_size);
    
    int position = (int) hash_val % htable->size;
    
    // if there are collisions, rehash
    while (htable->buckets[position].key 
            && !data_equal(key, key_size, htable->buckets[position].key, htable->buckets[position].key_size)){
        hash_val ^= htable->hash(hash_val, sizeof(hash_val));
        hash_val ^= htable->hash(key, key_size / 2);
        
        position = (int) hash_val % htable->size;
    }
    
    return position;
}

void add_htable_value(Hash_Table* htable, const byte* key, int key_size, const byte* value, int data_size){
    
}

int remove_htable_value(Hash_Table* htable, const byte* key, int key_size){
    int position = get_htable_position(key, key_size);
    
    
}

int get_htable_value(const Hash_Table* htable, const byte* key, int key_size, byte** value, int* data_size){
    
}

long int default_hash(byte* bytes, int size){
    long int z = 0x0110101011100110;
	for (unsigned int i = 0; i < size; i++){
		z ^= (long int)(bytes[i])) ^ ((i + 1) * (i + 10));
		z *= ~z >> 5;
		z -= (i * 255) ^ (int)(*(bytes + i));
	}
	
	return z;
}

// returns 1 if the two data arrays are equal, 0 otherwise
int data_equal(const byte* data1, int data1_size, const byte* data2, int data2_size){
    if (data1_size != data2_size) return 0;
    
    int i;
    for (i = 0; i < data1_size; i++){
        if (data1[i] != data2[i]) return 0;
    }
    
    return 1;
}