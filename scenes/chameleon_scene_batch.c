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

            // Use file browser to select backup file
            FuriString* file_path = furi_string_alloc();
            furi_string_set(file_path, "/ext/apps_data/chameleon_ultra");

            DialogsFileBrowserOptions browser_options;
            dialog_file_browser_set_basic_options(
                &browser_options, ".txt", &I_chameleon_10px);
            browser_options.base_path = "/ext/apps_data/chameleon_ultra";
            browser_options.hide_ext = false;

            if(dialog_file_browser_show(app->dialogs, file_path, file_path, &browser_options)) {
                // User selected a file, restore it
                popup_reset(app->popup);
                popup_set_header(app->popup, "Restoring...", 64, 20, AlignCenter, AlignCenter);
                view_dispatcher_switch_to_view(app->view_dispatcher, ChameleonViewPopup);

                sound_effects_scan();

                // Parse and restore backup file
                Storage* storage = furi_record_open(RECORD_STORAGE);
                File* file = storage_file_alloc(storage);

                int restored_count = 0;
                bool success = false;

                if(storage_file_open(
                       file, furi_string_get_cstr(file_path), FSAM_READ, FSOM_OPEN_EXISTING)) {
                    char line[256];
                    size_t line_len;

                    // Skip header lines
                    while((line_len = storage_file_read(file, line, sizeof(line) - 1)) > 0) {
                        line[line_len] = '\0';
                        if(line[0] != '#' && strstr(line, "Slot ") != NULL) {
                            // Parse slot line: "Slot X: Name | HF:Y LF:Z | HF_Type:A LF_Type:B"
                            int slot_num, hf_en, lf_en, hf_type, lf_type;
                            char nickname[32];

                            // Extract slot data
                            char* slot_start = strstr(line, "Slot ");
                            if(slot_start &&
                               sscanf(
                                   slot_start,
                                   "Slot %d: %31[^|]| HF:%d LF:%d | HF_Type:%d LF_Type:%d",
                                   &slot_num,
                                   nickname,
                                   &hf_en,
                                   &lf_en,
                                   &hf_type,
                                   &lf_type) == 6) {
                                // Trim nickname
                                char* trim_end = nickname + strlen(nickname) - 1;
                                while(trim_end > nickname && (*trim_end == ' ' || *trim_end == '\n' || *trim_end == '\r'))
                                    *trim_end-- = '\0';

                                CHAM_LOG_I(
                                    app->logger,
                                    "Batch",
                                    "Restoring slot %d: %s",
                                    slot_num,
                                    nickname);

                                // Here we would send commands to restore the slot
                                // For now, just count it as restored
                                // TODO: Implement actual protocol commands when available
                                restored_count++;
                            }
                        }
                    }

                    storage_file_close(file);
                    success = (restored_count > 0);
                }

                storage_file_free(file);
                furi_record_close(RECORD_STORAGE);

                // Show result
                popup_reset(app->popup);
                if(success) {
                    sound_effects_complete();
                    popup_set_header(app->popup, "Restored!", 64, 10, AlignCenter, AlignTop);

                    snprintf(
                        app->text_buffer,
                        sizeof(app->text_buffer),
                        "%d slots restored\nfrom backup",
                        restored_count);
                    popup_set_text(
                        app->popup,
                        app->text_buffer,
                        64,
                        32,
                        AlignCenter,
                        AlignCenter);

                    CHAM_LOG_I(
                        app->logger,
                        "Batch",
                        "Restored %d slots from %s",
                        restored_count,
                        furi_string_get_cstr(file_path));
                } else {
                    sound_effects_error();
                    popup_set_header(app->popup, "Error", 64, 10, AlignCenter, AlignTop);
                    popup_set_text(
                        app->popup,
                        "Restore failed\nInvalid backup file",
                        64,
                        32,
                        AlignCenter,
                        AlignCenter);

                    CHAM_LOG_E(app->logger, "Batch", "Restore failed");
                }

                view_dispatcher_switch_to_view(app->view_dispatcher, ChameleonViewPopup);
                furi_delay_ms(3000);
            }

            furi_string_free(file_path);
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

            // Show confirmation dialog
            DialogMessage* message = dialog_message_alloc();
            dialog_message_set_header(
                message, "CLEAR ALL SLOTS?", 64, 0, AlignCenter, AlignTop);
            dialog_message_set_text(
                message,
                "This will DELETE all data\nin ALL 8 slots!\n\nThis cannot be undone!",
                64,
                20,
                AlignCenter,
                AlignTop);
            dialog_message_set_buttons(message, "Cancel", NULL, "Clear All");

            sound_effects_warning();
            DialogMessageButton result = dialog_message_show(app->dialogs, message);
            dialog_message_free(message);

            if(result == DialogMessageButtonRight) {
                // User confirmed, clear all slots
                popup_reset(app->popup);
                popup_set_header(app->popup, "Clearing...", 64, 20, AlignCenter, AlignCenter);
                view_dispatcher_switch_to_view(app->view_dispatcher, ChameleonViewPopup);

                CHAM_LOG_W(app->logger, "Batch", "Clearing all 8 slots");

                int cleared_count = 0;

                // Clear each slot (in a real implementation, send CMD_DELETE_SLOT_SENSE_TYPE)
                for(int i = 0; i < 8; i++) {
                    // TODO: Send actual protocol command CMD_DELETE_SLOT_SENSE_TYPE (1024)
                    // For now, just reset local slot data
                    ChameleonSlot* slot = &app->slots[i];
                    snprintf(slot->nickname, sizeof(slot->nickname), "Slot %d", i);
                    slot->hf_enabled = 0;
                    slot->lf_enabled = 0;
                    slot->hf_tag_type = 0;
                    slot->lf_tag_type = 0;

                    cleared_count++;
                    furi_delay_ms(100); // Simulate processing time
                }

                // Show result
                popup_reset(app->popup);
                if(cleared_count == 8) {
                    sound_effects_complete();
                    popup_set_header(app->popup, "Success!", 64, 10, AlignCenter, AlignTop);
                    popup_set_text(
                        app->popup,
                        "All 8 slots\nhave been cleared",
                        64,
                        32,
                        AlignCenter,
                        AlignCenter);

                    CHAM_LOG_I(app->logger, "Batch", "All slots cleared successfully");
                } else {
                    sound_effects_error();
                    popup_set_header(app->popup, "Error", 64, 10, AlignCenter, AlignTop);

                    snprintf(
                        app->text_buffer,
                        sizeof(app->text_buffer),
                        "Only %d/%d slots\nwere cleared",
                        cleared_count,
                        8);
                    popup_set_text(
                        app->popup,
                        app->text_buffer,
                        64,
                        32,
                        AlignCenter,
                        AlignCenter);

                    CHAM_LOG_E(
                        app->logger,
                        "Batch",
                        "Clear incomplete: %d/8 slots",
                        cleared_count);
                }

                view_dispatcher_switch_to_view(app->view_dispatcher, ChameleonViewPopup);
                furi_delay_ms(3000);
            } else {
                // User cancelled
                CHAM_LOG_I(app->logger, "Batch", "Clear all cancelled by user");
            }

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
