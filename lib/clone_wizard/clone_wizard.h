#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <furi.h>

/**
 * @brief Tag Cloning Wizard
 *
 * Step-by-step wizard for cloning tags from real to Chameleon
 */

// Cloning wizard steps
typedef enum {
    CloneStepScan,           // Scan the tag
    CloneStepDetectType,     // Detect tag type
    CloneStepTestKeys,       // Test authentication keys
    CloneStepReadData,       // Read all blocks
    CloneStepSelectSlot,     // Choose target slot
    CloneStepWriteData,      // Write to Chameleon
    CloneStepValidate,       // Validate clone
    CloneStepComplete,       // Done
} CloneWizardStep;

// Clone result
typedef enum {
    CloneResultSuccess,
    CloneResultPartial,      // Partial success (some blocks failed)
    CloneResultFailed,
    CloneResultCancelled,
} CloneWizardResult;

// Tag data for cloning
typedef struct {
    uint8_t uid[10];
    uint8_t uid_len;
    uint8_t atqa[2];
    uint8_t sak;
    uint8_t tag_type;        // Detected type
    uint8_t blocks[256][16]; // Block data
    uint8_t blocks_count;
    bool blocks_readable[256]; // Which blocks were successfully read
    uint8_t found_keys_a[40][6]; // Key A for each sector
    uint8_t found_keys_b[40][6]; // Key B for each sector
    bool keys_found[40];     // Which keys were found
} CloneTagData;

// Cloning wizard state
typedef struct {
    CloneWizardStep current_step;
    CloneTagData tag_data;
    uint8_t target_slot;
    uint8_t progress_percent;
    CloneWizardResult result;
    char status_message[128];
    uint32_t start_time;
    uint32_t total_duration;
} CloneWizardState;

// Clone wizard instance
typedef struct CloneWizard CloneWizard;

/**
 * @brief Allocate clone wizard
 * @return CloneWizard instance
 */
CloneWizard* clone_wizard_alloc(void);

/**
 * @brief Free clone wizard
 * @param wizard Wizard instance
 */
void clone_wizard_free(CloneWizard* wizard);

/**
 * @brief Reset wizard to initial state
 * @param wizard Wizard instance
 */
void clone_wizard_reset(CloneWizard* wizard);

/**
 * @brief Get current wizard state
 * @param wizard Wizard instance
 * @return Current state
 */
const CloneWizardState* clone_wizard_get_state(CloneWizard* wizard);

/**
 * @brief Advance to next step
 * @param wizard Wizard instance
 * @return true if advanced successfully
 */
bool clone_wizard_next_step(CloneWizard* wizard);

/**
 * @brief Execute current step
 * @param wizard Wizard instance
 * @param callback Progress callback (optional)
 * @param context Callback context
 * @return true if step completed successfully
 */
bool clone_wizard_execute_step(
    CloneWizard* wizard,
    void (*callback)(uint8_t progress, const char* message, void* context),
    void* context);

/**
 * @brief Scan tag (Step 1)
 * @param wizard Wizard instance
 * @param uid Output UID
 * @param uid_len Output UID length
 * @param atqa Output ATQA
 * @param sak Output SAK
 * @return true if tag scanned
 */
bool clone_wizard_scan_tag(
    CloneWizard* wizard,
    uint8_t* uid,
    uint8_t* uid_len,
    uint8_t* atqa,
    uint8_t* sak);

/**
 * @brief Detect tag type (Step 2)
 * @param wizard Wizard instance
 * @return Detected tag type
 */
uint8_t clone_wizard_detect_type(CloneWizard* wizard);

/**
 * @brief Test keys and read data (Steps 3-4)
 * @param wizard Wizard instance
 * @param progress_callback Progress callback
 * @param context Callback context
 * @return Number of blocks successfully read
 */
uint8_t clone_wizard_read_tag(
    CloneWizard* wizard,
    void (*progress_callback)(uint8_t percent, void* context),
    void* context);

/**
 * @brief Write data to Chameleon slot (Step 6)
 * @param wizard Wizard instance
 * @param slot Target slot number
 * @param progress_callback Progress callback
 * @param context Callback context
 * @return Number of blocks successfully written
 */
uint8_t clone_wizard_write_to_slot(
    CloneWizard* wizard,
    uint8_t slot,
    void (*progress_callback)(uint8_t percent, void* context),
    void* context);

/**
 * @brief Validate clone (Step 7)
 * @param wizard Wizard instance
 * @return true if clone is valid
 */
bool clone_wizard_validate_clone(CloneWizard* wizard);

/**
 * @brief Save clone backup to file
 * @param wizard Wizard instance
 * @param filepath Output file path
 * @return true if saved successfully
 */
bool clone_wizard_save_backup(CloneWizard* wizard, const char* filepath);

/**
 * @brief Get step name
 * @param step Step enum
 * @return Step name string
 */
const char* clone_wizard_get_step_name(CloneWizardStep step);

/**
 * @brief Get result name
 * @param result Result enum
 * @return Result name string
 */
const char* clone_wizard_get_result_name(CloneWizardResult result);

/**
 * @brief Calculate clone similarity percentage
 * @param wizard Wizard instance
 * @return Similarity percentage (0-100)
 */
uint8_t clone_wizard_calculate_similarity(CloneWizard* wizard);
