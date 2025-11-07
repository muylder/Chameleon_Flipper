#include "access_bits_calc.h"
#include <string.h>
#include <stdio.h>

// Permission names
static const char* permission_names[] = {
    "Never",
    "Key A",
    "Key B",
    "Both Keys",
};

// Preset names
static const char* preset_names[] = {
    "Factory Default",
    "MAD Sector",
    "Read-Only",
    "Transport Card",
    "Hotel Card",
};

// Helper: Extract access condition bits for a block
static void extract_c1_c2_c3(const AccessBits* bits, uint8_t block, uint8_t* c1, uint8_t* c2, uint8_t* c3) {
    // Access bits structure (inverted bits in parentheses):
    // Byte 6: (!C2_3 !C2_2 !C2_1 !C2_0  !C1_3 !C1_2 !C1_1 !C1_0)
    // Byte 7: (!C3_3 !C3_2 !C3_1 !C3_0   C1_3  C1_2  C1_1  C1_0)
    // Byte 8: ( C3_3  C3_2  C3_1  C3_0   C2_3  C2_2  C2_1  C2_0)

    uint8_t c1_normal = (bits->byte7 >> block) & 0x01;
    uint8_t c2_normal = (bits->byte8 >> block) & 0x01;
    uint8_t c3_normal = (bits->byte8 >> (4 + block)) & 0x01;

    *c1 = c1_normal;
    *c2 = c2_normal;
    *c3 = c3_normal;
}

// Helper: Set access condition bits for a block
static void set_c1_c2_c3(AccessBits* bits, uint8_t block, uint8_t c1, uint8_t c2, uint8_t c3) {
    // Set normal bits
    if(c1) {
        bits->byte7 |= (1 << block);
    } else {
        bits->byte7 &= ~(1 << block);
    }

    if(c2) {
        bits->byte8 |= (1 << block);
    } else {
        bits->byte8 &= ~(1 << block);
    }

    if(c3) {
        bits->byte8 |= (1 << (4 + block));
    } else {
        bits->byte8 &= ~(1 << (4 + block));
    }

    // Set inverted bits in byte6
    uint8_t c1_inv = !c1;
    uint8_t c2_inv = !c2;
    uint8_t c3_inv = !c3;

    if(c1_inv) {
        bits->byte6 |= (1 << block);
    } else {
        bits->byte6 &= ~(1 << block);
    }

    if(c2_inv) {
        bits->byte6 |= (1 << (4 + block));
    } else {
        bits->byte6 &= ~(1 << (4 + block));
    }

    // C3 inverted is in byte7 upper nibble
    if(c3_inv) {
        bits->byte7 |= (1 << (4 + block));
    } else {
        bits->byte7 &= ~(1 << (4 + block));
    }
}

// Parse block permissions from C1/C2/C3
static BlockPermissions parse_block_permissions(uint8_t c1, uint8_t c2, uint8_t c3) {
    BlockPermissions perm = {0};

    // Data block access conditions (from MIFARE Classic datasheet)
    uint8_t condition = (c1 << 2) | (c2 << 1) | c3;

    switch(condition) {
    case 0b000: // C1=0, C2=0, C3=0
        perm.read = PermissionBoth;
        perm.write = PermissionBoth;
        perm.increment = PermissionBoth;
        perm.decrement_transfer_restore = PermissionBoth;
        break;

    case 0b010: // C1=0, C2=1, C3=0
        perm.read = PermissionBoth;
        perm.write = PermissionNever;
        perm.increment = PermissionNever;
        perm.decrement_transfer_restore = PermissionNever;
        break;

    case 0b100: // C1=1, C2=0, C3=0
        perm.read = PermissionBoth;
        perm.write = PermissionKeyB;
        perm.increment = PermissionNever;
        perm.decrement_transfer_restore = PermissionNever;
        break;

    case 0b110: // C1=1, C2=1, C3=0
        perm.read = PermissionBoth;
        perm.write = PermissionKeyB;
        perm.increment = PermissionKeyB;
        perm.decrement_transfer_restore = PermissionBoth;
        break;

    case 0b001: // C1=0, C2=0, C3=1
        perm.read = PermissionBoth;
        perm.write = PermissionNever;
        perm.increment = PermissionNever;
        perm.decrement_transfer_restore = PermissionBoth;
        break;

    case 0b011: // C1=0, C2=1, C3=1
        perm.read = PermissionKeyB;
        perm.write = PermissionKeyB;
        perm.increment = PermissionNever;
        perm.decrement_transfer_restore = PermissionNever;
        break;

    case 0b101: // C1=1, C2=0, C3=1
        perm.read = PermissionKeyB;
        perm.write = PermissionNever;
        perm.increment = PermissionNever;
        perm.decrement_transfer_restore = PermissionNever;
        break;

    case 0b111: // C1=1, C2=1, C3=1
        perm.read = PermissionNever;
        perm.write = PermissionNever;
        perm.increment = PermissionNever;
        perm.decrement_transfer_restore = PermissionNever;
        break;
    }

    return perm;
}

