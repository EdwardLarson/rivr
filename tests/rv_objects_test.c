#include <stdio.h>
#include "../src/rv_objects.h"
#include "test_tools.h"
#include "../src/vm.h"

void populate_with_members(Rivr_Object* object){
	int i;

	union {
		union Data_ as_rivr_data;
		long int as_int;
	} conversion_union;

	for (i = 0; i < object->n_members; i++){

		conversion_union.as_int = 0xF00 & (i * 2);

		object->member_lut[i] = i * 2; // so we can easily look up nonexistant IDs using odd IDs
		object->members[i] = conversion_union.as_rivr_data;
	}
}

int TEST_create(){

	Rivr_Object* new_obj_1 = obj_create(1);
	Assert(
		new_obj_1->n_members == 1,
		"object should have exactly 1 member",
		new_obj_1->n_members
	);

	Rivr_Object* new_obj_4 = obj_create(4);
	Assert(
		new_obj_4->n_members == 4,
		"object should have exactly 4 members",
		new_obj_4->n_members
	);


	return 1;
}

int TEST_copy(){
	Rivr_Object* new_obj = obj_create(4);
	Assert(
		new_obj->n_members == 4,
		"object should have exactly 4 members",
		new_obj->n_members
	);

	populate_with_members(new_obj);

	Rivr_Object* copied_obj = obj_copy(new_obj);

	int i;
	for (i = 0; i < new_obj->n_members; i++){
		Assert(
			new_obj->member_lut[i] == copied_obj->member_lut[i],
			"mismatch between original and copied object member IDs",
			i
		);

		Assert(
			new_obj->members[i].n == copied_obj->members[i].n,
			"mismatch between original and copied object member objects",
			i
		);
	}



	return 1;
}

int TEST_lookup(){
	Rivr_Object* obj = obj_create(4);
	populate_with_members(obj);

	int i;
	int lookup_result;

	for (i = 0; i < 8; i += 2){
		lookup_result = obj_look_up_id(obj, i);
		Assert(
			i/2 == lookup_result,
			"lookup on a known present member id failed",
			i
		);
	}

	for (i = 1; i < 9; i += 2){
		lookup_result = obj_look_up_id(obj, i);
		Assert(
			-1 == lookup_result,
			"lookup on a known missing member id succeeded",
			i
		);
	}

	return 1;

}

int TEST_retrieve(){
	Rivr_Object* obj = obj_create(4);
	populate_with_members(obj);

	int i;
	int lookup_result;
	Data member;

	for (i = 0; i < 8; i += 2){
		lookup_result = obj_retrieve_member(obj, i, &member);
		Assert(
			1 == lookup_result,
			"lookup on a known present member id failed",
			lookup_result
		);

		Assert(
			member.n == (0xF00 & i),
			"lookup on a known present member id succeeded, but returned the wrong member",
			member.n
		);
	}

	for (i = 1; i < 9; i += 2){
		lookup_result = obj_retrieve_member(obj, i, &member);
		Assert(
			0 == lookup_result,
			"lookup on a known missing member id succeeded",
			lookup_result
		);
	}

	return 1;
}

int TEST_compose(){
	Rivr_Object* obj_a = obj_create(4);
	populate_with_members(obj_a);

	Rivr_Object* obj_b = obj_copy(obj_a);
	int i;
	for (i = 0; i < obj_b->n_members; i++){
		obj_b->member_lut[i]++;
		obj_b->members[i].n++;
	}

	Rivr_Object* obj_c = obj_compose(obj_a, obj_b);

	Assert(
		obj_c->n_members == 8,
		"object resulting from compose has incorrect number of members",
		obj_c->n_members
	);

	for (i = 0; i < obj_c->n_members; i++){
		int _i = i / 2;
		Rivr_Object* parent_of_i;

		if (i & 1){
			// odd indices can be found in obj_b
			parent_of_i = obj_b;
		}
		else{
			// even indices can be found in obj_a
			parent_of_i = obj_a;
		}

		Assert(
			obj_c->member_lut[i] == parent_of_i->member_lut[_i],
			"object resulting from compose has incorrect location for member id",
			i
		);

		Assert(
			obj_c->members[i].n == parent_of_i->members[_i].n,
			"object resulting from compose has incorrect location for member",
			i
		);
	}

	return 1;
}

int (*tests[])(void) = {
	TEST_create,
	TEST_copy,
	TEST_lookup,
	TEST_retrieve,
	TEST_compose
};

#define N_TESTS 3

int main(int argc, char** argv){
	int n_successful = 0;
	int n_tests = sizeof(tests) / sizeof( int (*)(void) );

	printf("Running %d tests...\n", n_tests);

	int i;
	for (i = 0; i < n_tests; i++){
		n_successful += tests[i]();
	}

	printf("Succeeded in %d tests.\n", n_successful);
	
	return 0;
}