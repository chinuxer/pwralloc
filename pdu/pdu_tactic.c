#include "pdu_broker.h"
#include "pdu_tactic.h"
#include "pdu_ex_datatype.h"
#include <stdint.h>
#define __PDU_CORE_EXPORT__
#include "interware.h"

union PCU_RawData retriver_PCU_RawData_of_node(uint32_t id, PCURawData data_type)
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
    }
    default:
        return (union PCU_RawData){.int32_data = 0};
    }
}
union PCU_RawData retriver_PCU_RawData_of_contactors(uint32_t id, PCURawData data_type)
{
    if (id > CONTACTOR_MAX)
    {
        return (union PCU_RawData){.int32_data = 0};
    }
    return (union PCU_RawData){.int32_data = 0};
}
union PCU_RawData retriver_PCU_RawData_of_plug(uint32_t id, PCURawData data_type)
{
    if (id > PLUG_MAX)
    {
        return (union PCU_RawData){.int32_data = 0};
    }
    return (union PCU_RawData){.int32_data = 0};
}
union PCU_RawData retriver_PCU_RawData(uint32_t id, PCURawData data_type)
{

    if (data_type & PCUDATA_OF_NODE)
    {
        return retriver_PCU_RawData_of_node(id, data_type);
    }
    else if (data_type & PCUDATA_OF_CONTACTORS)
    {
        return retriver_PCU_RawData_of_contactors(id, data_type);
    }
    else if (data_type & PCUDATA_OF_PLUG)
    {
        return retriver_PCU_RawData_of_plug(id, data_type);
    }
    else
    {
        return (union PCU_RawData){.int32_data = 0};
    }
}

#define QUOTA_FLUCT 10.0f
#define MAXIM_PWR_OF_NODE 40.0f
#define MINIMUM_QUOTA 1
#define PRIORITY_ASSERT(x) ((gpPlugsArray->criteria & (x)) > 0)
#define GET_PCU_RAWDATA(id, data, type) (retriver_PCU_RawData(id, data).type##_data)
static uint32_t seniority_Of_Node(struct Alloc_plugObj *plug, struct Alloc_nodeObj *node, uint32_t id_contactor)
{
#define RANGE_NORMALIZE_LEFT 0
#define RANGE_NORMALIZE_RIGHT RANGE_NORMALIZE_LEFT + 100

    uint32_t workhours;

    if (plug->strategy_info.workhours_topnode <= plug->strategy_info.workhours_bottomnode)
    {
        return (RANGE_NORMALIZE_RIGHT - RANGE_NORMALIZE_LEFT) / 2;
    }
    else
    {
        workhours = GET_PCU_RAWDATA(ID_OF(node), PCUDATA_OF_NODE_WORKHOURS, int32);
        workhours = workhours > 0 ? workhours : 0;
        workhours = (workhours * (RANGE_NORMALIZE_RIGHT - RANGE_NORMALIZE_LEFT) / (plug->strategy_info.workhours_topnode - plug->strategy_info.workhours_bottomnode));
        workhours = RANGE_NORMALIZE_RIGHT - RANGE_NORMALIZE_LEFT - workhours;
        return workhours % (100 + 1);
    }
}

static uint32_t seniority_Of_Contactor(struct Alloc_plugObj *plug, struct Alloc_nodeObj *node, uint32_t id_contactor)
{
    return 0;
}

static uint32_t holding_Of_PlugChargedBy_Node(struct Alloc_plugObj *plug, struct Alloc_nodeObj *node, uint32_t id_contactor)
{
    struct Alloc_plugObj *plug_of_node = PLUG_REF(node->chargingplug_Id);
    uint8_t num = gear_num(plug_of_node);
    return num > (WEIGHT_HIERARCHY - 1) ? (WEIGHT_HIERARCHY - 1) : num;
}
static uint32_t usageratio_Of_Pool(struct Alloc_plugObj *plug, struct Alloc_nodeObj *node, uint32_t id_contactor)
{
    uint32_t nodes_worked = POOL_REF(POOL_OF_NODE(ID_OF(node)))->nodes_worked;
    nodes_worked = (NODES_PER_POOL - nodes_worked) * (WEIGHT_HIERARCHY - 1) / NODES_PER_POOL;
    return nodes_worked;
}

