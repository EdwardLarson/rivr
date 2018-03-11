#include <stdio.h>
#include <string.h>
#include "../src/rv_strings.h"
#include "test_tools.h"

#define LENGTH1 382
const char test_seq1[383] = "The Starry Mountains to the east of the Dawn Desert divide that dry, dark region from the lush, verdant jungles and forests of the West Cornucopia. The mountains stand at the intersection of cultures: the wild Dawn Deserters to the west; the civilized men of the Cornucopia to the East; the zealous Nightborn who are native to the high valleys; the primitive shepherds to the south.";

#define LENGTH2 11
const char test_seq2[12] = "Lorem ipsum";

#define LENGTH3 128
const char test_seq3[129] = "The woods throughout the northeast are young and crisscrossed with low stone walls that give evidence that someone farmed here. ";

int is_valid_string_structure(const Rivr_String* str){
	
	if (str == NULL){
		return 0;
		
	}else if (str->length <= 0){
		return 0;
		
	}else if(str->length <= 128){
		return 1;
		
	}else{
		if (str->data.subseqs[0] == NULL || str->data.subseqs[0] == NULL){
			return 0;
		}
		
		int length_match = str->length == str->data.subseqs[0]->length + str->data.subseqs[1]->length;
		
		if (length_match){
			return is_valid_string_structure(str->data.subseqs[0]) && is_valid_string_structure(str->data.subseqs[1]);
		}else{
			return 0;
		}
	}
}

int count_string_nodes(const Rivr_String* str){
	
	if (str->length > 128){
		return 1 + count_string_nodes(str->data.subseqs[0]) + count_string_nodes(str->data.subseqs[1]);
		
	}else{
		return 1;
		
	}
}

void string_print_metadata(const Rivr_String* str, int indent){
	
	int i = 0;
	for (i = 0; i < indent; i++){
		putc('\t', stdout);
	}
	
	printf("[length: %d]", str->length);
	
	if (str->length > 128){
		printf("[\n");
		string_print_metadata(str->data.subseqs[0], indent + 1);
		string_print_metadata(str->data.subseqs[1], indent + 1);
		
		for (i = 0; i < indent; i++){
			putc('\t', stdout);
		}
		printf("]\n");
	}
	else{
		printf("[");
		int j;
		for (j = 0; j < str->length; j++){
			putc(str->data.seq[j], stdout);
		}
		printf("]\n");
	}
}

//=============
// BEGIN TESTS
//=============

int TEST_string_create(){
	Rivr_String* rv_string;
	
	// single-leaf string
	rv_string = string_create(64, STRING_NONE);
	Assert(
		count_string_nodes(rv_string) == 1, 
		"string node count != 1", 
		count_string_nodes(rv_string)
	);
	string_destroy(rv_string);
	
	// multiple leaves
	rv_string = string_create(555, STRING_NONE);
	Assert(
		count_string_nodes(rv_string) == 15, 
		"string node count != 15", 
		count_string_nodes(rv_string)
	);
	string_destroy(rv_string);
	
	// multiple leaves, created with minimal nodes
	rv_string = string_create(555, STRING_MINNODES);
	Assert(
		count_string_nodes(rv_string) == 9, 
		"string minimal node count != 9", 
		count_string_nodes(rv_string)
	);
	string_destroy(rv_string);
	
	// single node, with a sequence applied
	rv_string = string_create_from_seq(test_seq3, LENGTH3, STRING_NONE);
	Assert(
		count_string_nodes(rv_string) == 1, 
		"string node count != 1 for seq3", 
		count_string_nodes(rv_string)
	);
	
	// multiple leaves, with a sequence applied
	rv_string = string_create_from_seq(test_seq1, LENGTH1, STRING_NONE);
	Assert(
		count_string_nodes(rv_string) == 7, 
		"string node count != 7 for seq1", 
		count_string_nodes(rv_string)
	);
	string_destroy(rv_string);
	
	// multiple leaves, with a sequence applied, created with minimal nodes
	rv_string = string_create_from_seq(test_seq1, LENGTH1, STRING_MINNODES);
	Assert(
		count_string_nodes(rv_string) == 5, 
		"string minimal node count != 5 for seq1", 
		count_string_nodes(rv_string)
	);
	string_destroy(rv_string);
	
	// string created with minnodes then rebalanced
	rv_string = string_create_from_seq(test_seq1, LENGTH1, STRING_MINNODES);
	Rivr_String* rv_string2 = string_rebalance(rv_string);
	Assert(
		count_string_nodes(rv_string2) == 7, 
		"string node count after rebalancing != 7 for seq1", 
		count_string_nodes(rv_string2)
	);
	
	string_destroy(rv_string);
	string_destroy(rv_string2);
	
	
	printf("Completed 7 tests in TEST_string_create\n");
	
	return 1;
}