// Calculate C1/C2/C3 from block permissions
static bool calculate_block_condition(const BlockPermissions* perm, uint8_t* c1, uint8_t* c2, uint8_t* c3) {
    // Match permissions to condition
    if(perm->read == PermissionBoth && perm->write == PermissionBoth &&
       perm->increment == PermissionBoth && perm->decrement_transfer_restore == PermissionBoth) {
        *c1 = 0; *c2 = 0; *c3 = 0;
        return true;
    }

    if(perm->read == PermissionBoth && perm->write == PermissionNever &&
       perm->increment == PermissionNever && perm->decrement_transfer_restore == PermissionNever) {
        *c1 = 0; *c2 = 1; *c3 = 0;
        return true;
    }

    if(perm->read == PermissionBoth && perm->write == PermissionKeyB &&
       perm->increment == PermissionNever && perm->decrement_transfer_restore == PermissionNever) {
        *c1 = 1; *c2 = 0; *c3 = 0;
        return true;
    }

    if(perm->read == PermissionBoth && perm->write == PermissionKeyB &&
       perm->increment == PermissionKeyB && perm->decrement_transfer_restore == PermissionBoth) {
        *c1 = 1; *c2 = 1; *c3 = 0;
        return true;
    }

    if(perm->read == PermissionBoth && perm->write == PermissionNever &&
       perm->increment == PermissionNever && perm->decrement_transfer_restore == PermissionBoth) {
        *c1 = 0; *c2 = 0; *c3 = 1;
        return true;
    }

    if(perm->read == PermissionKeyB && perm->write == PermissionKeyB &&
       perm->increment == PermissionNever && perm->decrement_transfer_restore == PermissionNever) {
        *c1 = 0; *c2 = 1; *c3 = 1;
        return true;
    }

    if(perm->read == PermissionKeyB && perm->write == PermissionNever &&
       perm->increment == PermissionNever && perm->decrement_transfer_restore == PermissionNever) {
        *c1 = 1; *c2 = 0; *c3 = 1;
        return true;
    }

    if(perm->read == PermissionNever && perm->write == PermissionNever &&
       perm->increment == PermissionNever && perm->decrement_transfer_restore == PermissionNever) {
        *c1 = 1; *c2 = 1; *c3 = 1;
        return true;
    }

    return false; // Invalid combination
}

// Parse trailer permissions from C1/C2/C3
static TrailerPermissions parse_trailer_permissions(uint8_t c1, uint8_t c2, uint8_t c3) {
    TrailerPermissions perm = {0};

    uint8_t condition = (c1 << 2) | (c2 << 1) | c3;

    switch(condition) {
    case 0b000: // Transport configuration
        perm.write_key_a = PermissionKeyA;
        perm.read_access_bits = PermissionKeyA;
        perm.write_access_bits = PermissionNever;
        perm.read_key_b = PermissionKeyA;
        perm.write_key_b = PermissionKeyA;
        break;

    case 0b010:
        perm.write_key_a = PermissionNever;
        perm.read_access_bits = PermissionKeyA;
        perm.write_access_bits = PermissionNever;
        perm.read_key_b = PermissionKeyA;
        perm.write_key_b = PermissionNever;
        break;

    case 0b100:
        perm.write_key_a = PermissionKeyB;
        perm.read_access_bits = PermissionBoth;
        perm.write_access_bits = PermissionNever;
        perm.read_key_b = PermissionNever;
        perm.write_key_b = PermissionKeyB;
        break;

    case 0b110:
        perm.write_key_a = PermissionNever;
        perm.read_access_bits = PermissionBoth;
        perm.write_access_bits = PermissionNever;
        perm.read_key_b = PermissionNever;
        perm.write_key_b = PermissionNever;
        break;

    case 0b001:
        perm.write_key_a = PermissionKeyA;
        perm.read_access_bits = PermissionKeyA;
        perm.write_access_bits = PermissionKeyA;
        perm.read_key_b = PermissionKeyA;
        perm.write_key_b = PermissionKeyA;
        break;

    case 0b011:
        perm.write_key_a = PermissionKeyB;
        perm.read_access_bits = PermissionBoth;
        perm.write_access_bits = PermissionKeyB;
        perm.read_key_b = PermissionNever;
        perm.write_key_b = PermissionKeyB;
        break;

    case 0b101:
        perm.write_key_a = PermissionNever;
        perm.read_access_bits = PermissionBoth;
        perm.write_access_bits = PermissionKeyB;
        perm.read_key_b = PermissionNever;
        perm.write_key_b = PermissionNever;
        break;

    case 0b111:
        perm.write_key_a = PermissionNever;
        perm.read_access_bits = PermissionBoth;
        perm.write_access_bits = PermissionNever;
        perm.read_key_b = PermissionNever;
        perm.write_key_b = PermissionNever;
        break;
    }

    return perm;
}

