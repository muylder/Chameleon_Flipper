#include "key_manager.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define TAG "KeyManager"

struct KeyManager {
    KeyEntry keys[MAX_KEYS];
    size_t count;
    FuriMutex* mutex;
};

// ============================================================================
// Default Mifare Classic Keys (Most Common)
// ============================================================================
// These are the most commonly used default keys for Mifare Classic cards
// Sources: mfoc, mfcuk, public key databases
// ============================================================================

typedef struct {
    const char* name;
    const uint8_t key[KEY_LENGTH];
} DefaultKey;

static const DefaultKey default_keys[] = {
    // Standard factory default
    {"Factory Default", {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}},

    // MAD (Mifare Application Directory)
    {"MAD Key A", {0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5}},
    {"MAD Key B", {0xB0, 0xB1, 0xB2, 0xB3, 0xB4, 0xB5}},

    // Transport tickets
    {"Transport 1", {0x00, 0x00, 0x00, 0x00, 0x00, 0x00}},
    {"Transport 2", {0xA0, 0xB0, 0xC0, 0xD0, 0xE0, 0xF0}},
    {"Transport 3", {0xA1, 0xB1, 0xC1, 0xD1, 0xE1, 0xF1}},

    // Hotel/Access cards
    {"Hotel 1", {0x01, 0x02, 0x03, 0x04, 0x05, 0x06}},
    {"Hotel 2", {0x48, 0x54, 0x4C, 0x4B, 0x45, 0x59}}, // "HTLKEY"

    // Common patterns
    {"All 0xAA", {0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA}},
    {"All 0xBB", {0xBB, 0xBB, 0xBB, 0xBB, 0xBB, 0xBB}},
    {"Sequence 1", {0x01, 0x23, 0x45, 0x67, 0x89, 0xAB}},
    {"Sequence 2", {0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC}},

    // NFC tools
    {"NFC Tools", {0xD3, 0xF7, 0xD3, 0xF7, 0xD3, 0xF7}},

    // Common custom
    {"Custom 1", {0x4D, 0x3A, 0x99, 0xC3, 0x51, 0xDD}},
    {"Custom 2", {0x1A, 0x98, 0x2C, 0x7E, 0x45, 0x9A}},

    // NDEF (NFC Forum)
    {"NDEF MAD", {0xD3, 0xF7, 0xD3, 0xF7, 0xD3, 0xF7}},
};

static const size_t default_keys_count = sizeof(default_keys) / sizeof(DefaultKey);

// ============================================================================
// Key Manager Implementation
// ============================================================================

KeyManager* key_manager_alloc() {
    KeyManager* manager = malloc(sizeof(KeyManager));
    memset(manager, 0, sizeof(KeyManager));

    manager->mutex = furi_mutex_alloc(FuriMutexTypeNormal);
    manager->count = 0;

    FURI_LOG_I(TAG, "Key manager allocated");

    return manager;
}

void key_manager_free(KeyManager* manager) {
    furi_assert(manager);

    furi_mutex_free(manager->mutex);
    free(manager);

    FURI_LOG_I(TAG, "Key manager freed");
}

bool key_manager_add_key(KeyManager* manager, const uint8_t* key, KeyType type, const char* name) {
    furi_assert(manager);
    furi_assert(key);

    furi_mutex_acquire(manager->mutex, FuriWaitForever);

    if(manager->count >= MAX_KEYS) {
        FURI_LOG_W(TAG, "Key database full");
        furi_mutex_release(manager->mutex);
        return false;
    }

    // Check if key already exists
    if(key_manager_key_exists(manager, key)) {
        FURI_LOG_W(TAG, "Key already exists");
        furi_mutex_release(manager->mutex);
        return false;
    }

    KeyEntry* entry = &manager->keys[manager->count];
    memcpy(entry->key, key, KEY_LENGTH);
    entry->type = type;
    entry->valid = true;

    if(name) {
        strncpy(entry->name, name, KEY_NAME_MAX_LEN - 1);
        entry->name[KEY_NAME_MAX_LEN - 1] = '\0';
    } else {
        snprintf(entry->name, KEY_NAME_MAX_LEN, "Key %zu", manager->count);
    }

    manager->count++;

    FURI_LOG_I(TAG, "Key added: %s (total: %zu)", entry->name, manager->count);

    furi_mutex_release(manager->mutex);
    return true;
}

