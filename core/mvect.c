#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <yapi.h>
#include <pstdlib.h>

typedef enum {
    NOTHING = 0,   /* An alias for a void object, also used to mark dropped
                      objects to make sure their address is not wrongly used in
                      case of interupts. Value *must* be 0 to be compatible
                      with array of entries being initially zero-filled. */
    SCALAR_INT,    /* A fast scalar int. */
    SCALAR_LONG,   /* A fast scalar long. */
    SCALAR_DOUBLE, /* A fast scalar double. */
    OBJECT         /* Any non-void object, although assumed to be void if
                      corresponding value is a null pointer. */
} entry_type;

typedef struct {
    entry_type type;
    union {
        int i;
        long l;
        double d;
        void* o;
    } val;
} entry;

typedef struct {
    long len;     // current length
    long maxlen;  // maximal lenght (size of allocated array)
    entry *arr;   // array of entries
} mvect;

#define MIN(a, b)          ((a) < (b) ? (a) : (b))
#define MAX(a, b)          ((a) > (b) ? (a) : (b))
#define ROUND_UP(a, b)     ((((b) - 1 + (a))/(b))*(b))
#define IN_RANGE(x, a, b)  (((a) <= (x)) & ((x) <= (b)))

#define MVECT_TYPE_NAME "mixed-vector"

#define CHUNK_SIZE 8          // storage is a multiple of this
#define MIN_SIZE   CHUNK_SIZE // minimum number of entries, should be a multiple of CHUNK_SIZE

static inline long adjust_index(const mvect* vec, long idx)
{
    if (idx <= 0) {
        // Apply Yorick's indexing rule.
        idx += vec->len;
    }
    if (!IN_RANGE(idx, 1, vec->len)) {
        y_error("index overreach beyond mixed vector bounds");
    }
    return idx - 1; // 1-based index -> 0-based index
}

// Push entry on top of stack. The entry is also forgotten if requested.
static inline void push_entry(entry* e, bool forget)
{
    int type = e->type;
    if (forget) {
        // Change entry type before pushing in case of interupts.
        e->type = NOTHING;
    }
    if (type == SCALAR_INT) {
        ypush_int(e->val.i);
    } else if (type == SCALAR_LONG) {
        ypush_long(e->val.l);
    } else if (type == SCALAR_DOUBLE) {
        ypush_double(e->val.d);
    } else if (type == NOTHING || (type == OBJECT && e->val.o == NULL)) {
        ypush_nil();
    } else if (type == OBJECT) {
        if (forget) {
            ypush_use(e->val.o);
        } else {
            ykeep_use(e->val.o);
        }
    } else {
        // Do not restore entry type so that, if this invalid entry is to be
        // forgotten, it will be considered as void the next time it is used.
        y_error("unexpected mixed vector entry type");
    }
}

// Store stack item in empty entry.
static inline void store_entry(entry* e, int iarg)
{
    if (e->type != NOTHING) y_error("attempt to store in non-empty mixed vector entry");
    int type = yarg_typeid(iarg);
    if ((type == Y_INT || type == Y_LONG || type == Y_DOUBLE) && yarg_rank(iarg) == 0) {
        // Fast scalar.
        if (type == Y_INT) {
            e->type = SCALAR_INT;
            e->val.i = ygets_i(iarg);
        } else if (type == Y_LONG) {
            e->type = SCALAR_LONG;
            e->val.l = ygets_l(iarg);
        } else /* Y_DOUBLE */ {
            e->type = SCALAR_DOUBLE;
            e->val.d = ygets_d(iarg);
        }
    } else {
        // Store as an object.
        e->val.o = type == Y_VOID ? NULL : yget_use(iarg);
        if (e->val.o != NULL) {
            e->type = OBJECT; // do this last in case of interupts
        }
    }
}

// Discard anything stored by entry taking care of interupts and make it empty.
static inline void free_entry(entry* e)
{
    int old_type = e->type;
    e->type = NOTHING; // do this before in case of interupts
    if ((old_type == OBJECT) & (e->val.o != NULL)) {
        ydrop_use(e->val.o);
    }
    e->val.o = NULL;
}

// Called by Yorick when object is no longer referenced.
static void mvect_free(void* addr)
{
    mvect* vec = addr;
    if (vec->arr != NULL) {
        for (long i = 0; i < vec->len; ++i) {
            if ((vec->arr[i].type == OBJECT) & (vec->arr[i].val.o != NULL)) {
                ydrop_use(vec->arr[i].val.o);
            }
        }
        p_free(vec->arr);
    }
}

static void mvect_print(void* addr)
{
    mvect* vec = addr;
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "%ld", vec->len);
    buffer[sizeof(buffer)-1] = 0;
    y_print(MVECT_TYPE_NAME, 0);
    y_print(" (len = ", 0);
    y_print(buffer, 0);
    y_print(")", 1);
}

static void mvect_eval(void* addr, int argc)
{
    // Extract arguments and push current item on top of stack.
    if (argc < 1 || argc > 2) y_error("expecting one or two arguments");
    mvect* vec = addr;
    int type = yarg_typeid(argc - 1);
    if ((type == Y_VOID) & (argc == 1)) {
        // vec() yields its length.
        ypush_long(vec->len);
        return;
    }
    entry* e = &vec->arr[adjust_index(vec, ygets_l(argc - 1))];
    if (yarg_subroutine()) {
        // Do not push entry on top of stack.
        if (argc == 2) {
            // Discard old contents and replace by the item on top of the stack.
            free_entry(e);
            store_entry(e, 0);
        }
    } else {
        // Push entry on top of stack.
        bool forget = (argc == 2);
        push_entry(e, forget);
        if (argc == 2) {
            // Replace by new item which is now right below the top of the stack.
            store_entry(e, 1);
        }
    }
}