bool access_bits_calculate(const SectorAccessConfig* config, AccessBits* bits) {
    if(!config || !bits) return false;

    memset(bits, 0, sizeof(AccessBits));

    // Calculate conditions for each block
    uint8_t c1_0, c2_0, c3_0;
    uint8_t c1_1, c2_1, c3_1;
    uint8_t c1_2, c2_2, c3_2;
    uint8_t c1_3, c2_3, c3_3;

    if(!calculate_block_condition(&config->block0, &c1_0, &c2_0, &c3_0)) return false;
    if(!calculate_block_condition(&config->block1, &c1_1, &c2_1, &c3_1)) return false;
    if(!calculate_block_condition(&config->block2, &c1_2, &c2_2, &c3_2)) return false;

    // Trailer (block 3) - TODO: implement trailer condition calculation
    // For now, use factory default (000)
    c1_3 = 0; c2_3 = 0; c3_3 = 0;

    // Set bits for each block
    set_c1_c2_c3(bits, 0, c1_0, c2_0, c3_0);
    set_c1_c2_c3(bits, 1, c1_1, c2_1, c3_1);
    set_c1_c2_c3(bits, 2, c1_2, c2_2, c3_2);
    set_c1_c2_c3(bits, 3, c1_3, c2_3, c3_3);

    bits->is_valid = access_bits_validate(bits);

    return bits->is_valid;
}

bool access_bits_parse(const AccessBits* bits, SectorAccessConfig* config) {
    if(!bits || !config) return false;

    if(!bits->is_valid) return false;

    memset(config, 0, sizeof(SectorAccessConfig));

    // Extract and parse each block
    for(uint8_t block = 0; block < 4; block++) {
        uint8_t c1, c2, c3;
        extract_c1_c2_c3(bits, block, &c1, &c2, &c3);

        if(block < 3) {
            // Data blocks
            BlockPermissions perm = parse_block_permissions(c1, c2, c3);

            if(block == 0) config->block0 = perm;
            else if(block == 1) config->block1 = perm;
            else if(block == 2) config->block2 = perm;
        } else {
            // Trailer block
            config->trailer = parse_trailer_permissions(c1, c2, c3);
        }
    }

    return true;
}

bool access_bits_validate(const AccessBits* bits) {
    if(!bits) return false;

    // Check that inverted bits match normal bits
    for(uint8_t block = 0; block < 4; block++) {
        uint8_t c1_normal = (bits->byte7 >> block) & 0x01;
        uint8_t c2_normal = (bits->byte8 >> block) & 0x01;
        uint8_t c3_normal = (bits->byte8 >> (4 + block)) & 0x01;

        uint8_t c1_inv = (bits->byte6 >> block) & 0x01;
        uint8_t c2_inv = (bits->byte6 >> (4 + block)) & 0x01;
        uint8_t c3_inv = (bits->byte7 >> (4 + block)) & 0x01;

        if(c1_normal == c1_inv) return false; // Should be inverted
        if(c2_normal == c2_inv) return false;
        if(c3_normal == c3_inv) return false;
    }

    return true;
}

