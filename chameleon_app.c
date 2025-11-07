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

    // Free protocol
    chameleon_protocol_free(app->protocol);

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

bool chameleon_app_connect_usb(ChameleonApp* app) {
    furi_assert(app);

    FURI_LOG_I(TAG, "Connecting via USB");

    if(!uart_handler_init(app->uart_handler)) {
        FURI_LOG_E(TAG, "Failed to initialize UART");
        return false;
    }

    uart_handler_start_rx(app->uart_handler);

    app->connection_type = ChameleonConnectionUSB;
    app->connection_status = ChameleonStatusConnected;

    FURI_LOG_I(TAG, "Connected via USB");
    return true;
}

bool chameleon_app_connect_ble(ChameleonApp* app) {
    furi_assert(app);

    FURI_LOG_I(TAG, "Connecting via BLE");

    if(!ble_handler_init(app->ble_handler)) {
        FURI_LOG_E(TAG, "Failed to initialize BLE");
        return false;
    }

    if(!ble_handler_start_scan(app->ble_handler)) {
        FURI_LOG_E(TAG, "Failed to start BLE scan");
        return false;
    }

    FURI_LOG_I(TAG, "BLE scan started");
    return true;
}

void chameleon_app_disconnect(ChameleonApp* app) {
    furi_assert(app);

    FURI_LOG_I(TAG, "Disconnecting");

    if(app->connection_type == ChameleonConnectionUSB) {
        uart_handler_deinit(app->uart_handler);
    } else if(app->connection_type == ChameleonConnectionBLE) {
        ble_handler_disconnect(app->ble_handler);
        ble_handler_deinit(app->ble_handler);
    }

    app->connection_type = ChameleonConnectionNone;
    app->connection_status = ChameleonStatusDisconnected;

    FURI_LOG_I(TAG, "Disconnected");
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

    // TODO: Wait for response and parse it
    // For now, set dummy data
    app->device_info.major_version = 1;
    app->device_info.minor_version = 0;
    app->device_info.chip_id = 0x123456789ABCDEF0;
    app->device_info.model = ChameleonModelUltra;
    app->device_info.mode = ChameleonModeEmulator;
    app->device_info.connected = true;

    FURI_LOG_I(TAG, "Device info retrieved");
    return true;
}

bool chameleon_app_get_slots_info(ChameleonApp* app) {
    furi_assert(app);

    FURI_LOG_I(TAG, "Getting slots info");

    // TODO: Implement GET_SLOT_INFO command
    // For now, set dummy data
    for(uint8_t i = 0; i < 8; i++) {
        snprintf(app->slots[i].nickname, sizeof(app->slots[i].nickname), "Slot %d", i);
    }

    FURI_LOG_I(TAG, "Slots info retrieved");
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
