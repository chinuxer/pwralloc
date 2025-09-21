#include "pdu_ex_datatype.h"
#ifdef __PDU_CORE_EXPORT__

typedef PowerDemand PlugInfoInst, *PlugInfoPtr;
typedef PowerSupply NodeInfoInst, *NodeInfoPtr;
typedef Contactor ContactorinfoInst, *ContactorinfoPtr;
typedef NodeInfoPtr (*func_subscribe_pwrnode)(uint8_t);
typedef PlugInfoPtr (*func_subscribe_pwrplug)(uint8_t);
typedef ContactorinfoPtr (*func_subscribe_contactor)(uint8_t);
typedef void (*func_set_pwrnode)(uint8_t, float, float);
typedef void (*func_trace_alarm)(uint8_t, uint8_t, uint8_t);
PlugInfoPtr __attribute__((weak)) proper_PwrDemandObj;
NodeInfoPtr __attribute__((weak)) proper_PwrSupplyObj;
ContactorinfoPtr __attribute__((weak)) proper_ContactorObj;
func_subscribe_pwrnode __attribute__((weak)) proper_get_PwrnodeInfo_ExportPDU;
func_subscribe_pwrplug __attribute__((weak)) proper_get_PwrplugInfo_ExportPDU;
func_subscribe_contactor __attribute__((weak)) proper_get_ContactorInfo_ExportPDU;
func_set_pwrnode __attribute__((weak)) proper_set_Pwrnode_Output;
func_trace_alarm __attribute__((weak)) proper_trace_Alarm;

void register_external_symbol(const char *name, void *symbol)
{
#define HANDLE_SYMBOL(type, ext)                \
    if (strncmp(name, #ext, sizeof(#ext)) == 0) \
    {                                           \
        proper_##ext = (type)symbol;            \
        return;                                 \
    }

    HANDLE_SYMBOL(PlugInfoPtr, PwrDemandObj);
    HANDLE_SYMBOL(NodeInfoPtr, PwrSupplyObj);
    HANDLE_SYMBOL(ContactorinfoPtr, ContactorObj);
    HANDLE_SYMBOL(func_set_pwrnode, set_Pwrnode_Output);
    HANDLE_SYMBOL(func_subscribe_contactor, get_ContactorInfo_ExportPDU);
    HANDLE_SYMBOL(func_subscribe_pwrnode, get_PwrnodeInfo_ExportPDU);
    HANDLE_SYMBOL(func_subscribe_pwrplug, get_PwrplugInfo_ExportPDU);
    HANDLE_SYMBOL(func_trace_alarm, trace_Alarm);

    return;
}

bool chk_registered_symbol(void)
{
#define CHECK_SYMBOL(nym)     \
    if (NULL == proper_##nym) \
    {                         \
        return false;         \
    }

    CHECK_SYMBOL(set_Pwrnode_Output)
    CHECK_SYMBOL(get_PwrnodeInfo_ExportPDU)
    CHECK_SYMBOL(get_PwrplugInfo_ExportPDU)
    CHECK_SYMBOL(get_ContactorInfo_ExportPDU)
    CHECK_SYMBOL(PwrDemandObj)
    CHECK_SYMBOL(PwrSupplyObj)
    CHECK_SYMBOL(ContactorObj)
    CHECK_SYMBOL(trace_Alarm)

    return true;
}
#else
void register_external_symbol(const char *name, void *symbol);

#define EXPORT_COPYOUT_OF(variant)                            \
    do                                                        \
    {                                                         \
        register_external_symbol(#variant, (void *)&variant); \
    } while (0)

#endif