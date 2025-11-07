#pragma once

#include <furi.h>
#include <gui/gui.h>
#include <gui/view_dispatcher.h>
#include <gui/scene_manager.h>
#include <gui/modules/submenu.h>
#include <gui/modules/variable_item_list.h>
#include <gui/modules/text_input.h>
#include <gui/modules/popup.h>
#include <gui/modules/widget.h>
#include <gui/modules/loading.h>
#include <dialogs/dialogs.h>
#include <notification/notification_messages.h>
#include <storage/storage.h>

#include "scenes/chameleon_scene.h"
#include "lib/chameleon_protocol/chameleon_protocol.h"
#include "lib/uart_handler/uart_handler.h"
#include "lib/ble_handler/ble_handler.h"
#include "lib/response_handler/response_handler.h"
#include "lib/logger/chameleon_logger.h"
#include "lib/key_manager/key_manager.h"
#include "lib/sound_effects/sound_effects.h"
#include "lib/settings/settings_manager.h"
#include "lib/statistics/statistics_manager.h"
#include "views/chameleon_animation_view.h"

#define TAG "ChameleonUltra"

// Connection types
typedef enum {
    ChameleonConnectionNone,
    ChameleonConnectionUSB,
    ChameleonConnectionBLE,
} ChameleonConnectionType;

// Connection status
typedef enum {
    ChameleonStatusDisconnected,
    ChameleonStatusConnecting,
    ChameleonStatusConnected,
    ChameleonStatusError,
} ChameleonStatus;

// Device modes (from protocol)
typedef enum {
    ChameleonModeReader = 0,
    ChameleonModeEmulator = 1,
} ChameleonDeviceMode;

// Device model
typedef enum {
    ChameleonModelUltra = 0,
    ChameleonModelLite = 1,
} ChameleonModel;

// Tag types for slots
typedef enum {
    TagTypeUnknown = 0,
    TagTypeMifareClassic1K = 1,
    TagTypeMifareClassic4K = 2,
    TagTypeMifareUltralight = 3,
    TagTypeNTAG213 = 4,
    TagTypeNTAG215 = 5,
    TagTypeNTAG216 = 6,
    TagTypeEM410X = 7,
    TagTypeHIDProx = 8,
} ChameleonTagType;

// Slot information
typedef struct {
    uint8_t slot_number;
    ChameleonTagType hf_tag_type;
    ChameleonTagType lf_tag_type;
    bool hf_enabled;
    bool lf_enabled;
    char nickname[33]; // 32 bytes + null terminator
} ChameleonSlot;

// Device information
typedef struct {
    uint8_t major_version;
    uint8_t minor_version;
    char git_version[32];
    uint64_t chip_id;
    ChameleonModel model;
    ChameleonDeviceMode mode;
    bool connected;
} ChameleonDeviceInfo;

// Views
typedef enum {
    ChameleonViewSubmenu,
    ChameleonViewVariableItemList,
    ChameleonViewTextInput,
    ChameleonViewPopup,
    ChameleonViewWidget,
    ChameleonViewLoading,
    ChameleonViewAnimation,
} ChameleonView;

// Main application structure
typedef struct {
    Gui* gui;
    ViewDispatcher* view_dispatcher;
    SceneManager* scene_manager;
    NotificationApp* notifications;
    DialogsApp* dialogs;
    Storage* storage;

    // Views
    Submenu* submenu;
    VariableItemList* variable_item_list;
    TextInput* text_input;
    Popup* popup;
    Widget* widget;
    Loading* loading;
    ChameleonAnimationView* animation_view;

    // Connection
    ChameleonConnectionType connection_type;
    ChameleonStatus connection_status;
    UartHandler* uart_handler;
    BleHandler* ble_handler;

    // Device data
    ChameleonDeviceInfo device_info;
    ChameleonSlot slots[8]; // 8 slots (0-7)
    uint8_t active_slot;

    // Temporary buffers
    char text_buffer[64];
    uint8_t data_buffer[512];

    // Protocol handler
    ChameleonProtocol* protocol;

    // Response handler
    ChameleonResponseHandler* response_handler;

    // Logger
    ChameleonLogger* logger;

    // Key manager
    KeyManager* key_manager;

    // Settings manager
    ChameleonSettingsManager* settings_manager;

    // Statistics manager
    StatisticsManager* statistics_manager;
} ChameleonApp;

// Application lifecycle
ChameleonApp* chameleon_app_alloc();
void chameleon_app_free(ChameleonApp* app);

// Connection management
bool chameleon_app_connect_usb(ChameleonApp* app);
bool chameleon_app_connect_ble(ChameleonApp* app);
void chameleon_app_disconnect(ChameleonApp* app);

// Device operations
bool chameleon_app_get_device_info(ChameleonApp* app);
bool chameleon_app_get_slots_info(ChameleonApp* app);
bool chameleon_app_set_active_slot(ChameleonApp* app, uint8_t slot);
bool chameleon_app_set_slot_nickname(ChameleonApp* app, uint8_t slot, const char* nickname);
bool chameleon_app_change_device_mode(ChameleonApp* app, ChameleonDeviceMode mode);

// Tag operations - Reading
bool chameleon_app_hf14a_scan(ChameleonApp* app, uint8_t* uid, uint8_t* uid_len, uint8_t* atqa, uint8_t* sak);
bool chameleon_app_mf1_read_block(ChameleonApp* app, uint8_t block, uint8_t key_type, const uint8_t* key, uint8_t* data);
bool chameleon_app_em410x_scan(ChameleonApp* app, uint8_t* id);
bool chameleon_app_save_tag_to_file(ChameleonApp* app, const char* filename, const uint8_t* data, size_t data_len);

// Tag operations - Writing
bool chameleon_app_mf1_write_emu_block(ChameleonApp* app, uint8_t block, const uint8_t* data);
bool chameleon_app_em410x_set_emu_id(ChameleonApp* app, const uint8_t* id);
bool chameleon_app_load_tag_from_file(ChameleonApp* app, const char* filepath, uint8_t* data, size_t* data_len, ChameleonTagType* tag_type);
