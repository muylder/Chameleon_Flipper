#include "../chameleon_app_i.h"
#include "../lib/sound_effects/sound_effects.h"
#include <gui/modules/variable_item_list.h>

// Advanced slot settings
typedef struct {
    uint8_t slot_number;
    bool hf_enabled;
    bool lf_enabled;
    uint8_t hf_tag_type;
    uint8_t lf_tag_type;
    uint8_t emulation_mode; // 0=Auto, 1=Manual, 2=Passive
    bool anti_collision;
    uint8_t response_delay; // 0-9 (0=instant, 9=slowest)
    bool random_uid;
    uint8_t button_action; // 0=None, 1=Switch Slot, 2=Toggle HF/LF
} SlotAdvancedConfig;

static SlotAdvancedConfig config;

// Tag type names
static const char* hf_tag_types[] = {
    "None",
    "MIFARE Classic 1K",
    "MIFARE Classic 4K",
    "MIFARE Ultralight",
    "NTAG213/215/216",
    "DESFire",
};

static const char* lf_tag_types[] = {
    "None",
    "EM410x",
    "HID Prox",
    "T5577",
};

static const char* emulation_modes[] = {
    "Auto",
    "Manual",
    "Passive",
};

static const char* button_actions[] = {
    "None",
    "Switch Slot",
    "Toggle HF/LF",
};

// Variable item callbacks
static void hf_enable_callback(VariableItem* item) {
    ChameleonApp* app = variable_item_get_context(item);
    uint8_t index = variable_item_get_current_value_index(item);
    config.hf_enabled = (index == 1);
    variable_item_set_current_value_text(item, config.hf_enabled ? "ON" : "OFF");
    sound_effects_click();
    CHAM_LOG_D(app->logger, "SlotAdv", "HF enabled: %d", config.hf_enabled);
}

static void lf_enable_callback(VariableItem* item) {
    ChameleonApp* app = variable_item_get_context(item);
    uint8_t index = variable_item_get_current_value_index(item);
    config.lf_enabled = (index == 1);
    variable_item_set_current_value_text(item, config.lf_enabled ? "ON" : "OFF");
    sound_effects_click();
    CHAM_LOG_D(app->logger, "SlotAdv", "LF enabled: %d", config.lf_enabled);
}

static void hf_tag_type_callback(VariableItem* item) {
    ChameleonApp* app = variable_item_get_context(item);
    uint8_t index = variable_item_get_current_value_index(item);
    config.hf_tag_type = index;
    variable_item_set_current_value_text(item, hf_tag_types[index]);
    sound_effects_click();
    CHAM_LOG_D(app->logger, "SlotAdv", "HF type: %d", config.hf_tag_type);
}

static void lf_tag_type_callback(VariableItem* item) {
    ChameleonApp* app = variable_item_get_context(item);
    uint8_t index = variable_item_get_current_value_index(item);
    config.lf_tag_type = index;
    variable_item_set_current_value_text(item, lf_tag_types[index]);
    sound_effects_click();
    CHAM_LOG_D(app->logger, "SlotAdv", "LF type: %d", config.lf_tag_type);
}

static void emulation_mode_callback(VariableItem* item) {
    ChameleonApp* app = variable_item_get_context(item);
    uint8_t index = variable_item_get_current_value_index(item);
    config.emulation_mode = index;
    variable_item_set_current_value_text(item, emulation_modes[index]);
    sound_effects_click();
    CHAM_LOG_D(app->logger, "SlotAdv", "Emulation mode: %d", config.emulation_mode);
}

static void anti_collision_callback(VariableItem* item) {
    ChameleonApp* app = variable_item_get_context(item);
    uint8_t index = variable_item_get_current_value_index(item);
    config.anti_collision = (index == 1);
    variable_item_set_current_value_text(item, config.anti_collision ? "ON" : "OFF");
    sound_effects_click();
    CHAM_LOG_D(app->logger, "SlotAdv", "Anti-collision: %d", config.anti_collision);
}

static void response_delay_callback(VariableItem* item) {
    ChameleonApp* app = variable_item_get_context(item);
    uint8_t index = variable_item_get_current_value_index(item);
    config.response_delay = index;

    char delay_str[16];
    if(index == 0) {
        snprintf(delay_str, sizeof(delay_str), "Instant");
    } else {
        snprintf(delay_str, sizeof(delay_str), "%d ms", index * 10);
    }
    variable_item_set_current_value_text(item, delay_str);
    sound_effects_click();
    CHAM_LOG_D(app->logger, "SlotAdv", "Response delay: %d", config.response_delay);
}

static void random_uid_callback(VariableItem* item) {
    ChameleonApp* app = variable_item_get_context(item);
    uint8_t index = variable_item_get_current_value_index(item);
    config.random_uid = (index == 1);
    variable_item_set_current_value_text(item, config.random_uid ? "ON" : "OFF");
    sound_effects_click();
    CHAM_LOG_D(app->logger, "SlotAdv", "Random UID: %d", config.random_uid);
}

