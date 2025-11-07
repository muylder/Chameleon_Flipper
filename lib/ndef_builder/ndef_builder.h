#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// NDEF Type Name Format (TNF)
typedef enum {
    NdefTnfEmpty = 0x00,
    NdefTnfWellKnown = 0x01,
    NdefTnfMimeMedia = 0x02,
    NdefTnfAbsoluteUri = 0x03,
    NdefTnfExternal = 0x04,
    NdefTnfUnknown = 0x05,
    NdefTnfUnchanged = 0x06,
    NdefTnfReserved = 0x07,
} NdefTnf;

// NDEF Record flags
#define NDEF_FLAG_MB 0x80 // Message Begin
#define NDEF_FLAG_ME 0x40 // Message End
#define NDEF_FLAG_CF 0x20 // Chunk Flag
#define NDEF_FLAG_SR 0x10 // Short Record
#define NDEF_FLAG_IL 0x08 // ID Length present

// Maximum sizes
#define NDEF_MAX_PAYLOAD_SIZE 256
#define NDEF_MAX_TYPE_LENGTH 32
#define NDEF_MAX_ID_LENGTH 32
#define NDEF_MAX_RECORDS 10

// NDEF Record structure
typedef struct {
    uint8_t tnf; // Type Name Format
    uint8_t type_length;
    uint8_t type[NDEF_MAX_TYPE_LENGTH];
    uint8_t id_length;
    uint8_t id[NDEF_MAX_ID_LENGTH];
    uint32_t payload_length;
    uint8_t payload[NDEF_MAX_PAYLOAD_SIZE];
    bool is_first; // MB flag
    bool is_last; // ME flag
    bool is_short; // SR flag (payload < 256 bytes)
    bool has_id; // IL flag
} NdefRecord;

// NDEF Message structure
typedef struct {
    NdefRecord records[NDEF_MAX_RECORDS];
    uint8_t record_count;
    size_t total_size;
} NdefMessage;

// NDEF Builder manager
typedef struct NdefBuilder NdefBuilder;

// URI identifier codes (for URI records)
typedef enum {
    UriPrefixNone = 0x00,
    UriPrefixHttpWww = 0x01, // http://www.
    UriPrefixHttpsWww = 0x02, // https://www.
    UriPrefixHttp = 0x03, // http://
    UriPrefixHttps = 0x04, // https://
    UriPrefixTel = 0x05, // tel:
    UriPrefixMailto = 0x06, // mailto:
    UriPrefixFtpAnonymous = 0x07, // ftp://anonymous:anonymous@
    UriPrefixFtpFtp = 0x08, // ftp://ftp.
    UriPrefixFtps = 0x09, // ftps://
    UriPrefixSftp = 0x0A, // sftp://
    UriPrefixSmb = 0x0B, // smb://
    UriPrefixNfs = 0x0C, // nfs://
    UriPrefixFtp = 0x0D, // ftp://
    UriPrefixDav = 0x0E, // dav://
    UriPrefixNews = 0x0F, // news:
    UriPrefixTelnet = 0x10, // telnet://
    UriPrefixImap = 0x11, // imap:
    UriPrefixRtsp = 0x12, // rtsp://
    UriPrefixUrn = 0x13, // urn:
    UriPrefixPop = 0x14, // pop:
    UriPrefixSip = 0x15, // sip:
    UriPrefixSips = 0x16, // sips:
    UriPrefixTftp = 0x17, // tftp:
    UriPrefixBtspp = 0x18, // btspp://
    UriPrefixBtl2cap = 0x19, // btl2cap://
    UriPrefixBtgoep = 0x1A, // btgoep://
    UriPrefixTcpobex = 0x1B, // tcpobex://
    UriPrefixIrdaobex = 0x1C, // irdaobex://
    UriPrefixFile = 0x1D, // file://
    UriPrefixUrnEpcId = 0x1E, // urn:epc:id:
    UriPrefixUrnEpcTag = 0x1F, // urn:epc:tag:
    UriPrefixUrnEpcPat = 0x20, // urn:epc:pat:
    UriPrefixUrnEpcRaw = 0x21, // urn:epc:raw:
    UriPrefixUrnEpc = 0x22, // urn:epc:
    UriPrefixUrnNfc = 0x23, // urn:nfc:
} UriPrefix;

// Text encoding
typedef enum {
    TextEncodingUtf8 = 0x00,
    TextEncodingUtf16 = 0x80,
} TextEncoding;

// WiFi authentication type
typedef enum {
    WifiAuthOpen = 0x0001,
    WifiAuthWpaPersonal = 0x0002,
    WifiAuthShared = 0x0004,
    WifiAuthWpaEnterprise = 0x0008,
    WifiAuthWpa2Enterprise = 0x0010,
    WifiAuthWpa2Personal = 0x0020,
    WifiAuthWpaWpa2Personal = 0x0022,
} WifiAuthType;

// WiFi encryption type
typedef enum {
    WifiEncryptNone = 0x0001,
    WifiEncryptWep = 0x0002,
    WifiEncryptTkip = 0x0004,
    WifiEncryptAes = 0x0008,
    WifiEncryptAesTkip = 0x000C,
} WifiEncryptType;

// Allocation and deallocation
NdefBuilder* ndef_builder_alloc(void);
void ndef_builder_free(NdefBuilder* builder);

// Message operations
void ndef_builder_clear(NdefBuilder* builder);
NdefMessage* ndef_builder_get_message(NdefBuilder* builder);
size_t ndef_builder_get_record_count(NdefBuilder* builder);
const NdefRecord* ndef_builder_get_record(NdefBuilder* builder, size_t index);
bool ndef_builder_remove_record(NdefBuilder* builder, size_t index);

// Build NDEF records (Well-Known Types)
bool ndef_builder_add_text_record(
    NdefBuilder* builder,
    const char* text,
    const char* language_code,
    TextEncoding encoding);

bool ndef_builder_add_uri_record(
    NdefBuilder* builder,
    const char* uri,
    UriPrefix prefix);

bool ndef_builder_add_url_record(NdefBuilder* builder, const char* url);

// Build NDEF records (External Types)
bool ndef_builder_add_wifi_record(
    NdefBuilder* builder,
    const char* ssid,
    const char* password,
    WifiAuthType auth,
    WifiEncryptType encrypt);

bool ndef_builder_add_app_launch_record(NdefBuilder* builder, const char* package_name);

bool ndef_builder_add_vcard_record(
    NdefBuilder* builder,
    const char* name,
    const char* phone,
    const char* email,
    const char* organization);

// Build NDEF records (MIME Type)
bool ndef_builder_add_mime_record(
    NdefBuilder* builder,
    const char* mime_type,
    const uint8_t* data,
    size_t data_length);

// Serialize message to bytes
size_t ndef_builder_serialize(NdefBuilder* builder, uint8_t* output, size_t max_size);

// Parse message from bytes
bool ndef_builder_parse(NdefBuilder* builder, const uint8_t* data, size_t length);

// Get human-readable description
size_t ndef_builder_get_description(
    NdefBuilder* builder,
    size_t record_index,
    char* output,
    size_t max_length);

// Validation
bool ndef_builder_validate(NdefBuilder* builder);

// Helper: Get URI prefix string
const char* ndef_get_uri_prefix_string(UriPrefix prefix);

// Helper: Get text encoding name
const char* ndef_get_text_encoding_name(TextEncoding encoding);

// Helper: Calculate record size
size_t ndef_record_calculate_size(const NdefRecord* record);
