#include "ble_travel.h"
#include "ble_core.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "host/ble_gatt.h"
#include "host/ble_hs.h"
#include "services/gap/ble_svc_gap.h"
#include <string.h>

static const char *TAG="ble_travel";
#define DEVICE_NAME "JarvisTravel"
static const ble_uuid128_t SVC_UUID=BLE_UUID128_INIT(
    0x00,0x13,0xcd,0x34,0xc0,0x51,0x2e,0x9d,0x8a,0x4f,0x91,0x2c,0x01,0x90,0x3a,0x7f);
static const ble_uuid128_t RX_UUID=BLE_UUID128_INIT(
    0x00,0x13,0xcd,0x34,0xc0,0x51,0x2e,0x9d,0x8a,0x4f,0x91,0x2c,0x02,0x90,0x3a,0x7f);
static portMUX_TYPE s_mux=portMUX_INITIALIZER_UNLOCKED;
static bool s_running,s_active,s_synced;
static uint8_t s_addr_type;
static uint32_t s_epoch;
static uint16_t s_conn=BLE_HS_CONN_HANDLE_NONE;
static uint16_t s_rx_handle;
static travel_data_t s_data;

static void start_advertising(void);
static void travel_sync(uint8_t addr_type){
    portENTER_CRITICAL(&s_mux);s_addr_type=addr_type;s_synced=true;portEXIT_CRITICAL(&s_mux);
    start_advertising();
}
static int write_access(uint16_t conn,uint16_t attr,struct ble_gatt_access_ctxt *ctxt,void *arg){
    (void)attr;(void)arg;
    if(ctxt->op!=BLE_GATT_ACCESS_OP_WRITE_CHR)return BLE_ATT_ERR_UNLIKELY;
    uint8_t frame[TRAVEL_FRAME_SIZE];uint16_t len=0;
    if(ble_hs_mbuf_to_flat(ctxt->om,frame,sizeof frame,&len)!=0 || len!=sizeof frame)
        return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
    uint32_t now=(uint32_t)(esp_timer_get_time()/1000);
    portENTER_CRITICAL(&s_mux);
    bool accepted=s_active && s_conn==conn && travel_data_accept(&s_data,frame,len,now);
    portEXIT_CRITICAL(&s_mux);
    return accepted?0:BLE_ATT_ERR_VALUE_NOT_ALLOWED;
}
const struct ble_gatt_svc_def BLE_TRAVEL_SVCS[]={
    {.type=BLE_GATT_SVC_TYPE_PRIMARY,.uuid=&SVC_UUID.u,.characteristics=(struct ble_gatt_chr_def[]){
        {.uuid=&RX_UUID.u,.access_cb=write_access,.flags=BLE_GATT_CHR_F_WRITE|BLE_GATT_CHR_F_WRITE_ENC,.val_handle=&s_rx_handle},
        {0},}},
    {0},
};
static int gap_event(struct ble_gap_event *ev,void *arg){
    (void)arg;
    switch(ev->type){
    case BLE_GAP_EVENT_CONNECT:
        if(ev->connect.status==0){
            portENTER_CRITICAL(&s_mux);if(s_active){s_conn=ev->connect.conn_handle;travel_data_reset(&s_data);}portEXIT_CRITICAL(&s_mux);
        }else start_advertising();
        return 0;
    case BLE_GAP_EVENT_DISCONNECT:
        portENTER_CRITICAL(&s_mux);if(s_conn==ev->disconnect.conn.conn_handle){s_conn=BLE_HS_CONN_HANDLE_NONE;travel_data_reset(&s_data);}bool active=s_active;portEXIT_CRITICAL(&s_mux);
        if(active)start_advertising();
        return 0;
    case BLE_GAP_EVENT_ADV_COMPLETE:start_advertising();return 0;
    default:return 0;
    }
}
// Keep the callback tied to the app instance that created this advertisement.
static int epoch_gap_event(struct ble_gap_event *ev,void *arg){
    uint32_t epoch=(uint32_t)(uintptr_t)arg;
    portENTER_CRITICAL(&s_mux);bool current=s_active&&s_epoch==epoch;portEXIT_CRITICAL(&s_mux);
    if(!current){
        if(ev->type==BLE_GAP_EVENT_CONNECT&&ev->connect.status==0)ble_gap_terminate(ev->connect.conn_handle,BLE_ERR_REM_USER_CONN_TERM);
        return 0;
    }
    return gap_event(ev,arg);
}
static void advertise_epoch(void){
    uint8_t addr;uint32_t epoch;bool ready;
    portENTER_CRITICAL(&s_mux);ready=s_running&&s_active&&s_synced;addr=s_addr_type;epoch=s_epoch;portEXIT_CRITICAL(&s_mux);
    if(!ready)return;
    struct ble_hs_adv_fields fields={0};fields.flags=BLE_HS_ADV_F_DISC_GEN|BLE_HS_ADV_F_BREDR_UNSUP;
    fields.uuids128=(ble_uuid128_t[]){SVC_UUID};fields.num_uuids128=1;fields.uuids128_is_complete=1;
    if(ble_gap_adv_set_fields(&fields)!=0)return;
    struct ble_hs_adv_fields rsp={0};rsp.name=(uint8_t*)DEVICE_NAME;rsp.name_len=strlen(DEVICE_NAME);rsp.name_is_complete=1;
    if(ble_gap_adv_rsp_set_fields(&rsp)!=0)return;
    struct ble_gap_adv_params params={0};params.conn_mode=BLE_GAP_CONN_MODE_UND;params.disc_mode=BLE_GAP_DISC_MODE_GEN;
    int rc=ble_gap_adv_start(addr,NULL,BLE_HS_FOREVER,&params,epoch_gap_event,(void*)(uintptr_t)epoch);
    if(rc!=0&&rc!=BLE_HS_EALREADY)ESP_LOGW(TAG,"advertising start rc=%d",rc);
}
static void start_advertising(void){advertise_epoch();}
bool ble_travel_start(void){
    portENTER_CRITICAL(&s_mux);if(s_active){portEXIT_CRITICAL(&s_mux);return true;}s_active=true;++s_epoch;travel_data_reset(&s_data);s_running=true;portEXIT_CRITICAL(&s_mux);
    ble_svc_gap_device_name_set(DEVICE_NAME);
    if(!ble_core_start(travel_sync)){
        portENTER_CRITICAL(&s_mux);s_active=false;s_running=false;portEXIT_CRITICAL(&s_mux);return false;
    }
    ESP_LOGI(TAG,"advertising as %s",DEVICE_NAME);return true;
}
void ble_travel_stop(void){
    uint16_t conn;portENTER_CRITICAL(&s_mux);if(!s_active){portEXIT_CRITICAL(&s_mux);return;}s_active=false;++s_epoch;conn=s_conn;s_conn=BLE_HS_CONN_HANDLE_NONE;travel_data_reset(&s_data);portEXIT_CRITICAL(&s_mux);
    int rc=ble_gap_adv_stop();if(rc!=0&&rc!=BLE_HS_EALREADY&&rc!=BLE_HS_EBUSY)ESP_LOGW(TAG,"advertising stop rc=%d",rc);
    if(conn!=BLE_HS_CONN_HANDLE_NONE){rc=ble_gap_terminate(conn,BLE_ERR_REM_USER_CONN_TERM);if(rc!=0&&rc!=BLE_HS_ENOTCONN)ESP_LOGW(TAG,"disconnect rc=%d",rc);}
}
void ble_travel_snapshot(travel_snapshot_t *out){
    if(!out)return;
    uint32_t now=(uint32_t)(esp_timer_get_time()/1000);
    portENTER_CRITICAL(&s_mux);out->active=s_active;out->connected=s_active&&s_conn!=BLE_HS_CONN_HANDLE_NONE;out->data=s_data;portEXIT_CRITICAL(&s_mux);
    out->status=travel_data_status(&out->data,now);
}
