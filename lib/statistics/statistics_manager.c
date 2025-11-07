#include "statistics_manager.h"
#include <storage/storage.h>
#include <furi_hal_rtc.h>
#include <datetime.h>

#define STATS_FILE_PATH "/ext/apps_data/chameleon_ultra/statistics.dat"
#define HISTORY_FILE_PATH "/ext/apps_data/chameleon_ultra/history.dat"
#define STATS_VERSION 1
#define MAX_HISTORY_ENTRIES 100

struct StatisticsManager {
    StatisticsData stats;
    StatHistoryEntry history[MAX_HISTORY_ENTRIES];
    size_t history_count;
    uint32_t session_start_time;
    FuriMutex* mutex;
};

StatisticsManager* statistics_manager_alloc(void) {
    StatisticsManager* manager = malloc(sizeof(StatisticsManager));
    memset(manager, 0, sizeof(StatisticsManager));

    manager->stats.version = STATS_VERSION;
    manager->mutex = furi_mutex_alloc(FuriMutexTypeNormal);

    return manager;
}

void statistics_manager_free(StatisticsManager* manager) {
    if(!manager) return;

    furi_mutex_free(manager->mutex);
    free(manager);
}

bool statistics_manager_load(StatisticsManager* manager) {
    if(!manager) return false;

    furi_mutex_acquire(manager->mutex, FuriWaitForever);

    Storage* storage = furi_record_open(RECORD_STORAGE);
    File* file = storage_file_alloc(storage);

    bool success = false;

    // Load statistics
    if(storage_file_open(file, STATS_FILE_PATH, FSAM_READ, FSOM_OPEN_EXISTING)) {
        uint32_t version;
        if(storage_file_read(file, &version, sizeof(uint32_t)) == sizeof(uint32_t)) {
            if(version == STATS_VERSION) {
                size_t read = storage_file_read(
                    file,
                    ((uint8_t*)&manager->stats) + sizeof(uint32_t),
                    sizeof(StatisticsData) - sizeof(uint32_t));

                if(read == sizeof(StatisticsData) - sizeof(uint32_t)) {
                    success = true;
                }
            }
        }
        storage_file_close(file);
    }

    // Load history
    if(storage_file_open(file, HISTORY_FILE_PATH, FSAM_READ, FSOM_OPEN_EXISTING)) {
        size_t count;
        if(storage_file_read(file, &count, sizeof(size_t)) == sizeof(size_t)) {
            if(count <= MAX_HISTORY_ENTRIES) {
                manager->history_count = count;
                storage_file_read(
                    file,
                    manager->history,
                    count * sizeof(StatHistoryEntry));
            }
        }
        storage_file_close(file);
    }

    storage_file_free(file);
    furi_record_close(RECORD_STORAGE);

    furi_mutex_release(manager->mutex);

    return success;
}

bool statistics_manager_save(StatisticsManager* manager) {
    if(!manager) return false;

    furi_mutex_acquire(manager->mutex, FuriWaitForever);

    Storage* storage = furi_record_open(RECORD_STORAGE);

    // Create directory
    storage_common_mkdir(storage, "/ext/apps_data/chameleon_ultra");

    File* file = storage_file_alloc(storage);
    bool success = false;

    // Save statistics
    if(storage_file_open(file, STATS_FILE_PATH, FSAM_WRITE, FSOM_CREATE_ALWAYS)) {
        size_t written = storage_file_write(file, &manager->stats, sizeof(StatisticsData));
        if(written == sizeof(StatisticsData)) {
            success = true;
        }
        storage_file_close(file);
    }

    // Save history
    if(storage_file_open(file, HISTORY_FILE_PATH, FSAM_WRITE, FSOM_CREATE_ALWAYS)) {
        storage_file_write(file, &manager->history_count, sizeof(size_t));
        storage_file_write(
            file,
            manager->history,
            manager->history_count * sizeof(StatHistoryEntry));
        storage_file_close(file);
    }

    storage_file_free(file);
    furi_record_close(RECORD_STORAGE);

    furi_mutex_release(manager->mutex);

    return success;
}