int TEST_string_compare_clone(){
	Rivr_String* str1 = string_create_from_seq(test_seq1, LENGTH1, 0);
	Rivr_String* str2 = string_create_from_seq(test_seq1, LENGTH1, 0);
	Assert(string_compare(str1, str2) == 0, "string comparison failure", string_compare(str1, str2));
	string_destroy(str2);
	
	str2 = string_clone(str1);
	Assert(string_compare(str1, str2) == 0, "string cloning failure", string_compare(str1, str2));
	string_destroy(str2);
	string_destroy(str1);
	
	printf("Completed 2 tests in TEST_string_compare_clone\n");
	
	return 1;
}

int TEST_string_flatten(){
	Rivr_String* rv_string;
	
	// single-leaf string
	rv_string = string_create_from_seq(test_seq2, LENGTH2, STRING_NONE);
	char* test_seq2_flat = string_flatten(rv_string);
	Assert(
		strncmp(test_seq2, test_seq2_flat, LENGTH2) == 0, 
		"flattened small string mismatch", 
		strncmp(test_seq2, test_seq2_flat, LENGTH2)
	);
	free(test_seq2_flat);
	string_destroy(rv_string);
	
	// multiple leaves
	rv_string = string_create_from_seq(test_seq1, LENGTH1, STRING_NONE);
	char* test_seq1_flat = string_flatten(rv_string);
	Assert(
		strncmp(test_seq1, test_seq1_flat, LENGTH1) == 0, 
		"flattened large string mismatch", 
		strncmp(test_seq1, test_seq1_flat, LENGTH1)
	);
	free(test_seq1_flat);
	string_destroy(rv_string);
	
	printf("Completed 2 tests in TEST_string_flatten\n");
	
	return 1;
}

