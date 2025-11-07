#include "chameleon_logger.h"
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <storage/storage.h>

#define TAG "ChameleonLogger"

struct ChameleonLogger {
    ChameleonLogEntry buffer[LOGGER_BUFFER_SIZE];
    uint32_t write_idx;
    uint32_t count;
    FuriMutex* mutex;
};

ChameleonLogger* chameleon_logger_alloc() {
    ChameleonLogger* logger = malloc(sizeof(ChameleonLogger));
    memset(logger, 0, sizeof(ChameleonLogger));

    logger->mutex = furi_mutex_alloc(FuriMutexTypeNormal);

    FURI_LOG_I(TAG, "Logger initialized");

    return logger;
}

void chameleon_logger_free(ChameleonLogger* logger) {
    furi_assert(logger);

    furi_mutex_free(logger->mutex);
    free(logger);

    FURI_LOG_I(TAG, "Logger freed");
}

void chameleon_logger_log(
    ChameleonLogger* logger,
    ChameleonLogLevel level,
    const char* tag,
    const char* format,
    ...) {
    furi_assert(logger);
    furi_assert(tag);
    furi_assert(format);

    furi_mutex_acquire(logger->mutex, FuriWaitForever);

    // Get current entry
    ChameleonLogEntry* entry = &logger->buffer[logger->write_idx];

    // Fill entry
    entry->timestamp = furi_get_tick();
    entry->level = level;
    entry->valid = true;

    // Copy tag (max 31 chars + null)
    strncpy(entry->tag, tag, sizeof(entry->tag) - 1);
    entry->tag[sizeof(entry->tag) - 1] = '\0';

    // Format message
    va_list args;
    va_start(args, format);
    vsnprintf(entry->message, sizeof(entry->message), format, args);
    va_end(args);

    // Advance write index (circular)
    logger->write_idx = (logger->write_idx + 1) % LOGGER_BUFFER_SIZE;

    // Update count (max LOGGER_BUFFER_SIZE)
    if(logger->count < LOGGER_BUFFER_SIZE) {
        logger->count++;
    }

    furi_mutex_release(logger->mutex);

    // Also log to Flipper's console
    switch(level) {
    case LogLevelDebug:
        FURI_LOG_D(tag, "%s", entry->message);
        break;
    case LogLevelInfo:
        FURI_LOG_I(tag, "%s", entry->message);
        break;
    case LogLevelWarn:
        FURI_LOG_W(tag, "%s", entry->message);
        break;
    case LogLevelError:
        FURI_LOG_E(tag, "%s", entry->message);
        break;
    }
}

uint32_t chameleon_logger_get_count(ChameleonLogger* logger) {
    furi_assert(logger);

    furi_mutex_acquire(logger->mutex, FuriWaitForever);
    uint32_t count = logger->count;
    furi_mutex_release(logger->mutex);

    return count;
}

bool chameleon_logger_get_entry(
    ChameleonLogger* logger,
    uint32_t index,
    ChameleonLogEntry* entry) {
    furi_assert(logger);
    furi_assert(entry);

    furi_mutex_acquire(logger->mutex, FuriWaitForever);

    if(index >= logger->count) {
        furi_mutex_release(logger->mutex);
        return false;
    }

    // Calculate actual buffer index
    // If buffer is full, oldest entry is at write_idx
    // If not full, oldest entry is at 0
    uint32_t oldest_idx = (logger->count == LOGGER_BUFFER_SIZE) ? logger->write_idx : 0;
    uint32_t actual_idx = (oldest_idx + index) % LOGGER_BUFFER_SIZE;

    memcpy(entry, &logger->buffer[actual_idx], sizeof(ChameleonLogEntry));

    furi_mutex_release(logger->mutex);

    return entry->valid;
}

void chameleon_logger_get_latest(
    ChameleonLogger* logger,
    ChameleonLogEntry* entries,
    uint32_t max_count,
    uint32_t* actual_count) {
    furi_assert(logger);
    furi_assert(entries);
    furi_assert(actual_count);

    furi_mutex_acquire(logger->mutex, FuriWaitForever);

    uint32_t count = logger->count < max_count ? logger->count : max_count;
    *actual_count = count;

    // Get latest entries (newest first)
    for(uint32_t i = 0; i < count; i++) {
        uint32_t log_index = logger->count - 1 - i;
        uint32_t oldest_idx = (logger->count == LOGGER_BUFFER_SIZE) ? logger->write_idx : 0;
        uint32_t actual_idx = (oldest_idx + log_index) % LOGGER_BUFFER_SIZE;

        memcpy(&entries[i], &logger->buffer[actual_idx], sizeof(ChameleonLogEntry));
    }

    furi_mutex_release(logger->mutex);
}

void chameleon_logger_clear(ChameleonLogger* logger) {
    furi_assert(logger);

    furi_mutex_acquire(logger->mutex, FuriWaitForever);

    memset(logger->buffer, 0, sizeof(logger->buffer));
    logger->write_idx = 0;
    logger->count = 0;

    furi_mutex_release(logger->mutex);

    FURI_LOG_I(TAG, "Logger cleared");
}

bool chameleon_logger_export_to_file(ChameleonLogger* logger, const char* filepath) {
    furi_assert(logger);
    furi_assert(filepath);

    FURI_LOG_I(TAG, "Exporting logs to: %s", filepath);

    Storage* storage = furi_record_open(RECORD_STORAGE);
    File* file = storage_file_alloc(storage);

    if(!storage_file_open(file, filepath, FSAM_WRITE, FSOM_CREATE_ALWAYS)) {
        FURI_LOG_E(TAG, "Failed to open file for export");
        storage_file_free(file);
        furi_record_close(RECORD_STORAGE);
        return false;
    }

    furi_mutex_acquire(logger->mutex, FuriWaitForever);

    // Write header
    const char* header = "Chameleon Ultra - Log Export\n"
                         "==============================\n\n";
    storage_file_write(file, header, strlen(header));

    // Write all entries (oldest to newest)
    for(uint32_t i = 0; i < logger->count; i++) {
        uint32_t oldest_idx = (logger->count == LOGGER_BUFFER_SIZE) ? logger->write_idx : 0;
        uint32_t actual_idx = (oldest_idx + i) % LOGGER_BUFFER_SIZE;

        ChameleonLogEntry* entry = &logger->buffer[actual_idx];
        if(!entry->valid) continue;

        // Format: [TIMESTAMP] [LEVEL] [TAG] Message
        char line[256];
        int len = snprintf(
            line,
            sizeof(line),
            "[%lu] [%s] [%s] %s\n",
            entry->timestamp,
            chameleon_logger_level_to_string(entry->level),
            entry->tag,
            entry->message);

        storage_file_write(file, line, len);
    }

    furi_mutex_release(logger->mutex);

    storage_file_close(file);
    storage_file_free(file);
    furi_record_close(RECORD_STORAGE);

    FURI_LOG_I(TAG, "Logs exported successfully: %lu entries", logger->count);

    return true;
}

const char* chameleon_logger_level_to_string(ChameleonLogLevel level) {
    switch(level) {
    case LogLevelDebug:
        return "DEBUG";
    case LogLevelInfo:
        return "INFO ";
    case LogLevelWarn:
        return "WARN ";
    case LogLevelError:
        return "ERROR";
    default:
        return "?????";
    }
}
