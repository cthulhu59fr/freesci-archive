#ifndef OLD_OBJECTS_H
#define OLD_OBJECTS_H

typedef FLEXARRAY(script_opcode,int number;) script_method;

typedef struct object_
{
	/*These are based on cached selector values, and set to the values
	 *the selectors had at load time. If the selectors are changed in
	 *instances, inconsistency will follow*/
	struct object_* parent;
	char* name;

	FLEXARRAY_NOEXTRA(struct object_*) children;

	/*No flexarray, size the size is known from the start*/
	script_method** methods;
	int method_count;

	int selector_count;
	int* selector_numbers;
} object;

typedef struct
{
  int id;
  object* class;
  byte* heap;
  int offset;
} instance;

object **object_map, *object_root;
int max_object;

#define SCRIPT_PRINT_METHODS	1
#define SCRIPT_PRINT_CHILDREN	2
#define SCRIPT_PRINT_SELECTORS  3
void printObject(object* obj, int flags);

int loadObjects();
void freeObject(object*);

extern char* globals[];

#endif