#include "performance_monitor.h"
#include <string.h>
#include <stdio.h>
#include <furi.h>
#include <storage/storage.h>

struct PerformanceMonitor {
    OperationMetrics metrics[PerfOpCount];
    RealtimeMetrics realtime;

    PerformanceHistoryEntry history[MAX_HISTORY_ENTRIES];
    size_t history_count;
    size_t history_index;

    PerformanceOperation current_operation;
    uint32_t operation_start_time;
    bool operation_in_progress;

    uint32_t session_start_time;
    uint32_t session_end_time;
    bool session_active;

    uint32_t total_operations;
    uint32_t total_failures;

    FuriMutex* mutex;
};

static const char* operation_names[] = {
    "Connect",
    "Disconnect",
    "Slot Switch",
    "Slot Read",
    "Slot Write",
    "Tag Read",
    "Tag Write",
    "Tag Scan",
    "Key Test",
    "Authentication",
    "Block Read",
    "Block Write",
    "Sector Read",
    "Sector Write",
    "Full Read",
    "Full Write",
};

PerformanceMonitor* performance_monitor_alloc(void) {
    PerformanceMonitor* monitor = malloc(sizeof(PerformanceMonitor));
    memset(monitor, 0, sizeof(PerformanceMonitor));

    monitor->mutex = furi_mutex_alloc(FuriMutexTypeNormal);

    // Initialize min values
    for(size_t i = 0; i < PerfOpCount; i++) {
        monitor->metrics[i].min_time_ms = UINT32_MAX;
    }

    return monitor;
}

void performance_monitor_free(PerformanceMonitor* monitor) {
    if(!monitor) return;

    furi_mutex_free(monitor->mutex);
    free(monitor);
}

void performance_monitor_start_operation(
    PerformanceMonitor* monitor,
    PerformanceOperation operation) {
    if(!monitor || operation >= PerfOpCount) return;

    furi_mutex_acquire(monitor->mutex, FuriWaitForever);

    monitor->current_operation = operation;
    monitor->operation_start_time = furi_get_tick();
    monitor->operation_in_progress = true;

    furi_mutex_release(monitor->mutex);
}

void performance_monitor_end_operation(
    PerformanceMonitor* monitor,
    bool success,
    uint32_t bytes_transferred) {
    if(!monitor || !monitor->operation_in_progress) return;

    uint32_t duration_ms = furi_get_tick() - monitor->operation_start_time;

    performance_monitor_record_operation(
        monitor,
        monitor->current_operation,
        duration_ms,
        success,
        bytes_transferred);

    monitor->operation_in_progress = false;
}

void performance_monitor_record_operation(
    PerformanceMonitor* monitor,
    PerformanceOperation operation,
    uint32_t duration_ms,
    bool success,
    uint32_t bytes_transferred) {
    if(!monitor || operation >= PerfOpCount) return;

    furi_mutex_acquire(monitor->mutex, FuriWaitForever);

    OperationMetrics* metrics = &monitor->metrics[operation];

    // Update metrics
    metrics->count++;
    metrics->total_time_ms += duration_ms;
    metrics->last_time_ms = duration_ms;

    if(duration_ms < metrics->min_time_ms) {
        metrics->min_time_ms = duration_ms;
    }

    if(duration_ms > metrics->max_time_ms) {
        metrics->max_time_ms = duration_ms;
    }

    if(metrics->count > 0) {
        metrics->avg_time_ms = metrics->total_time_ms / metrics->count;
    }

    if(!success) {
        metrics->failures++;
        monitor->total_failures++;
    }

    if(metrics->count > 0) {
        metrics->success_rate =
            ((metrics->count - metrics->failures) * 100.0f) / metrics->count;
    }

    // Update totals
    monitor->total_operations++;

    // Add to history
    PerformanceHistoryEntry* entry = &monitor->history[monitor->history_index];
    entry->timestamp = furi_get_tick();
    entry->operation = operation;
    entry->duration_ms = duration_ms;
    entry->success = success;
    entry->bytes_transferred = bytes_transferred;

    monitor->history_index = (monitor->history_index + 1) % MAX_HISTORY_ENTRIES;
    if(monitor->history_count < MAX_HISTORY_ENTRIES) {
        monitor->history_count++;
    }

    // Update realtime metrics
    monitor->realtime.bytes_transferred += bytes_transferred;

    // Calculate ops/sec and bytes/sec (based on last second of operations)
    uint32_t current_time = furi_get_tick();
    uint32_t ops_in_last_second = 0;
    uint32_t bytes_in_last_second = 0;

    for(size_t i = 0; i < monitor->history_count; i++) {
        const PerformanceHistoryEntry* h = &monitor->history[i];
        if(current_time - h->timestamp < 1000) {
            ops_in_last_second++;
            bytes_in_last_second += h->bytes_transferred;
        }
    }

    monitor->realtime.current_ops_per_second = ops_in_last_second;
    monitor->realtime.bytes_per_second = bytes_in_last_second;

    if(monitor->realtime.current_ops_per_second > monitor->realtime.peak_ops_per_second) {
        monitor->realtime.peak_ops_per_second = monitor->realtime.current_ops_per_second;
    }

    if(monitor->realtime.bytes_per_second > monitor->realtime.peak_bytes_per_second) {
        monitor->realtime.peak_bytes_per_second = monitor->realtime.bytes_per_second;
    }

    furi_mutex_release(monitor->mutex);
}

