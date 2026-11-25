/* Reads FWB_VECTORS_PATH at runtime and runs each vector through the rx. */
#include <ctype.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fwb/framing/framing.h"
#include "unity.h"

#ifndef FWB_VECTORS_PATH
#error "Define FWB_VECTORS_PATH to the path of vectors.txt"
#endif

void setUp(void)  {}
void tearDown(void) {}

/* ---- minimal parser ---- */

#define MAX_WIRE 268
#define MAX_ID   32
#define MAX_TITLE 128
#define MAX_NOTE  256

typedef struct {
    char         id[MAX_ID];
    fwb_status_t expect;
    uint16_t     link_id;
    uint8_t      wire[MAX_WIRE];
    size_t       wire_len;
    bool         has_wire;
} vector_t;

/* Returns pointer to value after "key[spaces]=[spaces]", or NULL. */
static const char *key_val(const char *line, const char *key)
{
    size_t klen = strlen(key);
    if (strncmp(line, key, klen) != 0) return NULL;
    const char *p = line + klen;
    while (*p == ' ' || *p == '\t') p++;
    if (*p != '=') return NULL;
    p++;
    while (*p == ' ' || *p == '\t') p++;
    return p;
}

static fwb_status_t parse_expect(const char *s)
{
    if (strcmp(s, "FWB_OK")                 == 0) return FWB_OK;
    if (strcmp(s, "FWB_ERR_BAD_CRC")        == 0) return FWB_ERR_BAD_CRC;
    if (strcmp(s, "FWB_ERR_BAD_VERSION")    == 0) return FWB_ERR_BAD_VERSION;
    if (strcmp(s, "FWB_ERR_BAD_FLAGS")      == 0) return FWB_ERR_BAD_FLAGS;
    if (strcmp(s, "FWB_ERR_FRAME_TOO_SHORT") == 0) return FWB_ERR_FRAME_TOO_SHORT;
    if (strcmp(s, "FWB_ERR_PAYLOAD_TOO_LARGE") == 0) return FWB_ERR_PAYLOAD_TOO_LARGE;
    if (strcmp(s, "FWB_ERR_LENGTH_MISMATCH") == 0) return FWB_ERR_LENGTH_MISMATCH;
    if (strcmp(s, "FWB_ERR_COBS_MALFORMED") == 0) return FWB_ERR_COBS_MALFORMED;
    return FWB_OK; /* unknown: treat as OK */
}

static uint16_t parse_link_id(const char *note)
{
    const char *p = strstr(note, "link_id=0x");
    if (p == NULL) {
        return 0x0000U;
    }
    p += strlen("link_id=0x");
    return (uint16_t)strtoul(p, NULL, 16);
}

/* Run one vector through the rx. FRM-I-010 (empty wire = just delimiter)
 * is handled: rx sees 0x00 from IDLE → FWB_OK, no frame. */
static void run_vector(const vector_t *v)
{
    fwb_framing_cfg_t cfg = {.link_id = v->link_id};
    fwb_framing_rx_t  rx;
    fwb_framing_frame_t frame;
    (void)fwb_framing_rx_init(&rx, &cfg, 0U);

    fwb_status_t last_err  = FWB_OK;
    bool         got_frame = false;

    for (size_t i = 0U; i < v->wire_len; i++) {
        bool r = false;
        fwb_status_t st = fwb_framing_rx_push(&rx, v->wire[i], &frame, &r);
        if (r) {
            got_frame = true;
            last_err  = FWB_OK;
        } else if (st != FWB_OK) {
            last_err = st;
        }
    }

    if (v->expect == FWB_OK) {
        /* FRM-I-010: wire = {0x00} → FWB_OK with no frame. */
        bool is_empty_delimiter = (v->wire_len == 1U && v->wire[0] == 0x00U);
        if (is_empty_delimiter) {
            TEST_ASSERT_EQUAL_INT_MESSAGE(FWB_OK, last_err, v->id);
            TEST_ASSERT_FALSE_MESSAGE(got_frame, v->id);
        } else {
            TEST_ASSERT_EQUAL_INT_MESSAGE(FWB_OK, last_err, v->id);
            TEST_ASSERT_TRUE_MESSAGE(got_frame, v->id);
        }
    } else {
        TEST_ASSERT_EQUAL_INT_MESSAGE((int)v->expect, (int)last_err, v->id);
    }
}

