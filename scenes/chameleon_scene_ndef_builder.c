#include "../chameleon_app_i.h"
#include "../lib/ndef_builder/ndef_builder.h"
#include "../lib/sound_effects/sound_effects.h"
#include <gui/modules/widget.h>

void chameleon_scene_ndef_builder_on_enter(void* context) {
    ChameleonApp* app = context;
    Widget* widget = app->widget;

    widget_reset(widget);

    FuriString* ndef_display = furi_string_alloc();

    furi_string_cat_printf(ndef_display, "=== NDEF BUILDER ===\n\n");

    // Create NDEF builder
    NdefBuilder* builder = ndef_builder_alloc();

    furi_string_cat_printf(ndef_display, "NFC Data Exchange\nFormat Builder\n\n");

    // Demo 1: URL record
    furi_string_cat_printf(ndef_display, "[1] URL Record\n");
    ndef_builder_add_url_record(builder, "https://github.com/RfidResearchGroup/ChameleonUltra");

    char desc[64];
    ndef_builder_get_description(builder, 0, desc, sizeof(desc));
    furi_string_cat_printf(ndef_display, "%s\n\n", desc);

    // Demo 2: Text record
    furi_string_cat_printf(ndef_display, "[2] Text Record\n");
    ndef_builder_add_text_record(builder, "Chameleon Ultra by Flipper!", "en", TextEncodingUtf8);

    ndef_builder_get_description(builder, 1, desc, sizeof(desc));
    furi_string_cat_printf(ndef_display, "%s\n\n", desc);

    // Demo 3: WiFi record
    furi_string_cat_printf(ndef_display, "[3] WiFi Config\n");
    ndef_builder_add_wifi_record(
        builder,
        "MyNetwork",
        "password123",
        WifiAuthWpa2Personal,
        WifiEncryptAes);

    ndef_builder_get_description(builder, 2, desc, sizeof(desc));
    furi_string_cat_printf(ndef_display, "%s\n", desc);
    furi_string_cat_printf(ndef_display, "SSID: MyNetwork\n");
    furi_string_cat_printf(ndef_display, "Auth: WPA2-PSK\n\n");

    // Demo 4: App Launch record
    furi_string_cat_printf(ndef_display, "[4] App Launch\n");
    ndef_builder_add_app_launch_record(builder, "com.proxgrind.chameleon");

    ndef_builder_get_description(builder, 3, desc, sizeof(desc));
    furi_string_cat_printf(ndef_display, "%s\n\n", desc);

    // Demo 5: vCard record
    furi_string_cat_printf(ndef_display, "[5] vCard Contact\n");
    ndef_builder_add_vcard_record(
        builder,
        "John Doe",
        "+1234567890",
        "john@example.com",
        "ACME Corp");

    ndef_builder_get_description(builder, 4, desc, sizeof(desc));
    furi_string_cat_printf(ndef_display, "%s\n", desc);
    furi_string_cat_printf(ndef_display, "Name: John Doe\n");
    furi_string_cat_printf(ndef_display, "Tel: +1234567890\n\n");

    // Serialize message
    uint8_t ndef_data[512];
    size_t ndef_size = ndef_builder_serialize(builder, ndef_data, sizeof(ndef_data));

    furi_string_cat_printf(ndef_display, "=== SERIALIZED ===\n");
    furi_string_cat_printf(ndef_display, "Total: %zu bytes\n", ndef_size);
    furi_string_cat_printf(ndef_display, "Records: %zu\n", ndef_builder_get_record_count(builder));

    // Show first 32 bytes in hex
    furi_string_cat_printf(ndef_display, "\nFirst 32 bytes:\n");
    for(size_t i = 0; i < 32 && i < ndef_size; i++) {
        furi_string_cat_printf(ndef_display, "%02X", ndef_data[i]);
        if((i + 1) % 16 == 0) {
            furi_string_cat_printf(ndef_display, "\n");
        } else if((i + 1) % 8 == 0) {
            furi_string_cat_printf(ndef_display, " ");
        }
    }

    furi_string_cat_printf(ndef_display, "\n\n");

    // Validation
    if(ndef_builder_validate(builder)) {
        furi_string_cat_printf(ndef_display, "✓ Message Valid\n");
    } else {
        furi_string_cat_printf(ndef_display, "✗ Message Invalid\n");
    }

    furi_string_cat_printf(ndef_display, "\nReady to write to\nChameleon NTAG emulation\n");

    furi_string_cat_printf(ndef_display, "\nPress OK to continue");

    widget_add_text_scroll_element(
        widget, 0, 0, 128, 64, furi_string_get_cstr(ndef_display));

    furi_string_free(ndef_display);
    ndef_builder_free(builder);

    sound_effects_success();

    view_dispatcher_switch_to_view(app->view_dispatcher, ChameleonViewWidget);
}

bool chameleon_scene_ndef_builder_on_event(void* context, SceneManagerEvent event) {
    UNUSED(context);
    UNUSED(event);
    return false;
}

void chameleon_scene_ndef_builder_on_exit(void* context) {
    ChameleonApp* app = context;
    widget_reset(app->widget);
}