bool shareable_By_Priority(PRIOR req_priority, PRIOR target_priority)
{
    return ((req_priority == target_priority) || (PRIOR_VIP == req_priority && PRIOR_BASE == target_priority));
}
bool deprivable_By_Priority(PRIOR req_priority, PRIOR target_priority)
{
#define UNOCCUPIED 2 // 空闲
#define DEPRIVABLE 1 // 占用但可剥夺
#define IMPARTICIP 0 // 不可剥夺
    bool bPreempt = false;
    switch (req_priority)
    {
    case PRIOR_EXTREME: // 最高优先级：可抢占任何优先级
        bPreempt = (target_priority == PRIOR_SVIP || target_priority == PRIOR_VIP || target_priority == PRIOR_BASE);
        break;
    case PRIOR_SVIP: // 可抢占优先级PRIOR_BASE
        bPreempt = (target_priority == PRIOR_BASE);
        break;
    case PRIOR_VIP:  // 不可抢占PRIOR_BASE优先级,只可分摊
                     // fallthru
    case PRIOR_BASE: // 该优先级不能抢占其他优先级
                     // fallthru
    default:
        bPreempt = false;
    }
    return bPreempt;
}

static uint32_t peer_Competitor(struct Alloc_plugObj *plug)
{
    ListObj *pos;
    uint32_t quantum = 0;
    VLA_INSTANT(uint32_t, chargingplug_array, CONTACTORS_PER_NODE);
    list_for_each(pos, &plug->copula)
    {
        uint32_t chargingplug_id = NODEREF_FROM_CONTACTOR(ID_OF(pos))->chargingplug_Id;
        uint32_t node_id = NODEREF_FROM_CONTACTOR(ID_OF(pos))->id;
        if (chargingplug_id == ID_OF(plug))
        {
            continue;
        }
        ListObj *contactor = get_Cross_BtwnPlugNode(node_id, chargingplug_id);
        if (contactor && !contactor->ia_contacted)
        {
            continue;
        }
        if (!shareable_By_Priority(plug->strategy_info.priority, PLUG_REF(chargingplug_id)->strategy_info.priority))
        {
            continue;
        }
        for (int i = 0; i < CONTACTORS_PER_NODE; i++)
        {
            if (chargingplug_array[i] == chargingplug_id)
            {
                break;
            }
            if (chargingplug_array[i] == 0)
            {
                chargingplug_array[i] = chargingplug_id;
                quantum += 1;
                break;
            }
        }
    }
    return quantum;
}
static uint32_t peer_Resource(struct Alloc_plugObj *plug)
{
    ListObj *pos;
    uint32_t quantum = 0;
    PRIOR req_priority = PRIOR_VAIN;
    PRIOR target_priority = PRIOR_VAIN;
    list_for_each(pos, &plug->copula)
    {
        req_priority = plug->strategy_info.priority;
        target_priority = NODEREF_FROM_CONTACTOR(ID_OF(pos))->priority;
        if (shareable_By_Priority(req_priority, target_priority))
        {
            quantum += 1;
        }
    }
    return quantum;
}

