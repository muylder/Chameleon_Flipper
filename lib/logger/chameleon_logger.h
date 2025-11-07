#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <furi.h>

#define LOGGER_BUFFER_SIZE 100
#define LOGGER_MESSAGE_MAX_LEN 128

// Log levels
typedef enum {
    LogLevelDebug = 0,
    LogLevelInfo = 1,
    LogLevelWarn = 2,
    LogLevelError = 3,
} ChameleonLogLevel;

// Log entry structure
typedef struct {
    uint32_t timestamp;
    ChameleonLogLevel level;
    char message[LOGGER_MESSAGE_MAX_LEN];
    char tag[32];
    bool valid;
} ChameleonLogEntry;

// Logger instance
typedef struct ChameleonLogger ChameleonLogger;

// Create/destroy
ChameleonLogger* chameleon_logger_alloc();
void chameleon_logger_free(ChameleonLogger* logger);

// Logging functions
void chameleon_logger_log(
    ChameleonLogger* logger,
    ChameleonLogLevel level,
    const char* tag,
    const char* format,
    ...);

// Convenience macros
#define CHAM_LOG_D(logger, tag, format, ...) \
    chameleon_logger_log(logger, LogLevelDebug, tag, format, ##__VA_ARGS__)
#define CHAM_LOG_I(logger, tag, format, ...) \
    chameleon_logger_log(logger, LogLevelInfo, tag, format, ##__VA_ARGS__)
#define CHAM_LOG_W(logger, tag, format, ...) \
    chameleon_logger_log(logger, LogLevelWarn, tag, format, ##__VA_ARGS__)
#define CHAM_LOG_E(logger, tag, format, ...) \
    chameleon_logger_log(logger, LogLevelError, tag, format, ##__VA_ARGS__)

// Retrieve logs
uint32_t chameleon_logger_get_count(ChameleonLogger* logger);
bool chameleon_logger_get_entry(ChameleonLogger* logger, uint32_t index, ChameleonLogEntry* entry);
void chameleon_logger_get_latest(
    ChameleonLogger* logger,
    ChameleonLogEntry* entries,
    uint32_t max_count,
    uint32_t* actual_count);

// Clear logs
void chameleon_logger_clear(ChameleonLogger* logger);

// Export to file
bool chameleon_logger_export_to_file(ChameleonLogger* logger, const char* filepath);

// Get level name
const char* chameleon_logger_level_to_string(ChameleonLogLevel level);
