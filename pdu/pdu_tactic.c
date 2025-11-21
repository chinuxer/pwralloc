#include "pdu_broker.h"
#include "pdu_tactic.h"
#include <stdint.h>

int32_t quota_Of_Demander(ID_TYPE plug_id, bool bIsMax)
{
    float gross = bIsMax ? PLUG_REF(plug_id)->strategy_info.longing : PLUG_REF(plug_id)->strategy_info.demand;
    gross = (gross > 0.0f) ? gross + QUOTA_FLUCT : 0.0f;

    float q = ceilf(gross / MAXIM_CURRENT_OF_NODE);

    /* 1. assert NaN / Inf */
    if (!(bool)isfinite(q))
    {
        return INT32_MIN;
    }
    // cstat #CERT-FLP36-C
    /* 2. within range int32_t  */
    if (q > (float)INT32_MAX)
    {
        return INT32_MAX; /* cast */
    }
    if (q < (float)INT32_MIN)
    {
        return INT32_MIN;
    }

    /* 3. checked conversion */
    return (int32_t)q;
}

static bool deprivable_By_Priority(PRIOR req_priority, PRIOR target_priority)
{
#define UNOCCUPIED 2 // 空闲
#define DEPRIVABLE 1 // 占用但可剥夺
#define IMPARTICIP 0 // 不可剥夺
    bool bPreempt;
    if (PRIOR_VAIN == req_priority)
    {
        return true;
    }
    switch (req_priority)
    {
    case PRIOR_EXTREME: // 最高优先级：可抢占任何优先级
        bPreempt = (target_priority == PRIOR_SVIP || target_priority == PRIOR_VIP || target_priority == PRIOR_BASE);
        break;
    case PRIOR_SVIP: // 可抢占优先级PRIOR_BASE
        bPreempt = (target_priority == PRIOR_BASE);
        break;

    case PRIOR_VIP:  // 不可抢占PRIOR_BASE优先级,只可分摊
                     /* MISRA C:2012 Rule 16.3: wilfull fallthru to default */
    case PRIOR_BASE: // 该优先级不能抢占其他优先级
                     /* MISRA C:2012 Rule 16.3: wilfull fallthru to default */
    default:
        bPreempt = false;
        break;
    }
    return bPreempt;
}
static bool shareable_By_Priority(PRIOR req_priority, PRIOR target_priority)
{
    return (req_priority == target_priority || (PRIOR_VIP == req_priority && PRIOR_BASE == target_priority));
}
static uint32_t peer_Competitor(const struct Alloc_plugObj *plug)
{
    ListObj *pos;
    uint32_t quantum = 0;
    VLA_INSTANT(ID_TYPE, chargingplug_array, CONTACTORS_PER_NODE);
    list_for_each(pos, &plug->copula)
    {
        ID_TYPE chargingplug_id = NODEREF_FROM_CONTACTOR(ID_OF(pos))->chargingplug_Id;
        ID_TYPE node_id = NODEREF_FROM_CONTACTOR(ID_OF(pos))->id;
        if (chargingplug_id == ID_OF(plug))
        {
            continue;
        }
        ListObj *contactor = get_Cross_BtwnPlugNode(node_id, chargingplug_id);
        if ((bool)contactor && !contactor->ia_contacted)
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
static uint32_t peer_Resource(const struct Alloc_plugObj *plug)
{
    ListObj *pos;
    uint32_t quantum = 0;
    PRIOR req_priority, target_priority;

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
static int32_t seniority_Of_Node_ScoringRule(const struct Alloc_plugObj *plug, const struct Alloc_nodeObj *node, ID_TYPE id_contactor)
{
#define RANGE_NORMALIZE 100

    uint32_t workhours;

    if (plug->strategy_info.workhours_topnode <= plug->strategy_info.workhours_bottomnode)
    {
        return (RANGE_NORMALIZE) / 2;
    }
    else
    {
        workhours = (uint32_t)GET_PCU_RAWDATA(ID_OF(node), PCUDATA_OF_NODE_WORKHOURS, int32);
        workhours = workhours > 0 ? workhours : 0;
        workhours = workhours * RANGE_NORMALIZE / (plug->strategy_info.workhours_topnode - plug->strategy_info.workhours_bottomnode);
        workhours = RANGE_NORMALIZE - workhours;
        return (int32_t)(workhours % (RANGE_NORMALIZE + 1));
    }
}
static int32_t seniority_Of_Contactor_ScoringRule(const struct Alloc_plugObj *plug, const struct Alloc_nodeObj *node, ID_TYPE id_contactor)
{
    return 0;
}
static int32_t holding_Of_PlugChargedBy_Node_ScoringRule(const struct Alloc_plugObj *plug, const struct Alloc_nodeObj *node, ID_TYPE id_contactor)
{
    struct Alloc_plugObj *plug_of_node = PLUG_REF(node->chargingplug_Id);
    int32_t num = (int32_t)gear_num(plug_of_node);
    return num > (WEIGHT_HIERARCHY - 1) ? (WEIGHT_HIERARCHY - 1) : num;
}
static int32_t usageratio_Of_Pool_ScoringRule(const struct Alloc_plugObj *plug, const struct Alloc_nodeObj *node, ID_TYPE id_contactor)
{
    uint32_t nodes_worked = POOL_REF(POOL_OF_NODE(ID_OF(node)))->nodes_worked;
    nodes_worked = (NODES_PER_POOL - nodes_worked) * (WEIGHT_HIERARCHY - 1) / NODES_PER_POOL;
    return (int32_t)nodes_worked;
}
static int32_t overload_Of_CopperBar_ScoringRule(const struct Alloc_plugObj *plug, const struct Alloc_nodeObj *node, ID_TYPE id_contactor)
{
    // cstat #CERT-FLP36-C #CERT-FLP34-C
    ID_TYPE bar_id = COPPERBAR_OF_CONTACTOR(id_contactor);
    ID_TYPE pool_id = POOL_OF_CONTACTOR(id_contactor);

    ListObj *pos;
    float total_ampere = 0.0f;
    int quota = quota_Of_Demander(ID_OF(plug), false);
    if (0 == quota)
    {
        return -1;
    }
    float step_ampere = (float)(plug->strategy_info.longing / (quota * 1.0));
    list_for_each(pos, &plug->copula)
    {
        if (!pos->ia_contacted)
        {
            continue;
        }
        if (pool_id != POOL_OF_CONTACTOR(ID_OF(pos)))
        {
            continue;
        }
        float current = GET_PCU_RAWDATA(ID_OF(pos), PCUDATA_OF_NODE_CURRENT, float);
        if (!pos->ia_contacted)
        {
            continue;
        }
        if (current > 5.0f)
        {
            total_ampere += current;
        }
        else
        {
            total_ampere += step_ampere;
        }
    }

    return (total_ampere + step_ampere > COPBAR_REF(bar_id)->current_max + 10.0f) ? 1 : -1;
}

static int32_t usage_Of_Node_ScoringRule(const struct Alloc_plugObj *plug, const struct Alloc_nodeObj *node, ID_TYPE id_contactor)
{

    if (!(bool)plug || !(bool)node)
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
    bool bPreempt;
    PRIOR req_priority = plug->strategy_info.priority;
    PRIOR target_priority = node->priority;

    bPreempt = deprivable_By_Priority(req_priority, target_priority); // 判断是否可剥夺

    return (bPreempt ? DEPRIVABLE : IMPARTICIP);
}
static int32_t shortage_Of_Plug_Reverse_ScoringRule(const struct Alloc_plugObj *plug, const struct Alloc_nodeObj *node, ID_TYPE id_contactor)
{
    ID_TYPE plug_id = node->chargingplug_Id;
    int32_t shortage = PLUG_REF(plug_id)->strategy_info.shortage_demand;
    return (int32_t)(POSTPRIMA_WEIGHT1 - 1 - shortage % POSTPRIMA_WEIGHT1);
}
static int32_t priority_Of_Plug_Reverse_ScoringRule(const struct Alloc_plugObj *plug, const struct Alloc_nodeObj *node, ID_TYPE id_contactor)
{
    PRIOR priority = node->priority;
    return (int32_t)((size_t)PRIOR_EXTREME - (size_t)priority);
}
static int32_t possess_Of_Plug_ScoringRule(const struct Alloc_plugObj *plug, const struct Alloc_nodeObj *node, ID_TYPE id_contactor)
{
    ID_TYPE plug_id = node->chargingplug_Id;
    uint32_t possess = gear_num(PLUG_REF(plug_id));
    return (int32_t)possess;
}
static int32_t sharable_Of_Plug_ScoringRule(const struct Alloc_plugObj *plug, const struct Alloc_nodeObj *node, ID_TYPE id_contactor)
{
    PRIOR req_priority = plug->strategy_info.priority;
    PRIOR target_priority = node->priority;
    int32_t shareship;
    if (plug->id == node->chargingplug_Id)
    {
        shareship = -1;
    }
    else
    {
        shareship = shareable_By_Priority(req_priority, target_priority) ? 1 : -1;
    }

    return shareship;
}

static inline int32_t succession_Heirs_ScoringRule(const STRAGTEGY_BASIS *factor, ID_TYPE current_chargee_id, ID_TYPE target_chargee_id)
{
    if (current_chargee_id == target_chargee_id)
    {
        return -1;
    }

    double duration_component = factor->duration * WEIGHT_HIERARCHY * 1.0 / (uint16_t)-1;
    double soc_component = POSTPRIMA_WEIGHT1 * (factor->soc * WEIGHT_HIERARCHY / 100.0);
    int32_t demand_component = POSTPRIMA_WEIGHT2 * factor->shortage_demand;
    int32_t priority_component = POSTPRIMA_WEIGHT3 * factor->priority;
    int32_t demand_flag_component = POSTPRIMA_WEIGHT4 * (factor->shortage_demand > 0);

    return (int32_t)(duration_component + soc_component + demand_component + priority_component + demand_flag_component);
}

const RATING_RANK SCOREDEED_ARRAY_PREEMPT[] = {
    [0] = {WEIGHT_CONTACTOR_ACT, seniority_Of_Contactor_ScoringRule, CRITERION_EQU_SANITY},
    [1] = {WEIGHT_WORK_TIME, seniority_Of_Node_ScoringRule, CRITERION_EQU_SANITY},
    [2] = {WEIGHT_POOL_OCCUPIED, usageratio_Of_Pool_ScoringRule, CRITERION_EQU_SANITY},
    [3] = {WEIGHT_HOLDING, holding_Of_PlugChargedBy_Node_ScoringRule, CRITERION_PRIOR},
    [4] = {WEIGHT_DEPRIV, usage_Of_Node_ScoringRule, CRITERION_PRIOR},
    [5] = {WEIGHT_POLARITY, overload_Of_CopperBar_ScoringRule, CRITERION_LMT_CAPACITY},
    [6] = {WEIGHT_BLANK, NULL, CRITERION_MIN_COST},
    [7] = {WEIGHT_BLANK, NULL, CRITERION_MIN_COST},
    [8] = {WEIGHT_BLANK, NULL, CRITERION_MAX_POWER},
    [9] = {WEIGHT_BLANK, NULL, CRITERION_MAX_EFFICIENCY},
};
const RATING_RANK SCOREDEED_ARRAY_RELIEVE[] = {
    [0] = {POSTPRIMA_WEIGHT0, shortage_Of_Plug_Reverse_ScoringRule, CRITERION_ALLOWANCE_MINIMUM},
    [1] = {POSTPRIMA_WEIGHT1, priority_Of_Plug_Reverse_ScoringRule, CRITERION_ALLOWANCE_MINIMUM},
    [2] = {POSTPRIMA_WEIGHT2, possess_Of_Plug_ScoringRule, CRITERION_ALLOWANCE_MINIMUM},
    [3] = {WEIGHT_BLANK, NULL, CRITERION_ALLOWANCE_MINIMUM},
};
const RATING_RANK SCOREDEED_ARRAY_SHARING[] = {
    [0] = {POSTPRIMA_WEIGHT0, shortage_Of_Plug_Reverse_ScoringRule, CRITERION_PEERS_FAIRNOFAVOR},
    [1] = {POSTPRIMA_WEIGHT1, possess_Of_Plug_ScoringRule, CRITERION_PEERS_FAIRNOFAVOR},
    [2] = {WEIGHT_POLARITY, sharable_Of_Plug_ScoringRule, CRITERION_PEERS_FAIRNOFAVOR},
    [3] = {WEIGHT_BLANK, NULL, CRITERION_PEERS_FAIRNOFAVOR},
};
static const EVALUATION_REFER_TAB Eulection_Tree[] = {
    EVALUATION_REFER_MACRO(PREEMPT),
    EVALUATION_REFER_MACRO(SHARING),
    EVALUATION_REFER_MACRO(RELIEVE),
    {handle_Tactic_INHERIT, "handle_Tactic_INHERIT", NULL}};
static int get_Score_Of_Node(const RATING_RANK *tab, struct Alloc_plugObj *plug, ID_TYPE id_contactor)
{
    struct Alloc_nodeObj *node = NODEREF_FROM_CONTACTOR(id_contactor);
    int score = 0;
    if (!(bool)tab || !(bool)node || !(bool)plug || !(bool)id_contactor)
    {
        return score;
    }
    if (node->discarded || CONTACTOR_REF(id_contactor)->discarded)
    {
        return score;
    }

    while (NULL != tab->func)
    {
        if (!PRIORITY_ASSERT(tab->criteria))
        {
            tab++;
            continue;
        }
        if (WEIGHT_POLARITY == tab->weight)
        {
            score *= tab->func(plug, node, id_contactor);
        }
        else
        {
            score += tab->func(plug, node, id_contactor) * tab->weight;
        }

        tab++;
    }
    return score;
}
static void score_print(const OPTIMAL *optimum, const OPTIMAL *candidates, uint32_t credits)
{
#define SCORE_PRINT_PERLINE (5)
    if (0 == credits)
    {
        return;
    }
    char buffer[256];
    int offset = 0;

    // Print optimal node info
    // cstat -CERT-ERR33-C_a
    offset += snprintf(buffer + offset, sizeof(buffer) - offset, "(%d)",
                       (NULL != optimum ? optimum->node_id : 0));

    // Print candidates dynamically
    for (int i = 0; i < credits && i < NODE_MAX && offset < sizeof(buffer) - 32; i++)
    {
        offset += snprintf(buffer + offset, sizeof(buffer) - offset, " %" PRIu32 ":%" PRId32,
                           candidates[i].node_id, candidates[i].score);
        if (SCORE_PRINT_PERLINE == i % (SCORE_PRINT_PERLINE + 1))
        {
            print_oneliner("%s", buffer);
            offset = 0;
        }
    }
    // cstat +CERT-ERR33-C_a
    if (0 < offset)
    {
        print_oneliner("%s", buffer);
    }
}
// cstat !MISRAC2012-Rule-8.7
void set_allocation_criterion(CRITERION criterion)
{
    CRITERIA |= (size_t)criterion;
}
static const OPTIMAL *find_highest_score(const OPTIMAL *aristo, size_t count, int blue_line)
{
    if (count <= 0)
    {
        return NULL;
    }
    const OPTIMAL *highest = aristo;
    for (size_t i = 1; i < count; i++)
    {
        if (aristo[i].score > highest->score)
        {
            highest = &aristo[i];
        }
    }

    return (highest->score >= blue_line ? highest : NULL);
}
static const OPTIMAL *grading_By_Scores(const RATING_RANK *prank, const ListObj *last, OPTIMAL *optional, size_t cnt, int32_t pass_line)
{
    if (!(bool)last || !(bool)optional)
    {
        return NULL;
    }

    (optional + cnt)->contactor_id = ID_OF(last);
    (optional + cnt)->node_id = NODE_OF_CONTACTOR(ID_OF(last));
    struct Alloc_plugObj *header = get_header_plug(ID_OF(last));
    if (!(bool)header)
    {
        return NULL;
    }
    (optional + cnt)->plug_id = header->id;
    (optional + cnt)->score = get_Score_Of_Node(prank, header, ID_OF(last));
    return find_highest_score(optional, cnt + 1, pass_line);
}
static void get_Amphextremum(struct Alloc_plugObj *chargee)
{

    ListObj *pos;
    uint32_t workhours;
    list_for_each(pos, &chargee->copula)
    {
        workhours = (uint32_t)GET_PCU_RAWDATA(NODE_OF_CONTACTOR(ID_OF(pos)), PCUDATA_OF_NODE_WORKHOURS, int32);
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
static void ordering_Procedure(const OPTIMAL *optimum, bool to_turnon)
{
    struct Alloc_plugObj *chargee = PLUG_REF(optimum->plug_id);
    struct Alloc_nodeObj *node = NODE_REF(optimum->node_id);
    if (!(bool)optimum || !(bool)chargee || !(bool)node || 0 == optimum->contactor_id)
    {
        return;
    }

    order_Node(node);
    order_Contactor(CONTACTOR_REF(optimum->contactor_id), to_turnon);
    order_Plug(chargee);
    outcomes_Collate_Allocation(OUTCOMES_INC);
    if (0 < node->chargingplug_Id && PRIOR_VAIN != node->priority)
    {
        struct Alloc_plugObj *plug_released = PLUG_REF(node->chargingplug_Id);

        plug_released->strategy_info.shortage_demand += 1;

        gear_remove(plug_released, node);
    }

    gear_insert(chargee, node);
    struct Tactic_PwrPoolObj *pool = POOL_REF(POOL_OF_NODE(optimum->node_id));
    if (NULL != pool && pool->nodes_worked < NODES_PER_POOL)
    {
        pool->nodes_worked += 1;
    }
}

static bool deordering_Procedure(struct Alloc_plugObj *chargee, ListObj *devorced)
{
    ID_TYPE pool_id = 0;
    struct Alloc_nodeObj *pnode = NODEREF_FROM_CONTACTOR(ID_OF(devorced));
    if (CONTACTOR_REF(ID_OF(devorced))->ia_contacted == true)
    {
        outcomes_Collate_Allocation(pnode->id, 0, 0);
        deorder_Contactor(devorced);
        outcomes_Collate_Allocation(OUTCOMES_INC);
    }

    if (pnode->chargingplug_Id == chargee->id)
    {
        pnode->value_Iset = 0.0f;
        pnode->value_Vset = 0.0f;
        pnode->priority = PRIOR_VAIN;
        pnode->chargingplug_Id = 0;
        draw_pwrnode(ID_OF(pnode), true);
        if (pool_id != POOL_OF_NODE(ID_OF(pnode)))
        {
            pool_id = POOL_OF_NODE(ID_OF(pnode));
        }
        POOL_REF(pool_id)->nodes_worked -= (int32_t)(!!(bool)((POOL_REF(pool_id)->nodes_worked)));
        gear_remove(chargee, pnode);
        return true;
    }
    return false;
}

static bool evaluate_candidates(struct Alloc_plugObj *chargee, char const *funcion_name, int32_t pass_line)
{
    if (!(bool)chargee || !(bool)funcion_name)
    {
        return false;
    }
    const RATING_RANK *prank = NULL;
    for (int i = 0; i < sizeof(Eulection_Tree) / sizeof(EVALUATION_REFER_TAB); i++)
    {
        if (strncmp(funcion_name, Eulection_Tree[i].name, 21) == 0)
        {
            prank = Eulection_Tree[i].rank;
            break;
        }
    }
    if (NULL == prank)
    {
        return false;
    }
    uint32_t credits = list_len_safe(&chargee->copula, NODE_MAX);
    if (0 == credits)
    {
        return false;
    }
    VLA_INSTANT(OPTIMAL, candidates, credits);
    ListObj *pos;
    const OPTIMAL *optimum = NULL;

    int cnt = 0;
    list_for_each(pos, &chargee->copula)
    {
        optimum = grading_By_Scores(prank, pos, candidates, cnt++ % credits, pass_line);
    }
    print_oneliner("[Tactic] %s:%" PRId32, funcion_name, pass_line);
    score_print(optimum, candidates, credits);
    if (NULL == optimum)
    {
        order_Plug(chargee);
        return false;
    }
    if ((0 == optimum->node_id) || (0 == optimum->contactor_id))
    {
        return false;
    }
    if ((NODE_MAX < optimum->node_id) || (CONTACTOR_MAX < optimum->contactor_id))
    {
        return false;
    }

    // print_oneliner("[Tactic] Allocating Node %d to Plug %d with Score %d", optimum.node_id, ID_OF(chargee), optimum.score);
    int shortage = chargee->strategy_info.shortage_demand - 1;
    chargee->strategy_info.shortage_demand = (shortage < 0 ? 0 : shortage);
#define CONTACTON true
    ordering_Procedure(optimum, !CONTACTON);
    return true;
}
static bool handle_Tactic_PREEMPT(struct Alloc_plugObj *chargee)
{
    bool res = evaluate_candidates(chargee, __func__, (WEIGHT_DEPRIV * DEPRIVABLE));
    return res;
}

static bool handle_Tactic_RELIEVE(struct Alloc_plugObj *chargee)
{
    bool res = evaluate_candidates(chargee, __func__, (POSTPRIMA_WEIGHT2 * MINIMUM_QUOTA));
    return res;
}
static bool handle_Tactic_SHARING(struct Alloc_plugObj *chargee)
{
    uint32_t peers = peer_Competitor(chargee) + 1;
    uint32_t resources = peer_Resource(chargee);
    if (0 == resources || 0 == peers)
    {
        return false;
    }
    uint32_t floor_supply = (resources + 1) / peers;
    floor_supply = (floor_supply > 1) ? floor_supply : 1;
    if (floor_supply <= gear_num(chargee))
    {
        return false;
    }

    bool res = evaluate_candidates(chargee, __func__, (int32_t)((floor_supply + 1) * POSTPRIMA_WEIGHT1));
    return res;
}

static bool handle_Tactic_INHERIT(struct Alloc_plugObj *chargee)
{
    if (!chargee)
    {
        return false;
    }
    uint32_t listlengh = list_len_safe(&chargee->copula, NODE_MAX);
    if (listlengh == 0)
    {
        return false;
    }
    const ListObj *pos;
    list_for_each(pos, &chargee->copula)
    {
        const struct Alloc_nodeObj *node_ref = NODEREF_FROM_CONTACTOR(ID_OF(pos));
        if (node_ref->chargingplug_Id > 0 || node_ref->priority != PRIOR_VAIN || node_ref->discarded)
        {
            continue;
        }
        ID_TYPE leftmost_id = leftmost_Contactor_IN_Pool(ID_OF(pos));
        bool occupied = false;
        VLA_INSTANT(OPTIMAL, candidates, CONTACTORS_PER_NODE);
        for (ID_TYPE id = leftmost_id; id < leftmost_id + CONTACTORS_PER_NODE; id++)
        {
            struct Alloc_plugObj *other_chargee = get_header_plug(id);
            if (!other_chargee || CONTACTOR_REF(id)->discarded)
            {
                continue;
            }

            occupied = occupied || CONTACTOR_REF(id)->ia_contacted;
            STRAGTEGY_BASIS *factor = &other_chargee->strategy_info;
            candidates[(id - leftmost_id) % CONTACTORS_PER_NODE] = (OPTIMAL){
                .contactor_id = id,
                .node_id = NODE_OF_CONTACTOR(id),
                .plug_id = ID_OF(other_chargee),
                .score = succession_Heirs_ScoringRule(factor, ID_OF(other_chargee), ID_OF(chargee))};
        }
        if (occupied)
        {
            continue;
        }
        int32_t qualifying = (int32_t)(MINIMUM_QUOTA * POSTPRIMA_WEIGHT4 + PRIOR_BASE * POSTPRIMA_WEIGHT3 + MINIMUM_QUOTA * POSTPRIMA_WEIGHT2);
        const OPTIMAL *optimum = find_highest_score(candidates, CONTACTORS_PER_NODE, qualifying);
        if (!optimum || !optimum->node_id)
        {
            continue;
        }
        struct Alloc_plugObj *target = PLUG_REF(optimum->plug_id);
        if (target->strategy_info.shortage_demand > 0)
        {
            target->strategy_info.shortage_demand--;
        }
        print_oneliner("[Tactic] %s:%" PRId32, __func__, qualifying);
        score_print(optimum, candidates, CONTACTORS_PER_NODE);
        ordering_Procedure(optimum, CONTACTON);
    }
    return true;
#undef GRADING_BY
#undef TRUNCATE_SCALING
}
void rescinding_Dealer(struct Alloc_plugObj *chargee, int kickoff)
{
    if (0 == kickoff)
    {
        return;
    }
    if (kickoff == gear_num(chargee))
    {
        order_Plug(chargee);
    }
    outcomes_Collate_Allocation(OUTCOMES_CLR);
    ListObj *pos;

    list_for_each(pos, &chargee->copula)
    {
        if (0 == kickoff)
        {
            break;
        }

        if (deordering_Procedure(chargee, pos) && kickoff > 0)
        {
            kickoff--;
        }
    }

    // print_oneliner("[Tactic] Recycling Plug %d completed.", ID_OF(chargee));
    chargee->strategy_info.tag_realloc = REALLOCATOR;
    print_oneliner(NULL);
    return;
}

void insulatchk_Dealer(struct Alloc_plugObj *chargee)
{
    get_Amphextremum(chargee);
    outcomes_Collate_Allocation(OUTCOMES_CLR);
    bool res = PREEMPT[Eulection_Tree].func(chargee);
    if (!res)
    {
        res = SHARING[Eulection_Tree].func(chargee);
    }
    if (!res)
    {
        res = RELIEVE[Eulection_Tree].func(chargee);
    }
    if (res)
    {
        chargee->strategy_info.tag_realloc = REALLOCATEE;
    }
    print_oneliner(NULL);
    return;
}
void allocating_Dealer(struct Alloc_plugObj *chargee, int quota)
{

    if (quota <= 0)
    {
        return;
    }
    outcomes_Collate_Allocation(OUTCOMES_CLR);
    chargee->strategy_info.shortage_demand = quota;
    bool res = NULL;
    for (int i = 0; i < quota; i++)
    {
        res = (bool)PREEMPT[Eulection_Tree].func(chargee) || res;
    }
    quota = chargee->strategy_info.shortage_demand;
    for (int j = 0; j < quota; j++)
    {
        res = (bool)SHARING[Eulection_Tree].func(chargee) || res;
    }
    if (res)
    {
        chargee->strategy_info.tag_realloc = REALLOCATEE;
    }
    print_oneliner(NULL);
    return;
}

void reallocate_Dealer(struct Alloc_plugObj *chargee)
{
    outcomes_Collate_Allocation(OUTCOMES_CLR);
    if (REALLOCATOR == chargee->strategy_info.tag_realloc)
    {
        INHERIT[Eulection_Tree].func(chargee);
    }
    else if (REALLOCATEE == chargee->strategy_info.tag_realloc)
    {
        ListObj *pos;
        list_for_each(pos, &chargee->copula)
        {
            if (NODEREF_FROM_CONTACTOR(ID_OF(pos))->chargingplug_Id == chargee->id && false == pos->ia_contacted)
            {
                outcomes_Collate_Allocation(NODEREF_FROM_CONTACTOR(ID_OF(pos))->id, pos->id, 0);
                pos->ia_contacted = true;
                draw_contactor(ID_OF(pos), true);
                outcomes_Collate_Allocation(OUTCOMES_INC);
            }
        }
    }
    print_oneliner(NULL);
    chargee->strategy_info.tag_realloc = NEITHERFLAG;
}