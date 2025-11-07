#include "nfc_bridge.h"
#include <string.h>

struct NfcBridge {
    uint8_t sequence; // Command sequence number
    bool initialized;
    FuriMutex* mutex;
};

static const char* command_names[] = {
    "Invalid",
    "Ping",
    "Get Status",
    "Switch Slot",
    "Get Slot Info",
    "Set Mode",
    "Read UID",
    "Write UID",
    "Enable Slot",
    "Unknown",
    "Unknown",
    "Unknown",
    "Unknown",
    "Unknown",
    "Unknown",
    "Reboot",
};

static const char* status_names[] = {
    "Success",
    "Error",
    "Invalid Command",
    "Busy",
    "Timeout",
};

NfcBridge* nfc_bridge_alloc(void) {
    NfcBridge* bridge = malloc(sizeof(NfcBridge));
    memset(bridge, 0, sizeof(NfcBridge));

    bridge->mutex = furi_mutex_alloc(FuriMutexTypeNormal);
    bridge->sequence = 0;
    bridge->initialized = false;

    return bridge;
}

void nfc_bridge_free(NfcBridge* bridge) {
    if(!bridge) return;

    if(bridge->initialized) {
        nfc_bridge_deinit(bridge);
    }

    furi_mutex_free(bridge->mutex);
    free(bridge);
}

bool nfc_bridge_init(NfcBridge* bridge) {
    if(!bridge) return false;

    furi_mutex_acquire(bridge->mutex, FuriWaitForever);

    // TODO: Initialize Flipper Zero NFC subsystem
    // This would use: furi_record_open(RECORD_NFC)
    // and configure for NTAG emulation mode

    bridge->initialized = true;
    bridge->sequence = 0;

    furi_mutex_release(bridge->mutex);

    return true;
}

void nfc_bridge_deinit(NfcBridge* bridge) {
    if(!bridge || !bridge->initialized) return;

    furi_mutex_acquire(bridge->mutex, FuriWaitForever);

    // TODO: Deinitialize NFC subsystem
    // furi_record_close(RECORD_NFC)

    bridge->initialized = false;

    furi_mutex_release(bridge->mutex);
}

uint8_t nfc_bridge_calc_checksum(const uint8_t* data, size_t len) {
    uint8_t checksum = 0;
    for(size_t i = 0; i < len; i++) {
        checksum ^= data[i];
    }
    return checksum;
}

bool nfc_bridge_validate_response(const NfcBridgeResponseFrame* response) {
    if(!response) return false;

    // Check magic byte
    if(response->magic != NFC_BRIDGE_MAGIC) {
        return false;
    }

    // Validate checksum
    uint8_t calc_checksum = nfc_bridge_calc_checksum(
        (const uint8_t*)response,
        sizeof(NfcBridgeResponseFrame) - 1); // Exclude checksum byte

    return (calc_checksum == response->checksum);
}

