#pragma once

#include <stdint.h>
#include <stddef.h>

/**
 * @brief Common Mifare key database
 *
 * Contains well-known default keys used in Mifare Classic tags
 */

// Key entry structure
typedef struct {
    const char* name;
    const uint8_t key[6];
    const char* description;
} MifareKeyEntry;

/**
 * @brief Get the number of keys in the database
 * @return Number of keys
 */
size_t mifare_keys_db_get_count(void);

/**
 * @brief Get a key entry by index
 * @param index Index of the key (0 to count-1)
 * @return Pointer to key entry, or NULL if invalid index
 */
const MifareKeyEntry* mifare_keys_db_get_key(size_t index);

/**
 * @brief Find a key by name
 * @param name Name of the key
 * @return Pointer to key entry, or NULL if not found
 */
const MifareKeyEntry* mifare_keys_db_find_by_name(const char* name);

/**
 * @brief Check if a key matches any in the database
 * @param key 6-byte key to check
 * @return Pointer to matching entry, or NULL if not found
 */
const MifareKeyEntry* mifare_keys_db_find_by_key(const uint8_t key[6]);

/**
 * @brief Export all keys to a file
 * @param filepath Path to export file
 * @return true if successful
 */
bool mifare_keys_db_export_to_file(const char* filepath);

/**
 * @brief Import keys from a file (NFC Tools / Proxmark format)
 * @param filepath Path to import file
 * @param callback Callback for each imported key (name, key_data)
 * @param context Context passed to callback
 * @return Number of keys imported
 */
size_t mifare_keys_db_import_from_file(
    const char* filepath,
    void (*callback)(const char* name, const uint8_t key[6], void* context),
    void* context);
