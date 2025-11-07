#include "../chameleon_app_i.h"
#include "../lib/uid_generator/uid_generator.h"
#include "../lib/sound_effects/sound_effects.h"
#include <gui/modules/widget.h>

void chameleon_scene_uid_generator_on_enter(void* context) {
    ChameleonApp* app = context;
    Widget* widget = app->widget;

    widget_reset(widget);

    FuriString* uid_display = furi_string_alloc();

    furi_string_cat_printf(uid_display, "=== UID GENERATOR ===\n\n");

    // Generate examples of each UID type
    Uid uid4, uid7, uid10;

    // 4-byte UID
    uid_generate_random(UidType4Byte, &uid4);
    char hex4[32];
    uid_to_hex_string(&uid4, hex4, true);

    furi_string_cat_printf(uid_display, "[4-BYTE MIFARE CLASSIC]\n");
    furi_string_cat_printf(uid_display, "UID: %s\n", hex4);
    furi_string_cat_printf(uid_display, "BCC: %02X\n", uid4.bcc0);
    furi_string_cat_printf(uid_display, "Valid: %s\n\n", uid4.is_valid ? "YES" : "NO");

    // 7-byte UID
    uid_generate_random(UidType7Byte, &uid7);
    char hex7[32];
    uid_to_hex_string(&uid7, hex7, false);

    furi_string_cat_printf(uid_display, "[7-BYTE ULTRALIGHT]\n");
    furi_string_cat_printf(uid_display, "UID: %s\n", hex7);
    furi_string_cat_printf(uid_display, "BCC0: %02X\n", uid7.bcc0);
    furi_string_cat_printf(uid_display, "BCC1: %02X\n", uid7.bcc1);
    furi_string_cat_printf(uid_display, "Valid: %s\n\n", uid7.is_valid ? "YES" : "NO");

    // 10-byte UID
    uid_generate_random(UidType10Byte, &uid10);
    char hex10[32];
    uid_to_hex_string(&uid10, hex10, false);

    furi_string_cat_printf(uid_display, "[10-BYTE EXTENDED]\n");
    furi_string_cat_printf(uid_display, "UID: %s\n", hex10);
    furi_string_cat_printf(uid_display, "BCC0: %02X\n", uid10.bcc0);
    furi_string_cat_printf(uid_display, "BCC1: %02X\n", uid10.bcc1);
    furi_string_cat_printf(uid_display, "Valid: %s\n\n", uid10.is_valid ? "YES" : "NO");

    // Features
    furi_string_cat_printf(uid_display, "[FEATURES]\n");
    furi_string_cat_printf(uid_display, "• Auto BCC calculation\n");
    furi_string_cat_printf(uid_display, "• Validation\n");
    furi_string_cat_printf(uid_display, "• Batch generation\n");
    furi_string_cat_printf(uid_display, "• Hex conversion\n\n");

    furi_string_cat_printf(uid_display, "Press OK to return");

    widget_add_text_scroll_element(
        widget,
        0,
        0,
        128,
        64,
        furi_string_get_cstr(uid_display));

    furi_string_free(uid_display);

    sound_effects_success();

    view_dispatcher_switch_to_view(app->view_dispatcher, ChameleonViewWidget);
}

bool chameleon_scene_uid_generator_on_event(void* context, SceneManagerEvent event) {
    UNUSED(context);
    UNUSED(event);
    return false;
}

void chameleon_scene_uid_generator_on_exit(void* context) {
    ChameleonApp* app = context;
    widget_reset(app->widget);
}
