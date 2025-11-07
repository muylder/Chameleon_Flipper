#include "tag_collection.h"
#include <string.h>
#include <storage/storage.h>
#include <datetime.h>

#define COLLECTION_FILE_PATH "/ext/apps_data/chameleon_ultra/tag_collection.dat"
#define COLLECTION_VERSION 1

struct TagCollection {
    CollectionTag tags[MAX_COLLECTION_TAGS];
    size_t count;
    FuriMutex* mutex;
};

static const char* category_names[] = {
    "Uncategorized",
    "Hotel",
    "Transport",
    "Access Control",
    "Home",
    "Office",
    "Gym",
    "Other",
};

static const char* category_icons[] = {
    "?",
    "H",
    "T",
    "A",
    "*",
    "O",
    "G",
    "+",
};

TagCollection* tag_collection_alloc(void) {
    TagCollection* collection = malloc(sizeof(TagCollection));
    memset(collection, 0, sizeof(TagCollection));

    collection->mutex = furi_mutex_alloc(FuriMutexTypeNormal);
    collection->count = 0;

    return collection;
}

void tag_collection_free(TagCollection* collection) {
    if(!collection) return;

    furi_mutex_free(collection->mutex);
    free(collection);
}

size_t tag_collection_load(TagCollection* collection) {
    if(!collection) return 0;

    furi_mutex_acquire(collection->mutex, FuriWaitForever);

    Storage* storage = furi_record_open(RECORD_STORAGE);
    File* file = storage_file_alloc(storage);

    size_t loaded = 0;

    if(storage_file_open(file, COLLECTION_FILE_PATH, FSAM_READ, FSOM_OPEN_EXISTING)) {
        uint32_t version;
        if(storage_file_read(file, &version, sizeof(uint32_t)) == sizeof(uint32_t)) {
            if(version == COLLECTION_VERSION) {
                size_t count;
                if(storage_file_read(file, &count, sizeof(size_t)) == sizeof(size_t)) {
                    if(count <= MAX_COLLECTION_TAGS) {
                        size_t read = storage_file_read(
                            file,
                            collection->tags,
                            count * sizeof(CollectionTag));

                        if(read == count * sizeof(CollectionTag)) {
                            collection->count = count;
                            loaded = count;
                        }
                    }
                }
            }
        }
        storage_file_close(file);
    }

    storage_file_free(file);
    furi_record_close(RECORD_STORAGE);

    furi_mutex_release(collection->mutex);

    return loaded;
}

bool tag_collection_save(TagCollection* collection) {
    if(!collection) return false;

    furi_mutex_acquire(collection->mutex, FuriWaitForever);

    Storage* storage = furi_record_open(RECORD_STORAGE);

    storage_common_mkdir(storage, "/ext/apps_data/chameleon_ultra");

    File* file = storage_file_alloc(storage);
    bool success = false;

    if(storage_file_open(file, COLLECTION_FILE_PATH, FSAM_WRITE, FSOM_CREATE_ALWAYS)) {
        uint32_t version = COLLECTION_VERSION;
        storage_file_write(file, &version, sizeof(uint32_t));

        storage_file_write(file, &collection->count, sizeof(size_t));

        size_t written = storage_file_write(
            file,
            collection->tags,
            collection->count * sizeof(CollectionTag));

        if(written == collection->count * sizeof(CollectionTag)) {
            success = true;
        }

        storage_file_close(file);
    }

    storage_file_free(file);
    furi_record_close(RECORD_STORAGE);

    furi_mutex_release(collection->mutex);

    return success;
}

bool tag_collection_add(TagCollection* collection, const CollectionTag* tag) {
    if(!collection || !tag) return false;

    furi_mutex_acquire(collection->mutex, FuriWaitForever);

    if(collection->count >= MAX_COLLECTION_TAGS) {
        furi_mutex_release(collection->mutex);
        return false;
    }

    memcpy(&collection->tags[collection->count], tag, sizeof(CollectionTag));
    collection->tags[collection->count].date_added = furi_hal_rtc_get_timestamp();
    collection->tags[collection->count].use_count = 0;
    collection->tags[collection->count].assigned_slot = 0xFF; // No slot

    collection->count++;

    furi_mutex_release(collection->mutex);

    return true;
}

