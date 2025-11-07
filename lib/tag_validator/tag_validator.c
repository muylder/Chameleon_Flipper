#include "tag_validator.h"
#include <string.h>
#include <storage/storage.h>

struct TagValidator {
    TagData reference; // Real tag data
    TagData test;      // Emulated tag data
    FuriMutex* mutex;
};

static const char* test_type_names[] = {
    "UID Match",
    "ATQA Match",
    "SAK Match",
    "Block Read",
    "Auth Key A",
    "Auth Key B",
    "Anti-Collision",
    "Response Timing",
};

static const char* result_names[] = {
    "PASS",
    "FAIL",
    "SKIP",
    "ERROR",
};

TagValidator* tag_validator_alloc(void) {
    TagValidator* validator = malloc(sizeof(TagValidator));
    memset(validator, 0, sizeof(TagValidator));

    validator->mutex = furi_mutex_alloc(FuriMutexTypeNormal);

    return validator;
}

void tag_validator_free(TagValidator* validator) {
    if(!validator) return;

    furi_mutex_free(validator->mutex);
    free(validator);
}

void tag_validator_set_reference(TagValidator* validator, const TagData* tag_data) {
    if(!validator || !tag_data) return;

    furi_mutex_acquire(validator->mutex, FuriWaitForever);
    memcpy(&validator->reference, tag_data, sizeof(TagData));
    furi_mutex_release(validator->mutex);
}

void tag_validator_set_test(TagValidator* validator, const TagData* tag_data) {
    if(!validator || !tag_data) return;

    furi_mutex_acquire(validator->mutex, FuriWaitForever);
    memcpy(&validator->test, tag_data, sizeof(TagData));
    furi_mutex_release(validator->mutex);
}

// Individual test implementations
static bool test_uid_match(TagValidator* validator, TagValidationTestResult* result) {
    result->type = TestTypeUIDMatch;

    if(validator->reference.uid_len != validator->test.uid_len) {
        result->result = TestResultFail;
        snprintf(
            result->details,
            sizeof(result->details),
            "UID length mismatch: ref=%d test=%d",
            validator->reference.uid_len,
            validator->test.uid_len);
        return false;
    }

    if(memcmp(
           validator->reference.uid,
           validator->test.uid,
           validator->reference.uid_len) != 0) {
        result->result = TestResultFail;

        // Format UIDs for display
        char ref_uid[32] = "";
        char test_uid[32] = "";
        for(int i = 0; i < validator->reference.uid_len; i++) {
            char byte[4];
            snprintf(byte, sizeof(byte), "%02X", validator->reference.uid[i]);
            strcat(ref_uid, byte);
        }
        for(int i = 0; i < validator->test.uid_len; i++) {
            char byte[4];
            snprintf(byte, sizeof(byte), "%02X", validator->test.uid[i]);
            strcat(test_uid, byte);
        }

        snprintf(
            result->details,
            sizeof(result->details),
            "UID mismatch\nRef:%s\nTest:%s",
            ref_uid,
            test_uid);
        return false;
    }

    result->result = TestResultPass;
    snprintf(
        result->details,
        sizeof(result->details),
        "UID matches (%d bytes)",
        validator->reference.uid_len);
    return true;
}

static bool test_atqa_match(TagValidator* validator, TagValidationTestResult* result) {
    result->type = TestTypeATQAMatch;

    if(memcmp(validator->reference.atqa, validator->test.atqa, 2) != 0) {
        result->result = TestResultFail;
        snprintf(
            result->details,
            sizeof(result->details),
            "ATQA mismatch: ref=%02X%02X test=%02X%02X",
            validator->reference.atqa[0],
            validator->reference.atqa[1],
            validator->test.atqa[0],
            validator->test.atqa[1]);
        return false;
    }

    result->result = TestResultPass;
    snprintf(
        result->details,
        sizeof(result->details),
        "ATQA matches: %02X%02X",
        validator->reference.atqa[0],
        validator->reference.atqa[1]);
    return true;
}

static bool test_sak_match(TagValidator* validator, TagValidationTestResult* result) {
    result->type = TestTypeSAKMatch;

    if(validator->reference.sak != validator->test.sak) {
        result->result = TestResultFail;
        snprintf(
            result->details,
            sizeof(result->details),
            "SAK mismatch: ref=%02X test=%02X",
            validator->reference.sak,
            validator->test.sak);
        return false;
    }

    result->result = TestResultPass;
    snprintf(
        result->details,
        sizeof(result->details),
        "SAK matches: %02X",
        validator->reference.sak);
    return true;
}

static bool test_block_read(TagValidator* validator, TagValidationTestResult* result) {
    result->type = TestTypeBlockRead;

    uint8_t blocks_compared = 0;
    uint8_t blocks_matched = 0;
    uint8_t max_blocks = validator->reference.block_count < validator->test.block_count ?
                             validator->reference.block_count :
                             validator->test.block_count;

    for(uint8_t i = 0; i < max_blocks; i++) {
        blocks_compared++;
        if(memcmp(validator->reference.block_data[i], validator->test.block_data[i], 16) == 0) {
            blocks_matched++;
        }
    }

    if(blocks_matched == blocks_compared && blocks_compared > 0) {
        result->result = TestResultPass;
        snprintf(
            result->details,
            sizeof(result->details),
            "All %d blocks match",
            blocks_compared);
        return true;
    } else if(blocks_compared > 0) {
        result->result = TestResultFail;
        snprintf(
            result->details,
            sizeof(result->details),
            "Only %d/%d blocks match",
            blocks_matched,
            blocks_compared);
        return false;
    } else {
        result->result = TestResultSkipped;
        snprintf(result->details, sizeof(result->details), "No blocks to compare");
        return false;
    }
}

