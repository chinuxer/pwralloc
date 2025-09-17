
#ifdef __PDU_CORE_EXPORT__

typedef PowerDemand PlugInfoInst, *PlugInfoPtr;
typedef PowerSupply NodeInfoInst, *NodeInfoPtr;
typedef Contactor ContactorinfoInst, *ContactorinfoPtr;
typedef void (*func_set_pwrnode)(uint8_t, float, float);
typedef void (*func_trace_alarm)(uint8_t, uint8_t, uint8_t);
PlugInfoPtr __attribute__((weak)) PwrDemandObj;
NodeInfoPtr __attribute__((weak)) PwrSupplyObj;
ContactorinfoPtr __attribute__((weak)) ContactorObj;
func_set_pwrnode __attribute__((weak)) set_Pwrnode_Output;
func_trace_alarm __attribute__((weak)) trace_Alarm;
PlugInfoPtr
get_PlugInfo(uint32_t plug_id)
{
    return PwrDemandObj + plug_id;
}

NodeInfoPtr get_NodeInfo(uint32_t node_id)
{
    return PwrSupplyObj + node_id;
}

ContactorinfoPtr get_ContactInfo(uint32_t contactor_id)
{
    return ContactorObj + contactor_id;
}

void register_external_symbol(const char *name, void *symbol)
{
#define HANDLE_SYMBOL(type, ext)                \
    if (strncmp(name, #ext, sizeof(#ext)) == 0) \
    {                                           \
        ext = (type)symbol;                     \
        return;                                 \
    }

    HANDLE_SYMBOL(PlugInfoPtr, PwrDemandObj);
    HANDLE_SYMBOL(NodeInfoPtr, PwrSupplyObj);
    HANDLE_SYMBOL(ContactorinfoPtr, ContactorObj);
    HANDLE_SYMBOL(func_set_pwrnode, set_Pwrnode_Output);
    HANDLE_SYMBOL(func_trace_alarm, trace_Alarm);
    
    return;
}
#else
void register_external_symbol(const char *name, void *symbol);
#define EXPORT_COPYOUT_OF(variant)                            \
    do                                                        \
    {                                                         \
        register_external_symbol(#variant, (void *)&variant); \
    } while (0)

#endif