bool tag_collection_remove(TagCollection* collection, size_t index) {
    if(!collection || index >= collection->count) return false;

    furi_mutex_acquire(collection->mutex, FuriWaitForever);

    // Shift remaining tags
    for(size_t i = index; i < collection->count - 1; i++) {
        memcpy(&collection->tags[i], &collection->tags[i + 1], sizeof(CollectionTag));
    }

    collection->count--;

    furi_mutex_release(collection->mutex);

    return true;
}

const CollectionTag* tag_collection_get(TagCollection* collection, size_t index) {
    if(!collection || index >= collection->count) return NULL;

    return &collection->tags[index];
}

size_t tag_collection_get_count(TagCollection* collection) {
    if(!collection) return 0;
    return collection->count;
}

size_t tag_collection_find_by_name(
    TagCollection* collection,
    const char* name,
    size_t* results,
    size_t max_results) {
    if(!collection || !name || !results) return 0;

    furi_mutex_acquire(collection->mutex, FuriWaitForever);

    size_t found = 0;

    for(size_t i = 0; i < collection->count && found < max_results; i++) {
        if(strstr(collection->tags[i].name, name) != NULL) {
            results[found++] = i;
        }
    }

    furi_mutex_release(collection->mutex);

    return found;
}

size_t tag_collection_find_by_category(
    TagCollection* collection,
    TagCategory category,
    size_t* results,
    size_t max_results) {
    if(!collection || !results) return 0;

    furi_mutex_acquire(collection->mutex, FuriWaitForever);

    size_t found = 0;

    for(size_t i = 0; i < collection->count && found < max_results; i++) {
        if(collection->tags[i].category == category) {
            results[found++] = i;
        }
    }

    furi_mutex_release(collection->mutex);

    return found;
}

size_t tag_collection_get_favorites(
    TagCollection* collection,
    size_t* results,
    size_t max_results) {
    if(!collection || !results) return 0;

    furi_mutex_acquire(collection->mutex, FuriWaitForever);

    size_t found = 0;

    for(size_t i = 0; i < collection->count && found < max_results; i++) {
        if(collection->tags[i].is_favorite) {
            results[found++] = i;
        }
    }

    furi_mutex_release(collection->mutex);

    return found;
}

bool tag_collection_toggle_favorite(TagCollection* collection, size_t index) {
    if(!collection || index >= collection->count) return false;

    furi_mutex_acquire(collection->mutex, FuriWaitForever);

    collection->tags[index].is_favorite = !collection->tags[index].is_favorite;
    bool new_status = collection->tags[index].is_favorite;

    furi_mutex_release(collection->mutex);

    return new_status;
}

bool tag_collection_update(TagCollection* collection, size_t index, const CollectionTag* tag) {
    if(!collection || !tag || index >= collection->count) return false;

    furi_mutex_acquire(collection->mutex, FuriWaitForever);

    // Preserve some metadata
    uint32_t date_added = collection->tags[index].date_added;
    uint32_t use_count = collection->tags[index].use_count;

    memcpy(&collection->tags[index], tag, sizeof(CollectionTag));

    collection->tags[index].date_added = date_added;
    collection->tags[index].use_count = use_count;

    furi_mutex_release(collection->mutex);

    return true;
}

bool tag_collection_deploy_to_slot(TagCollection* collection, size_t index, uint8_t slot) {
    if(!collection || index >= collection->count || slot > 7) return false;

    furi_mutex_acquire(collection->mutex, FuriWaitForever);

    // TODO: Actually write tag data to Chameleon slot

    collection->tags[index].assigned_slot = slot;
    collection->tags[index].last_used = furi_hal_rtc_get_timestamp();
    collection->tags[index].use_count++;

    furi_mutex_release(collection->mutex);

    return true;
}

