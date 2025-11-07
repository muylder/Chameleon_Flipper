#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// Attack modes
typedef enum {
    AttackModeKeyA = 0,
    AttackModeKeyB,
    AttackModeBoth,
} AttackMode;

// Attack status
typedef enum {
    AttackStatusIdle = 0,
    AttackStatusRunning,
    AttackStatusPaused,
    AttackStatusCompleted,
    AttackStatusCancelled,
    AttackStatusError,
} AttackStatus;

// Attack type
typedef enum {
    AttackTypeDictionary = 0, // Standard wordlist attack
    AttackTypeNested, // Nested authentication attack
    AttackTypeDarkside, // Darkside attack (PRNG weakness)
    AttackTypeHardnested, // Hardnested attack
    AttackTypeBruteforce, // Full brute-force (slow!)
} AttackType;

// Key entry
typedef struct {
    uint8_t key[6];
    char description[32];
    bool is_default;
} KeyEntry;

// Attack result for a sector
typedef struct {
    uint8_t sector;
    bool key_a_found;
    bool key_b_found;
    uint8_t key_a[6];
    uint8_t key_b[6];
    uint32_t attempts_a;
    uint32_t attempts_b;
    uint32_t time_ms;
} SectorAttackResult;

// Attack statistics
typedef struct {
    uint32_t total_attempts;
    uint32_t keys_found;
    uint32_t keys_failed;
    uint32_t sectors_complete;
    uint32_t sectors_remaining;
    uint32_t elapsed_time_ms;
    uint32_t estimated_time_remaining_ms;
    float success_rate;
    float keys_per_second;
} AttackStatistics;

// Progress callback
typedef void (*AttackProgressCallback)(
    uint8_t percent,
    const AttackStatistics* stats,
    void* context);

// Dictionary Attack Manager
typedef struct DictionaryAttack DictionaryAttack;

// Allocation and deallocation
DictionaryAttack* dictionary_attack_alloc(void);
void dictionary_attack_free(DictionaryAttack* attack);

// Wordlist management
bool dictionary_attack_load_wordlist(DictionaryAttack* attack, const char* filepath);
bool dictionary_attack_load_default_keys(DictionaryAttack* attack);
size_t dictionary_attack_get_wordlist_size(DictionaryAttack* attack);
const KeyEntry* dictionary_attack_get_key(DictionaryAttack* attack, size_t index);
bool dictionary_attack_add_custom_key(DictionaryAttack* attack, const uint8_t* key, const char* description);
void dictionary_attack_clear_wordlist(DictionaryAttack* attack);

// Default keys database
#define DEFAULT_KEYS_COUNT 16

// Attack configuration
void dictionary_attack_set_mode(DictionaryAttack* attack, AttackMode mode);
void dictionary_attack_set_type(DictionaryAttack* attack, AttackType type);
void dictionary_attack_set_target_sectors(DictionaryAttack* attack, uint8_t* sectors, uint8_t count);
void dictionary_attack_set_all_sectors(DictionaryAttack* attack, bool classic_1k); // 16 or 40 sectors

// Attack execution
bool dictionary_attack_start(
    DictionaryAttack* attack,
    AttackProgressCallback progress_cb,
    void* context);
void dictionary_attack_pause(DictionaryAttack* attack);
void dictionary_attack_resume(DictionaryAttack* attack);
void dictionary_attack_stop(DictionaryAttack* attack);

// Attack status
AttackStatus dictionary_attack_get_status(DictionaryAttack* attack);
const AttackStatistics* dictionary_attack_get_statistics(DictionaryAttack* attack);
const SectorAttackResult* dictionary_attack_get_sector_result(DictionaryAttack* attack, uint8_t sector);
size_t dictionary_attack_get_all_results(
    DictionaryAttack* attack,
    SectorAttackResult* results,
    size_t max_results);

// Results management
bool dictionary_attack_export_results(DictionaryAttack* attack, const char* filepath);
bool dictionary_attack_export_found_keys(DictionaryAttack* attack, const char* filepath);

// Utilities
const char* dictionary_attack_get_mode_name(AttackMode mode);
const char* dictionary_attack_get_type_name(AttackType type);
const char* dictionary_attack_get_status_name(AttackStatus status);

// Get default keys
void dictionary_attack_get_default_keys(KeyEntry* keys, size_t* count);