static void calculate_success_rates(StatisticsManager* manager) {
    // Tag read success rate
    uint32_t total_reads = manager->stats.tag_reads_success + manager->stats.tag_reads_failed;
    if(total_reads > 0) {
        manager->stats.tag_read_success_rate =
            (float)manager->stats.tag_reads_success / total_reads * 100.0f;
    }

    // Tag write success rate
    uint32_t total_writes = manager->stats.tag_writes_success + manager->stats.tag_writes_failed;
    if(total_writes > 0) {
        manager->stats.tag_write_success_rate =
            (float)manager->stats.tag_writes_success / total_writes * 100.0f;
    }

    // Key test success rate
    uint32_t total_key_tests = manager->stats.key_tests_success + manager->stats.key_tests_failed;
    if(total_key_tests > 0) {
        manager->stats.key_test_success_rate =
            (float)manager->stats.key_tests_success / total_key_tests * 100.0f;
    }
}

void statistics_manager_record_operation(
    StatisticsManager* manager,
    StatOperationType type,
    StatOperationResult result,
    const char* details) {
    if(!manager) return;

    furi_mutex_acquire(manager->mutex, FuriWaitForever);

    // Update counters
    switch(type) {
    case StatOpTypeTagRead:
        if(result == StatResultSuccess) {
            manager->stats.tag_reads_success++;
        } else {
            manager->stats.tag_reads_failed++;
        }
        break;
    case StatOpTypeTagWrite:
        if(result == StatResultSuccess) {
            manager->stats.tag_writes_success++;
        } else {
            manager->stats.tag_writes_failed++;
        }
        break;
    case StatOpTypeSlotSwitch:
        manager->stats.slot_switches++;
        break;
    case StatOpTypeKeyTest:
        if(result == StatResultSuccess) {
            manager->stats.key_tests_success++;
        } else {
            manager->stats.key_tests_failed++;
        }
        break;
    case StatOpTypeBackup:
        manager->stats.backups_created++;
        break;
    case StatOpTypeRestore:
        manager->stats.restores_done++;
        break;
    case StatOpTypeConnect:
        manager->stats.connections_made++;
        break;
    default:
        break;
    }

    // Add to history
    if(manager->history_count < MAX_HISTORY_ENTRIES) {
        // Add new entry
        manager->history[manager->history_count].type = type;
        manager->history[manager->history_count].result = result;
        manager->history[manager->history_count].timestamp = furi_hal_rtc_get_timestamp();

        if(details) {
            snprintf(
                manager->history[manager->history_count].details,
                sizeof(manager->history[manager->history_count].details),
                "%s",
                details);
        } else {
            manager->history[manager->history_count].details[0] = '\0';
        }

        manager->history_count++;
    } else {
        // Shift history and add new entry
        memmove(
            &manager->history[0],
            &manager->history[1],
            (MAX_HISTORY_ENTRIES - 1) * sizeof(StatHistoryEntry));

        manager->history[MAX_HISTORY_ENTRIES - 1].type = type;
        manager->history[MAX_HISTORY_ENTRIES - 1].result = result;
        manager->history[MAX_HISTORY_ENTRIES - 1].timestamp = furi_hal_rtc_get_timestamp();

        if(details) {
            snprintf(
                manager->history[MAX_HISTORY_ENTRIES - 1].details,
                sizeof(manager->history[MAX_HISTORY_ENTRIES - 1].details),
                "%s",
                details);
        }
    }

    calculate_success_rates(manager);

    furi_mutex_release(manager->mutex);
}

void statistics_manager_record_slot_usage(StatisticsManager* manager, uint8_t slot_number) {
    if(!manager || slot_number >= 8) return;

    furi_mutex_acquire(manager->mutex, FuriWaitForever);
    manager->stats.slot_usage_count[slot_number]++;
    furi_mutex_release(manager->mutex);
}

void statistics_manager_start_session(StatisticsManager* manager) {
    if(!manager) return;

    furi_mutex_acquire(manager->mutex, FuriWaitForever);
    manager->session_start_time = furi_hal_rtc_get_timestamp();
    manager->stats.total_sessions++;
    manager->stats.last_session_timestamp = manager->session_start_time;
    furi_mutex_release(manager->mutex);
}

