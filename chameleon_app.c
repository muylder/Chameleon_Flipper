#include "chameleon_app_i.h"
#include <stdlib.h>
#include <string.h>

#undef TAG
#define TAG "ChameleonApp"

static bool chameleon_app_custom_event_callback(void* context, uint32_t event) {
    furi_assert(context);
    ChameleonApp* app = context;
    return scene_manager_handle_custom_event(app->scene_manager, event);
}

static bool chameleon_app_back_event_callback(void* context) {
    furi_assert(context);
    ChameleonApp* app = context;
    return scene_manager_handle_back_event(app->scene_manager);
}

ChameleonApp* chameleon_app_alloc() {
    ChameleonApp* app = malloc(sizeof(ChameleonApp));
    memset(app, 0, sizeof(ChameleonApp));

    // Initialize services
    app->gui = furi_record_open(RECORD_GUI);
    app->notifications = furi_record_open(RECORD_NOTIFICATION);
    app->dialogs = furi_record_open(RECORD_DIALOGS);
    app->storage = furi_record_open(RECORD_STORAGE);

    // Initialize view dispatcher
    app->view_dispatcher = view_dispatcher_alloc();
    view_dispatcher_set_event_callback_context(app->view_dispatcher, app);
    view_dispatcher_set_custom_event_callback(app->view_dispatcher, chameleon_app_custom_event_callback);
    view_dispatcher_set_navigation_event_callback(app->view_dispatcher, chameleon_app_back_event_callback);
    view_dispatcher_attach_to_gui(app->view_dispatcher, app->gui, ViewDispatcherTypeFullscreen);

    // Initialize scene manager
    app->scene_manager = scene_manager_alloc(&chameleon_scene_handlers, app);

    // Initialize views
    app->submenu = submenu_alloc();
    view_dispatcher_add_view(app->view_dispatcher, ChameleonViewSubmenu, submenu_get_view(app->submenu));

    app->variable_item_list = variable_item_list_alloc();
    view_dispatcher_add_view(
        app->view_dispatcher,
        ChameleonViewVariableItemList,
        variable_item_list_get_view(app->variable_item_list));

    app->text_input = text_input_alloc();
    view_dispatcher_add_view(
        app->view_dispatcher,
        ChameleonViewTextInput,
        text_input_get_view(app->text_input));

    app->popup = popup_alloc();
    view_dispatcher_add_view(app->view_dispatcher, ChameleonViewPopup, popup_get_view(app->popup));

    app->widget = widget_alloc();
    view_dispatcher_add_view(app->view_dispatcher, ChameleonViewWidget, widget_get_view(app->widget));

    app->loading = loading_alloc();
    view_dispatcher_add_view(app->view_dispatcher, ChameleonViewLoading, loading_get_view(app->loading));

    app->animation_view = chameleon_animation_view_alloc();
    view_dispatcher_add_view(
        app->view_dispatcher,
        ChameleonViewAnimation,
        chameleon_animation_view_get_view(app->animation_view));

    // Initialize protocol
    app->protocol = chameleon_protocol_alloc();

    // Initialize response handler
    app->response_handler = chameleon_response_handler_alloc();

    // Initialize logger
    app->logger = chameleon_logger_alloc();
    CHAM_LOG_I(app->logger, TAG, "Chameleon Ultra app initialized");

    // Initialize key manager
    app->key_manager = key_manager_alloc();
    key_manager_load_defaults(app->key_manager);  // Load default Mifare keys
    CHAM_LOG_I(app->logger, TAG, "Key manager initialized with %zu keys", key_manager_get_count(app->key_manager));

    // Initialize settings manager
    app->settings_manager = settings_manager_alloc();
    settings_manager_load(app->settings_manager);  // Load saved settings
    CHAM_LOG_I(app->logger, TAG, "Settings loaded (sound:%d haptic:%d)",
        settings_manager_get(app->settings_manager)->sound_enabled,
        settings_manager_get(app->settings_manager)->haptic_enabled);

    // Initialize handlers
    app->uart_handler = uart_handler_alloc();
    app->ble_handler = ble_handler_alloc();

    // Initialize connection state
    app->connection_type = ChameleonConnectionNone;
    app->connection_status = ChameleonStatusDisconnected;

    // Initialize slots
    for(uint8_t i = 0; i < 8; i++) {
        app->slots[i].slot_number = i;
        app->slots[i].hf_tag_type = TagTypeUnknown;
        app->slots[i].lf_tag_type = TagTypeUnknown;
        app->slots[i].hf_enabled = false;
        app->slots[i].lf_enabled = false;
        memset(app->slots[i].nickname, 0, sizeof(app->slots[i].nickname));
    }

    return app;
}

