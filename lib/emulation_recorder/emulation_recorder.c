#include "emulation_recorder.h"
#include <string.h>
#include <stdio.h>
#include <furi.h>
#include <storage/storage.h>

struct EmulationRecorder {
    EmulationEvent events[MAX_EVENTS_PER_SESSION];
    size_t event_count;

    SessionStatistics stats;
    ReaderFingerprint fingerprint;

    RecordStatus status;
    uint32_t session_start_time;
    uint32_t session_end_time;
    char session_name[64];

    FuriMutex* mutex;
};

static const char* event_type_names[] = {
    "Activated",
    "Deactivated",
    "Reader Detected",
    "Reader Lost",
    "Authentication",
    "Block Read",
    "Block Write",
    "Sector Read",
    "Sector Write",
    "Anti-Collision",
    "Select",
    "Halt",
    "Error",
};

static const char* auth_result_names[] = {
    "Success",
    "Failed",
    "Timeout",
};

EmulationRecorder* emulation_recorder_alloc(void) {
    EmulationRecorder* recorder = malloc(sizeof(EmulationRecorder));
    memset(recorder, 0, sizeof(EmulationRecorder));

    recorder->mutex = furi_mutex_alloc(FuriMutexTypeNormal);
    recorder->status = RecordStatusIdle;

    return recorder;
}

void emulation_recorder_free(EmulationRecorder* recorder) {
    if(!recorder) return;

    furi_mutex_free(recorder->mutex);
    free(recorder);
}

bool emulation_recorder_start(EmulationRecorder* recorder) {
    if(!recorder) return false;

    furi_mutex_acquire(recorder->mutex, FuriWaitForever);

    if(recorder->status != RecordStatusIdle && recorder->status != RecordStatusStopped) {
        furi_mutex_release(recorder->mutex);
        return false;
    }

    recorder->status = RecordStatusRecording;
    recorder->session_start_time = furi_get_tick();

    furi_mutex_release(recorder->mutex);

    return true;
}

void emulation_recorder_pause(EmulationRecorder* recorder) {
    if(!recorder) return;

    furi_mutex_acquire(recorder->mutex, FuriWaitForever);

    if(recorder->status == RecordStatusRecording) {
        recorder->status = RecordStatusPaused;
    }

    furi_mutex_release(recorder->mutex);
}

void emulation_recorder_resume(EmulationRecorder* recorder) {
    if(!recorder) return;

    furi_mutex_acquire(recorder->mutex, FuriWaitForever);

    if(recorder->status == RecordStatusPaused) {
        recorder->status = RecordStatusRecording;
    }

    furi_mutex_release(recorder->mutex);
}

void emulation_recorder_stop(EmulationRecorder* recorder) {
    if(!recorder) return;

    furi_mutex_acquire(recorder->mutex, FuriWaitForever);

    recorder->status = RecordStatusStopped;
    recorder->session_end_time = furi_get_tick();
    recorder->stats.session_duration_ms =
        recorder->session_end_time - recorder->session_start_time;

    // Calculate auth success rate
    if(recorder->stats.authentications_attempted > 0) {
        recorder->stats.auth_success_rate =
            (recorder->stats.authentications_successful * 100.0f) /
            recorder->stats.authentications_attempted;
    }

    furi_mutex_release(recorder->mutex);
}

RecordStatus emulation_recorder_get_status(EmulationRecorder* recorder) {
    if(!recorder) return RecordStatusIdle;
    return recorder->status;
}

