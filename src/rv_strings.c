#include "rv_strings.h"

#define left_subseq(str) (str->data.subseqs[0])
#define right_subseq(str) (str->data.subseqs[1])
#define sequence(str) str->data.seq
#define is_leaf(str) (str->length <= 128)

// create an empty string tree 
Rivr_String* string_create(int len, int flags){
	if (len <= 0){
		return NULL;
	}if (len > 128){
		
		int len_left;
		
		if (flags == STRING_MINNODES){
			len_left = (len / 2) < 128 ? 128 : (len / 2) - ((len / 2) % 128);
		}else{
			len_left = len / 2;
		}
		
		int len_right = len - len_left;
		
		return string_merge(
			string_create(len_left, flags), 
			string_create(len_right, flags)
		);
		
	}else{
		
		Rivr_String* str = calloc(1, sizeof(Rivr_String));
		str->length = len;
		/*
		int i;
		for (i = 0; i < len; i++){
			sequence(str)[i] = '\0';
		}
		*/
		
		return str;
	}
}

Rivr_String* string_create_from_seq(const char* array, int arraylen, int flags){
	
	if (!array) return NULL;
	if (arraylen < 0) return NULL;
	
	Rivr_String* str = string_create(arraylen, flags);
	string_replace_seq(str, 0, array, arraylen);
	
	return str;
}

Rivr_String* string_clone(const Rivr_String* str){
	
	if (!str) return NULL;
	
	Rivr_String* str_copy = malloc(sizeof(Rivr_String));
	str_copy->length = str->length;
	
	if (is_leaf(str)){
		int i;
		for (i = 0; i < str->length; i++){
			sequence(str_copy)[i] = sequence(str)[i];
		}
	}else{
		left_subseq(str_copy) = string_clone(left_subseq(str));
		right_subseq(str_copy) = string_clone(right_subseq(str));
	}
	
	return str_copy;
}

int string_compare(const Rivr_String* str1, const Rivr_String* str2){
	
	if (str1->length != str2->length){
		return str1->length - str2->length;
	}
	
	return -1;
}

Rivr_String* string_rebalance(const Rivr_String* str){
	if (!str) return NULL;
	
	Rivr_String* rebalanced = string_create(str->length, STRING_NONE);
	
	string_replace_str(rebalanced, 0, str);
	
	return rebalanced;
}

Rivr_String* string_merge(Rivr_String* str1, Rivr_String* str2){
	
	if (!str1 || !str2) return NULL;
	
	Rivr_String* new_parent = malloc(sizeof(Rivr_String));
	new_parent->length = str1->length + str2->length;
	
	if ( new_parent->length <= 128){
		int i;
		int parent_i;
		
		for (i = 0; i < str1->length; i++){
			sequence(new_parent)[parent_i] = sequence(str1)[i];
			parent_i++;
		}
		for (i = 0; i < str2->length; i++){
			sequence(new_parent)[parent_i] = sequence(str2)[i];
			parent_i++;
		}
	}else{
		left_subseq(new_parent) = str1;
		right_subseq(new_parent) = str2;
	}
	
	return new_parent;
}

void string_destroy(Rivr_String* str){
	
	if (!str) return;
	
	if (str->length > 128){
		string_destroy(left_subseq(str));
		string_destroy(right_subseq(str));
	}
	
	free(str);
}

char string_char_at(const Rivr_String* str, int index){
	int local_index;
	const Rivr_String* leaf_node = string_get_leaf(str, index, &local_index);
	
	return sequence(leaf_node)[local_index];
}

// get the struct in which a given index actually starts
const Rivr_String* string_get_leaf(const Rivr_String* str, int index, int* local_index){
	
	if (!str) return NULL;
	if (index < 0) return NULL;
	
	// case 1: str is a single-sequence string, index must be stored here
	if (is_leaf(str)){
		if (str->length > index){
			if (local_index) *local_index = index;
			return str;
		}else{
			return NULL; // index is somehow out of bounds
		}
	}
	else{ // case 2: str has two sub-sequences
		int left_subseq_len = left_subseq(str)->length;
		
		if (index < left_subseq_len){
			return string_get_leaf(left_subseq(str), index, local_index);
		}else{
			return string_get_leaf(right_subseq(str), index - left_subseq_len, local_index);
		}
	}
}

