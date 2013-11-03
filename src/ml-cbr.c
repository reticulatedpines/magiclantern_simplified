#include <dryos.h>
#include <string.h>
#include <ml-cbr.h>

// Compile-time switches
// Enable debug printfs
#define ML_CBR_DEBUG 0

// Make structs smaller
#define ML_CBR_COMPRESSED 1

#define NODE_POOL_SIZE 255
#define RECORD_POOL_SIZE 255

#if ML_CBR_COMPRESSED
#define COMPRESSED __attribute__((__packed__, __aligned__))
#else
#define COMPRESSED
#endif

#define SEMAPHORE struct semaphore
#define SEMAPHORE_INIT(sem) do { sem = create_named_semaphore(#sem"_sem", 0); } while(0)
#define LOCK(x) do { ASSERT(initialized); take_semaphore((x), 0); } while(0)
#define UNLOCK give_semaphore

#if ML_CBR_DEBUG
#define dbg_printf(fmt,...) do { console_printf(fmt, ## __VA_ARGS__); } while(0)
#else
#define dbg_printf(fmt,...) do {} while(0)
#endif

struct cbr_node {
    cbr_func cbr;
    unsigned int priority;
    struct cbr_node * next;
} COMPRESSED;

struct cbr_record {
    char name[16];
    struct cbr_node * first;
} COMPRESSED;

struct cbr_node_arena {
    struct cbr_node pool[NODE_POOL_SIZE];
    struct cbr_node_arena * next;
} COMPRESSED;

struct cbr_record_arena {
    struct cbr_record pool[RECORD_POOL_SIZE];
    struct cbr_record_arena * next;
} COMPRESSED;

static int initialized = 0;

static struct cbr_node_arena * cbr_node_pool = NULL;
static struct cbr_record_arena * cbr_record_pool = NULL;

static SEMAPHORE * ml_cbr_lock = NULL;

static inline int fast_compare(const char * fst, const char * snd) {
    dbg_printf("Checking %s <-> %s\n", fst, snd);
    return ((*(int64_t*) fst) == (*(int64_t*) snd))
            &&
            ((*(int64_t*) fst + 8) == (*(int64_t*) snd + 8));
}

static inline struct cbr_node_arena * create_node_arena()
{
   struct cbr_node_arena * result = (struct cbr_node_arena *) malloc(sizeof (struct cbr_node_arena));
   result->next = NULL;
   memset(result->pool, 0, NODE_POOL_SIZE * sizeof(result->pool[0]));
   return result;
}

static inline struct cbr_record_arena * create_record_arena()
{
    struct cbr_record_arena * result = (struct cbr_record_arena *) malloc(sizeof (struct cbr_record_arena));
    result->next = NULL;
    memset(result->pool, 0, RECORD_POOL_SIZE * sizeof(result->pool[0]));
    return result;
}

static struct cbr_record * find_record(const char * event, unsigned int return_new) {
    ASSERT(event != NULL);
    struct cbr_record_arena * current = cbr_record_pool;
    struct cbr_record * first_free = NULL;
    while (current != NULL) {
        int i;
        for (i = 0; i < RECORD_POOL_SIZE; ++i) {
            if (current->pool[i].name[0] == '\0') {
                if (return_new && first_free == NULL) {
                    dbg_printf("Free record found @ %d\n", i);
                    first_free = &(current->pool[i]);
                }
            } else {
                if (fast_compare(event, current->pool[i].name)) {
                    return &(current->pool[i]);
                }
            }
        }
        current = current->next;
    }
    dbg_printf("No existing record found\n");
    if (first_free != NULL) {
        strncpy(first_free->name, event, 16);
        first_free->first = NULL;
        dbg_printf("%s\n", first_free->name);
    }
    return first_free;
}

static struct cbr_node * find_free_node() {
    struct cbr_node_arena * current = cbr_node_pool;
    while (current != NULL) {
        int i;
        for (i = 0; i < NODE_POOL_SIZE; i++) {
            if (current->pool[i].cbr == NULL) {
                return &(current->pool[i]);
            }
        }
        current = current->next;
    }
    return NULL;
}

static struct cbr_node_arena * expand_cbr_node_pool() {
    dbg_printf("WARNING EXPANDING CBR NODE POOL\n");
    struct cbr_node_arena * current = cbr_node_pool;
    while (current->next != NULL) {
        current = current->next;
    }
    current->next = create_node_arena();
    return current->next;
}