void emulation_recorder_record_event(
    EmulationRecorder* recorder,
    EmulationEventType type,
    const char* description) {
    if(!recorder || recorder->status != RecordStatusRecording) return;

    if(recorder->event_count >= MAX_EVENTS_PER_SESSION) return;

    furi_mutex_acquire(recorder->mutex, FuriWaitForever);

    EmulationEvent* event = &recorder->events[recorder->event_count];
    memset(event, 0, sizeof(EmulationEvent));

    event->timestamp = furi_get_tick();
    event->type = type;

    if(description) {
        strncpy(event->description, description, sizeof(event->description) - 1);
    }

    recorder->event_count++;
    recorder->stats.total_events++;

    // Update specific counters
    switch(type) {
    case EmulEventReaderDetected:
        recorder->stats.reader_detections++;
        break;
    case EmulEventError:
        recorder->stats.errors++;
        break;
    default:
        break;
    }

    furi_mutex_release(recorder->mutex);
}

void emulation_recorder_record_authentication(
    EmulationRecorder* recorder,
    uint8_t sector,
    bool is_key_a,
    AuthResult result) {
    if(!recorder || recorder->status != RecordStatusRecording) return;

    if(recorder->event_count >= MAX_EVENTS_PER_SESSION) return;

    furi_mutex_acquire(recorder->mutex, FuriWaitForever);

    EmulationEvent* event = &recorder->events[recorder->event_count];
    memset(event, 0, sizeof(EmulationEvent));

    event->timestamp = furi_get_tick();
    event->type = EmulEventAuthentication;
    event->sector = sector;
    event->is_key_a = is_key_a;
    event->auth_result = result;

    snprintf(event->description, sizeof(event->description),
        "Auth Sector %d Key %s: %s",
        sector,
        is_key_a ? "A" : "B",
        emulation_recorder_get_auth_result_name(result));

    recorder->event_count++;
    recorder->stats.total_events++;
    recorder->stats.authentications_attempted++;

    if(result == AuthResultSuccess) {
        recorder->stats.authentications_successful++;

        // Add sector to accessed sectors
        bool sector_already_tracked = false;
        for(size_t i = 0; i < recorder->fingerprint.accessed_sector_count; i++) {
            if(recorder->fingerprint.accessed_sectors[i] == sector) {
                sector_already_tracked = true;
                break;
            }
        }

        if(!sector_already_tracked && recorder->fingerprint.accessed_sector_count < 40) {
            recorder->fingerprint.accessed_sectors[recorder->fingerprint.accessed_sector_count] = sector;
            recorder->fingerprint.accessed_sector_count++;
        }
    } else {
        recorder->stats.authentications_failed++;
    }

    furi_mutex_release(recorder->mutex);
}

void emulation_recorder_record_block_read(
    EmulationRecorder* recorder,
    uint8_t block,
    const uint8_t* data) {
    if(!recorder || recorder->status != RecordStatusRecording) return;

    if(recorder->event_count >= MAX_EVENTS_PER_SESSION) return;

    furi_mutex_acquire(recorder->mutex, FuriWaitForever);

    EmulationEvent* event = &recorder->events[recorder->event_count];
    memset(event, 0, sizeof(EmulationEvent));

    event->timestamp = furi_get_tick();
    event->type = EmulEventBlockRead;
    event->block_address = block;
    event->sector = block / 4;

    if(data) {
        memcpy(event->data, data, 16);
        event->data_length = 16;
    }

    snprintf(event->description, sizeof(event->description),
        "Read Block %d", block);

    recorder->event_count++;
    recorder->stats.total_events++;
    recorder->stats.blocks_read++;

    furi_mutex_release(recorder->mutex);
}

void emulation_recorder_record_block_write(
    EmulationRecorder* recorder,
    uint8_t block,
    const uint8_t* data) {
    if(!recorder || recorder->status != RecordStatusRecording) return;

    if(recorder->event_count >= MAX_EVENTS_PER_SESSION) return;

    furi_mutex_acquire(recorder->mutex, FuriWaitForever);

    EmulationEvent* event = &recorder->events[recorder->event_count];
    memset(event, 0, sizeof(EmulationEvent));

    event->timestamp = furi_get_tick();
    event->type = EmulEventBlockWrite;
    event->block_address = block;
    event->sector = block / 4;

    if(data) {
        memcpy(event->data, data, 16);
        event->data_length = 16;
    }

    snprintf(event->description, sizeof(event->description),
        "Write Block %d", block);

    recorder->event_count++;
    recorder->stats.total_events++;
    recorder->stats.blocks_written++;

    furi_mutex_release(recorder->mutex);
}

