#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <furi.h>

/**
 * @brief NFC Wireless Bridge Protocol
 *
 * Enables wireless communication between Flipper Zero and Chameleon Ultra via NFC.
 * The Chameleon emulates an NTAG tag, and the Flipper reads/writes commands to it.
 *
 * Protocol uses NTAG215/216 pages for command/response exchange.
 */

// NFC Bridge protocol constants
#define NFC_BRIDGE_PAGE_CMD 16      // Page for command (block 16)
#define NFC_BRIDGE_PAGE_DATA 17     // Page for command data (block 17+)
#define NFC_BRIDGE_PAGE_RESPONSE 32 // Page for response (block 32)
#define NFC_BRIDGE_PAGE_STATUS 33   // Page for status (block 33)

#define NFC_BRIDGE_MAX_DATA_SIZE 64 // Max data payload
#define NFC_BRIDGE_MAGIC 0xC4       // Magic byte (0xC4 = Chameleon)

// Command IDs for NFC Bridge
typedef enum {
    NfcBridgeCmdPing = 0x01,          // Test connectivity
    NfcBridgeCmdGetStatus = 0x02,     // Get device status
    NfcBridgeCmdSwitchSlot = 0x03,    // Switch active slot
    NfcBridgeCmdGetSlotInfo = 0x04,   // Get slot information
    NfcBridgeCmdSetMode = 0x05,       // Set device mode (reader/emulator)
    NfcBridgeCmdReadUID = 0x06,       // Read UID from active slot
    NfcBridgeCmdWriteUID = 0x07,      // Write UID to slot
    NfcBridgeCmdEnableSlot = 0x08,    // Enable/disable HF/LF
    NfcBridgeCmdReboot = 0x0F,        // Reboot device
} NfcBridgeCommand;

// Response status codes
typedef enum {
    NfcBridgeStatusSuccess = 0x00,
    NfcBridgeStatusError = 0x01,
    NfcBridgeStatusInvalidCmd = 0x02,
    NfcBridgeStatusBusy = 0x03,
    NfcBridgeStatusTimeout = 0x04,
} NfcBridgeStatus;

// Command frame structure (written to NTAG pages)
typedef struct {
    uint8_t magic;   // Magic byte (0xC4)
    uint8_t cmd_id;  // Command ID
    uint8_t seq;     // Sequence number
    uint8_t len;     // Data length
    uint8_t data[NFC_BRIDGE_MAX_DATA_SIZE]; // Command data
    uint8_t checksum; // Simple XOR checksum
} __attribute__((packed)) NfcBridgeCommandFrame;

// Response frame structure (read from NTAG pages)
typedef struct {
    uint8_t magic;    // Magic byte (0xC4)
    uint8_t status;   // Status code
    uint8_t seq;      // Sequence number (matches command)
    uint8_t len;      // Response data length
    uint8_t data[NFC_BRIDGE_MAX_DATA_SIZE]; // Response data
    uint8_t checksum; // Simple XOR checksum
} __attribute__((packed)) NfcBridgeResponseFrame;

// NFC Bridge instance
typedef struct NfcBridge NfcBridge;

/**
 * @brief Allocate NFC Bridge
 * @return NfcBridge instance
 */
NfcBridge* nfc_bridge_alloc(void);

/**
 * @brief Free NFC Bridge
 * @param bridge Bridge instance
 */
void nfc_bridge_free(NfcBridge* bridge);

/**
 * @brief Initialize NFC for bridge mode
 * @param bridge Bridge instance
 * @return true if initialized
 */
bool nfc_bridge_init(NfcBridge* bridge);

/**
 * @brief Deinitialize NFC bridge
 * @param bridge Bridge instance
 */
void nfc_bridge_deinit(NfcBridge* bridge);

/**
 * @brief Send command via NFC
 * @param bridge Bridge instance
 * @param cmd_id Command ID
 * @param data Command data
 * @param data_len Data length
 * @param response Output response frame
 * @param timeout_ms Timeout in milliseconds
 * @return true if command sent and response received
 */
bool nfc_bridge_send_command(
    NfcBridge* bridge,
    NfcBridgeCommand cmd_id,
    const uint8_t* data,
    uint8_t data_len,
    NfcBridgeResponseFrame* response,
    uint32_t timeout_ms);

/**
 * @brief Ping Chameleon Ultra via NFC
 * @param bridge Bridge instance
 * @return true if ping successful
 */
bool nfc_bridge_ping(NfcBridge* bridge);

/**
 * @brief Get Chameleon status via NFC
 * @param bridge Bridge instance
 * @param active_slot Output active slot number
 * @param device_mode Output device mode
 * @return true if status received
 */
bool nfc_bridge_get_status(NfcBridge* bridge, uint8_t* active_slot, uint8_t* device_mode);

/**
 * @brief Switch slot via NFC
 * @param bridge Bridge instance
 * @param slot_number Slot to switch to (0-7)
 * @return true if slot switched
 */
bool nfc_bridge_switch_slot(NfcBridge* bridge, uint8_t slot_number);

/**
 * @brief Calculate frame checksum
 * @param data Frame data
 * @param len Data length
 * @return Checksum value
 */
uint8_t nfc_bridge_calc_checksum(const uint8_t* data, size_t len);

/**
 * @brief Validate response frame
 * @param response Response frame
 * @return true if valid
 */
bool nfc_bridge_validate_response(const NfcBridgeResponseFrame* response);

/**
 * @brief Get command name
 * @param cmd_id Command ID
 * @return Command name string
 */
const char* nfc_bridge_get_command_name(NfcBridgeCommand cmd_id);

/**
 * @brief Get status name
 * @param status Status code
 * @return Status name string
 */
const char* nfc_bridge_get_status_name(NfcBridgeStatus status);
