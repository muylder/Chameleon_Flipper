#include "../chameleon_app_i.h"
#include "../lib/nfc_bridge/nfc_bridge.h"
#include "../lib/sound_effects/sound_effects.h"
#include <gui/modules/submenu.h>

typedef enum {
    SubmenuIndexPing,
    SubmenuIndexGetStatus,
    SubmenuIndexSwitchSlot,
    SubmenuIndexAbout,
} SubmenuIndex;

static void chameleon_scene_nfc_bridge_submenu_callback(void* context, uint32_t index) {
    ChameleonApp* app = context;
    view_dispatcher_send_custom_event(app->view_dispatcher, index);
}

void chameleon_scene_nfc_bridge_on_enter(void* context) {
    ChameleonApp* app = context;
    Submenu* submenu = app->submenu;

    submenu_reset(submenu);
    submenu_set_header(submenu, "NFC Wireless Bridge");

    submenu_add_item(
        submenu,
        "Ping Chameleon",
        SubmenuIndexPing,
        chameleon_scene_nfc_bridge_submenu_callback,
        app);

    submenu_add_item(
        submenu,
        "Get Status",
        SubmenuIndexGetStatus,
        chameleon_scene_nfc_bridge_submenu_callback,
        app);

    submenu_add_item(
        submenu,
        "Switch Slot (NFC)",
        SubmenuIndexSwitchSlot,
        chameleon_scene_nfc_bridge_submenu_callback,
        app);

    submenu_add_item(
        submenu,
        "About NFC Bridge",
        SubmenuIndexAbout,
        chameleon_scene_nfc_bridge_submenu_callback,
        app);

    view_dispatcher_switch_to_view(app->view_dispatcher, ChameleonViewSubmenu);
}

