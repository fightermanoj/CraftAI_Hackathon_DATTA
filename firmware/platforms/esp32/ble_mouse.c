#include "mouse_output_if.h"
#include "app_config.h"
#include "logger.h"
#include "esp_err.h"
#include "host/ble_hs.h"
#include "host/ble_gap.h"
#include "host/ble_gatt.h"
#include "host/ble_uuid.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"
#include "os/os_mbuf.h"
#include <string.h>

static const char *TAG = "ble_mouse";
static uint16_t s_conn = BLE_HS_CONN_HANDLE_NONE;
static uint16_t s_report_handle;
static uint8_t s_addr_type;
static bool s_ready;
static int gap_event(struct ble_gap_event *event, void *arg);

static const uint8_t s_report_map[] = {
    0x05,0x01,0x09,0x02,0xA1,0x01,0x09,0x01,0xA1,0x00,
    0x05,0x09,0x19,0x01,0x29,0x03,0x15,0x00,0x25,0x01,0x95,0x03,0x75,0x01,0x81,0x02,
    0x95,0x01,0x75,0x05,0x81,0x03,0x05,0x01,0x09,0x30,0x09,0x31,0x15,0x81,0x25,0x7F,
    0x75,0x08,0x95,0x02,0x81,0x06,0xC0,0xC0
};
static uint8_t s_protocol;
static uint8_t s_control;

static int access_cb(uint16_t conn, uint16_t attr, struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    uintptr_t id = (uintptr_t)arg;
    if (id == 1) return os_mbuf_append(ctxt->om, s_report_map, sizeof(s_report_map)) == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
    if (id == 2) return os_mbuf_append(ctxt->om, &s_protocol, 1) == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
    if (id == 3) return os_mbuf_append(ctxt->om, &s_control, 1) == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
    return 0;
}

static const struct ble_gatt_svc_def s_services[] = {
    { .type = BLE_GATT_SVC_TYPE_PRIMARY, .uuid = BLE_UUID16_DECLARE(0x1812), .characteristics = (struct ble_gatt_chr_def[]){
        { .uuid = BLE_UUID16_DECLARE(0x2A4A), .access_cb = access_cb, .arg = (void *)1, .flags = BLE_GATT_CHR_F_READ },
        { .uuid = BLE_UUID16_DECLARE(0x2A4B), .access_cb = access_cb, .arg = (void *)2, .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_WRITE_NO_RSP },
        { .uuid = BLE_UUID16_DECLARE(0x2A4C), .access_cb = access_cb, .arg = (void *)3, .flags = BLE_GATT_CHR_F_WRITE_NO_RSP },
        { .uuid = BLE_UUID16_DECLARE(0x2A4D), .val_handle = &s_report_handle, .flags = BLE_GATT_CHR_F_NOTIFY },
        { 0 }
    }}, { 0 }
};

static void advertise(void)
{
    struct ble_hs_adv_fields fields = {0};
    struct ble_gap_adv_params params = {0};
    fields.flags = BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP;
    fields.name = (uint8_t *)APP_BLE_NAME;
    fields.name_len = strlen(APP_BLE_NAME);
    fields.name_is_complete = 1;
    fields.appearance = 0x03C2;
    fields.appearance_is_present = 1;
    ble_gap_adv_set_fields(&fields);
    params.conn_mode = BLE_GAP_CONN_MODE_UND;
    params.disc_mode = BLE_GAP_DISC_MODE_GEN;
    int rc = ble_gap_adv_start(s_addr_type, NULL, BLE_HS_FOREVER, &params, gap_event, NULL);
    if (rc != 0) ESP_LOGE(TAG, "advertising failed rc=%d", rc);
    else ESP_LOGI(TAG, "advertising as '%s'", APP_BLE_NAME);
}

static int gap_event(struct ble_gap_event *event, void *arg)
{
    switch (event->type) {
    case BLE_GAP_EVENT_CONNECT:
        if (event->connect.status == 0) {
            s_conn = event->connect.conn_handle;
            s_ready = true;
            ESP_LOGI(TAG, "host connected");
        } else advertise();
        break;
    case BLE_GAP_EVENT_DISCONNECT:
        s_conn = BLE_HS_CONN_HANDLE_NONE;
        s_ready = false;
        advertise();
        break;
    case BLE_GAP_EVENT_ADV_COMPLETE:
        advertise();
        break;
    default:
        break;
    }
    return 0;
}

static void on_sync(void)
{
    int rc = ble_hs_id_infer_auto(0, &s_addr_type);
    if (rc == 0) advertise();
}

static void host_task(void *arg)
{
    nimble_port_run();
    nimble_port_freertos_deinit();
}

bool mouse_output_init(void)
{
    esp_err_t err = nimble_port_init();
    if (err != ESP_OK) return false;
    ble_hs_cfg.sync_cb = on_sync;
    ble_svc_gap_init();
    ble_svc_gatt_init();
    if (ble_gatts_count_cfg(s_services) != 0 || ble_gatts_add_svcs(s_services) != 0) return false;
    if (ble_svc_gap_device_name_set(APP_BLE_NAME) != 0) return false;
    ble_hs_cfg.sm_io_cap = BLE_HS_IO_NO_INPUT_OUTPUT;
    ble_hs_cfg.sm_bonding = 1;
    nimble_port_freertos_init(host_task);
    return true;
}

bool mouse_output_connected(void)
{
    return s_ready && s_conn != BLE_HS_CONN_HANDLE_NONE;
}

bool mouse_output_send(int8_t dx, int8_t dy)
{
    if (!s_ready || s_conn == BLE_HS_CONN_HANDLE_NONE) return false;
    uint8_t report[3] = {0, (uint8_t)dx, (uint8_t)dy};
    struct os_mbuf *om = ble_hs_mbuf_from_flat(report, sizeof(report));
    return om && ble_gatts_notify_custom(s_conn, s_report_handle, om) == 0;
}
