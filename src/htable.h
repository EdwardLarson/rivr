#include "vm.h"

//struct _Hash_Table_Bucket;
typedef struct _Hash_Table_Bucket{
    byte* key;
    byte* value;
    
    int key_size;
    int value_size;
    
} Hash_Table_Bucket;

typedef struct {
    int size;
    int n_elem;
    
    Hash_Table_Bucket* buckets;
    long int (*hash) (byte*, int);
} Hash_Table;

Hash_Table* create_htable(int initial_size
Hash_Table* create_htable(int initial_size, long int (*hash_function) (byte*, int));

int resize_htable(Hash_Table* htable, int new_size);
int get_htable_position(const Hash_Table* htable, const byte* key, int key_size);
void add_htable_value(Hash_Table* htable, const byte* key, int key_size, const byte* value, int data_size);
int remove_htable_value(Hash_Table* htable, const byte* key, int key_size);

int get_htable_value(const Hash_Table* htable, const byte* key, int key_size, byte** value, int* data_size);

long int default_hash(byte*, int);
int data_equal(const byte* data1, int data1_size, const byte* data2, int data2_size);