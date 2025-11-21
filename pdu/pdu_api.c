
#include "pdu_broker.h"
#define __PDU_CORE_EXPORT__
#include "interware.h"
#define __OUTCOMES_GLBVAR__
#include "pdu_tactic.h"
void outcomes_Collate_Allocation(ID_TYPE node_id, ID_TYPE contactor_on_id, ID_TYPE contactor_off_id)
{
    static NODE_OPRATION_CTRL outcome IN_PDU_RAM_SECTION = {.running_worx = NULL, .sheduled_worx = NULL};
    if (node_id > NODE_MAX || contactor_on_id > CONTACTOR_MAX || contactor_off_id > CONTACTOR_MAX)
    {
        return;
    }

    if (0 == node_id && 0 == contactor_on_id && 0 == contactor_off_id)
    {
        NODE_OPRATION *current = g_sheduled_worx;
        while (current != NULL)
        {
            current->node_id = 0;
            current->contactor_on_id = 0;
            current->contactor_off_id = 0;
            current = current->next;
        }
        outcome.running_worx = g_sheduled_worx;
        outcome.sheduled_worx = g_sheduled_worx;
        return;
    }
    if (0 < node_id && 0 < contactor_on_id && 0 < contactor_off_id)
    {
        outcome.running_worx = (NULL == outcome.running_worx->next ? outcome.running_worx : outcome.running_worx->next);
        return;
    }

    if (0 < node_id)
    {
        outcome.running_worx->node_id = node_id;
    }

    if (0 < contactor_on_id)
    {
        outcome.running_worx->contactor_on_id = contactor_on_id;
    }

    if (0 < contactor_off_id)
    {
        outcome.running_worx->contactor_off_id = contactor_off_id;
    }
}
static union PCU_RawData retriver_PCU_RawData_of_node(ID_TYPE id, PCURawData data_type)
{
    if (id > NODE_MAX)
    {
        return (union PCU_RawData){.int32_data = 0};
    }
    switch (data_type)
    {
    case PCUDATA_OF_NODE_WORKHOURS:
    {
        uint32_t workhours = proper_get_PwrnodeInfo_ExportPDU(id)->work_time; // Placeholder for actual workhours retrieval logic
        return (union PCU_RawData){.int32_data = workhours};
        break;
    }
    default:
        return (union PCU_RawData){.int32_data = 0};
        break;
    }
}
static union PCU_RawData retriver_PCU_RawData_of_contactors(ID_TYPE id, PCURawData data_type)
{
    if (id > CONTACTOR_MAX)
    {
        return (union PCU_RawData){.int32_data = 0};
    }

    return (union PCU_RawData){.int32_data = 0};
}
static union PCU_RawData retriver_PCU_RawData_of_plug(ID_TYPE id, PCURawData data_type)
{
    if (id > PLUG_MAX)
    {
        return (union PCU_RawData){.int32_data = 0};
    }
    switch (data_type)
    {
    case PCUDATA_OF_PLUG_MAXIMUM_CURRENT:
    {
        return (union PCU_RawData){.float_data = proper_get_PwrplugInfo_ExportPDU(id)->max_req};
        break;
    }
    case PCUDATA_OF_PLUG_DEMAND_CURRENT:
    {
        float fval = proper_get_PwrplugInfo_ExportPDU(id)->current_req;
        if (isnan(fval) || isinf(fval))
        {
            return (union PCU_RawData){.float_data = 0};
        }
        else
        {
            return (union PCU_RawData){.float_data = fval};
        }
        break;
    }
    case PCUDATA_OF_PLUG_STATUS:
    {
        return (union PCU_RawData){.uint32_data = proper_get_PwrplugInfo_ExportPDU(id)->status};
        break;
    }
    case PCUDATA_OF_PLUG_PRIORITY:
    {
        return (union PCU_RawData){.uint32_data = proper_get_PwrplugInfo_ExportPDU(id)->priority};
        break;
    }
    case PCUDATA_OF_PLUG_VOLTAGE:
    {
        return (union PCU_RawData){.float_data = proper_get_PwrplugInfo_ExportPDU(id)->voltage_req};
        break;
    }
    default:
        return (union PCU_RawData){.int32_data = 0};
        break;
    }
    return (union PCU_RawData){.int32_data = 0};
}
union PCU_RawData retriver_PCU_RawData(ID_TYPE id, PCURawData data_type)
{

