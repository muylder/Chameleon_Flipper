#include "../chameleon_app_i.h"

typedef enum {
    KeysSubmenuIndexView,
    KeysSubmenuIndexTest,
    KeysSubmenuIndexExport,
    KeysSubmenuIndexClearAll,
} KeysSubmenuIndex;

static void chameleon_scene_keys_submenu_callback(void* context, uint32_t index) {
    ChameleonApp* app = context;
    view_dispatcher_send_custom_event(app->view_dispatcher, index);
}

void chameleon_scene_keys_on_enter(void* context) {
    ChameleonApp* app = context;

    submenu_reset(app->submenu);
    submenu_set_header(app->submenu, "Mifare Keys");

    size_t key_count = key_manager_get_count(app->key_manager);
    snprintf(app->text_buffer, sizeof(app->text_buffer), "View Keys (%zu)", key_count);

    submenu_add_item(
        app->submenu,
        app->text_buffer,
        KeysSubmenuIndexView,
        chameleon_scene_keys_submenu_callback,
        app);

    submenu_add_item(
        app->submenu,
        "Test Keys (Auto)",
        KeysSubmenuIndexTest,
        chameleon_scene_keys_submenu_callback,
        app);

    submenu_add_item(
        app->submenu,
        "Export to File",
        KeysSubmenuIndexExport,
        chameleon_scene_keys_submenu_callback,
        app);

    submenu_add_item(
        app->submenu,
        "Clear All Keys",
        KeysSubmenuIndexClearAll,
        chameleon_scene_keys_submenu_callback,
        app);

    view_dispatcher_switch_to_view(app->view_dispatcher, ChameleonViewSubmenu);
}

