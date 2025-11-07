#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <furi.h>

// Validation test types
typedef enum {
    TestTypeUIDMatch,           // UID comparison
    TestTypeATQAMatch,          // ATQA response match
    TestTypeSAKMatch,           // SAK match
    TestTypeBlockRead,          // Block read comparison
    TestTypeAuthenticationA,    // Key A authentication
    TestTypeAuthenticationB,    // Key B authentication
    TestTypeAntiCollision,      // Anti-collision timing
    TestTypeResponseTiming,     // Response delay validation
} TagValidationTestType;

// Test result
typedef enum {
    TestResultPass,
    TestResultFail,
    TestResultSkipped,
    TestResultError,
} TagValidationResult;

// Individual test result
typedef struct {
    TagValidationTestType type;
    TagValidationResult result;
    char details[128]; // Details about the test/failure
    uint32_t duration_ms; // Test duration
} TagValidationTestResult;

// Complete validation report
typedef struct {
    uint8_t tests_total;
    uint8_t tests_passed;
    uint8_t tests_failed;
    uint8_t tests_skipped;
    uint8_t tests_errored;
    float success_rate;
    uint32_t total_duration_ms;

    TagValidationTestResult test_results[16]; // Max 16 tests
} TagValidationReport;

// Tag data for comparison
typedef struct {
    uint8_t uid[10];
    uint8_t uid_len;
    uint8_t atqa[2];
    uint8_t sak;
    uint8_t block_data[256][16]; // Max 256 blocks
    uint8_t block_count;
} TagData;

// Tag validator
typedef struct TagValidator TagValidator;

/**
 * @brief Allocate tag validator
 * @return TagValidator instance
 */
TagValidator* tag_validator_alloc(void);

/**
 * @brief Free tag validator
 * @param validator Validator instance
 */
void tag_validator_free(TagValidator* validator);

/**
 * @brief Set reference tag data (real tag)
 * @param validator Validator instance
 * @param tag_data Reference tag data
 */
void tag_validator_set_reference(TagValidator* validator, const TagData* tag_data);

/**
 * @brief Set test tag data (emulated tag)
 * @param validator Validator instance
 * @param tag_data Test tag data
 */
void tag_validator_set_test(TagValidator* validator, const TagData* tag_data);

/**
 * @brief Run all validation tests
 * @param validator Validator instance
 * @param report Output validation report
 * @return true if validation completed
 */
bool tag_validator_run_tests(TagValidator* validator, TagValidationReport* report);

/**
 * @brief Run a specific test
 * @param validator Validator instance
 * @param test_type Test type to run
 * @param result Output test result
 * @return true if test completed
 */
bool tag_validator_run_single_test(
    TagValidator* validator,
    TagValidationTestType test_type,
    TagValidationTestResult* result);

/**
 * @brief Get test type name
 * @param test_type Test type
 * @return Test name string
 */
const char* tag_validator_get_test_name(TagValidationTestType test_type);

/**
 * @brief Get result name
 * @param result Result type
 * @return Result name string
 */
const char* tag_validator_get_result_name(TagValidationResult result);

/**
 * @brief Export validation report to file
 * @param validator Validator instance
 * @param report Validation report
 * @param filepath Output file path
 * @return true if exported successfully
 */
bool tag_validator_export_report(
    TagValidator* validator,
    const TagValidationReport* report,
    const char* filepath);
