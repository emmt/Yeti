/*
 * tuples.c -
 *
 * Implement tuple-like objects for Yorick.
 *
 *-----------------------------------------------------------------------------
 *
 * This file is part of Yeti (https://github.com/emmt/Yeti) released under the
 * MIT "Expat" license.
 *
 * Copyright (C) 1996-2020: Éric Thiébaut.
 *
 *-----------------------------------------------------------------------------
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define USE_OLD_API
#define OFFSETOF(T, m) ((long)&(((T*)0)->m))

#include <yapi.h>

typedef struct _tuple {
  long   numb;   /* number of objects in tuple */
  void*  objs[]; /* contents */
} tuple_t;

static void
free_tuple(void* addr)
{
    tuple_t* tuple = (tuple_t*)addr;
    long i;
    for (i = 0; i < tuple->numb; ++i) {
        if (tuple->objs[i] != NULL) {
            ydrop_use(tuple->objs[i]);
        }
    }
}

static void
print_tuple(void* addr)
{
    char buf[32];
    tuple_t* tuple = (tuple_t*)addr;
    sprintf(buf, "%ld", tuple->numb);
    y_print("tuple (", 0);
    y_print(buf, 0);
    y_print((tuple->numb > 1 ? " elements)" : " element)"), 1);
}

static void
eval_tuple(void* addr, int argc)
{
    tuple_t* tuple = (tuple_t*)addr;
    if (argc != 1) {
        y_error("expecting exactly one argument");
    }
    if (yarg_nil(0)) {
        ypush_long(tuple->numb);
    } else {
        long index = ygets_l(0);
        if (index <= 0) {
            index += tuple->numb;
        }
        if (index < 1 || index > tuple->numb) {
            /* FIXME: we could push nil for out of range indices */
            y_error("out of range tuple index");
        }
        ykeep_use(tuple->objs[index - 1]);
    }
}

static void
extract_tuple(void* addr, char* name)
{
    tuple_t* tuple = (tuple_t*)addr;
    if (strcmp(name, "number") == 0) {
        ypush_long(tuple->numb);
    } else {
        y_error("bad member");
    }
}

static y_userobj_t tuple_type = {
    "tuple",
    free_tuple,
    print_tuple,
    eval_tuple,
    extract_tuple,
    NULL
};

void Y_is_tuple(int argc)
{
    if (argc != 1) y_error("is_tuple() takes exactly one argument");
    int type = yarg_typeid(0);
    int res;
    if (type == Y_OPAQUE) {
      const char* name = yget_obj(0, NULL);
      res = (name != NULL && strcmp(name, tuple_type.type_name) == 0);
    } else {
      res = 0;
    }
    ypush_int(res);
}

void Y_tuple(int argc)
{
    int i;
    long size = OFFSETOF(tuple_t, objs) + argc*sizeof(void*);
    tuple_t* tup = (tuple_t*)ypush_obj(&tuple_type, size);
    tup->numb = argc;
    for (i = 0; i < argc; ++i) {
        tup->objs[i] = yget_use(argc-i);
    }
}
