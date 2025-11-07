#include "ndef_builder.h"
#include <string.h>
#include <stdio.h>
#include <furi.h>

struct NdefBuilder {
    NdefMessage message;
    FuriMutex* mutex;
};

// URI prefix strings
static const char* uri_prefix_strings[] = {
    "",
    "http://www.",
    "https://www.",
    "http://",
    "https://",
    "tel:",
    "mailto:",
    "ftp://anonymous:anonymous@",
    "ftp://ftp.",
    "ftps://",
    "sftp://",
    "smb://",
    "nfs://",
    "ftp://",
    "dav://",
    "news:",
    "telnet://",
    "imap:",
    "rtsp://",
    "urn:",
    "pop:",
    "sip:",
    "sips:",
    "tftp:",
    "btspp://",
    "btl2cap://",
    "btgoep://",
    "tcpobex://",
    "irdaobex://",
    "file://",
    "urn:epc:id:",
    "urn:epc:tag:",
    "urn:epc:pat:",
    "urn:epc:raw:",
    "urn:epc:",
    "urn:nfc:",
};

NdefBuilder* ndef_builder_alloc(void) {
    NdefBuilder* builder = malloc(sizeof(NdefBuilder));
    memset(builder, 0, sizeof(NdefBuilder));
    builder->mutex = furi_mutex_alloc(FuriMutexTypeNormal);
    return builder;
}

void ndef_builder_free(NdefBuilder* builder) {
    if(!builder) return;
    furi_mutex_free(builder->mutex);
    free(builder);
}

void ndef_builder_clear(NdefBuilder* builder) {
    if(!builder) return;
    furi_mutex_acquire(builder->mutex, FuriWaitForever);
    memset(&builder->message, 0, sizeof(NdefMessage));
    furi_mutex_release(builder->mutex);
}

NdefMessage* ndef_builder_get_message(NdefBuilder* builder) {
    if(!builder) return NULL;
    return &builder->message;
}

size_t ndef_builder_get_record_count(NdefBuilder* builder) {
    if(!builder) return 0;
    return builder->message.record_count;
}

const NdefRecord* ndef_builder_get_record(NdefBuilder* builder, size_t index) {
    if(!builder || index >= builder->message.record_count) return NULL;
    return &builder->message.records[index];
}

bool ndef_builder_remove_record(NdefBuilder* builder, size_t index) {
    if(!builder || index >= builder->message.record_count) return false;

    furi_mutex_acquire(builder->mutex, FuriWaitForever);

    // Shift remaining records
    for(size_t i = index; i < builder->message.record_count - 1; i++) {
        memcpy(
            &builder->message.records[i],
            &builder->message.records[i + 1],
            sizeof(NdefRecord));
    }

    builder->message.record_count--;

    // Update MB/ME flags
    if(builder->message.record_count > 0) {
        builder->message.records[0].is_first = true;
        builder->message.records[builder->message.record_count - 1].is_last = true;
    }

    furi_mutex_release(builder->mutex);
    return true;
}

static bool add_record(NdefBuilder* builder, const NdefRecord* record) {
    if(!builder || !record) return false;

    furi_mutex_acquire(builder->mutex, FuriWaitForever);

    if(builder->message.record_count >= NDEF_MAX_RECORDS) {
        furi_mutex_release(builder->mutex);
        return false;
    }

    // Copy record
    memcpy(
        &builder->message.records[builder->message.record_count],
        record,
        sizeof(NdefRecord));

    builder->message.record_count++;

    // Update MB/ME flags for all records
    for(size_t i = 0; i < builder->message.record_count; i++) {
        builder->message.records[i].is_first = (i == 0);
        builder->message.records[i].is_last = (i == builder->message.record_count - 1);
    }

    furi_mutex_release(builder->mutex);
    return true;
}

bool ndef_builder_add_text_record(
    NdefBuilder* builder,
    const char* text,
    const char* language_code,
    TextEncoding encoding) {
    if(!builder || !text || !language_code) return false;

    NdefRecord record = {0};

    record.tnf = NdefTnfWellKnown;
    record.type_length = 1;
    record.type[0] = 'T'; // Text RTD

    // Payload: status byte + language code + text
    uint8_t lang_len = strlen(language_code);
    if(lang_len > 63) lang_len = 63; // Max language code length

    uint8_t status = encoding | lang_len;
    record.payload[0] = status;

    // Copy language code
    memcpy(&record.payload[1], language_code, lang_len);

    // Copy text
    size_t text_len = strlen(text);
    size_t max_text_len = NDEF_MAX_PAYLOAD_SIZE - 1 - lang_len;
    if(text_len > max_text_len) text_len = max_text_len;

    memcpy(&record.payload[1 + lang_len], text, text_len);

    record.payload_length = 1 + lang_len + text_len;
    record.is_short = (record.payload_length < 256);

    return add_record(builder, &record);
}

