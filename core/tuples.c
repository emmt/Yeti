/*
 * tuples.c -
 *
 * Implement tuple-like objects for Yorick.
 *
 *-----------------------------------------------------------------------------
 *
 * Copyright (C) 1996-2020, Éric Thiébaut.
 *
 * This software is governed by the CeCILL-C license under French law and
 * abiding by the rules of distribution of free software.  You can use, modify
 * and/or redistribute the software under the terms of the CeCILL-C license as
 * circulated by CEA, CNRS and INRIA at the following URL
 * "http://www.cecill.info".
 *
 * As a counterpart to the access to the source code and rights to copy,
 * modify and redistribute granted by the license, users are provided only
 * with a limited warranty and the software's author, the holder of the
 * economic rights, and the successive licensors have only limited liability.
 *
 * In this respect, the user's attention is drawn to the risks associated with
 * loading, using, modifying and/or developing or reproducing the software by
 * the user in light of its specific status of free software, that may mean
 * that it is complicated to manipulate, and that also therefore means that it
 * is reserved for developers and experienced professionals having in-depth
 * computer knowledge. Users are therefore encouraged to load and test the
 * software's suitability as regards their requirements in conditions enabling
 * the security of their systems and/or data to be ensured and, more
 * generally, to use and operate it in the same conditions as regards
 * security.
 *
 * The fact that you are presently reading this means that you have had
 * knowledge of the CeCILL-C license and that you accept its terms.
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
