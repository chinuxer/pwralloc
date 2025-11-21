#include "pdu_param.h"
#include "interware.h"
#include <stdlib.h>
#define __DATABASE_IMPORT__
#include "pdu_main.h"

KeyValue_Array *gTopoConfigParam = NULL;


void pseudotopos(void)
{
    KeyValue_Array *param_ref = NULL;
    (void)param_ref;
    /* Not used, avoid warning */
    pdu_calloc(0);
    add_KeyValue(NULL,0);
#include "pseudotopos.h"
    return;
}

// 初始化RTT
void rtt_init(void)
{
    // 初始化RTT缓冲区，通道0用于输入输出
    SEGGER_RTT_Init();
    SEGGER_RTT_ConfigUpBuffer(0, NULL, NULL, 0, SEGGER_RTT_MODE_NO_BLOCK_TRIM);
    SEGGER_RTT_ConfigDownBuffer(0, NULL, NULL, 0, SEGGER_RTT_MODE_NO_BLOCK_TRIM);
}

Contactor *get_ContactorInfo_ExportPDU(uint8_t contactor_id)
{
    if (contactor_id > sizeof(ContactorObj) / sizeof(ContactorObj[0]))
    {
        return NULL;
    }
    return ContactorObj + contactor_id;
}
PowerDemand *get_PwrplugInfo_ExportPDU(uint8_t plug_id)
{
    if (plug_id > sizeof(PwrDemandObj) / sizeof(PwrDemandObj[0]))
    {
        return NULL;
    }
    return PwrDemandObj + plug_id - 1;
}
PowerSupply *get_PwrnodeInfo_ExportPDU(uint8_t node_id)
{
    if (node_id > sizeof(PwrSupplyObj) / sizeof(PwrSupplyObj[0]))
    {
        return NULL;
    }
    return PwrSupplyObj + node_id;
}

void set_Pwrnode_Output(uint8_t contactor_id, float voltage, float current)
{
    // print_oneliner("[PCU] set_Pwrnode_Output: contactor_id=%d, voltage=%f, current=%f", contactor_id, voltage, current);
}
void trace_Alarm(uint8_t node_id, uint8_t alarm_level, uint8_t alarm_status)
{
    print_oneliner("trace_Alarm: node_id=%d, alarm_level=%d, alarm_status=%d", node_id, alarm_level, alarm_status);
}
void register_to_pdu(void)
{
    SYMBOL_TYPE symbol;
    EXPORT_COPYOUT_OF(symbol, set_Pwrnode_Output);
    EXPORT_COPYOUT_OF(symbol, PwrDemandObj);
    EXPORT_COPYOUT_OF(symbol, PwrSupplyObj);
    EXPORT_COPYOUT_OF(symbol, ContactorObj);
    EXPORT_COPYOUT_OF(symbol, get_PwrnodeInfo_ExportPDU);
    EXPORT_COPYOUT_OF(symbol, get_PwrplugInfo_ExportPDU);
    EXPORT_COPYOUT_OF(symbol, get_ContactorInfo_ExportPDU);
    EXPORT_COPYOUT_OF(symbol, trace_Alarm);
}

void pseudata_init(void)
{
    // 虚拟数据初始化
    for (int i = 0; i < UPPER_LIMIT_OF_NODES; i++)
    {
        PwrSupplyObj[i] = (PowerSupply){
            .pwrnode_id = (uint8_t)(i + 1),
            .max_voltage = 450.0f,
            .max_current = 100.0f,
            .max_power = 40.0f,
            .current_power = 0.0f,
            .temperature = 25.0f + rand() % 100 / 10.0f,
            .work_time = (rand() % 10000) + 1,
            .status = MOD_INIT,
        };
    }
    for (int i = 0; i < UPPER_LIMIT_OF_PLUGS; i++)
    {
        PwrDemandObj[i] = (PowerDemand){
            .plugin_id = (uint8_t)(i + 1),
            .voltage_req = 0.0f,
            .current_req = 0.0f,
            .power_req = 0.0f,
            .priority = 0,
            .status = PLUGIN_UNPLUGGED,
        };
    }
    for (int i = 0; i < UPPER_LIMIT_OF_CONTACTORS; i++)
    {
        ContactorObj[i] = (Contactor){
            .contactor_id = (uint8_t)(i + 1),
            .onoff = false,
        };
    }
}

void fake_sub_routine(void)
{
    static uint32_t cnt = 0;
    if (0 == (cnt++ & 0x10000))
    {
        return;
    }
    cnt = 0;
    // 虚拟数据更新
    for (int i = 0; i < UPPER_LIMIT_OF_PLUGS; i++)
    {
        if (PwrDemandObj[i].status == PLUGIN_INSULATCHK)
        {
            PwrDemandObj[i].status = PLUGIN_CHARGING;
            continue;
        }
        if (PwrDemandObj[i].status == PLUGIN_PLUGGED)
        {
            PwrDemandObj[i].status = PLUGIN_INSULATCHK;
           // PwrDemandObj[i].current_req = PwrDemandObj[i].max_req * (0.5 + rand() % 50 / 100.0);
            FSM_mainEntry_PDU(PDU_CMD_CHARGING, i + 1, PwrDemandObj[i].max_req);
        }
    }
}

void rtt_cli_task(void);
int main(void)
{
    pseudata_init();
    register_to_pdu();
    rtt_init();
    pseudotopos();

    if (PDU_STA_WORKING != FSM_mainEntry_PDU(PDU_CMD_INITIAT).retcode)
    {
        return 0;
    }
    while (true)
    {
        rtt_cli_task();
        fake_sub_routine();
        FSM_mainEntry_PDU(PDU_CMD_WORKING);
    }
}