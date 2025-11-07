#include "../chameleon_app_i.h"
#include "../lib/emulation_recorder/emulation_recorder.h"
#include "../lib/sound_effects/sound_effects.h"
#include <gui/modules/widget.h>

void chameleon_scene_emulation_recorder_on_enter(void* context) {
    ChameleonApp* app = context;
    Widget* widget = app->widget;

    widget_reset(widget);

    FuriString* recorder_display = furi_string_alloc();

    furi_string_cat_printf(recorder_display, "=== EMULATION RECORDER ===\n\n");

    // Create emulation recorder
    EmulationRecorder* recorder = emulation_recorder_alloc();

    // Start a demo session
    emulation_recorder_start_session(recorder, "Demo Session");

    furi_string_cat_printf(recorder_display, "Session: Demo Session\n");
    furi_string_cat_printf(recorder_display, "Status: Recording\n\n");

    // Simulate emulation events
    emulation_recorder_record_event(recorder, EmulEventActivated, "Tag activated");
    emulation_recorder_record_event(recorder, EmulEventReaderDetected, "Reader detected");

    // Simulate authentication attempts
    for(uint8_t s = 0; s < 4; s++) {
        emulation_recorder_record_authentication(
            recorder, s, true, (s < 3) ? AuthResultSuccess : AuthResultFailed);

        if(s < 3) {
            // Simulate block reads for successful auths
            for(uint8_t b = 0; b < 3; b++) {
                uint8_t block = s * 4 + b;
                uint8_t data[16] = {0};
                for(int i = 0; i < 16; i++) data[i] = block + i;
                emulation_recorder_record_block_read(recorder, block, data);
            }
        }
    }

    emulation_recorder_record_event(recorder, EmulEventReaderLost, "Reader disconnected");

    // End session
    emulation_recorder_end_session(recorder);

    // Analyze reader
    emulation_recorder_analyze_reader(recorder);

    // Display statistics
    const SessionStatistics* stats = emulation_recorder_get_statistics(recorder);

    furi_string_cat_printf(recorder_display, "=== STATISTICS ===\n");
    furi_string_cat_printf(recorder_display, "Events: %lu\n", stats->total_events);
    furi_string_cat_printf(recorder_display, "Duration: %lu ms\n", stats->session_duration_ms);
    furi_string_cat_printf(recorder_display, "\n");

    furi_string_cat_printf(recorder_display, "Readers: %lu\n", stats->reader_detections);
    furi_string_cat_printf(recorder_display, "Auth attempts: %lu\n",
        stats->authentications_attempted);
    furi_string_cat_printf(recorder_display, "Auth success: %lu\n",
        stats->authentications_successful);
    furi_string_cat_printf(recorder_display, "Auth failed: %lu\n",
        stats->authentications_failed);
    furi_string_cat_printf(recorder_display, "Success rate: %.1f%%\n\n",
        stats->auth_success_rate);

    furi_string_cat_printf(recorder_display, "Blocks read: %lu\n", stats->blocks_read);
    furi_string_cat_printf(recorder_display, "Blocks written: %lu\n", stats->blocks_written);
    furi_string_cat_printf(recorder_display, "Errors: %lu\n\n", stats->errors);

    // Display reader fingerprint
    const ReaderFingerprint* fingerprint = emulation_recorder_get_reader_fingerprint(recorder);

    furi_string_cat_printf(recorder_display, "=== READER ANALYSIS ===\n");
    furi_string_cat_printf(recorder_display, "Type: %s\n", fingerprint->reader_type);
    furi_string_cat_printf(recorder_display, "Sectors accessed: %d\n",
        fingerprint->accessed_sector_count);

    if(fingerprint->accessed_sector_count > 0) {
        furi_string_cat_printf(recorder_display, "List: ");
        for(uint8_t i = 0; i < fingerprint->accessed_sector_count && i < 8; i++) {
            furi_string_cat_printf(recorder_display, "%d", fingerprint->accessed_sectors[i]);
            if(i < fingerprint->accessed_sector_count - 1) {
                furi_string_cat_printf(recorder_display, ",");
            }
        }
        if(fingerprint->accessed_sector_count > 8) {
            furi_string_cat_printf(recorder_display, "...");
        }
        furi_string_cat_printf(recorder_display, "\n");
    }

    // Suspicious activity check
    if(emulation_recorder_detect_suspicious_activity(recorder)) {
        furi_string_cat_printf(recorder_display, "\n⚠ Suspicious activity!\n");
    } else {
        furi_string_cat_printf(recorder_display, "\n✓ Normal activity\n");
    }

    // Show last few events
    furi_string_cat_printf(recorder_display, "\n=== RECENT EVENTS ===\n");

    EmulationEvent events[5];
    size_t event_count = emulation_recorder_get_events(recorder, events, 5);

    for(size_t i = 0; i < event_count && i < 3; i++) {
        furi_string_cat_printf(recorder_display, "%s\n",
            emulation_recorder_get_event_type_name(events[i].type));
    }

    furi_string_cat_printf(recorder_display, "\nPress OK to continue");

    widget_add_text_scroll_element(
        widget, 0, 0, 128, 64, furi_string_get_cstr(recorder_display));

    furi_string_free(recorder_display);
    emulation_recorder_free(recorder);

    sound_effects_success();

    view_dispatcher_switch_to_view(app->view_dispatcher, ChameleonViewWidget);
}

bool chameleon_scene_emulation_recorder_on_event(void* context, SceneManagerEvent event) {
    UNUSED(context);
    UNUSED(event);
    return false;
}

void chameleon_scene_emulation_recorder_on_exit(void* context) {
    ChameleonApp* app = context;
    widget_reset(app->widget);
}
