#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <furi.h>

/**
 * @brief Tag Collection Manager
 *
 * Manages a library of cloned/saved tags with metadata
 */

#define TAG_NAME_MAX_LEN 32
#define TAG_CATEGORY_MAX_LEN 16
#define TAG_NOTES_MAX_LEN 64
#define MAX_COLLECTION_TAGS 100

// Tag categories
typedef enum {
    TagCategoryUncategorized,
    TagCategoryHotel,
    TagCategoryTransport,
    TagCategoryAccessControl,
    TagCategoryHome,
    TagCategoryOffice,
    TagCategoryGym,
    TagCategoryOther,
} TagCategory;

// Collection tag entry
typedef struct {
    char name[TAG_NAME_MAX_LEN];
    char category_name[TAG_CATEGORY_MAX_LEN];
    TagCategory category;
    char notes[TAG_NOTES_MAX_LEN];

    // Tag data
    uint8_t uid[10];
    uint8_t uid_len;
    uint8_t tag_type;
    uint8_t blocks[256][16];
    uint8_t blocks_count;

    // Metadata
    uint32_t date_added;      // Unix timestamp
    uint32_t last_used;       // Unix timestamp
    uint32_t use_count;       // Number of times deployed
    bool is_favorite;
    uint8_t assigned_slot;    // Which slot it's currently in (0xFF if none)

    // File reference
    char backup_path[128];    // Path to backup file
} CollectionTag;

// Tag collection
typedef struct TagCollection TagCollection;

/**
 * @brief Allocate tag collection
 * @return TagCollection instance
 */
TagCollection* tag_collection_alloc(void);

/**
 * @brief Free tag collection
 * @param collection Collection instance
 */
void tag_collection_free(TagCollection* collection);

/**
 * @brief Load collection from storage
 * @param collection Collection instance
 * @return Number of tags loaded
 */
size_t tag_collection_load(TagCollection* collection);

/**
 * @brief Save collection to storage
 * @param collection Collection instance
 * @return true if saved successfully
 */
bool tag_collection_save(TagCollection* collection);

/**
 * @brief Add tag to collection
 * @param collection Collection instance
 * @param tag Tag to add
 * @return true if added successfully
 */
bool tag_collection_add(TagCollection* collection, const CollectionTag* tag);

/**
 * @brief Remove tag from collection
 * @param collection Collection instance
 * @param index Tag index
 * @return true if removed successfully
 */
bool tag_collection_remove(TagCollection* collection, size_t index);

/**
 * @brief Get tag by index
 * @param collection Collection instance
 * @param index Tag index
 * @return Pointer to tag, or NULL if invalid index
 */
const CollectionTag* tag_collection_get(TagCollection* collection, size_t index);

/**
 * @brief Get collection size
 * @param collection Collection instance
 * @return Number of tags in collection
 */
size_t tag_collection_get_count(TagCollection* collection);

/**
 * @brief Find tags by name
 * @param collection Collection instance
 * @param name Tag name (partial match)
 * @param results Output array for matching indices
 * @param max_results Maximum results
 * @return Number of matches found
 */
size_t tag_collection_find_by_name(
    TagCollection* collection,
    const char* name,
    size_t* results,
    size_t max_results);

/**
 * @brief Find tags by category
 * @param collection Collection instance
 * @param category Category to search
 * @param results Output array for matching indices
 * @param max_results Maximum results
 * @return Number of matches found
 */
size_t tag_collection_find_by_category(
    TagCollection* collection,
    TagCategory category,
    size_t* results,
    size_t max_results);

/**
 * @brief Get favorite tags
 * @param collection Collection instance
 * @param results Output array for favorite indices
 * @param max_results Maximum results
 * @return Number of favorites
 */
size_t tag_collection_get_favorites(
    TagCollection* collection,
    size_t* results,
    size_t max_results);

/**
 * @brief Toggle favorite status
 * @param collection Collection instance
 * @param index Tag index
 * @return New favorite status
 */
bool tag_collection_toggle_favorite(TagCollection* collection, size_t index);

/**
 * @brief Update tag metadata
 * @param collection Collection instance
 * @param index Tag index
 * @param tag Updated tag data
 * @return true if updated successfully
 */
bool tag_collection_update(TagCollection* collection, size_t index, const CollectionTag* tag);

/**
 * @brief Deploy tag to Chameleon slot
 * @param collection Collection instance
 * @param index Tag index
 * @param slot Target slot number
 * @return true if deployed successfully
 */
bool tag_collection_deploy_to_slot(TagCollection* collection, size_t index, uint8_t slot);

/**
 * @brief Sort collection
 * @param collection Collection instance
 * @param by_favorites Sort favorites first
 * @param by_date Sort by date
 * @param by_name Sort by name
 */
void tag_collection_sort(
    TagCollection* collection,
    bool by_favorites,
    bool by_date,
    bool by_name);

/**
 * @brief Export collection to file
 * @param collection Collection instance
 * @param filepath Output file path
 * @return true if exported successfully
 */
bool tag_collection_export(TagCollection* collection, const char* filepath);

/**
 * @brief Import collection from file
 * @param collection Collection instance
 * @param filepath Input file path
 * @return Number of tags imported
 */
size_t tag_collection_import(TagCollection* collection, const char* filepath);

/**
 * @brief Get category name
 * @param category Category enum
 * @return Category name string
 */
const char* tag_collection_get_category_name(TagCategory category);

/**
 * @brief Get category icon/emoji
 * @param category Category enum
 * @return Category icon string
 */
const char* tag_collection_get_category_icon(TagCategory category);