int TEST_string_replace(){
	Rivr_String* rv_string;
	const char test_seq2_edited[LENGTH2] = "Lorem_Ipsum";
	const char small_insert[3] = "_I";
	const char large_insert[146] = "harsh and inhospitable land from the rich emerald-green 'Ocean of Trees' that makes up most of the West Cornucopia. Cultures are divided too: the";
	const char test_seq1_edited[LENGTH1] = "The Starry Mountains to the east of the Dawn Desert divide that harsh and inhospitable land from the rich emerald-green 'Ocean of Trees' that makes up most of the West Cornucopia. Cultures are divided too: the Dawn Deserters to the west; the civilized men of the Cornucopia to the East; the zealous Nightborn who are native to the high valleys; the primitive shepherds to the south.";
	char* test_seq2_edited_flat;
	char* test_seq1_edited_flat;
	
	
	// replace part of single-node string with char sequence
	rv_string = string_create_from_seq(test_seq2, LENGTH2, STRING_NONE);
	string_replace_seq(rv_string, 5, small_insert, 2);
	test_seq2_edited_flat = string_flatten(rv_string);
	Assert(
		strncmp(test_seq2_edited, test_seq2_edited_flat, LENGTH2) == 0, 
		"failed to replace with seq in single-node string", 
		strncmp(test_seq2_edited, test_seq2_edited_flat, LENGTH2)
	);
	string_destroy(rv_string);
	free(test_seq2_edited_flat);
	
	// replace part of multi-node string with char sequence
	rv_string = string_create_from_seq(test_seq1, LENGTH1, STRING_NONE);
	string_replace_seq(rv_string, 64, large_insert, 145);
	test_seq1_edited_flat = string_flatten(rv_string);
	Assert(
		strncmp(test_seq1_edited, test_seq1_edited_flat, LENGTH1) == 0, 
		"failed to replace with seq in multi-node string", 
		strncmp(test_seq1_edited, test_seq1_edited_flat, LENGTH1)
	);
	string_destroy(rv_string);
	free(test_seq1_edited_flat);
	
	Rivr_String* rv_string2;
	
	// replace part of single-node string with rivr string
	rv_string = string_create_from_seq(test_seq2, LENGTH2, STRING_NONE);
	rv_string2 = string_create_from_seq(small_insert, 2, 0);
	string_replace_str(rv_string, 5, rv_string2);
	test_seq2_edited_flat = string_flatten(rv_string);
	Assert(
		strncmp(test_seq2_edited, test_seq2_edited_flat, LENGTH2) == 0, 
		"failed to replace with string in single-node string", 
		strncmp(test_seq2_edited, test_seq2_edited_flat, LENGTH2)
	);
	string_destroy(rv_string);
	string_destroy(rv_string2);
	free(test_seq2_edited_flat);
	
	// replace part of multi-node string with rivr string
	rv_string = string_create_from_seq(test_seq1, LENGTH1, STRING_NONE);
	rv_string2 = string_create_from_seq(large_insert, 145, 0);
	string_replace_str(rv_string, 64, rv_string2);
	test_seq1_edited_flat = string_flatten(rv_string);
	Assert(
		strncmp(test_seq1_edited, test_seq1_edited_flat, LENGTH1) == 0, 
		"failed to replace with string in multi-node string", 
		strncmp(test_seq1_edited, test_seq1_edited_flat, LENGTH1)
	);
	string_destroy(rv_string);
	string_destroy(rv_string2);
	free(test_seq1_edited_flat);
	
	printf("Completed 4 tests in TEST_string_replace\n");
	
	return 1;
}

