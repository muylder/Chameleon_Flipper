#include "clone_wizard.h"
#include <string.h>
#include <storage/storage.h>

struct CloneWizard {
    CloneWizardState state;
    FuriMutex* mutex;
};

static const char* step_names[] = {
    "Scan Tag",
    "Detect Type",
    "Test Keys",
    "Read Data",
    "Select Slot",
    "Write Data",
    "Validate",
    "Complete",
};

static const char* result_names[] = {
    "Success",
    "Partial Success",
    "Failed",
    "Cancelled",
};

CloneWizard* clone_wizard_alloc(void) {
    CloneWizard* wizard = malloc(sizeof(CloneWizard));
    memset(wizard, 0, sizeof(CloneWizard));

    wizard->mutex = furi_mutex_alloc(FuriMutexTypeNormal);
    clone_wizard_reset(wizard);

    return wizard;
}

void clone_wizard_free(CloneWizard* wizard) {
    if(!wizard) return;

    furi_mutex_free(wizard->mutex);
    free(wizard);
}

void clone_wizard_reset(CloneWizard* wizard) {
    if(!wizard) return;

    furi_mutex_acquire(wizard->mutex, FuriWaitForever);

    memset(&wizard->state, 0, sizeof(CloneWizardState));
    wizard->state.current_step = CloneStepScan;
    wizard->state.result = CloneResultFailed;
    wizard->state.progress_percent = 0;
    snprintf(
        wizard->state.status_message,
        sizeof(wizard->state.status_message),
        "Ready to clone");

    furi_mutex_release(wizard->mutex);
}

const CloneWizardState* clone_wizard_get_state(CloneWizard* wizard) {
    if(!wizard) return NULL;
    return &wizard->state;
}

bool clone_wizard_next_step(CloneWizard* wizard) {
    if(!wizard) return false;

    furi_mutex_acquire(wizard->mutex, FuriWaitForever);

    if(wizard->state.current_step < CloneStepComplete) {
        wizard->state.current_step++;
        wizard->state.progress_percent = (wizard->state.current_step * 100) / CloneStepComplete;

        furi_mutex_release(wizard->mutex);
        return true;
    }

    furi_mutex_release(wizard->mutex);
    return false;
}

bool clone_wizard_scan_tag(
    CloneWizard* wizard,
    uint8_t* uid,
    uint8_t* uid_len,
    uint8_t* atqa,
    uint8_t* sak) {
    if(!wizard || !uid || !uid_len || !atqa || !sak) return false;

    furi_mutex_acquire(wizard->mutex, FuriWaitForever);

    // Simulate tag scan (in real implementation, call HF14A_SCAN)
    // Mock data for demonstration
    wizard->state.tag_data.uid_len = 4;
    wizard->state.tag_data.uid[0] = 0x04;
    wizard->state.tag_data.uid[1] = 0xAB;
    wizard->state.tag_data.uid[2] = 0xCD;
    wizard->state.tag_data.uid[3] = 0xEF;

    wizard->state.tag_data.atqa[0] = 0x44;
    wizard->state.tag_data.atqa[1] = 0x00;
    wizard->state.tag_data.sak = 0x08;

    // Copy to outputs
    *uid_len = wizard->state.tag_data.uid_len;
    memcpy(uid, wizard->state.tag_data.uid, *uid_len);
    memcpy(atqa, wizard->state.tag_data.atqa, 2);
    *sak = wizard->state.tag_data.sak;

    snprintf(
        wizard->state.status_message,
        sizeof(wizard->state.status_message),
        "Tag scanned: UID %02X%02X%02X%02X",
        uid[0],
        uid[1],
        uid[2],
        uid[3]);

    furi_mutex_release(wizard->mutex);
    return true;
}