static int compare_favorites(const void* a, const void* b) {
    const CollectionTag* tag_a = (const CollectionTag*)a;
    const CollectionTag* tag_b = (const CollectionTag*)b;

    return tag_b->is_favorite - tag_a->is_favorite;
}

static int compare_date(const void* a, const void* b) {
    const CollectionTag* tag_a = (const CollectionTag*)a;
    const CollectionTag* tag_b = (const CollectionTag*)b;

    return (int)(tag_b->date_added - tag_a->date_added);
}

static int compare_name(const void* a, const void* b) {
    const CollectionTag* tag_a = (const CollectionTag*)a;
    const CollectionTag* tag_b = (const CollectionTag*)b;

    return strcmp(tag_a->name, tag_b->name);
}

void tag_collection_sort(
    TagCollection* collection,
    bool by_favorites,
    bool by_date,
    bool by_name) {
    if(!collection || collection->count == 0) return;

    furi_mutex_acquire(collection->mutex, FuriWaitForever);

    // Simple sorting - can be enhanced with multi-key sort
    if(by_favorites) {
        qsort(collection->tags, collection->count, sizeof(CollectionTag), compare_favorites);
    } else if(by_date) {
        qsort(collection->tags, collection->count, sizeof(CollectionTag), compare_date);
    } else if(by_name) {
        qsort(collection->tags, collection->count, sizeof(CollectionTag), compare_name);
    }

    furi_mutex_release(collection->mutex);
}

bool tag_collection_export(TagCollection* collection, const char* filepath) {
    if(!collection || !filepath) return false;

    Storage* storage = furi_record_open(RECORD_STORAGE);
    File* file = storage_file_alloc(storage);

    bool success = false;

    if(storage_file_open(file, filepath, FSAM_WRITE, FSOM_CREATE_ALWAYS)) {
        FuriString* export_text = furi_string_alloc();

        furi_string_cat_printf(export_text, "# Chameleon Ultra Tag Collection Export\n\n");
        furi_string_cat_printf(export_text, "Total Tags: %zu\n\n", collection->count);

        for(size_t i = 0; i < collection->count; i++) {
            const CollectionTag* tag = &collection->tags[i];

            furi_string_cat_printf(export_text, "[Tag %zu]\n", i + 1);
            furi_string_cat_printf(export_text, "Name: %s\n", tag->name);
            furi_string_cat_printf(export_text, "Category: %s\n", tag->category_name);
            furi_string_cat_printf(export_text, "Favorite: %s\n", tag->is_favorite ? "YES" : "NO");

            furi_string_cat_printf(export_text, "UID: ");
            for(uint8_t j = 0; j < tag->uid_len; j++) {
                furi_string_cat_printf(export_text, "%02X", tag->uid[j]);
            }
            furi_string_cat_printf(export_text, "\n");

            furi_string_cat_printf(export_text, "Uses: %lu\n", tag->use_count);
            furi_string_cat_printf(export_text, "Notes: %s\n\n", tag->notes);
        }

        storage_file_write(file, furi_string_get_cstr(export_text), furi_string_size(export_text));

        furi_string_free(export_text);
        storage_file_close(file);
        success = true;
    }

    storage_file_free(file);
    furi_record_close(RECORD_STORAGE);

    return success;
}

size_t tag_collection_import(TagCollection* collection, const char* filepath) {
    // TODO: Implement import parser
    UNUSED(collection);
    UNUSED(filepath);
    return 0;
}

const char* tag_collection_get_category_name(TagCategory category) {
    if(category >= 0 && category < sizeof(category_names) / sizeof(category_names[0])) {
        return category_names[category];
    }
    return "Unknown";
}

const char* tag_collection_get_category_icon(TagCategory category) {
    if(category >= 0 && category < sizeof(category_icons) / sizeof(category_icons[0])) {
        return category_icons[category];
    }
    return "?";
}