void statistics_manager_end_session(StatisticsManager* manager) {
    if(!manager) return;

    furi_mutex_acquire(manager->mutex, FuriWaitForever);
    if(manager->session_start_time > 0) {
        uint32_t session_duration = furi_hal_rtc_get_timestamp() - manager->session_start_time;
        manager->stats.total_runtime_seconds += session_duration;
        manager->session_start_time = 0;
    }
    furi_mutex_release(manager->mutex);
}

const StatisticsData* statistics_manager_get_data(StatisticsManager* manager) {
    if(!manager) return NULL;
    return &manager->stats;
}

size_t statistics_manager_get_history_count(StatisticsManager* manager) {
    if(!manager) return 0;
    return manager->history_count;
}

const StatHistoryEntry* statistics_manager_get_history_entry(
    StatisticsManager* manager,
    size_t index) {
    if(!manager || index >= manager->history_count) return NULL;

    // Return in reverse order (newest first)
    return &manager->history[manager->history_count - 1 - index];
}

void statistics_manager_reset(StatisticsManager* manager) {
    if(!manager) return;

    furi_mutex_acquire(manager->mutex, FuriWaitForever);
    memset(&manager->stats, 0, sizeof(StatisticsData));
    manager->stats.version = STATS_VERSION;
    manager->history_count = 0;
    furi_mutex_release(manager->mutex);
}

bool statistics_manager_export(StatisticsManager* manager, const char* filepath) {
    if(!manager || !filepath) return false;

    Storage* storage = furi_record_open(RECORD_STORAGE);
    File* file = storage_file_alloc(storage);

    bool success = false;

    if(storage_file_open(file, filepath, FSAM_WRITE, FSOM_CREATE_ALWAYS)) {
        FuriString* export_text = furi_string_alloc();

        furi_string_cat_printf(export_text, "# Chameleon Ultra Statistics Export\n\n");

        furi_string_cat_printf(export_text, "## Summary\n");
        furi_string_cat_printf(export_text, "Total Sessions: %lu\n", manager->stats.total_sessions);
        furi_string_cat_printf(
            export_text,
            "Total Runtime: %lu seconds (%.1f hours)\n",
            manager->stats.total_runtime_seconds,
            manager->stats.total_runtime_seconds / 3600.0f);
        furi_string_cat_printf(export_text, "\n");

        furi_string_cat_printf(export_text, "## Operations\n");
        furi_string_cat_printf(
            export_text,
            "Tag Reads: %lu success, %lu failed (%.1f%% success)\n",
            manager->stats.tag_reads_success,
            manager->stats.tag_reads_failed,
            manager->stats.tag_read_success_rate);
        furi_string_cat_printf(
            export_text,
            "Tag Writes: %lu success, %lu failed (%.1f%% success)\n",
            manager->stats.tag_writes_success,
            manager->stats.tag_writes_failed,
            manager->stats.tag_write_success_rate);
        furi_string_cat_printf(
            export_text,
            "Key Tests: %lu success, %lu failed (%.1f%% success)\n",
            manager->stats.key_tests_success,
            manager->stats.key_tests_failed,
            manager->stats.key_test_success_rate);
        furi_string_cat_printf(export_text, "Slot Switches: %lu\n", manager->stats.slot_switches);
        furi_string_cat_printf(export_text, "Backups Created: %lu\n", manager->stats.backups_created);
        furi_string_cat_printf(export_text, "Restores Done: %lu\n", manager->stats.restores_done);
        furi_string_cat_printf(export_text, "Connections: %lu\n", manager->stats.connections_made);
        furi_string_cat_printf(export_text, "\n");

        furi_string_cat_printf(export_text, "## Slot Usage\n");
        for(int i = 0; i < 8; i++) {
            furi_string_cat_printf(
                export_text,
                "Slot %d: %u times\n",
                i,
                manager->stats.slot_usage_count[i]);
        }

        storage_file_write(
            file,
            furi_string_get_cstr(export_text),
            furi_string_size(export_text));

        furi_string_free(export_text);
        storage_file_close(file);
        success = true;
    }

    storage_file_free(file);
    furi_record_close(RECORD_STORAGE);

    return success;
}
