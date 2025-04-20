#include "unity.h"
#include "scale_logic.h" // Include the header for the module being tested
#include <string.h>      // For memset

// --- Mock HAL Functions (Needed for some tests) ---
// We need to provide mock implementations for any HAL functions
// that ScaleLogic calls directly, if any are complex. Tare/Sample don't
// directly affect the Update logic being tested here, so we might not need complex mocks.
// Example simple mock:
static LoadCellReading_t mock_reading;
void hal_LoadCell_Tare(void) { /* Mock does nothing */ }
bool hal_Storage_Save_Float(const char* ns, const char* key, float val) { return true; } // Mock success
bool hal_Storage_Load_Float(const char* ns, const char* key, float* val) { *val = 0.0f; return false; } // Mock not found


// --- Test Globals ---
static ScaleState_t test_state;

// --- Test Setup/Teardown ---
void setUp(void) {
    // Ran before each test function
    ScaleLogic_Init(&test_state); // Initialize state before each test
    mock_reading = (LoadCellReading_t){0}; // Reset mock reading
}

void tearDown(void) {
    // Ran after each test function
    // (Clean up resources if needed)
}

// --- Test Cases ---
void test_ScaleLogic_Init_Defaults(void) {
    TEST_ASSERT_EQUAL(MODE_WEIGHING, test_state.current_mode);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, test_state.current_weight_g);
    TEST_ASSERT_EQUAL_INT(0, test_state.item_count);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, test_state.average_item_weight_g);
    TEST_ASSERT_FALSE(test_state.is_stable);
    TEST_ASSERT_FALSE(test_state.is_overload);
}

void test_ScaleLogic_Update_WeightAndStability(void) {
    mock_reading.weight_grams = 123.4f;
    mock_reading.is_stable = true;
    mock_reading.is_overload = false;

    ScaleLogic_Update(&test_state, &mock_reading);

    TEST_ASSERT_EQUAL_FLOAT(123.4f, test_state.current_weight_g);
    TEST_ASSERT_TRUE(test_state.is_stable);
    TEST_ASSERT_FALSE(test_state.is_overload);
}

void test_ScaleLogic_Update_Overload(void) {
    mock_reading.weight_grams = 5500.0f; // Assuming max is 5000
    mock_reading.is_stable = false;
    mock_reading.is_overload = true; // Assume HAL sets this

    ScaleLogic_Update(&test_state, &mock_reading);

    TEST_ASSERT_TRUE(test_state.is_overload);
    TEST_ASSERT_EQUAL(MODE_ERROR, test_state.current_mode);
    TEST_ASSERT_EQUAL_STRING("OVERLOAD!", test_state.status_message);
    TEST_ASSERT_EQUAL_INT(0, test_state.item_count); // Count should reset
}


void test_ScaleLogic_Counting_Simple(void) {
    // Setup: Set average weight and mode first
    test_state.average_item_weight_g = 10.5f;
    test_state.current_mode = MODE_COUNTING;

    // Test: Apply a stable weight
    mock_reading.weight_grams = 52.6f; // Should be 5 items (52.6 / 10.5 = 5.009)
    mock_reading.is_stable = true;
    mock_reading.is_overload = false;
    ScaleLogic_Update(&test_state, &mock_reading);

    TEST_ASSERT_EQUAL_INT(5, test_state.item_count);
    TEST_ASSERT_EQUAL_STRING("Stable (Count)", test_state.status_message);

    // Test: Apply unstable weight
    mock_reading.weight_grams = 53.0f;
    mock_reading.is_stable = false;
    ScaleLogic_Update(&test_state, &mock_reading);

    TEST_ASSERT_EQUAL_INT(5, test_state.item_count); // Count should persist from last stable reading
    TEST_ASSERT_EQUAL_STRING("...", test_state.status_message); // Should indicate instability


     // Test: Weight below half average
    mock_reading.weight_grams = 4.0f; // Less than 10.5 / 2
    mock_reading.is_stable = true;
    ScaleLogic_Update(&test_state, &mock_reading);

    TEST_ASSERT_EQUAL_INT(0, test_state.item_count); // Should be 0 items
}


void test_ScaleLogic_SetSampleWeight_Success(void) {
    test_state.current_mode = MODE_WEIGHING;
    test_state.current_weight_g = 25.2f; // Weight to set as sample
    test_state.is_stable = true;

    ScaleLogic_RequestSetSample(&test_state);

    TEST_ASSERT_EQUAL(MODE_COUNTING, test_state.current_mode);
    TEST_ASSERT_EQUAL_FLOAT(25.2f, test_state.average_item_weight_g);
    TEST_ASSERT_EQUAL_STRING("Sample Set", test_state.status_message);
    // Check if count updated immediately (depends on implementation)
    TEST_ASSERT_EQUAL_INT(1, test_state.item_count); // 25.2 / 25.2 = 1
}

void test_ScaleLogic_SetSampleWeight_FailUnstable(void) {
    test_state.current_mode = MODE_WEIGHING;
    test_state.current_weight_g = 25.2f;
    test_state.is_stable = false; // Unstable

    ScaleLogic_RequestSetSample(&test_state);

    TEST_ASSERT_EQUAL(MODE_WEIGHING, test_state.current_mode); // Should not change mode
    TEST_ASSERT_EQUAL_FLOAT(0.0f, test_state.average_item_weight_g); // Should not be set
    TEST_ASSERT_EQUAL_STRING("Unstable!", test_state.status_message);
}

// --- Main Test Runner ---
// This part depends on how Unity is integrated (e.g., with PlatformIO)
// Usually, you just define the tests, and the framework calls them.
// If running standalone, you might need:
/*
int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_ScaleLogic_Init_Defaults);
    RUN_TEST(test_ScaleLogic_Update_WeightAndStability);
    RUN_TEST(test_ScaleLogic_Update_Overload);
    RUN_TEST(test_ScaleLogic_Counting_Simple);
    RUN_TEST(test_ScaleLogic_SetSampleWeight_Success);
    RUN_TEST(test_ScaleLogic_SetSampleWeight_FailUnstable);
    // Add RUN_TEST for all other test functions
    return UNITY_END();
}
*/

// Make sure your PlatformIO config or build system knows how to build and run these tests.