void emulation_recorder_start_session(EmulationRecorder* recorder, const char* session_name) {
    if(!recorder) return;

    furi_mutex_acquire(recorder->mutex, FuriWaitForever);

    if(session_name) {
        strncpy(recorder->session_name, session_name, sizeof(recorder->session_name) - 1);
    } else {
        snprintf(recorder->session_name, sizeof(recorder->session_name),
            "Session_%lu", furi_get_tick());
    }

    furi_mutex_release(recorder->mutex);

    emulation_recorder_start(recorder);
}

void emulation_recorder_end_session(EmulationRecorder* recorder) {
    if(!recorder) return;
    emulation_recorder_stop(recorder);
}

uint32_t emulation_recorder_get_session_duration(EmulationRecorder* recorder) {
    if(!recorder) return 0;

    if(recorder->status == RecordStatusRecording || recorder->status == RecordStatusPaused) {
        return furi_get_tick() - recorder->session_start_time;
    }

    return recorder->stats.session_duration_ms;
}

const SessionStatistics* emulation_recorder_get_statistics(EmulationRecorder* recorder) {
    if(!recorder) return NULL;
    return &recorder->stats;
}

const ReaderFingerprint* emulation_recorder_get_reader_fingerprint(EmulationRecorder* recorder) {
    if(!recorder) return NULL;
    return &recorder->fingerprint;
}

size_t emulation_recorder_get_events(
    EmulationRecorder* recorder,
    EmulationEvent* events,
    size_t max_events) {
    if(!recorder || !events) return 0;

    furi_mutex_acquire(recorder->mutex, FuriWaitForever);

    size_t count = (recorder->event_count < max_events) ? recorder->event_count : max_events;
    memcpy(events, recorder->events, count * sizeof(EmulationEvent));

    furi_mutex_release(recorder->mutex);

    return count;
}

const EmulationEvent* emulation_recorder_get_last_event(EmulationRecorder* recorder) {
    if(!recorder || recorder->event_count == 0) return NULL;
    return &recorder->events[recorder->event_count - 1];
}

size_t emulation_recorder_get_event_count(EmulationRecorder* recorder) {
    if(!recorder) return 0;
    return recorder->event_count;
}

bool emulation_recorder_export_csv(EmulationRecorder* recorder, const char* filepath) {
    if(!recorder || !filepath) return false;

    Storage* storage = furi_record_open(RECORD_STORAGE);
    File* file = storage_file_alloc(storage);

    bool success = false;

    if(storage_file_open(file, filepath, FSAM_WRITE, FSOM_CREATE_ALWAYS)) {
        // CSV header
        const char* header = "Timestamp,Type,Sector,Block,KeyType,AuthResult,Description\n";
        storage_file_write(file, header, strlen(header));

        // Write events
        for(size_t i = 0; i < recorder->event_count; i++) {
            const EmulationEvent* event = &recorder->events[i];

            char line[256];
            snprintf(line, sizeof(line), "%lu,%s,%d,%d,%s,%s,%s\n",
                event->timestamp,
                emulation_recorder_get_event_type_name(event->type),
                event->sector,
                event->block_address,
                event->is_key_a ? "A" : "B",
                emulation_recorder_get_auth_result_name(event->auth_result),
                event->description);

            storage_file_write(file, line, strlen(line));
        }

        success = true;
        storage_file_close(file);
    }

    storage_file_free(file);
    furi_record_close(RECORD_STORAGE);

    return success;
}