    if (((uint32_t)data_type & (uint32_t)PCUDATA_OF_NODE) > 0)
    {
        return retriver_PCU_RawData_of_node(id, data_type);
    }
    else if (((uint32_t)data_type & (uint32_t)PCUDATA_OF_CONTACTORS) > 0)
    {
        return retriver_PCU_RawData_of_contactors(id, data_type);
    }
    else if (((uint32_t)data_type & (uint32_t)PCUDATA_OF_PLUG) > 0)
    {
        return retriver_PCU_RawData_of_plug(id, data_type);
    }
    else
    {
        return (union PCU_RawData){.int32_data = 0};
    }
}

ID_TYPE leftmost_Contactor_IN_Pool(ID_TYPE id_contactor)
{
    if (0u == id_contactor)
    {
        return 0;
    }
    ID_TYPE node_id = NODE_OF_CONTACTOR(id_contactor);

    while (NODE_OF_CONTACTOR(id_contactor - 1) == node_id)
    {
        id_contactor--;
    }
    return id_contactor;
}
void order_Node(struct Alloc_nodeObj *node)
{

    node->value_Iset = 100.0f;
    node->value_Vset = 400.0f;
    draw_pwrnode(ID_OF(node), true);
}

void order_Plug(struct Alloc_plugObj *plug)
{

    const ListObj *header = &plug->copula;
    uint32_t length = list_len_safe(header, CONTACTOR_MAX);
    length = length / NODES_PER_POOL + (uint32_t)!!(bool)(length % NODES_PER_POOL);

    VLA_INSTANT(size_t, thisplug_pos_array, length);

    ListObj *pos;

#define ADD_TO_ARRAY(new_id, array, cnt, len) \
    do                                        \
    {                                         \
        uint32_t _added = 0;                  \
        for (int i = 0; i < (cnt); i++)       \
        {                                     \
            if ((array)[i] == (new_id))       \
            {                                 \
                _added = 1;                   \
                break;                        \
            }                                 \
        }                                     \
        if (!(bool)_added && (cnt) < (len))   \
        {                                     \
            (array)[cnt] = (new_id);          \
            (cnt)++;                          \
        }                                     \
    } while (0)

    int index = 0;
    list_for_each(pos, header)
    {
        ID_TYPE matrix = (POOL_OF_CONTACTOR(ID_OF(pos)) - 1);
        ID_TYPE plug_pos = (ID_TYPE)(matrix * CONTACTORS_PER_NODE + (ID_OF(pos) - 1) % CONTACTORS_PER_NODE + 1);

        ADD_TO_ARRAY(plug_pos, thisplug_pos_array, index, length);
    }
    for (index = 0; index < length; index++)
    {
        draw_plug(thisplug_pos_array[index], true);
    }
#undef ADD_TO_ARRAY
}

void order_Contactor(ListObj *contactor, bool to_turnon)
{
    ID_TYPE current_id = ID_OF(contactor);
    struct Alloc_nodeObj *node_ref = NODEREF_FROM_CONTACTOR(current_id);
    if (to_turnon)
    {
        contactor->ia_contacted = true;
        outcomes_Collate_Allocation(node_ref->id, current_id, 0);
        draw_contactor(current_id, true);
        return;
    }
    // 检查同一个节点其他接触器是否已经闭合
    ID_TYPE leftmost_contactor_id = leftmost_Contactor_IN_Pool(current_id);
    for (ID_TYPE id = leftmost_contactor_id; id < (CONTACTORS_PER_NODE + leftmost_contactor_id); id++)
    {
        if (CONTACTOR_REF(id)->ia_contacted == true)
        {
            CONTACTOR_REF(id)->ia_contacted = false;
            outcomes_Collate_Allocation(node_ref->id, 0, id);
            draw_contactor(id, true);
            // !!!Continue processing rather than returning immediately
        }
    }
}

void deorder_Contactor(ListObj *contactor)
{
    contactor->ia_contacted = false;

    outcomes_Collate_Allocation(0, 0, ID_OF(contactor));
    draw_contactor(ID_OF(contactor), true);
}