static int insert_cbr(struct cbr_record * record, cbr_func cbr, unsigned int prio) {
    ASSERT(record != NULL && cbr != NULL);
    struct cbr_node * new_node = find_free_node();
    if (new_node == NULL) {
        struct cbr_node_arena * new_arena = expand_cbr_node_pool();
        if (new_arena == NULL || &(new_arena->pool[0]) == NULL) {
            return -1;
        } else {
            new_node = &(new_arena->pool[0]);
        }
    }

    new_node->cbr = cbr;
    new_node->priority = prio;

    if (record->first == NULL) {
        dbg_printf("First record is new, assigning\n");
        record->first = new_node;
    } else {
        struct cbr_node * current = record->first;
        struct cbr_node * prev = NULL;

        while (current != NULL) {
            if (current->priority < prio) {
                if (prev == NULL) {
                    record->first = new_node;
                    new_node->next = current;
                    return 0;
                } else {
                    prev->next = new_node;
                    new_node->next = current;
                    return 0;
                }
            }
            prev = current;
            if (current->next == NULL) {
                current->next = new_node;
                return 0;
            } else {
                current = current->next;
            }
        }
        return -1;
    }
    return 0;
}

static struct cbr_record_arena * expand_cbr_record_pool() {
    dbg_printf("WARNING EXPANDING CBR RECORD POOL\n");
    struct cbr_record_arena * current = cbr_record_pool;
    while (current->next != NULL) {
        current = current->next;
    }
    current->next = create_record_arena();
    return current->next;
}

int ml_register_cbr(const char * event, cbr_func cbr, unsigned int prio) {
    ASSERT(event != NULL && cbr != NULL);
    int retval = -1;
    LOCK(ml_cbr_lock);
    ml_unregister_cbr(event, cbr);
    struct cbr_record * record = find_record(event, 1);
    if (record == NULL) {
        struct cbr_record_arena * new_arena = expand_cbr_record_pool();
        if (new_arena == NULL || &(new_arena->pool[0]) == NULL) {
            retval = -1;
            goto end;
        } else {
            record = &(new_arena->pool[0]);
        }
    }
    retval = insert_cbr(record, cbr, prio);
    end:
    UNLOCK(ml_cbr_lock);
    return retval;
}

int ml_unregister_cbr(const char* event, cbr_func cbr) {
    ASSERT(event != NULL && cbr != NULL);
    LOCK(ml_cbr_lock);
    struct cbr_record * record = find_record(event, 0);
    int retval = -1;
    int count = 0;
    if (record == NULL) {
        dbg_printf("Unknown event %s\n", event);
        retval = -1;
        goto end;
    }
    struct cbr_node * current = record->first;
    struct cbr_node * prev = NULL;
    while(current != NULL) {
        if(current->cbr == cbr) {
            if(prev == NULL)
            {
                record->first = current->next;
                //TODO: Possibile optimization: remove record if current->next is NULL
            } else {
                prev->next = current->next;
            }
            current->cbr = NULL;
            current->next = NULL;
            count++;
        }
        prev = current;
        current = current->next;
    }
    retval = 0;
end:
    dbg_printf("Removed %d CBRs\n", count);
    UNLOCK(ml_cbr_lock);
    return retval;
}

void ml_notify_cbr(const char * event, void * data) {
    ASSERT(event != NULL);
    LOCK(ml_cbr_lock);
    struct cbr_record * record = find_record(event, 0);
    if (record == NULL) {
        return;
    }
    struct cbr_node * call = record->first;
    while (call != NULL) {
        if (call->cbr != NULL) {
            if (call->cbr(event, data) == ML_CBR_STOP) {
                break;
            }
        }
        call = call->next;
    }
    UNLOCK(ml_cbr_lock);
}

void debug_cbr_tree(const char * event) {
    ASSERT(event != NULL);
    LOCK(ml_cbr_lock);
    struct cbr_record * record = find_record(event, 0);
    struct cbr_node * node = record->first;
    while (node != NULL) {
        dbg_printf("P:%d\tCBR@0x%x\n", node->priority, (void*)node->cbr);
        node = node->next;
    }
    UNLOCK(ml_cbr_lock);
}

void _ml_cbr_init() {
    ASSERT(!initialized);
    ASSERT(cbr_node_pool == NULL);
    ASSERT(cbr_record_pool == NULL);
    ASSERT(ml_cbr_lock == NULL);
    cbr_node_pool = create_node_arena();
    cbr_record_pool = create_record_arena();
    SEMAPHORE_INIT(ml_cbr_lock);
    initialized = 1;
}
