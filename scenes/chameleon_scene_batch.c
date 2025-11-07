#include "../chameleon_app_i.h"

typedef enum {
    BatchSubmenuIndexBackupAll,
    BatchSubmenuIndexRestoreAll,
    BatchSubmenuIndexClearAll,
} BatchSubmenuIndex;

static void chameleon_scene_batch_submenu_callback(void* context, uint32_t index) {
    ChameleonApp* app = context;
    view_dispatcher_send_custom_event(app->view_dispatcher, index);
}

void chameleon_scene_batch_on_enter(void* context) {
    ChameleonApp* app = context;

    submenu_reset(app->submenu);
    submenu_set_header(app->submenu, "Batch Operations");

    submenu_add_item(
        app->submenu,
        "Backup All Slots",
        BatchSubmenuIndexBackupAll,
        chameleon_scene_batch_submenu_callback,
        app);

    submenu_add_item(
        app->submenu,
        "Restore All Slots",
        BatchSubmenuIndexRestoreAll,
        chameleon_scene_batch_submenu_callback,
        app);

    submenu_add_item(
        app->submenu,
        "Clear All Slots",
        BatchSubmenuIndexClearAll,
        chameleon_scene_batch_submenu_callback,
        app);

    view_dispatcher_switch_to_view(app->view_dispatcher, ChameleonViewSubmenu);
}

