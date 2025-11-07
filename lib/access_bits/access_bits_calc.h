#pragma once

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief MIFARE Classic Access Bits Calculator
 *
 * Calculates and validates access bits for MIFARE Classic sector trailers
 */

// Access permissions
typedef enum {
    PermissionNever,
    PermissionKeyA,
    PermissionKeyB,
    PermissionBoth,
} AccessPermission;

// Block permissions
typedef struct {
    AccessPermission read;
    AccessPermission write;
    AccessPermission increment;
    AccessPermission decrement_transfer_restore;
} BlockPermissions;

// Sector trailer permissions
typedef struct {
    AccessPermission write_key_a;
    AccessPermission read_access_bits;
    AccessPermission write_access_bits;
    AccessPermission read_key_b;
    AccessPermission write_key_b;
} TrailerPermissions;

// Complete sector access configuration
typedef struct {
    BlockPermissions block0;
    BlockPermissions block1;
    BlockPermissions block2;
    TrailerPermissions trailer;
} SectorAccessConfig;

// Access bits (3 bytes)
typedef struct {
    uint8_t byte6;  // Typically at offset 6 in sector trailer
    uint8_t byte7;  // Typically at offset 7
    uint8_t byte8;  // Typically at offset 8
    bool is_valid;
} AccessBits;

/**
 * @brief Calculate access bits from permissions
 * @param config Sector access configuration
 * @param bits Output access bits
 * @return true if calculated successfully
 */
bool access_bits_calculate(const SectorAccessConfig* config, AccessBits* bits);

/**
 * @brief Parse access bits to permissions
 * @param bits Access bits
 * @param config Output sector access configuration
 * @return true if parsed successfully
 */
bool access_bits_parse(const AccessBits* bits, SectorAccessConfig* config);

/**
 * @brief Validate access bits
 * @param bits Access bits to validate
 * @return true if valid (C1, C2, C3 match inverted versions)
 */
bool access_bits_validate(const AccessBits* bits);

/**
 * @brief Get preset access configuration
 * @param preset_id Preset ID (0=Factory, 1=Transport, 2=MAD, etc.)
 * @param config Output configuration
 * @return true if preset exists
 */
bool access_bits_get_preset(uint8_t preset_id, SectorAccessConfig* config);

/**
 * @brief Format access bits as hex string
 * @param bits Access bits
 * @param output Output buffer (min 7 bytes: "XX XX XX")
 * @return Number of characters written
 */
size_t access_bits_to_hex_string(const AccessBits* bits, char* output);

/**
 * @brief Parse access bits from hex string
 * @param hex_string Hex string (e.g. "FF 07 80")
 * @param bits Output access bits
 * @return true if parsed successfully
 */
bool access_bits_from_hex_string(const char* hex_string, AccessBits* bits);

/**
 * @brief Get permission name
 * @param perm Permission enum
 * @return Permission name string
 */
const char* access_bits_get_permission_name(AccessPermission perm);

/**
 * @brief Get preset name
 * @param preset_id Preset ID
 * @return Preset name string
 */
const char* access_bits_get_preset_name(uint8_t preset_id);

/**
 * @brief Check if configuration allows reading blocks with Key A
 * @param config Sector configuration
 * @param block Block number (0-2)
 * @return true if readable with Key A
 */
bool access_bits_can_read_with_key_a(const SectorAccessConfig* config, uint8_t block);

/**
 * @brief Check if configuration allows writing blocks with Key B
 * @param config Sector configuration
 * @param block Block number (0-2)
 * @return true if writable with Key B
 */
bool access_bits_can_write_with_key_b(const SectorAccessConfig* config, uint8_t block);

// Common preset IDs
#define PRESET_FACTORY 0          // FF 07 80 (transport/factory default)
#define PRESET_MAD 1              // MAD sector access
#define PRESET_READ_ONLY 2        // Read-only with Key B
#define PRESET_TRANSPORT_CARD 3   // Common transport card config
#define PRESET_HOTEL_CARD 4       // Hotel key card config

/**
 * @brief Create full sector trailer
 * @param key_a Key A (6 bytes)
 * @param access_bits Access bits
 * @param key_b Key B (6 bytes)
 * @param trailer Output trailer (16 bytes)
 */
void access_bits_create_trailer(
    const uint8_t* key_a,
    const AccessBits* access_bits,
    const uint8_t* key_b,
    uint8_t* trailer);
