
#include "stm32f4xx.h"
#include "SEGGER_RTT.h"
#include "pdu_param.h"
#include "interware.h"
#include "pdu_main.h"

PowerDemand PwrDemandObj[21] = {0};
PowerSupply PwrSupplyObj[24] = {0};
Contactor ContactorObj[144] = {0};

KeyValue_Array *gTopoConfigParam = NULL;

void pseudotopos(void)
{
    KeyValue_Array *param_ref = NULL;
    param_ref=param_ref;

#include "pseudotopos.h"
    return;
}

void set_Pwrnode_Output(uint8_t contactor_id, float voltage, float current)
{
    printf("set_Pwrnode_Output: contactor_id=%d, voltage=%f, current=%f\r\n", contactor_id, voltage, current);
}
void trace_Alarm(uint8_t node_id, uint8_t alarm_level, uint8_t alarm_status)
{
    printf("trace_Alarm: node_id=%d, alarm_level=%d, alarm_status=%d\r\n", node_id, alarm_level, alarm_status);
}

PDU_STA Main_of_PDU(PDU_CMD cmd);
void register_to_pdu(void)
{
    EXPORT_COPYOUT_OF(set_Pwrnode_Output);
    EXPORT_COPYOUT_OF(PwrDemandObj);
    EXPORT_COPYOUT_OF(PwrSupplyObj);
    EXPORT_COPYOUT_OF(ContactorObj);
    EXPORT_COPYOUT_OF(trace_Alarm);
}
int main(void)
{
    register_to_pdu();
    SEGGER_RTT_ConfigUpBuffer(0, NULL, NULL, 0, SEGGER_RTT_MODE_NO_BLOCK_TRIM);
    pseudotopos();
    PDU_STA state = PDU_STA_UNKNOWN;
    for (state = Main_of_PDU(PDU_CMD_INITIAT); PDU_STA_WORKING == state; state = Main_of_PDU(PDU_CMD_WORKON))
    {
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