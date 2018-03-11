#include <malloc.h>
#include <stdlib.h>

#define MAP_POST 0
#define MAP_PRE 1

#define STRING_NONE 0
#define STRING_MINNODES	1

struct Rivr_String_;
typedef struct Rivr_String_ {
	int length;
	
	// string object is either a single sequence of up to x chars
	// or it is a pointer to two halves of the string
	
	union {
		char seq[128];
		struct Rivr_String_* subseqs[2];
	} data;
	
} Rivr_String;

void string_destroy(Rivr_String* str);

// Creation
Rivr_String* string_create(int len, int flags);
Rivr_String* string_create_from_seq(const char* array, int arraylen, int flags);
Rivr_String* string_clone(const Rivr_String* str);
Rivr_String* string_rebalance(const Rivr_String* str);

void string_replace_seq(Rivr_String* str, int index, const char* seq, int len);
void string_replace_str(Rivr_String* str1, int index, const Rivr_String* str2);

void string_insert_seq(Rivr_String* str, int index, const char* seq, int len);
void string_insert_str(Rivr_String* str1, int index, const Rivr_String* str2);

void string_remove(Rivr_String* str, int begin, int end);

Rivr_String* string_substring(const Rivr_String* str, int begin, int end);

Rivr_String* string_merge(Rivr_String* str1, Rivr_String* str2);

unsigned int string_generate_hash(Rivr_String* str);

char string_char_at(const Rivr_String* str, int index);

char* string_flatten(const Rivr_String* str);
int string_flatten_subseq(const Rivr_String* str, char* array, int index, int arraylen);
const Rivr_String* string_get_leaf(const Rivr_String* str, int index, int* local_index);
int string_compare(const Rivr_String* str1, const Rivr_String* str2);

void string_print(const Rivr_String* str);