const OperationMetrics* performance_monitor_get_metrics(
    PerformanceMonitor* monitor,
    PerformanceOperation operation) {
    if(!monitor || operation >= PerfOpCount) return NULL;
    return &monitor->metrics[operation];
}

const RealtimeMetrics* performance_monitor_get_realtime(PerformanceMonitor* monitor) {
    if(!monitor) return NULL;
    return &monitor->realtime;
}

size_t performance_monitor_get_history(
    PerformanceMonitor* monitor,
    PerformanceHistoryEntry* entries,
    size_t max_entries) {
    if(!monitor || !entries) return 0;

    furi_mutex_acquire(monitor->mutex, FuriWaitForever);

    size_t count = (monitor->history_count < max_entries) ? monitor->history_count : max_entries;

    // Copy most recent entries
    size_t start_idx = (monitor->history_index >= count) ?
        (monitor->history_index - count) : 0;

    for(size_t i = 0; i < count; i++) {
        size_t idx = (start_idx + i) % MAX_HISTORY_ENTRIES;
        memcpy(&entries[i], &monitor->history[idx], sizeof(PerformanceHistoryEntry));
    }

    furi_mutex_release(monitor->mutex);

    return count;
}

const PerformanceHistoryEntry* performance_monitor_get_last_entry(PerformanceMonitor* monitor) {
    if(!monitor || monitor->history_count == 0) return NULL;

    size_t last_idx = (monitor->history_index == 0) ?
        (MAX_HISTORY_ENTRIES - 1) : (monitor->history_index - 1);

    return &monitor->history[last_idx];
}

uint32_t performance_monitor_get_total_operations(PerformanceMonitor* monitor) {
    if(!monitor) return 0;
    return monitor->total_operations;
}

uint32_t performance_monitor_get_total_time_ms(PerformanceMonitor* monitor) {
    if(!monitor) return 0;

    uint32_t total = 0;
    for(size_t i = 0; i < PerfOpCount; i++) {
        total += monitor->metrics[i].total_time_ms;
    }

    return total;
}

uint32_t performance_monitor_get_total_bytes(PerformanceMonitor* monitor) {
    if(!monitor) return 0;
    return monitor->realtime.bytes_transferred;
}

float performance_monitor_get_overall_success_rate(PerformanceMonitor* monitor) {
    if(!monitor || monitor->total_operations == 0) return 0.0f;

    uint32_t successes = monitor->total_operations - monitor->total_failures;
    return (successes * 100.0f) / monitor->total_operations;
}

void performance_monitor_start_session(PerformanceMonitor* monitor) {
    if(!monitor) return;

    furi_mutex_acquire(monitor->mutex, FuriWaitForever);

    monitor->session_start_time = furi_get_tick();
    monitor->session_active = true;

    furi_mutex_release(monitor->mutex);
}

void performance_monitor_end_session(PerformanceMonitor* monitor) {
    if(!monitor) return;

    furi_mutex_acquire(monitor->mutex, FuriWaitForever);

    monitor->session_end_time = furi_get_tick();
    monitor->session_active = false;

    furi_mutex_release(monitor->mutex);
}

uint32_t performance_monitor_get_session_duration(PerformanceMonitor* monitor) {
    if(!monitor) return 0;

    if(monitor->session_active) {
        return furi_get_tick() - monitor->session_start_time;
    } else {
        return monitor->session_end_time - monitor->session_start_time;
    }
}