void chameleon_app_free(ChameleonApp* app) {
    furi_assert(app);

    // Disconnect if connected
    chameleon_app_disconnect(app);

    // Free handlers
    uart_handler_free(app->uart_handler);
    ble_handler_free(app->ble_handler);

    // Free protocol and response handler
    chameleon_protocol_free(app->protocol);
    chameleon_response_handler_free(app->response_handler);

    // Free key manager
    key_manager_free(app->key_manager);

    // Save and free settings
    settings_manager_save(app->settings_manager);
    settings_manager_free(app->settings_manager);

    // Free logger
    CHAM_LOG_I(app->logger, TAG, "Chameleon Ultra app shutting down");
    chameleon_logger_free(app->logger);

    // Free views
    view_dispatcher_remove_view(app->view_dispatcher, ChameleonViewSubmenu);
    submenu_free(app->submenu);

    view_dispatcher_remove_view(app->view_dispatcher, ChameleonViewVariableItemList);
    variable_item_list_free(app->variable_item_list);

    view_dispatcher_remove_view(app->view_dispatcher, ChameleonViewTextInput);
    text_input_free(app->text_input);

    view_dispatcher_remove_view(app->view_dispatcher, ChameleonViewPopup);
    popup_free(app->popup);

    view_dispatcher_remove_view(app->view_dispatcher, ChameleonViewWidget);
    widget_free(app->widget);

    view_dispatcher_remove_view(app->view_dispatcher, ChameleonViewLoading);
    loading_free(app->loading);

    view_dispatcher_remove_view(app->view_dispatcher, ChameleonViewAnimation);
    chameleon_animation_view_free(app->animation_view);

    // Free scene manager
    scene_manager_free(app->scene_manager);

    // Free view dispatcher
    view_dispatcher_free(app->view_dispatcher);

    // Close services
    furi_record_close(RECORD_GUI);
    furi_record_close(RECORD_NOTIFICATION);
    furi_record_close(RECORD_DIALOGS);
    furi_record_close(RECORD_STORAGE);

    free(app);
}

// UART RX callback - passa dados recebidos para o response handler
static void chameleon_app_uart_rx_callback(const uint8_t* data, size_t length, void* context) {
    ChameleonApp* app = context;
    chameleon_response_handler_process_data(app->response_handler, data, length);
}

bool chameleon_app_connect_usb(ChameleonApp* app) {
    furi_assert(app);

    FURI_LOG_I(TAG, "Connecting via USB");
    CHAM_LOG_I(app->logger, "USB", "Initiating USB connection");

    if(!uart_handler_init(app->uart_handler)) {
        FURI_LOG_E(TAG, "Failed to initialize UART");
        CHAM_LOG_E(app->logger, "USB", "Failed to initialize UART handler");
        sound_effects_error();  // Play error sound
        return false;
    }

    // Configure UART to send received data to response handler
    uart_handler_set_rx_callback(app->uart_handler, chameleon_app_uart_rx_callback, app);

    uart_handler_start_rx(app->uart_handler);

    app->connection_type = ChameleonConnectionUSB;
    app->connection_status = ChameleonStatusConnected;

    FURI_LOG_I(TAG, "Connected via USB");
    CHAM_LOG_I(app->logger, "USB", "Successfully connected via USB");
    sound_effects_success();  // Play success sound
    return true;
}

bool chameleon_app_connect_ble(ChameleonApp* app) {
    furi_assert(app);

    FURI_LOG_I(TAG, "Connecting via BLE");
    CHAM_LOG_I(app->logger, "BLE", "Initiating BLE connection");

    if(!ble_handler_init(app->ble_handler)) {
        FURI_LOG_E(TAG, "Failed to initialize BLE");
        CHAM_LOG_E(app->logger, "BLE", "Failed to initialize BLE handler");
        return false;
    }

    if(!ble_handler_start_scan(app->ble_handler)) {
        FURI_LOG_E(TAG, "Failed to start BLE scan");
        CHAM_LOG_E(app->logger, "BLE", "Failed to start BLE scan");
        return false;
    }

    FURI_LOG_I(TAG, "BLE scan started");
    CHAM_LOG_I(app->logger, "BLE", "BLE scan started successfully");
    return true;
}

void chameleon_app_disconnect(ChameleonApp* app) {
    furi_assert(app);

    FURI_LOG_I(TAG, "Disconnecting");

    if(app->connection_type == ChameleonConnectionUSB) {
        CHAM_LOG_I(app->logger, "USB", "Disconnecting USB");
        uart_handler_deinit(app->uart_handler);
    } else if(app->connection_type == ChameleonConnectionBLE) {
        CHAM_LOG_I(app->logger, "BLE", "Disconnecting BLE");
        ble_handler_disconnect(app->ble_handler);
        ble_handler_deinit(app->ble_handler);
    }

    app->connection_type = ChameleonConnectionNone;
    app->connection_status = ChameleonStatusDisconnected;

    FURI_LOG_I(TAG, "Disconnected");
    CHAM_LOG_I(app->logger, "Connection", "Device disconnected");
}

