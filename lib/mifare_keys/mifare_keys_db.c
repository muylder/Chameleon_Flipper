#include "mifare_keys_db.h"
#include <string.h>
#include <storage/storage.h>
#include <toolbox/hex.h>

// Database of common Mifare Classic keys
static const MifareKeyEntry mifare_keys_database[] = {
    // Default keys
    {
        .name = "Default FF",
        .key = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
        .description = "Factory default key (all 0xFF)",
    },
    {
        .name = "Default 00",
        .key = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
        .description = "Blank key (all 0x00)",
    },

    // MAD (Mifare Application Directory) keys
    {
        .name = "MAD Key A",
        .key = {0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5},
        .description = "MAD sector key A",
    },
    {
        .name = "MAD Key B",
        .key = {0xB0, 0xB1, 0xB2, 0xB3, 0xB4, 0xB5},
        .description = "MAD sector key B",
    },

    // NFC Forum keys
    {
        .name = "NDEF Key",
        .key = {0xD3, 0xF7, 0xD3, 0xF7, 0xD3, 0xF7},
        .description = "NFC Forum MAD key",
    },

    // Common vendor/transport keys
    {
        .name = "NXP Default",
        .key = {0xA0, 0xB0, 0xC0, 0xD0, 0xE0, 0xF0},
        .description = "NXP factory default",
    },
    {
        .name = "Infineon Default",
        .key = {0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC},
        .description = "Infineon MIFARE key",
    },

    // Public transport keys (educational purposes)
    {
        .name = "Transport 1",
        .key = {0x00, 0x00, 0x00, 0x00, 0x00, 0x01},
        .description = "Common transport key variant",
    },
    {
        .name = "Transport 2",
        .key = {0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5},
        .description = "Public transport default",
    },

    // Hotel/access control keys
    {
        .name = "Hotel 1",
        .key = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
        .description = "Hotel key card default",
    },
    {
        .name = "Access 1",
        .key = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
        .description = "Access control default",
    },

    // Vigik (French access control)
    {
        .name = "Vigik",
        .key = {0xA2, 0x98, 0x38, 0x39, 0x73, 0x69},
        .description = "Vigik system key",
    },

    // Common patterns
    {
        .name = "Pattern AA",
        .key = {0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA},
        .description = "All 0xAA pattern",
    },
    {
        .name = "Pattern BB",
        .key = {0xBB, 0xBB, 0xBB, 0xBB, 0xBB, 0xBB},
        .description = "All 0xBB pattern",
    },
    {
        .name = "Pattern 11",
        .key = {0x11, 0x11, 0x11, 0x11, 0x11, 0x11},
        .description = "All 0x11 pattern",
    },

    // Incremental patterns
    {
        .name = "Incremental",
        .key = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06},
        .description = "Sequential 01-06",
    },
    {
        .name = "Reverse",
        .key = {0x06, 0x05, 0x04, 0x03, 0x02, 0x01},
        .description = "Reverse sequential",
    },

    // MIFARE Plus defaults
    {
        .name = "MIFARE Plus SL1",
        .key = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
        .description = "MIFARE Plus Security Level 1",
    },

    // Additional common keys
    {
        .name = "Key 123456",
        .key = {0x12, 0x34, 0x56, 0x78, 0x90, 0xAB},
        .description = "Common test key",
    },
    {
        .name = "Key ABCDEF",
        .key = {0xAB, 0xCD, 0xEF, 0x12, 0x34, 0x56},
        .description = "Common test key variant",
    },

    // Backdoor/debugging keys (some known exploits)
    {
        .name = "Debug 1",
        .key = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55},
        .description = "Debug/testing key",
    },
    {
        .name = "Debug 2",
        .key = {0x55, 0x44, 0x33, 0x22, 0x11, 0x00},
        .description = "Debug/testing key reverse",
    },
};

static const size_t MIFARE_KEYS_DB_COUNT =
    sizeof(mifare_keys_database) / sizeof(mifare_keys_database[0]);

size_t mifare_keys_db_get_count(void) {
    return MIFARE_KEYS_DB_COUNT;
}

