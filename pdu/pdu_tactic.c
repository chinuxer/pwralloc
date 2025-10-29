#include "pdu_broker.h"
#include "pdu_tactic.h"
#include "pdu_ex_datatype.h"
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
#define BE_WANTED(x) (-1.0f * (x))
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

static uint32_t usageratio_Of_Pool(struct Alloc_plugObj *plug, struct Alloc_nodeObj *node, uint32_t id_contactor)
{
    uint32_t nodes_worked = POOL_REF(POOL_OF_NODE(ID_OF(node)))->nodes_worked;
    nodes_worked = (NODES_PER_POOL - nodes_worked) * (WEIGHT_HIERARCHY - 1) / NODES_PER_POOL;
    return nodes_worked;
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
    case PRIOR_VIP:  // 不可抢占PRIOR_BASE优先级
                     // fallthru
    case PRIOR_BASE: // 该优先级不能抢占其他优先级
                     // fallthru
    default:
        bPreempt = false;
    }
    return bPreempt;
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
    struct Alloc_plugObj *plug_released = PLUG_REF(node->chargingplug_Id);
    if (gear_num(plug_released) <= MINIMUM_QUOTA) // 如果当前节点被占用且充电枪资源低于最低配额
    {
        return IMPARTICIP;
    }
    bool bPreempt = false;
    PRIOR req_priority = plug->strategy_info.priority;
    PRIOR target_priority = node->priority;
    uint32_t floor_supply = POOL_REF(POOL_OF_NODE(ID_OF(node)))->plugs_linked + 1;
    uint32_t seer = gear_num(plug_released);
    if (0 < floor_supply)
    {
        floor_supply = PRIORITY_ASSERT(CRITERION_PRIOR_EARLYPREFER) ? (seer > MINIMUM_QUOTA ? seer - MINIMUM_QUOTA : seer) : (PRIORITY_ASSERT(CRITERION_PRIOR_FAIRNOFAVOR) ? ((NODES_PER_POOL + 1) / floor_supply) : 0);
    }
    if ((req_priority == target_priority) || (PRIOR_VIP >= req_priority && PRIOR_VIP >= target_priority)) // 等优先级均分或先到先得 根据策略判断是否可剥夺
    {

        seer -= MINIMUM_QUOTA;
        if (seer >= floor_supply)
        {
            return DEPRIVABLE;
        }
        else
        {
            return IMPARTICIP;
        }
    }

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
    [3] = {WEIGHT_IDLE, usage_Of_Node, CRITERION_PRIOR},
    [4] = {WEIGHT_BLANK, overload_Of_CopperBar, CRITERION_LMT_CAPACITY},
    [5] = {WEIGHT_BLANK, NULL, CRITERION_MIN_COST},
    [6] = {WEIGHT_BLANK, NULL, CRITERION_MIN_COST},
    [7] = {WEIGHT_BLANK, NULL, CRITERION_MAX_POWER},
    [8] = {WEIGHT_BLANK, NULL, CRITERION_MAX_EFFICIENCY},
    [9] = {WEIGHT_BLANK, NULL, CRITERION_MAX_EFFICIENCY},
};