void performance_monitor_reset_metrics(PerformanceMonitor* monitor) {
    if(!monitor) return;

    furi_mutex_acquire(monitor->mutex, FuriWaitForever);

    memset(monitor->metrics, 0, sizeof(monitor->metrics));
    for(size_t i = 0; i < PerfOpCount; i++) {
        monitor->metrics[i].min_time_ms = UINT32_MAX;
    }

    monitor->total_operations = 0;
    monitor->total_failures = 0;

    furi_mutex_release(monitor->mutex);
}

void performance_monitor_clear_history(PerformanceMonitor* monitor) {
    if(!monitor) return;

    furi_mutex_acquire(monitor->mutex, FuriWaitForever);

    memset(monitor->history, 0, sizeof(monitor->history));
    monitor->history_count = 0;
    monitor->history_index = 0;

    furi_mutex_release(monitor->mutex);
}

void performance_monitor_reset_all(PerformanceMonitor* monitor) {
    if(!monitor) return;

    performance_monitor_reset_metrics(monitor);
    performance_monitor_clear_history(monitor);

    furi_mutex_acquire(monitor->mutex, FuriWaitForever);
    memset(&monitor->realtime, 0, sizeof(RealtimeMetrics));
    furi_mutex_release(monitor->mutex);
}

PerformanceAlert performance_monitor_check_alerts(PerformanceMonitor* monitor) {
    if(!monitor) return PerfAlertNone;

    // Check for slow operations
    for(size_t i = 0; i < PerfOpCount; i++) {
        if(monitor->metrics[i].avg_time_ms > 5000) {
            return PerfAlertSlowOperation;
        }
    }

    // Check for high latency
    if(monitor->metrics[PerfOpBlockRead].avg_time_ms > 100) {
        return PerfAlertHighLatency;
    }

    // Check for frequent failures
    float overall_success = performance_monitor_get_overall_success_rate(monitor);
    if(overall_success < 80.0f && monitor->total_operations > 10) {
        return PerfAlertFrequentFailures;
    }

    return PerfAlertNone;
}

const char* performance_monitor_get_alert_message(PerformanceAlert alert) {
    static const char* messages[] = {
        "No alerts",
        "Slow operation detected",
        "High latency detected",
        "Frequent failures",
        "Memory warning",
    };

    if(alert < sizeof(messages) / sizeof(messages[0])) {
        return messages[alert];
    }

    return "Unknown alert";
}

bool performance_monitor_export_csv(PerformanceMonitor* monitor, const char* filepath) {
    if(!monitor || !filepath) return false;

    Storage* storage = furi_record_open(RECORD_STORAGE);
    File* file = storage_file_alloc(storage);

    bool success = false;

    if(storage_file_open(file, filepath, FSAM_WRITE, FSOM_CREATE_ALWAYS)) {
        // CSV header
        const char* header = "Timestamp,Operation,Duration(ms),Success,Bytes\n";
        storage_file_write(file, header, strlen(header));

        // Write history entries
        for(size_t i = 0; i < monitor->history_count; i++) {
            const PerformanceHistoryEntry* entry = &monitor->history[i];

            char line[256];
            snprintf(line, sizeof(line), "%lu,%s,%lu,%d,%lu\n",
                entry->timestamp,
                performance_monitor_get_operation_name(entry->operation),
                entry->duration_ms,
                entry->success ? 1 : 0,
                entry->bytes_transferred);

            storage_file_write(file, line, strlen(line));
        }

        success = true;
        storage_file_close(file);
    }

    storage_file_free(file);
    furi_record_close(RECORD_STORAGE);

    return success;
}

const char* performance_monitor_get_operation_name(PerformanceOperation operation) {
    if(operation < PerfOpCount) {
        return operation_names[operation];
    }
    return "Unknown";
}

uint32_t performance_monitor_calculate_throughput(uint32_t bytes, uint32_t time_ms) {
    if(time_ms == 0) return 0;
    return (bytes * 1000) / time_ms; // bytes per second
}

void performance_monitor_run_benchmark(PerformanceMonitor* monitor) {
    if(!monitor) return;

    // Simulate various operations for benchmark
    for(int i = 0; i < 10; i++) {
        performance_monitor_record_operation(
            monitor, PerfOpBlockRead, 50 + (i * 5), true, 16);
        performance_monitor_record_operation(
            monitor, PerfOpBlockWrite, 80 + (i * 8), true, 16);
    }

    for(int i = 0; i < 5; i++) {
        performance_monitor_record_operation(
            monitor, PerfOpSectorRead, 200 + (i * 20), true, 64);
        performance_monitor_record_operation(
            monitor, PerfOpSectorWrite, 320 + (i * 32), true, 64);
    }
}
