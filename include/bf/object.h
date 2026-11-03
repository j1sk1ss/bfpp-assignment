#ifndef BF_OBJECT_H_
#define BF_OBJECT_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef struct bf_object bf_object;
typedef struct bf_type_info bf_type_info;

struct bf_type_info {
    const char*             name;
    const bf_type_info*     parent;
};

typedef struct {
    const bf_type_info*     type;
    void                    (*destroy)(bf_object*);
} bf_object_vtable;

struct bf_object {
    const bf_object_vtable* vptr;
};

extern const bf_type_info bf_type_object;

void        OBJ_init(bf_object* self, const bf_object_vtable* vptr);
void        OBJ_destroy(bf_object* self);
const char* OBJ_type_name(const bf_object* self);
int         OBJ_is_a(const bf_object* self, const bf_type_info* type);

#define bf_as_object(ptr) ((bf_object*)(ptr))
#define bf_as_const_object(ptr) ((const bf_object*)(ptr))

#ifdef __cplusplus
}
#endif

#endif