uint8_t clone_wizard_detect_type(CloneWizard* wizard) {
    if(!wizard) return 0;

    furi_mutex_acquire(wizard->mutex, FuriWaitForever);

    // Detect based on SAK
    uint8_t sak = wizard->state.tag_data.sak;
    uint8_t tag_type = 0;

    if(sak == 0x08) {
        tag_type = 1; // MIFARE Classic 1K
        wizard->state.tag_data.blocks_count = 64;
        snprintf(
            wizard->state.status_message,
            sizeof(wizard->state.status_message),
            "Detected: MIFARE Classic 1K");
    } else if(sak == 0x18) {
        tag_type = 2; // MIFARE Classic 4K
        wizard->state.tag_data.blocks_count = 256;
        snprintf(
            wizard->state.status_message,
            sizeof(wizard->state.status_message),
            "Detected: MIFARE Classic 4K");
    } else if(sak == 0x00) {
        tag_type = 3; // MIFARE Ultralight
        wizard->state.tag_data.blocks_count = 16;
        snprintf(
            wizard->state.status_message,
            sizeof(wizard->state.status_message),
            "Detected: MIFARE Ultralight");
    } else {
        tag_type = 0; // Unknown
        snprintf(
            wizard->state.status_message,
            sizeof(wizard->state.status_message),
            "Unknown tag type (SAK: %02X)",
            sak);
    }

    wizard->state.tag_data.tag_type = tag_type;

    furi_mutex_release(wizard->mutex);
    return tag_type;
}

uint8_t clone_wizard_read_tag(
    CloneWizard* wizard,
    void (*progress_callback)(uint8_t percent, void* context),
    void* context) {
    if(!wizard) return 0;

    furi_mutex_acquire(wizard->mutex, FuriWaitForever);

    uint8_t blocks_read = 0;
    uint8_t total_blocks = wizard->state.tag_data.blocks_count;

    // Simulate reading blocks
    for(uint8_t block = 0; block < total_blocks; block++) {
        // Mock: fill with dummy data
        for(uint8_t i = 0; i < 16; i++) {
            wizard->state.tag_data.blocks[block][i] = (block + i) & 0xFF;
        }

        wizard->state.tag_data.blocks_readable[block] = true;
        blocks_read++;

        // Update progress
        uint8_t percent = (block * 100) / total_blocks;
        if(progress_callback) {
            progress_callback(percent, context);
        }

        // Small delay to simulate real reading
        furi_delay_ms(10);
    }

    snprintf(
        wizard->state.status_message,
        sizeof(wizard->state.status_message),
        "Read %d/%d blocks",
        blocks_read,
        total_blocks);

    furi_mutex_release(wizard->mutex);
    return blocks_read;
}

uint8_t clone_wizard_write_to_slot(
    CloneWizard* wizard,
    uint8_t slot,
    void (*progress_callback)(uint8_t percent, void* context),
    void* context) {
    if(!wizard || slot > 7) return 0;

    furi_mutex_acquire(wizard->mutex, FuriWaitForever);

    wizard->state.target_slot = slot;
    uint8_t blocks_written = 0;
    uint8_t total_blocks = wizard->state.tag_data.blocks_count;

    // Simulate writing blocks
    for(uint8_t block = 0; block < total_blocks; block++) {
        if(wizard->state.tag_data.blocks_readable[block]) {
            // TODO: Write block to Chameleon using protocol
            // For now, just simulate
            blocks_written++;

            // Update progress
            uint8_t percent = (block * 100) / total_blocks;
            if(progress_callback) {
                progress_callback(percent, context);
            }

            furi_delay_ms(10);
        }
    }

    snprintf(
        wizard->state.status_message,
        sizeof(wizard->state.status_message),
        "Wrote %d/%d blocks to slot %d",
        blocks_written,
        total_blocks,
        slot);

    furi_mutex_release(wizard->mutex);
    return blocks_written;
}

bool clone_wizard_validate_clone(CloneWizard* wizard) {
    if(!wizard) return false;

    furi_mutex_acquire(wizard->mutex, FuriWaitForever);

    // Simulate validation
    // In real implementation: read back from Chameleon and compare

    uint8_t similarity = clone_wizard_calculate_similarity(wizard);

    bool valid = (similarity >= 95); // 95% or better is valid

    if(valid) {
        wizard->state.result = CloneResultSuccess;
        snprintf(
            wizard->state.status_message,
            sizeof(wizard->state.status_message),
            "Clone validated: %d%% match",
            similarity);
    } else if(similarity >= 75) {
        wizard->state.result = CloneResultPartial;
        snprintf(
            wizard->state.status_message,
            sizeof(wizard->state.status_message),
            "Partial clone: %d%% match",
            similarity);
    } else {
        wizard->state.result = CloneResultFailed;
        snprintf(
            wizard->state.status_message,
            sizeof(wizard->state.status_message),
            "Clone failed: %d%% match",
            similarity);
    }

    furi_mutex_release(wizard->mutex);
    return valid;
}

