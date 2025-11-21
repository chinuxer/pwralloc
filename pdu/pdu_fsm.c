
#include "pdu_broker.h"
#include "pdu_tactic.h"

bool init_PDU_Broker(void);
bool hear_Canaries_Twittering(void);
bool opt_Visual_Print_Flag(VISUAL_PRINT_OPRT flag);
void print_TopoGraph(void);

typedef struct
{
    PDU_CMD current_cmd;
    alignas(4) bool worxtream;
    PDU_STA (*handler)(va_list *args);
} StateTransition;

static inline bool verify_Contactor_Status(ID_TYPE contactor_id, bool is_contactor_on)
{
    bool state = false;
    return (is_contactor_on == state);
}

static inline bool verify_Node_Status(ID_TYPE contactor_id)
{
    return true;
}
static bool assert_LastDeed_Undone(void)
{
    return false;
    bool ret = false;
#define CHECK_ON true
    
    for (NODE_OPRATION *pwrite_off = g_sheduled_worx; NULL != pwrite_off; pwrite_off = pwrite_off->next)
    {
        // Check contactor ON status
        if (0 < pwrite_off->contactor_on_id)
        {
            if (verify_Contactor_Status(pwrite_off->contactor_on_id, CHECK_ON))
                pwrite_off->contactor_on_id = 0;
            ret = ret || (0 < pwrite_off->contactor_on_id);
        }
        
        // Check contactor OFF status
        if (0 < pwrite_off->contactor_off_id)
        {
            if (verify_Contactor_Status(pwrite_off->contactor_off_id, !CHECK_ON))
                pwrite_off->contactor_off_id = 0;
            ret = ret || (0 < pwrite_off->contactor_off_id);
        }
        
        // Check node status - clear if all conditions met
        if (0 < pwrite_off->node_id)
        {
            if (verify_Node_Status(pwrite_off->node_id) && 
                0 == pwrite_off->contactor_off_id && 
                0 == pwrite_off->contactor_on_id)
            {
                pwrite_off->node_id = 0;
            }
            ret = ret || (0 < pwrite_off->node_id);
        }
    }
    return ret;
}

static bool assert_NextDeed_Todo(void)
{
    if (opt_Visual_Print_Flag(GET_STATUS) && g_sheduled_worx->node_id > 0)
    {
        char buf[64 + 32] = {0};
        NODE_OPRATION *p = g_sheduled_worx;
        while (NULL != p && p->node_id > 0)
        {
            int written_len = snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf),
                                       "(%" PRIu32 ")%" PRIu32 ":%" PRIu32 "; ",
                                       p->node_id,
                                       p->contactor_on_id,
                                       p->contactor_off_id);
            if (written_len <= 0)
            {
                break;
            }

            p = p->next;
        }
        print_oneliner("next deed: %s", buf);
    }
    return (g_sheduled_worx->node_id > 0);
}

static PDU_STA handle_init_cmd(va_list *args)
{
    outcomes_Collate_Allocation(OUTCOMES_CLR);
    return (init_PDU_Broker() ? PDU_STA_WORKING : PDU_STA_DISCARD);
}

static PDU_STA handle_plugin_cmd(va_list *args)
{
    va_list copy;
    va_copy(copy, *args);
    ID_TYPE CCU_placed_id = (ID_TYPE)va_arg(copy, ID_TYPE);
    PRIOR priority = (PRIOR)va_arg(copy, int);
    va_end(copy);
    if (CCU_placed_id > PLUG_MAX)
    {
        return PDU_STA_FAILURE;
    }
    struct Alloc_plugObj *chargee = PLUG_REF(CCU_placed_id);

    if (PRIOR_VAIN != chargee->strategy_info.priority ||
        NODE_VAIN != chargee->next_charger)
    {
        return PDU_STA_FAILURE;
    }

    chargee->strategy_info.priority = priority;
    insulatchk_Dealer(chargee);
    if (assert_NextDeed_Todo())
    {
        return PDU_STA_PENDING;
    }
    return PDU_STA_FAILURE;
}

static PDU_STA handle_charging_cmd(va_list *args)
{
#define MAX_DEMAND_CURRENT 620.0f
#define GET_MAX_DEMAND_FLAG true
    va_list copy;
    va_copy(copy, *args);
    ID_TYPE CCU_ordering_id = (ID_TYPE)va_arg(copy, ID_TYPE);
    float longfor_current = va_arg(copy, double);
    va_end(copy);
    if (CCU_ordering_id > PLUG_MAX)
    {
        return PDU_STA_FAILURE;
    }
    struct Alloc_plugObj *chargee = PLUG_REF(CCU_ordering_id);

    if (PRIOR_VAIN == chargee->strategy_info.priority)
    {
        return PDU_STA_FAILURE;
    }

    if (longfor_current <= 0.0f || longfor_current > MAX_DEMAND_CURRENT)
    {
        return PDU_STA_FAILURE;
    }

    chargee->strategy_info.longing = longfor_current;
    chargee->strategy_info.demand = longfor_current;
    int32_t quota = quota_Of_Demander(CCU_ordering_id, GET_MAX_DEMAND_FLAG);
    quota = (int32_t)((quota > NODE_MAX) ? NODE_MAX : (quota > MINIMUM_QUOTA ? quota : 0));
    allocating_Dealer(chargee, quota - MINIMUM_QUOTA);

    if (assert_NextDeed_Todo())
    {
        return PDU_STA_PENDING;
    }
    return PDU_STA_FAILURE;
}

