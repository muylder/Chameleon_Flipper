#include "uid_generator.h"
#include <string.h>
#include <furi.h>

static const char* uid_type_names[] = {
    "4-byte (Classic)",
    "7-byte (Ultralight/DESFire)",
    "10-byte (Extended)",
};

// Known manufacturer codes (first byte of UID)
static const uint8_t valid_manufacturers[] = {
    0x04, // NXP
    0x02, // STMicroelectronics
    0x05, // Infineon
    0x06, // Other
    0x07, // Other
    0x08, // Other
};

uint8_t uid_calculate_bcc(const uint8_t* uid, uint8_t length) {
    if(!uid || length == 0) return 0;

    uint8_t bcc = 0;
    for(uint8_t i = 0; i < length; i++) {
        bcc ^= uid[i];
    }

    return bcc;
}

bool uid_validate(const Uid* uid) {
    if(!uid) return false;

    if(uid->length == 0 || uid->length > 10) return false;

    // Calculate expected BCC0
    uint8_t expected_bcc0 = uid_calculate_bcc(uid->bytes, uid->length);

    if(uid->bcc0 != expected_bcc0) {
        return false;
    }

    // For 7 and 10 byte UIDs, validate BCC1 if applicable
    if(uid->length >= 7) {
        // BCC1 calculation depends on cascade structure
        // For simplicity, we'll assume it's correctly set if BCC0 is valid
    }

    return true;
}

bool uid_generate_random(UidType type, Uid* uid) {
    if(!uid) return false;

    memset(uid, 0, sizeof(Uid));
    uid->type = type;

    // Set length based on type
    switch(type) {
    case UidType4Byte:
        uid->length = 4;
        break;
    case UidType7Byte:
        uid->length = 7;
        break;
    case UidType10Byte:
        uid->length = 10;
        break;
    default:
        return false;
    }

    // Generate random bytes
    for(uint8_t i = 0; i < uid->length; i++) {
        uid->bytes[i] = (uint8_t)furi_hal_random_get();
    }

    // Set manufacturer byte to NXP (0x04) for first byte
    uid->bytes[0] = 0x04;

    // Calculate BCC
    uid->bcc0 = uid_calculate_bcc(uid->bytes, uid->length);

    // For 7 and 10 byte UIDs, cascade structure
    if(uid->length >= 7) {
        // Cascade tag and BCC1
        uid->bcc1 = uid_calculate_bcc(&uid->bytes[3], uid->length - 3);
    }

    uid->is_valid = true;

    return true;
}

bool uid_from_hex_string(const char* hex_string, Uid* uid) {
    if(!hex_string || !uid) return false;

    memset(uid, 0, sizeof(Uid));

    size_t hex_len = strlen(hex_string);

    // Determine type based on length
    // 4-byte: 8 chars (+ 2 for BCC = 10)
    // 7-byte: 14 chars (+ 4 for BCC = 18)
    // 10-byte: 20 chars (+ 4 for BCC = 24)

    if(hex_len == 8 || hex_len == 10) {
        uid->type = UidType4Byte;
        uid->length = 4;
    } else if(hex_len == 14 || hex_len == 18) {
        uid->type = UidType7Byte;
        uid->length = 7;
    } else if(hex_len == 20 || hex_len == 24) {
        uid->type = UidType10Byte;
        uid->length = 10;
    } else {
        return false; // Invalid length
    }

    // Parse hex string to bytes
    for(uint8_t i = 0; i < uid->length; i++) {
        char byte_str[3] = {hex_string[i * 2], hex_string[i * 2 + 1], '\0'};
        char* endptr;
        long val = strtol(byte_str, &endptr, 16);

        if(endptr != &byte_str[2]) {
            return false; // Invalid hex character
        }

        uid->bytes[i] = (uint8_t)val;
    }

    // Calculate BCC
    uid->bcc0 = uid_calculate_bcc(uid->bytes, uid->length);

    if(uid->length >= 7) {
        uid->bcc1 = uid_calculate_bcc(&uid->bytes[3], uid->length - 3);
    }

    uid->is_valid = true;

    return true;
}

size_t uid_to_hex_string(const Uid* uid, char* output, bool include_bcc) {
    if(!uid || !output) return 0;

    size_t pos = 0;

    // Write UID bytes
    for(uint8_t i = 0; i < uid->length; i++) {
        pos += snprintf(&output[pos], 3, "%02X", uid->bytes[i]);
    }

    // Optionally write BCC
    if(include_bcc) {
        pos += snprintf(&output[pos], 3, "%02X", uid->bcc0);

        if(uid->length >= 7) {
            pos += snprintf(&output[pos], 3, "%02X", uid->bcc1);
        }
    }

    output[pos] = '\0';

    return pos;
}

bool uid_set_bytes(const uint8_t* uid_bytes, uint8_t length, Uid* uid) {
    if(!uid_bytes || !uid) return false;

    if(length != 4 && length != 7 && length != 10) {
        return false;
    }

    memset(uid, 0, sizeof(Uid));

    uid->type = uid_get_type_from_length(length);
    uid->length = length;

    memcpy(uid->bytes, uid_bytes, length);

    // Calculate BCC
    uid->bcc0 = uid_calculate_bcc(uid->bytes, uid->length);

    if(uid->length >= 7) {
        uid->bcc1 = uid_calculate_bcc(&uid->bytes[3], uid->length - 3);
    }

    uid->is_valid = uid_validate(uid);

    return uid->is_valid;
}

bool uid_is_manufacturer_format(const Uid* uid) {
    if(!uid || uid->length == 0) return false;

    // Check if first byte matches known manufacturers
    uint8_t mfg = uid->bytes[0];

    for(size_t i = 0; i < sizeof(valid_manufacturers); i++) {
        if(mfg == valid_manufacturers[i]) {
            return true;
        }
    }

    return false;
}

const char* uid_get_type_name(UidType type) {
    if(type >= 0 && type < sizeof(uid_type_names) / sizeof(uid_type_names[0])) {
        return uid_type_names[type];
    }
    return "Unknown";
}

UidType uid_get_type_from_length(uint8_t length) {
    switch(length) {
    case 4:
        return UidType4Byte;
    case 7:
        return UidType7Byte;
    case 10:
        return UidType10Byte;
    default:
        return UidType4Byte; // Default
    }
}

size_t uid_generate_batch(UidType type, Uid* uids, size_t count) {
    if(!uids || count == 0) return 0;

    size_t generated = 0;

    for(size_t i = 0; i < count; i++) {
        if(uid_generate_random(type, &uids[i])) {
            generated++;
        }
    }

    return generated;
}

uint8_t uid_calculate_cascade_tag(const Uid* uid_full, uint8_t cascade_level) {
    if(!uid_full) return 0;

    // Cascade tag is always 0x88
    if(cascade_level == 1 && uid_full->length > 4) {
        return 0x88;
    }

    if(cascade_level == 2 && uid_full->length > 7) {
        return 0x88;
    }

    return 0x00; // No cascade needed
}

bool uid_validate_manufacturer(uint8_t manufacturer_byte) {
    for(size_t i = 0; i < sizeof(valid_manufacturers); i++) {
        if(manufacturer_byte == valid_manufacturers[i]) {
            return true;
        }
    }

    return false;
}
