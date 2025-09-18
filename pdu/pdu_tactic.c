#include "pdu_broker.h"
#include "pdu_tactic.h"
#define __PDU_CORE_EXPORT__
#include "interware.h"
static void subscribe_Node(struct Alloc_nodeObj *node)
{
    node->energon = true;
    node->value_Iset = 100.0f;
    node->value_Vset = 400.0f;
    draw_pwrnode(node->id);
}

static void subscribe_Plug(struct Alloc_plugObj *plug)
{
    plug->energon = true;
    draw_plug(plug->id);
}

static void subscribe_Contactor(ListObj *contactor)
{
    float iset = NODEREF_FROM_CONTACTOR(ID_OF(contactor))->value_Iset;
    float vset = NODEREF_FROM_CONTACTOR(ID_OF(contactor))->value_Vset;
    set_Pwrnode_Output(ID_OF(contactor), iset, vset);
    contactor->is_contactee = true;
    draw_contactor(ID_OF(contactor));
}

static void allocate_Charger_Supply_Irregular(uint32_t plug_id)
{
    struct Alloc_plugObj *chargee = PLUG_REF(plug_id);

    if (chargee->energon || ENDING != chargee->next_charger)
    {
        return;
    }
    ListObj *pos;
    list_for_each(pos, PLUG_LINKER_REF(plug_id))
    {
        if (false == NODE_REF(ID_OF(pos))->energon)
        {
            subscribe_Node(NODEREF_FROM_CONTACTOR(ID_OF(pos)));
            subscribe_Contactor(CONTACTOR_REF(ID_OF(pos)));
            subscribe_Plug(chargee);
        }
    }
    chargee->energon = true;
}

static uint32_t discover_Chargee_Demand_Routine(void)
{
    PlugInfoPtr plug_info = NULL;
    for (uint8_t plug_id = 1; plug_id < PLUG_MAX; plug_id)
    {
        plug_info = get_PwrplugInfo_ExportPDU(plug_id);

        if (false == PLUG_REF(plug_id)->energon && plug_info->status == PLUGIN_PLUGGED)
        {
            PLUG_REF(plug_id)->demand = plug_info->power_req;
            print_oneliner("pug#%d", plug_id);
            return plug_id;
        }
    }
    return 0;
}
void allocative_Routine(void)
{
    uint32_t plug_id = 0;
    if (plug_id = discover_Chargee_Demand_Routine())
    {
        allocate_Charger_Supply_Irregular(plug_id);
    }
}