int TEST_string_insert(){
	Rivr_String* rv_string;
	char* test_result_flattened;
	const char test_seq2_edited[23] = "LoremLorem ipsum ipsum";
	const char test_seq3_edited[140] = "The woods throughout the northeast are Lorem ipsumyoung and crisscrossed with low stone walls that give evidence that someone farmed here. ";
	const char test_seq1_edited[511] = "The Starry Mountains to the east of the Dawn Desert divide that dry, dark region from the lush, verdant jungles and forests of the West Cornucopia. The woods throughout the northeast are young and crisscrossed with low stone walls that give evidence that someone farmed here. The mountains stand at the intersection of cultures: the wild Dawn Deserters to the west; the civilized men of the Cornucopia to the East; the zealous Nightborn who are native to the high valleys; the primitive shepherds to the south.";
	
	// insert seq into a single-node string
	rv_string = string_create_from_seq(test_seq2, LENGTH2, STRING_NONE);
	string_insert_seq(rv_string, 5, test_seq2, LENGTH2);
	test_result_flattened = string_flatten(rv_string);
	Assert(
		strncmp(test_seq2_edited, test_result_flattened, 22) == 0,
		"failed to insert sequence into single-node string",
		strncmp(test_seq2_edited, test_result_flattened, 22)
	);
	free(test_result_flattened);
	string_destroy(rv_string);
	
	
	// insert seq into a single-node string which will become a multi-node string
	rv_string = string_create_from_seq(test_seq3, LENGTH3, STRING_NONE);
	string_insert_seq(rv_string, 39, test_seq2, LENGTH2);
	test_result_flattened = string_flatten(rv_string);
	Assert(
		strncmp(test_seq3_edited, test_result_flattened, 139) == 0,
		"failed to insert sequence into single-node string of length 128",
		strncmp(test_seq3_edited, test_result_flattened, 139)
	);
	free(test_result_flattened);
	string_destroy(rv_string);
	
	// insert seq into a multi-node string
	rv_string = string_create_from_seq(test_seq1, LENGTH1, STRING_NONE);
	string_insert_seq(rv_string, 148, test_seq3, LENGTH3);
	test_result_flattened = string_flatten(rv_string);
	Assert(
		strncmp(test_seq1_edited, test_result_flattened, LENGTH1 + LENGTH3) == 0,
		"failed to insert sequence into multi-node string",
		strncmp(test_seq1_edited, test_result_flattened, LENGTH1 + LENGTH3)
	);
	free(test_result_flattened);
	string_destroy(rv_string);
	
	Rivr_String* rv_string2 = string_create_from_seq(test_seq2, LENGTH2, STRING_NONE);
	
	// insert string into a single-node string
	rv_string = string_create_from_seq(test_seq2, LENGTH2, STRING_NONE);
	string_insert_str(rv_string, 5, rv_string2);
	test_result_flattened = string_flatten(rv_string);
	Assert(
		strncmp(test_seq2_edited, test_result_flattened, 22) == 0,
		"failed to insert string into single-node string",
		strncmp(test_seq2_edited, test_result_flattened, 22)
	);
	free(test_result_flattened);
	string_destroy(rv_string);
	
	
	// insert string into a single-node string which will become a multi-node string
	rv_string = string_create_from_seq(test_seq3, LENGTH3, STRING_NONE);
	string_insert_str(rv_string, 39, rv_string2);
	test_result_flattened = string_flatten(rv_string);
	Assert(
		strncmp(test_seq3_edited, test_result_flattened, 139) == 0,
		"failed to insert string into single-node string of length 128",
		strncmp(test_seq3_edited, test_result_flattened, 139)
	);
	free(test_result_flattened);
	string_destroy(rv_string);
	
	string_destroy(rv_string2);
	rv_string2 = string_create_from_seq(test_seq3, LENGTH3, STRING_NONE);
	
	
	// insert string into a multi-node string
	rv_string = string_create_from_seq(test_seq1, LENGTH1, STRING_NONE);
	
	string_insert_str(rv_string, 148, rv_string2);
	test_result_flattened = string_flatten(rv_string);
	
	Assert(
		strncmp(test_seq1_edited, test_result_flattened, 511) == 0,
		"failed to insert string of length 128 into multi-node string",
		strncmp(test_seq1_edited, test_result_flattened, 511)
	);
	free(test_result_flattened);
	string_destroy(rv_string);
	string_destroy(rv_string2);
	
	
	printf("Completed 6 tests in TEST_string_insert\n");
	
	return 1;
}

int TEST_string_remove(){
	Rivr_String* rv_string;
	char* test_result_flattened;
	const char test1_result_expected[5] = "Lorem";
	const char test2_result_expected[376] = "The Mountains to the east of the Dawn Desert divide that dry, dark region from the lush, verdant jungles and forests of the West Cornucopia. The mountains stand at the intersection of cultures: the wild Dawn Deserters to the west; the civilized men of the Cornucopia to the East; the zealous Nightborn who are native to the high valleys; the primitive shepherds to the south.";
	const char test3_result_expected[244] = "The Starry Mountains to the east of the Dawn Desert divide that dry, dark region from the lush, verdant jungles and forests of the West Cornucopia. the zealous Nightborn who are native to the high valleys; the primitive shepherds to the south.";
	
	// remove from a single-node string
	rv_string = string_create_from_seq(test_seq2, LENGTH2, STRING_NONE);
	string_remove(rv_string, 5, 11);
	test_result_flattened = string_flatten(rv_string);
	Assert(
		strncmp(test_result_flattened, test1_result_expected, 5) == 0,
		"failed to remove from a single-node string",
		strncmp(test_result_flattened, test1_result_expected, 5)
	);
	free(test_result_flattened);
	string_destroy(rv_string);
	
	
	// remove a small (<=128 chars) section of a multi-node string
	rv_string = string_create_from_seq(test_seq1, LENGTH1, STRING_NONE);
	string_remove(rv_string, 4, 11);
	test_result_flattened = string_flatten(rv_string);
	Assert(
		strncmp(test_result_flattened, test2_result_expected, 376) == 0,
		"failed to remove a small segment from a single-node string",
		strncmp(test_result_flattened, test2_result_expected, 376)
	);
	free(test_result_flattened);
	string_destroy(rv_string);
	
	// remove a large (>128 chars) section of a multi-node string
	rv_string = string_create_from_seq(test_seq1, LENGTH1, STRING_NONE);
	string_remove(rv_string, 148, 287);
	test_result_flattened = string_flatten(rv_string);
	Assert(
		strncmp(test_result_flattened, test3_result_expected, 376) == 0,
		"failed to remove a large segment from a multi-node string",
		strncmp(test_result_flattened, test3_result_expected, 376)
	);
	free(test_result_flattened);
	string_destroy(rv_string);
	
	printf("Completed 3 tests in TEST_string_remove\n");
	
	return 1;
}

