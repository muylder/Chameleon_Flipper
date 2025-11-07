#include "../chameleon_app_i.h"
#include "../lib/statistics/statistics_manager.h"
#include <gui/modules/widget.h>

static const char* operation_type_names[] = {
    "Tag Read",
    "Tag Write",
    "Slot Switch",
    "Key Test",
    "Backup",
    "Restore",
    "Connect",
    "Disconnect",
};

void chameleon_scene_statistics_on_enter(void* context) {
    ChameleonApp* app = context;
    Widget* widget = app->widget;

    widget_reset(widget);

    const StatisticsData* stats = statistics_manager_get_data(app->statistics_manager);
    if(!stats) {
        widget_add_text_scroll_element(
            widget,
            0,
            0,
            128,
            64,
            "Statistics not available");
        view_dispatcher_switch_to_view(app->view_dispatcher, ChameleonViewWidget);
        return;
    }

    FuriString* stats_display = furi_string_alloc();

    // Header
    furi_string_cat_printf(stats_display, "=== STATISTICS ===\n\n");

    // Session Info
    furi_string_cat_printf(stats_display, "[SESSIONS]\n");
    furi_string_cat_printf(stats_display, "Total: %lu\n", stats->total_sessions);
    furi_string_cat_printf(
        stats_display,
        "Runtime: %lu sec (%.1fh)\n",
        stats->total_runtime_seconds,
        stats->total_runtime_seconds / 3600.0f);

    // Last session
    if(stats->last_session_timestamp > 0) {
        DateTime dt;
        datetime_timestamp_to_datetime(stats->last_session_timestamp, &dt);
        furi_string_cat_printf(
            stats_display,
            "Last: %02d/%02d/%04d %02d:%02d\n",
            dt.day,
            dt.month,
            dt.year,
            dt.hour,
            dt.minute);
    }

    furi_string_cat_printf(stats_display, "\n");

    // Operations
    furi_string_cat_printf(stats_display, "[OPERATIONS]\n");

    // Tag Reads
    uint32_t total_reads = stats->tag_reads_success + stats->tag_reads_failed;
    if(total_reads > 0) {
        furi_string_cat_printf(
            stats_display,
            "Reads: %lu/%lu (%.0f%%)\n",
            stats->tag_reads_success,
            total_reads,
            stats->tag_read_success_rate);
    } else {
        furi_string_cat_printf(stats_display, "Reads: 0\n");
    }

    // Tag Writes
    uint32_t total_writes = stats->tag_writes_success + stats->tag_writes_failed;
    if(total_writes > 0) {
        furi_string_cat_printf(
            stats_display,
            "Writes: %lu/%lu (%.0f%%)\n",
            stats->tag_writes_success,
            total_writes,
            stats->tag_write_success_rate);
    } else {
        furi_string_cat_printf(stats_display, "Writes: 0\n");
    }

    // Key Tests
    uint32_t total_key_tests = stats->key_tests_success + stats->key_tests_failed;
    if(total_key_tests > 0) {
        furi_string_cat_printf(
            stats_display,
            "Key Tests: %lu/%lu (%.0f%%)\n",
            stats->key_tests_success,
            total_key_tests,
            stats->key_test_success_rate);
    } else {
        furi_string_cat_printf(stats_display, "Key Tests: 0\n");
    }

    furi_string_cat_printf(stats_display, "Slot Switches: %lu\n", stats->slot_switches);
    furi_string_cat_printf(stats_display, "Backups: %lu\n", stats->backups_created);
    furi_string_cat_printf(stats_display, "Restores: %lu\n", stats->restores_done);
    furi_string_cat_printf(stats_display, "Connections: %lu\n", stats->connections_made);

    furi_string_cat_printf(stats_display, "\n");

    // Slot Usage
    furi_string_cat_printf(stats_display, "[SLOT USAGE]\n");

    // Find most used slot
    uint8_t most_used_slot = 0;
    uint32_t most_used_count = 0;
    for(int i = 0; i < 8; i++) {
        if(stats->slot_usage_count[i] > most_used_count) {
            most_used_count = stats->slot_usage_count[i];
            most_used_slot = i;
        }
    }

    for(int i = 0; i < 8; i++) {
        furi_string_cat_printf(stats_display, "Slot %d: %u", i, stats->slot_usage_count[i]);
        if(i == most_used_slot && most_used_count > 0) {
            furi_string_cat_printf(stats_display, " *");
        }
        furi_string_cat_printf(stats_display, "\n");
    }

    furi_string_cat_printf(stats_display, "\n");

    // Recent History
    furi_string_cat_printf(stats_display, "[RECENT HISTORY]\n");
    size_t history_count = statistics_manager_get_history_count(app->statistics_manager);

    if(history_count > 0) {
        size_t show_count = history_count > 10 ? 10 : history_count;
        for(size_t i = 0; i < show_count; i++) {
            const StatHistoryEntry* entry =
                statistics_manager_get_history_entry(app->statistics_manager, i);

            if(entry) {
                DateTime dt;
                datetime_timestamp_to_datetime(entry->timestamp, &dt);

                const char* result_str = entry->result == StatResultSuccess ? "OK" : "FAIL";

                furi_string_cat_printf(
                    stats_display,
                    "%02d:%02d %s [%s]\n",
                    dt.hour,
                    dt.minute,
                    operation_type_names[entry->type],
                    result_str);

                if(entry->details[0] != '\0') {
                    furi_string_cat_printf(stats_display, "  %s\n", entry->details);
                }
            }
        }
    } else {
        furi_string_cat_printf(stats_display, "No history yet\n");
    }

    furi_string_cat_printf(stats_display, "\n[OK] to return");

    widget_add_text_scroll_element(
        widget,
        0,
        0,
        128,
        64,
        furi_string_get_cstr(stats_display));

    furi_string_free(stats_display);

    view_dispatcher_switch_to_view(app->view_dispatcher, ChameleonViewWidget);
}

bool chameleon_scene_statistics_on_event(void* context, SceneManagerEvent event) {
    UNUSED(context);
    UNUSED(event);
    return false;
}

void chameleon_scene_statistics_on_exit(void* context) {
    ChameleonApp* app = context;
    widget_reset(app->widget);
}
