#ifndef PDUTACTIC_H
#define PDUTACTIC_H
#include "pdu_broker.h"
#include "pdu_ex_datatype.h"

#define DECIMAL(x) ((x) / 10 ? 100 : 10)
#define WEIGHT_BLANK 0
#define WEIGHT_CONTACTOR_ACT 1                                                      // 接触器动作次数权重
                                                                                    // #define WEIGHT_WORK_TIME DECIMAL(CONTACTORS_PER_NODE)              // 工作时间权重（均衡使用）
                                                                                    // #define WEIGHT_POOL_OCCUPIED DECIMAL(NODES_MAX) * WEIGHT_WORK_TIME // 功率池占用权重（均衡使用）
                                                                                    // #define WEIGHT_DEPRIV DECIMAL(NODES_PER_POOL) * WEIGHT_POOL_OCCUPIED // 空闲状态权重（优先分配空闲节点）
#define WEIGHT_HIERARCHY 10                                                         // 权重分布层级
#define WEIGHT_WORK_TIME WEIGHT_HIERARCHY                                           // 工作时间权重（均衡使用）
#define WEIGHT_POOL_OCCUPIED WEIGHT_HIERARCHY * WEIGHT_HIERARCHY * WEIGHT_WORK_TIME // 功率池占用权重（均衡使用）
#define WEIGHT_HOLDING WEIGHT_HIERARCHY *WEIGHT_POOL_OCCUPIED                       // 已占节点权重（优先分配持有节点多的枪所占据的节点）
#define WEIGHT_DEPRIV WEIGHT_HIERARCHY *WEIGHT_HOLDING                              // 可否抢占权重（优先分配空闲节点,再次分配可抢占节点）
#define WEIGHT_POLARITY 0                                                           // 铜排限流权重

#define POSTPRIMA_WEIGHT0 1
#define POSTPRIMA_WEIGHT1 (WEIGHT_HIERARCHY * WEIGHT_HIERARCHY)
#define POSTPRIMA_WEIGHT2 (WEIGHT_HIERARCHY * POSTPRIMA_WEIGHT1)
#define POSTPRIMA_WEIGHT3 (WEIGHT_HIERARCHY * POSTPRIMA_WEIGHT2)
#define POSTPRIMA_WEIGHT4 (WEIGHT_HIERARCHY * POSTPRIMA_WEIGHT3)

#define MAKE_HANDLE_NAME(x) (handle_Tactic_##x)
#define MAKE_SCOREDEED_ARRAY(x) (SCOREDEED_ARRAY_##x)
#define EVALUATION_REFER_MACRO(x) {MAKE_HANDLE_NAME(x), "handle_Tactic_" #x, MAKE_SCOREDEED_ARRAY(x)}
#define GET_PCU_RAWDATA(id, data, type) (retriver_PCU_RawData(id, data).type##_data)

#define QUOTA_FLUCT 10.0f
#define MAXIM_PWR_OF_NODE 40.0f
#define MAXIM_CURRENT_OF_NODE 100.0f
#define MINIMUM_QUOTA 1
#define PRIORITY_ASSERT(x) ((CRITERIA & (x)) > 0)

#define OUTCOMES_CLR 0, 0, 0
#define OUTCOMES_INC 1, 1, 1

#define HEIGHT_WORK_SHEDULE 6
#ifdef __OUTCOMES_GLBVAR__
NODE_OPRATION *g_sheduled_worx IN_PCU_RAM_SECTION = &(NODE_OPRATION){1, 0, 0, &(NODE_OPRATION){2, 0, 0, &(NODE_OPRATION){3, 0, 0, &(NODE_OPRATION){4, 0, 0, &(NODE_OPRATION){5, 0, 0, &(NODE_OPRATION){6, 0, 0, NULL}}}}}};
#else
extern NODE_OPRATION *g_sheduled_worx IN_PCU_RAM_SECTION;
#endif
typedef struct
{
    int score;
    ID_TYPE plug_id;
    ID_TYPE node_id;
    ID_TYPE contactor_id;

} OPTIMAL;

typedef struct
{
    int32_t weight;
    int32_t (*func)(const struct Alloc_plugObj *,const struct Alloc_nodeObj *, ID_TYPE);
    uint32_t criteria;
} RATING_RANK;

enum
{
    PREEMPT = 0, // 抢占
    SHARING,     // 分享
    RELIEVE,     // 救济
    INHERIT      // 继承
};

typedef struct
{
    NODE_OPRATION *running_worx;
    NODE_OPRATION *sheduled_worx;
} NODE_OPRATION_CTRL;

typedef struct
{
    bool (*func)(struct Alloc_plugObj *);
    char const *name;
    const RATING_RANK *rank;
} EVALUATION_REFER_TAB;
void draw_pwrnode(ID_TYPE node, bool post_scrip);
void draw_plug(ID_TYPE plug, bool post_scrip);
void draw_contactor(ID_TYPE contactor, bool post_scrip);
void order_Node(struct Alloc_nodeObj *node);
void order_Plug(struct Alloc_plugObj *plug);
void order_Contactor(ListObj *contactor, bool to_turnon);
void deorder_Contactor(ListObj *contactor);
ID_TYPE leftmost_Contactor_IN_Pool(ID_TYPE id_contactor);
ID_TYPE discover_Chargee_Demand_Routine(PLuginStatus plug_state_assert);
ID_TYPE discover_Chargee_Divorce_Routine(void);
bool gear_insert(struct Alloc_plugObj *plug, struct Alloc_nodeObj *target);
MATCHURE gear_remove(struct Alloc_plugObj *plug, struct Alloc_nodeObj *target);
uint32_t gear_num(const struct Alloc_plugObj *plug);
struct Alloc_plugObj *get_header_plug(ID_TYPE contactor_id);
ListObj *get_Cross_BtwnPlugNode(ID_TYPE node_id, ID_TYPE plug_id);
union PCU_RawData retriver_PCU_RawData(ID_TYPE id, PCURawData data_type);
static bool handle_Tactic_PREEMPT(struct Alloc_plugObj *chargee);
static bool handle_Tactic_SHARING(struct Alloc_plugObj *chargee);
static bool handle_Tactic_RELIEVE(struct Alloc_plugObj *chargee);
static bool handle_Tactic_INHERIT(struct Alloc_plugObj *chargee);
void rescinding_Dealer(struct Alloc_plugObj *chargee, int kickoff);
void insulatchk_Dealer(struct Alloc_plugObj *chargee);
void reallocate_Dealer(struct Alloc_plugObj *chargee);
void allocating_Dealer(struct Alloc_plugObj *chargee, int quota);
void outcomes_Collate_Allocation(ID_TYPE node_id, ID_TYPE contactor_on_id, ID_TYPE contactor_off_id);
int32_t quota_Of_Demander(ID_TYPE plug_id, bool bIsMax);
#endif