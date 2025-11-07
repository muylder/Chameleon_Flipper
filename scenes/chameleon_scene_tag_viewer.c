#include "../chameleon_app_i.h"
#include <gui/modules/widget.h>

// Sample tag data for demonstration (would come from actual tag read)
static const uint8_t sample_tag_data[] = {
    0x04, 0xAB, 0xCD, 0xEF, 0x12, 0x34, 0x56, 0x78, // Block 0 (UID)
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Block 1
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Block 2
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x07, // Block 3 (Sector trailer)
    0x88, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Block 4
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Block 5
};

/**
 * @brief Format tag data as hex dump with ASCII representation
 * @param data Tag data bytes
 * @param data_len Length of data
 * @param output Output string
 * @param show_ascii Show ASCII column
 */
static void format_hex_dump(
    const uint8_t* data,
    size_t data_len,
    FuriString* output,
    bool show_ascii) {
    furi_string_cat_printf(output, "=== HEX DUMP ===\n\n");

    for(size_t i = 0; i < data_len; i += 16) {
        // Address
        furi_string_cat_printf(output, "%04X: ", i);

        // Hex bytes
        for(size_t j = 0; j < 16; j++) {
            if(i + j < data_len) {
                furi_string_cat_printf(output, "%02X ", data[i + j]);
            } else {
                furi_string_cat_printf(output, "   ");
            }

            // Space between 8-byte groups
            if(j == 7) furi_string_cat_printf(output, " ");
        }

        // ASCII representation
        if(show_ascii) {
            furi_string_cat_printf(output, " |");
            for(size_t j = 0; j < 16 && (i + j) < data_len; j++) {
                uint8_t c = data[i + j];
                if(c >= 32 && c <= 126) {
                    furi_string_cat_printf(output, "%c", c);
                } else {
                    furi_string_cat_printf(output, ".");
                }
            }
            furi_string_cat_printf(output, "|");
        }

        furi_string_cat_printf(output, "\n");
    }
}

/**
 * @brief Format Mifare sector view
 * @param data Tag data
 * @param data_len Length of data
 * @param output Output string
 */
static void format_mifare_sector_view(
    const uint8_t* data,
    size_t data_len,
    FuriString* output) {
    furi_string_cat_printf(output, "=== MIFARE SECTORS ===\n\n");

    size_t sector = 0;
    size_t block_in_sector = 0;

    for(size_t block = 0; block * 16 < data_len; block++) {
        const uint8_t* block_data = &data[block * 16];

        // New sector header
        if(block_in_sector == 0) {
            furi_string_cat_printf(output, "Sector %zu:\n", sector);
        }

        // Block type indicator
        const char* block_type;
        if(block == 0) {
            block_type = "UID";
        } else if(block_in_sector == 3) {
            block_type = "TRL"; // Trailer
        } else {
            block_type = "DAT"; // Data
        }

        // Format block
        furi_string_cat_printf(output, " [%zu]%s ", block, block_type);
        for(size_t i = 0; i < 16 && (block * 16 + i) < data_len; i++) {
            furi_string_cat_printf(output, "%02X", block_data[i]);
            if(i == 7) furi_string_cat_printf(output, " ");
        }
        furi_string_cat_printf(output, "\n");

        // Move to next block/sector
        block_in_sector++;
        if(block_in_sector == 4) {
            block_in_sector = 0;
            sector++;
            furi_string_cat_printf(output, "\n");
        }
    }
}

/**
 * @brief Format tag summary info
 * @param data Tag data
 * @param data_len Length of data
 * @param output Output string
 */
static void format_tag_summary(const uint8_t* data, size_t data_len, FuriString* output) {
    furi_string_cat_printf(output, "=== TAG INFO ===\n\n");

    // UID (first 4 or 7 bytes typically)
    furi_string_cat_printf(output, "UID: ");
    for(size_t i = 0; i < 7 && i < data_len; i++) {
        furi_string_cat_printf(output, "%02X", data[i]);
        if(i < 6) furi_string_cat_printf(output, ":");
    }
    furi_string_cat_printf(output, "\n\n");

    // Type detection
    if(data_len >= 16) {
        uint8_t sak = data[8]; // Approximation
        furi_string_cat_printf(output, "Type: ");
        if(sak == 0x08) {
            furi_string_cat_printf(output, "MIFARE Classic 1K\n");
        } else if(sak == 0x18) {
            furi_string_cat_printf(output, "MIFARE Classic 4K\n");
        } else if(sak == 0x00) {
            furi_string_cat_printf(output, "MIFARE Ultralight\n");
        } else {
            furi_string_cat_printf(output, "Unknown (%02X)\n", sak);
        }
    }

    furi_string_cat_printf(output, "Size: %zu bytes\n", data_len);
    furi_string_cat_printf(output, "Blocks: %zu\n", data_len / 16);
    furi_string_cat_printf(output, "Sectors: %zu\n\n", (data_len / 16) / 4);

    // Memory usage
    size_t non_zero = 0;
    for(size_t i = 0; i < data_len; i++) {
        if(data[i] != 0x00 && data[i] != 0xFF) non_zero++;
    }
    float usage = (float)non_zero / data_len * 100.0f;
    furi_string_cat_printf(output, "Data Usage: %.1f%%\n", usage);
}

void chameleon_scene_tag_viewer_on_enter(void* context) {
    ChameleonApp* app = context;
    Widget* widget = app->widget;

    widget_reset(widget);

    // Build comprehensive tag view
    FuriString* tag_view = furi_string_alloc();

    // Summary
    format_tag_summary(sample_tag_data, sizeof(sample_tag_data), tag_view);
    furi_string_cat_printf(tag_view, "\n");

    // Sector view
    format_mifare_sector_view(sample_tag_data, sizeof(sample_tag_data), tag_view);
    furi_string_cat_printf(tag_view, "\n");

    // Hex dump with ASCII
    format_hex_dump(sample_tag_data, sizeof(sample_tag_data), tag_view, true);

    furi_string_cat_printf(tag_view, "\n\n[OK] to return");

    widget_add_text_scroll_element(widget, 0, 0, 128, 64, furi_string_get_cstr(tag_view));

    furi_string_free(tag_view);

    view_dispatcher_switch_to_view(app->view_dispatcher, ChameleonViewWidget);
}

bool chameleon_scene_tag_viewer_on_event(void* context, SceneManagerEvent event) {
    UNUSED(context);
    UNUSED(event);
    return false;
}

void chameleon_scene_tag_viewer_on_exit(void* context) {
    ChameleonApp* app = context;
    widget_reset(app->widget);
}
