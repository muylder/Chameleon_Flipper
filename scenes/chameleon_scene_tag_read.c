#include "../chameleon_app_i.h"

typedef enum {
    TagReadEventAnimationDone = 0,
} TagReadEvent;

static void chameleon_scene_tag_read_animation_callback(void* context) {
    ChameleonApp* app = context;
    view_dispatcher_send_custom_event(app->view_dispatcher, TagReadEventAnimationDone);
}

void chameleon_scene_tag_read_on_enter(void* context) {
    ChameleonApp* app = context;

    // Show transfer animation for tag reading
    chameleon_animation_view_set_type(app->animation_view, ChameleonAnimationTransfer);
    chameleon_animation_view_set_callback(
        app->animation_view,
        chameleon_scene_tag_read_animation_callback,
        app);

    view_dispatcher_switch_to_view(app->view_dispatcher, ChameleonViewAnimation);
    chameleon_animation_view_start(app->animation_view);

    // Start tag reading in parallel
    // Try HF scan first
    uint8_t uid[10];
    uint8_t uid_len = 0;
    uint8_t atqa[2];
    uint8_t sak;

    bool found = false;

    FURI_LOG_I("TagRead", "Scanning for HF tags...");

    if(chameleon_app_hf14a_scan(app, uid, &uid_len, atqa, &sak)) {
        FURI_LOG_I("TagRead", "HF tag detected!");

        // Format tag info
        char tag_info[256];
        int pos = snprintf(tag_info, sizeof(tag_info), "HF Tag Found!\n\nUID: ");

        for(uint8_t i = 0; i < uid_len; i++) {
            pos += snprintf(&tag_info[pos], sizeof(tag_info) - pos, "%02X", uid[i]);
            if(i < uid_len - 1) pos += snprintf(&tag_info[pos], sizeof(tag_info) - pos, ":");
        }

        pos += snprintf(&tag_info[pos], sizeof(tag_info) - pos, "\nATQA: %02X%02X\nSAK: %02X", atqa[0], atqa[1], sak);

        // Determine tag type
        const char* tag_type = "Unknown";
        if(sak == 0x08) tag_type = "Mifare Classic 1K";
        else if(sak == 0x18) tag_type = "Mifare Classic 4K";
        else if(sak == 0x00) tag_type = "Mifare Ultralight";
        else if(sak == 0x20) tag_type = "Mifare Plus";

        pos += snprintf(&tag_info[pos], sizeof(tag_info) - pos, "\nType: %s", tag_type);

        // Store in buffer for display
        snprintf(app->text_buffer, sizeof(app->text_buffer), "%s", tag_info);

        found = true;
    }

    // If no HF tag, try LF
    if(!found) {
        FURI_LOG_I("TagRead", "No HF tag, trying LF...");
        uint8_t em_id[5];

        if(chameleon_app_em410x_scan(app, em_id)) {
            FURI_LOG_I("TagRead", "LF tag detected!");

            char tag_info[128];
            int pos = snprintf(tag_info, sizeof(tag_info), "LF Tag Found!\n\nEM410X ID:\n");

            for(uint8_t i = 0; i < 5; i++) {
                pos += snprintf(&tag_info[pos], sizeof(tag_info) - pos, "%02X", em_id[i]);
                if(i < 4) pos += snprintf(&tag_info[pos], sizeof(tag_info) - pos, ":");
            }

            snprintf(app->text_buffer, sizeof(app->text_buffer), "%s", tag_info);

            found = true;
        }
    }

    if(!found) {
        FURI_LOG_W("TagRead", "No tag detected");
        snprintf(app->text_buffer, sizeof(app->text_buffer), "");
    }
}

bool chameleon_scene_tag_read_on_event(void* context, SceneManagerEvent event) {
    ChameleonApp* app = context;
    bool consumed = false;

    if(event.type == SceneManagerEventTypeCustom) {
        if(event.event == TagReadEventAnimationDone) {
            // Animation done, show results
            if(strlen(app->text_buffer) > 0) {
                // Tag was found, show in widget
                widget_reset(app->widget);
                widget_add_text_scroll_element(app->widget, 0, 0, 128, 64, app->text_buffer);
                view_dispatcher_switch_to_view(app->view_dispatcher, ChameleonViewWidget);

                // Auto-return after 5 seconds
                furi_delay_ms(5000);
            } else {
                // No tag found, show popup
                popup_reset(app->popup);
                popup_set_header(app->popup, "No Tag Found", 64, 10, AlignCenter, AlignTop);
                popup_set_text(app->popup, "No HF or LF tag\ndetected", 64, 32, AlignCenter, AlignCenter);
                view_dispatcher_switch_to_view(app->view_dispatcher, ChameleonViewPopup);
                furi_delay_ms(2000);
            }

            scene_manager_previous_scene(app->scene_manager);
            consumed = true;
        }
    }

    return consumed;
}

void chameleon_scene_tag_read_on_exit(void* context) {
    ChameleonApp* app = context;
    chameleon_animation_view_stop(app->animation_view);
    popup_reset(app->popup);
    widget_reset(app->widget);
    memset(app->text_buffer, 0, sizeof(app->text_buffer));
}
