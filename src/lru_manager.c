
typedef struct lru_entry {
    void *value;
    struct lru_entry *previous;
    struct lru_entry *next;

} lru_entry_t;

typedef struct lru_manager {
} lru_manager_t;
