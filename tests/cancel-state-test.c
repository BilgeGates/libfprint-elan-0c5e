/* SPDX-License-Identifier: LGPL-2.1-or-later */
#include <gio/gio.h>
#include <stdint.h>
typedef struct {
  guint failures;
  GQuark domain;
  gint code;
} FpiSsm;
typedef struct {
  FpiSsm *task_ssm;
  GError *cancel_error;
  guint pid;
} FpiDeviceElanmoc;
typedef FpiDeviceElanmoc FpDevice;
typedef void (*Callback)(FpiDeviceElanmoc *, uint8_t *, gsize, GError *);
#define FP_DEVICE(p) (p)
#define FP_DEVICE_ERROR_PROTO 1
#define ELAN_MAX_HDR_LEN 3
struct elanmoc_cmd {
  unsigned char cmd_header[3];
  int cmd_len, resp_len;
};
static Callback pending;
static guint writes;
static void *fpi_device_get_usb_device(FpDevice *d) { return d; }
static guint g_usb_device_get_pid(void *d) { return ((FpDevice *)d)->pid; }
static GError *fpi_device_error_new(int n) {
  return g_error_new_literal(g_quark_from_static_string("test"), n, "protocol");
}
static void fpi_ssm_mark_failed(FpiSsm *s, GError *e) {
  g_assert_nonnull(e);
  s->failures++;
  s->domain = e->domain;
  s->code = e->code;
  g_error_free(e);
}
static void elanmoc_get_cmd(FpDevice *d, uint8_t *cmd, gsize n, gsize r,
                            gboolean c, Callback cb) {
  (void)d;
  g_assert_cmpuint(n, ==, 3);
  g_assert_cmpmem(cmd, n, "\x40\xff\x02", 3);
  g_assert_cmpuint(r, ==, 0);
  g_assert_false(c);
  g_free(cmd);
  pending = cb;
  writes++;
}
#include "driver-cancel-functions.inc"
int main(void) {
  for (unsigned mode = 0; mode < 6; mode++) {
    FpiSsm s = {0};
    FpDevice d = {.task_ssm = &s, .pid = mode == 1 ? 0x0c7d : 0x0c5e};
    pending = NULL;
    writes = 0;
    gboolean cancelling = mode != 2;
    GError *e = g_error_new_literal(
        G_IO_ERROR, cancelling ? G_IO_ERROR_CANCELLED : G_IO_ERROR_FAILED,
        "source error");
    gboolean handled = elanmoc_handle_cancel(&d, e);
    g_assert_cmpint(handled, ==, mode != 1 && mode != 2);
    g_assert_cmpuint(s.failures, ==, 0);
    if (!handled) {
      g_assert_cmpuint(writes, ==, 0);
      g_assert_null(d.cancel_error);
      g_error_free(e);
      continue;
    }
    g_assert_cmpuint(writes, ==, 1);
    g_assert_true(d.cancel_error == e);
    g_assert_nonnull(pending);
    GError *out_error =
        mode == 3   ? g_error_new_literal(G_IO_ERROR, G_IO_ERROR_BROKEN_PIPE,
                                          "abort write failed")
        : mode == 5 ? g_error_new_literal(G_IO_ERROR, G_IO_ERROR_TIMED_OUT,
                                          "abort write timeout")
                    : NULL;
    pending(&d, NULL, mode == 4 ? 1 : 0, out_error);
    g_assert_null(d.cancel_error);
    g_assert_cmpuint(writes, ==, 1);
    g_assert_cmpuint(s.failures, ==, 1);
    g_assert_cmpint(s.code, ==,
                    mode == 3   ? G_IO_ERROR_BROKEN_PIPE
                    : mode == 4 ? FP_DEVICE_ERROR_PROTO
                    : mode == 5 ? G_IO_ERROR_TIMED_OUT
                                : G_IO_ERROR_CANCELLED);
  }
  g_print("PASS cancellation: 6 cases; abort OUT completes before cancellation "
          "is reported\n");
}