static PDU_STA handle_plugout_cmd(va_list *args)
{
    va_list copy;
    va_copy(copy, *args);
    ID_TYPE CCU_deorder_id = (ID_TYPE)va_arg(copy, ID_TYPE);
    va_end(copy);
    if (CCU_deorder_id > PLUG_MAX)
    {
        return PDU_STA_FAILURE;
    }
    struct Alloc_plugObj *chargee = PLUG_REF(CCU_deorder_id);

    if (PRIOR_VAIN == chargee->strategy_info.priority)
    {
        return PDU_STA_FAILURE;
    }

    // Reset chargee strategy info
    chargee->strategy_info.demand = 0.0f;
    chargee->strategy_info.longing = 0.0f;
    chargee->strategy_info.workhours_bottomnode = 0;
    chargee->strategy_info.workhours_topnode = 0;
    chargee->strategy_info.priority = PRIOR_VAIN;
    chargee->strategy_info.shortage_demand = 0;
    order_Plug(chargee);

    int32_t kickoff = (int32_t)gear_num(chargee);
    print_oneliner("[Tactic] Recycling Plug %d for %d nodes",
                   ID_OF(chargee), kickoff);
    rescinding_Dealer(chargee, kickoff);

    if (assert_NextDeed_Todo())
    {
        return PDU_STA_PENDING;
    }
    return PDU_STA_FAILURE;
}

static PDU_STA handle_visual_cmd(va_list *args)
{
    va_list copy;
    va_copy(copy, *args);
    bool data = va_arg(copy, bool);
    va_end(copy);
    if (data)
    {
        opt_Visual_Print_Flag(SET_VERBOSE);
        print_TopoGraph();
    }
    else
    {
        opt_Visual_Print_Flag(SET_TACITMOD);
    }

    return PDU_STA_WORKING;
}
static float get_diffmean_value(const float *pool, uint8_t length)
{
    float average = 0.0f;
    for (uint8_t i = 1; i < length; i++)
    {

        average = average + (pool[i - 1] - average) / (i * 1.0f);
    }
    return average;
}
static float stable_Required_Current(float current, ID_TYPE plug_id)
{
#define FLUCTION_THRESHOLD 10.0f
#define STABLISH_THRESHOLD 50
#define RECOVERY_THRESHOLD 5
    float last_interpolated = 0.0f;
    struct Tactic_ReqCurrentObj *pdata = SETTLER_REF(plug_id);

    if (0 == plug_id)
    {
        for (int i = 0; i < DEMAND_CURRENT_SAMPLENUM; i++)
        {
            pdata->sample_current_pool[i] = current;
        }
        pdata->counter = 0;
        pdata->index = 0;
        return last_interpolated;
    }

    last_interpolated = get_diffmean_value(pdata->sample_current_pool, DEMAND_CURRENT_SAMPLENUM);

    if (fabsf(current - last_interpolated) > FLUCTION_THRESHOLD)
    {
        pdata->sample_current_pool[pdata->index++ % DEMAND_CURRENT_SAMPLENUM] = current;
        pdata->counter -= STABLISH_THRESHOLD / RECOVERY_THRESHOLD;
        pdata->counter = (pdata->counter <= 0 ? 0 : pdata->counter);
    }
    else
    {
        pdata->counter = (pdata->counter + 1 >= STABLISH_THRESHOLD ? STABLISH_THRESHOLD : pdata->counter + 1);
    }
    return (0 == pdata->counter) ? 0.0f : (STABLISH_THRESHOLD == pdata->counter ? last_interpolated : current);
}