bool ndef_builder_add_uri_record(
    NdefBuilder* builder,
    const char* uri,
    UriPrefix prefix) {
    if(!builder || !uri) return false;

    NdefRecord record = {0};

    record.tnf = NdefTnfWellKnown;
    record.type_length = 1;
    record.type[0] = 'U'; // URI RTD

    // Payload: identifier code + URI
    record.payload[0] = prefix;

    size_t uri_len = strlen(uri);
    size_t max_uri_len = NDEF_MAX_PAYLOAD_SIZE - 1;
    if(uri_len > max_uri_len) uri_len = max_uri_len;

    memcpy(&record.payload[1], uri, uri_len);

    record.payload_length = 1 + uri_len;
    record.is_short = (record.payload_length < 256);

    return add_record(builder, &record);
}

bool ndef_builder_add_url_record(NdefBuilder* builder, const char* url) {
    if(!builder || !url) return false;

    // Auto-detect prefix
    UriPrefix prefix = UriPrefixNone;

    if(strncmp(url, "https://www.", 12) == 0) {
        prefix = UriPrefixHttpsWww;
        url += 12;
    } else if(strncmp(url, "http://www.", 11) == 0) {
        prefix = UriPrefixHttpWww;
        url += 11;
    } else if(strncmp(url, "https://", 8) == 0) {
        prefix = UriPrefixHttps;
        url += 8;
    } else if(strncmp(url, "http://", 7) == 0) {
        prefix = UriPrefixHttp;
        url += 7;
    } else if(strncmp(url, "tel:", 4) == 0) {
        prefix = UriPrefixTel;
        url += 4;
    } else if(strncmp(url, "mailto:", 7) == 0) {
        prefix = UriPrefixMailto;
        url += 7;
    }

    return ndef_builder_add_uri_record(builder, url, prefix);
}

bool ndef_builder_add_wifi_record(
    NdefBuilder* builder,
    const char* ssid,
    const char* password,
    WifiAuthType auth,
    WifiEncryptType encrypt) {
    if(!builder || !ssid) return false;

    NdefRecord record = {0};

    record.tnf = NdefTnfMimeMedia;
    record.type_length = 26;
    memcpy(record.type, "application/vnd.wfa.wsc", 23);

    // Build WiFi Simple Config payload
    uint8_t* payload = record.payload;
    size_t offset = 0;

    // Credential (0x100E)
    payload[offset++] = 0x10;
    payload[offset++] = 0x0E;

    size_t cred_length_offset = offset;
    offset += 2; // Skip length for now

    size_t cred_start = offset;

    // Network Index (0x1026)
    payload[offset++] = 0x10;
    payload[offset++] = 0x26;
    payload[offset++] = 0x00;
    payload[offset++] = 0x01;
    payload[offset++] = 0x01; // Index 1

    // SSID (0x1045)
    payload[offset++] = 0x10;
    payload[offset++] = 0x45;
    uint8_t ssid_len = strlen(ssid);
    payload[offset++] = 0x00;
    payload[offset++] = ssid_len;
    memcpy(&payload[offset], ssid, ssid_len);
    offset += ssid_len;

    // Auth Type (0x1003)
    payload[offset++] = 0x10;
    payload[offset++] = 0x03;
    payload[offset++] = 0x00;
    payload[offset++] = 0x02;
    payload[offset++] = (auth >> 8) & 0xFF;
    payload[offset++] = auth & 0xFF;

    // Encryption Type (0x100F)
    payload[offset++] = 0x10;
    payload[offset++] = 0x0F;
    payload[offset++] = 0x00;
    payload[offset++] = 0x02;
    payload[offset++] = (encrypt >> 8) & 0xFF;
    payload[offset++] = encrypt & 0xFF;

    // Network Key (0x1027) - Password
    if(password && strlen(password) > 0) {
        payload[offset++] = 0x10;
        payload[offset++] = 0x27;
        uint8_t pass_len = strlen(password);
        payload[offset++] = 0x00;
        payload[offset++] = pass_len;
        memcpy(&payload[offset], password, pass_len);
        offset += pass_len;
    }

    // MAC Address (0x1020) - Optional, use zeros
    payload[offset++] = 0x10;
    payload[offset++] = 0x20;
    payload[offset++] = 0x00;
    payload[offset++] = 0x06;
    memset(&payload[offset], 0xFF, 6); // Broadcast MAC
    offset += 6;

    // Update credential length
    size_t cred_length = offset - cred_start;
    payload[cred_length_offset] = (cred_length >> 8) & 0xFF;
    payload[cred_length_offset + 1] = cred_length & 0xFF;

    record.payload_length = offset;
    record.is_short = (record.payload_length < 256);

    return add_record(builder, &record);
}

