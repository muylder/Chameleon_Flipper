#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <furi.h>

// Operation types for statistics tracking
typedef enum {
    StatOpTypeTagRead,
    StatOpTypeTagWrite,
    StatOpTypeSlotSwitch,
    StatOpTypeKeyTest,
    StatOpTypeBackup,
    StatOpTypeRestore,
    StatOpTypeConnect,
    StatOpTypeDisconnect,
    StatOpTypeCount, // Must be last
} StatOperationType;

// Operation result
typedef enum {
    StatResultSuccess,
    StatResultFailure,
} StatOperationResult;

// History entry
typedef struct {
    StatOperationType type;
    StatOperationResult result;
    uint32_t timestamp; // Unix timestamp
    char details[64]; // Optional details (tag UID, slot number, etc.)
} StatHistoryEntry;

// Statistics data
typedef struct {
    uint32_t version; // File format version

    // Operation counters
    uint32_t tag_reads_success;
    uint32_t tag_reads_failed;
    uint32_t tag_writes_success;
    uint32_t tag_writes_failed;
    uint32_t slot_switches;
    uint32_t key_tests_success;
    uint32_t key_tests_failed;
    uint32_t backups_created;
    uint32_t restores_done;
    uint32_t connections_made;

    // Session tracking
    uint32_t total_sessions;
    uint32_t total_runtime_seconds;
    uint32_t last_session_timestamp;

    // Most used slot
    uint8_t slot_usage_count[8];

    // Success rates (calculated)
    float tag_read_success_rate;
    float tag_write_success_rate;
    float key_test_success_rate;
} StatisticsData;

// Statistics manager
typedef struct StatisticsManager StatisticsManager;

/**
 * @brief Allocate statistics manager
 * @return StatisticsManager instance
 */
StatisticsManager* statistics_manager_alloc(void);

/**
 * @brief Free statistics manager
 * @param manager Manager instance
 */
void statistics_manager_free(StatisticsManager* manager);

/**
 * @brief Load statistics from file
 * @param manager Manager instance
 * @return true if loaded successfully
 */
bool statistics_manager_load(StatisticsManager* manager);

/**
 * @brief Save statistics to file
 * @param manager Manager instance
 * @return true if saved successfully
 */
bool statistics_manager_save(StatisticsManager* manager);

/**
 * @brief Record an operation
 * @param manager Manager instance
 * @param type Operation type
 * @param result Result (success/failure)
 * @param details Optional details string
 */
void statistics_manager_record_operation(
    StatisticsManager* manager,
    StatOperationType type,
    StatOperationResult result,
    const char* details);

/**
 * @brief Record slot usage
 * @param manager Manager instance
 * @param slot_number Slot number (0-7)
 */
void statistics_manager_record_slot_usage(StatisticsManager* manager, uint8_t slot_number);

/**
 * @brief Start a new session
 * @param manager Manager instance
 */
void statistics_manager_start_session(StatisticsManager* manager);

/**
 * @brief End current session
 * @param manager Manager instance
 */
void statistics_manager_end_session(StatisticsManager* manager);

/**
 * @brief Get statistics data (read-only)
 * @param manager Manager instance
 * @return Pointer to statistics data
 */
const StatisticsData* statistics_manager_get_data(StatisticsManager* manager);

/**
 * @brief Get history entry count
 * @param manager Manager instance
 * @return Number of history entries
 */
size_t statistics_manager_get_history_count(StatisticsManager* manager);

/**
 * @brief Get history entry by index
 * @param manager Manager instance
 * @param index Entry index (0 = most recent)
 * @return Pointer to history entry, or NULL if invalid
 */
const StatHistoryEntry* statistics_manager_get_history_entry(
    StatisticsManager* manager,
    size_t index);

/**
 * @brief Clear all statistics
 * @param manager Manager instance
 */
void statistics_manager_reset(StatisticsManager* manager);

/**
 * @brief Export statistics to text file
 * @param manager Manager instance
 * @param filepath Path to export file
 * @return true if exported successfully
 */
bool statistics_manager_export(StatisticsManager* manager, const char* filepath);
