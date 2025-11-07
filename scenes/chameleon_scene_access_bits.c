#include "../chameleon_app_i.h"
#include "../lib/access_bits/access_bits_calc.h"
#include "../lib/sound_effects/sound_effects.h"
#include <gui/modules/widget.h>

void chameleon_scene_access_bits_on_enter(void* context) {
    ChameleonApp* app = context;
    Widget* widget = app->widget;

    widget_reset(widget);

    FuriString* calc_display = furi_string_alloc();

    furi_string_cat_printf(calc_display, "=== ACCESS BITS ===\n\n");

    // Demo: Show all presets
    furi_string_cat_printf(calc_display, "MIFARE Classic Access\nBits Calculator\n\n");

    for(uint8_t preset_id = 0; preset_id < 3; preset_id++) {
        SectorAccessConfig config;
        AccessBits bits;

        if(access_bits_get_preset(preset_id, &config)) {
            // Calculate access bits
            if(access_bits_calculate(&config, &bits)) {
                char hex_string[10];
                access_bits_to_hex_string(&bits, hex_string);

                furi_string_cat_printf(
                    calc_display,
                    "[%s]\n",
                    access_bits_get_preset_name(preset_id));
                furi_string_cat_printf(calc_display, "Bytes: %s\n", hex_string);

                // Validate
                if(bits.is_valid) {
                    furi_string_cat_printf(calc_display, "Status: ✓ Valid\n");
                } else {
                    furi_string_cat_printf(calc_display, "Status: ✗ Invalid\n");
                }

                // Show block 0 permissions
                furi_string_cat_printf(calc_display, "\nBlock 0:\n");
                furi_string_cat_printf(
                    calc_display,
                    "  Read: %s\n",
                    access_bits_get_permission_name(config.block0.read));
                furi_string_cat_printf(
                    calc_display,
                    "  Write: %s\n",
                    access_bits_get_permission_name(config.block0.write));

                furi_string_cat_printf(calc_display, "\n");
            }
        }
    }

    // Custom example
    furi_string_cat_printf(calc_display, "[CUSTOM EXAMPLE]\n");
    furi_string_cat_printf(calc_display, "Parse: FF 07 80\n");

    AccessBits factory_bits;
    if(access_bits_from_hex_string("FF 07 80", &factory_bits)) {
        SectorAccessConfig parsed_config;

        if(access_bits_parse(&factory_bits, &parsed_config)) {
            furi_string_cat_printf(calc_display, "✓ Parsed successfully\n");
            furi_string_cat_printf(
                calc_display,
                "Block 0 Read: %s\n",
                access_bits_get_permission_name(parsed_config.block0.read));
        }
    }

    furi_string_cat_printf(calc_display, "\n");

    // Trailer builder demo
    furi_string_cat_printf(calc_display, "[TRAILER BUILDER]\n");
    uint8_t trailer[16];
    uint8_t key_a[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    uint8_t key_b[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

    SectorAccessConfig default_config;
    access_bits_get_preset(PRESET_FACTORY, &default_config);

    AccessBits trailer_bits;
    access_bits_calculate(&default_config, &trailer_bits);

    access_bits_create_trailer(key_a, &trailer_bits, key_b, trailer);

    furi_string_cat_printf(calc_display, "Generated trailer:\n");
    for(int i = 0; i < 16; i++) {
        furi_string_cat_printf(calc_display, "%02X", trailer[i]);
        if(i == 5 || i == 9) {
            furi_string_cat_printf(calc_display, " ");
        }
    }

    furi_string_cat_printf(calc_display, "\n\nPress OK to continue");

    widget_add_text_scroll_element(
        widget, 0, 0, 128, 64, furi_string_get_cstr(calc_display));

    furi_string_free(calc_display);

    sound_effects_success();

    view_dispatcher_switch_to_view(app->view_dispatcher, ChameleonViewWidget);
}

bool chameleon_scene_access_bits_on_event(void* context, SceneManagerEvent event) {
    UNUSED(context);
    UNUSED(event);
    return false;
}

void chameleon_scene_access_bits_on_exit(void* context) {
    ChameleonApp* app = context;
    widget_reset(app->widget);
}
