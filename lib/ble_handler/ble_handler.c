#include "ble_handler.h"
#include <furi.h>
#include <furi_hal.h>
#include <furi_hal_bt.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#define TAG "BleHandler"
#define MAX_DEVICES 10
#define DEVICE_NAME_MAX_LEN 32

// ============================================================================
// Nordic UART Service (NUS) UUIDs - Used by Chameleon Ultra
// ============================================================================
// Service UUID:        6E400001-B5A3-F393-E0A9-E50E24DCCA9E
// RX Characteristic:   6E400002-B5A3-F393-E0A9-E50E24DCCA9E (Write to device)
// TX Characteristic:   6E400003-B5A3-F393-E0A9-E50E24DCCA9E (Notifications from device)
//
// IMPORTANT: This implementation is currently a STUB.
// Flipper Zero's public API does not support BLE Central mode (scanning/connecting).
// See docs/BLE_LIMITATIONS.md for details.
// For now, USE USB CONNECTION which is fully functional.
// ============================================================================

#define NUS_SERVICE_UUID "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
#define NUS_RX_CHAR_UUID "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define NUS_TX_CHAR_UUID "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"

typedef struct {
    char name[DEVICE_NAME_MAX_LEN];
    uint8_t mac[6];
    int8_t rssi;
} BleDevice;

struct BleHandler {
    BleStatus status;
    BleHandlerRxCallback rx_callback;
    void* rx_context;
    BleHandlerStatusCallback status_callback;
    void* status_context;

    BleDevice devices[MAX_DEVICES];
    size_t device_count;

    bool initialized;
    FuriThread* scan_thread;
    bool scanning;
};

BleHandler* ble_handler_alloc() {
    BleHandler* handler = malloc(sizeof(BleHandler));
    memset(handler, 0, sizeof(BleHandler));
    handler->status = BleStatusDisconnected;
    return handler;
}

void ble_handler_free(BleHandler* handler) {
    furi_assert(handler);

    if(handler->initialized) {
        ble_handler_deinit(handler);
    }

    free(handler);
}

bool ble_handler_init(BleHandler* handler) {
    furi_assert(handler);

    if(handler->initialized) {
        FURI_LOG_W(TAG, "Already initialized");
        return true;
    }

    FURI_LOG_I(TAG, "Initializing BLE (STUB MODE - See docs/BLE_LIMITATIONS.md)");

    // STUB IMPLEMENTATION
    // Flipper Zero's public API (furi_hal_bt) only supports BLE Peripheral mode.
    // BLE Central mode (scanning/connecting to external devices) is not available.
    // This is a framework ready for when BLE Central API becomes available.
    //
    // Required for full implementation:
    // 1. BLE GAP scanning for devices advertising NUS service
    // 2. GATT service discovery to find characteristics
    // 3. Subscribe to TX characteristic (6E400003...) for notifications
    // 4. Write to RX characteristic (6E400002...) to send commands
    //
    // For now, this returns success but all operations are simulated.

    handler->initialized = true;
    handler->status = BleStatusDisconnected;

    FURI_LOG_W(TAG, "BLE initialized in STUB mode - USB connection recommended");
    return true;
}

void ble_handler_deinit(BleHandler* handler) {
    furi_assert(handler);

    if(!handler->initialized) {
        return;
    }

    ble_handler_disconnect(handler);
    ble_handler_stop_scan(handler);

    handler->initialized = false;

    FURI_LOG_I(TAG, "BLE deinitialized");
}

void ble_handler_set_rx_callback(
    BleHandler* handler,
    BleHandlerRxCallback callback,
    void* context) {
    furi_assert(handler);
    handler->rx_callback = callback;
    handler->rx_context = context;
}

void ble_handler_set_status_callback(
    BleHandler* handler,
    BleHandlerStatusCallback callback,
    void* context) {
    furi_assert(handler);
    handler->status_callback = callback;
    handler->status_context = context;
}

static int32_t ble_scan_thread(void* context) {
    BleHandler* handler = context;

    FURI_LOG_I(TAG, "Scan thread started");

    // Simulate scanning for Chameleon Ultra devices
    // In a real implementation, this would use BLE GAP scanning

    handler->device_count = 0;

    // Simulate finding a device (for testing)
    // TODO: Implement actual BLE scanning
    furi_delay_ms(2000);

    if(handler->scanning) {
        snprintf(handler->devices[0].name, DEVICE_NAME_MAX_LEN, "Chameleon Ultra");
        handler->devices[0].rssi = -60;
        handler->device_count = 1;
        FURI_LOG_I(TAG, "Found device: %s", handler->devices[0].name);
    }

    FURI_LOG_I(TAG, "Scan thread finished");
    return 0;
}

