#pragma once

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief UID Generator and Calculator
 *
 * Generates valid UIDs with correct BCC for MIFARE tags
 */

// UID types
typedef enum {
    UidType4Byte,     // MIFARE Classic (4 bytes + 1 BCC)
    UidType7Byte,     // MIFARE Ultralight/DESFire (7 bytes + 2 BCC)
    UidType10Byte,    // Extended UID (10 bytes + 2 BCC)
} UidType;

// UID structure
typedef struct {
    uint8_t bytes[10];
    uint8_t length;      // Actual UID bytes (without BCC)
    uint8_t bcc0;        // First BCC byte
    uint8_t bcc1;        // Second BCC byte (for 7/10 byte UIDs)
    UidType type;
    bool is_valid;
} Uid;

/**
 * @brief Calculate BCC (Block Check Character) for UID
 * @param uid UID bytes
 * @param length UID length
 * @return BCC value
 */
uint8_t uid_calculate_bcc(const uint8_t* uid, uint8_t length);

/**
 * @brief Validate UID and BCC
 * @param uid UID structure
 * @return true if UID and BCC are valid
 */
bool uid_validate(const Uid* uid);

/**
 * @brief Generate random UID
 * @param type UID type (4, 7, or 10 bytes)
 * @param uid Output UID structure
 * @return true if generated successfully
 */
bool uid_generate_random(UidType type, Uid* uid);

/**
 * @brief Generate UID from hex string
 * @param hex_string Hex string (e.g. "04ABCDEF")
 * @param uid Output UID structure
 * @return true if parsed successfully
 */
bool uid_from_hex_string(const char* hex_string, Uid* uid);

/**
 * @brief Convert UID to hex string
 * @param uid UID structure
 * @param output Output buffer (min 21 bytes for 10-byte UID + BCC)
 * @param include_bcc Include BCC bytes in output
 * @return Number of characters written
 */
size_t uid_to_hex_string(const Uid* uid, char* output, bool include_bcc);

/**
 * @brief Set UID bytes and auto-calculate BCC
 * @param uid_bytes UID bytes (without BCC)
 * @param length UID length (4, 7, or 10)
 * @param uid Output UID structure
 * @return true if successful
 */
bool uid_set_bytes(const uint8_t* uid_bytes, uint8_t length, Uid* uid);

/**
 * @brief Check if UID is manufacturer-locked
 * @param uid UID structure
 * @return true if UID follows manufacturer format
 */
bool uid_is_manufacturer_format(const Uid* uid);

/**
 * @brief Get UID type name
 * @param type UID type
 * @return Type name string
 */
const char* uid_get_type_name(UidType type);

/**
 * @brief Parse UID type from length
 * @param length UID length
 * @return UID type
 */
UidType uid_get_type_from_length(uint8_t length);

/**
 * @brief Generate batch of random UIDs
 * @param type UID type
 * @param uids Output array
 * @param count Number of UIDs to generate
 * @return Number of UIDs generated
 */
size_t uid_generate_batch(UidType type, Uid* uids, size_t count);

/**
 * @brief Calculate cascade tag byte
 * @param uid_full Full UID with cascade
 * @param cascade_level Cascade level (1-3)
 * @return Cascade tag byte
 */
uint8_t uid_calculate_cascade_tag(const Uid* uid_full, uint8_t cascade_level);

/**
 * @brief Validate MIFARE manufacturer byte (first byte)
 * @param manufacturer_byte First UID byte
 * @return true if valid manufacturer
 */
bool uid_validate_manufacturer(uint8_t manufacturer_byte);
