#include "../chameleon_app_i.h"
#include "../lib/performance_monitor/performance_monitor.h"
#include "../lib/sound_effects/sound_effects.h"
#include <gui/modules/widget.h>

void chameleon_scene_performance_on_enter(void* context) {
    ChameleonApp* app = context;
    Widget* widget = app->widget;

    widget_reset(widget);

    FuriString* perf_display = furi_string_alloc();

    furi_string_cat_printf(perf_display, "=== PERFORMANCE ===\n\n");

    // Create performance monitor
    PerformanceMonitor* monitor = performance_monitor_alloc();

    performance_monitor_start_session(monitor);

    // Run benchmark
    performance_monitor_run_benchmark(monitor);

    furi_string_cat_printf(perf_display, "Benchmark Results:\n\n");

    // Show key operations
    const char* key_ops[] = {
        "Block Read",
        "Block Write",
        "Sector Read",
        "Sector Write"
    };

    PerformanceOperation ops[] = {
        PerfOpBlockRead,
        PerfOpBlockWrite,
        PerfOpSectorRead,
        PerfOpSectorWrite
    };

    for(size_t i = 0; i < 4; i++) {
        const OperationMetrics* metrics = performance_monitor_get_metrics(monitor, ops[i]);

        if(metrics && metrics->count > 0) {
            furi_string_cat_printf(perf_display, "%s:\n", key_ops[i]);
            furi_string_cat_printf(perf_display, "  Avg: %lu ms\n", metrics->avg_time_ms);
            furi_string_cat_printf(perf_display, "  Min: %lu ms\n", metrics->min_time_ms);
            furi_string_cat_printf(perf_display, "  Max: %lu ms\n", metrics->max_time_ms);
            furi_string_cat_printf(perf_display, "  Count: %lu\n", metrics->count);

            // Calculate throughput for read/write ops
            if(i < 2) { // Block operations (16 bytes)
                uint32_t throughput = performance_monitor_calculate_throughput(
                    16 * metrics->count,
                    metrics->total_time_ms);
                furi_string_cat_printf(perf_display, "  ~%lu B/s\n", throughput);
            } else { // Sector operations (64 bytes)
                uint32_t throughput = performance_monitor_calculate_throughput(
                    64 * metrics->count,
                    metrics->total_time_ms);
                furi_string_cat_printf(perf_display, "  ~%lu B/s\n", throughput);
            }

            furi_string_cat_printf(perf_display, "\n");
        }
    }

    // Realtime metrics
    const RealtimeMetrics* realtime = performance_monitor_get_realtime(monitor);

    furi_string_cat_printf(perf_display, "=== REALTIME ===\n");
    furi_string_cat_printf(perf_display, "Ops/sec: %lu\n", realtime->current_ops_per_second);
    furi_string_cat_printf(perf_display, "Peak: %lu ops/s\n", realtime->peak_ops_per_second);
    furi_string_cat_printf(perf_display, "Bytes/sec: %lu\n", realtime->bytes_per_second);
    furi_string_cat_printf(perf_display, "Total: %lu bytes\n\n", realtime->bytes_transferred);

    // Overall statistics
    furi_string_cat_printf(perf_display, "=== OVERALL ===\n");
    furi_string_cat_printf(
        perf_display,
        "Operations: %lu\n",
        performance_monitor_get_total_operations(monitor));
    furi_string_cat_printf(
        perf_display,
        "Success: %.1f%%\n",
        performance_monitor_get_overall_success_rate(monitor));
    furi_string_cat_printf(
        perf_display,
        "Time: %lu ms\n",
        performance_monitor_get_total_time_ms(monitor));

    performance_monitor_end_session(monitor);
    furi_string_cat_printf(
        perf_display,
        "Session: %lu ms\n",
        performance_monitor_get_session_duration(monitor));

    // Check alerts
    PerformanceAlert alert = performance_monitor_check_alerts(monitor);
    if(alert != PerfAlertNone) {
        furi_string_cat_printf(
            perf_display,
            "\n⚠ %s\n",
            performance_monitor_get_alert_message(alert));
    }

    furi_string_cat_printf(perf_display, "\nPress OK to continue");

    widget_add_text_scroll_element(
        widget, 0, 0, 128, 64, furi_string_get_cstr(perf_display));

    furi_string_free(perf_display);
    performance_monitor_free(monitor);

    sound_effects_success();

    view_dispatcher_switch_to_view(app->view_dispatcher, ChameleonViewWidget);
}

bool chameleon_scene_performance_on_event(void* context, SceneManagerEvent event) {
    UNUSED(context);
    UNUSED(event);
    return false;
}

void chameleon_scene_performance_on_exit(void* context) {
    ChameleonApp* app = context;
    widget_reset(app->widget);
}
