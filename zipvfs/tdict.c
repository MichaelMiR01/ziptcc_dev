typedef struct dict_entry_s {
    char *key;
    void* value;
} dict_entry_s;

typedef struct dict_s {
    int len;
    int cap;
    dict_entry_s *entry;
} dict_s, *dict_t;

int dict_find_index(dict_t dict, const char *key) {
    for (int i = 0; i < dict->len; i++) {
        if (!strcmp(dict->entry[i].key, key)) {
            return i;
        }
    }
    return -1;
}

void* dict_find(dict_t dict, const char *key) {
    int idx = dict_find_index(dict, key);
    return idx == -1 ? NULL : dict->entry[idx].value;
}

void dict_add(dict_t dict, const char *key, void* value) {
   int idx = dict_find_index(dict, key);
   if (idx != -1) {
       dict->entry[idx].value = value;
       return;
   }
   if (dict->len == dict->cap) {
       dict->cap *= 2;
       dict->entry = tcc_realloc(dict->entry, dict->cap * sizeof(dict_entry_s));
   }
   dict->entry[dict->len].key = tcc_strdup(key);
   dict->entry[dict->len].value = value;
   dict->len++;
}

dict_t dict_new(void) {
    dict_s proto = {0, 10, tcc_malloc(10 * sizeof(dict_entry_s))};
    dict_t d = tcc_malloc(sizeof(dict_s));
    *d = proto;
    return d;
}

void dict_free(dict_t dict, void (*delete)(void*)) {
    for (int i = 0; i < dict->len; i++) {
        if((delete!=NULL)&&(dict->entry[i].value!=NULL)) {
            (delete)(dict->entry[i].value);
            dict->entry[i].value=NULL;
        }
        tcc_free(dict->entry[i].key);
    }
    tcc_free(dict->entry);
    tcc_free(dict);
}