int get_Score_Of_Node(RATING_RANK *tab, struct Alloc_plugObj *plug, uint32_t id_contactor)
{
    struct Alloc_nodeObj *node = NODEREF_FROM_CONTACTOR(id_contactor);
    if (!tab || !node)
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

    (optional + cnt)->score *= (optional + cnt)->preselected ? -1 : 1;

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
void ordering_Procedure(OPTIMAL *optimum)
{
    struct Alloc_plugObj *chargee = PLUG_REF(optimum->plug_id);
    struct Alloc_nodeObj *node = NODE_REF(optimum->node_id);
    if (!optimum || !chargee || !node || 0 == optimum->contactor_id)
    {
        return;
    }
    optimum->preselected = true;
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

void makeup_Shortage_Demand(struct Alloc_plugObj *chargee)
{
#define GRADING_BY(demand, priority) ((int)(demand + 100 * priority))
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
    VLA_INSTANT(OPTIMAL, candidates, PLUG_MAX);
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
                (candidates + (ID_OF(other_chargee) - 1) % PLUG_MAX)->contactor_id = id;
                (candidates + (ID_OF(other_chargee) - 1) % PLUG_MAX)->node_id = NODE_OF_CONTACTOR(id);
                (candidates + (ID_OF(other_chargee) - 1) % PLUG_MAX)->plug_id = ID_OF(other_chargee);
                (candidates + (ID_OF(other_chargee) - 1) % PLUG_MAX)->score = GRADING_BY(other_chargee->strategy_info.shortage_demand, other_chargee->strategy_info.priority);
            }
            else
            {
                (candidates + (ID_OF(other_chargee) - 1) % PLUG_MAX)->score = -1;
            }
        }
        if (occupied)
        {
            continue;
        }
        OPTIMAL *optimum = find_highest_score(candidates, PLUG_MAX, 0);
        if (NULL == optimum)
        {
            continue;
        }
        other_chargee = PLUG_REF(optimum->plug_id);
        other_chargee->strategy_info.shortage_demand -= !!other_chargee->strategy_info.shortage_demand;
        ordering_Procedure(optimum);
    }
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
    print_oneliner("[Tactic] Recycling Plug %d completed.", ID_OF(chargee));
}

void allocating_Dealer(struct Alloc_plugObj *chargee, int quota)
{
    uint32_t credits = list_len_safe(&chargee->copula, NODE_MAX);
    if (0 == credits || quota <= 0)
    {
        return;
    }

    VLA_INSTANT(OPTIMAL, candidates, credits);
    ListObj *pos;
    OPTIMAL *optimum = NULL;
    get_Amphextremum(chargee);
    int shortage = quota;
    for (int i = 0; i < quota; i++)
    {
        int cnt = 0;
        list_for_each(pos, &chargee->copula)
        {
            optimum = grading_Charger_Linked(pos, candidates, cnt++);
        }
        if (4 == credits)
        {
            print_oneliner("(%d) %d:%d %d:%d %d:%d %d:%d", (optimum ? optimum->node_id : 0), candidates[0].node_id, candidates[0].score, candidates[1].node_id, candidates[1].score, candidates[2].node_id, candidates[2].score, candidates[3].node_id, candidates[3].score);
        }
        if (8 == credits)
        {
            print_oneliner("(%d) %d:%d %d:%d %d:%d %d:%d %d:%d %d:%d %d:%d %d:%d", (optimum ? optimum->node_id : 0), candidates[0].node_id, candidates[0].score, candidates[1].node_id, candidates[1].score, candidates[2].node_id, candidates[2].score, candidates[3].node_id, candidates[3].score, candidates[4].node_id, candidates[4].score, candidates[5].node_id, candidates[5].score, candidates[6].node_id, candidates[6].score, candidates[7].node_id, candidates[7].score);
        }
        if (NULL == optimum)
        {
            break;
        }
        if ((0 == optimum->node_id) || (0 == optimum->contactor_id))
        {
            break;
        }
        if ((NODE_MAX < optimum->node_id) || (CONTACTOR_MAX < optimum->contactor_id))
        {
            break;
        }

        // print_oneliner("[Tactic] Allocating Node %d to Plug %d with Score %d", optimum.node_id, ID_OF(chargee), optimum.score);
        shortage = (--shortage < 0 ? 0 : shortage);
        optimum->plug_id = ID_OF(chargee);
        ordering_Procedure(optimum);
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
            if (PRIOR_VAIN != chargee->strategy_info.priority || NODE_VAIN == chargee->next_charger)
            {
                return;
            }
            recycling_Dearler(chargee);
            makeup_Shortage_Demand(chargee);
        }
    }

    checkout_Charger_Supply(&CCU_order_id, &CCU_deorder_id);
    return;
}