// load a sequence from a string into an array at an index
// Rivr_String str is assumed to be single-sequence
int string_load_seq(const Rivr_String* str, char* array, int index, int arraylen){
	if (!str || !array) return 0;
	if (arraylen <= 0 || index < 0) return 0;
	
	int leaf_i = 0;
	
	while (leaf_i < str->length && index < arraylen){
		array[index] = sequence(str)[leaf_i];
		leaf_i++;
		index++;
	}
	
	return index;
}

int string_flatten_subseq(Rivr_String* str, char* array, int index, int arraylen){
	if (!str || !array) return 0;
	if (arraylen <= 0 || index < 0) return 0;
	
	if (is_leaf(str)){
		// copy the base sequence into the array
		return string_load_seq(str, array, index, arraylen);
		
	}else{
		// flatten each subsequence of this string
		index = string_flatten_subseq(left_subseq(str), array, index, arraylen);
		return string_flatten_subseq(right_subseq(str), array, index, arraylen);
	}
}

// flatten a distributed river string into one contiguous array
char* string_flatten(Rivr_String* str){
	if (!str) return NULL;
	
	char* array = malloc (sizeof(char) * str->length);
	
	string_flatten_subseq(str, array, 0, str->length);
	
	return array;
}

// replace part of a string with a sequence of characters
void string_replace_seq(Rivr_String* str, int index, const char* seq, int len){
	
	if (!str || !seq) return;
	if (len <= 0 || index < 0) return;
	
	
	if(is_leaf(str)){
		// copy seq into this leaf
		int i;
		for (i = 0; i < len; i++){
			sequence(str)[i + index] = seq[i];
		}
	}else{
		if (index >= left_subseq(str)->length){
			// only need to go into right subseq
			string_replace_seq(right_subseq(str), index - left_subseq(str)->length, seq, len);
			
		}else if (index + len <= left_subseq(str)->length){
			// only need to go into left subseq
			string_replace_seq(left_subseq(str), index, seq, len);
		}else{
			string_replace_seq(left_subseq(str), index, seq, left_subseq(str)->length - index);
			string_replace_seq(right_subseq(str), 0, &seq[left_subseq(str)->length - index], len - left_subseq(str)->length + index);
		}
	}
}

void string_replace_str(Rivr_String* str1, int index, const Rivr_String* str2){
	if (str2->length + index > str1->length){
		return;
	}
	
	if (is_leaf(str2)){
		// write to str1
		string_replace_seq(str1, index, sequence(str2), str2->length);
	}else{
		string_replace_str(str1, index, left_subseq(str2));
		string_replace_str(str1, index + left_subseq(str2)->length, right_subseq(str2));
	}
}

// insert a character sequence into a string at a given index
void string_insert_seq(Rivr_String* str, int index, const char* seq, int len){
	
	if (!str || !seq) return;
	if (len <= 0 || index < 0) return;
	
	if (is_leaf(str)){
		if (str->length + len <= 128){
			// insert seq into existing string node
			
			// move existing sequence to make room for insert
			int i;
			for (i = 0; i < len; i++){
				sequence(str)[i + index + len] = sequence(str)[i + index];
			}
			
			// copy sequence into node
			for (i = 0; i < len; i++){
				sequence(str)[i + index] = seq[i];
			}
			
		}else{
			
			// create new dummy node to point to rearranged string data
			Rivr_String* new_node = string_create(str->length + len, STRING_NONE);
			
			// map first portion of original string onto new_node
			string_replace_seq(new_node, 0, sequence(str), index);
			
			// map inserted sequence onto new_node, with offset
			string_replace_seq(new_node, index, seq, len);
			
			// map second portion of original string on new_node, with offset
			string_replace_seq(new_node, index + len, &sequence(str)[index], str->length - index);
			
			left_subseq(str) = left_subseq(new_node);
			right_subseq(str) = right_subseq(new_node);
			free(new_node);
		}
		
		str->length += len;
	}else{
		int left_subseq_len = left_subseq(str)->length;
		
		if (index < left_subseq_len){
			string_insert_seq(left_subseq(str), index, seq, len);
			
			str->length += len;
		}else{
			string_insert_seq(right_subseq(str), index - left_subseq_len, seq, len);
			
			str->length += len;
		}
	}
}

void string_insert_str(Rivr_String* str1, int index, const Rivr_String* str2){
	return;
}

void string_remove(Rivr_String* str, int begin, int end){
	return;
}

Rivr_String* string_substring(const Rivr_String* str, int begin, int end){
	return string_create(1, STRING_NONE);
}
