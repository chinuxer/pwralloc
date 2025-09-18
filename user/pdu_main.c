
#include "stm32f4xx.h"
#include "SEGGER_RTT.h"
#include "pdu_param.h"
#include "interware.h"
#define __DATABASE_IMPORT__
#include "pdu_main.h"

KeyValue_Array *gTopoConfigParam = NULL;

void pseudotopos(void)
{
    KeyValue_Array *param_ref = NULL;
    param_ref = param_ref;
    /* Not used, avoid warning */

#include "pseudotopos.h"
    return;
}

static char bidirectional_rx_buffer[RTT_RX_BUFFER_SIZE];
// 初始化RTT
void rtt_init(void)
{
    // 初始化RTT缓冲区，通道0用于输入输出
    SEGGER_RTT_Init();
    SEGGER_RTT_ConfigUpBuffer(0, NULL, NULL, 0, SEGGER_RTT_MODE_NO_BLOCK_TRIM);
    SEGGER_RTT_ConfigDownBuffer(0, "BidiChannel-Rx", bidirectional_rx_buffer, sizeof(bidirectional_rx_buffer), SEGGER_RTT_MODE_NO_BLOCK_TRIM);
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
    return PwrDemandObj + plug_id;
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
    print_oneliner("set_Pwrnode_Output: contactor_id=%d, voltage=%f, current=%f\r\n", contactor_id, voltage, current);
}
void trace_Alarm(uint8_t node_id, uint8_t alarm_level, uint8_t alarm_status)
{
    print_oneliner("trace_Alarm: node_id=%d, alarm_level=%d, alarm_status=%d\r\n", node_id, alarm_level, alarm_status);
}

PDU_STA Main_of_PDU(PDU_CMD cmd);
void register_to_pdu(void)
{
    EXPORT_COPYOUT_OF(set_Pwrnode_Output);
    EXPORT_COPYOUT_OF(PwrDemandObj);
    EXPORT_COPYOUT_OF(PwrSupplyObj);
    EXPORT_COPYOUT_OF(ContactorObj);
    EXPORT_COPYOUT_OF(get_PwrnodeInfo_ExportPDU);
    EXPORT_COPYOUT_OF(get_PwrplugInfo_ExportPDU);
    EXPORT_COPYOUT_OF(get_ContactorInfo_ExportPDU);
    EXPORT_COPYOUT_OF(trace_Alarm);
}

void rtt_cli_task(void);
int main(void)
{
    register_to_pdu();
    rtt_init();
    pseudotopos();

    PDU_STA state = PDU_STA_UNKNOWN;
    for (state = Main_of_PDU(PDU_CMD_INITIAT); PDU_STA_WORKING == state; state = Main_of_PDU(PDU_CMD_WORKON))
    {
        rtt_cli_task();
        if (0)
        {
            state = Main_of_PDU(PDU_CMD_STANDBY);
        }
        if (0)
        {
            state = Main_of_PDU(PDU_CMD_VISUAL);
        }
    }
}