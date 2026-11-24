#include "fwb/common/status.h"

#include "unity.h"

void setUp(void)
{
}
void tearDown(void)
{
}

static void test_fwb_ok_vaut_zero(void)
{
    TEST_ASSERT_EQUAL_INT(0, (int)FWB_OK);
}

static void test_erreurs_sont_negatives(void)
{
    TEST_ASSERT_LESS_THAN_INT(0, (int)FWB_ERR_INVALID_ARG);
    TEST_ASSERT_LESS_THAN_INT(0, (int)FWB_ERR_BUFFER_TOO_SMALL);
    TEST_ASSERT_LESS_THAN_INT(0, (int)FWB_ERR_NOT_SUPPORTED);
    TEST_ASSERT_LESS_THAN_INT(0, (int)FWB_ERR_FRAME_TOO_SHORT);
    TEST_ASSERT_LESS_THAN_INT(0, (int)FWB_ERR_PAYLOAD_TOO_LARGE);
    TEST_ASSERT_LESS_THAN_INT(0, (int)FWB_ERR_LENGTH_MISMATCH);
    TEST_ASSERT_LESS_THAN_INT(0, (int)FWB_ERR_BAD_CRC);
    TEST_ASSERT_LESS_THAN_INT(0, (int)FWB_ERR_BAD_VERSION);
    TEST_ASSERT_LESS_THAN_INT(0, (int)FWB_ERR_BAD_FLAGS);
    TEST_ASSERT_LESS_THAN_INT(0, (int)FWB_ERR_COBS_MALFORMED);
    TEST_ASSERT_LESS_THAN_INT(0, (int)FWB_ERR_OVERFLOW);
    TEST_ASSERT_LESS_THAN_INT(0, (int)FWB_ERR_TIMEOUT);
    TEST_ASSERT_LESS_THAN_INT(0, (int)FWB_ERR_REPLAY);
    TEST_ASSERT_LESS_THAN_INT(0, (int)FWB_ERR_TOO_OLD);
    TEST_ASSERT_LESS_THAN_INT(0, (int)FWB_ERR_SEQ_GAP);
}

static void test_codes_distincts(void)
{
    /* Un alias accidentel effacerait un diagnostic en production. */
    TEST_ASSERT_NOT_EQUAL_INT((int)FWB_OK, (int)FWB_ERR_INVALID_ARG);
    TEST_ASSERT_NOT_EQUAL_INT((int)FWB_ERR_REPLAY, (int)FWB_ERR_TOO_OLD);
    TEST_ASSERT_NOT_EQUAL_INT((int)FWB_ERR_TOO_OLD, (int)FWB_ERR_SEQ_GAP);
}

static void test_status_str_renvoie_non_nul(void)
{
    TEST_ASSERT_NOT_NULL(fwb_status_str(FWB_OK));
    TEST_ASSERT_NOT_NULL(fwb_status_str(FWB_ERR_INVALID_ARG));
    TEST_ASSERT_NOT_NULL(fwb_status_str(FWB_ERR_BAD_CRC));
    TEST_ASSERT_NOT_NULL(fwb_status_str(FWB_ERR_SEQ_GAP));
}

static void test_status_str_code_inconnu(void)
{
    fwb_status_t inconnue = (fwb_status_t)(-99);
    TEST_ASSERT_NOT_NULL(fwb_status_str(inconnue));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_fwb_ok_vaut_zero);
    RUN_TEST(test_erreurs_sont_negatives);
    RUN_TEST(test_codes_distincts);
    RUN_TEST(test_status_str_renvoie_non_nul);
    RUN_TEST(test_status_str_code_inconnu);
    return UNITY_END();
}
