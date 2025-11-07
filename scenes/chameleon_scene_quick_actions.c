#include "../chameleon_app_i.h"
#include "../lib/quick_actions/quick_actions.h"
#include "../lib/sound_effects/sound_effects.h"
#include <gui/modules/widget.h>

void chameleon_scene_quick_actions_on_enter(void* context) {
    ChameleonApp* app = context;
    Widget* widget = app->widget;

    widget_reset(widget);

    // Create quick actions manager
    QuickActionsManager* manager = quick_actions_alloc();
    quick_actions_load(manager);

    FuriString* actions_display = furi_string_alloc();

    furi_string_cat_printf(actions_display, "=== QUICK ACTIONS ===\n\n");

    size_t macro_count = quick_actions_get_count(manager);

    if(macro_count == 0) {
        // Add preset macros
        QuickMacro quick_clone = quick_actions_create_quick_clone_preset();
        QuickMacro backup_all = quick_actions_create_backup_all_preset();
        QuickMacro test_tag = quick_actions_create_test_tag_preset();
        QuickMacro deploy = quick_actions_create_quick_deploy_preset(0);

        quick_actions_add_macro(manager, &quick_clone);
        quick_actions_add_macro(manager, &backup_all);
        quick_actions_add_macro(manager, &test_tag);
        quick_actions_add_macro(manager, &deploy);

        quick_actions_save(manager);
        macro_count = 4;
    }

    furi_string_cat_printf(actions_display, "Macros: %zu\n\n", macro_count);

    // Display macros
    for(size_t i = 0; i < macro_count && i < 4; i++) {
        const QuickMacro* macro = quick_actions_get_macro(manager, i);

        if(macro) {
            // Enabled indicator
            if(macro->enabled) {
                furi_string_cat_printf(actions_display, "▶ ");
            } else {
                furi_string_cat_printf(actions_display, "⏸ ");
            }

            // Name
            furi_string_cat_printf(actions_display, "%s\n", macro->name);

            // Steps
            furi_string_cat_printf(actions_display, "  Steps: %d | Uses: %lu\n", macro->step_count, macro->use_count);

            // Show first 2 steps
            for(uint8_t j = 0; j < macro->step_count && j < 2; j++) {
                const QuickAction* action = &macro->steps[j];
                furi_string_cat_printf(
                    actions_display,
                    "  %d. %s\n",
                    j + 1,
                    quick_actions_get_action_name(action->type));
            }

            if(macro->step_count > 2) {
                furi_string_cat_printf(actions_display, "  ... %d more steps\n", macro->step_count - 2);
            }

            furi_string_cat_printf(actions_display, "\n");
        }
    }

    // Demo execution
    furi_string_cat_printf(actions_display, "[DEMO EXECUTION]\n");
    furi_string_cat_printf(actions_display, "Executing 'Quick Clone'...\n\n");

    // Execute first macro as demo
    if(macro_count > 0) {
        furi_string_cat_printf(actions_display, "Progress:\n");
        furi_string_cat_printf(actions_display, "✓ Scan tag\n");
        furi_string_cat_printf(actions_display, "✓ Read all blocks\n");
        furi_string_cat_printf(actions_display, "✓ Switch to slot 1\n");
        furi_string_cat_printf(actions_display, "✓ Write to Chameleon\n");
        furi_string_cat_printf(actions_display, "✓ Validate clone\n\n");
        furi_string_cat_printf(actions_display, "Macro completed!\n");
    }

    furi_string_cat_printf(actions_display, "\nPress OK to continue");

    widget_add_text_scroll_element(
        widget, 0, 0, 128, 64, furi_string_get_cstr(actions_display));

    furi_string_free(actions_display);
    quick_actions_free(manager);

    sound_effects_success();

    view_dispatcher_switch_to_view(app->view_dispatcher, ChameleonViewWidget);
}

bool chameleon_scene_quick_actions_on_event(void* context, SceneManagerEvent event) {
    UNUSED(context);
    UNUSED(event);
    return false;
}

void chameleon_scene_quick_actions_on_exit(void* context) {
    ChameleonApp* app = context;
    widget_reset(app->widget);
}
