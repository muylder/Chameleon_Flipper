#include "../chameleon_app_i.h"

typedef enum {
    LogsSubmenuIndexViewLogs,
    LogsSubmenuIndexExportLogs,
    LogsSubmenuIndexClearLogs,
} LogsSubmenuIndex;

static void chameleon_scene_logs_submenu_callback(void* context, uint32_t index) {
    ChameleonApp* app = context;
    view_dispatcher_send_custom_event(app->view_dispatcher, index);
}

void chameleon_scene_logs_on_enter(void* context) {
    ChameleonApp* app = context;

    submenu_reset(app->submenu);
    submenu_set_header(app->submenu, "System Logs");

    submenu_add_item(
        app->submenu,
        "View Logs",
        LogsSubmenuIndexViewLogs,
        chameleon_scene_logs_submenu_callback,
        app);

    submenu_add_item(
        app->submenu,
        "Export to File",
        LogsSubmenuIndexExportLogs,
        chameleon_scene_logs_submenu_callback,
        app);

    submenu_add_item(
        app->submenu,
        "Clear Logs",
        LogsSubmenuIndexClearLogs,
        chameleon_scene_logs_submenu_callback,
        app);

    view_dispatcher_switch_to_view(app->view_dispatcher, ChameleonViewSubmenu);
}

bool chameleon_scene_logs_on_event(void* context, SceneManagerEvent event) {
    ChameleonApp* app = context;
    bool consumed = false;

    if(event.type == SceneManagerEventTypeCustom) {
        switch(event.event) {
        case LogsSubmenuIndexViewLogs: {
            // Get latest logs
            ChameleonLogEntry entries[20]; // Show last 20 logs
            uint32_t count = 0;
            chameleon_logger_get_latest(app->logger, entries, 20, &count);

            if(count == 0) {
                // No logs
                popup_reset(app->popup);
                popup_set_header(app->popup, "No Logs", 64, 10, AlignCenter, AlignTop);
                popup_set_text(
                    app->popup,
                    "No log entries\nyet",
                    64,
                    32,
                    AlignCenter,
                    AlignCenter);
                view_dispatcher_switch_to_view(app->view_dispatcher, ChameleonViewPopup);
                furi_delay_ms(2000);
                view_dispatcher_switch_to_view(app->view_dispatcher, ChameleonViewSubmenu);
            } else {
                // Build log text
                FuriString* log_text = furi_string_alloc();

                for(uint32_t i = 0; i < count; i++) {
                    ChameleonLogEntry* entry = &entries[i];

                    // Format: [LEVEL] [TAG] Message
                    furi_string_cat_printf(
                        log_text,
                        "[%s] [%s]\n%s\n\n",
                        chameleon_logger_level_to_string(entry->level),
                        entry->tag,
                        entry->message);
                }

                // Show in widget
                widget_reset(app->widget);
                widget_add_text_scroll_element(
                    app->widget, 0, 0, 128, 64, furi_string_get_cstr(log_text));
                view_dispatcher_switch_to_view(app->view_dispatcher, ChameleonViewWidget);

                furi_string_free(log_text);

                // Wait for user to exit (back button)
                // Widget will auto-exit on back
            }
            consumed = true;
            break;
        }

        case LogsSubmenuIndexExportLogs: {
            // Export logs to file
            char filepath[128];
            snprintf(
                filepath,
                sizeof(filepath),
                "/ext/apps_data/chameleon_ultra/logs_%lu.txt",
                furi_get_tick());

            // Show loading
            popup_reset(app->popup);
            popup_set_header(app->popup, "Exporting...", 64, 20, AlignCenter, AlignCenter);
            view_dispatcher_switch_to_view(app->view_dispatcher, ChameleonViewPopup);

            bool success = chameleon_logger_export_to_file(app->logger, filepath);

            popup_reset(app->popup);
            if(success) {
                popup_set_header(app->popup, "Success!", 64, 10, AlignCenter, AlignTop);
                FuriString* msg = furi_string_alloc();
                furi_string_printf(msg, "Exported to:\n%s", filepath);
                popup_set_text(
                    app->popup,
                    furi_string_get_cstr(msg),
                    64,
                    32,
                    AlignCenter,
                    AlignCenter);
                furi_string_free(msg);

                CHAM_LOG_I(app->logger, "Logs", "Logs exported to %s", filepath);
            } else {
                popup_set_header(app->popup, "Error", 64, 10, AlignCenter, AlignTop);
                popup_set_text(
                    app->popup,
                    "Failed to export\nlogs",
                    64,
                    32,
                    AlignCenter,
                    AlignCenter);

                CHAM_LOG_E(app->logger, "Logs", "Failed to export logs");
            }

            view_dispatcher_switch_to_view(app->view_dispatcher, ChameleonViewPopup);
            furi_delay_ms(3000);
            view_dispatcher_switch_to_view(app->view_dispatcher, ChameleonViewSubmenu);

            consumed = true;
            break;
        }

        case LogsSubmenuIndexClearLogs: {
            // Clear logs
            chameleon_logger_clear(app->logger);

            CHAM_LOG_I(app->logger, "Logs", "Logs cleared by user");

            popup_reset(app->popup);
            popup_set_header(app->popup, "Cleared", 64, 20, AlignCenter, AlignCenter);
            popup_set_text(app->popup, "All logs cleared", 64, 32, AlignCenter, AlignCenter);
            view_dispatcher_switch_to_view(app->view_dispatcher, ChameleonViewPopup);
            furi_delay_ms(1500);
            view_dispatcher_switch_to_view(app->view_dispatcher, ChameleonViewSubmenu);

            consumed = true;
            break;
        }
        }
    } else if(event.type == SceneManagerEventTypeBack) {
        // Widget back button pressed - return to submenu
        view_dispatcher_switch_to_view(app->view_dispatcher, ChameleonViewSubmenu);
        consumed = true;
    }

    return consumed;
}

void chameleon_scene_logs_on_exit(void* context) {
    ChameleonApp* app = context;
    submenu_reset(app->submenu);
    popup_reset(app->popup);
    widget_reset(app->widget);
}