static uint32_t usage_Of_Node(struct Alloc_plugObj *plug, struct Alloc_nodeObj *node, uint32_t id_contactor)
{

    if (!plug || !node)
    {
        return 0;
    }

    if (0 == node->chargingplug_Id && node->priority == PRIOR_VAIN) // 节点空闲
    {
        return UNOCCUPIED;
    }
    if (node->chargingplug_Id == ID_OF(plug)) // 节点被当前充电枪占用
    {
        return IMPARTICIP;
    }
    struct Alloc_plugObj *plug_released = PLUG_REF(node->chargingplug_Id);
    if (gear_num(plug_released) <= MINIMUM_QUOTA) // 如果当前节点被占用且充电枪资源低于最低配额
    {
        return IMPARTICIP;
    }
    bool bPreempt = false;
    PRIOR req_priority = plug->strategy_info.priority;
    PRIOR target_priority = node->priority;

    bPreempt = deprivable_By_Priority(req_priority, target_priority); // 判断是否可剥夺

    return (bPreempt ? DEPRIVABLE : IMPARTICIP);
}
static uint32_t overload_Of_CopperBar(struct Alloc_plugObj *plug, struct Alloc_nodeObj *node, uint32_t id_contactor)
{
    return 0;
}
RATING_RANK SCOREDEED_ARRAY[] = {
    [0] = {WEIGHT_CONTACTOR_ACT, seniority_Of_Contactor, CRITERION_EQU_SANITY},
    [1] = {WEIGHT_WORK_TIME, seniority_Of_Node, CRITERION_EQU_SANITY},
    [2] = {WEIGHT_POOL_OCCUPIED, usageratio_Of_Pool, CRITERION_EQU_SANITY},
    [3] = {WEIGHT_HOLDING, holding_Of_PlugChargedBy_Node, CRITERION_PRIOR},
    [4] = {WEIGHT_DEPRIV, usage_Of_Node, CRITERION_PRIOR},
    [5] = {WEIGHT_COPBAR, overload_Of_CopperBar, CRITERION_LMT_CAPACITY},
    [6] = {WEIGHT_BLANK, NULL, CRITERION_MIN_COST},
    [7] = {WEIGHT_BLANK, NULL, CRITERION_MIN_COST},
    [8] = {WEIGHT_BLANK, NULL, CRITERION_MAX_POWER},
    [9] = {WEIGHT_BLANK, NULL, CRITERION_MAX_EFFICIENCY},
};

int get_Score_Of_Node(RATING_RANK *tab, struct Alloc_plugObj *plug, uint32_t id_contactor)
{
    struct Alloc_nodeObj *node = NODEREF_FROM_CONTACTOR(id_contactor);
    if (!tab || !node || !plug || !id_contactor)
    {
        return 0;
    }
    int score = 0;
    while (tab && tab->func && PRIORITY_ASSERT(tab->criteria))
    {
        score += tab->func(plug, node, id_contactor) * tab->weight;
        tab++;
    }
    return score;
}

static void score_print(OPTIMAL *optimum, OPTIMAL *candidates, uint32_t credits)
{
    if (credits > 0)
    {
        char buffer[512];
        int offset = 0;

        // Print optimal node info
        offset += snprintf(buffer + offset, sizeof(buffer) - offset, "(%d)",
                           (optimum ? optimum->node_id : 0));

        // Print candidates dynamically
        for (int i = 0; i < credits && i < NODE_MAX && offset < sizeof(buffer) - 32; i++)
        {
            offset += snprintf(buffer + offset, sizeof(buffer) - offset, " %d:%d",
                               candidates[i].node_id, candidates[i].score);
        }

        print_oneliner("%s", buffer);
    }
}
void set_allocation_criterion(CRITERION criterion)
{
    gpPlugsArray->criteria |= criterion;
}

static void order_Node(struct Alloc_nodeObj *node)
{

    node->value_Iset = 100.0f;
    node->value_Vset = 400.0f;

    draw_pwrnode(ID_OF(node), true);
}

