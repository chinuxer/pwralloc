#include "pdu_broker.h"
#include "pdu_tactic.h"
#define __PDU_CORE_EXPORT__
#include "interware.h"

void set_allocation_criterion(CRITERION criterion)
{
    gpPlugsArray->criterion = criterion;
}
CRITERION get_allocation_criterion(void)
{
    return gpPlugsArray->criterion;
}
static void order_Node(struct Alloc_nodeObj *node)
{
    node->energon = true;
    node->value_Iset = 100.0f;
    node->value_Vset = 400.0f;
    draw_pwrnode(node->id, true);
}

static void order_Plug(struct Alloc_plugObj *plug)
{
    plug->energon = true;
    const ListObj *header = &plug->koinon;
    int length = list_len_safe(header, CONTACTOR_MAX);
    length = length / NODES_PER_POOL + !!(length % NODES_PER_POOL);
    size_t showid[length];

    ListObj *pos;
#define showid_for_each() for (index = 0; index < length; index++)

#define ADD_TO_SHOWID(new_id)            \
    do                                   \
    {                                    \
        int _added = 0;                  \
        for (int i = 0; i < index; i++)  \
        {                                \
            if ((showid)[i] == (new_id)) \
            {                            \
                _added = 1;              \
                break;                   \
            }                            \
        }                                \
        if (!_added && index < (length)) \
        {                                \
            (showid)[index] = (new_id);  \
            index++;                     \
        }                                \
    } while (0)
    int index = 0;
    showid_for_each()
    {
        showid[index] = 0;
    }

    index = 0;
    list_for_each(pos, header)
    {
        int matrix = POOL_OF_CONTACTOR(ID_OF(pos)) - 1;
        int plug_pos = matrix * CONTACTORS_PER_NODE + (ID_OF(pos) - 1) % CONTACTORS_PER_NODE + 1;

        ADD_TO_SHOWID(plug_pos);
    }
    showid_for_each()
    {
        draw_plug(showid[index], true);
    }
}

static void order_Contactor(ListObj *contactor)
{
    float iset = NODEREF_FROM_CONTACTOR(ID_OF(contactor))->value_Iset;
    float vset = NODEREF_FROM_CONTACTOR(ID_OF(contactor))->value_Vset;
    proper_set_Pwrnode_Output(ID_OF(contactor), vset, iset);
    contactor->is_contactee = true;
    draw_contactor(ID_OF(contactor), true);
}

static void order_Charger_Supply(uint32_t plug_id)
{
    struct Alloc_plugObj *chargee = PLUG_REF(plug_id);

    if (chargee->energon || ENDING != chargee->next_charger)
    {
        return;
    }
    ListObj *pos;
    list_for_each(pos, PLUG_LINKER_REF(plug_id))
    {
        if (false == NODEREF_FROM_CONTACTOR(ID_OF(pos))->energon)
        {
            order_Node(NODEREF_FROM_CONTACTOR(ID_OF(pos)));
            order_Contactor(CONTACTOR_REF(ID_OF(pos)));
            order_Plug(chargee);
            chargee->next_charger = NODE_REF(ID_OF(pos));
            gear_insert(chargee, NODE_REF(ID_OF(pos)));
            break;
        }
    }
    chargee->energon = true;
}

static uint32_t discover_Chargee_Demand_Routine(void)
{
    PlugInfoPtr plug_info = NULL;
    for (uint8_t plug_id = 1; plug_id <= PLUG_MAX; plug_id++)
    {
        plug_info = proper_get_PwrplugInfo_ExportPDU(plug_id);
        if (!plug_info)
        {
            continue;
        }
        if (false == PLUG_REF(plug_id)->energon && plug_info->status == PLUGIN_PLUGGED)
        {
            PLUG_REF(plug_id)->demand = plug_info->power_req;
            return plug_id;
        }
    }
    return 0;
}

void grading_Charger_Linked(struct Alloc_nodeObj *node, OPTIMAL *aristo, CRITERION criteria)
{
    switch (criteria)
    {
    case CRITERION_MIN_COST:

        break;
    case CRITERION_MAX_POWER:
        break;
    case CRITERION_EQU_SANITY:
        break;
    case CRITERION_MAX_EFFICIENCY:
        break;
    case CRITERION_PRIOR:
        break;
    case CRITERION_LMT_CAPACITY:
        break;
    default:
        break;
    }
}
static void checkout_Charger_Supply(uint32_t *plug_id)
{
    *plug_id = 0;
}
void allocative_Routine(void)
{
    static uint32_t plug_id = 0;
    if (!plug_id)
    {
        if (plug_id = discover_Chargee_Demand_Routine())
        {
            order_Charger_Supply(plug_id);
        }
        return;
    }

    checkout_Charger_Supply(&plug_id);
}