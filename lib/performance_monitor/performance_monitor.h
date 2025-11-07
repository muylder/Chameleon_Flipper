#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// Operation types for monitoring
typedef enum {
    PerfOpConnect = 0,
    PerfOpDisconnect,
    PerfOpSlotSwitch,
    PerfOpSlotRead,
    PerfOpSlotWrite,
    PerfOpTagRead,
    PerfOpTagWrite,
    PerfOpTagScan,
    PerfOpKeyTest,
    PerfOpAuthentication,
    PerfOpBlockRead,
    PerfOpBlockWrite,
    PerfOpSectorRead,
    PerfOpSectorWrite,
    PerfOpFullRead,
    PerfOpFullWrite,
    PerfOpCount, // Must be last
} PerformanceOperation;

// Performance metrics for a single operation
typedef struct {
    uint32_t count;
    uint32_t total_time_ms;
    uint32_t min_time_ms;
    uint32_t max_time_ms;
    uint32_t avg_time_ms;
    uint32_t last_time_ms;
    uint32_t failures;
    float success_rate;
} OperationMetrics;

// Real-time performance data
typedef struct {
    uint32_t current_ops_per_second;
    uint32_t peak_ops_per_second;
    uint32_t bytes_transferred;
    uint32_t bytes_per_second;
    uint32_t peak_bytes_per_second;
    float cpu_usage_percent;
    uint32_t memory_used_bytes;
} RealtimeMetrics;

// Performance history entry
#define MAX_HISTORY_ENTRIES 100

typedef struct {
    uint32_t timestamp;
    PerformanceOperation operation;
    uint32_t duration_ms;
    bool success;
    uint32_t bytes_transferred;
} PerformanceHistoryEntry;

// Performance alerts/warnings
typedef enum {
    PerfAlertNone = 0,
    PerfAlertSlowOperation,
    PerfAlertHighLatency,
    PerfAlertFrequentFailures,
    PerfAlertMemoryWarning,
} PerformanceAlert;

// Performance Monitor manager
typedef struct PerformanceMonitor PerformanceMonitor;

// Allocation and deallocation
PerformanceMonitor* performance_monitor_alloc(void);
void performance_monitor_free(PerformanceMonitor* monitor);

// Recording operations
void performance_monitor_start_operation(
    PerformanceMonitor* monitor,
    PerformanceOperation operation);

void performance_monitor_end_operation(
    PerformanceMonitor* monitor,
    bool success,
    uint32_t bytes_transferred);

void performance_monitor_record_operation(
    PerformanceMonitor* monitor,
    PerformanceOperation operation,
    uint32_t duration_ms,
    bool success,
    uint32_t bytes_transferred);

// Metrics retrieval
const OperationMetrics* performance_monitor_get_metrics(
    PerformanceMonitor* monitor,
    PerformanceOperation operation);

const RealtimeMetrics* performance_monitor_get_realtime(PerformanceMonitor* monitor);

// History management
size_t performance_monitor_get_history(
    PerformanceMonitor* monitor,
    PerformanceHistoryEntry* entries,
    size_t max_entries);

const PerformanceHistoryEntry* performance_monitor_get_last_entry(PerformanceMonitor* monitor);

// Statistics
uint32_t performance_monitor_get_total_operations(PerformanceMonitor* monitor);
uint32_t performance_monitor_get_total_time_ms(PerformanceMonitor* monitor);
uint32_t performance_monitor_get_total_bytes(PerformanceMonitor* monitor);
float performance_monitor_get_overall_success_rate(PerformanceMonitor* monitor);

// Session management
void performance_monitor_start_session(PerformanceMonitor* monitor);
void performance_monitor_end_session(PerformanceMonitor* monitor);
uint32_t performance_monitor_get_session_duration(PerformanceMonitor* monitor);

// Reset and clear
void performance_monitor_reset_metrics(PerformanceMonitor* monitor);
void performance_monitor_clear_history(PerformanceMonitor* monitor);
void performance_monitor_reset_all(PerformanceMonitor* monitor);

// Alerts
PerformanceAlert performance_monitor_check_alerts(PerformanceMonitor* monitor);
const char* performance_monitor_get_alert_message(PerformanceAlert alert);

// Persistence
bool performance_monitor_save(PerformanceMonitor* monitor, const char* filepath);
bool performance_monitor_load(PerformanceMonitor* monitor, const char* filepath);
bool performance_monitor_export_csv(PerformanceMonitor* monitor, const char* filepath);

// Utilities
const char* performance_monitor_get_operation_name(PerformanceOperation operation);
uint32_t performance_monitor_calculate_throughput(uint32_t bytes, uint32_t time_ms);

// Benchmarking
void performance_monitor_run_benchmark(PerformanceMonitor* monitor);
