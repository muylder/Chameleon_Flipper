#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// Emulation event types
typedef enum {
    EmulEventActivated = 0,
    EmulEventDeactivated,
    EmulEventReaderDetected,
    EmulEventReaderLost,
    EmulEventAuthentication,
    EmulEventBlockRead,
    EmulEventBlockWrite,
    EmulEventSectorRead,
    EmulEventSectorWrite,
    EmulEventAntiCollision,
    EmulEventSelect,
    EmulEventHalt,
    EmulEventError,
} EmulationEventType;

// Authentication result
typedef enum {
    AuthResultSuccess = 0,
    AuthResultFailed,
    AuthResultTimeout,
} AuthResult;

// Emulation event
typedef struct {
    uint32_t timestamp;
    EmulationEventType type;
    uint8_t block_address;
    uint8_t sector;
    bool is_key_a;
    AuthResult auth_result;
    uint8_t data[16];
    uint8_t data_length;
    char description[64];
} EmulationEvent;

// Session statistics
typedef struct {
    uint32_t total_events;
    uint32_t reader_detections;
    uint32_t authentications_attempted;
    uint32_t authentications_successful;
    uint32_t authentications_failed;
    uint32_t blocks_read;
    uint32_t blocks_written;
    uint32_t sectors_accessed;
    uint32_t errors;
    uint32_t session_duration_ms;
    float auth_success_rate;
} SessionStatistics;

// Reader fingerprint (for reader identification)
typedef struct {
    uint8_t uid_request_count;
    uint8_t select_count;
    uint8_t auth_attempts;
    uint32_t avg_auth_time_ms;
    uint8_t accessed_sectors[40];
    uint8_t accessed_sector_count;
    bool appears_to_be_known_reader;
    char reader_type[32];
} ReaderFingerprint;

// Recording status
typedef enum {
    RecordStatusIdle = 0,
    RecordStatusRecording,
    RecordStatusPaused,
    RecordStatusStopped,
} RecordStatus;

#define MAX_EVENTS_PER_SESSION 1000

// Emulation Recorder manager
typedef struct EmulationRecorder EmulationRecorder;

// Allocation and deallocation
EmulationRecorder* emulation_recorder_alloc(void);
void emulation_recorder_free(EmulationRecorder* recorder);

// Recording control
bool emulation_recorder_start(EmulationRecorder* recorder);
void emulation_recorder_pause(EmulationRecorder* recorder);
void emulation_recorder_resume(EmulationRecorder* recorder);
void emulation_recorder_stop(EmulationRecorder* recorder);
RecordStatus emulation_recorder_get_status(EmulationRecorder* recorder);

// Event recording
void emulation_recorder_record_event(
    EmulationRecorder* recorder,
    EmulationEventType type,
    const char* description);

void emulation_recorder_record_authentication(
    EmulationRecorder* recorder,
    uint8_t sector,
    bool is_key_a,
    AuthResult result);

void emulation_recorder_record_block_read(
    EmulationRecorder* recorder,
    uint8_t block,
    const uint8_t* data);

void emulation_recorder_record_block_write(
    EmulationRecorder* recorder,
    uint8_t block,
    const uint8_t* data);

// Session management
void emulation_recorder_start_session(EmulationRecorder* recorder, const char* session_name);
void emulation_recorder_end_session(EmulationRecorder* recorder);
uint32_t emulation_recorder_get_session_duration(EmulationRecorder* recorder);

// Statistics
const SessionStatistics* emulation_recorder_get_statistics(EmulationRecorder* recorder);
const ReaderFingerprint* emulation_recorder_get_reader_fingerprint(EmulationRecorder* recorder);

// Event retrieval
size_t emulation_recorder_get_events(
    EmulationRecorder* recorder,
    EmulationEvent* events,
    size_t max_events);

const EmulationEvent* emulation_recorder_get_last_event(EmulationRecorder* recorder);
size_t emulation_recorder_get_event_count(EmulationRecorder* recorder);

// Event filtering
size_t emulation_recorder_get_events_by_type(
    EmulationRecorder* recorder,
    EmulationEventType type,
    EmulationEvent* events,
    size_t max_events);

size_t emulation_recorder_get_events_by_sector(
    EmulationRecorder* recorder,
    uint8_t sector,
    EmulationEvent* events,
    size_t max_events);

// Export and persistence
bool emulation_recorder_export_session(
    EmulationRecorder* recorder,
    const char* filepath);

bool emulation_recorder_export_csv(
    EmulationRecorder* recorder,
    const char* filepath);

bool emulation_recorder_export_json(
    EmulationRecorder* recorder,
    const char* filepath);

// Analysis
void emulation_recorder_analyze_reader(EmulationRecorder* recorder);
bool emulation_recorder_detect_suspicious_activity(EmulationRecorder* recorder);

// Clear and reset
void emulation_recorder_clear_events(EmulationRecorder* recorder);
void emulation_recorder_reset_statistics(EmulationRecorder* recorder);
void emulation_recorder_reset_all(EmulationRecorder* recorder);

// Utilities
const char* emulation_recorder_get_event_type_name(EmulationEventType type);
const char* emulation_recorder_get_auth_result_name(AuthResult result);
const char* emulation_recorder_get_status_name(RecordStatus status);