static PDU_STA handle_pending_timeout(size_t *pending_timeout)
{
#define THRESHOLD_TIMEOUT_WORXTREAM 100

    if (++*pending_timeout > THRESHOLD_TIMEOUT_WORXTREAM)
    {
        *pending_timeout = 0;
        // undo_Last_Deed
        {
            for (NODE_OPRATION *pwrite_off = g_sheduled_worx; NULL != pwrite_off; pwrite_off = pwrite_off->next)
            {
                if (0 < pwrite_off->node_id)
                {
                    NODE_REF(pwrite_off->node_id)->discarded = true;
                    NODE_REF(pwrite_off->node_id)->chargingplug_Id = 0;
                    NODE_REF(pwrite_off->node_id)->priority = PRIOR_VAIN;
                    draw_pwrnode(pwrite_off->node_id,true);
                }
                if (0 < pwrite_off->contactor_on_id)
                {
                    CONTACTOR_REF(pwrite_off->contactor_on_id)->ia_contacted = false;
                    CONTACTOR_REF(pwrite_off->contactor_on_id)->discarded = true;
                    draw_contactor(pwrite_off->contactor_on_id, true);
                }
                if (0 < pwrite_off->contactor_off_id)
                {
                    CONTACTOR_REF(pwrite_off->contactor_off_id)->ia_contacted = true;
                    CONTACTOR_REF(pwrite_off->contactor_off_id)->discarded = true;
                    draw_contactor(pwrite_off->contactor_off_id, true);
                }
            }
        }
        return PDU_STA_FAILURE;
    }
    return PDU_STA_PENDING;
}
static PDU_STA handle_working_cmd(va_list *args)
{
    static size_t pending_timeout IN_PDU_RAM_SECTION = 0;
    if (!hear_Canaries_Twittering())
    {
        return PDU_STA_DISCARD;
    }
    if (assert_LastDeed_Undone())
    {
        return handle_pending_timeout(&pending_timeout);
    }
    pending_timeout = 0;
#define FOREACH_PLUG_ID(x) for (ID_TYPE(x) = 1; (x) <= PLUG_MAX; (x)++)

    FOREACH_PLUG_ID(plug_id)
    {
        struct Alloc_plugObj *chargee = PLUG_REF(plug_id);
        if ((bool)chargee && NEITHERFLAG != chargee->strategy_info.tag_realloc)
        {
            reallocate_Dealer(chargee);
            if (assert_NextDeed_Todo())
            {
                return PDU_STA_PENDING;
            }
        }
    }
    FOREACH_PLUG_ID(plug_id)
    {
        struct Alloc_plugObj *chargee = PLUG_REF(plug_id);
        if (PRIOR_VAIN == chargee->strategy_info.priority || NODE_VAIN == chargee->next_charger)
        {
            continue;
        }
        float current = GET_PCU_RAWDATA(plug_id, PCUDATA_OF_PLUG_DEMAND_CURRENT, float);

        current = stable_Required_Current(current, plug_id);
        if(current < FLUCTION_THRESHOLD)
        {
            continue;
        }
        if ((chargee->strategy_info.demand - current) > (MAXIM_PWR_OF_NODE * 1000.0f / 400.0 - FLUCTION_THRESHOLD))
        {
            print_oneliner("current value shift:%3.1fA",current);            
            chargee->strategy_info.demand = current;
            order_Plug(chargee);
            int32_t quota = quota_Of_Demander(plug_id, !GET_MAX_DEMAND_FLAG);
            int32_t heldsome = (int32_t)gear_num(chargee);
            quota = heldsome > quota ? heldsome - quota : 0;
            if (0 < quota)
            {
                rescinding_Dealer(chargee, quota);
                if (assert_NextDeed_Todo())
                {
                    return PDU_STA_PENDING;
                }
            }
        }
    }
    return PDU_STA_WORKING;
}
static PDU_STA handle_standby_cmd(va_list *args)
{
    return PDU_STA_STANDBY;
}

static PDU_RET FSM_pull(const StateTransition *transitions, PDU_CMD cmd, va_list args)
{
    PDU_RET ret = {.retcode = PDU_STA_UNKNOWN, .retdata = NULL};

    for (const StateTransition *p = transitions; PDU_CMD_UNKNOWN != p->current_cmd; p++)
    {
        if (p->current_cmd == cmd)
        {
            ret.retcode = p->handler(&args); // copy args to handler,avoid args list destroyed by handler
            ret.retdata = (p->worxtream ? g_sheduled_worx : NULL);
            return ret;
        }
    }

    return (PDU_RET){PDU_STA_UNKNOWN, NULL};
}

PDU_RET FSM_mainEntry_PDU(PDU_CMD cmd, ...)
{
#define WORXTREM_POST true
    static const StateTransition g_state_transition[] =
        {
            {PDU_CMD_INITIAT, !WORXTREM_POST, handle_init_cmd},
            {PDU_CMD_PLUGIN, WORXTREM_POST, handle_plugin_cmd},
            {PDU_CMD_CHARGING, WORXTREM_POST, handle_charging_cmd},
            {PDU_CMD_PLUGOUT, WORXTREM_POST, handle_plugout_cmd},
            {PDU_CMD_STANDBY, !WORXTREM_POST, handle_standby_cmd},
            {PDU_CMD_WORKING, WORXTREM_POST, handle_working_cmd},
            {PDU_CMD_VISUAL, !WORXTREM_POST, handle_visual_cmd},
            {PDU_CMD_UNKNOWN, !WORXTREM_POST, NULL},
        };
    va_list args;
    va_start(args, cmd);
    return FSM_pull(g_state_transition, cmd, args);
    va_end(args);
}