bool chameleon_app_get_device_info(ChameleonApp* app) {
    furi_assert(app);

    FURI_LOG_I(TAG, "Getting device info");

    // Build GET_APP_VERSION command
    uint8_t cmd_buffer[CHAMELEON_FRAME_OVERHEAD];
    size_t cmd_len;

    if(!chameleon_protocol_build_cmd_no_data(app->protocol, CMD_GET_APP_VERSION, cmd_buffer, &cmd_len)) {
        FURI_LOG_E(TAG, "Failed to build GET_APP_VERSION command");
        return false;
    }

    // Send command
    if(app->connection_type == ChameleonConnectionUSB) {
        uart_handler_send(app->uart_handler, cmd_buffer, cmd_len);
    } else if(app->connection_type == ChameleonConnectionBLE) {
        ble_handler_send(app->ble_handler, cmd_buffer, cmd_len);
    } else {
        FURI_LOG_E(TAG, "Not connected");
        return false;
    }

    // Wait for response
    ChameleonResponse response;
    if(!chameleon_response_handler_wait_for_response(
           app->response_handler, CMD_GET_APP_VERSION, &response, RESPONSE_TIMEOUT_MS)) {
        FURI_LOG_E(TAG, "Timeout waiting for device info");
        return false;
    }

    // Check response status
    if(response.status != STATUS_SUCCESS) {
        FURI_LOG_E(TAG, "Device info request failed: status=%04X", response.status);
        return false;
    }

    // Parse response data (should be 2 bytes: major.minor)
    if(response.data_len >= 2) {
        app->device_info.major_version = response.data[0];
        app->device_info.minor_version = response.data[1];
        app->device_info.connected = true;

        FURI_LOG_I(
            TAG,
            "Device firmware version: %d.%d",
            app->device_info.major_version,
            app->device_info.minor_version);
    }

    // Get chip ID
    if(!chameleon_protocol_build_cmd_no_data(app->protocol, CMD_GET_DEVICE_CHIP_ID, cmd_buffer, &cmd_len)) {
        FURI_LOG_E(TAG, "Failed to build GET_DEVICE_CHIP_ID command");
        return false;
    }

    if(app->connection_type == ChameleonConnectionUSB) {
        uart_handler_send(app->uart_handler, cmd_buffer, cmd_len);
    } else {
        ble_handler_send(app->ble_handler, cmd_buffer, cmd_len);
    }

    if(chameleon_response_handler_wait_for_response(
           app->response_handler, CMD_GET_DEVICE_CHIP_ID, &response, RESPONSE_TIMEOUT_MS)) {
        if(response.status == STATUS_SUCCESS && response.data_len >= 8) {
            // Parse 8-byte chip ID (Big Endian)
            app->device_info.chip_id = 0;
            for(uint8_t i = 0; i < 8; i++) {
                app->device_info.chip_id = (app->device_info.chip_id << 8) | response.data[i];
            }
            FURI_LOG_I(TAG, "Chip ID: %016llX", app->device_info.chip_id);
        }
    }

    // Get device model
    if(!chameleon_protocol_build_cmd_no_data(app->protocol, CMD_GET_DEVICE_MODEL, cmd_buffer, &cmd_len)) {
        FURI_LOG_E(TAG, "Failed to build GET_DEVICE_MODEL command");
        return false;
    }

    if(app->connection_type == ChameleonConnectionUSB) {
        uart_handler_send(app->uart_handler, cmd_buffer, cmd_len);
    } else {
        ble_handler_send(app->ble_handler, cmd_buffer, cmd_len);
    }

    if(chameleon_response_handler_wait_for_response(
           app->response_handler, CMD_GET_DEVICE_MODEL, &response, RESPONSE_TIMEOUT_MS)) {
        if(response.status == STATUS_SUCCESS && response.data_len >= 1) {
            app->device_info.model = (ChameleonModel)response.data[0];
            FURI_LOG_I(TAG, "Device model: %s", app->device_info.model == ChameleonModelUltra ? "Ultra" : "Lite");
        }
    }

    // Get device mode
    if(!chameleon_protocol_build_cmd_no_data(app->protocol, CMD_GET_DEVICE_MODE, cmd_buffer, &cmd_len)) {
        FURI_LOG_E(TAG, "Failed to build GET_DEVICE_MODE command");
        return false;
    }

    if(app->connection_type == ChameleonConnectionUSB) {
        uart_handler_send(app->uart_handler, cmd_buffer, cmd_len);
    } else {
        ble_handler_send(app->ble_handler, cmd_buffer, cmd_len);
    }

    if(chameleon_response_handler_wait_for_response(
           app->response_handler, CMD_GET_DEVICE_MODE, &response, RESPONSE_TIMEOUT_MS)) {
        if(response.status == STATUS_SUCCESS && response.data_len >= 1) {
            app->device_info.mode = (ChameleonDeviceMode)response.data[0];
            FURI_LOG_I(TAG, "Device mode: %s", app->device_info.mode == ChameleonModeReader ? "Reader" : "Emulator");
        }
    }

    FURI_LOG_I(TAG, "Device info retrieved successfully");
    return true;
}

