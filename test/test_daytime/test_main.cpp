// Décision jour/nuit : pio test -e native
#include <unity.h>
#include <stdlib.h>
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

// Règle de src/config.cpp : changements d'heure du 29 mars et du 25 octobre 2026
void test_local_time_dst_transitions() {
    setenv("TZ", "CET-1CEST,M3.5.0,M10.5.0/3", 1);
    tzset();
    TEST_ASSERT_EQUAL(1 * 60 + 59, localMinutesOf(1774745940)); // 29/03 00:59 UTC : 01:59 hiver
    TEST_ASSERT_EQUAL(3 * 60, localMinutesOf(1774746000));      // 29/03 01:00 UTC : 03:00 été
    TEST_ASSERT_EQUAL(2 * 60 + 59, localMinutesOf(1792889940)); // 25/10 00:59 UTC : 02:59 été
    TEST_ASSERT_EQUAL(2 * 60, localMinutesOf(1792890000));      // 25/10 01:00 UTC : 02:00 hiver
    TEST_ASSERT_EQUAL(12 * 60, localMinutesOf(1782900000));     // 01/07 10:00 UTC : 12:00 été
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_normal_day);
    RUN_TEST(test_sunset_after_midnight);
    RUN_TEST(test_local_time_dst_transitions);
    return UNITY_END();
}