int TEST_string_arithmetic(){
	
	Rivr_String* rv_string;
	Rivr_String* rv_string2;
	Rivr_String* rv_string3;
	char* test_result;
	const char test1_result_expected[LENGTH2 + LENGTH3 + 1] = "The woods throughout the northeast are young and crisscrossed with low stone walls that give evidence that someone farmed here. Lorem ipsum";
	const char test2_result_expected[LENGTH2 + LENGTH2 + 1] = "Lorem ipsumLorem ipsum";
	const char test3_result_expected[LENGTH1 + LENGTH1 + 1] = "The Starry Mountains to the east of the Dawn Desert divide that dry, dark region from the lush, verdant jungles and forests of the West Cornucopia. The mountains stand at the intersection of cultures: the wild Dawn Deserters to the west; the civilized men of the Cornucopia to the East; the zealous Nightborn who are native to the high valleys; the primitive shepherds to the south.The Starry Mountains to the east of the Dawn Desert divide that dry, dark region from the lush, verdant jungles and forests of the West Cornucopia. The mountains stand at the intersection of cultures: the wild Dawn Deserters to the west; the civilized men of the Cornucopia to the East; the zealous Nightborn who are native to the high valleys; the primitive shepherds to the south.";
	
	// merge two single-node strings to create a multi-node string
	rv_string = string_create_from_seq(test_seq3, LENGTH3, STRING_NONE);
	rv_string2 = string_create_from_seq(test_seq2, LENGTH2, STRING_NONE);
	rv_string3 = string_merge(rv_string, rv_string2);
	Assert(
		count_string_nodes(rv_string3) == count_string_nodes(rv_string) + count_string_nodes(rv_string2) + 1,
		"node count mismatch after merging single-node strings",
		count_string_nodes(rv_string3) - (count_string_nodes(rv_string) + count_string_nodes(rv_string2) + 1)
	);
	test_result = string_flatten(rv_string3);
	Assert(
		strncmp(test_result, test1_result_expected, LENGTH2 + LENGTH3) == 0,
		"flattened string mismatch in multi-node string after merging single-node strings",
		strncmp(test_result, test1_result_expected, LENGTH2 + LENGTH3)
	);
	string_destroy(rv_string3);
	free(test_result);
	
	// merge two small strings to create a single node string
	rv_string = string_create_from_seq(test_seq2, LENGTH2, STRING_NONE);
	rv_string2 = string_create_from_seq(test_seq2, LENGTH2, STRING_NONE);
	rv_string3 = string_merge(rv_string, rv_string2);
	Assert(
		count_string_nodes(rv_string3) == 1,
		"node count != 1 for a merged string of length <= 128",
		count_string_nodes(rv_string3)
	);
	test_result = string_flatten(rv_string3);
	Assert(
		strncmp(test_result, test2_result_expected, LENGTH2 + LENGTH2) == 0,
		"flattened string mismatch in single-node string after merging single-node strings",
		strncmp(test_result, test2_result_expected, LENGTH2 + LENGTH2)
	);
	string_destroy(rv_string);
	string_destroy(rv_string2);
	string_destroy(rv_string3);
	free(test_result);
	
	// merge two multi-node strings
	rv_string = string_create_from_seq(test_seq1, LENGTH1, STRING_NONE);
	rv_string2 = string_create_from_seq(test_seq1, LENGTH1, STRING_MINNODES);
	rv_string3 = string_merge(rv_string, rv_string2);
	Assert(
		count_string_nodes(rv_string3) == 1 + count_string_nodes(rv_string) + count_string_nodes(rv_string2),
		"node count mismatch after merging multi-node string",
		count_string_nodes(rv_string3) - (1 + count_string_nodes(rv_string) + count_string_nodes(rv_string2))
	);
	test_result = string_flatten(rv_string3);
	Assert(
		strncmp(test_result, test3_result_expected, LENGTH1 + LENGTH1) == 0,
		"flattened string mismatch in multi-node string after merging multi-node strings",
		strncmp(test_result, test3_result_expected, LENGTH1 + LENGTH1)
	);
	string_destroy(rv_string3);
	free(test_result);
	
	// take substring of single-node string
	
	// take substring of multi-node string
	
	printf("Completed 3 implemented, 2 UNIMPLEMENTED tests in TEST_string_arithmetic\n");
	
	return 1;
}