bool key_manager_remove_key(KeyManager* manager, size_t index) {
    furi_assert(manager);

    furi_mutex_acquire(manager->mutex, FuriWaitForever);

    if(index >= manager->count) {
        FURI_LOG_E(TAG, "Invalid index: %zu", index);
        furi_mutex_release(manager->mutex);
        return false;
    }

    // Shift keys down
    for(size_t i = index; i < manager->count - 1; i++) {
        memcpy(&manager->keys[i], &manager->keys[i + 1], sizeof(KeyEntry));
    }

    manager->count--;

    FURI_LOG_I(TAG, "Key removed at index %zu (remaining: %zu)", index, manager->count);

    furi_mutex_release(manager->mutex);
    return true;
}

void key_manager_clear_all(KeyManager* manager) {
    furi_assert(manager);

    furi_mutex_acquire(manager->mutex, FuriWaitForever);

    memset(manager->keys, 0, sizeof(manager->keys));
    manager->count = 0;

    FURI_LOG_I(TAG, "All keys cleared");

    furi_mutex_release(manager->mutex);
}

size_t key_manager_get_count(KeyManager* manager) {
    furi_assert(manager);

    furi_mutex_acquire(manager->mutex, FuriWaitForever);
    size_t count = manager->count;
    furi_mutex_release(manager->mutex);

    return count;
}

bool key_manager_get_key(KeyManager* manager, size_t index, KeyEntry* entry) {
    furi_assert(manager);
    furi_assert(entry);

    furi_mutex_acquire(manager->mutex, FuriWaitForever);

    if(index >= manager->count) {
        furi_mutex_release(manager->mutex);
        return false;
    }

    memcpy(entry, &manager->keys[index], sizeof(KeyEntry));

    furi_mutex_release(manager->mutex);
    return true;
}

bool key_manager_find_key(KeyManager* manager, const uint8_t* key, size_t* index) {
    furi_assert(manager);
    furi_assert(key);

    furi_mutex_acquire(manager->mutex, FuriWaitForever);

    for(size_t i = 0; i < manager->count; i++) {
        if(memcmp(manager->keys[i].key, key, KEY_LENGTH) == 0) {
            if(index) *index = i;
            furi_mutex_release(manager->mutex);
            return true;
        }
    }

    furi_mutex_release(manager->mutex);
    return false;
}

bool key_manager_key_exists(KeyManager* manager, const uint8_t* key) {
    return key_manager_find_key(manager, key, NULL);
}

void key_manager_load_defaults(KeyManager* manager) {
    furi_assert(manager);

    FURI_LOG_I(TAG, "Loading %zu default keys", default_keys_count);

    for(size_t i = 0; i < default_keys_count; i++) {
        key_manager_add_key(
            manager,
            default_keys[i].key,
            KeyTypeA,  // Most defaults are Key A
            default_keys[i].name);
    }

    FURI_LOG_I(TAG, "Default keys loaded: %zu", key_manager_get_count(manager));
}

bool key_manager_import_from_file(KeyManager* manager, const char* filepath) {
    furi_assert(manager);
    furi_assert(filepath);

    FURI_LOG_I(TAG, "Importing keys from: %s", filepath);

    Storage* storage = furi_record_open(RECORD_STORAGE);
    File* file = storage_file_alloc(storage);

    if(!storage_file_open(file, filepath, FSAM_READ, FSOM_OPEN_EXISTING)) {
        FURI_LOG_E(TAG, "Failed to open file for import");
        storage_file_free(file);
        furi_record_close(RECORD_STORAGE);
        return false;
    }

    char line[128];
    size_t imported = 0;

    while(storage_file_read(file, line, sizeof(line)) > 0) {
        // Parse line: "FFFFFFFFFFFF,Factory Default,A"
        // Format: KEY(12 hex chars),NAME,TYPE

        char* key_str = strtok(line, ",");
        char* name_str = strtok(NULL, ",");
        char* type_str = strtok(NULL, ",\n\r");

        if(!key_str || strlen(key_str) != 12) continue;

        uint8_t key[KEY_LENGTH];
        if(!key_manager_parse_key(key_str, key)) continue;

        KeyType type = KeyTypeA;
        if(type_str && (type_str[0] == 'B' || type_str[0] == 'b')) {
            type = KeyTypeB;
        }

        if(key_manager_add_key(manager, key, type, name_str ? name_str : "Imported")) {
            imported++;
        }
    }

    storage_file_close(file);
    storage_file_free(file);
    furi_record_close(RECORD_STORAGE);

    FURI_LOG_I(TAG, "Keys imported: %zu", imported);
    return imported > 0;
}