bool chameleon_app_get_slots_info(ChameleonApp* app) {
    furi_assert(app);

    FURI_LOG_I(TAG, "Getting slots info");

    // Build GET_SLOT_INFO command
    uint8_t cmd_buffer[CHAMELEON_FRAME_OVERHEAD];
    size_t cmd_len;

    if(!chameleon_protocol_build_cmd_no_data(app->protocol, CMD_GET_SLOT_INFO, cmd_buffer, &cmd_len)) {
        FURI_LOG_E(TAG, "Failed to build GET_SLOT_INFO command");
        return false;
    }

    // Send command
    if(app->connection_type == ChameleonConnectionUSB) {
        uart_handler_send(app->uart_handler, cmd_buffer, cmd_len);
    } else if(app->connection_type == ChameleonConnectionBLE) {
        ble_handler_send(app->ble_handler, cmd_buffer, cmd_len);
    } else {
        FURI_LOG_E(TAG, "Not connected");
        return false;
    }

    // Wait for response
    ChameleonResponse response;
    if(!chameleon_response_handler_wait_for_response(
           app->response_handler, CMD_GET_SLOT_INFO, &response, RESPONSE_TIMEOUT_MS)) {
        FURI_LOG_E(TAG, "Timeout waiting for slots info");
        return false;
    }

    // Check response status
    if(response.status != STATUS_SUCCESS) {
        FURI_LOG_E(TAG, "Slots info request failed: status=%04X", response.status);
        return false;
    }

    // Parse response data (32 bytes: 8 slots x 4 bytes each)
    // Each slot has: HF type (1 byte) + LF type (1 byte) + 2 reserved bytes
    if(response.data_len >= 32) {
        for(uint8_t i = 0; i < 8; i++) {
            app->slots[i].hf_tag_type = (ChameleonTagType)response.data[i * 4];
            app->slots[i].lf_tag_type = (ChameleonTagType)response.data[i * 4 + 1];

            FURI_LOG_D(TAG, "Slot %d: HF=%d, LF=%d", i, app->slots[i].hf_tag_type, app->slots[i].lf_tag_type);
        }
    }

    // Get nicknames for each slot
    for(uint8_t i = 0; i < 8; i++) {
        // TODO: Implementar GET_SLOT_TAG_NICK quando disponível
        // Por agora, manter nicknames existentes ou usar default
        if(strlen(app->slots[i].nickname) == 0) {
            snprintf(app->slots[i].nickname, sizeof(app->slots[i].nickname), "Slot %d", i);
        }
    }

    FURI_LOG_I(TAG, "Slots info retrieved successfully");
    return true;
}

bool chameleon_app_set_active_slot(ChameleonApp* app, uint8_t slot) {
    furi_assert(app);
    furi_assert(slot < 8);

    FURI_LOG_I(TAG, "Setting active slot to %d", slot);

    // Build SET_ACTIVE_SLOT command
    uint8_t cmd_buffer[CHAMELEON_FRAME_OVERHEAD + 1];
    size_t cmd_len;

    if(!chameleon_protocol_build_cmd_with_data(app->protocol, CMD_SET_ACTIVE_SLOT, &slot, 1, cmd_buffer, &cmd_len)) {
        FURI_LOG_E(TAG, "Failed to build SET_ACTIVE_SLOT command");
        return false;
    }

    // Send command
    if(app->connection_type == ChameleonConnectionUSB) {
        uart_handler_send(app->uart_handler, cmd_buffer, cmd_len);
    } else if(app->connection_type == ChameleonConnectionBLE) {
        ble_handler_send(app->ble_handler, cmd_buffer, cmd_len);
    } else {
        FURI_LOG_E(TAG, "Not connected");
        return false;
    }

    app->active_slot = slot;

    FURI_LOG_I(TAG, "Active slot set to %d", slot);
    return true;
}

