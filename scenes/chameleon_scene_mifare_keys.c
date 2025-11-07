#include "../chameleon_app_i.h"
#include "../lib/mifare_keys/mifare_keys_db.h"
#include "../lib/sound_effects/sound_effects.h"
#include <gui/modules/submenu.h>

typedef enum {
    SubmenuIndexBrowseDatabase,
    SubmenuIndexAddAllToManager,
    SubmenuIndexExportToFile,
    SubmenuIndexImportFromFile,
    SubmenuIndexTestKeys,
} SubmenuIndex;

static void chameleon_scene_mifare_keys_submenu_callback(void* context, uint32_t index) {
    ChameleonApp* app = context;
    view_dispatcher_send_custom_event(app->view_dispatcher, index);
}

void chameleon_scene_mifare_keys_on_enter(void* context) {
    ChameleonApp* app = context;
    Submenu* submenu = app->submenu;

    submenu_reset(submenu);
    submenu_set_header(submenu, "Mifare Key Database");

    submenu_add_item(
        submenu,
        "Browse Database",
        SubmenuIndexBrowseDatabase,
        chameleon_scene_mifare_keys_submenu_callback,
        app);

    submenu_add_item(
        submenu,
        "Add All to Manager",
        SubmenuIndexAddAllToManager,
        chameleon_scene_mifare_keys_submenu_callback,
        app);

    submenu_add_item(
        submenu,
        "Export to File",
        SubmenuIndexExportToFile,
        chameleon_scene_mifare_keys_submenu_callback,
        app);

    submenu_add_item(
        submenu,
        "Import from File",
        SubmenuIndexImportFromFile,
        chameleon_scene_mifare_keys_submenu_callback,
        app);

    submenu_add_item(
        submenu,
        "Test Keys on Tag",
        SubmenuIndexTestKeys,
        chameleon_scene_mifare_keys_submenu_callback,
        app);

    view_dispatcher_switch_to_view(app->view_dispatcher, ChameleonViewSubmenu);
}

// Callback for importing keys
static void import_key_callback(const char* name, const uint8_t key[6], void* context) {
    ChameleonApp* app = context;
    if(app && app->key_manager) {
        key_manager_add_key(app->key_manager, key, name);
    }
}

bool chameleon_scene_mifare_keys_on_event(void* context, SceneManagerEvent event) {
    ChameleonApp* app = context;
    bool consumed = false;

    if(event.type == SceneManagerEventTypeCustom) {
        switch(event.event) {
        case SubmenuIndexBrowseDatabase: {
            // Navigate to database browser scene
            scene_manager_next_scene(app->scene_manager, ChameleonSceneMifareKeysBrowse);
            consumed = true;
            break;
        }

        case SubmenuIndexAddAllToManager: {
            // Add all keys from database to key manager
            if(app->key_manager) {
                size_t count = mifare_keys_db_get_count();
                size_t added = 0;

                for(size_t i = 0; i < count; i++) {
                    const MifareKeyEntry* entry = mifare_keys_db_get_key(i);
                    if(entry) {
                        if(key_manager_add_key(app->key_manager, entry->key, entry->name)) {
                            added++;
                        }
                    }
                }

                CHAM_LOG_I(
                    app->logger,
                    "MifareKeys",
                    "Added %zu/%zu keys to manager",
                    added,
                    count);

                // Show notification
                FuriString* msg = furi_string_alloc();
                furi_string_printf(msg, "Added %zu keys\nto manager", added);

                DialogMessage* message = dialog_message_alloc();
                dialog_message_set_header(message, "Success", 64, 10, AlignCenter, AlignTop);
                dialog_message_set_text(
                    message, furi_string_get_cstr(msg), 64, 32, AlignCenter, AlignCenter);
                dialog_message_set_buttons(message, NULL, "OK", NULL);
                dialog_message_show(app->dialogs, message);
                dialog_message_free(message);
                furi_string_free(msg);

                sound_effects_success();
            }
            consumed = true;
            break;
        }

        case SubmenuIndexExportToFile: {
            // Export database to file
            const char* filepath = "/ext/apps_data/chameleon_ultra/mifare_keys_db.txt";
            if(mifare_keys_db_export_to_file(filepath)) {
                CHAM_LOG_I(app->logger, "MifareKeys", "Exported to %s", filepath);

                DialogMessage* message = dialog_message_alloc();
                dialog_message_set_header(message, "Exported!", 64, 10, AlignCenter, AlignTop);
                dialog_message_set_text(
                    message,
                    "Keys exported to:\nmifare_keys_db.txt",
                    64,
                    32,
                    AlignCenter,
                    AlignCenter);
                dialog_message_set_buttons(message, NULL, "OK", NULL);
                dialog_message_show(app->dialogs, message);
                dialog_message_free(message);

                sound_effects_success();
            } else {
                CHAM_LOG_E(app->logger, "MifareKeys", "Export failed");
                sound_effects_error();
            }
            consumed = true;
            break;
        }

        case SubmenuIndexImportFromFile: {
            // Import keys from file
            const char* filepath = "/ext/apps_data/chameleon_ultra/custom_keys.txt";
            size_t imported =
                mifare_keys_db_import_from_file(filepath, import_key_callback, app);

            CHAM_LOG_I(app->logger, "MifareKeys", "Imported %zu keys", imported);

            FuriString* msg = furi_string_alloc();
            if(imported > 0) {
                furi_string_printf(msg, "Imported %zu keys\nfrom custom_keys.txt", imported);
                sound_effects_success();
            } else {
                furi_string_printf(
                    msg,
                    "No keys imported\nPlace custom_keys.txt\nin apps_data/chameleon_ultra/");
                sound_effects_warning();
            }

            DialogMessage* message = dialog_message_alloc();
            dialog_message_set_header(
                message,
                imported > 0 ? "Success" : "Info",
                64,
                10,
                AlignCenter,
                AlignTop);
            dialog_message_set_text(
                message, furi_string_get_cstr(msg), 64, 32, AlignCenter, AlignCenter);
            dialog_message_set_buttons(message, NULL, "OK", NULL);
            dialog_message_show(app->dialogs, message);
            dialog_message_free(message);
            furi_string_free(msg);

            consumed = true;
            break;
        }

        case SubmenuIndexTestKeys: {
            // Test all keys against a tag (future implementation)
            DialogMessage* message = dialog_message_alloc();
            dialog_message_set_header(message, "Coming Soon", 64, 10, AlignCenter, AlignTop);
            dialog_message_set_text(
                message,
                "Key testing will be\navailable in next update",
                64,
                32,
                AlignCenter,
                AlignCenter);
            dialog_message_set_buttons(message, NULL, "OK", NULL);
            dialog_message_show(app->dialogs, message);
            dialog_message_free(message);
            consumed = true;
            break;
        }
        }
    }

    return consumed;
}

void chameleon_scene_mifare_keys_on_exit(void* context) {
    ChameleonApp* app = context;
    submenu_reset(app->submenu);
}