bool chameleon_scene_batch_on_event(void* context, SceneManagerEvent event) {
    ChameleonApp* app = context;
    bool consumed = false;

    if(event.type == SceneManagerEventTypeCustom) {
        switch(event.event) {
        case BatchSubmenuIndexBackupAll: {
            if(app->connection_status != ChameleonStatusConnected) {
                popup_reset(app->popup);
                popup_set_header(app->popup, "Error", 64, 10, AlignCenter, AlignTop);
                popup_set_text(
                    app->popup,
                    "Not connected\nto device",
                    64,
                    32,
                    AlignCenter,
                    AlignCenter);
                view_dispatcher_switch_to_view(app->view_dispatcher, ChameleonViewPopup);
                furi_delay_ms(2000);
                view_dispatcher_switch_to_view(app->view_dispatcher, ChameleonViewSubmenu);
                consumed = true;
                break;
            }

            // Show progress
            popup_reset(app->popup);
            popup_set_header(app->popup, "Backing Up...", 64, 20, AlignCenter, AlignCenter);
            view_dispatcher_switch_to_view(app->view_dispatcher, ChameleonViewPopup);

            sound_effects_scan();

            // Get all slots info
            chameleon_app_get_slots_info(app);

            // Create backup filename
            char filepath[128];
            snprintf(
                filepath,
                sizeof(filepath),
                "/ext/apps_data/chameleon_ultra/backup_%lu.txt",
                furi_get_tick());

            // Save backup
            Storage* storage = furi_record_open(RECORD_STORAGE);
            File* file = storage_file_alloc(storage);

            bool success = false;
            if(storage_file_open(file, filepath, FSAM_WRITE, FSOM_CREATE_ALWAYS)) {
                // Write header
                const char* header = "# Chameleon Ultra Backup\n"
                                     "# All 8 Slots\n\n";
                storage_file_write(file, header, strlen(header));

                // Write each slot
                for(int i = 0; i < 8; i++) {
                    ChameleonSlot* slot = &app->slots[i];
                    char line[256];
                    int len = snprintf(
                        line,
                        sizeof(line),
                        "Slot %d: %s | HF:%d LF:%d | HF_Type:%d LF_Type:%d\n",
                        slot->slot_number,
                        slot->nickname,
                        slot->hf_enabled,
                        slot->lf_enabled,
                        slot->hf_tag_type,
                        slot->lf_tag_type);
                    storage_file_write(file, line, len);
                }

                storage_file_close(file);
                success = true;
            }

            storage_file_free(file);
            furi_record_close(RECORD_STORAGE);

            // Show result
            popup_reset(app->popup);
            if(success) {
                sound_effects_complete();
                popup_set_header(app->popup, "Success!", 64, 10, AlignCenter, AlignTop);

                const char* short_path = strrchr(filepath, '/');
                if(!short_path) short_path = filepath;
                else short_path++;

                snprintf(
                    app->text_buffer,
                    sizeof(app->text_buffer),
                    "8 slots backed up\n%s",
                    short_path);
                popup_set_text(
                    app->popup,
                    app->text_buffer,
                    64,
                    32,
                    AlignCenter,
                    AlignCenter);

                CHAM_LOG_I(app->logger, "Batch", "All slots backed up to %s", filepath);
            } else {
                sound_effects_error();
                popup_set_header(app->popup, "Error", 64, 10, AlignCenter, AlignTop);
                popup_set_text(
                    app->popup,
                    "Backup failed",
                    64,
                    32,
                    AlignCenter,
                    AlignCenter);

                CHAM_LOG_E(app->logger, "Batch", "Backup failed");
            }

            view_dispatcher_switch_to_view(app->view_dispatcher, ChameleonViewPopup);
            furi_delay_ms(3000);
            view_dispatcher_switch_to_view(app->view_dispatcher, ChameleonViewSubmenu);

            consumed = true;
            break;
        }

        case BatchSubmenuIndexRestoreAll: {
            // Show info about restore
            popup_reset(app->popup);
            popup_set_header(app->popup, "Coming Soon", 64, 10, AlignCenter, AlignTop);
            popup_set_text(
                app->popup,
                "Restore feature\ncoming in\nnext update",
                64,
                32,
                AlignCenter,
                AlignCenter);
            view_dispatcher_switch_to_view(app->view_dispatcher, ChameleonViewPopup);
            furi_delay_ms(2000);
            view_dispatcher_switch_to_view(app->view_dispatcher, ChameleonViewSubmenu);

            consumed = true;
            break;
        }

        case BatchSubmenuIndexClearAll: {
            if(app->connection_status != ChameleonStatusConnected) {
                popup_reset(app->popup);
                popup_set_header(app->popup, "Error", 64, 10, AlignCenter, AlignTop);
                popup_set_text(
                    app->popup,
                    "Not connected\nto device",
                    64,
                    32,
                    AlignCenter,
                    AlignCenter);
                view_dispatcher_switch_to_view(app->view_dispatcher, ChameleonViewPopup);
                furi_delay_ms(2000);
                view_dispatcher_switch_to_view(app->view_dispatcher, ChameleonViewSubmenu);
                consumed = true;
                break;
            }

            // Show warning
            popup_reset(app->popup);
            popup_set_header(app->popup, "Warning", 64, 10, AlignCenter, AlignTop);
            popup_set_text(
                app->popup,
                "This will clear\nALL 8 slots!\nProceed?",
                64,
                32,
                AlignCenter,
                AlignCenter);
            view_dispatcher_switch_to_view(app->view_dispatcher, ChameleonViewPopup);
            sound_effects_warning();
            furi_delay_ms(3000);

            // Coming soon - would need to implement clear command
            popup_reset(app->popup);
            popup_set_header(app->popup, "Coming Soon", 64, 10, AlignCenter, AlignTop);
            popup_set_text(
                app->popup,
                "Clear all feature\ncoming in\nnext update",
                64,
                32,
                AlignCenter,
                AlignCenter);
            view_dispatcher_switch_to_view(app->view_dispatcher, ChameleonViewPopup);
            furi_delay_ms(2000);
            view_dispatcher_switch_to_view(app->view_dispatcher, ChameleonViewSubmenu);

            consumed = true;
            break;
        }
        }
    }

    return consumed;
}

void chameleon_scene_batch_on_exit(void* context) {
    ChameleonApp* app = context;
    submenu_reset(app->submenu);
    popup_reset(app->popup);
    memset(app->text_buffer, 0, sizeof(app->text_buffer));
}