bool chameleon_app_set_slot_nickname(ChameleonApp* app, uint8_t slot, const char* nickname) {
    furi_assert(app);
    furi_assert(slot < 8);
    furi_assert(nickname);

    FURI_LOG_I(TAG, "Setting slot %d nickname to: %s", slot, nickname);

    // Build SET_SLOT_TAG_NICK command
    uint8_t data[33];
    data[0] = slot;
    size_t nick_len = strlen(nickname);
    if(nick_len > 32) nick_len = 32;
    memcpy(&data[1], nickname, nick_len);

    uint8_t cmd_buffer[CHAMELEON_FRAME_OVERHEAD + 33];
    size_t cmd_len;

    if(!chameleon_protocol_build_cmd_with_data(
           app->protocol, CMD_SET_SLOT_TAG_NICK, data, 1 + nick_len, cmd_buffer, &cmd_len)) {
        FURI_LOG_E(TAG, "Failed to build SET_SLOT_TAG_NICK command");
        return false;
    }

    // Send command
    if(app->connection_type == ChameleonConnectionUSB) {
        uart_handler_send(app->uart_handler, cmd_buffer, cmd_len);
    } else if(app->connection_type == ChameleonConnectionBLE) {
        ble_handler_send(app->ble_handler, cmd_buffer, cmd_len);
    } else {
        FURI_LOG_E(TAG, "Not connected");
        return false;
    }

    // Update local cache
    strncpy(app->slots[slot].nickname, nickname, sizeof(app->slots[slot].nickname) - 1);

    FURI_LOG_I(TAG, "Slot nickname updated");
    return true;
}

bool chameleon_app_change_device_mode(ChameleonApp* app, ChameleonDeviceMode mode) {
    furi_assert(app);

    FURI_LOG_I(TAG, "Changing device mode to %d", mode);

    uint8_t mode_byte = (uint8_t)mode;

    uint8_t cmd_buffer[CHAMELEON_FRAME_OVERHEAD + 1];
    size_t cmd_len;

    if(!chameleon_protocol_build_cmd_with_data(
           app->protocol, CMD_CHANGE_DEVICE_MODE, &mode_byte, 1, cmd_buffer, &cmd_len)) {
        FURI_LOG_E(TAG, "Failed to build CHANGE_DEVICE_MODE command");
        return false;
    }

    // Send command
    if(app->connection_type == ChameleonConnectionUSB) {
        uart_handler_send(app->uart_handler, cmd_buffer, cmd_len);
    } else if(app->connection_type == ChameleonConnectionBLE) {
        ble_handler_send(app->ble_handler, cmd_buffer, cmd_len);
    } else {
        FURI_LOG_E(TAG, "Not connected");
        return false;
    }

    app->device_info.mode = mode;

    FURI_LOG_I(TAG, "Device mode changed");
    return true;
}

// Tag Operations

bool chameleon_app_hf14a_scan(ChameleonApp* app, uint8_t* uid, uint8_t* uid_len, uint8_t* atqa, uint8_t* sak) {
    furi_assert(app);
    furi_assert(uid);
    furi_assert(uid_len);

    FURI_LOG_I(TAG, "Scanning for HF14A tags");
    CHAM_LOG_I(app->logger, "TagRead", "Starting HF14A scan");
    sound_effects_scan();  // Play scan sound

    // Build HF14A_SCAN command
    uint8_t cmd_buffer[CHAMELEON_FRAME_OVERHEAD];
    size_t cmd_len;

    if(!chameleon_protocol_build_cmd_no_data(app->protocol, CMD_HF14A_SCAN, cmd_buffer, &cmd_len)) {
        FURI_LOG_E(TAG, "Failed to build HF14A_SCAN command");
        CHAM_LOG_E(app->logger, "TagRead", "Failed to build HF14A_SCAN command");
        return false;
    }

    // Send command
    if(app->connection_type == ChameleonConnectionUSB) {
        uart_handler_send(app->uart_handler, cmd_buffer, cmd_len);
    } else if(app->connection_type == ChameleonConnectionBLE) {
        ble_handler_send(app->ble_handler, cmd_buffer, cmd_len);
    } else {
        FURI_LOG_E(TAG, "Not connected");
        return false;
    }

    // Wait for response
    ChameleonResponse response;
    if(!chameleon_response_handler_wait_for_response(
           app->response_handler, CMD_HF14A_SCAN, &response, RESPONSE_TIMEOUT_MS)) {
        FURI_LOG_E(TAG, "Timeout waiting for HF14A scan");
        return false;
    }

    // Check response status
    if(response.status != STATUS_HF_TAG_OK) {
        FURI_LOG_E(TAG, "HF14A scan failed: status=%04X", response.status);
        CHAM_LOG_E(app->logger, "TagRead", "HF14A scan failed with status %04X", response.status);
        return false;
    }

    // Parse response: UID (variable length) + ATQA (2 bytes) + SAK (1 byte) + ATS (variable)
    // Minimum: 4 (UID) + 2 (ATQA) + 1 (SAK) = 7 bytes
    if(response.data_len < 7) {
        FURI_LOG_E(TAG, "Invalid HF14A response length: %u", response.data_len);
        CHAM_LOG_E(app->logger, "TagRead", "Invalid HF14A response length: %u", response.data_len);
        return false;
    }

    // Extract UID length (first byte)
    *uid_len = response.data[0];
    if(*uid_len > 10) *uid_len = 10; // Max UID length

    // Copy UID
    memcpy(uid, &response.data[1], *uid_len);

    // Copy ATQA (2 bytes after UID)
    if(atqa) {
        memcpy(atqa, &response.data[1 + *uid_len], 2);
    }

    // Copy SAK (1 byte after ATQA)
    if(sak) {
        *sak = response.data[1 + *uid_len + 2];
    }

    FURI_LOG_I(TAG, "HF14A tag found: UID len=%u", *uid_len);
    CHAM_LOG_I(app->logger, "TagRead", "HF14A tag found (UID length=%u)", *uid_len);
    sound_effects_success();  // Play success sound
    return true;
}

