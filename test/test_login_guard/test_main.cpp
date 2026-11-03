// Limitation des essais de mot de passe : pio test -e native
#include <unity.h>
#include "login_guard.h"

void setUp() {}
void tearDown() {}

static const uint32_t LOCK = LoginGuard::LOCK_MS;

void test_lock_after_max_failures_and_expiry() {
    const uint32_t ip = 1, t = 1000;
    for (int i = 0; i < 4; i++) LoginGuard::recordFailure(ip, t);
    TEST_ASSERT_FALSE(LoginGuard::isLocked(ip, t));        // 4 échecs : pas encore bloqué
    LoginGuard::recordFailure(ip, t);
    TEST_ASSERT_TRUE(LoginGuard::isLocked(ip, t));         // 5e échec : bloqué
    TEST_ASSERT_FALSE(LoginGuard::isLocked(2, t));         // une autre IP n'est pas gênée
    TEST_ASSERT_TRUE(LoginGuard::isLocked(ip, t + LOCK));  // toujours bloqué à la limite
    TEST_ASSERT_FALSE(LoginGuard::isLocked(ip, t + LOCK + 1)); // expiré
}

void test_success_clears_failures() {
    const uint32_t ip = 3, t = 10 * LOCK;
    for (int i = 0; i < 4; i++) LoginGuard::recordFailure(ip, t);
    LoginGuard::clear(ip, t);
    LoginGuard::recordFailure(ip, t);
    TEST_ASSERT_FALSE(LoginGuard::isLocked(ip, t));
}

// Quatre adresses en échec remplissent la table : une cinquième est refusée
void test_full_table_refuses_unknown_ip() {
    const uint32_t t = 20 * LOCK;
    for (uint32_t ip = 10; ip < 14; ip++) LoginGuard::recordFailure(ip, t);
    TEST_ASSERT_TRUE(LoginGuard::isLocked(99, t));
    TEST_ASSERT_FALSE(LoginGuard::isLocked(99, t + LOCK + 1)); // libérée à l'expiration
}

// millis() repasse par zéro tous les 49,7 jours
void test_millis_wraparound() {
    const uint32_t ip = 7, t = 0xFFFFFF00u;
    for (int i = 0; i < 5; i++) LoginGuard::recordFailure(ip, t);
    TEST_ASSERT_TRUE(LoginGuard::isLocked(ip, 100));
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_lock_after_max_failures_and_expiry);
    RUN_TEST(test_success_clears_failures);
    RUN_TEST(test_full_table_refuses_unknown_ip);
    RUN_TEST(test_millis_wraparound);
    return UNITY_END();
}