const MifareKeyEntry* mifare_keys_db_get_key(size_t index) {
    if(index >= MIFARE_KEYS_DB_COUNT) {
        return NULL;
    }
    return &mifare_keys_database[index];
}

const MifareKeyEntry* mifare_keys_db_find_by_name(const char* name) {
    if(!name) return NULL;

    for(size_t i = 0; i < MIFARE_KEYS_DB_COUNT; i++) {
        if(strcmp(mifare_keys_database[i].name, name) == 0) {
            return &mifare_keys_database[i];
        }
    }
    return NULL;
}

const MifareKeyEntry* mifare_keys_db_find_by_key(const uint8_t key[6]) {
    if(!key) return NULL;

    for(size_t i = 0; i < MIFARE_KEYS_DB_COUNT; i++) {
        if(memcmp(mifare_keys_database[i].key, key, 6) == 0) {
            return &mifare_keys_database[i];
        }
    }
    return NULL;
}

bool mifare_keys_db_export_to_file(const char* filepath) {
    if(!filepath) return false;

    Storage* storage = furi_record_open(RECORD_STORAGE);
    File* file = storage_file_alloc(storage);

    bool success = false;

    // Create directory if needed
    storage_common_mkdir(storage, "/ext/apps_data/chameleon_ultra");

    if(storage_file_open(file, filepath, FSAM_WRITE, FSOM_CREATE_ALWAYS)) {
        // Write header
        const char* header = "# Chameleon Ultra - Mifare Key Database\n"
                             "# Format: KeyName AABBCCDDEEFF Description\n"
                             "# Each line: name, 12 hex digits (6 bytes), description\n\n";
        storage_file_write(file, header, strlen(header));

        // Write all keys
        for(size_t i = 0; i < MIFARE_KEYS_DB_COUNT; i++) {
            const MifareKeyEntry* entry = &mifare_keys_database[i];

            // Format: "Name AABBCCDDEEFF Description\n"
            char line[256];
            snprintf(
                line,
                sizeof(line),
                "%-20s %02X%02X%02X%02X%02X%02X  # %s\n",
                entry->name,
                entry->key[0],
                entry->key[1],
                entry->key[2],
                entry->key[3],
                entry->key[4],
                entry->key[5],
                entry->description);

            storage_file_write(file, line, strlen(line));
        }

        success = true;
    }

    storage_file_close(file);
    storage_file_free(file);
    furi_record_close(RECORD_STORAGE);

    return success;
}

size_t mifare_keys_db_import_from_file(
    const char* filepath,
    void (*callback)(const char* name, const uint8_t key[6], void* context),
    void* context) {
    if(!filepath || !callback) return 0;

    Storage* storage = furi_record_open(RECORD_STORAGE);
    File* file = storage_file_alloc(storage);

    size_t imported_count = 0;

    if(storage_file_open(file, filepath, FSAM_READ, FSOM_OPEN_EXISTING)) {
        char line[256];
        size_t line_len;

        while((line_len = storage_file_read(file, line, sizeof(line) - 1)) > 0) {
            line[line_len] = '\0';

            // Skip comments and empty lines
            if(line[0] == '#' || line[0] == '\n' || line[0] == '\r') {
                continue;
            }

            // Parse line: "Name AABBCCDDEEFF # Description"
            char name[64];
            char key_hex[13];
            uint8_t key[6];

            if(sscanf(line, "%63s %12s", name, key_hex) == 2) {
                // Convert hex string to bytes
                if(strlen(key_hex) == 12) {
                    bool valid = true;
                    for(int i = 0; i < 6; i++) {
                        char byte_str[3] = {key_hex[i * 2], key_hex[i * 2 + 1], '\0'};
                        char* endptr;
                        long val = strtol(byte_str, &endptr, 16);
                        if(endptr != &byte_str[2]) {
                            valid = false;
                            break;
                        }
                        key[i] = (uint8_t)val;
                    }

                    if(valid) {
                        callback(name, key, context);
                        imported_count++;
                    }
                }
            }
        }
    }

    storage_file_close(file);
    storage_file_free(file);
    furi_record_close(RECORD_STORAGE);

    return imported_count;
}