bool chameleon_app_mf1_read_block(ChameleonApp* app, uint8_t block, uint8_t key_type, const uint8_t* key, uint8_t* data) {
    furi_assert(app);
    furi_assert(key);
    furi_assert(data);

    FURI_LOG_I(TAG, "Reading Mifare block %u", block);

    // Build MF1_READ_ONE_BLOCK command
    // Data: key_type (1) + block (1) + key (6) = 8 bytes
    uint8_t cmd_data[8];
    cmd_data[0] = key_type; // 0x60 for Key A, 0x61 for Key B
    cmd_data[1] = block;
    memcpy(&cmd_data[2], key, 6);

    uint8_t cmd_buffer[CHAMELEON_FRAME_OVERHEAD + 8];
    size_t cmd_len;

    if(!chameleon_protocol_build_cmd_with_data(app->protocol, CMD_MF1_READ_ONE_BLOCK, cmd_data, 8, cmd_buffer, &cmd_len)) {
        FURI_LOG_E(TAG, "Failed to build MF1_READ_ONE_BLOCK command");
        return false;
    }

    // Send command
    if(app->connection_type == ChameleonConnectionUSB) {
        uart_handler_send(app->uart_handler, cmd_buffer, cmd_len);
    } else if(app->connection_type == ChameleonConnectionBLE) {
        ble_handler_send(app->ble_handler, cmd_buffer, cmd_len);
    } else {
        FURI_LOG_E(TAG, "Not connected");
        return false;
    }

    // Wait for response
    ChameleonResponse response;
    if(!chameleon_response_handler_wait_for_response(
           app->response_handler, CMD_MF1_READ_ONE_BLOCK, &response, RESPONSE_TIMEOUT_MS)) {
        FURI_LOG_E(TAG, "Timeout waiting for block read");
        return false;
    }

    // Check response status
    if(response.status != STATUS_HF_TAG_OK) {
        FURI_LOG_E(TAG, "Block read failed: status=%04X", response.status);
        return false;
    }

    // Response should be 16 bytes (one Mifare block)
    if(response.data_len != 16) {
        FURI_LOG_E(TAG, "Invalid block data length: %u", response.data_len);
        return false;
    }

    memcpy(data, response.data, 16);

    FURI_LOG_I(TAG, "Block %u read successfully", block);
    return true;
}

bool chameleon_app_em410x_scan(ChameleonApp* app, uint8_t* id) {
    furi_assert(app);
    furi_assert(id);

    FURI_LOG_I(TAG, "Scanning for EM410X tags");
    CHAM_LOG_I(app->logger, "TagRead", "Starting EM410X scan");

    // Build EM410X_SCAN command
    uint8_t cmd_buffer[CHAMELEON_FRAME_OVERHEAD];
    size_t cmd_len;

    if(!chameleon_protocol_build_cmd_no_data(app->protocol, CMD_EM410X_SCAN, cmd_buffer, &cmd_len)) {
        FURI_LOG_E(TAG, "Failed to build EM410X_SCAN command");
        CHAM_LOG_E(app->logger, "TagRead", "Failed to build EM410X_SCAN command");
        return false;
    }

    // Send command
    if(app->connection_type == ChameleonConnectionUSB) {
        uart_handler_send(app->uart_handler, cmd_buffer, cmd_len);
    } else if(app->connection_type == ChameleonConnectionBLE) {
        ble_handler_send(app->ble_handler, cmd_buffer, cmd_len);
    } else {
        FURI_LOG_E(TAG, "Not connected");
        return false;
    }

    // Wait for response
    ChameleonResponse response;
    if(!chameleon_response_handler_wait_for_response(
           app->response_handler, CMD_EM410X_SCAN, &response, RESPONSE_TIMEOUT_MS)) {
        FURI_LOG_E(TAG, "Timeout waiting for EM410X scan");
        return false;
    }

    // Check response status
    if(response.status != STATUS_LF_TAG_OK) {
        FURI_LOG_E(TAG, "EM410X scan failed: status=%04X", response.status);
        CHAM_LOG_E(app->logger, "TagRead", "EM410X scan failed with status %04X", response.status);
        return false;
    }

    // Response should be 5 bytes (EM410X ID)
    if(response.data_len != 5) {
        FURI_LOG_E(TAG, "Invalid EM410X response length: %u", response.data_len);
        CHAM_LOG_E(app->logger, "TagRead", "Invalid EM410X response length: %u", response.data_len);
        return false;
    }

    memcpy(id, response.data, 5);

    FURI_LOG_I(TAG, "EM410X tag found");
    CHAM_LOG_I(app->logger, "TagRead", "EM410X tag found successfully");
    return true;
}

