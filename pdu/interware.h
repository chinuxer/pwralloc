#pragma once
#include "pdu_ex_datatype.h"

typedef PowerDemand PlugInfoInst, *mirrorptr_PwrDemandObj;
typedef PowerSupply NodeInfoInst, *mirrorptr_PwrSupplyObj;
typedef Contactor ContactorinfoInst, *mirrorptr_ContactorObj;
typedef mirrorptr_PwrSupplyObj (*mirrorptr_get_PwrnodeInfo_ExportPDU)(uint8_t);
typedef mirrorptr_PwrDemandObj (*mirrorptr_get_PwrplugInfo_ExportPDU)(uint8_t);
typedef mirrorptr_ContactorObj (*mirrorptr_get_ContactorInfo_ExportPDU)(uint8_t);
typedef void (*mirrorptr_set_Pwrnode_Output)(uint8_t, float, float);
typedef void (*mirrorptr_trace_Alarm)(uint8_t, uint8_t, uint8_t);

#define EXPORT_LIST                              \
    EXSYMBOL_XMACRO(set_Pwrnode_Output)          \
    EXSYMBOL_XMACRO(get_PwrnodeInfo_ExportPDU)   \
    EXSYMBOL_XMACRO(get_PwrplugInfo_ExportPDU)   \
    EXSYMBOL_XMACRO(get_ContactorInfo_ExportPDU) \
    EXSYMBOL_XMACRO(PwrDemandObj)                \
    EXSYMBOL_XMACRO(PwrSupplyObj)                \
    EXSYMBOL_XMACRO(ContactorObj)                \
    EXSYMBOL_XMACRO(trace_Alarm)
typedef union
{
#define UNION_MEMBER(x) comb_##x  
#define EXSYMBOL_XMACRO(x) mirrorptr_##x UNION_MEMBER(x);
    EXPORT_LIST
#undef EXSYMBOL_XMACRO
} SYMBOL_TYPE;


#ifdef __PDU_CORE_EXPORT__
#define REGISTERED_NAME(x) proper_##x
#define EXSYMBOL_XMACRO(x) static mirrorptr_##x REGISTERED_NAME(x) = NULL;

EXPORT_LIST
#undef EXSYMBOL_XMACRO


void register_external_symbol(const char *name, const SYMBOL_TYPE symbol)
{
    //cstat -DEFINE-hash-multiple
#define EXSYMBOL_XMACRO(ext)                    \
    if (strncmp(name, #ext, sizeof(#ext)) == 0) \
    {                                           \
        REGISTERED_NAME(ext) = (symbol.UNION_MEMBER(ext));     \
        return;                                 \
    }

    //cstat +DEFINE-hash-multiple
    EXPORT_LIST
    return;
#undef EXSYMBOL_XMACRO
}

bool chk_registered_symbol(void)
{
#define EXSYMBOL_XMACRO(nym)  \
    if (NULL == REGISTERED_NAME(nym)) \
    {                         \
        return false;         \
    }

    EXPORT_LIST

    return true;
#undef EXSYMBOL_XMACRO
}
#else

void register_external_symbol(const char *name, const SYMBOL_TYPE symbol);

#define EXPORT_COPYOUT_OF(symbol, variant)          \
    do                                              \
    {                                               \
        symbol.comb_##variant = variant;            \
        register_external_symbol(#variant, symbol); \
    } while (0)

#endif