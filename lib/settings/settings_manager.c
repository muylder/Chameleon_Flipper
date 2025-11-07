#include "settings_manager.h"
#include <stdlib.h>
#include <string.h>
#include <storage/storage.h>

#define TAG "SettingsManager"

struct ChameleonSettingsManager {
    ChameleonSettings settings;
    FuriMutex* mutex;
};

// Default settings
static const ChameleonSettings default_settings = {
    .version = SETTINGS_VERSION,
    .sound_enabled = true,
    .haptic_enabled = true,
    .last_connection_type = 0,  // None
    .animations_enabled = true,
    .auto_reconnect = false,
    .response_timeout_ms = 2000,
    .debug_logging = false,
};

ChameleonSettingsManager* settings_manager_alloc() {
    ChameleonSettingsManager* manager = malloc(sizeof(ChameleonSettingsManager));
    memset(manager, 0, sizeof(ChameleonSettingsManager));

    manager->mutex = furi_mutex_alloc(FuriMutexTypeNormal);

    // Load defaults
    memcpy(&manager->settings, &default_settings, sizeof(ChameleonSettings));

    FURI_LOG_I(TAG, "Settings manager allocated");

    return manager;
}

void settings_manager_free(ChameleonSettingsManager* manager) {
    furi_assert(manager);

    furi_mutex_free(manager->mutex);
    free(manager);

    FURI_LOG_I(TAG, "Settings manager freed");
}

bool settings_manager_load(ChameleonSettingsManager* manager) {
    furi_assert(manager);

    FURI_LOG_I(TAG, "Loading settings from: %s", SETTINGS_PATH);

    Storage* storage = furi_record_open(RECORD_STORAGE);
    File* file = storage_file_alloc(storage);

    bool success = false;

    if(storage_file_open(file, SETTINGS_PATH, FSAM_READ, FSOM_OPEN_EXISTING)) {
        // Read version first
        uint8_t version;
        if(storage_file_read(file, &version, 1) == 1) {
            if(version == SETTINGS_VERSION) {
                // Read rest of settings
                furi_mutex_acquire(manager->mutex, FuriWaitForever);

                manager->settings.version = version;
                size_t read = storage_file_read(
                    file,
                    ((uint8_t*)&manager->settings) + 1,  // Skip version byte
                    sizeof(ChameleonSettings) - 1);

                if(read == sizeof(ChameleonSettings) - 1) {
                    success = true;
                    FURI_LOG_I(TAG, "Settings loaded successfully");
                } else {
                    FURI_LOG_W(TAG, "Incomplete settings file, using defaults");
                }

                furi_mutex_release(manager->mutex);
            } else {
                FURI_LOG_W(TAG, "Settings version mismatch: %d != %d", version, SETTINGS_VERSION);
            }
        }

        storage_file_close(file);
    } else {
        FURI_LOG_I(TAG, "Settings file not found, using defaults");
    }

    storage_file_free(file);
    furi_record_close(RECORD_STORAGE);

    return success;
}

bool settings_manager_save(ChameleonSettingsManager* manager) {
    furi_assert(manager);

    FURI_LOG_I(TAG, "Saving settings to: %s", SETTINGS_PATH);

    Storage* storage = furi_record_open(RECORD_STORAGE);

    // Ensure directory exists
    storage_common_mkdir(storage, "/ext/apps_data");
    storage_common_mkdir(storage, "/ext/apps_data/chameleon_ultra");

    File* file = storage_file_alloc(storage);

    bool success = false;

    if(storage_file_open(file, SETTINGS_PATH, FSAM_WRITE, FSOM_CREATE_ALWAYS)) {
        furi_mutex_acquire(manager->mutex, FuriWaitForever);

        size_t written = storage_file_write(file, &manager->settings, sizeof(ChameleonSettings));

        if(written == sizeof(ChameleonSettings)) {
            success = true;
            FURI_LOG_I(TAG, "Settings saved successfully");
        } else {
            FURI_LOG_E(TAG, "Failed to write settings");
        }

        furi_mutex_release(manager->mutex);

        storage_file_close(file);
    } else {
        FURI_LOG_E(TAG, "Failed to open settings file for writing");
    }

    storage_file_free(file);
    furi_record_close(RECORD_STORAGE);

    return success;
}

ChameleonSettings* settings_manager_get(ChameleonSettingsManager* manager) {
    furi_assert(manager);
    return &manager->settings;
}

void settings_manager_set_sound_enabled(ChameleonSettingsManager* manager, bool enabled) {
    furi_assert(manager);
    furi_mutex_acquire(manager->mutex, FuriWaitForever);
    manager->settings.sound_enabled = enabled;
    furi_mutex_release(manager->mutex);
    FURI_LOG_I(TAG, "Sound: %s", enabled ? "ON" : "OFF");
}

void settings_manager_set_haptic_enabled(ChameleonSettingsManager* manager, bool enabled) {
    furi_assert(manager);
    furi_mutex_acquire(manager->mutex, FuriWaitForever);
    manager->settings.haptic_enabled = enabled;
    furi_mutex_release(manager->mutex);
    FURI_LOG_I(TAG, "Haptic: %s", enabled ? "ON" : "OFF");
}

void settings_manager_set_animations_enabled(ChameleonSettingsManager* manager, bool enabled) {
    furi_assert(manager);
    furi_mutex_acquire(manager->mutex, FuriWaitForever);
    manager->settings.animations_enabled = enabled;
    furi_mutex_release(manager->mutex);
    FURI_LOG_I(TAG, "Animations: %s", enabled ? "ON" : "OFF");
}

void settings_manager_set_auto_reconnect(ChameleonSettingsManager* manager, bool enabled) {
    furi_assert(manager);
    furi_mutex_acquire(manager->mutex, FuriWaitForever);
    manager->settings.auto_reconnect = enabled;
    furi_mutex_release(manager->mutex);
    FURI_LOG_I(TAG, "Auto-reconnect: %s", enabled ? "ON" : "OFF");
}

void settings_manager_set_last_connection_type(ChameleonSettingsManager* manager, uint8_t type) {
    furi_assert(manager);
    furi_mutex_acquire(manager->mutex, FuriWaitForever);
    manager->settings.last_connection_type = type;
    furi_mutex_release(manager->mutex);
    FURI_LOG_I(TAG, "Last connection type: %d", type);
}

void settings_manager_set_response_timeout(ChameleonSettingsManager* manager, uint32_t timeout_ms) {
    furi_assert(manager);
    furi_mutex_acquire(manager->mutex, FuriWaitForever);
    manager->settings.response_timeout_ms = timeout_ms;
    furi_mutex_release(manager->mutex);
    FURI_LOG_I(TAG, "Response timeout: %lu ms", timeout_ms);
}

void settings_manager_set_debug_logging(ChameleonSettingsManager* manager, bool enabled) {
    furi_assert(manager);
    furi_mutex_acquire(manager->mutex, FuriWaitForever);
    manager->settings.debug_logging = enabled;
    furi_mutex_release(manager->mutex);
    FURI_LOG_I(TAG, "Debug logging: %s", enabled ? "ON" : "OFF");
}

void settings_manager_reset_defaults(ChameleonSettingsManager* manager) {
    furi_assert(manager);

    FURI_LOG_I(TAG, "Resetting to defaults");

    furi_mutex_acquire(manager->mutex, FuriWaitForever);
    memcpy(&manager->settings, &default_settings, sizeof(ChameleonSettings));
    furi_mutex_release(manager->mutex);
}
