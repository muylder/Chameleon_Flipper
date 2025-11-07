#include "../chameleon_app_i.h"
#include "../lib/clone_wizard/clone_wizard.h"
#include "../lib/sound_effects/sound_effects.h"
#include <gui/modules/widget.h>

void chameleon_scene_clone_wizard_on_enter(void* context) {
    ChameleonApp* app = context;
    Widget* widget = app->widget;

    widget_reset(widget);

    // Create clone wizard demo
    CloneWizard* wizard = clone_wizard_alloc();

    // Simulate wizard execution
    FuriString* wizard_display = furi_string_alloc();

    furi_string_cat_printf(wizard_display, "=== TAG CLONING WIZARD ===\n\n");

    // Demo: Execute all steps
    const CloneWizardState* state = clone_wizard_get_state(wizard);

    furi_string_cat_printf(wizard_display, "[WIZARD DEMO]\n");
    furi_string_cat_printf(wizard_display, "This wizard helps you:\n");
    furi_string_cat_printf(wizard_display, "1. Scan original tag\n");
    furi_string_cat_printf(wizard_display, "2. Detect tag type\n");
    furi_string_cat_printf(wizard_display, "3. Test auth keys\n");
    furi_string_cat_printf(wizard_display, "4. Read all blocks\n");
    furi_string_cat_printf(wizard_display, "5. Select slot\n");
    furi_string_cat_printf(wizard_display, "6. Write to Chameleon\n");
    furi_string_cat_printf(wizard_display, "7. Validate clone\n");
    furi_string_cat_printf(wizard_display, "8. Save backup\n\n");

    // Execute mock wizard
    uint8_t uid[10], uid_len, atqa[2], sak;

    // Step 1: Scan
    if(clone_wizard_scan_tag(wizard, uid, &uid_len, atqa, &sak)) {
        furi_string_cat_printf(wizard_display, "✓ Tag scanned\n");
        furi_string_cat_printf(wizard_display, "  UID: ");
        for(uint8_t i = 0; i < uid_len; i++) {
            furi_string_cat_printf(wizard_display, "%02X", uid[i]);
        }
        furi_string_cat_printf(wizard_display, "\n");
    }

    // Step 2: Detect type
    uint8_t tag_type = clone_wizard_detect_type(wizard);
    furi_string_cat_printf(wizard_display, "✓ Type detected: ");
    if(tag_type == 1) {
        furi_string_cat_printf(wizard_display, "MIFARE Classic 1K\n");
    } else {
        furi_string_cat_printf(wizard_display, "Unknown\n");
    }

    // Step 3-4: Read
    furi_string_cat_printf(wizard_display, "✓ Reading blocks...\n");
    uint8_t blocks_read = clone_wizard_read_tag(wizard, NULL, NULL);
    furi_string_cat_printf(wizard_display, "  %d blocks read\n\n", blocks_read);

    // Result
    furi_string_cat_printf(wizard_display, "[STATUS]\n");
    furi_string_cat_printf(wizard_display, "Ready to clone!\n");
    furi_string_cat_printf(wizard_display, "Similarity: 100%%\n\n");

    furi_string_cat_printf(wizard_display, "Press OK to continue");

    widget_add_text_scroll_element(
        widget,
        0,
        0,
        128,
        64,
        furi_string_get_cstr(wizard_display));

    furi_string_free(wizard_display);
    clone_wizard_free(wizard);

    sound_effects_success();

    view_dispatcher_switch_to_view(app->view_dispatcher, ChameleonViewWidget);
}

bool chameleon_scene_clone_wizard_on_event(void* context, SceneManagerEvent event) {
    UNUSED(context);
    UNUSED(event);
    return false;
}

void chameleon_scene_clone_wizard_on_exit(void* context) {
    ChameleonApp* app = context;
    widget_reset(app->widget);
}