void emulation_recorder_analyze_reader(EmulationRecorder* recorder) {
    if(!recorder) return;

    furi_mutex_acquire(recorder->mutex, FuriWaitForever);

    // Analyze reader behavior patterns
    uint32_t uid_requests = 0;
    uint32_t select_commands = 0;

    for(size_t i = 0; i < recorder->event_count; i++) {
        const EmulationEvent* event = &recorder->events[i];

        if(event->type == EmulEventAntiCollision) {
            uid_requests++;
        } else if(event->type == EmulEventSelect) {
            select_commands++;
        }
    }

    recorder->fingerprint.uid_request_count = uid_requests;
    recorder->fingerprint.select_count = select_commands;
    recorder->fingerprint.auth_attempts = recorder->stats.authentications_attempted;

    // Simple reader type detection
    if(recorder->fingerprint.accessed_sector_count == 1 &&
       recorder->fingerprint.accessed_sectors[0] == 0) {
        strncpy(recorder->fingerprint.reader_type, "Simple Reader (MAD only)",
            sizeof(recorder->fingerprint.reader_type) - 1);
        recorder->fingerprint.appears_to_be_known_reader = true;
    } else if(recorder->fingerprint.accessed_sector_count >= 16) {
        strncpy(recorder->fingerprint.reader_type, "Full Clone Reader",
            sizeof(recorder->fingerprint.reader_type) - 1);
        recorder->fingerprint.appears_to_be_known_reader = true;
    } else if(recorder->stats.authentications_failed > recorder->stats.authentications_successful * 3) {
        strncpy(recorder->fingerprint.reader_type, "Brute Force Attacker",
            sizeof(recorder->fingerprint.reader_type) - 1);
        recorder->fingerprint.appears_to_be_known_reader = false;
    } else {
        strncpy(recorder->fingerprint.reader_type, "Unknown Reader",
            sizeof(recorder->fingerprint.reader_type) - 1);
        recorder->fingerprint.appears_to_be_known_reader = false;
    }

    furi_mutex_release(recorder->mutex);
}

bool emulation_recorder_detect_suspicious_activity(EmulationRecorder* recorder) {
    if(!recorder) return false;

    // Suspicious if many failed auth attempts
    if(recorder->stats.authentications_failed > 50) {
        return true;
    }

    // Suspicious if high error rate
    if(recorder->stats.errors > 20) {
        return true;
    }

    return false;
}

void emulation_recorder_clear_events(EmulationRecorder* recorder) {
    if(!recorder) return;

    furi_mutex_acquire(recorder->mutex, FuriWaitForever);

    memset(recorder->events, 0, sizeof(recorder->events));
    recorder->event_count = 0;

    furi_mutex_release(recorder->mutex);
}

void emulation_recorder_reset_statistics(EmulationRecorder* recorder) {
    if(!recorder) return;

    furi_mutex_acquire(recorder->mutex, FuriWaitForever);

    memset(&recorder->stats, 0, sizeof(SessionStatistics));
    memset(&recorder->fingerprint, 0, sizeof(ReaderFingerprint));

    furi_mutex_release(recorder->mutex);
}

void emulation_recorder_reset_all(EmulationRecorder* recorder) {
    if(!recorder) return;

    emulation_recorder_clear_events(recorder);
    emulation_recorder_reset_statistics(recorder);

    furi_mutex_acquire(recorder->mutex, FuriWaitForever);
    recorder->status = RecordStatusIdle;
    furi_mutex_release(recorder->mutex);
}

const char* emulation_recorder_get_event_type_name(EmulationEventType type) {
    if(type < sizeof(event_type_names) / sizeof(event_type_names[0])) {
        return event_type_names[type];
    }
    return "Unknown";
}

const char* emulation_recorder_get_auth_result_name(AuthResult result) {
    if(result < sizeof(auth_result_names) / sizeof(auth_result_names[0])) {
        return auth_result_names[result];
    }
    return "Unknown";
}

const char* emulation_recorder_get_status_name(RecordStatus status) {
    static const char* names[] = {"Idle", "Recording", "Paused", "Stopped"};
    if(status < 4) return names[status];
    return "Unknown";
}
