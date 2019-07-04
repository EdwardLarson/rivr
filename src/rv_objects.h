#include "rv_types.h"

struct Rivr_Object_;

typedef int Member_ID_t;
typedef struct Rivr_Object_ {
	int n_members;

	// ordered map of member IDs
	Member_ID_t* member_lut;
	// members; their index can be found by searching the member_lut for the corresponding ID
	union Data_* members;


} Rivr_Object;

Rivr_Object* obj_create(int n_members);
void obj_destroy(Rivr_Object* object);
Rivr_Object* obj_copy(const Rivr_Object* original);
Rivr_Object* obj_compose(const Rivr_Object* first, const Rivr_Object* second);
int obj_look_up_id(const Rivr_Object* object, Member_ID_t id);
int obj_retrieve_member(Rivr_Object* object, Member_ID_t id, union Data_* dest);
