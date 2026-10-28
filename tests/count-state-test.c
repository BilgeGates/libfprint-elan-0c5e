/* SPDX-License-Identifier: LGPL-2.1-or-later
 * Exercise the driver's actual COUNT callback, with no USB/device access.
 */
#include <glib.h>
#include <stdarg.h>
#include <stdint.h>
typedef enum {
  FPI_DEVICE_ACTION_OPEN,
  FPI_DEVICE_ACTION_ENROLL,
  FPI_DEVICE_ACTION_LIST,
  FPI_DEVICE_ACTION_VERIFY,
  FPI_DEVICE_ACTION_IDENTIFY
} FpiDeviceAction;
typedef struct {
  guint completed, next, failed;
} FpiSsm;
typedef struct {
  FpiSsm *task_ssm;
  GPtrArray *list_result;
  guint curr_enrolled, pid;
  guint fw_ver, x_trace, y_trace;
  FpiDeviceAction action;
  guint reports, completions;
} FpiDeviceElanmoc;
typedef FpiDeviceElanmoc FpDevice;
#define FP_DEVICE(d) (d)
#define FP_DEVICE_ERROR_PROTO 1
#define FP_DEVICE_ERROR_NOT_SUPPORTED 2
#define FPI_MATCH_FAIL 0
#define fp_info(...) ((void)0)
static void *fpi_device_get_usb_device(FpDevice *d) { return d; }
static guint g_usb_device_get_pid(void *d) { return ((FpDevice *)d)->pid; }
static FpiDeviceAction fpi_device_get_current_action(FpDevice *d) {
  return d->action;
}
static void fpi_ssm_mark_failed(FpiSsm *s, GError *e) {
  g_assert_nonnull(e);
  s->failed++;
  g_error_free(e);
}
static void fpi_ssm_mark_completed(FpiSsm *s) { s->completed++; }
static void fpi_ssm_next_state(FpiSsm *s) { s->next++; }
static GError *fpi_device_error_new_msg(int code, const char *format, ...) {
  va_list ap;
  va_start(ap, format);
  GError *e =
      g_error_new_valist(g_quark_from_static_string("test"), code, format, ap);
  va_end(ap);
  return e;
}
static void fpi_device_verify_report(FpDevice *d, int match, void *p,
                                     GError *e) {
  g_assert_cmpint(match, ==, FPI_MATCH_FAIL);
  g_assert_null(p);
  g_assert_null(e);
  d->reports++;
}
static void fpi_device_identify_report(FpDevice *d, void *p, void *q,
                                       GError *e) {
  g_assert_null(p);
  g_assert_null(q);
  g_assert_null(e);
  d->reports++;
}
static void fpi_device_verify_complete(FpDevice *d, GError *e) {
  g_assert_null(e);
  d->completions++;
}
static void fpi_device_identify_complete(FpDevice *d, GError *e) {
  g_assert_null(e);
  d->completions++;
}
#include "driver-count-function.inc"
int main(void) {
  unsigned cases = 0;
  for (unsigned pid = 0; pid < 2; pid++)
    for (unsigned act = 0; act <= FPI_DEVICE_ACTION_IDENTIFY; act++)
      for (gsize n = 0; n < 5; n++)
        for (unsigned status = 0; status < 256; status++) {
          FpiSsm s = {0};
          FpDevice d = {.task_ssm = &s,
                        .pid = pid ? 0x0c5e : 0x0c7d,
                        .action = act,
                        .curr_enrolled = 99};
          d.list_result = g_ptr_array_new();
          uint8_t *reply = g_malloc0(MAX(n, 1));
          if (n)
            reply[0] = 0x40;
          if (n > 1)
            reply[1] = status;
          elanmoc_get_enrolled_cb(&d, reply, n, NULL);
          gboolean valid = n == 2 && status <= 10;
          gboolean empty =
              valid && pid && status == 0 && act >= FPI_DEVICE_ACTION_VERIFY;
          g_assert_cmpuint(s.failed, ==, !valid);
          g_assert_cmpuint(s.completed, ==, empty);
          g_assert_cmpuint(s.next, ==, valid && !empty);
          g_assert_cmpuint(d.completions, ==, empty);
          g_assert_cmpuint(d.reports, ==,
                           empty && act != FPI_DEVICE_ACTION_LIST);
          if (valid)
            g_assert_cmpuint(d.curr_enrolled, ==, status);
          g_clear_pointer(&d.list_result, g_ptr_array_unref);
          g_free(reply);
          cases++;
        }
  /* Wrong magic must be rejected even when the status byte says zero. */
  for (unsigned magic = 0; magic < 256; magic++)
    if (magic != 0x40) {
      FpiSsm s = {0};
      FpDevice d = {
          .task_ssm = &s, .pid = 0x0c5e, .action = FPI_DEVICE_ACTION_VERIFY};
      uint8_t reply[] = {magic, 0};
      elanmoc_get_enrolled_cb(&d, reply, 2, NULL);
      g_assert_cmpuint(s.failed, ==, 1);
      g_assert_cmpuint(d.reports, ==, 0);
      cases++;
    }
  for (gsize n = 0; n <= 8; n++) {
    FpiSsm state = {0};
    FpDevice d = {.task_ssm = &state, .pid = 0x0c5e};
    uint8_t *reply = g_malloc0(MAX(n, 1));
    const uint8_t dimensions[] = {0x4f, 0, 0x4f, 0};
    memcpy(reply, dimensions, MIN(n, sizeof(dimensions)));
    elanmoc_cmd_dim_cb(&d, reply, n, NULL);
    g_assert_cmpuint(state.next, ==, n == 4);
    g_assert_cmpuint(state.failed, ==, n != 4);
    memset(&state, 0, sizeof(state));
    if (n)
      reply[0] = 0x30;
    if (n > 1)
      reply[1] = 0x14;
    elanmoc_cmd_ver_cb(&d, reply, n, NULL);
    g_assert_cmpuint(state.next, ==, n == 2);
    g_assert_cmpuint(state.failed, ==, n != 2);
    g_free(reply);
    cases += 2;
  }
  for (unsigned fw = 0; fw < 65536; fw++) {
    FpiSsm state = {0};
    FpDevice d = {.task_ssm = &state, .pid = 0x0c5e};
    uint8_t reply[] = {fw >> 8, fw & 255};
    elanmoc_cmd_ver_cb(&d, reply, 2, NULL);
    g_assert_cmpuint(state.next, ==, fw == 0x3014);
    g_assert_cmpuint(state.failed, ==, fw != 0x3014);
    cases++;
  }
  g_print(
      "PASS COUNT/FW/DIM callbacks: %u cases, including stale-identity bypass "
      "prevention\n",
      cases);
}
