/* SPDX-License-Identifier: LGPL-2.1-or-later
 * Real driver + public API, synthetic GUsb only. No libusb initialization.
 */
#include <drivers_api.h>
#include <execinfo.h>
#include <unistd.h>
#include <json-glib/json-glib.h>
#include "drivers/elanmoc/elanmoc.h"

static guint8 command[256], identity[69];
static gsize command_size;
static guint enrolled, captures, info_reads, aborts;
static gboolean identified, match_next = TRUE, cancel_next, closing;
static guint ready_reads;
static GCancellable *external_cancel;
static const char *scenario;
static gboolean mode (const char *name) { return g_str_equal (scenario, name); }

G_GNUC_NORETURN int libusb_init (libusb_context **context)
{
  (void) context;
  g_error ("Physical USB initialization is forbidden in the offline test");
}

void g_usb_device_bulk_transfer_async (GUsbDevice *self, guint8 endpoint,
                                       guint8 *buffer, gsize length, guint timeout,
                                       GCancellable *cancellable,
                                       GAsyncReadyCallback callback, gpointer userdata)
{
  (void) timeout;
  g_assert_true (g_usb_device_is_emulated (self));
  GTask *task = g_task_new (self, cancellable, callback, userdata);
  if (!(endpoint & 0x80))
    {
      g_assert_cmpuint (length, <=, sizeof command);
      memcpy (command, buffer, MIN (length, sizeof command));
      command_size = length;
      if (length == 3 && command[2] == 2)
        {
          aborts++;
          if ((closing && mode ("close-out-error")) ||
              (!closing && mode ("abort-failed")))
            {
              g_task_return_new_error (task, G_IO_ERROR, G_IO_ERROR_FAILED,
                                       "synthetic abort/close transport failure");
              g_object_unref (task); return;
            }
        }
      if (length >= 3 && command[2] == 0x11)
        {
          g_assert_cmpuint (length, ==, 72);
          memcpy (identity, command + 3, sizeof identity);
          if (mode ("short-commit-out"))
            { g_task_return_int (task, length - 1); g_object_unref (task); return; }
        }
      if (length >= 3 && command[2] == 0x13)
        {
          g_assert_cmpuint (length, ==, 72);
          g_assert_cmpmem (command + 3, 69, identity, 69);
          if (mode ("short-delete-out"))
            { g_task_return_int (task, length - 1); g_object_unref (task); return; }
        }
      g_task_return_int (task, length);
    }
  else
    {
      guint8 response[128] = {0x40, 0};
      gsize size = 2;
      g_assert_cmpuint (command_size, >=, 2);
      if (command_size == 2 && command[1] == 0x19)
        { response[0] = 0x30; response[1] = mode ("wrong-fw") ? 0x15 : 0x14; }
      else if (command_size == 2 && command[1] == 0x0c)
        { memcpy (response, "\x4f\0\x4f\0", 4); size = 4; if (mode ("wrong-dim")) response[2]++; }
      else if (command[2] == 0)
        { ready_reads++; response[1] = mode ("ready-last") && ready_reads <= ELAN_MOC_CAL_RETRY ? 0 : 3; }
      else if (command[2] == 4) response[1] = mode ("bad-count") ? 11 : enrolled;
      else if (command[2] == 1)
        { g_assert_cmpuint (endpoint, ==, 0x84); captures++; }
      else if (command[2] == 3)
        {
          g_assert_cmpuint (endpoint, ==, 0x84);
          if (cancel_next)
            {
              cancel_next = FALSE;
              if (external_cancel) g_cancellable_cancel (external_cancel);
              g_task_return_new_error (task, G_IO_ERROR, G_IO_ERROR_CANCELLED, "synthetic cancelled scan");
              g_object_unref (task); return;
            }
          identified = enrolled && match_next;
          response[1] = identified ? 0 : 0xfd;
        }
      else if (command[2] == 0x11)
        { if (mode ("commit-refused")) response[1] = 0xff; else enrolled++; }
      else if (command[2] == 0x13)
        {
          if (mode ("delete-refused")) response[1] = 0xff;
          else if (!mode ("delete-no-effect")) enrolled--;
        }
      else if (command[2] == 0x12)
        {
          info_reads++;
          if (!identified || mode ("info-rejected")) response[1] = 0xff;
          else
            {
              size = mode ("info64") ? 64 : 71;
              memcpy (response + 2, identity, size - 2);
              if (mode ("short-info")) size = 7;
              if (mode ("empty-info")) size = 0;
              if (mode ("wrong-uid")) response[22] = response[22] == '0' ? '1' : '0';
              if (mode ("foreign-info")) memcpy (response + 7, "WIN-", 4);
              if (mode ("bad-date")) memcpy (response + 11, "20260230", 8);
              if (mode ("bad-utf8")) response[31] = 0xff;
              if (mode ("long-info-uid")) response[6] = 255;
              if (mode ("nul-info-uid")) response[12] = 0;
            }
        }
      else
        {
          g_assert_cmpuint (command[2], ==, 0x14);
          if (mode ("short-mode")) size = 1;
        }
      g_assert_cmpuint (length, >=, size);
      memcpy (buffer, response, size);
      g_task_return_int (task, size);
    }
  g_object_unref (task);
}