bool chameleon_app_save_tag_to_file(ChameleonApp* app, const char* filename, const uint8_t* data, size_t data_len) {
    furi_assert(app);
    furi_assert(filename);
    furi_assert(data);

    FURI_LOG_I(TAG, "Saving tag to file: %s", filename);

    File* file = storage_file_alloc(app->storage);

    char full_path[256];
    snprintf(full_path, sizeof(full_path), "/ext/apps_data/chameleon_ultra/%s", filename);

    if(!storage_file_open(file, full_path, FSAM_WRITE, FSOM_CREATE_ALWAYS)) {
        FURI_LOG_E(TAG, "Failed to open file for writing");
        storage_file_free(file);
        return false;
    }

    size_t written = storage_file_write(file, data, data_len);
    storage_file_close(file);
    storage_file_free(file);

    if(written != data_len) {
        FURI_LOG_E(TAG, "Failed to write complete data: %zu/%zu", written, data_len);
        return false;
    }

    FURI_LOG_I(TAG, "Tag saved successfully: %zu bytes", written);
    return true;
}

// ============================================================================
// Tag Writing Functions
// ============================================================================

bool chameleon_app_mf1_write_emu_block(ChameleonApp* app, uint8_t block, const uint8_t* data) {
    furi_assert(app);
    furi_assert(data);

    FURI_LOG_I(TAG, "Writing Mifare Classic block %u to emulation", block);
    CHAM_LOG_I(app->logger, "TagWrite", "Writing Mifare block %u", block);

    // Build MF1_WRITE_EMU_BLOCK_DATA command
    // Data format: block (1 byte) + data (16 bytes)
    uint8_t cmd_data[17];
    cmd_data[0] = block;
    memcpy(&cmd_data[1], data, 16);

    uint8_t frame[128];
    size_t frame_len = chameleon_protocol_build_frame(
        app->protocol,
        0x1004, // CMD_MF1_WRITE_EMU_BLOCK_DATA
        0x0000, // STATUS: not used in command
        cmd_data,
        17,
        frame,
        sizeof(frame));

    // Send command
    bool sent = false;
    if(app->connection_type == ChameleonConnectionUSB) {
        sent = uart_handler_send(app->uart_handler, frame, frame_len);
    } else if(app->connection_type == ChameleonConnectionBLE) {
        sent = ble_handler_send(app->ble_handler, frame, frame_len);
    }

    if(!sent) {
        FURI_LOG_E(TAG, "Failed to send MF1_WRITE_EMU_BLOCK_DATA");
        CHAM_LOG_E(app->logger, "TagWrite", "Failed to send MF1 write command");
        return false;
    }

    // Wait for response
    ChameleonResponse response;
    if(!chameleon_response_handler_wait_for_response(
           app->response_handler, 0x1004, &response, RESPONSE_TIMEOUT_MS)) {
        FURI_LOG_E(TAG, "Timeout waiting for MF1_WRITE_EMU_BLOCK_DATA response");
        CHAM_LOG_E(app->logger, "TagWrite", "Timeout waiting for MF1 write response");
        return false;
    }

    // Check status (0x0000 = success)
    if(response.status != 0x0000) {
        FURI_LOG_E(TAG, "MF1_WRITE_EMU_BLOCK_DATA failed with status: %04X", response.status);
        CHAM_LOG_E(app->logger, "TagWrite", "MF1 write failed with status %04X", response.status);
        return false;
    }

    FURI_LOG_I(TAG, "Block %u written successfully", block);
    CHAM_LOG_I(app->logger, "TagWrite", "Mifare block %u written successfully", block);
    return true;
}

