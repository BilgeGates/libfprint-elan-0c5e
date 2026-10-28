/* SPDX-License-Identifier: LGPL-2.1-or-later
 * Deterministic mutation tests of the actual private parsers. Synthetic only.
 */
#include <json-glib/json-glib.h>
#include "../libfprint/drivers/elanmoc/elanmoc.c"

G_GNUC_NORETURN int
libusb_init (libusb_context **context)
{
  (void) context;
  g_error ("Physical USB initialization forbidden in parser tests");
}

G_GNUC_NORETURN void
g_usb_device_bulk_transfer_async (GUsbDevice *self, guint8 endpoint,
                                   guint8 *buffer, gsize length, guint timeout,
                                   GCancellable *cancellable,
                                   GAsyncReadyCallback callback, gpointer userdata)
{
  (void) self; (void) endpoint; (void) buffer; (void) length; (void) timeout;
  (void) cancellable; (void) callback; (void) userdata;
  g_error ("Parser tests must not submit USB transfers");
}

static void
initialized (GObject *object, GAsyncResult *result, gpointer data)
{
  g_autoptr(GError) error = NULL;
  g_assert_true (g_async_initable_init_finish (G_ASYNC_INITABLE (object), result, &error));
  g_assert_no_error (error);
  *(gboolean *) data = TRUE;
}

static void
check_frame (FpiDeviceElanmoc *self, const guint8 *frame, gsize length,
             gboolean modern)
{
  /* Exact allocation catches reads past actual_length, not only buffer capacity. */
  g_autofree guint8 *copy = length ? g_memdup2 (frame, length) : NULL;
  g_autoptr(GError) error = NULL;
  g_autoptr(FpPrint) print = modern ?
    create_print_from_response (self, copy, length, &error) :
    create_print_from_0c5e_response (self, copy, length, &error);
  if (print)
    {
      g_assert_no_error (error);
      g_assert_cmpuint (length, >=, modern ? 5 : 64);
      if (!modern)
        {
          g_assert_cmpuint (length, <=, 71);
          g_assert_cmpuint (copy[6], <=, 57);
          g_assert_cmpuint (copy[6], >=, 24);
        }
    }
}

int
main (int argc, char **argv)
{
  g_assert_cmpint (argc, ==, 2);
  g_setenv ("FP_DEVICE_EMULATION", "1", TRUE);
  g_autoptr(GError) error = NULL;
  g_autoptr(GUsbContext) context = g_object_new (G_USB_TYPE_CONTEXT, NULL);
  g_autoptr(JsonParser) parser = json_parser_new ();
  g_assert_true (json_parser_load_from_data (parser,
    "{\"UsbDevices\":[{\"PlatformId\":\"security-test\",\"IdVendor\":1267,\"IdProduct\":3166}]}", -1, &error));
  g_assert_true (g_usb_context_load (context,
    json_node_get_object (json_parser_get_root (parser)), &error));
  g_autoptr(GPtrArray) devices = g_usb_context_get_devices (context);
  g_assert_cmpuint (devices->len, ==, 1);
  g_autoptr(FpDevice) device = g_object_new (fpi_device_elanmoc_get_type (),
    "fpi-usb-device", g_ptr_array_index (devices, 0), NULL);
  gboolean ready = FALSE;
  g_async_initable_init_async (G_ASYNC_INITABLE (device), 0, NULL, initialized, &ready);
  while (!ready) g_main_context_iteration (NULL, TRUE);
  FpiDeviceElanmoc *self = FPI_DEVICE_ELANMOC (device);
  if (g_str_equal (argv[1], "modern-short"))
    {
      const guint8 short_frame[] = { 0x43 };
      check_frame (self, short_frame, sizeof short_frame, TRUE);
    }
  else if (g_str_equal (argv[1], "modern-date"))
    {
      const char *uid = "FP1-20260230-7-12345678-offline-fuzz";
      guint8 frame[64] = {0x43, 0};
      frame[4] = strlen (uid); memcpy (frame + 5, uid, strlen (uid));
      g_autoptr(GError) bad_error = NULL;
      g_autoptr(FpPrint) print = create_print_from_response (self, frame, sizeof frame, &bad_error);
      g_assert_null (print); g_assert_nonnull (bad_error);
    }
  else
    {
      g_autoptr(GRand) random = g_rand_new_with_seed (0x0c5e3014);
      const gchar *uid = "FP1-20261027-7-12345678-offline-fuzz";
      guint8 valid[128] = {0x40, 0};
      valid[6] = strlen (uid);
      memcpy (valid + 7, uid, strlen (uid));
      guint cases = 0;
      gint64 started = g_get_monotonic_time ();
      for (guint modern = 0; modern <= (g_str_equal (argv[1], "all") ? 1u : 0u); modern++)
        {
          for (guint length = 0; length <= 128; length++)
            for (guint fill = 0; fill <= 255; fill++)
              {
                guint8 frame[128];
                memset (frame, fill, sizeof frame);
                check_frame (self, frame, length, modern); cases++;
              }
          /* Mutate each byte of both accepted legacy frame sizes. */
          if (!modern)
            for (guint length = 64; length <= 71; length += 7)
              for (guint offset = 0; offset < length; offset++)
                for (guint value = 0; value <= 255; value++)
                  {
                    guint8 frame[128];
                    memcpy (frame, valid, sizeof frame); frame[offset] = value;
                    check_frame (self, frame, length, FALSE); cases++;
                  }
          for (guint i = 0; i < 100000; i++)
            {
              guint8 frame[128];
              guint length = g_rand_int_range (random, 0, 129);
              memcpy (frame, valid, sizeof frame);
              if (modern)
                {
                  memset (frame, 0, sizeof frame); frame[0] = 0x43;
                  frame[4] = strlen (uid); memcpy (frame + 5, uid, strlen (uid));
                }
              for (guint mutation = 0; mutation < 4; mutation++)
                frame[g_rand_int_range (random, 0, 128)] = g_rand_int_range (random, 0, 256);
              check_frame (self, frame, length, modern); cases++;
            }
        }
      g_print ("PASS actual parser mutations: %u cases in %.3f seconds\n",
               cases, (g_get_monotonic_time () - started) / 1000000.0);
    }
  g_object_run_dispose (G_OBJECT (context));
  return 0;
}