static void order_Plug(struct Alloc_plugObj *plug)
{

    const ListObj *header = &plug->copula;
    int length = list_len_safe(header, CONTACTOR_MAX);
    length = length / NODES_PER_POOL + !!(length % NODES_PER_POOL);

    VLA_INSTANT(size_t, thisplug_pos_array, length);

    ListObj *pos;

#define ADD_TO_ARRAY(new_id, array, cnt, len) \
    do                                        \
    {                                         \
        int _added = 0;                       \
        for (int i = 0; i < cnt; i++)         \
        {                                     \
            if ((array)[i] == (new_id))       \
            {                                 \
                _added = 1;                   \
                break;                        \
            }                                 \
        }                                     \
        if (!_added && cnt < (len))           \
        {                                     \
            (array)[cnt] = (new_id);          \
            cnt++;                            \
        }                                     \
    } while (0)

    int index = 0;
    list_for_each(pos, header)
    {
        int matrix = POOL_OF_CONTACTOR(ID_OF(pos)) - 1;
        int plug_pos = matrix * CONTACTORS_PER_NODE + (ID_OF(pos) - 1) % CONTACTORS_PER_NODE + 1;

        ADD_TO_ARRAY(plug_pos, thisplug_pos_array, index, length);
    }
    for (index = 0; index < length; index++)
    {
        draw_plug(thisplug_pos_array[index], true);
    }
#undef ADD_TO_ARRAY
}

uint32_t leftmost_Contactor_IN_Pool(uint32_t id_contactor)
{
    if (0 == id_contactor)
    {
        return 0;
    }
    uint32_t node_id = NODE_OF_CONTACTOR(id_contactor);

    while (NODE_OF_CONTACTOR(id_contactor - 1) == node_id)
    {
        id_contactor--;
    }
    return id_contactor;
}
static void order_Contactor(ListObj *contactor)
{
    float iset = NODEREF_FROM_CONTACTOR(ID_OF(contactor))->value_Iset;
    float vset = NODEREF_FROM_CONTACTOR(ID_OF(contactor))->value_Vset;
    proper_set_Pwrnode_Output(ID_OF(contactor), vset, iset);

    // 检查同一个节点其他接触器是否已经闭合
    uint32_t leftmost_contactor_id = leftmost_Contactor_IN_Pool(ID_OF(contactor));
    for (uint32_t id = leftmost_contactor_id; id < (CONTACTORS_PER_NODE + leftmost_contactor_id); id++)
    {
        if (CONTACTOR_REF(id)->ia_contacted == true)
        {
            CONTACTOR_REF(id)->ia_contacted = false;

            draw_contactor(id, false);
        }
    }
    contactor->ia_contacted = true;
    draw_contactor(ID_OF(contactor), true);
}

static OPTIMAL *find_highest_score(OPTIMAL *aristo, int count, int blue_line)
{
    if (count <= 0)
        return NULL;

    OPTIMAL *highest = &aristo[0];
    for (int i = 1; i < count; i++)
    {
        if (aristo[i].score > highest->score)
        {
            highest = &aristo[i];
        }
    }

    return (highest->score >= blue_line ? highest : NULL);
}
OPTIMAL *grading_Charger_Linked(const ListObj *last, OPTIMAL *optional, int cnt)
{
    if (!last || !optional)
    {
        return NULL;
    }

    (optional + cnt)->contactor_id = ID_OF(last);
    (optional + cnt)->node_id = NODE_OF_CONTACTOR(ID_OF(last));
    struct Alloc_plugObj *header = get_header_plug(ID_OF(last));
    (optional + cnt)->score = get_Score_Of_Node(SCOREDEED_ARRAY, header, ID_OF(last));
    //(optional + cnt)->score *= (optional + cnt)->foreheld ? -1 : 1;
    return find_highest_score(optional, cnt + 1, PASS_LINE);
}

