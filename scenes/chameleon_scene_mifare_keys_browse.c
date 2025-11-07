#include "../chameleon_app_i.h"
#include "../lib/mifare_keys/mifare_keys_db.h"
#include "../lib/sound_effects/sound_effects.h"
#include <gui/modules/widget.h>

void chameleon_scene_mifare_keys_browse_on_enter(void* context) {
    ChameleonApp* app = context;
    Widget* widget = app->widget;

    widget_reset(widget);

    // Build display of all keys
    FuriString* keys_display = furi_string_alloc();
    size_t count = mifare_keys_db_get_count();

    furi_string_cat_printf(keys_display, "=== MIFARE KEY DATABASE ===\n");
    furi_string_cat_printf(keys_display, "Total Keys: %zu\n\n", count);

    for(size_t i = 0; i < count; i++) {
        const MifareKeyEntry* entry = mifare_keys_db_get_key(i);
        if(entry) {
            // Format: [Name]
            //         AABBCCDDEEFF
            //         Description
            furi_string_cat_printf(keys_display, "[%zu] %s\n", i + 1, entry->name);
            furi_string_cat_printf(
                keys_display,
                "    %02X%02X%02X%02X%02X%02X\n",
                entry->key[0],
                entry->key[1],
                entry->key[2],
                entry->key[3],
                entry->key[4],
                entry->key[5]);
            furi_string_cat_printf(keys_display, "    %s\n", entry->description);

            // Add spacing every 3 keys
            if((i + 1) % 3 == 0 && i < count - 1) {
                furi_string_cat_printf(keys_display, "\n");
            }
        }
    }

    furi_string_cat_printf(keys_display, "\n[OK] to return");

    widget_add_text_scroll_element(
        widget, 0, 0, 128, 64, furi_string_get_cstr(keys_display));

    furi_string_free(keys_display);

    view_dispatcher_switch_to_view(app->view_dispatcher, ChameleonViewWidget);
}

bool chameleon_scene_mifare_keys_browse_on_event(void* context, SceneManagerEvent event) {
    ChameleonApp* app = context;
    bool consumed = false;

    if(event.type == SceneManagerEventTypeCustom) {
        // Handle custom events if needed
    }

    return consumed;
}

void chameleon_scene_mifare_keys_browse_on_exit(void* context) {
    ChameleonApp* app = context;
    widget_reset(app->widget);
}