static void test_all_vectors(void)
{
    FILE *f = fopen(FWB_VECTORS_PATH, "r");
    if (f == NULL) {
        TEST_FAIL_MESSAGE("Cannot open vectors.txt: " FWB_VECTORS_PATH);
        return;
    }

    char     line[512];
    vector_t v;
    memset(&v, 0, sizeof(v));
    int    vectors_run = 0;
    bool   in_wire     = false;

    while (fgets(line, sizeof(line), f) != NULL) {
        /* Strip trailing newline. */
        size_t len = strlen(line);
        while (len > 0U && (line[len - 1U] == '\n' || line[len - 1U] == '\r')) {
            line[--len] = '\0';
        }

        if (line[0] == '[' && strchr(line, ']') != NULL) {
            /* New section: run previous vector if any. */
            if (v.id[0] != '\0' && v.has_wire) {
                run_vector(&v);
                vectors_run++;
            }
            /* Reset. */
            memset(&v, 0, sizeof(v));
            in_wire = false;
            /* Extract id between [ ]. */
            size_t i = 1U;
            size_t j = 0U;
            while (line[i] && line[i] != ']' && j + 1U < sizeof(v.id)) {
                v.id[j++] = line[i++];
            }
            v.id[j] = '\0';
            continue;
        }

        if (v.id[0] == '\0') continue; /* Not inside a section yet. */

        /* Blank line: stop wire parsing. */
        if (len == 0U) {
            in_wire = false;
            continue;
        }

        /* Comments. */
        if (line[0] == '#') continue;

        /* "wire =" starts hex block; subsequent indented lines are data. */
        const char *wire_val = key_val(line, "wire");
        if (wire_val != NULL) {
            in_wire = true;
            /* Inline hex after "wire =" (may be empty). */
            const char *p = wire_val;
            if (*p != '\0' && *p != '#') {
                /* Parse hex on this same line. */
                while (*p != '\0') {
                    while (*p == ' ' || *p == '\t') p++;
                    if (!isxdigit((unsigned char)*p)) break;
                    char hi = *p++;
                    if (!isxdigit((unsigned char)*p)) break;
                    char lo = *p++;
                    char buf[3] = {hi, lo, '\0'};
                    if (v.wire_len < MAX_WIRE) {
                        v.wire[v.wire_len++] = (uint8_t)strtoul(buf, NULL, 16);
                    }
                }
            }
            v.has_wire = true;
            continue;
        }

        if (in_wire && (line[0] == ' ' || line[0] == '\t')) {
            /* Continuation hex line. */
            const char *p = line;
            while (*p != '\0') {
                while (*p == ' ' || *p == '\t') p++;
                if (!isxdigit((unsigned char)*p)) break;
                char hi = *p++;
                if (!isxdigit((unsigned char)*p)) break;
                char lo = *p++;
                char buf[3] = {hi, lo, '\0'};
                if (v.wire_len < MAX_WIRE) {
                    v.wire[v.wire_len++] = (uint8_t)strtoul(buf, NULL, 16);
                }
            }
            continue;
        }

        in_wire = false;

        /* "expect = ..." */
        const char *exp_val = key_val(line, "expect");
        if (exp_val != NULL) {
            char val[64] = {0};
            const char *p = exp_val;
            size_t i = 0U;
            while (*p && *p != ' ' && *p != '\t' && i + 1U < sizeof(val)) {
                val[i++] = *p++;
            }
            v.expect = parse_expect(val);
            continue;
        }

        /* "title" or "note" may carry link_id=0x... */
        if (key_val(line, "title") != NULL || key_val(line, "note") != NULL) {
            uint16_t lid = parse_link_id(line);
            if (lid != 0U) {
                v.link_id = lid;
            }
            continue;
        }
    }

    /* Run last vector. */
    if (v.id[0] != '\0' && v.has_wire) {
        run_vector(&v);
        vectors_run++;
    }

    fclose(f);

    char msg[64];
    snprintf(msg, sizeof(msg), "Ran %d vectors", vectors_run);
    TEST_MESSAGE(msg);
    TEST_ASSERT_GREATER_THAN_INT(0, vectors_run);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_all_vectors);
    return UNITY_END();
}
