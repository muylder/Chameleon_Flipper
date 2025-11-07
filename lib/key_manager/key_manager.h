#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <furi.h>
#include <storage/storage.h>

#define KEY_LENGTH 6  // Mifare Classic key = 6 bytes
#define MAX_KEYS 256  // Maximum keys in database
#define KEY_NAME_MAX_LEN 32

// Key type (Mifare Classic)
typedef enum {
    KeyTypeA = 0,
    KeyTypeB = 1,
} KeyType;

// Key entry structure
typedef struct {
    uint8_t key[KEY_LENGTH];
    char name[KEY_NAME_MAX_LEN];
    KeyType type;
    bool valid;
} KeyEntry;

// Key manager instance
typedef struct KeyManager KeyManager;

// Create/destroy
KeyManager* key_manager_alloc();
void key_manager_free(KeyManager* manager);

// Key database operations
bool key_manager_add_key(KeyManager* manager, const uint8_t* key, KeyType type, const char* name);
bool key_manager_remove_key(KeyManager* manager, size_t index);
void key_manager_clear_all(KeyManager* manager);
size_t key_manager_get_count(KeyManager* manager);
bool key_manager_get_key(KeyManager* manager, size_t index, KeyEntry* entry);

// Search
bool key_manager_find_key(KeyManager* manager, const uint8_t* key, size_t* index);
bool key_manager_key_exists(KeyManager* manager, const uint8_t* key);

// Load default keys (common Mifare keys)
void key_manager_load_defaults(KeyManager* manager);

// Import/Export
bool key_manager_import_from_file(KeyManager* manager, const char* filepath);
bool key_manager_export_to_file(KeyManager* manager, const char* filepath);

// Key testing (for Mifare Classic attack)
typedef bool (*KeyTestCallback)(const uint8_t* key, KeyType type, void* context);
size_t key_manager_test_keys(
    KeyManager* manager,
    KeyTestCallback callback,
    void* context,
    uint8_t* found_key,
    KeyType* found_type);

// Key formatting
void key_manager_format_key(const uint8_t* key, char* buffer, size_t buffer_size);
bool key_manager_parse_key(const char* str, uint8_t* key);