bool ndef_builder_add_app_launch_record(NdefBuilder* builder, const char* package_name) {
    if(!builder || !package_name) return false;

    NdefRecord record = {0};

    record.tnf = NdefTnfExternal;
    record.type_length = 15;
    memcpy(record.type, "android.com:pkg", 15);

    // Payload is just the package name
    size_t pkg_len = strlen(package_name);
    if(pkg_len > NDEF_MAX_PAYLOAD_SIZE) pkg_len = NDEF_MAX_PAYLOAD_SIZE;

    memcpy(record.payload, package_name, pkg_len);
    record.payload_length = pkg_len;
    record.is_short = (record.payload_length < 256);

    return add_record(builder, &record);
}

bool ndef_builder_add_vcard_record(
    NdefBuilder* builder,
    const char* name,
    const char* phone,
    const char* email,
    const char* organization) {
    if(!builder || !name) return false;

    NdefRecord record = {0};

    record.tnf = NdefTnfMimeMedia;
    record.type_length = 9;
    memcpy(record.type, "text/vcard", 10);

    // Build vCard 3.0
    char vcard[NDEF_MAX_PAYLOAD_SIZE];
    size_t len = 0;

    len += snprintf(&vcard[len], NDEF_MAX_PAYLOAD_SIZE - len, "BEGIN:VCARD\r\n");
    len += snprintf(&vcard[len], NDEF_MAX_PAYLOAD_SIZE - len, "VERSION:3.0\r\n");
    len += snprintf(&vcard[len], NDEF_MAX_PAYLOAD_SIZE - len, "FN:%s\r\n", name);
    len += snprintf(&vcard[len], NDEF_MAX_PAYLOAD_SIZE - len, "N:%s\r\n", name);

    if(phone) {
        len += snprintf(&vcard[len], NDEF_MAX_PAYLOAD_SIZE - len, "TEL:%s\r\n", phone);
    }

    if(email) {
        len += snprintf(&vcard[len], NDEF_MAX_PAYLOAD_SIZE - len, "EMAIL:%s\r\n", email);
    }

    if(organization) {
        len += snprintf(&vcard[len], NDEF_MAX_PAYLOAD_SIZE - len, "ORG:%s\r\n", organization);
    }

    len += snprintf(&vcard[len], NDEF_MAX_PAYLOAD_SIZE - len, "END:VCARD\r\n");

    memcpy(record.payload, vcard, len);
    record.payload_length = len;
    record.is_short = (record.payload_length < 256);

    return add_record(builder, &record);
}

bool ndef_builder_add_mime_record(
    NdefBuilder* builder,
    const char* mime_type,
    const uint8_t* data,
    size_t data_length) {
    if(!builder || !mime_type || !data) return false;

    NdefRecord record = {0};

    record.tnf = NdefTnfMimeMedia;
    record.type_length = strlen(mime_type);
    if(record.type_length > NDEF_MAX_TYPE_LENGTH) {
        record.type_length = NDEF_MAX_TYPE_LENGTH;
    }
    memcpy(record.type, mime_type, record.type_length);

    if(data_length > NDEF_MAX_PAYLOAD_SIZE) {
        data_length = NDEF_MAX_PAYLOAD_SIZE;
    }

    memcpy(record.payload, data, data_length);
    record.payload_length = data_length;
    record.is_short = (record.payload_length < 256);

    return add_record(builder, &record);
}

size_t ndef_record_calculate_size(const NdefRecord* record) {
    if(!record) return 0;

    size_t size = 0;

    // Header byte
    size += 1;

    // Type length
    size += 1;

    // Payload length (1 or 4 bytes depending on SR flag)
    if(record->is_short) {
        size += 1;
    } else {
        size += 4;
    }

    // ID length (if IL flag set)
    if(record->has_id) {
        size += 1;
    }

    // Type
    size += record->type_length;

    // ID
    if(record->has_id) {
        size += record->id_length;
    }

    // Payload
    size += record->payload_length;

    return size;
}

