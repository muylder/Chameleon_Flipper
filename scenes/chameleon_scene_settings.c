#include "../chameleon_app_i.h"

typedef enum {
    SettingsItemSound,
    SettingsItemHaptic,
    SettingsItemAnimations,
    SettingsItemAutoReconnect,
    SettingsItemDebugLogging,
    SettingsItemResetDefaults,
} SettingsItem;

static void settings_sound_callback(VariableItem* item) {
    ChameleonApp* app = variable_item_get_context(item);
    ChameleonSettings* settings = settings_manager_get(app->settings_manager);

    uint8_t index = variable_item_get_current_value_index(item);
    settings->sound_enabled = (index == 1);

    variable_item_set_current_value_text(item, settings->sound_enabled ? "ON" : "OFF");

    sound_effects_click();
    settings_manager_save(app->settings_manager);

    CHAM_LOG_I(app->logger, "Settings", "Sound: %s", settings->sound_enabled ? "ON" : "OFF");
}

static void settings_haptic_callback(VariableItem* item) {
    ChameleonApp* app = variable_item_get_context(item);
    ChameleonSettings* settings = settings_manager_get(app->settings_manager);

    uint8_t index = variable_item_get_current_value_index(item);
    settings->haptic_enabled = (index == 1);

    variable_item_set_current_value_text(item, settings->haptic_enabled ? "ON" : "OFF");

    if(settings->haptic_enabled) {
        sound_effects_haptic_medium();
    }
    settings_manager_save(app->settings_manager);

    CHAM_LOG_I(app->logger, "Settings", "Haptic: %s", settings->haptic_enabled ? "ON" : "OFF");
}

static void settings_animations_callback(VariableItem* item) {
    ChameleonApp* app = variable_item_get_context(item);
    ChameleonSettings* settings = settings_manager_get(app->settings_manager);

    uint8_t index = variable_item_get_current_value_index(item);
    settings->animations_enabled = (index == 1);

    variable_item_set_current_value_text(item, settings->animations_enabled ? "ON" : "OFF");

    sound_effects_click();
    settings_manager_save(app->settings_manager);

    CHAM_LOG_I(app->logger, "Settings", "Animations: %s", settings->animations_enabled ? "ON" : "OFF");
}

static void settings_auto_reconnect_callback(VariableItem* item) {
    ChameleonApp* app = variable_item_get_context(item);
    ChameleonSettings* settings = settings_manager_get(app->settings_manager);

    uint8_t index = variable_item_get_current_value_index(item);
    settings->auto_reconnect = (index == 1);

    variable_item_set_current_value_text(item, settings->auto_reconnect ? "ON" : "OFF");

    sound_effects_click();
    settings_manager_save(app->settings_manager);

    CHAM_LOG_I(app->logger, "Settings", "Auto-reconnect: %s", settings->auto_reconnect ? "ON" : "OFF");
}

static void settings_debug_logging_callback(VariableItem* item) {
    ChameleonApp* app = variable_item_get_context(item);
    ChameleonSettings* settings = settings_manager_get(app->settings_manager);

    uint8_t index = variable_item_get_current_value_index(item);
    settings->debug_logging = (index == 1);

    variable_item_set_current_value_text(item, settings->debug_logging ? "ON" : "OFF");

    sound_effects_click();
    settings_manager_save(app->settings_manager);

    CHAM_LOG_I(app->logger, "Settings", "Debug logging: %s", settings->debug_logging ? "ON" : "OFF");
}

static void settings_reset_callback(VariableItem* item) {
    ChameleonApp* app = variable_item_get_context(item);

    sound_effects_warning();

    // Reset to defaults
    settings_manager_reset_defaults(app->settings_manager);
    settings_manager_save(app->settings_manager);

    CHAM_LOG_I(app->logger, "Settings", "Reset to defaults");

    // Refresh display
    scene_manager_previous_scene(app->scene_manager);
    scene_manager_next_scene(app->scene_manager, ChameleonSceneSettings);

    UNUSED(item);
}

void chameleon_scene_settings_on_enter(void* context) {
    ChameleonApp* app = context;
    VariableItemList* variable_item_list = app->variable_item_list;
    ChameleonSettings* settings = settings_manager_get(app->settings_manager);

    variable_item_list_reset(variable_item_list);
    variable_item_list_set_header(variable_item_list, "Settings");

    VariableItem* item;

    // Sound
    item = variable_item_list_add(
        variable_item_list,
        "Sound Effects",
        2,  // OFF/ON
        settings_sound_callback,
        app);
    variable_item_set_current_value_index(item, settings->sound_enabled ? 1 : 0);
    variable_item_set_current_value_text(item, settings->sound_enabled ? "ON" : "OFF");

    // Haptic
    item = variable_item_list_add(
        variable_item_list,
        "Haptic Feedback",
        2,  // OFF/ON
        settings_haptic_callback,
        app);
    variable_item_set_current_value_index(item, settings->haptic_enabled ? 1 : 0);
    variable_item_set_current_value_text(item, settings->haptic_enabled ? "ON" : "OFF");

    // Animations
    item = variable_item_list_add(
        variable_item_list,
        "Animations",
        2,  // OFF/ON
        settings_animations_callback,
        app);
    variable_item_set_current_value_index(item, settings->animations_enabled ? 1 : 0);
    variable_item_set_current_value_text(item, settings->animations_enabled ? "ON" : "OFF");

    // Auto-reconnect
    item = variable_item_list_add(
        variable_item_list,
        "Auto-Reconnect",
        2,  // OFF/ON
        settings_auto_reconnect_callback,
        app);
    variable_item_set_current_value_index(item, settings->auto_reconnect ? 1 : 0);
    variable_item_set_current_value_text(item, settings->auto_reconnect ? "ON" : "OFF");

    // Debug logging
    item = variable_item_list_add(
        variable_item_list,
        "Debug Logging",
        2,  // OFF/ON
        settings_debug_logging_callback,
        app);
    variable_item_set_current_value_index(item, settings->debug_logging ? 1 : 0);
    variable_item_set_current_value_text(item, settings->debug_logging ? "ON" : "OFF");

    // Reset to defaults
    item = variable_item_list_add(
        variable_item_list,
        "Reset to Defaults",
        1,  // Single button
        settings_reset_callback,
        app);
    variable_item_set_current_value_text(item, "RESET");

    view_dispatcher_switch_to_view(app->view_dispatcher, ChameleonViewVariableItemList);

    CHAM_LOG_I(app->logger, "Settings", "Settings menu opened");
}

bool chameleon_scene_settings_on_event(void* context, SceneManagerEvent event) {
    UNUSED(context);
    UNUSED(event);
    return false;
}

void chameleon_scene_settings_on_exit(void* context) {
    ChameleonApp* app = context;
    variable_item_list_reset(app->variable_item_list);
}