static uint32_t discover_Chargee_Depart_Routine(void)
{
    PlugInfoPtr plug_info = NULL;
    for (uint8_t plug_id = 1; plug_id <= PLUG_MAX; plug_id++)
    {
        plug_info = proper_get_PwrplugInfo_ExportPDU(plug_id);
        if (!plug_info)
        {
            continue;
        }
        struct Alloc_plugObj *plug = PLUG_REF(plug_id);
        if (PRIOR_VAIN != plug->strategy_info.priority && plug_info->status == PLUGIN_UNPLUGGED)
        {
            plug->strategy_info.demand = 0;
            plug->strategy_info.workhours_bottomnode = 0;
            plug->strategy_info.workhours_topnode = 0;
            plug->strategy_info.priority = PRIOR_VAIN;
            plug->strategy_info.shortage_demand = 0;

            return plug_id;
        }
    }
    return 0;
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
        if (PLUG_REF(plug_id)->strategy_info.demand < 0.0)
        {
            continue;
        }
        if (PRIOR_VAIN == PLUG_REF(plug_id)->strategy_info.priority && plug_info->status == PLUGIN_PLUGGED)
        {
            PLUG_REF(plug_id)->strategy_info.demand = plug_info->power_req;
            PLUG_REF(plug_id)->strategy_info.priority = plug_info->priority;
            return plug_id;
        }
    }
    return 0;
}

static void checkout_Charger_Supply(uint32_t *order_plug_id, uint32_t *deorder_plug_id)
{
    if (!order_plug_id || !deorder_plug_id)
    {
        return;
    }

    *order_plug_id = 0;
    *deorder_plug_id = 0;
}

static inline int quota_Of_Demander(uint32_t plug_id)
{
    float gross = PLUG_REF(plug_id)->strategy_info.demand;
    gross = (gross > 0.0f) ? gross + QUOTA_FLUCT : 0.0f;
    return (int)ceilf(gross / MAXIM_PWR_OF_NODE);
}

static void get_Amphextremum(struct Alloc_plugObj *chargee)
{

    ListObj *pos;
    uint32_t workhours;
    list_for_each(pos, &chargee->copula)
    {
        workhours = GET_PCU_RAWDATA(NODE_OF_CONTACTOR(ID_OF(pos)), PCUDATA_OF_NODE_WORKHOURS, int32);
        if (workhours < chargee->strategy_info.workhours_bottomnode)
        {
            chargee->strategy_info.workhours_bottomnode = workhours;
        }
        if (workhours > chargee->strategy_info.workhours_topnode)
        {
            chargee->strategy_info.workhours_topnode = workhours;
        }
    }
}
static uint32_t count_Charging_Plug(uint32_t pool_id)
{
    if (pool_id > POOL_MAX)
    {
        return 0;
    }
    pool_id -= 1;
    uint64_t count_bitmap = 0;
    for (int node_id = (pool_id * NODES_PER_POOL + 1); node_id < (pool_id * NODES_PER_POOL + 1 + NODES_PER_POOL); node_id++)
    {
        if (NODE_REF(node_id)->chargingplug_Id != 0)
        {
            count_bitmap |= (1 << (NODE_REF(node_id)->chargingplug_Id));
        }
    }
    int count = 0;
    while (count_bitmap)
    {
        count += count_bitmap & 1;
        count_bitmap >>= 1;
    }

    return count;
}
static void ordering_Procedure(const OPTIMAL *optimum)
{
    struct Alloc_plugObj *chargee = PLUG_REF(optimum->plug_id);
    struct Alloc_nodeObj *node = NODE_REF(optimum->node_id);
    if (!optimum || !chargee || !node || 0 == optimum->contactor_id)
    {
        return;
    }

    order_Node(node);
    order_Contactor(CONTACTOR_REF(optimum->contactor_id));
    order_Plug(chargee);

    if (0 < node->chargingplug_Id && PRIOR_VAIN != node->priority)
    {
        struct Alloc_plugObj *plug_released = PLUG_REF(node->chargingplug_Id);

        plug_released->strategy_info.shortage_demand += 1;

        gear_remove(plug_released, node);
    }

    gear_insert(chargee, node);
    POOL_REF(POOL_OF_NODE(optimum->node_id))->nodes_worked += (POOL_REF(POOL_OF_NODE(optimum->node_id))->nodes_worked < NODES_PER_POOL);
    POOL_REF(POOL_OF_NODE(optimum->node_id))->plugs_linked = count_Charging_Plug(POOL_OF_NODE(optimum->node_id));
}