bool ble_handler_start_scan(BleHandler* handler) {
    furi_assert(handler);

    if(!handler->initialized) {
        FURI_LOG_E(TAG, "Not initialized");
        return false;
    }

    if(handler->scanning) {
        FURI_LOG_W(TAG, "Already scanning");
        return false;
    }

    FURI_LOG_I(TAG, "Starting BLE scan");

    handler->scanning = true;
    handler->device_count = 0;
    handler->status = BleStatusScanning;

    if(handler->status_callback) {
        handler->status_callback(handler->status, handler->status_context);
    }

    handler->scan_thread = furi_thread_alloc();
    furi_thread_set_name(handler->scan_thread, "BleScanThread");
    furi_thread_set_stack_size(handler->scan_thread, 2048);
    furi_thread_set_context(handler->scan_thread, handler);
    furi_thread_set_callback(handler->scan_thread, ble_scan_thread);
    furi_thread_start(handler->scan_thread);

    return true;
}

void ble_handler_stop_scan(BleHandler* handler) {
    furi_assert(handler);

    if(!handler->scanning) {
        return;
    }

    FURI_LOG_I(TAG, "Stopping BLE scan");

    handler->scanning = false;

    if(handler->scan_thread) {
        furi_thread_join(handler->scan_thread);
        furi_thread_free(handler->scan_thread);
        handler->scan_thread = NULL;
    }

    handler->status = BleStatusDisconnected;

    if(handler->status_callback) {
        handler->status_callback(handler->status, handler->status_context);
    }
}

size_t ble_handler_get_device_count(BleHandler* handler) {
    furi_assert(handler);
    return handler->device_count;
}

const char* ble_handler_get_device_name(BleHandler* handler, size_t index) {
    furi_assert(handler);

    if(index >= handler->device_count) {
        return NULL;
    }

    return handler->devices[index].name;
}

bool ble_handler_connect(BleHandler* handler, size_t device_index) {
    furi_assert(handler);

    if(!handler->initialized) {
        FURI_LOG_E(TAG, "Not initialized");
        return false;
    }

    if(device_index >= handler->device_count) {
        FURI_LOG_E(TAG, "Invalid device index: %zu", device_index);
        return false;
    }

    FURI_LOG_I(TAG, "Connecting to: %s", handler->devices[device_index].name);

    handler->status = BleStatusConnecting;

    if(handler->status_callback) {
        handler->status_callback(handler->status, handler->status_context);
    }

    // Simulate connection
    // TODO: Implement actual BLE connection with GATT services
    furi_delay_ms(1000);

    handler->status = BleStatusConnected;

    if(handler->status_callback) {
        handler->status_callback(handler->status, handler->status_context);
    }

    FURI_LOG_I(TAG, "Connected to: %s", handler->devices[device_index].name);

    return true;
}

void ble_handler_disconnect(BleHandler* handler) {
    furi_assert(handler);

    if(handler->status != BleStatusConnected) {
        return;
    }

    FURI_LOG_I(TAG, "Disconnecting");

    // TODO: Implement actual BLE disconnection

    handler->status = BleStatusDisconnected;

    if(handler->status_callback) {
        handler->status_callback(handler->status, handler->status_context);
    }

    FURI_LOG_I(TAG, "Disconnected");
}

bool ble_handler_send(BleHandler* handler, const uint8_t* data, size_t length) {
    furi_assert(handler);
    furi_assert(data);

    if(handler->status != BleStatusConnected) {
        FURI_LOG_E(TAG, "Not connected");
        return false;
    }

    FURI_LOG_D(TAG, "Sending %zu bytes via BLE", length);

    // TODO: Implement actual BLE data transmission via GATT characteristic

    return true;
}

BleStatus ble_handler_get_status(BleHandler* handler) {
    furi_assert(handler);
    return handler->status;
}

bool ble_handler_is_connected(BleHandler* handler) {
    furi_assert(handler);
    return (handler->status == BleStatusConnected);
}