int TEST_string_search(){
	
	// search for a small sequence in a single-node string
	
	// search for a small (<=128 chars) sequence in a multi-node string
	
	// search for a large (>128 chars) sequence in a multi-node string
	
	printf("Completed 3 UNIMPLEMENTED tests in TEST_string_search\n");
	
	return 1;
}

int TEST_string_hash(){
	
	Rivr_String* rv_string;
	Rivr_String* rv_string2;
	unsigned int hash1;
	unsigned int hash2;
	
	// verify hash function works
	rv_string = string_create_from_seq(test_seq1, LENGTH1, STRING_NONE);
	hash1 = string_generate_hash(rv_string);
	Assert(
		hash1 != 0,
		"string's hash was 0",
		hash1
	);
	
	// generate a hash which should be different
	rv_string2 = string_create_from_seq(test_seq3, LENGTH3, STRING_NONE);
	hash2 = string_generate_hash(rv_string2);
	Assert(
		hash2 != hash1,
		"two different strings had the same hash",
		hash2
	);
	string_destroy(rv_string2);
	
	// compare string hashes with different internal structure, but same data
	rv_string2 = string_create_from_seq(test_seq1, LENGTH1, STRING_MINNODES);
	hash2 = string_generate_hash(rv_string2);
	Assert(
		hash1 == hash2,
		"identical data with different hashes",
		hash2
	);
	
	
	string_destroy(rv_string);
	string_destroy(rv_string2);
	
	printf("Completed 3 tests in TEST_string_hash\n");
	
	return 1;
}

int TEST_string_print(){
	Rivr_String* rv_string;
	
	printf("Printing small string:");
	rv_string = string_create_from_seq(test_seq2, LENGTH2, STRING_NONE);
	string_print(rv_string, printf);
	putc('\n', stdout);
	
	string_destroy(rv_string);
	
	printf("Printing large string:");
	rv_string = string_create_from_seq(test_seq1, LENGTH1, STRING_NONE);
	string_print(rv_string, printf);
	putc('\n', stdout);
	
	string_destroy(rv_string);
	
	printf("Completed 2 tests in TEST_string_print\n");
	
	return 1;
}

int main(int argc, char** argv){
	
	int n_successful = 0;
	
	n_successful += TEST_string_create();
	n_successful += TEST_string_flatten();
	n_successful += TEST_string_compare_clone();
	n_successful += TEST_string_replace();
	n_successful += TEST_string_insert();
	n_successful += TEST_string_remove();
	n_successful += TEST_string_arithmetic();
	n_successful += TEST_string_search();
	n_successful += TEST_string_hash();
	n_successful += TEST_string_print();
	
	printf("Succeeded in %d tests.\n", n_successful);
	
	return 0;
}
