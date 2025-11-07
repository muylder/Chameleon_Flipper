#include "quick_actions.h"
#include <string.h>
#include <furi.h>
#include <storage/storage.h>

#define MACROS_FILE_PATH "/ext/apps_data/chameleon_ultra/macros.dat"
#define MACROS_FILE_VERSION 1

struct QuickActionsManager {
    QuickMacro macros[MAX_MACROS];
    size_t macro_count;
    FuriMutex* mutex;
    bool is_executing;
    uint8_t current_step;
};

// Action type names
static const char* action_names[] = {
    "Scan Tag",
    "Read Tag",
    "Write Tag",
    "Switch Slot",
    "Backup Slot",
    "Restore Slot",
    "Validate Tag",
    "Test Keys",
    "Connect Device",
    "Disconnect Device",
    "Play Sound",
    "Delay",
    "Set Mode",
};

QuickActionsManager* quick_actions_alloc(void) {
    QuickActionsManager* manager = malloc(sizeof(QuickActionsManager));

    memset(manager, 0, sizeof(QuickActionsManager));
    manager->mutex = furi_mutex_alloc(FuriMutexTypeNormal);
    manager->macro_count = 0;
    manager->is_executing = false;

    return manager;
}

void quick_actions_free(QuickActionsManager* manager) {
    if(!manager) return;

    furi_mutex_free(manager->mutex);
    free(manager);
}

bool quick_actions_save(QuickActionsManager* manager) {
    if(!manager) return false;

    furi_mutex_acquire(manager->mutex, FuriWaitForever);

    Storage* storage = furi_record_open(RECORD_STORAGE);
    File* file = storage_file_alloc(storage);

    bool success = false;

    // Ensure directory exists
    storage_common_mkdir(storage, "/ext/apps_data");
    storage_common_mkdir(storage, "/ext/apps_data/chameleon_ultra");

    if(storage_file_open(file, MACROS_FILE_PATH, FSAM_WRITE, FSOM_CREATE_ALWAYS)) {
        // Write version
        uint32_t version = MACROS_FILE_VERSION;
        storage_file_write(file, &version, sizeof(version));

        // Write macro count
        storage_file_write(file, &manager->macro_count, sizeof(manager->macro_count));

        // Write all macros
        if(manager->macro_count > 0) {
            storage_file_write(
                file,
                manager->macros,
                sizeof(QuickMacro) * manager->macro_count);
        }

        success = true;
        storage_file_close(file);
    }

    storage_file_free(file);
    furi_record_close(RECORD_STORAGE);

    furi_mutex_release(manager->mutex);

    return success;
}

size_t quick_actions_load(QuickActionsManager* manager) {
    if(!manager) return 0;

    furi_mutex_acquire(manager->mutex, FuriWaitForever);

    Storage* storage = furi_record_open(RECORD_STORAGE);
    File* file = storage_file_alloc(storage);

    size_t loaded = 0;

    if(storage_file_open(file, MACROS_FILE_PATH, FSAM_READ, FSOM_OPEN_EXISTING)) {
        // Read version
        uint32_t version = 0;
        storage_file_read(file, &version, sizeof(version));

        if(version == MACROS_FILE_VERSION) {
            // Read macro count
            size_t count = 0;
            storage_file_read(file, &count, sizeof(count));

            if(count > 0 && count <= MAX_MACROS) {
                // Read macros
                storage_file_read(file, manager->macros, sizeof(QuickMacro) * count);
                manager->macro_count = count;
                loaded = count;
            }
        }

        storage_file_close(file);
    }

    storage_file_free(file);
    furi_record_close(RECORD_STORAGE);

    furi_mutex_release(manager->mutex);

    return loaded;
}

bool quick_actions_add_macro(QuickActionsManager* manager, const QuickMacro* macro) {
    if(!manager || !macro) return false;

    furi_mutex_acquire(manager->mutex, FuriWaitForever);

    bool success = false;

    if(manager->macro_count < MAX_MACROS) {
        memcpy(&manager->macros[manager->macro_count], macro, sizeof(QuickMacro));
        manager->macro_count++;
        success = true;
    }

    furi_mutex_release(manager->mutex);

    return success;
}