bool nfc_bridge_send_command(
    NfcBridge* bridge,
    NfcBridgeCommand cmd_id,
    const uint8_t* data,
    uint8_t data_len,
    NfcBridgeResponseFrame* response,
    uint32_t timeout_ms) {
    if(!bridge || !bridge->initialized || !response) return false;
    if(data_len > NFC_BRIDGE_MAX_DATA_SIZE) return false;

    furi_mutex_acquire(bridge->mutex, FuriWaitForever);

    // Build command frame
    NfcBridgeCommandFrame cmd_frame;
    memset(&cmd_frame, 0, sizeof(cmd_frame));

    cmd_frame.magic = NFC_BRIDGE_MAGIC;
    cmd_frame.cmd_id = cmd_id;
    cmd_frame.seq = bridge->sequence++;
    cmd_frame.len = data_len;

    if(data && data_len > 0) {
        memcpy(cmd_frame.data, data, data_len);
    }

    // Calculate checksum
    cmd_frame.checksum = nfc_bridge_calc_checksum(
        (const uint8_t*)&cmd_frame,
        sizeof(cmd_frame) - 1);

    // TODO: Write command to NTAG pages via NFC
    // This would use NFC API to:
    // 1. Detect Chameleon Ultra NTAG tag
    // 2. Write cmd_frame to pages starting at NFC_BRIDGE_PAGE_CMD
    // 3. Wait for response with timeout
    // 4. Read response from pages starting at NFC_BRIDGE_PAGE_RESPONSE

    // SIMULATION: For demonstration, create a mock response
    memset(response, 0, sizeof(NfcBridgeResponseFrame));
    response->magic = NFC_BRIDGE_MAGIC;
    response->status = NfcBridgeStatusSuccess;
    response->seq = cmd_frame.seq;
    response->len = 0;

    // Example response data for specific commands
    switch(cmd_id) {
    case NfcBridgeCmdPing:
        // Ping response: "PONG"
        memcpy(response->data, "PONG", 4);
        response->len = 4;
        break;

    case NfcBridgeCmdGetStatus:
        // Status response: [active_slot, device_mode]
        response->data[0] = 0; // Active slot 0
        response->data[1] = 1; // Emulator mode
        response->len = 2;
        break;

    case NfcBridgeCmdSwitchSlot:
        // Switch slot response: [new_slot]
        response->data[0] = data ? data[0] : 0;
        response->len = 1;
        break;

    default:
        response->status = NfcBridgeStatusInvalidCmd;
        break;
    }

    // Calculate response checksum
    response->checksum = nfc_bridge_calc_checksum(
        (const uint8_t*)response,
        sizeof(NfcBridgeResponseFrame) - 1);

    furi_mutex_release(bridge->mutex);

    // Validate response
    return nfc_bridge_validate_response(response);
}

bool nfc_bridge_ping(NfcBridge* bridge) {
    if(!bridge) return false;

    NfcBridgeResponseFrame response;
    if(!nfc_bridge_send_command(bridge, NfcBridgeCmdPing, NULL, 0, &response, 1000)) {
        return false;
    }

    if(response.status != NfcBridgeStatusSuccess) {
        return false;
    }

    // Check for "PONG" response
    if(response.len == 4 && memcmp(response.data, "PONG", 4) == 0) {
        return true;
    }

    return false;
}

bool nfc_bridge_get_status(NfcBridge* bridge, uint8_t* active_slot, uint8_t* device_mode) {
    if(!bridge || !active_slot || !device_mode) return false;

    NfcBridgeResponseFrame response;
    if(!nfc_bridge_send_command(bridge, NfcBridgeCmdGetStatus, NULL, 0, &response, 1000)) {
        return false;
    }

    if(response.status != NfcBridgeStatusSuccess || response.len < 2) {
        return false;
    }

    *active_slot = response.data[0];
    *device_mode = response.data[1];

    return true;
}

bool nfc_bridge_switch_slot(NfcBridge* bridge, uint8_t slot_number) {
    if(!bridge || slot_number > 7) return false;

    NfcBridgeResponseFrame response;
    if(!nfc_bridge_send_command(
           bridge,
           NfcBridgeCmdSwitchSlot,
           &slot_number,
           1,
           &response,
           1000)) {
        return false;
    }

    if(response.status != NfcBridgeStatusSuccess) {
        return false;
    }

    // Verify slot switched
    if(response.len >= 1 && response.data[0] == slot_number) {
        return true;
    }

    return false;
}

const char* nfc_bridge_get_command_name(NfcBridgeCommand cmd_id) {
    if(cmd_id >= 0 && cmd_id < sizeof(command_names) / sizeof(command_names[0])) {
        return command_names[cmd_id];
    }
    return "Unknown";
}

const char* nfc_bridge_get_status_name(NfcBridgeStatus status) {
    if(status >= 0 && status < sizeof(status_names) / sizeof(status_names[0])) {
        return status_names[status];
    }
    return "Unknown";
}
