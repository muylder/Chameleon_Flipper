#include "../chameleon_app_i.h"
#include "../lib/dictionary_attack/dictionary_attack.h"
#include "../lib/sound_effects/sound_effects.h"
#include <gui/modules/widget.h>

void chameleon_scene_dictionary_attack_on_enter(void* context) {
    ChameleonApp* app = context;
    Widget* widget = app->widget;

    widget_reset(widget);

    FuriString* attack_display = furi_string_alloc();

    furi_string_cat_printf(attack_display, "=== DICTIONARY ATTACK ===\n\n");

    // Create attack manager
    DictionaryAttack* attack = dictionary_attack_alloc();

    // Load default keys
    dictionary_attack_load_default_keys(attack);

    furi_string_cat_printf(
        attack_display,
        "Wordlist: %zu keys\n\n",
        dictionary_attack_get_wordlist_size(attack));

    // Show some default keys
    furi_string_cat_printf(attack_display, "Default Keys:\n");
    for(size_t i = 0; i < 6; i++) {
        const KeyEntry* key = dictionary_attack_get_key(attack, i);
        if(key) {
            furi_string_cat_printf(
                attack_display,
                "%02X%02X%02X%02X%02X%02X\n",
                key->key[0], key->key[1], key->key[2],
                key->key[3], key->key[4], key->key[5]);
        }
    }

    furi_string_cat_printf(attack_display, "...and %zu more\n\n",
        dictionary_attack_get_wordlist_size(attack) - 6);

    // Configure attack
    dictionary_attack_set_mode(attack, AttackModeBoth);
    dictionary_attack_set_type(attack, AttackTypeDictionary);
    dictionary_attack_set_all_sectors(attack, true); // 1K card

    furi_string_cat_printf(attack_display, "[DEMO ATTACK]\n");
    furi_string_cat_printf(attack_display, "Mode: Both Keys\n");
    furi_string_cat_printf(attack_display, "Target: All sectors\n");
    furi_string_cat_printf(attack_display, "Type: Dictionary\n\n");

    // Simulate attack execution
    furi_string_cat_printf(attack_display, "Running attack...\n\n");

    // Execute (mock)
    dictionary_attack_start(attack, NULL, NULL);

    const AttackStatistics* stats = dictionary_attack_get_statistics(attack);

    furi_string_cat_printf(attack_display, "=== RESULTS ===\n");
    furi_string_cat_printf(attack_display, "Status: %s\n",
        dictionary_attack_get_status_name(dictionary_attack_get_status(attack)));
    furi_string_cat_printf(attack_display, "Keys found: %lu\n", stats->keys_found);
    furi_string_cat_printf(attack_display, "Keys failed: %lu\n", stats->keys_failed);
    furi_string_cat_printf(attack_display, "Total attempts: %lu\n", stats->total_attempts);
    furi_string_cat_printf(attack_display, "Success rate: %.1f%%\n", stats->success_rate);
    furi_string_cat_printf(attack_display, "Time: %lu ms\n", stats->elapsed_time_ms);
    furi_string_cat_printf(attack_display, "Speed: %.1f keys/s\n\n", stats->keys_per_second);

    // Show found keys for first 3 sectors
    furi_string_cat_printf(attack_display, "Found Keys:\n");
    for(uint8_t i = 0; i < 3; i++) {
        const SectorAttackResult* result = dictionary_attack_get_sector_result(attack, i);
        if(result && result->key_a_found) {
            furi_string_cat_printf(
                attack_display,
                "S%02d A: %02X%02X%02X%02X%02X%02X\n",
                i,
                result->key_a[0], result->key_a[1],
                result->key_a[2], result->key_a[3],
                result->key_a[4], result->key_a[5]);
        }
        if(result && result->key_b_found) {
            furi_string_cat_printf(
                attack_display,
                "S%02d B: %02X%02X%02X%02X%02X%02X\n",
                i,
                result->key_b[0], result->key_b[1],
                result->key_b[2], result->key_b[3],
                result->key_b[4], result->key_b[5]);
        }
    }

    furi_string_cat_printf(attack_display, "\nPress OK to continue");

    widget_add_text_scroll_element(
        widget, 0, 0, 128, 64, furi_string_get_cstr(attack_display));

    furi_string_free(attack_display);
    dictionary_attack_free(attack);

    sound_effects_success();

    view_dispatcher_switch_to_view(app->view_dispatcher, ChameleonViewWidget);
}

bool chameleon_scene_dictionary_attack_on_event(void* context, SceneManagerEvent event) {
    UNUSED(context);
    UNUSED(event);
    return false;
}

void chameleon_scene_dictionary_attack_on_exit(void* context) {
    ChameleonApp* app = context;
    widget_reset(app->widget);
}