bool chameleon_app_em410x_set_emu_id(ChameleonApp* app, const uint8_t* id) {
    furi_assert(app);
    furi_assert(id);

    FURI_LOG_I(TAG, "Setting EM410X emulation ID");
    CHAM_LOG_I(app->logger, "TagWrite", "Setting EM410X emulation ID");

    // Build EM410X_SET_EMU_ID command
    // Data: 5 bytes (EM410X ID)
    uint8_t frame[128];
    size_t frame_len = chameleon_protocol_build_frame(
        app->protocol,
        0x3002, // CMD_EM410X_SET_EMU_ID
        0x0000, // STATUS: not used in command
        id,
        5,
        frame,
        sizeof(frame));

    // Send command
    bool sent = false;
    if(app->connection_type == ChameleonConnectionUSB) {
        sent = uart_handler_send(app->uart_handler, frame, frame_len);
    } else if(app->connection_type == ChameleonConnectionBLE) {
        sent = ble_handler_send(app->ble_handler, frame, frame_len);
    }

    if(!sent) {
        FURI_LOG_E(TAG, "Failed to send EM410X_SET_EMU_ID");
        CHAM_LOG_E(app->logger, "TagWrite", "Failed to send EM410X set command");
        return false;
    }

    // Wait for response
    ChameleonResponse response;
    if(!chameleon_response_handler_wait_for_response(
           app->response_handler, 0x3002, &response, RESPONSE_TIMEOUT_MS)) {
        FURI_LOG_E(TAG, "Timeout waiting for EM410X_SET_EMU_ID response");
        CHAM_LOG_E(app->logger, "TagWrite", "Timeout waiting for EM410X set response");
        return false;
    }

    // Check status (0x0000 = success)
    if(response.status != 0x0000) {
        FURI_LOG_E(TAG, "EM410X_SET_EMU_ID failed with status: %04X", response.status);
        CHAM_LOG_E(app->logger, "TagWrite", "EM410X set failed with status %04X", response.status);
        return false;
    }

    FURI_LOG_I(TAG, "EM410X ID set successfully");
    CHAM_LOG_I(app->logger, "TagWrite", "EM410X ID set successfully");
    return true;
}

bool chameleon_app_load_tag_from_file(
    ChameleonApp* app,
    const char* filepath,
    uint8_t* data,
    size_t* data_len,
    ChameleonTagType* tag_type) {
    furi_assert(app);
    furi_assert(filepath);
    furi_assert(data);
    furi_assert(data_len);
    furi_assert(tag_type);

    FURI_LOG_I(TAG, "Loading tag from file: %s", filepath);

    File* file = storage_file_alloc(app->storage);

    if(!storage_file_open(file, filepath, FSAM_READ, FSOM_OPEN_EXISTING)) {
        FURI_LOG_E(TAG, "Failed to open file for reading");
        storage_file_free(file);
        return false;
    }

    // Read file content
    size_t file_size = storage_file_size(file);
    if(file_size == 0 || file_size > 512) {
        FURI_LOG_E(TAG, "Invalid file size: %zu", file_size);
        storage_file_close(file);
        storage_file_free(file);
        return false;
    }

    size_t read = storage_file_read(file, data, file_size);
    storage_file_close(file);
    storage_file_free(file);

    if(read != file_size) {
        FURI_LOG_E(TAG, "Failed to read complete file: %zu/%zu", read, file_size);
        return false;
    }

    *data_len = read;

    // Try to detect tag type from file extension or content
    *tag_type = TagTypeUnknown;

    if(strstr(filepath, ".nfc") != NULL) {
        // Flipper .nfc file - parse it
        // Simple heuristic: if size is small (5 bytes), likely EM410X
        // If larger (64+ bytes), likely Mifare Classic
        if(read == 5) {
            *tag_type = TagTypeEM410X;
        } else if(read >= 64) {
            // Could be Mifare Classic or Ultralight
            // For now, default to Classic 1K
            *tag_type = TagTypeMifareClassic1K;
        }
    } else if(strstr(filepath, ".rfid") != NULL) {
        // RFID file (usually LF tags)
        *tag_type = TagTypeEM410X;
    }

    FURI_LOG_I(TAG, "Tag loaded: %zu bytes, type=%u", *data_len, *tag_type);
    return true;
}

int32_t chameleon_ultra_app(void* p) {
    UNUSED(p);

    FURI_LOG_I(TAG, "Chameleon Ultra app starting");

    ChameleonApp* app = chameleon_app_alloc();

    // Start with main menu scene
    scene_manager_next_scene(app->scene_manager, ChameleonSceneStart);

    // Run view dispatcher
    view_dispatcher_run(app->view_dispatcher);

    // Cleanup
    chameleon_app_free(app);

    FURI_LOG_I(TAG, "Chameleon Ultra app finished");

    return 0;
}
