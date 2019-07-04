#include "rv_objects.h"
#include "vm.h"
#include <malloc.h>

Rivr_Object* obj_create(int n_members){
	Rivr_Object* new_object = malloc(sizeof(Rivr_Object));

	new_object->n_members = n_members;
	new_object->member_lut = malloc(sizeof(int) * n_members);
	new_object->members = malloc(sizeof(Data*) * n_members);

	return new_object;
}

void obj_destroy(Rivr_Object* object){
	free(object->members);
	free(object->member_lut);
	free(object);
}

Rivr_Object* obj_copy(const Rivr_Object* original){
	Rivr_Object* new_object = obj_create(original->n_members);

	memcpy(new_object->member_lut, original->member_lut, original->n_members * sizeof(Member_ID_t)); 
	memcpy(new_object->members, original->members, original->n_members * sizeof(Data)); 

	return new_object;
}

// takes two objects and returns a new object with a member array which is a union of the two
Rivr_Object* obj_compose(const Rivr_Object* first, const Rivr_Object* second){

	int n_composed_members = first->n_members + second->n_members;

	// array to hold the merged member ids
	// at most the number of members is the sum or each aprent, but there may be overlaps, so this array may be reduced in size later
	Member_ID_t* merged_member_lut = malloc(sizeof(int) * n_composed_members);
	Data* merged_member_array = malloc(sizeof(Data) * n_composed_members);

	int i_first = 0;
	int i_second = 0;
	int i_new = 0;

	int n_duplicates;

	while (i_first < first->n_members && i_second < second->n_members){
		int id_from_first = first->member_lut[i_first];
		int id_from_second = second->member_lut[i_second];

		if (id_from_first < id_from_second){
			merged_member_lut[i_new] = id_from_first;
			merged_member_array[i_new] = first->members[i_first];

			i_first++;
		}
		else if (id_from_first > id_from_second){
			merged_member_lut[i_new] = id_from_second;
			merged_member_array[i_new] = second->members[i_second];

			i_second++;
		}
		else{
			// conflict! we have an overlap in member IDs
			// need to choose one member to take precedent
			// for now, just let the fist object take precedent
			merged_member_lut[i_new] = id_from_first;
			i_first++;

			n_duplicates++;
		}

		i_new++;
	}

	// now one or both of the objects has copied all their members over
	// copy the rest from the other over

	for (; i_first < first->n_members; i_first++){
		merged_member_lut[i_new] = first->member_lut[i_first];
		merged_member_array[i_new] = first->members[i_first];

		i_new++;
	}
	for (; i_second < second->n_members; i_second++){
		merged_member_lut[i_new] = second->member_lut[i_second];
		merged_member_array[i_new] = second->members[i_second];

		i_new++;
	}

	// resize the new member lut to trim any space left empty by duplicates
	n_composed_members -= n_duplicates;
	realloc(merged_member_lut, sizeof(int) * n_composed_members);
	realloc(merged_member_array, sizeof(Data*) * n_composed_members);

	Rivr_Object* new_object = malloc(sizeof(Rivr_Object));

	new_object->n_members = n_composed_members;
	new_object->member_lut = merged_member_lut;
	new_object->members = merged_member_array;

	return new_object;

}

// find the index of a member matching the given ID in a given object
// returns -1 if the member was not found
int obj_look_up_id(const Rivr_Object* object, Member_ID_t id){

	int lower_bound = 0;
	int upper_bound = object->n_members;
	int i;
	Member_ID_t current_id;

	while(lower_bound != upper_bound) {
		i = ((upper_bound - lower_bound) / 2) + lower_bound;
		current_id = object->member_lut[i];

		if (current_id > id){
			if (i == upper_bound) break;
			upper_bound = i;
		}
		else if (current_id < id){
			if (i == lower_bound) break;
			lower_bound = i;

		}
		else { // current_id = id
			return i;

		}

	};
	return -1;

}

// get the member indexed by the given ID from object
// if the object has a member matching the id, writes the member to dest and returns 1
// otherwise return 0
int obj_retrieve_member(Rivr_Object* object, Member_ID_t id, Data* dest){
	int member_index = obj_look_up_id(object, id);

	if (member_index >= 0){
		*dest = object->members[member_index];
		return 1;
	}
	else{
		return 0;
	}
}