bool key_manager_export_to_file(KeyManager* manager, const char* filepath) {
    furi_assert(manager);
    furi_assert(filepath);

    FURI_LOG_I(TAG, "Exporting keys to: %s", filepath);

    Storage* storage = furi_record_open(RECORD_STORAGE);
    File* file = storage_file_alloc(storage);

    if(!storage_file_open(file, filepath, FSAM_WRITE, FSOM_CREATE_ALWAYS)) {
        FURI_LOG_E(TAG, "Failed to open file for export");
        storage_file_free(file);
        furi_record_close(RECORD_STORAGE);
        return false;
    }

    // Write header
    const char* header = "# Mifare Classic Key Database\n"
                         "# Format: KEY(12 hex),NAME,TYPE(A/B)\n\n";
    storage_file_write(file, header, strlen(header));

    furi_mutex_acquire(manager->mutex, FuriWaitForever);

    for(size_t i = 0; i < manager->count; i++) {
        KeyEntry* entry = &manager->keys[i];
        if(!entry->valid) continue;

        char line[128];
        char key_str[16];
        key_manager_format_key(entry->key, key_str, sizeof(key_str));

        int len = snprintf(
            line,
            sizeof(line),
            "%s,%s,%c\n",
            key_str,
            entry->name,
            entry->type == KeyTypeA ? 'A' : 'B');

        storage_file_write(file, line, len);
    }

    furi_mutex_release(manager->mutex);

    storage_file_close(file);
    storage_file_free(file);
    furi_record_close(RECORD_STORAGE);

    FURI_LOG_I(TAG, "Keys exported: %zu", manager->count);
    return true;
}

size_t key_manager_test_keys(
    KeyManager* manager,
    KeyTestCallback callback,
    void* context,
    uint8_t* found_key,
    KeyType* found_type) {
    furi_assert(manager);
    furi_assert(callback);

    FURI_LOG_I(TAG, "Testing %zu keys", manager->count);

    furi_mutex_acquire(manager->mutex, FuriWaitForever);

    for(size_t i = 0; i < manager->count; i++) {
        KeyEntry* entry = &manager->keys[i];
        if(!entry->valid) continue;

        if(callback(entry->key, entry->type, context)) {
            // Key found!
            if(found_key) memcpy(found_key, entry->key, KEY_LENGTH);
            if(found_type) *found_type = entry->type;

            FURI_LOG_I(TAG, "Key found at index %zu: %s", i, entry->name);

            furi_mutex_release(manager->mutex);
            return i;
        }
    }

    furi_mutex_release(manager->mutex);

    FURI_LOG_W(TAG, "No key found after testing %zu keys", manager->count);
    return SIZE_MAX; // Not found
}

void key_manager_format_key(const uint8_t* key, char* buffer, size_t buffer_size) {
    furi_assert(key);
    furi_assert(buffer);

    snprintf(
        buffer,
        buffer_size,
        "%02X%02X%02X%02X%02X%02X",
        key[0], key[1], key[2], key[3], key[4], key[5]);
}

bool key_manager_parse_key(const char* str, uint8_t* key) {
    furi_assert(str);
    furi_assert(key);

    if(strlen(str) != 12) return false;

    for(int i = 0; i < KEY_LENGTH; i++) {
        char byte_str[3] = {str[i * 2], str[i * 2 + 1], '\0'};
        char* endptr;
        long val = strtol(byte_str, &endptr, 16);

        if(*endptr != '\0' || val < 0 || val > 0xFF) {
            return false;
        }

        key[i] = (uint8_t)val;
    }

    return true;
}
