#include "dictionary_attack.h"
#include <string.h>
#include <stdio.h>
#include <furi.h>
#include <storage/storage.h>

#define MAX_WORDLIST_SIZE 1000
#define MAX_SECTORS 40

// Default MIFARE Classic keys database
static const KeyEntry default_keys_db[DEFAULT_KEYS_COUNT] = {
    {{0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}, "Factory Default", true},
    {{0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, "All Zeros", true},
    {{0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5}, "MAD Key A", true},
    {{0xB0, 0xB1, 0xB2, 0xB3, 0xB4, 0xB5}, "MAD Key B", true},
    {{0xD3, 0xF7, 0xD3, 0xF7, 0xD3, 0xF7}, "NFC Key", true},
    {{0xA0, 0xB0, 0xC0, 0xD0, 0xE0, 0xF0}, "Common 1", true},
    {{0x00, 0x11, 0x22, 0x33, 0x44, 0x55}, "Common 2", true},
    {{0x01, 0x23, 0x45, 0x67, 0x89, 0xAB}, "Sequential", true},
    {{0x48, 0x4F, 0x54, 0x45, 0x4C, 0x00}, "HOTEL", true},
    {{0x4D, 0x49, 0x46, 0x41, 0x52, 0x45}, "MIFARE", true},
    {{0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF}, "Test Key", true},
    {{0x71, 0x4C, 0x5C, 0x88, 0x6E, 0x97}, "Transport 1", true},
    {{0x58, 0x7E, 0xE5, 0xF9, 0x35, 0x0F}, "Transport 2", true},
    {{0xA6, 0x4B, 0xC4, 0x1B, 0x20, 0x8A}, "Hotel 1", true},
    {{0x49, 0xFA, 0xE4, 0xE3, 0x84, 0x9F}, "Hotel 2", true},
    {{0x01, 0x02, 0x03, 0x04, 0x05, 0x06}, "Incremental", true},
};

struct DictionaryAttack {
    KeyEntry wordlist[MAX_WORDLIST_SIZE];
    size_t wordlist_size;

    AttackMode mode;
    AttackType type;
    AttackStatus status;

    uint8_t target_sectors[MAX_SECTORS];
    uint8_t target_sector_count;

    SectorAttackResult results[MAX_SECTORS];
    AttackStatistics stats;

    uint32_t start_time;
    uint32_t pause_time;

    AttackProgressCallback progress_callback;
    void* callback_context;

    FuriMutex* mutex;
};

DictionaryAttack* dictionary_attack_alloc(void) {
    DictionaryAttack* attack = malloc(sizeof(DictionaryAttack));
    memset(attack, 0, sizeof(DictionaryAttack));

    attack->mutex = furi_mutex_alloc(FuriMutexTypeNormal);
    attack->mode = AttackModeBoth;
    attack->type = AttackTypeDictionary;
    attack->status = AttackStatusIdle;

    return attack;
}

void dictionary_attack_free(DictionaryAttack* attack) {
    if(!attack) return;

    furi_mutex_free(attack->mutex);
    free(attack);
}

bool dictionary_attack_load_default_keys(DictionaryAttack* attack) {
    if(!attack) return false;

    furi_mutex_acquire(attack->mutex, FuriWaitForever);

    attack->wordlist_size = 0;

    for(size_t i = 0; i < DEFAULT_KEYS_COUNT; i++) {
        memcpy(&attack->wordlist[attack->wordlist_size], &default_keys_db[i], sizeof(KeyEntry));
        attack->wordlist_size++;
    }

    furi_mutex_release(attack->mutex);

    return true;
}

bool dictionary_attack_load_wordlist(DictionaryAttack* attack, const char* filepath) {
    if(!attack || !filepath) return false;

    furi_mutex_acquire(attack->mutex, FuriWaitForever);

    Storage* storage = furi_record_open(RECORD_STORAGE);
    File* file = storage_file_alloc(storage);

    bool success = false;

    if(storage_file_open(file, filepath, FSAM_READ, FSOM_OPEN_EXISTING)) {
        char line[128];
        attack->wordlist_size = 0;

        while(storage_file_read(file, line, sizeof(line)) > 0 &&
              attack->wordlist_size < MAX_WORDLIST_SIZE) {
            // Parse key line (format: FFFFFFFFFFFF or FF:FF:FF:FF:FF:FF)
            uint8_t key[6];
            int matched = sscanf(line, "%2hhx%2hhx%2hhx%2hhx%2hhx%2hhx",
                &key[0], &key[1], &key[2], &key[3], &key[4], &key[5]);

            if(matched != 6) {
                // Try colon-separated format
                matched = sscanf(line, "%2hhx:%2hhx:%2hhx:%2hhx:%2hhx:%2hhx",
                    &key[0], &key[1], &key[2], &key[3], &key[4], &key[5]);
            }

            if(matched == 6) {
                memcpy(attack->wordlist[attack->wordlist_size].key, key, 6);
                snprintf(attack->wordlist[attack->wordlist_size].description,
                    sizeof(attack->wordlist[attack->wordlist_size].description),
                    "Custom %zu", attack->wordlist_size + 1);
                attack->wordlist[attack->wordlist_size].is_default = false;
                attack->wordlist_size++;
            }
        }

        success = true;
        storage_file_close(file);
    }

    storage_file_free(file);
    furi_record_close(RECORD_STORAGE);

    furi_mutex_release(attack->mutex);

    return success;
}

size_t dictionary_attack_get_wordlist_size(DictionaryAttack* attack) {
    if(!attack) return 0;
    return attack->wordlist_size;
}

const KeyEntry* dictionary_attack_get_key(DictionaryAttack* attack, size_t index) {
    if(!attack || index >= attack->wordlist_size) return NULL;
    return &attack->wordlist[index];
}

bool dictionary_attack_add_custom_key(
    DictionaryAttack* attack,
    const uint8_t* key,
    const char* description) {
    if(!attack || !key) return false;

    furi_mutex_acquire(attack->mutex, FuriWaitForever);

    if(attack->wordlist_size >= MAX_WORDLIST_SIZE) {
        furi_mutex_release(attack->mutex);
        return false;
    }

    memcpy(attack->wordlist[attack->wordlist_size].key, key, 6);
    if(description) {
        strncpy(attack->wordlist[attack->wordlist_size].description,
            description,
            sizeof(attack->wordlist[attack->wordlist_size].description) - 1);
    }
    attack->wordlist[attack->wordlist_size].is_default = false;
    attack->wordlist_size++;

    furi_mutex_release(attack->mutex);

    return true;
}

void dictionary_attack_clear_wordlist(DictionaryAttack* attack) {
    if(!attack) return;

    furi_mutex_acquire(attack->mutex, FuriWaitForever);
    attack->wordlist_size = 0;
    furi_mutex_release(attack->mutex);
}

void dictionary_attack_set_mode(DictionaryAttack* attack, AttackMode mode) {
    if(!attack) return;
    attack->mode = mode;
}

void dictionary_attack_set_type(DictionaryAttack* attack, AttackType type) {
    if(!attack) return;
    attack->type = type;
}

void dictionary_attack_set_target_sectors(
    DictionaryAttack* attack,
    uint8_t* sectors,
    uint8_t count) {
    if(!attack || !sectors) return;

    furi_mutex_acquire(attack->mutex, FuriWaitForever);

    attack->target_sector_count = (count > MAX_SECTORS) ? MAX_SECTORS : count;
    memcpy(attack->target_sectors, sectors, attack->target_sector_count);

    furi_mutex_release(attack->mutex);
}

void dictionary_attack_set_all_sectors(DictionaryAttack* attack, bool classic_1k) {
    if(!attack) return;

    uint8_t sector_count = classic_1k ? 16 : 40;

    furi_mutex_acquire(attack->mutex, FuriWaitForever);

    attack->target_sector_count = sector_count;
    for(uint8_t i = 0; i < sector_count; i++) {
        attack->target_sectors[i] = i;
    }

    furi_mutex_release(attack->mutex);
}

static bool test_key_on_sector(
    DictionaryAttack* attack,
    uint8_t sector,
    const uint8_t* key,
    bool is_key_a) {
    // Mock implementation - in real scenario this would call Chameleon Ultra
    // to test authentication with the key

    UNUSED(attack);
    UNUSED(sector);
    UNUSED(key);
    UNUSED(is_key_a);

    // Simulate testing delay
    furi_delay_ms(10);

    // For demo: Check if key is factory default
    static const uint8_t factory_key[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    if(memcmp(key, factory_key, 6) == 0) {
        return true; // Found!
    }

    // Small chance of finding other keys (for demo)
    return (furi_hal_random_get() % 100) < 5;
}

bool dictionary_attack_start(
    DictionaryAttack* attack,
    AttackProgressCallback progress_cb,
    void* context) {
    if(!attack) return false;

    if(attack->wordlist_size == 0) return false;
    if(attack->target_sector_count == 0) return false;

    furi_mutex_acquire(attack->mutex, FuriWaitForever);

    // Initialize
    attack->status = AttackStatusRunning;
    attack->progress_callback = progress_cb;
    attack->callback_context = context;
    attack->start_time = furi_get_tick();

    memset(&attack->stats, 0, sizeof(AttackStatistics));
    memset(attack->results, 0, sizeof(attack->results));

    attack->stats.sectors_remaining = attack->target_sector_count;

    furi_mutex_release(attack->mutex);

    // Attack loop (in real implementation, this would run in a separate thread)
    for(uint8_t s = 0; s < attack->target_sector_count; s++) {
        if(attack->status != AttackStatusRunning) break;

        uint8_t sector = attack->target_sectors[s];
        SectorAttackResult* result = &attack->results[sector];
        result->sector = sector;

        uint32_t sector_start = furi_get_tick();

        // Test Key A
        if(attack->mode == AttackModeKeyA || attack->mode == AttackModeBoth) {
            for(size_t k = 0; k < attack->wordlist_size; k++) {
                if(attack->status != AttackStatusRunning) break;

                const uint8_t* key = attack->wordlist[k].key;
                attack->stats.total_attempts++;
                result->attempts_a++;

                if(test_key_on_sector(attack, sector, key, true)) {
                    result->key_a_found = true;
                    memcpy(result->key_a, key, 6);
                    attack->stats.keys_found++;
                    break;
                }

                // Update progress every 10 attempts
                if(attack->stats.total_attempts % 10 == 0 && progress_cb) {
                    uint32_t elapsed = furi_get_tick() - attack->start_time;
                    attack->stats.elapsed_time_ms = elapsed;
                    attack->stats.keys_per_second =
                        (attack->stats.total_attempts * 1000.0f) / elapsed;

                    uint8_t percent = ((s * 100) / attack->target_sector_count);
                    progress_cb(percent, &attack->stats, context);
                }
            }

            if(!result->key_a_found) {
                attack->stats.keys_failed++;
            }
        }

        // Test Key B
        if(attack->mode == AttackModeKeyB || attack->mode == AttackModeBoth) {
            for(size_t k = 0; k < attack->wordlist_size; k++) {
                if(attack->status != AttackStatusRunning) break;

                const uint8_t* key = attack->wordlist[k].key;
                attack->stats.total_attempts++;
                result->attempts_b++;

                if(test_key_on_sector(attack, sector, key, false)) {
                    result->key_b_found = true;
                    memcpy(result->key_b, key, 6);
                    attack->stats.keys_found++;
                    break;
                }
            }

            if(!result->key_b_found) {
                attack->stats.keys_failed++;
            }
        }

        result->time_ms = furi_get_tick() - sector_start;
        attack->stats.sectors_complete++;
        attack->stats.sectors_remaining--;
    }

    // Finalize
    furi_mutex_acquire(attack->mutex, FuriWaitForever);

    if(attack->status == AttackStatusRunning) {
        attack->status = AttackStatusCompleted;
    }

    attack->stats.elapsed_time_ms = furi_get_tick() - attack->start_time;
    if(attack->stats.total_attempts > 0) {
        attack->stats.success_rate =
            (attack->stats.keys_found * 100.0f) / attack->stats.total_attempts;
    }

    if(progress_cb) {
        progress_cb(100, &attack->stats, context);
    }

    furi_mutex_release(attack->mutex);

    return true;
}

void dictionary_attack_pause(DictionaryAttack* attack) {
    if(!attack) return;

    furi_mutex_acquire(attack->mutex, FuriWaitForever);

    if(attack->status == AttackStatusRunning) {
        attack->status = AttackStatusPaused;
        attack->pause_time = furi_get_tick();
    }

    furi_mutex_release(attack->mutex);
}

void dictionary_attack_resume(DictionaryAttack* attack) {
    if(!attack) return;

    furi_mutex_acquire(attack->mutex, FuriWaitForever);

    if(attack->status == AttackStatusPaused) {
        attack->status = AttackStatusRunning;
        // Adjust start time to account for pause duration
        attack->start_time += (furi_get_tick() - attack->pause_time);
    }

    furi_mutex_release(attack->mutex);
}

void dictionary_attack_stop(DictionaryAttack* attack) {
    if(!attack) return;

    furi_mutex_acquire(attack->mutex, FuriWaitForever);
    attack->status = AttackStatusCancelled;
    furi_mutex_release(attack->mutex);
}

AttackStatus dictionary_attack_get_status(DictionaryAttack* attack) {
    if(!attack) return AttackStatusIdle;
    return attack->status;
}

const AttackStatistics* dictionary_attack_get_statistics(DictionaryAttack* attack) {
    if(!attack) return NULL;
    return &attack->stats;
}

const SectorAttackResult* dictionary_attack_get_sector_result(
    DictionaryAttack* attack,
    uint8_t sector) {
    if(!attack || sector >= MAX_SECTORS) return NULL;
    return &attack->results[sector];
}

size_t dictionary_attack_get_all_results(
    DictionaryAttack* attack,
    SectorAttackResult* results,
    size_t max_results) {
    if(!attack || !results) return 0;

    size_t count = 0;
    for(uint8_t i = 0; i < MAX_SECTORS && count < max_results; i++) {
        if(attack->results[i].sector != 0 ||
           attack->results[i].key_a_found ||
           attack->results[i].key_b_found) {
            memcpy(&results[count], &attack->results[i], sizeof(SectorAttackResult));
            count++;
        }
    }

    return count;
}

bool dictionary_attack_export_found_keys(DictionaryAttack* attack, const char* filepath) {
    if(!attack || !filepath) return false;

    Storage* storage = furi_record_open(RECORD_STORAGE);
    File* file = storage_file_alloc(storage);

    bool success = false;

    if(storage_file_open(file, filepath, FSAM_WRITE, FSOM_CREATE_ALWAYS)) {
        for(uint8_t i = 0; i < MAX_SECTORS; i++) {
            const SectorAttackResult* result = &attack->results[i];

            if(result->key_a_found || result->key_b_found) {
                char line[128];

                if(result->key_a_found) {
                    snprintf(line, sizeof(line),
                        "Sector %02d Key A: %02X%02X%02X%02X%02X%02X\n",
                        result->sector,
                        result->key_a[0], result->key_a[1], result->key_a[2],
                        result->key_a[3], result->key_a[4], result->key_a[5]);
                    storage_file_write(file, line, strlen(line));
                }

                if(result->key_b_found) {
                    snprintf(line, sizeof(line),
                        "Sector %02d Key B: %02X%02X%02X%02X%02X%02X\n",
                        result->sector,
                        result->key_b[0], result->key_b[1], result->key_b[2],
                        result->key_b[3], result->key_b[4], result->key_b[5]);
                    storage_file_write(file, line, strlen(line));
                }
            }
        }

        success = true;
        storage_file_close(file);
    }

    storage_file_free(file);
    furi_record_close(RECORD_STORAGE);

    return success;
}

const char* dictionary_attack_get_mode_name(AttackMode mode) {
    static const char* names[] = {"Key A", "Key B", "Both Keys"};
    if(mode < 3) return names[mode];
    return "Unknown";
}

const char* dictionary_attack_get_type_name(AttackType type) {
    static const char* names[] = {
        "Dictionary",
        "Nested",
        "Darkside",
        "Hardnested",
        "Bruteforce"
    };
    if(type < 5) return names[type];
    return "Unknown";
}

const char* dictionary_attack_get_status_name(AttackStatus status) {
    static const char* names[] = {
        "Idle",
        "Running",
        "Paused",
        "Completed",
        "Cancelled",
        "Error"
    };
    if(status < 6) return names[status];
    return "Unknown";
}

void dictionary_attack_get_default_keys(KeyEntry* keys, size_t* count) {
    if(!keys || !count) return;

    size_t num = (*count < DEFAULT_KEYS_COUNT) ? *count : DEFAULT_KEYS_COUNT;
    memcpy(keys, default_keys_db, num * sizeof(KeyEntry));
    *count = num;
}
