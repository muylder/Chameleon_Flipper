#include "../chameleon_app_i.h"
#include "../lib/tag_collection/tag_collection.h"
#include "../lib/sound_effects/sound_effects.h"
#include <gui/modules/widget.h>

void chameleon_scene_tag_collection_on_enter(void* context) {
    ChameleonApp* app = context;
    Widget* widget = app->widget;

    widget_reset(widget);

    // Create tag collection demo
    TagCollection* collection = tag_collection_alloc();
    tag_collection_load(collection);

    FuriString* collection_display = furi_string_alloc();

    furi_string_cat_printf(collection_display, "=== TAG COLLECTION ===\n\n");

    size_t tag_count = tag_collection_get_count(collection);

    if(tag_count == 0) {
        // Demo: Add sample tags
        CollectionTag sample_tags[3] = {0};

        // Hotel key card
        strncpy(sample_tags[0].name, "Hotel Room 305", TAG_NAME_MAX_LEN - 1);
        sample_tags[0].category = TagCategoryHotel;
        strncpy(sample_tags[0].category_name, "Hotel", TAG_CATEGORY_MAX_LEN - 1);
        strncpy(sample_tags[0].notes, "Vacation hotel key", TAG_NOTES_MAX_LEN - 1);
        sample_tags[0].uid_len = 4;
        sample_tags[0].uid[0] = 0x04;
        sample_tags[0].uid[1] = 0xAB;
        sample_tags[0].uid[2] = 0xCD;
        sample_tags[0].uid[3] = 0xEF;
        sample_tags[0].tag_type = 1;
        sample_tags[0].is_favorite = true;
        sample_tags[0].use_count = 15;
        sample_tags[0].assigned_slot = 0;

        // Transit card
        strncpy(sample_tags[1].name, "Metro Card", TAG_NAME_MAX_LEN - 1);
        sample_tags[1].category = TagCategoryTransport;
        strncpy(sample_tags[1].category_name, "Transport", TAG_CATEGORY_MAX_LEN - 1);
        strncpy(sample_tags[1].notes, "Daily commute", TAG_NOTES_MAX_LEN - 1);
        sample_tags[1].uid_len = 7;
        sample_tags[1].uid[0] = 0x04;
        sample_tags[1].tag_type = 2;
        sample_tags[1].is_favorite = true;
        sample_tags[1].use_count = 42;
        sample_tags[1].assigned_slot = 1;

        // Gym access
        strncpy(sample_tags[2].name, "Gym FOB", TAG_NAME_MAX_LEN - 1);
        sample_tags[2].category = TagCategoryGym;
        strncpy(sample_tags[2].category_name, "Gym", TAG_CATEGORY_MAX_LEN - 1);
        strncpy(sample_tags[2].notes, "24/7 access", TAG_NOTES_MAX_LEN - 1);
        sample_tags[2].uid_len = 4;
        sample_tags[2].tag_type = 1;
        sample_tags[2].is_favorite = false;
        sample_tags[2].use_count = 8;
        sample_tags[2].assigned_slot = 0xFF;

        for(int i = 0; i < 3; i++) {
            tag_collection_add(collection, &sample_tags[i]);
        }

        tag_collection_save(collection);
        tag_count = 3;
    }

    furi_string_cat_printf(collection_display, "Total Tags: %zu\n\n", tag_count);

    // Display tags
    for(size_t i = 0; i < tag_count && i < 5; i++) {
        const CollectionTag* tag = tag_collection_get(collection, i);

        if(tag) {
            // Favorite indicator
            if(tag->is_favorite) {
                furi_string_cat_printf(collection_display, "★ ");
            } else {
                furi_string_cat_printf(collection_display, "  ");
            }

            // Name
            furi_string_cat_printf(collection_display, "%s\n", tag->name);

            // UID
            furi_string_cat_printf(collection_display, "  UID: ");
            for(uint8_t j = 0; j < tag->uid_len; j++) {
                furi_string_cat_printf(collection_display, "%02X", tag->uid[j]);
            }
            furi_string_cat_printf(collection_display, "\n");

            // Category and slot
            furi_string_cat_printf(
                collection_display,
                "  %s | Uses: %lu",
                tag->category_name,
                tag->use_count);

            if(tag->assigned_slot != 0xFF) {
                furi_string_cat_printf(collection_display, " | Slot %d", tag->assigned_slot);
            }

            furi_string_cat_printf(collection_display, "\n\n");
        }
    }

    if(tag_count > 5) {
        furi_string_cat_printf(collection_display, "... and %zu more\n\n", tag_count - 5);
    }

    // Statistics
    size_t favorites[10];
    size_t fav_count = tag_collection_get_favorites(collection, favorites, 10);
    furi_string_cat_printf(collection_display, "Favorites: %zu\n", fav_count);

    furi_string_cat_printf(collection_display, "\nPress OK to continue");

    widget_add_text_scroll_element(
        widget, 0, 0, 128, 64, furi_string_get_cstr(collection_display));

    furi_string_free(collection_display);
    tag_collection_free(collection);

    sound_effects_success();

    view_dispatcher_switch_to_view(app->view_dispatcher, ChameleonViewWidget);
}

bool chameleon_scene_tag_collection_on_event(void* context, SceneManagerEvent event) {
    UNUSED(context);
    UNUSED(event);
    return false;
}

void chameleon_scene_tag_collection_on_exit(void* context) {
    ChameleonApp* app = context;
    widget_reset(app->widget);
}