size_t ndef_builder_serialize(NdefBuilder* builder, uint8_t* output, size_t max_size) {
    if(!builder || !output) return 0;

    furi_mutex_acquire(builder->mutex, FuriWaitForever);

    size_t offset = 0;

    for(size_t i = 0; i < builder->message.record_count; i++) {
        const NdefRecord* record = &builder->message.records[i];

        size_t record_size = ndef_record_calculate_size(record);
        if(offset + record_size > max_size) {
            break;
        }

        // Build header byte
        uint8_t header = record->tnf & 0x07;
        if(record->is_first) header |= NDEF_FLAG_MB;
        if(record->is_last) header |= NDEF_FLAG_ME;
        if(record->is_short) header |= NDEF_FLAG_SR;
        if(record->has_id) header |= NDEF_FLAG_IL;

        output[offset++] = header;

        // Type length
        output[offset++] = record->type_length;

        // Payload length
        if(record->is_short) {
            output[offset++] = record->payload_length & 0xFF;
        } else {
            output[offset++] = (record->payload_length >> 24) & 0xFF;
            output[offset++] = (record->payload_length >> 16) & 0xFF;
            output[offset++] = (record->payload_length >> 8) & 0xFF;
            output[offset++] = record->payload_length & 0xFF;
        }

        // ID length (if present)
        if(record->has_id) {
            output[offset++] = record->id_length;
        }

        // Type
        memcpy(&output[offset], record->type, record->type_length);
        offset += record->type_length;

        // ID (if present)
        if(record->has_id) {
            memcpy(&output[offset], record->id, record->id_length);
            offset += record->id_length;
        }

        // Payload
        memcpy(&output[offset], record->payload, record->payload_length);
        offset += record->payload_length;
    }

    builder->message.total_size = offset;

    furi_mutex_release(builder->mutex);

    return offset;
}

size_t ndef_builder_get_description(
    NdefBuilder* builder,
    size_t record_index,
    char* output,
    size_t max_length) {
    if(!builder || !output || record_index >= builder->message.record_count) return 0;

    const NdefRecord* record = &builder->message.records[record_index];
    size_t len = 0;

    // Determine record type
    if(record->tnf == NdefTnfWellKnown && record->type_length == 1) {
        if(record->type[0] == 'T') {
            // Text record
            uint8_t status = record->payload[0];
            uint8_t lang_len = status & 0x3F;

            len = snprintf(output, max_length, "Text: ");
            if(len < max_length) {
                size_t text_offset = 1 + lang_len;
                size_t text_len = record->payload_length - text_offset;
                if(text_len > max_length - len - 1) text_len = max_length - len - 1;
                memcpy(&output[len], &record->payload[text_offset], text_len);
                len += text_len;
                output[len] = '\0';
            }
        } else if(record->type[0] == 'U') {
            // URI record
            uint8_t prefix_code = record->payload[0];
            const char* prefix = ndef_get_uri_prefix_string(prefix_code);

            len = snprintf(output, max_length, "URL: %s", prefix);
            if(len < max_length) {
                size_t uri_len = record->payload_length - 1;
                if(uri_len > max_length - len - 1) uri_len = max_length - len - 1;
                memcpy(&output[len], &record->payload[1], uri_len);
                len += uri_len;
                output[len] = '\0';
            }
        }
    } else if(record->tnf == NdefTnfExternal) {
        if(memcmp(record->type, "android.com:pkg", 15) == 0) {
            len = snprintf(output, max_length, "App: ");
            if(len < max_length) {
                size_t pkg_len = record->payload_length;
                if(pkg_len > max_length - len - 1) pkg_len = max_length - len - 1;
                memcpy(&output[len], record->payload, pkg_len);
                len += pkg_len;
                output[len] = '\0';
            }
        }
    } else if(record->tnf == NdefTnfMimeMedia) {
        if(memcmp(record->type, "text/vcard", 10) == 0) {
            len = snprintf(output, max_length, "vCard contact");
        } else if(memcmp(record->type, "application/vnd.wfa.wsc", 23) == 0) {
            len = snprintf(output, max_length, "WiFi credentials");
        } else {
            len = snprintf(output, max_length, "MIME: %.*s", record->type_length, record->type);
        }
    }

    return len;
}

bool ndef_builder_validate(NdefBuilder* builder) {
    if(!builder) return false;

    if(builder->message.record_count == 0) return true; // Empty message is valid

    // Check that first record has MB flag
    if(!builder->message.records[0].is_first) return false;

    // Check that last record has ME flag
    if(!builder->message.records[builder->message.record_count - 1].is_last) return false;

    // Check that only first has MB and only last has ME
    for(size_t i = 1; i < builder->message.record_count; i++) {
        if(builder->message.records[i].is_first) return false;
    }

    for(size_t i = 0; i < builder->message.record_count - 1; i++) {
        if(builder->message.records[i].is_last) return false;
    }

    return true;
}

const char* ndef_get_uri_prefix_string(UriPrefix prefix) {
    if(prefix < sizeof(uri_prefix_strings) / sizeof(uri_prefix_strings[0])) {
        return uri_prefix_strings[prefix];
    }
    return "";
}

const char* ndef_get_text_encoding_name(TextEncoding encoding) {
    return (encoding == TextEncodingUtf8) ? "UTF-8" : "UTF-16";
}