static void mvect_extract(void* addr, char* name)
{
    mvect* vec = addr;
    int c0 = name[0];
    if (c0 == 'l' && strcmp(name, "len") == 0) {
        ypush_long(vec->len);
        return;
    }
    y_error("unknown mixed vector member");
}

static y_userobj_t mvect_type = {
    MVECT_TYPE_NAME,
    mvect_free,
    mvect_print,
    mvect_eval,
    mvect_extract,
    NULL
};

// Push a new mixed vector on top of stack.
static mvect* push_mvect(long len)
{
    if (len < 0) y_error("invalid mixed vector length");
    mvect* vec = ypush_obj(&mvect_type, sizeof(mvect));
    long maxlen = ROUND_UP(MAX(len, MIN_SIZE), CHUNK_SIZE);
    size_t size = maxlen*sizeof(entry);
    vec->arr = p_malloc(size);
    if (vec->arr == NULL) y_error("not enough memory");
    memset(vec->arr, 0, size); // zero-fill all entries so that they are considered as being void
    vec->maxlen = maxlen;
    vec->len = len;
    return vec;
}

// Resize an existing mixed vector.
static mvect* resize_mvect(mvect* vec, long len)
{
    if (len < 0) y_error("invalid mixed vector length");

    // Determine the allocated size of the array of entries.
    long maxlen = vec->maxlen;
    long minlen = ROUND_UP(MAX(len, MIN_SIZE), CHUNK_SIZE);
    if (maxlen < len) {
        // Storage must be augmented. If growing by 150% is not sufficient,
        // adjust to requested length.
        maxlen += maxlen/2;
        if (maxlen < minlen) {
            maxlen = minlen;
        }
    } else if (maxlen > 2*minlen) {
        // Storage can be significantly reduced.
        maxlen = minlen;
    }

    // Delete entries if length is reduced.
    for (long i = vec->len - 1; i >= len; --i) {
        entry* e = &vec->arr[i];
        --vec->len;
        if ((e->type == OBJECT) & (e->val.o != NULL)) {
            e->type = NOTHING;
            ydrop_use(e->val.o);
        } else if (e->type != NOTHING) {
            e->type = NOTHING;
        }
        e->val.o = NULL;
    }

    // Resize array of entries if needed.
    if (maxlen != vec->maxlen) {
        // Reallocate array of entries and zero-fill all new entries so that
        // they are considered as being void.
        size_t newsize = maxlen*sizeof(entry);
        size_t oldsize = vec->maxlen*sizeof(entry);
        vec->arr = p_realloc(vec->arr, newsize);
        vec->maxlen = maxlen;
        if (oldsize < newsize) {
            memset((char*)vec->arr + oldsize, 0, newsize - oldsize);
        }
    }
    vec->len = len;
    return vec;
}

void Y_mvect_create(int argc)
{
    if (argc != 1) y_error("expecting exactly one argument");
    long len = ygets_l(0);
    push_mvect(len);
}

void Y_mvect_collect(int argc)
{
    mvect* vec = push_mvect(argc);
    // Stack index starts at `argc`, not `argc - 1`, because result has been
    // pushed on top of stack.
    for (int i = 0; argc > 0; ++i, --argc) {
        store_entry(&vec->arr[i], argc);
    }
}

void Y_mvect_store(int argc)
{
    if (argc != 3) y_error("expecting exactly three arguments");
    mvect* vec = yget_obj(2, &mvect_type);
    entry* e = &vec->arr[adjust_index(vec, ygets_l(1))];
    int iarg = 0; // position in stack of new item
    if (yarg_subroutine()) {
        // Just discard old item, taking care of interupts
        free_entry(e);
    } else {
        // Push old item on top of stack before replacing in mixed vector.
        push_entry(e, true);
        ++iarg;
    }
    // Store new item in mixed vector.
    store_entry(e, iarg);
}

void Y_mvect_resize(int argc)
{
    if (argc != 2) y_error("expecting exactly two arguments");
    mvect* vec = yget_obj(1, &mvect_type);
    long newlen = ygets_l(0);
    resize_mvect(vec, newlen);
    yarg_drop(1); // leave mixed vector on top of stack
}

void Y_mvect_push(int argc)
{
    if (argc < 1) y_error("expecting at least one argument");
    int iarg = argc - 1;
    mvect* vec = yget_obj(iarg, &mvect_type);
    if (argc > 1) {
        long oldlen = vec->len;
        long newlen = oldlen + iarg;
        resize_mvect(vec, newlen);
        vec->len = oldlen; // forget new slots, so that vec->len is always up to date
        for (long i = oldlen; i < newlen; ++i) {
            store_entry(&vec->arr[i], --iarg);
            ++vec->len;
        }
    }
    yarg_drop(argc - 1); // leave mixed vector on top of stack
}

void Y_is_mvect(int argc)
{
    if (argc != 1) y_error("expecting exactly one argument");
    const char* name = yget_obj(0, NULL);
    ypush_int(name == mvect_type.type_name ? 1 : 0);
}