static void initialized (GObject *object, GAsyncResult *result, gpointer pointer)
{
  g_autoptr(GError) error = NULL;
  g_assert_true (g_async_initable_init_finish (G_ASYNC_INITABLE (object), result, &error));
  g_assert_no_error (error);
  *(gboolean *) pointer = TRUE;
}

static void fatal_trace (const gchar *domain, GLogLevelFlags level,
                         const gchar *message, gpointer pointer)
{
  (void) domain; (void) level; (void) pointer;
  void *frames[40];
  g_printerr ("GLib fatal: %s\n", message);
  backtrace_symbols_fd (frames, backtrace (frames, 40), STDERR_FILENO);
}

int main (int argc, char **argv)
{
  g_assert_cmpint (argc, ==, 2); scenario = argv[1];
  g_log_set_handler ("GLib", G_LOG_LEVEL_ERROR | G_LOG_FLAG_FATAL, fatal_trace, NULL);
  g_setenv ("FP_DEVICE_EMULATION", "1", TRUE);
  g_autoptr(GError) error = NULL;
  g_autoptr(GCancellable) verify_cancel = NULL;
  /* Bypass GInitable deliberately: no libusb context, discovery or hardware. */
  g_autoptr(GUsbContext) usb_context = g_object_new (G_USB_TYPE_CONTEXT, NULL);
  g_autoptr(JsonParser) parser = json_parser_new ();
  g_assert_true (json_parser_load_from_data (parser,
    "{\"UsbDevices\":[{\"PlatformId\":\"offline-only\",\"IdVendor\":1267,\"IdProduct\":3166}]}", -1, &error));
  g_assert_true (g_usb_context_load (usb_context,
    json_node_get_object (json_parser_get_root (parser)), &error));
  g_autoptr(GPtrArray) devices = g_usb_context_get_devices (usb_context);
  g_assert_cmpuint (devices->len, ==, 1);
  GUsbDevice *usb = g_ptr_array_index (devices, 0);
  g_assert_true (g_usb_device_is_emulated (usb));
  g_autoptr(FpDevice) device = g_object_new (fpi_device_elanmoc_get_type (), "fpi-usb-device", usb, NULL);
  gboolean ready = FALSE;
  g_async_initable_init_async (G_ASYNC_INITABLE (device), 0, NULL, initialized, &ready);
  while (!ready) g_main_context_iteration (NULL, TRUE);
  g_assert_false (fp_device_has_feature (device, FP_DEVICE_FEATURE_STORAGE_LIST));
  g_assert_false (fp_device_has_feature (device, FP_DEVICE_FEATURE_STORAGE_CLEAR));
  g_assert_true (fp_device_has_feature (device, FP_DEVICE_FEATURE_STORAGE_DELETE));
  gboolean opened = fp_device_open_sync (device, NULL, &error);
  if (mode ("wrong-fw") || mode ("wrong-dim") || mode ("short-mode") || mode ("bad-count"))
    {
      g_assert_false (opened); g_assert_nonnull (error); g_clear_error (&error);
      g_assert_cmpuint (captures, ==, 0); g_assert_cmpuint (enrolled, ==, 0);
      g_print ("PASS rejected invalid initialization: %s\n", scenario);
      g_object_run_dispose (G_OBJECT (usb_context));
      return 0;
    }
  g_assert_true (opened); g_assert_no_error (error);
  g_autoptr(GPtrArray) prints = fp_device_list_prints_sync (device, NULL, &error);
  g_assert_null (prints);
  g_assert_error (error, FP_DEVICE_ERROR, FP_DEVICE_ERROR_NOT_SUPPORTED);
  g_clear_error (&error);
  g_assert_cmpuint (info_reads, ==, 0);
  /* Own the template reference before passing its floating ref to enroll. */
  g_autoptr(FpPrint) template = g_object_ref_sink (fp_print_new (device));
  g_autofree gchar *long_username = g_strnfill (64, 'x');
  fp_print_set_username (template, mode ("long-user") ? long_username : "offline-test");
  fp_print_set_finger (template, FP_FINGER_RIGHT_INDEX);
  g_autoptr(FpPrint) print = fp_device_enroll_sync (device, template, NULL, NULL, NULL, &error);
  if (mode ("commit-refused") || mode ("short-commit-out") || mode ("long-user"))
    {
      g_assert_null (print); g_assert_nonnull (error); g_clear_error (&error);
      g_assert_cmpuint (enrolled, ==, 0); goto done;
    }
  g_assert_no_error (error); g_assert_nonnull (print);
  g_assert_cmpuint (captures, ==, 9);
  gboolean matched = FALSE;
  gboolean ok = fp_device_verify_sync (device, print, NULL, NULL, NULL, &matched, NULL, &error);
  if (mode ("info-rejected") || mode ("short-info") || mode ("empty-info") ||
      mode ("bad-date") || mode ("bad-utf8") || mode ("long-info-uid") || mode ("nul-info-uid"))
    {
      g_assert_false (ok); g_assert_false (matched); g_assert_nonnull (error);
      g_clear_error (&error); goto done;
    }
  if (mode ("wrong-uid") || mode ("foreign-info"))
    { g_assert_true (ok); g_assert_no_error (error); g_assert_false (matched); goto done; }
  g_assert_true (ok); g_assert_no_error (error); g_assert_true (matched);
  match_next = FALSE;
  guint previous_info = info_reads;
  g_assert_true (fp_device_verify_sync (device, print, NULL, NULL, NULL, &matched, NULL, &error));
  g_assert_no_error (error); g_assert_false (matched);
  g_assert_cmpuint (info_reads, ==, previous_info);
  cancel_next = TRUE;
  if (mode ("real-cancel"))
    { verify_cancel = g_cancellable_new (); external_cancel = verify_cancel; }
  g_assert_false (fp_device_verify_sync (device, print, verify_cancel, NULL, NULL, &matched, NULL, &error));
  external_cancel = NULL;
  gint expected_cancel_error = mode ("abort-failed") ? G_IO_ERROR_FAILED : G_IO_ERROR_CANCELLED;
  g_assert_error (error, G_IO_ERROR, expected_cancel_error);
  g_clear_error (&error);
  g_assert_cmpuint (aborts, ==, 1);
  match_next = TRUE;
  g_assert_true (fp_device_verify_sync (device, print, NULL, NULL, NULL, &matched, NULL, &error));
  g_assert_no_error (error); g_assert_true (matched);
  previous_info = info_reads;
  ok = fp_device_delete_print_sync (device, print, NULL, &error);
  g_assert_cmpuint (info_reads, ==, previous_info);
  if (mode ("delete-refused") || mode ("delete-no-effect") || mode ("short-delete-out"))
    { g_assert_false (ok); g_assert_nonnull (error); g_clear_error (&error); }
  else
    {
      g_assert_true (ok); g_assert_no_error (error); g_assert_cmpuint (enrolled, ==, 0);
      g_assert_true (fp_device_verify_sync (device, print, NULL, NULL, NULL, &matched, NULL, &error));
      g_assert_no_error (error); g_assert_false (matched);
    }
  if (mode ("stress"))
    {
      for (guint cycle = 0; cycle < 100; cycle++)
        {
          g_autoptr(FpPrint) cycle_template = g_object_ref_sink (fp_print_new (device));
          fp_print_set_username (cycle_template, "offline-stress");
          fp_print_set_finger (cycle_template, FP_FINGER_RIGHT_INDEX);
          g_autoptr(FpPrint) cycle_print = fp_device_enroll_sync (device, cycle_template, NULL, NULL, NULL, &error);
          g_assert_no_error (error); g_assert_nonnull (cycle_print);
          for (guint scan = 0; scan < 10; scan++)
            {
              g_assert_true (fp_device_verify_sync (device, cycle_print, NULL, NULL, NULL, &matched, NULL, &error));
              g_assert_no_error (error); g_assert_true (matched);
            }
          g_assert_true (fp_device_delete_print_sync (device, cycle_print, NULL, &error));
          g_assert_no_error (error); g_assert_cmpuint (enrolled, ==, 0);
        }
      g_print ("PASS 100 enrollment/delete cycles + 1000 matching operations\n");
    }
done:
  closing = TRUE;
  gboolean closed = fp_device_close_sync (device, NULL, &error);
  if (mode ("close-out-error"))
    { g_assert_false (closed); g_assert_error (error, G_IO_ERROR, G_IO_ERROR_FAILED); g_clear_error (&error); }
  else
    { g_assert_true (closed); g_assert_no_error (error); }
  g_print ("PASS full API + ownership/finalization: %s\n", scenario);
  /* Match FpContext disposal: break the synthetic context/device reference cycle. */
  g_object_run_dispose (G_OBJECT (usb_context));
  return 0;
}
