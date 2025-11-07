#include "../chameleon_app_i.h"

void chameleon_scene_diagnostic_on_enter(void* context) {
    ChameleonApp* app = context;
    Widget* widget = app->widget;

    widget_reset(widget);

    // Get device info
    chameleon_app_get_device_info(app);
    chameleon_app_get_slots_info(app);

    // Build comprehensive diagnostic info
    FuriString* info = furi_string_alloc();

    // Header
    furi_string_cat_printf(info, "=== CHAMELEON ULTRA ===\n");
    furi_string_cat_printf(info, "Advanced Diagnostics\n\n");

    // Device Info
    furi_string_cat_printf(info, "[DEVICE]\n");
    furi_string_cat_printf(
        info,
        "Firmware: v%d.%d\n",
        app->device_info.major_version,
        app->device_info.minor_version);
    furi_string_cat_printf(
        info,
        "Model: %s\n",
        app->device_info.model == ChameleonModelUltra ? "Ultra" : "Lite");
    furi_string_cat_printf(
        info,
        "Mode: %s\n",
        app->device_info.mode == ChameleonModeReader ? "Reader" : "Emulator");
    furi_string_cat_printf(info, "Chip: %08llX\n\n", app->device_info.chip_id);

    // Connection Info
    furi_string_cat_printf(info, "[CONNECTION]\n");
    furi_string_cat_printf(
        info,
        "Type: %s\n",
        app->connection_type == ChameleonConnectionUSB ? "USB" :
        app->connection_type == ChameleonConnectionBLE ? "BLE" :
        "None");
    furi_string_cat_printf(
        info,
        "Status: %s\n",
        app->connection_status == ChameleonStatusConnected ? "Connected" :
        app->connection_status == ChameleonStatusConnecting ? "Connecting..." :
        app->connection_status == ChameleonStatusError ? "Error" :
        "Disconnected");
    furi_string_cat_printf(
        info,
        "Uptime: %lu sec\n\n",
        furi_get_tick() / furi_kernel_get_tick_frequency());

    // Active Slot Info
    furi_string_cat_printf(info, "[ACTIVE SLOT]\n");
    ChameleonSlot* slot = &app->slots[app->active_slot];
    furi_string_cat_printf(info, "Number: %d\n", slot->slot_number);
    furi_string_cat_printf(info, "Name: %s\n", slot->nickname);
    furi_string_cat_printf(
        info,
        "HF: %s ",
        slot->hf_enabled ? "ON" : "OFF");
    if(slot->hf_enabled) {
        furi_string_cat_printf(
            info,
            "(Type %d)",
            slot->hf_tag_type);
    }
    furi_string_cat_printf(info, "\n");
    furi_string_cat_printf(
        info,
        "LF: %s ",
        slot->lf_enabled ? "ON" : "OFF");
    if(slot->lf_enabled) {
        furi_string_cat_printf(
            info,
            "(Type %d)",
            slot->lf_tag_type);
    }
    furi_string_cat_printf(info, "\n\n");

    // System Stats
    furi_string_cat_printf(info, "[SYSTEM STATS]\n");
    furi_string_cat_printf(info, "Keys: %zu\n", key_manager_get_count(app->key_manager));
    furi_string_cat_printf(info, "Logs: %lu\n", chameleon_logger_get_count(app->logger));
    furi_string_cat_printf(info, "Free heap: %zu bytes\n", memmgr_get_free_heap());
    furi_string_cat_printf(info, "Uptime: %lu ticks\n", furi_get_tick());

    widget_add_text_scroll_element(widget, 0, 0, 128, 64, furi_string_get_cstr(info));

    furi_string_free(info);

    CHAM_LOG_I(app->logger, "Diagnostic", "Diagnostic info displayed");

    view_dispatcher_switch_to_view(app->view_dispatcher, ChameleonViewWidget);
}

bool chameleon_scene_diagnostic_on_event(void* context, SceneManagerEvent event) {
    UNUSED(context);
    UNUSED(event);
    return false;
}

void chameleon_scene_diagnostic_on_exit(void* context) {
    ChameleonApp* app = context;
    widget_reset(app->widget);
}