void handle_Demands_From_Charger(struct Alloc_plugObj *chargee, int *pshortage)
{
    uint32_t credits = list_len_safe(&chargee->copula, NODE_MAX);
    if (0 == credits)
    {
        return;
    }
    VLA_INSTANT(OPTIMAL, candidates, credits);
    ListObj *pos;
    OPTIMAL *optimum = NULL;

    int cnt = 0;
    list_for_each(pos, &chargee->copula)
    {
        optimum = grading_Charger_Linked(pos, candidates, cnt++ % credits);
    }

    score_print(optimum, candidates, credits);
    if (NULL == optimum)
    {
        order_Plug(chargee);
        return;
    }
    if ((0 == optimum->node_id) || (0 == optimum->contactor_id))
    {
        return;
    }
    if ((NODE_MAX < optimum->node_id) || (CONTACTOR_MAX < optimum->contactor_id))
    {
        return;
    }

    // print_oneliner("[Tactic] Allocating Node %d to Plug %d with Score %d", optimum.node_id, ID_OF(chargee), optimum.score);
    *pshortage = ((*pshortage - 1) < 0 ? 0 : *pshortage - 1);
    optimum->plug_id = ID_OF(chargee);

    ordering_Procedure(optimum);
}

static void handle_Lowest_Node_Holdsome(struct Alloc_plugObj *chargee, int *pshortage)
{
#define GRADING_BY(shortage, prior, possess) ((int)(POSTPRIMA_WEIGHT1 - 1 - shortage % POSTPRIMA_WEIGHT1 + POSTPRIMA_WEIGHT1 * (PRIOR_EXTREME - prior) + POSTPRIMA_WEIGHT2 * possess))
    if (!chargee || !PRIORITY_ASSERT(CRITERION_ALLOWANCE_MINIMUM))
    {
        return;
    }
    uint32_t resources = list_len_safe(&chargee->copula, NODE_MAX);
    VLA_INSTANT(OPTIMAL, candidates, resources);
    ListObj *pos;
    int cnt = 0;
    list_for_each(pos, &chargee->copula)
    {
        uint32_t plug_id = NODEREF_FROM_CONTACTOR(ID_OF(pos))->chargingplug_Id;
        if (plug_id == 0)
        {
            continue;
        }

        struct Alloc_plugObj *plug_released = PLUG_REF(plug_id);
        candidates[cnt++ % resources] = (OPTIMAL){
            .plug_id = ID_OF(chargee),
            .node_id = NODE_OF_CONTACTOR(ID_OF(pos)),
            .contactor_id = ID_OF(pos),
            .score = GRADING_BY(plug_released->strategy_info.shortage_demand, NODE_REF(ID_OF(pos))->priority, gear_num(plug_released))};
    }

    OPTIMAL *optimum = find_highest_score(candidates, resources, POSTPRIMA_WEIGHT2 * MINIMUM_QUOTA);
    score_print(optimum, candidates, resources);
    if (NULL == optimum || 0 == optimum->node_id)
    {
        return;
    }
    *pshortage = ((*pshortage - 1) < 0 ? 0 : *pshortage - 1);
    ordering_Procedure(optimum);
#undef GRADING_BY
}
void handle_Shortage_From_Peers(struct Alloc_plugObj *chargee, int *pshortage)
{
#define GRADING_BY(shortage, possess, sharable) ((int)(POSTPRIMA_WEIGHT1 - 1 - shortage % POSTPRIMA_WEIGHT1 + POSTPRIMA_WEIGHT1 * possess + POSTPRIMA_WEIGHT2 * (sharable)))
    if (!chargee || !PRIORITY_ASSERT(CRITERION_PEERS_FAIRNOFAVOR))
    {
        return;
    }

    uint32_t peers = peer_Competitor(chargee) + 1;
    uint32_t resources = peer_Resource(chargee);
    if (0 == resources || 0 == peers)
    {
        return;
    }
    uint32_t floor_supply = (resources + 1) / peers;
    floor_supply = (floor_supply > 1) ? floor_supply : 1;
    if (floor_supply <= gear_num(chargee))
    {
        return;
    }
    VLA_INSTANT(OPTIMAL, candidates, resources);
    const ListObj *pos;
    int cnt = 0;
    list_for_each(pos, &chargee->copula)
    {
        uint32_t plug_id = NODEREF_FROM_CONTACTOR(ID_OF(pos))->chargingplug_Id;
        if (plug_id == 0)
        {
            continue;
        }

        struct Alloc_plugObj *plug_released = PLUG_REF(plug_id);
        PRIOR req_priority = chargee->strategy_info.priority;
        PRIOR target_priority = NODEREF_FROM_CONTACTOR(ID_OF(pos))->priority;

        candidates[cnt++ % resources] = (OPTIMAL){
            .contactor_id = ID_OF(pos),
            .node_id = NODE_OF_CONTACTOR(ID_OF(pos)),
            .score = GRADING_BY(plug_released->strategy_info.shortage_demand, gear_num(plug_released), shareable_By_Priority(req_priority, target_priority)),
            .plug_id = ID_OF(chargee)};
    }

    OPTIMAL *optimum = find_highest_score(candidates, resources, (floor_supply + 1) * POSTPRIMA_WEIGHT1 + POSTPRIMA_WEIGHT2);
    score_print(optimum, candidates, resources);
    if (NULL == optimum || 0 == optimum->node_id)
    {
        return;
    }
    *pshortage = ((*pshortage - 1) < 0 ? 0 : *pshortage - 1);
    ordering_Procedure(optimum);
#undef GRADING_BY
}
void meet_Remand_Shortage(struct Alloc_plugObj *chargee)
{
#define GRADING_BY(demand, priority) ((int)(demand + POSTPRIMA_WEIGHT1 * priority))

    if (!chargee)
    {
        return;
    }
    uint32_t list_len = list_len_safe(&chargee->copula, NODE_MAX);

    if (list_len == 0)
    {
        return;
    }
    struct Alloc_plugObj *other_chargee = NULL;
    uint32_t leftmost_id = 0;
    const ListObj *pos;
    VLA_INSTANT(OPTIMAL, candidates, CONTACTORS_PER_NODE);
    list_for_each(pos, &chargee->copula)
    {
        if (pos->ia_contacted == true)
        {
            continue;
        }
        leftmost_id = leftmost_Contactor_IN_Pool(ID_OF(pos));
        if (0 == leftmost_id)
        {
            continue;
        };
        bool occupied = false;

        for (uint32_t id = leftmost_id; id < (leftmost_id + CONTACTORS_PER_NODE); id++)
        {
            other_chargee = get_header_plug(id);
            occupied |= CONTACTOR_REF(id)->ia_contacted;
            if (NULL != other_chargee && 0 < other_chargee->strategy_info.shortage_demand)
            {

                *(candidates + (id - leftmost_id) % CONTACTORS_PER_NODE) = (OPTIMAL){
                    .contactor_id = id,
                    .node_id = NODE_OF_CONTACTOR(id),
                    .plug_id = ID_OF(other_chargee),
                    .score = GRADING_BY(other_chargee->strategy_info.shortage_demand, other_chargee->strategy_info.priority)};
            }
            else
            {
                *(candidates + (id - leftmost_id) % CONTACTORS_PER_NODE) = (OPTIMAL){
                    .contactor_id = 0,
                    .node_id = NODE_OF_CONTACTOR(id),
                    .plug_id = 0,
                    .score = -1};
            }
        }
        if (occupied)
        {
            continue;
        }
        OPTIMAL *optimum = find_highest_score(candidates, CONTACTORS_PER_NODE, 0);
        score_print(optimum, candidates, CONTACTORS_PER_NODE);
        if (NULL == optimum || 0 == optimum->node_id)
        {
            continue;
        }
        other_chargee = PLUG_REF(optimum->plug_id);
        other_chargee->strategy_info.shortage_demand -= !!other_chargee->strategy_info.shortage_demand;
        ordering_Procedure(optimum);
    }
#undef GRADING_BY
}
void recycling_Dearler(struct Alloc_plugObj *chargee)
{
    if (!chargee)
    {
        return;
    }
    order_Plug(chargee);

    uint32_t pool_id = 0;
    struct Alloc_nodeObj *pnode = NULL;

    const ListObj *pos;
    list_for_each(pos, &chargee->copula)
    {
        if (CONTACTOR_REF(ID_OF(pos))->ia_contacted == true)
        {
            CONTACTOR_REF(ID_OF(pos))->ia_contacted = false;
            draw_contactor(ID_OF(pos), true);
        }
        pnode = NODEREF_FROM_CONTACTOR(ID_OF(pos));
        if (pnode->chargingplug_Id == chargee->id)
        {
            pnode->value_Iset = 0;
            pnode->value_Vset = 0;
            pnode->priority = PRIOR_VAIN;
            pnode->chargingplug_Id = 0;
            draw_pwrnode(ID_OF(pnode), true);
            if (pool_id != POOL_OF_NODE(ID_OF(pnode)))
            {
                pool_id = POOL_OF_NODE(ID_OF(pnode));
                POOL_REF(pool_id)->plugs_linked -= !!(POOL_REF(pool_id)->plugs_linked);
            }
            POOL_REF(pool_id)->nodes_worked -= !!(POOL_REF(pool_id)->nodes_worked);
        }
    }
    gear_clear(chargee);
    // print_oneliner("[Tactic] Recycling Plug %d completed.", ID_OF(chargee));
}