bool quick_actions_remove_macro(QuickActionsManager* manager, size_t index) {
    if(!manager || index >= manager->macro_count) return false;

    furi_mutex_acquire(manager->mutex, FuriWaitForever);

    // Shift remaining macros
    for(size_t i = index; i < manager->macro_count - 1; i++) {
        memcpy(&manager->macros[i], &manager->macros[i + 1], sizeof(QuickMacro));
    }

    manager->macro_count--;

    furi_mutex_release(manager->mutex);

    return true;
}

const QuickMacro* quick_actions_get_macro(QuickActionsManager* manager, size_t index) {
    if(!manager || index >= manager->macro_count) return NULL;

    return &manager->macros[index];
}

size_t quick_actions_get_count(QuickActionsManager* manager) {
    if(!manager) return 0;
    return manager->macro_count;
}

bool quick_actions_execute_macro(
    QuickActionsManager* manager,
    size_t index,
    QuickActionProgressCallback progress_cb,
    void* context) {
    if(!manager || index >= manager->macro_count) return false;

    furi_mutex_acquire(manager->mutex, FuriWaitForever);

    QuickMacro* macro = &manager->macros[index];

    if(!macro->enabled) {
        furi_mutex_release(manager->mutex);
        return false;
    }

    manager->is_executing = true;
    bool success = true;

    // Execute each step
    for(uint8_t i = 0; i < macro->step_count; i++) {
        manager->current_step = i;
        QuickAction* action = &macro->steps[i];

        // Report progress
        if(progress_cb) {
            uint8_t percent = (i * 100) / macro->step_count;
            progress_cb(percent, action->type, context);
        }

        // Execute action (mock implementation - actual implementation would call real functions)
        switch(action->type) {
        case ActionTypeScanTag:
            // TODO: Call actual scan function
            furi_delay_ms(100);
            break;

        case ActionTypeReadTag:
            // TODO: Call actual read function
            furi_delay_ms(200);
            break;

        case ActionTypeWriteTag:
            // TODO: Call actual write function
            furi_delay_ms(300);
            break;

        case ActionTypeSwitchSlot:
            // TODO: Call slot switch function with action->param1
            furi_delay_ms(50);
            break;

        case ActionTypeBackupSlot:
            // TODO: Call backup function with action->param1
            furi_delay_ms(150);
            break;

        case ActionTypeRestoreSlot:
            // TODO: Call restore function with action->param1
            furi_delay_ms(150);
            break;

        case ActionTypeValidateTag:
            // TODO: Call validation function
            furi_delay_ms(200);
            break;

        case ActionTypeTestKeys:
            // TODO: Call key test function
            furi_delay_ms(500);
            break;

        case ActionTypeConnectDevice:
            // TODO: Call connection function
            furi_delay_ms(100);
            break;

        case ActionTypeDisconnectDevice:
            // TODO: Call disconnection function
            furi_delay_ms(50);
            break;

        case ActionTypePlaySound:
            // TODO: Call sound function with action->param1 (sound ID)
            furi_delay_ms(10);
            break;

        case ActionTypeDelay:
            // Delay for param1 * 100 ms
            furi_delay_ms(action->param1 * 100);
            break;

        case ActionTypeSetMode:
            // TODO: Call mode set function with action->param1
            furi_delay_ms(50);
            break;

        default:
            success = false;
            break;
        }

        if(!success) break;
    }

    // Final progress
    if(progress_cb && success) {
        progress_cb(100, ActionTypeScanTag, context);
    }

    // Update use count
    macro->use_count++;

    manager->is_executing = false;
    manager->current_step = 0;

    furi_mutex_release(manager->mutex);

    return success;
}

bool quick_actions_is_executing(QuickActionsManager* manager) {
    if(!manager) return false;
    return manager->is_executing;
}

uint8_t quick_actions_get_current_step(QuickActionsManager* manager) {
    if(!manager) return 0;
    return manager->current_step;
}

bool quick_actions_update_macro(
    QuickActionsManager* manager,
    size_t index,
    const QuickMacro* macro) {
    if(!manager || !macro || index >= manager->macro_count) return false;

    furi_mutex_acquire(manager->mutex, FuriWaitForever);

    memcpy(&manager->macros[index], macro, sizeof(QuickMacro));

    furi_mutex_release(manager->mutex);

    return true;
}

bool quick_actions_toggle_enabled(QuickActionsManager* manager, size_t index) {
    if(!manager || index >= manager->macro_count) return false;

    furi_mutex_acquire(manager->mutex, FuriWaitForever);

    manager->macros[index].enabled = !manager->macros[index].enabled;
    bool new_state = manager->macros[index].enabled;

    furi_mutex_release(manager->mutex);

    return new_state;
}

