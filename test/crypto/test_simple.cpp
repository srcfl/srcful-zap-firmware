#include <unity.h>
#include <Arduino.h>

void test_simple_assertion(void) {
    TEST_ASSERT_TRUE(true);
    Serial.println("Simple assertion test passed!");
} 