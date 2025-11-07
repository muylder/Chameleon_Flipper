#include "../chameleon_app_i.h"
#include "../lib/tag_validator/tag_validator.h"
#include "../lib/sound_effects/sound_effects.h"
#include <gui/modules/widget.h>

void chameleon_scene_tag_validation_on_enter(void* context) {
    ChameleonApp* app = context;
    Widget* widget = app->widget;

    widget_reset(widget);

    // Create sample validation report for demonstration
    TagValidator* validator = tag_validator_alloc();

    // Sample reference tag (real tag)
    TagData reference = {
        .uid = {0x04, 0xAB, 0xCD, 0xEF, 0x12, 0x34, 0x80},
        .uid_len = 7,
        .atqa = {0x44, 0x00},
        .sak = 0x08,
        .block_count = 4,
    };

    // Sample blocks
    memset(reference.block_data[0], 0x04, 16); // Block 0
    memset(reference.block_data[1], 0x00, 16); // Block 1
    memset(reference.block_data[2], 0x00, 16); // Block 2
    reference.block_data[3][0] = 0xFF; // Block 3 (trailer)

    // Sample test tag (emulated - with one intentional difference)
    TagData test = reference; // Copy reference
    test.block_data[1][0] = 0x01; // Intentional difference in block 1

    tag_validator_set_reference(validator, &reference);
    tag_validator_set_test(validator, &test);

    // Run validation
    TagValidationReport report;
    tag_validator_run_tests(validator, &report);

    // Build display
    FuriString* validation_display = furi_string_alloc();

    furi_string_cat_printf(validation_display, "=== TAG VALIDATION ===\n\n");

    // Summary
    furi_string_cat_printf(validation_display, "[SUMMARY]\n");
    furi_string_cat_printf(validation_display, "Tests: %d total\n", report.tests_total);
    furi_string_cat_printf(validation_display, "Pass: %d\n", report.tests_passed);
    furi_string_cat_printf(validation_display, "Fail: %d\n", report.tests_failed);
    furi_string_cat_printf(validation_display, "Skip: %d\n", report.tests_skipped);

    if(report.tests_errored > 0) {
        furi_string_cat_printf(validation_display, "Errors: %d\n", report.tests_errored);
    }

    furi_string_cat_printf(validation_display, "Success: %.0f%%\n", report.success_rate);
    furi_string_cat_printf(validation_display, "Time: %lu ms\n\n", report.total_duration_ms);

    // Test results
    furi_string_cat_printf(validation_display, "[TEST RESULTS]\n");

    for(int i = 0; i < report.tests_total; i++) {
        const TagValidationTestResult* result = &report.test_results[i];

        const char* status_icon;
        switch(result->result) {
        case TestResultPass:
            status_icon = "✓";
            break;
        case TestResultFail:
            status_icon = "✗";
            break;
        case TestResultSkipped:
            status_icon = "-";
            break;
        default:
            status_icon = "!";
            break;
        }

        furi_string_cat_printf(
            validation_display,
            "%s %s\n",
            status_icon,
            tag_validator_get_test_name(result->type));

        // Show details for failed tests
        if(result->result == TestResultFail) {
            furi_string_cat_printf(validation_display, "  %s\n", result->details);
        } else if(result->result == TestResultPass) {
            furi_string_cat_printf(validation_display, "  OK (%lu ms)\n", result->duration_ms);
        }
    }

    furi_string_cat_printf(validation_display, "\n");

    // Overall result
    if(report.success_rate == 100.0f) {
        furi_string_cat_printf(validation_display, "✓ VALIDATION PASSED\n");
        furi_string_cat_printf(validation_display, "Emulation is accurate!\n");
        sound_effects_success();
    } else if(report.success_rate >= 75.0f) {
        furi_string_cat_printf(validation_display, "⚠ PARTIAL MATCH\n");
        furi_string_cat_printf(validation_display, "Some differences detected\n");
        sound_effects_warning();
    } else {
        furi_string_cat_printf(validation_display, "✗ VALIDATION FAILED\n");
        furi_string_cat_printf(validation_display, "Emulation needs fixing\n");
        sound_effects_error();
    }

    furi_string_cat_printf(validation_display, "\n[OK] to return");

    widget_add_text_scroll_element(
        widget,
        0,
        0,
        128,
        64,
        furi_string_get_cstr(validation_display));

    furi_string_free(validation_display);
    tag_validator_free(validator);

    view_dispatcher_switch_to_view(app->view_dispatcher, ChameleonViewWidget);
}

bool chameleon_scene_tag_validation_on_event(void* context, SceneManagerEvent event) {
    UNUSED(context);
    UNUSED(event);
    return false;
}

void chameleon_scene_tag_validation_on_exit(void* context) {
    ChameleonApp* app = context;
    widget_reset(app->widget);
}