bool chameleon_scene_nfc_bridge_on_event(void* context, SceneManagerEvent event) {
    ChameleonApp* app = context;
    bool consumed = false;

    if(event.type == SceneManagerEventTypeCustom) {
        switch(event.event) {
        case SubmenuIndexPing: {
            popup_reset(app->popup);
            popup_set_header(app->popup, "NFC Bridge", 64, 10, AlignCenter, AlignTop);
            popup_set_text(
                app->popup,
                "Place Flipper near\nChameleon Ultra\n(NFC antenna)",
                64,
                28,
                AlignCenter,
                AlignCenter);
            view_dispatcher_switch_to_view(app->view_dispatcher, ChameleonViewPopup);

            furi_delay_ms(2000);

            // Simulate NFC ping
            NfcBridge* bridge = nfc_bridge_alloc();
            nfc_bridge_init(bridge);

            bool success = nfc_bridge_ping(bridge);

            nfc_bridge_deinit(bridge);
            nfc_bridge_free(bridge);

            // Show result
            popup_reset(app->popup);
            if(success) {
                sound_effects_success();
                popup_set_header(app->popup, "Success!", 64, 10, AlignCenter, AlignTop);
                popup_set_text(
                    app->popup,
                    "Chameleon responded\nvia NFC!\n\nPONG received",
                    64,
                    28,
                    AlignCenter,
                    AlignCenter);

                CHAM_LOG_I(app->logger, "NfcBridge", "Ping successful via NFC");
            } else {
                sound_effects_error();
                popup_set_header(app->popup, "Failed", 64, 10, AlignCenter, AlignTop);
                popup_set_text(
                    app->popup,
                    "No response\n\nEnsure Chameleon is\nin NFC bridge mode",
                    64,
                    28,
                    AlignCenter,
                    AlignCenter);

                CHAM_LOG_E(app->logger, "NfcBridge", "Ping failed");
            }

            view_dispatcher_switch_to_view(app->view_dispatcher, ChameleonViewPopup);
            furi_delay_ms(3000);
            view_dispatcher_switch_to_view(app->view_dispatcher, ChameleonViewSubmenu);

            consumed = true;
            break;
        }

        case SubmenuIndexGetStatus: {
            popup_reset(app->popup);
            popup_set_header(app->popup, "Reading...", 64, 20, AlignCenter, AlignCenter);
            view_dispatcher_switch_to_view(app->view_dispatcher, ChameleonViewPopup);

            furi_delay_ms(1500);

            // Simulate NFC status read
            NfcBridge* bridge = nfc_bridge_alloc();
            nfc_bridge_init(bridge);

            uint8_t active_slot = 0;
            uint8_t device_mode = 0;

            bool success = nfc_bridge_get_status(bridge, &active_slot, &device_mode);

            nfc_bridge_deinit(bridge);
            nfc_bridge_free(bridge);

            // Show result
            popup_reset(app->popup);
            if(success) {
                sound_effects_success();
                popup_set_header(app->popup, "Status via NFC", 64, 10, AlignCenter, AlignTop);

                const char* mode_str = (device_mode == 1) ? "Emulator" : "Reader";

                snprintf(
                    app->text_buffer,
                    sizeof(app->text_buffer),
                    "Active Slot: %d\nMode: %s\n\nRead via NFC!",
                    active_slot,
                    mode_str);

                popup_set_text(app->popup, app->text_buffer, 64, 28, AlignCenter, AlignCenter);

                CHAM_LOG_I(
                    app->logger,
                    "NfcBridge",
                    "Status read: slot=%d mode=%d",
                    active_slot,
                    device_mode);
            } else {
                sound_effects_error();
                popup_set_header(app->popup, "Failed", 64, 10, AlignCenter, AlignTop);
                popup_set_text(
                    app->popup,
                    "Could not read status\nvia NFC",
                    64,
                    32,
                    AlignCenter,
                    AlignCenter);
            }

            view_dispatcher_switch_to_view(app->view_dispatcher, ChameleonViewPopup);
            furi_delay_ms(3000);
            view_dispatcher_switch_to_view(app->view_dispatcher, ChameleonViewSubmenu);

            consumed = true;
            break;
        }

        case SubmenuIndexSwitchSlot: {
            popup_reset(app->popup);
            popup_set_header(app->popup, "Switch Slot", 64, 10, AlignCenter, AlignTop);
            popup_set_text(
                app->popup,
                "Switching to slot 1\nvia NFC...",
                64,
                32,
                AlignCenter,
                AlignCenter);
            view_dispatcher_switch_to_view(app->view_dispatcher, ChameleonViewPopup);

            furi_delay_ms(1500);

            // Simulate NFC slot switch
            NfcBridge* bridge = nfc_bridge_alloc();
            nfc_bridge_init(bridge);

            bool success = nfc_bridge_switch_slot(bridge, 1);

            nfc_bridge_deinit(bridge);
            nfc_bridge_free(bridge);

            // Show result
            popup_reset(app->popup);
            if(success) {
                sound_effects_success();
                popup_set_header(app->popup, "Success!", 64, 10, AlignCenter, AlignTop);
                popup_set_text(
                    app->popup,
                    "Switched to slot 1\nwirelessly via NFC!",
                    64,
                    32,
                    AlignCenter,
                    AlignCenter);

                CHAM_LOG_I(app->logger, "NfcBridge", "Slot switched to 1 via NFC");
            } else {
                sound_effects_error();
                popup_set_header(app->popup, "Failed", 64, 10, AlignCenter, AlignTop);
                popup_set_text(app->popup, "Switch failed", 64, 32, AlignCenter, AlignCenter);
            }

            view_dispatcher_switch_to_view(app->view_dispatcher, ChameleonViewPopup);
            furi_delay_ms(3000);
            view_dispatcher_switch_to_view(app->view_dispatcher, ChameleonViewSubmenu);

            consumed = true;
            break;
        }

        case SubmenuIndexAbout: {
            DialogMessage* message = dialog_message_alloc();
            dialog_message_set_header(
                message,
                "NFC Wireless Bridge",
                64,
                0,
                AlignCenter,
                AlignTop);
            dialog_message_set_text(
                message,
                "Protocol:\n"
                "• NTAG emulation on Chameleon\n"
                "• Commands via NFC pages\n"
                "• Wireless control!\n\n"
                "Status: EXPERIMENTAL",
                64,
                16,
                AlignCenter,
                AlignTop);
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

void chameleon_scene_nfc_bridge_on_exit(void* context) {
    ChameleonApp* app = context;
    submenu_reset(app->submenu);
    popup_reset(app->popup);
}
