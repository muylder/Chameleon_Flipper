#include "../chameleon_app_i.h"

typedef enum {
    TagWriteEventAnimationDone = 0,
    TagWriteEventFileSelected = 1,
} TagWriteEvent;

static void chameleon_scene_tag_write_animation_callback(void* context) {
    ChameleonApp* app = context;
    view_dispatcher_send_custom_event(app->view_dispatcher, TagWriteEventAnimationDone);
}

void chameleon_scene_tag_write_on_enter(void* context) {
    ChameleonApp* app = context;

    // Show file browser to select tag file
    DialogsFileBrowserOptions browser_options;
    dialog_file_browser_set_basic_options(&browser_options, ".nfc", NULL);
    browser_options.base_path = "/ext/nfc";
    browser_options.hide_ext = false;

    FuriString* file_path = furi_string_alloc();
    furi_string_set(file_path, browser_options.base_path);

    bool file_selected = dialog_file_browser_show(app->dialogs, file_path, file_path, &browser_options);

    if(!file_selected) {
        // User cancelled, go back
        furi_string_free(file_path);
        scene_manager_previous_scene(app->scene_manager);
        return;
    }

    FURI_LOG_I("TagWrite", "Selected file: %s", furi_string_get_cstr(file_path));

    // Load tag data from file
    uint8_t tag_data[512];
    size_t tag_data_len = 0;
    ChameleonTagType tag_type = TagTypeUnknown;

    if(!chameleon_app_load_tag_from_file(
           app, furi_string_get_cstr(file_path), tag_data, &tag_data_len, &tag_type)) {
        FURI_LOG_E("TagWrite", "Failed to load tag file");

        // Show error
        popup_reset(app->popup);
        popup_set_header(app->popup, "Error", 64, 10, AlignCenter, AlignTop);
        popup_set_text(app->popup, "Failed to load\ntag file", 64, 32, AlignCenter, AlignCenter);
        view_dispatcher_switch_to_view(app->view_dispatcher, ChameleonViewPopup);
        furi_delay_ms(2000);

        furi_string_free(file_path);
        scene_manager_previous_scene(app->scene_manager);
        return;
    }

    furi_string_free(file_path);

    // Show transfer animation during write
    chameleon_animation_view_set_type(app->animation_view, ChameleonAnimationTransfer);
    chameleon_animation_view_set_callback(
        app->animation_view, chameleon_scene_tag_write_animation_callback, app);
    view_dispatcher_switch_to_view(app->view_dispatcher, ChameleonViewAnimation);
    chameleon_animation_view_start(app->animation_view);

    // Write tag data to Chameleon based on type
    bool write_success = false;

    if(tag_type == TagTypeEM410X && tag_data_len == 5) {
        // Write EM410X ID
        FURI_LOG_I("TagWrite", "Writing EM410X tag");
        write_success = chameleon_app_em410x_set_emu_id(app, tag_data);

    } else if(tag_type == TagTypeMifareClassic1K || tag_type == TagTypeMifareClassic4K) {
        // Write Mifare Classic blocks
        FURI_LOG_I("TagWrite", "Writing Mifare Classic tag");

        // Calculate number of blocks (each block is 16 bytes)
        size_t num_blocks = tag_data_len / 16;
        if(num_blocks > 64) num_blocks = 64; // Max 64 blocks for 1K

        write_success = true;
        for(size_t i = 0; i < num_blocks && write_success; i++) {
            write_success = chameleon_app_mf1_write_emu_block(app, i, &tag_data[i * 16]);
            if(!write_success) {
                FURI_LOG_E("TagWrite", "Failed to write block %zu", i);
                break;
            }
        }

    } else {
        FURI_LOG_W("TagWrite", "Unsupported tag type: %u", tag_type);
        snprintf(
            app->text_buffer,
            sizeof(app->text_buffer),
            "Unsupported tag type");
    }

    // Store result in text buffer
    if(write_success) {
        snprintf(
            app->text_buffer,
            sizeof(app->text_buffer),
            "Tag written!\n\n%zu bytes\nType: %s",
            tag_data_len,
            (tag_type == TagTypeEM410X) ? "EM410X" :
            (tag_type == TagTypeMifareClassic1K) ? "Mifare 1K" :
            (tag_type == TagTypeMifareClassic4K) ? "Mifare 4K" :
            "Unknown");
    } else {
        snprintf(app->text_buffer, sizeof(app->text_buffer), "");
    }
}

bool chameleon_scene_tag_write_on_event(void* context, SceneManagerEvent event) {
    ChameleonApp* app = context;
    bool consumed = false;

    if(event.type == SceneManagerEventTypeCustom) {
        if(event.event == TagWriteEventAnimationDone) {
            // Animation done, show results
            if(strlen(app->text_buffer) > 0) {
                // Write successful
                chameleon_animation_view_set_type(app->animation_view, ChameleonAnimationSuccess);
                chameleon_animation_view_set_callback(app->animation_view, NULL, NULL);
                chameleon_animation_view_start(app->animation_view);

                // Wait for success animation
                furi_delay_ms(4000);

                // Show result widget
                widget_reset(app->widget);
                widget_add_text_scroll_element(app->widget, 0, 0, 128, 64, app->text_buffer);
                view_dispatcher_switch_to_view(app->view_dispatcher, ChameleonViewWidget);

                // Auto-return after 3 seconds
                furi_delay_ms(3000);
            } else {
                // Write failed, show error
                chameleon_animation_view_set_type(app->animation_view, ChameleonAnimationError);
                chameleon_animation_view_set_callback(app->animation_view, NULL, NULL);
                chameleon_animation_view_start(app->animation_view);

                furi_delay_ms(4000);

                popup_reset(app->popup);
                popup_set_header(app->popup, "Write Failed", 64, 10, AlignCenter, AlignTop);
                popup_set_text(
                    app->popup,
                    "Failed to write\ntag to Chameleon",
                    64,
                    32,
                    AlignCenter,
                    AlignCenter);
                view_dispatcher_switch_to_view(app->view_dispatcher, ChameleonViewPopup);
                furi_delay_ms(2000);
            }

            scene_manager_previous_scene(app->scene_manager);
            consumed = true;
        }
    }

    return consumed;
}

void chameleon_scene_tag_write_on_exit(void* context) {
    ChameleonApp* app = context;
    chameleon_animation_view_stop(app->animation_view);
    popup_reset(app->popup);
    widget_reset(app->widget);
    memset(app->text_buffer, 0, sizeof(app->text_buffer));
}
