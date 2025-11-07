#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <furi.h>

#define SETTINGS_PATH "/ext/apps_data/chameleon_ultra/settings.conf"
#define SETTINGS_VERSION 1

// Application settings
typedef struct {
    uint8_t version;

    // Sound & Haptic
    bool sound_enabled;
    bool haptic_enabled;

    // Last connection
    uint8_t last_connection_type;  // 0=None, 1=USB, 2=BLE

    // UI preferences
    bool animations_enabled;
    bool auto_reconnect;

    // Advanced
    uint32_t response_timeout_ms;
    bool debug_logging;

} ChameleonSettings;

// Settings manager
typedef struct ChameleonSettingsManager ChameleonSettingsManager;

// Create/destroy
ChameleonSettingsManager* settings_manager_alloc();
void settings_manager_free(ChameleonSettingsManager* manager);

// Load/Save
bool settings_manager_load(ChameleonSettingsManager* manager);
bool settings_manager_save(ChameleonSettingsManager* manager);

// Get current settings
ChameleonSettings* settings_manager_get(ChameleonSettingsManager* manager);

// Set individual settings
void settings_manager_set_sound_enabled(ChameleonSettingsManager* manager, bool enabled);
void settings_manager_set_haptic_enabled(ChameleonSettingsManager* manager, bool enabled);
void settings_manager_set_animations_enabled(ChameleonSettingsManager* manager, bool enabled);
void settings_manager_set_auto_reconnect(ChameleonSettingsManager* manager, bool enabled);
void settings_manager_set_last_connection_type(ChameleonSettingsManager* manager, uint8_t type);
void settings_manager_set_response_timeout(ChameleonSettingsManager* manager, uint32_t timeout_ms);
void settings_manager_set_debug_logging(ChameleonSettingsManager* manager, bool enabled);

// Reset to defaults
void settings_manager_reset_defaults(ChameleonSettingsManager* manager);
