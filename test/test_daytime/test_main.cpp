// Décision jour/nuit : pio test -e native
#include <unity.h>
#include "daytime.h"

void setUp() {}
void tearDown() {}

// Ouverture 07:00, fermeture 21:00
void test_normal_day() {
    TEST_ASSERT_FALSE(isDaytime(420, 1260, 419));  // 06:59 : nuit
    TEST_ASSERT_TRUE(isDaytime(420, 1260, 420));   // 07:00 : ouverture
    TEST_ASSERT_TRUE(isDaytime(420, 1260, 1259));  // 20:59 : jour
    TEST_ASSERT_FALSE(isDaytime(420, 1260, 1260)); // 21:00 : fermeture
    TEST_ASSERT_FALSE(isDaytime(420, 1260, 0));    // minuit : nuit
}

// Fermeture après minuit (00:30) par un décalage : jour de 07:00 à 00:30
void test_sunset_after_midnight() {
    TEST_ASSERT_TRUE(isDaytime(420, 30, 1439));    // 23:59 : jour
    TEST_ASSERT_TRUE(isDaytime(420, 30, 10));      // 00:10 : encore jour
    TEST_ASSERT_FALSE(isDaytime(420, 30, 30));     // 00:30 : fermeture
    TEST_ASSERT_FALSE(isDaytime(420, 30, 180));    // 03:00 : nuit
    TEST_ASSERT_TRUE(isDaytime(420, 30, 420));     // 07:00 : ouverture
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_normal_day);
    RUN_TEST(test_sunset_after_midnight);
    return UNITY_END();
}