void allocating_Dealer(struct Alloc_plugObj *chargee, int quota)
{

    if (quota <= 0)
    {
        return;
    }

    get_Amphextremum(chargee);
    int shortage = quota;
    for (int i = 0; i < quota; i++)
    {
        handle_Demands_From_Charger(chargee, &shortage);
    }
    for (int j = 0; j < shortage; j++)
    {
        handle_Shortage_From_Peers(chargee, &shortage);
    }
    if (0 < shortage)
    {
        handle_Lowest_Node_Holdsome(chargee, &shortage);
    }

    chargee->strategy_info.shortage_demand = shortage;

    return;
}

void allocating_Reception(void)
{
    static uint32_t CCU_order_id = 0, CCU_deorder_id = 0;
    if (!CCU_order_id)
    {
        CCU_order_id = discover_Chargee_Demand_Routine();
        if (0 < CCU_order_id)
        {
            struct Alloc_plugObj *chargee = PLUG_REF(CCU_order_id);
            if (PRIOR_VAIN == chargee->strategy_info.priority || NODE_VAIN != chargee->next_charger)
            {
                return;
            }
            int quota = quota_Of_Demander(CCU_order_id); // 充电桩需要功率配额节点数
                                                         // int credits = credits_Line_Supply(chargee, NODE_MAX); // 充电桩可供分配节点数
                                                         // credits = credits > 0 ? credits : 0;
            quota = (quota > NODE_MAX) ? NODE_MAX : (quota > 0 ? quota : 0);
            allocating_Dealer(chargee, quota);
        }
    }
    if (!CCU_deorder_id)
    {
        CCU_deorder_id = discover_Chargee_Depart_Routine();
        if (0 < CCU_deorder_id)
        {
            struct Alloc_plugObj *chargee = PLUG_REF(CCU_deorder_id);
            if (PRIOR_VAIN != chargee->strategy_info.priority)
            {
                return;
            }
            if (NODE_VAIN == chargee->next_charger)
            {
                order_Plug(chargee);
            }
            else
            {
                recycling_Dearler(chargee);
                meet_Remand_Shortage(chargee);
            }
        }
    }

    checkout_Charger_Supply(&CCU_order_id, &CCU_deorder_id);
    return;
}