bool chameleon_scene_keys_on_event(void* context, SceneManagerEvent event) {
    ChameleonApp* app = context;
    bool consumed = false;

    if(event.type == SceneManagerEventTypeCustom) {
        switch(event.event) {
        case KeysSubmenuIndexView: {
            // Show all keys in widget
            size_t count = key_manager_get_count(app->key_manager);

            if(count == 0) {
                popup_reset(app->popup);
                popup_set_header(app->popup, "No Keys", 64, 10, AlignCenter, AlignTop);
                popup_set_text(
                    app->popup,
                    "No keys in\ndatabase",
                    64,
                    32,
                    AlignCenter,
                    AlignCenter);
                view_dispatcher_switch_to_view(app->view_dispatcher, ChameleonViewPopup);
                furi_delay_ms(2000);
                view_dispatcher_switch_to_view(app->view_dispatcher, ChameleonViewSubmenu);
            } else {
                // Build key list
                FuriString* key_text = furi_string_alloc();
                furi_string_cat_printf(key_text, "Mifare Keys (%zu)\n\n", count);

                for(size_t i = 0; i < count && i < 20; i++) {  // Show max 20
                    KeyEntry entry;
                    if(key_manager_get_key(app->key_manager, i, &entry)) {
                        char key_str[16];
                        key_manager_format_key(entry.key, key_str, sizeof(key_str));

                        furi_string_cat_printf(
                            key_text,
                            "%zu. %s\n   %s (%c)\n",
                            i + 1,
                            entry.name,
                            key_str,
                            entry.type == KeyTypeA ? 'A' : 'B');
                    }
                }

                if(count > 20) {
                    furi_string_cat_printf(key_text, "\n...and %zu more", count - 20);
                }

                widget_reset(app->widget);
                widget_add_text_scroll_element(
                    app->widget, 0, 0, 128, 64, furi_string_get_cstr(key_text));
                view_dispatcher_switch_to_view(app->view_dispatcher, ChameleonViewWidget);

                furi_string_free(key_text);
            }
            consumed = true;
            break;
        }

        case KeysSubmenuIndexTest: {
            // Show info about key testing
            popup_reset(app->popup);
            popup_set_header(app->popup, "Key Testing", 64, 10, AlignCenter, AlignTop);
            popup_set_text(
                app->popup,
                "Use during\nTag Read to\ntest keys\nautomatically",
                64,
                32,
                AlignCenter,
                AlignCenter);
            view_dispatcher_switch_to_view(app->view_dispatcher, ChameleonViewPopup);
            furi_delay_ms(3000);
            view_dispatcher_switch_to_view(app->view_dispatcher, ChameleonViewSubmenu);

            consumed = true;
            break;
        }

        case KeysSubmenuIndexExport: {
            // Export keys to file
            char filepath[128];
            snprintf(
                filepath,
                sizeof(filepath),
                "/ext/apps_data/chameleon_ultra/keys_%lu.txt",
                furi_get_tick());

            popup_reset(app->popup);
            popup_set_header(app->popup, "Exporting...", 64, 20, AlignCenter, AlignCenter);
            view_dispatcher_switch_to_view(app->view_dispatcher, ChameleonViewPopup);

            bool success = key_manager_export_to_file(app->key_manager, filepath);

            popup_reset(app->popup);
            if(success) {
                popup_set_header(app->popup, "Success!", 64, 10, AlignCenter, AlignTop);

                // Show shorter path
                const char* short_path = strrchr(filepath, '/');
                if(!short_path) short_path = filepath;
                else short_path++; // Skip the '/'

                snprintf(app->text_buffer, sizeof(app->text_buffer), "Exported to:\n%s", short_path);
                popup_set_text(
                    app->popup,
                    app->text_buffer,
                    64,
                    32,
                    AlignCenter,
                    AlignCenter);

                CHAM_LOG_I(app->logger, "Keys", "Keys exported to %s", filepath);
            } else {
                popup_set_header(app->popup, "Error", 64, 10, AlignCenter, AlignTop);
                popup_set_text(
                    app->popup,
                    "Failed to\nexport keys",
                    64,
                    32,
                    AlignCenter,
                    AlignCenter);

                CHAM_LOG_E(app->logger, "Keys", "Failed to export keys");
            }

            view_dispatcher_switch_to_view(app->view_dispatcher, ChameleonViewPopup);
            furi_delay_ms(3000);
            view_dispatcher_switch_to_view(app->view_dispatcher, ChameleonViewSubmenu);

            consumed = true;
            break;
        }

        case KeysSubmenuIndexClearAll: {
            // Clear all keys with confirmation
            size_t count = key_manager_get_count(app->key_manager);

            if(count == 0) {
                popup_reset(app->popup);
                popup_set_header(app->popup, "Empty", 64, 20, AlignCenter, AlignCenter);
                popup_set_text(
                    app->popup,
                    "No keys to clear",
                    64,
                    32,
                    AlignCenter,
                    AlignCenter);
                view_dispatcher_switch_to_view(app->view_dispatcher, ChameleonViewPopup);
                furi_delay_ms(1500);
                view_dispatcher_switch_to_view(app->view_dispatcher, ChameleonViewSubmenu);
            } else {
                // Show warning
                popup_reset(app->popup);
                popup_set_header(app->popup, "Clearing Keys", 64, 10, AlignCenter, AlignTop);
                snprintf(
                    app->text_buffer,
                    sizeof(app->text_buffer),
                    "Removing %zu keys\nReload defaults?",
                    count);
                popup_set_text(
                    app->popup,
                    app->text_buffer,
                    64,
                    32,
                    AlignCenter,
                    AlignCenter);
                view_dispatcher_switch_to_view(app->view_dispatcher, ChameleonViewPopup);

                furi_delay_ms(2000);

                // Clear and reload defaults
                key_manager_clear_all(app->key_manager);
                key_manager_load_defaults(app->key_manager);

                CHAM_LOG_I(
                    app->logger,
                    "Keys",
                    "Keys cleared and defaults reloaded (%zu keys)",
                    key_manager_get_count(app->key_manager));

                popup_reset(app->popup);
                popup_set_header(app->popup, "Done!", 64, 10, AlignCenter, AlignTop);
                snprintf(
                    app->text_buffer,
                    sizeof(app->text_buffer),
                    "Defaults restored\n%zu keys loaded",
                    key_manager_get_count(app->key_manager));
                popup_set_text(
                    app->popup,
                    app->text_buffer,
                    64,
                    32,
                    AlignCenter,
                    AlignCenter);
                view_dispatcher_switch_to_view(app->view_dispatcher, ChameleonViewPopup);

                furi_delay_ms(2000);
                view_dispatcher_switch_to_view(app->view_dispatcher, ChameleonViewSubmenu);
            }

            consumed = true;
            break;
        }
        }
    } else if(event.type == SceneManagerEventTypeBack) {
        // Widget back button - return to submenu
        view_dispatcher_switch_to_view(app->view_dispatcher, ChameleonViewSubmenu);
        consumed = true;
    }

    return consumed;
}

void chameleon_scene_keys_on_exit(void* context) {
    ChameleonApp* app = context;
    submenu_reset(app->submenu);
    popup_reset(app->popup);
    widget_reset(app->widget);
    memset(app->text_buffer, 0, sizeof(app->text_buffer));
}