const char* quick_actions_get_action_name(ActionType type) {
    if(type >= 0 && type < (sizeof(action_names) / sizeof(action_names[0]))) {
        return action_names[type];
    }
    return "Unknown";
}

// Preset macro creators
QuickMacro quick_actions_create_quick_clone_preset(void) {
    QuickMacro macro = {0};

    strncpy(macro.name, "Quick Clone", sizeof(macro.name) - 1);
    macro.enabled = true;
    macro.use_count = 0;
    macro.step_count = 5;

    // Step 1: Scan tag
    macro.steps[0].type = ActionTypeScanTag;
    strncpy(macro.steps[0].description, "Scan original tag", sizeof(macro.steps[0].description) - 1);

    // Step 2: Read tag
    macro.steps[1].type = ActionTypeReadTag;
    strncpy(macro.steps[1].description, "Read all blocks", sizeof(macro.steps[1].description) - 1);

    // Step 3: Switch to slot 1
    macro.steps[2].type = ActionTypeSwitchSlot;
    macro.steps[2].param1 = 1;
    strncpy(macro.steps[2].description, "Switch to slot 1", sizeof(macro.steps[2].description) - 1);

    // Step 4: Write tag
    macro.steps[3].type = ActionTypeWriteTag;
    strncpy(macro.steps[3].description, "Write to Chameleon", sizeof(macro.steps[3].description) - 1);

    // Step 5: Validate
    macro.steps[4].type = ActionTypeValidateTag;
    strncpy(macro.steps[4].description, "Validate clone", sizeof(macro.steps[4].description) - 1);

    return macro;
}

QuickMacro quick_actions_create_backup_all_preset(void) {
    QuickMacro macro = {0};

    strncpy(macro.name, "Backup All Slots", sizeof(macro.name) - 1);
    macro.enabled = true;
    macro.use_count = 0;
    macro.step_count = 8;

    // Backup all 8 slots
    for(uint8_t i = 0; i < 8; i++) {
        macro.steps[i].type = ActionTypeBackupSlot;
        macro.steps[i].param1 = i;
        snprintf(
            macro.steps[i].description,
            sizeof(macro.steps[i].description),
            "Backup slot %d",
            i);
    }

    return macro;
}

QuickMacro quick_actions_create_test_tag_preset(void) {
    QuickMacro macro = {0};

    strncpy(macro.name, "Test Tag", sizeof(macro.name) - 1);
    macro.enabled = true;
    macro.use_count = 0;
    macro.step_count = 4;

    // Step 1: Scan
    macro.steps[0].type = ActionTypeScanTag;
    strncpy(macro.steps[0].description, "Scan tag", sizeof(macro.steps[0].description) - 1);

    // Step 2: Test keys
    macro.steps[1].type = ActionTypeTestKeys;
    strncpy(macro.steps[1].description, "Test all keys", sizeof(macro.steps[1].description) - 1);

    // Step 3: Read
    macro.steps[2].type = ActionTypeReadTag;
    strncpy(macro.steps[2].description, "Read accessible blocks", sizeof(macro.steps[2].description) - 1);

    // Step 4: Validate
    macro.steps[3].type = ActionTypeValidateTag;
    strncpy(macro.steps[3].description, "Validate tag structure", sizeof(macro.steps[3].description) - 1);

    return macro;
}

QuickMacro quick_actions_create_quick_deploy_preset(uint8_t slot) {
    QuickMacro macro = {0};

    snprintf(macro.name, sizeof(macro.name), "Deploy Slot %d", slot);
    macro.enabled = true;
    macro.use_count = 0;
    macro.step_count = 3;

    // Step 1: Connect
    macro.steps[0].type = ActionTypeConnectDevice;
    strncpy(macro.steps[0].description, "Connect to Chameleon", sizeof(macro.steps[0].description) - 1);

    // Step 2: Switch slot
    macro.steps[1].type = ActionTypeSwitchSlot;
    macro.steps[1].param1 = slot;
    snprintf(
        macro.steps[1].description,
        sizeof(macro.steps[1].description),
        "Switch to slot %d",
        slot);

    // Step 3: Set active mode
    macro.steps[2].type = ActionTypeSetMode;
    macro.steps[2].param1 = 1; // Active mode
    strncpy(macro.steps[2].description, "Activate emulation", sizeof(macro.steps[2].description) - 1);

    return macro;
}
