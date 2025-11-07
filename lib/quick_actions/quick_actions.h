#pragma once

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Quick Actions and Macros System
 *
 * Pre-defined and custom action sequences for one-tap operations
 */

#define MAX_MACRO_STEPS 10
#define MAX_MACROS 20
#define MACRO_NAME_MAX_LEN 32

// Action types
typedef enum {
    ActionTypeNone,
    ActionTypeScanTag,
    ActionTypeReadTag,
    ActionTypeWriteTag,
    ActionTypeSwitchSlot,
    ActionTypeBackupSlot,
    ActionTypeRestoreSlot,
    ActionTypeValidateTag,
    ActionTypeTestKeys,
    ActionTypeConnectDevice,
    ActionTypeDisconnectDevice,
    ActionTypePlaySound,
    ActionTypeDelay,
} ActionType;

// Single action
typedef struct {
    ActionType type;
    uint8_t param1;  // Slot number, delay ms, etc.
    uint8_t param2;  // Additional parameter
    char description[32];
} QuickAction;

// Macro (sequence of actions)
typedef struct {
    char name[MACRO_NAME_MAX_LEN];
    QuickAction steps[MAX_MACRO_STEPS];
    uint8_t step_count;
    bool enabled;
    uint32_t use_count;
} QuickMacro;

// Quick actions manager
typedef struct QuickActionsManager QuickActionsManager;

/**
 * @brief Allocate quick actions manager
 * @return Manager instance
 */
QuickActionsManager* quick_actions_alloc(void);

/**
 * @brief Free quick actions manager
 * @param manager Manager instance
 */
void quick_actions_free(QuickActionsManager* manager);

/**
 * @brief Load macros from storage
 * @param manager Manager instance
 * @return Number of macros loaded
 */
size_t quick_actions_load(QuickActionsManager* manager);

/**
 * @brief Save macros to storage
 * @param manager Manager instance
 * @return true if saved successfully
 */
bool quick_actions_save(QuickActionsManager* manager);

/**
 * @brief Add macro
 * @param manager Manager instance
 * @param macro Macro to add
 * @return true if added successfully
 */
bool quick_actions_add_macro(QuickActionsManager* manager, const QuickMacro* macro);

/**
 * @brief Get macro by index
 * @param manager Manager instance
 * @param index Macro index
 * @return Pointer to macro, or NULL
 */
const QuickMacro* quick_actions_get_macro(QuickActionsManager* manager, size_t index);

/**
 * @brief Get macro count
 * @param manager Manager instance
 * @return Number of macros
 */
size_t quick_actions_get_count(QuickActionsManager* manager);

/**
 * @brief Execute macro
 * @param manager Manager instance
 * @param index Macro index
 * @param callback Progress callback
 * @param context Callback context
 * @return true if executed successfully
 */
bool quick_actions_execute_macro(
    QuickActionsManager* manager,
    size_t index,
    void (*callback)(const char* status, void* context),
    void* context);

/**
 * @brief Create preset macros
 * @param manager Manager instance
 * @return Number of presets created
 */
size_t quick_actions_create_presets(QuickActionsManager* manager);

/**
 * @brief Get action type name
 * @param type Action type
 * @return Action name string
 */
const char* quick_actions_get_action_name(ActionType type);

// Preset macro creators
QuickMacro quick_actions_create_quick_clone_macro(void);
QuickMacro quick_actions_create_backup_all_macro(void);
QuickMacro quick_actions_create_test_tag_macro(void);
QuickMacro quick_actions_create_quick_deploy_macro(uint8_t slot);