bool clone_wizard_save_backup(CloneWizard* wizard, const char* filepath) {
    if(!wizard || !filepath) return false;

    Storage* storage = furi_record_open(RECORD_STORAGE);
    File* file = storage_file_alloc(storage);

    bool success = false;

    if(storage_file_open(file, filepath, FSAM_WRITE, FSOM_CREATE_ALWAYS)) {
        FuriString* backup = furi_string_alloc();

        furi_string_cat_printf(backup, "# Chameleon Ultra Clone Backup\n\n");

        // Tag info
        furi_string_cat_printf(backup, "[TAG INFO]\n");
        furi_string_cat_printf(backup, "Type: %d\n", wizard->state.tag_data.tag_type);
        furi_string_cat_printf(backup, "UID: ");
        for(uint8_t i = 0; i < wizard->state.tag_data.uid_len; i++) {
            furi_string_cat_printf(backup, "%02X", wizard->state.tag_data.uid[i]);
        }
        furi_string_cat_printf(backup, "\n");
        furi_string_cat_printf(
            backup,
            "ATQA: %02X%02X\n",
            wizard->state.tag_data.atqa[0],
            wizard->state.tag_data.atqa[1]);
        furi_string_cat_printf(backup, "SAK: %02X\n", wizard->state.tag_data.sak);
        furi_string_cat_printf(backup, "Blocks: %d\n\n", wizard->state.tag_data.blocks_count);

        // Block data
        furi_string_cat_printf(backup, "[BLOCKS]\n");
        for(uint8_t block = 0; block < wizard->state.tag_data.blocks_count; block++) {
            if(wizard->state.tag_data.blocks_readable[block]) {
                furi_string_cat_printf(backup, "Block %03d: ", block);
                for(uint8_t i = 0; i < 16; i++) {
                    furi_string_cat_printf(
                        backup,
                        "%02X",
                        wizard->state.tag_data.blocks[block][i]);
                }
                furi_string_cat_printf(backup, "\n");
            }
        }

        storage_file_write(file, furi_string_get_cstr(backup), furi_string_size(backup));

        furi_string_free(backup);
        storage_file_close(file);
        success = true;
    }

    storage_file_free(file);
    furi_record_close(RECORD_STORAGE);

    return success;
}

const char* clone_wizard_get_step_name(CloneWizardStep step) {
    if(step >= 0 && step < sizeof(step_names) / sizeof(step_names[0])) {
        return step_names[step];
    }
    return "Unknown";
}

const char* clone_wizard_get_result_name(CloneWizardResult result) {
    if(result >= 0 && result < sizeof(result_names) / sizeof(result_names[0])) {
        return result_names[result];
    }
    return "Unknown";
}

uint8_t clone_wizard_calculate_similarity(CloneWizard* wizard) {
    if(!wizard) return 0;

    // Calculate based on blocks successfully read/written
    uint8_t total = wizard->state.tag_data.blocks_count;
    uint8_t readable = 0;

    for(uint8_t i = 0; i < total; i++) {
        if(wizard->state.tag_data.blocks_readable[i]) {
            readable++;
        }
    }

    if(total == 0) return 0;

    return (readable * 100) / total;
}

bool clone_wizard_execute_step(
    CloneWizard* wizard,
    void (*callback)(uint8_t progress, const char* message, void* context),
    void* context) {
    if(!wizard) return false;

    bool success = false;
    CloneWizardStep step = wizard->state.current_step;

    switch(step) {
    case CloneStepScan: {
        uint8_t uid[10], uid_len, atqa[2], sak;
        success = clone_wizard_scan_tag(wizard, uid, &uid_len, atqa, &sak);
        break;
    }

    case CloneStepDetectType:
        success = (clone_wizard_detect_type(wizard) != 0);
        break;

    case CloneStepTestKeys:
        // Simulate key testing
        success = true;
        if(callback) callback(100, "Keys tested", context);
        break;

    case CloneStepReadData: {
        uint8_t blocks = clone_wizard_read_tag(wizard, NULL, NULL);
        success = (blocks > 0);
        break;
    }

    case CloneStepWriteData: {
        uint8_t blocks = clone_wizard_write_to_slot(wizard, wizard->state.target_slot, NULL, NULL);
        success = (blocks > 0);
        break;
    }

    case CloneStepValidate:
        success = clone_wizard_validate_clone(wizard);
        break;

    default:
        success = true;
        break;
    }

    return success;
}