bool tag_validator_run_single_test(
    TagValidator* validator,
    TagValidationTestType test_type,
    TagValidationTestResult* result) {
    if(!validator || !result) return false;

    furi_mutex_acquire(validator->mutex, FuriWaitForever);

    uint32_t start_time = furi_get_tick();
    bool success = false;

    switch(test_type) {
    case TestTypeUIDMatch:
        success = test_uid_match(validator, result);
        break;
    case TestTypeATQAMatch:
        success = test_atqa_match(validator, result);
        break;
    case TestTypeSAKMatch:
        success = test_sak_match(validator, result);
        break;
    case TestTypeBlockRead:
        success = test_block_read(validator, result);
        break;
    default:
        result->type = test_type;
        result->result = TestResultSkipped;
        snprintf(result->details, sizeof(result->details), "Test not implemented");
        break;
    }

    result->duration_ms = furi_get_tick() - start_time;

    furi_mutex_release(validator->mutex);

    return success;
}

bool tag_validator_run_tests(TagValidator* validator, TagValidationReport* report) {
    if(!validator || !report) return false;

    memset(report, 0, sizeof(TagValidationReport));

    // Run basic tests
    TagValidationTestType tests[] = {
        TestTypeUIDMatch,
        TestTypeATQAMatch,
        TestTypeSAKMatch,
        TestTypeBlockRead,
    };

    uint32_t total_start = furi_get_tick();

    for(size_t i = 0; i < sizeof(tests) / sizeof(tests[0]); i++) {
        TagValidationTestResult* result = &report->test_results[report->tests_total];

        tag_validator_run_single_test(validator, tests[i], result);

        report->tests_total++;

        switch(result->result) {
        case TestResultPass:
            report->tests_passed++;
            break;
        case TestResultFail:
            report->tests_failed++;
            break;
        case TestResultSkipped:
            report->tests_skipped++;
            break;
        case TestResultError:
            report->tests_errored++;
            break;
        }
    }

    report->total_duration_ms = furi_get_tick() - total_start;

    // Calculate success rate
    if(report->tests_total > report->tests_skipped) {
        report->success_rate = (float)report->tests_passed /
                               (report->tests_total - report->tests_skipped) * 100.0f;
    }

    return true;
}

const char* tag_validator_get_test_name(TagValidationTestType test_type) {
    if(test_type < sizeof(test_type_names) / sizeof(test_type_names[0])) {
        return test_type_names[test_type];
    }
    return "Unknown";
}

const char* tag_validator_get_result_name(TagValidationResult result) {
    if(result < sizeof(result_names) / sizeof(result_names[0])) {
        return result_names[result];
    }
    return "Unknown";
}

bool tag_validator_export_report(
    TagValidator* validator,
    const TagValidationReport* report,
    const char* filepath) {
    if(!validator || !report || !filepath) return false;

    Storage* storage = furi_record_open(RECORD_STORAGE);
    File* file = storage_file_alloc(storage);

    bool success = false;

    if(storage_file_open(file, filepath, FSAM_WRITE, FSOM_CREATE_ALWAYS)) {
        FuriString* export_text = furi_string_alloc();

        furi_string_cat_printf(export_text, "# Tag Validation Report\n\n");

        furi_string_cat_printf(export_text, "## Summary\n");
        furi_string_cat_printf(export_text, "Total Tests: %d\n", report->tests_total);
        furi_string_cat_printf(export_text, "Passed: %d\n", report->tests_passed);
        furi_string_cat_printf(export_text, "Failed: %d\n", report->tests_failed);
        furi_string_cat_printf(export_text, "Skipped: %d\n", report->tests_skipped);
        furi_string_cat_printf(export_text, "Errors: %d\n", report->tests_errored);
        furi_string_cat_printf(export_text, "Success Rate: %.1f%%\n", report->success_rate);
        furi_string_cat_printf(
            export_text,
            "Total Duration: %lu ms\n\n",
            report->total_duration_ms);

        furi_string_cat_printf(export_text, "## Test Results\n\n");

        for(int i = 0; i < report->tests_total; i++) {
            const TagValidationTestResult* result = &report->test_results[i];

            furi_string_cat_printf(
                export_text,
                "[%s] %s - %s\n",
                tag_validator_get_result_name(result->result),
                tag_validator_get_test_name(result->type),
                result->details);
            furi_string_cat_printf(export_text, "    Duration: %lu ms\n\n", result->duration_ms);
        }

        storage_file_write(
            file,
            furi_string_get_cstr(export_text),
            furi_string_size(export_text));

        furi_string_free(export_text);
        storage_file_close(file);
        success = true;
    }

    storage_file_free(file);
    furi_record_close(RECORD_STORAGE);

    return success;
}