bool access_bits_get_preset(uint8_t preset_id, SectorAccessConfig* config) {
    if(!config) return false;

    memset(config, 0, sizeof(SectorAccessConfig));

    switch(preset_id) {
    case PRESET_FACTORY: // FF 07 80 (transport configuration)
        // All blocks: read/write with both keys
        config->block0.read = PermissionBoth;
        config->block0.write = PermissionBoth;
        config->block0.increment = PermissionBoth;
        config->block0.decrement_transfer_restore = PermissionBoth;

        config->block1 = config->block0;
        config->block2 = config->block0;

        // Trailer: Key A can write everything
        config->trailer.write_key_a = PermissionKeyA;
        config->trailer.read_access_bits = PermissionKeyA;
        config->trailer.write_access_bits = PermissionNever;
        config->trailer.read_key_b = PermissionKeyA;
        config->trailer.write_key_b = PermissionKeyA;
        break;

    case PRESET_READ_ONLY:
        // All blocks: read with both, write never
        config->block0.read = PermissionBoth;
        config->block0.write = PermissionNever;
        config->block1 = config->block0;
        config->block2 = config->block0;

        config->trailer.write_key_a = PermissionNever;
        config->trailer.read_access_bits = PermissionBoth;
        config->trailer.write_access_bits = PermissionNever;
        config->trailer.read_key_b = PermissionNever;
        config->trailer.write_key_b = PermissionNever;
        break;

    case PRESET_MAD:
    case PRESET_TRANSPORT_CARD:
    case PRESET_HOTEL_CARD:
        // Use factory default for now
        return access_bits_get_preset(PRESET_FACTORY, config);

    default:
        return false;
    }

    return true;
}

size_t access_bits_to_hex_string(const AccessBits* bits, char* output) {
    if(!bits || !output) return 0;

    return snprintf(output, 9, "%02X %02X %02X", bits->byte6, bits->byte7, bits->byte8);
}

bool access_bits_from_hex_string(const char* hex_string, AccessBits* bits) {
    if(!hex_string || !bits) return false;

    memset(bits, 0, sizeof(AccessBits));

    // Parse "XX XX XX" or "XXXXXX" format
    int matched = sscanf(hex_string, "%hhx %hhx %hhx", &bits->byte6, &bits->byte7, &bits->byte8);

    if(matched != 3) {
        // Try without spaces
        matched = sscanf(hex_string, "%02hhx%02hhx%02hhx", &bits->byte6, &bits->byte7, &bits->byte8);
    }

    if(matched == 3) {
        bits->is_valid = access_bits_validate(bits);
        return bits->is_valid;
    }

    return false;
}

const char* access_bits_get_permission_name(AccessPermission perm) {
    if(perm >= 0 && perm < (sizeof(permission_names) / sizeof(permission_names[0]))) {
        return permission_names[perm];
    }
    return "Unknown";
}

const char* access_bits_get_preset_name(uint8_t preset_id) {
    if(preset_id < (sizeof(preset_names) / sizeof(preset_names[0]))) {
        return preset_names[preset_id];
    }
    return "Unknown";
}

bool access_bits_can_read_with_key_a(const SectorAccessConfig* config, uint8_t block) {
    if(!config || block > 2) return false;

    const BlockPermissions* perm;
    if(block == 0) perm = &config->block0;
    else if(block == 1) perm = &config->block1;
    else perm = &config->block2;

    return (perm->read == PermissionKeyA || perm->read == PermissionBoth);
}

bool access_bits_can_write_with_key_b(const SectorAccessConfig* config, uint8_t block) {
    if(!config || block > 2) return false;

    const BlockPermissions* perm;
    if(block == 0) perm = &config->block0;
    else if(block == 1) perm = &config->block1;
    else perm = &config->block2;

    return (perm->write == PermissionKeyB || perm->write == PermissionBoth);
}

void access_bits_create_trailer(
    const uint8_t* key_a,
    const AccessBits* access_bits,
    const uint8_t* key_b,
    uint8_t* trailer) {
    if(!trailer) return;

    memset(trailer, 0, 16);

    // Copy Key A (bytes 0-5)
    if(key_a) {
        memcpy(trailer, key_a, 6);
    } else {
        // Use default FFFFFFFFFFFF
        memset(trailer, 0xFF, 6);
    }

    // Copy access bits (bytes 6-8)
    if(access_bits) {
        trailer[6] = access_bits->byte6;
        trailer[7] = access_bits->byte7;
        trailer[8] = access_bits->byte8;
    } else {
        // Use factory default FF 07 80
        trailer[6] = 0xFF;
        trailer[7] = 0x07;
        trailer[8] = 0x80;
    }

    // Byte 9 is user data (GPB), set to 0x69 (common value)
    trailer[9] = 0x69;

    // Copy Key B (bytes 10-15)
    if(key_b) {
        memcpy(&trailer[10], key_b, 6);
    } else {
        // Use default FFFFFFFFFFFF
        memset(&trailer[10], 0xFF, 6);
    }
}