static void button_action_callback(VariableItem* item) {
    ChameleonApp* app = variable_item_get_context(item);
    uint8_t index = variable_item_get_current_value_index(item);
    config.button_action = index;
    variable_item_set_current_value_text(item, button_actions[index]);
    sound_effects_click();
    CHAM_LOG_D(app->logger, "SlotAdv", "Button action: %d", config.button_action);
}

void chameleon_scene_slot_advanced_on_enter(void* context) {
    ChameleonApp* app = context;
    VariableItemList* var_item_list = app->var_item_list;

    // Initialize config from current slot (or defaults)
    config.slot_number = app->active_slot;
    config.hf_enabled = app->slots[app->active_slot].hf_enabled;
    config.lf_enabled = app->slots[app->active_slot].lf_enabled;
    config.hf_tag_type = app->slots[app->active_slot].hf_tag_type;
    config.lf_tag_type = app->slots[app->active_slot].lf_tag_type;
    config.emulation_mode = 0; // Default to Auto
    config.anti_collision = true; // Default enabled
    config.response_delay = 0; // Default instant
    config.random_uid = false; // Default off
    config.button_action = 0; // Default none

    variable_item_list_reset(var_item_list);
    variable_item_list_set_header(var_item_list, "Advanced Slot Config");

    VariableItem* item;

    // HF Enable
    item = variable_item_list_add(var_item_list, "HF Enable", 2, hf_enable_callback, app);
    variable_item_set_current_value_index(item, config.hf_enabled ? 1 : 0);
    variable_item_set_current_value_text(item, config.hf_enabled ? "ON" : "OFF");

    // LF Enable
    item = variable_item_list_add(var_item_list, "LF Enable", 2, lf_enable_callback, app);
    variable_item_set_current_value_index(item, config.lf_enabled ? 1 : 0);
    variable_item_set_current_value_text(item, config.lf_enabled ? "ON" : "OFF");

    // HF Tag Type
    item = variable_item_list_add(
        var_item_list,
        "HF Tag Type",
        sizeof(hf_tag_types) / sizeof(hf_tag_types[0]),
        hf_tag_type_callback,
        app);
    variable_item_set_current_value_index(item, config.hf_tag_type);
    variable_item_set_current_value_text(item, hf_tag_types[config.hf_tag_type]);

    // LF Tag Type
    item = variable_item_list_add(
        var_item_list,
        "LF Tag Type",
        sizeof(lf_tag_types) / sizeof(lf_tag_types[0]),
        lf_tag_type_callback,
        app);
    variable_item_set_current_value_index(item, config.lf_tag_type);
    variable_item_set_current_value_text(item, lf_tag_types[config.lf_tag_type]);

    // Emulation Mode
    item = variable_item_list_add(
        var_item_list,
        "Emulation Mode",
        sizeof(emulation_modes) / sizeof(emulation_modes[0]),
        emulation_mode_callback,
        app);
    variable_item_set_current_value_index(item, config.emulation_mode);
    variable_item_set_current_value_text(item, emulation_modes[config.emulation_mode]);

    // Anti-Collision
    item =
        variable_item_list_add(var_item_list, "Anti-Collision", 2, anti_collision_callback, app);
    variable_item_set_current_value_index(item, config.anti_collision ? 1 : 0);
    variable_item_set_current_value_text(item, config.anti_collision ? "ON" : "OFF");

    // Response Delay
    item =
        variable_item_list_add(var_item_list, "Response Delay", 10, response_delay_callback, app);
    variable_item_set_current_value_index(item, config.response_delay);
    variable_item_set_current_value_text(item, config.response_delay == 0 ? "Instant" : "0 ms");

    // Random UID
    item = variable_item_list_add(var_item_list, "Random UID", 2, random_uid_callback, app);
    variable_item_set_current_value_index(item, config.random_uid ? 1 : 0);
    variable_item_set_current_value_text(item, config.random_uid ? "ON" : "OFF");

    // Button Action
    item = variable_item_list_add(
        var_item_list,
        "Button Action",
        sizeof(button_actions) / sizeof(button_actions[0]),
        button_action_callback,
        app);
    variable_item_set_current_value_index(item, config.button_action);
    variable_item_set_current_value_text(item, button_actions[config.button_action]);

    view_dispatcher_switch_to_view(app->view_dispatcher, ChameleonViewVarItemList);
}

bool chameleon_scene_slot_advanced_on_event(void* context, SceneManagerEvent event) {
    UNUSED(context);
    UNUSED(event);
    return false;
}

void chameleon_scene_slot_advanced_on_exit(void* context) {
    ChameleonApp* app = context;
    variable_item_list_reset(app->var_item_list);

    // Save configuration (in real implementation, send to device)
    CHAM_LOG_I(
        app->logger,
        "SlotAdv",
        "Advanced config saved for slot %d",
        config.slot_number);
}
