#ifndef PDUTACTIC_H
#define PDUTACTIC_H
#include "pdu_broker.h"

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
#define WEIGHT_COPBAR WEIGHT_HIERARCHY *WEIGHT_DEPRIV                               // 铜排限流权重
#define PASS_LINE (WEIGHT_DEPRIV * DEPRIVABLE)

#define POSTPRIMA_WEIGHT1 (WEIGHT_HIERARCHY * WEIGHT_HIERARCHY)
#define POSTPRIMA_WEIGHT2 (WEIGHT_HIERARCHY * POSTPRIMA_WEIGHT1)
#define POSTPRIMA_WEIGHT3 (WEIGHT_HIERARCHY * POSTPRIMA_WEIGHT2)
typedef struct
{
    int score;
    uint32_t plug_id;
    uint32_t node_id;
    uint32_t contactor_id;

} OPTIMAL;

typedef struct
{
    uint32_t weight;
    uint32_t (*func)(struct Alloc_plugObj *, struct Alloc_nodeObj *, uint32_t);
    uint32_t criteria;
} RATING_RANK;

void draw_pwrnode(int nodebool, bool post_scrip);
void draw_plug(int plug, bool post_scrip);
void draw_contactor(int contactor, bool post_scrip);
bool gear_insert(struct Alloc_plugObj *plug, struct Alloc_nodeObj *target);
MATCHURE gear_remove(struct Alloc_plugObj *plug, struct Alloc_nodeObj *target);
uint32_t gear_num(const struct Alloc_plugObj *plug);
void gear_clear(struct Alloc_plugObj *plug);
struct Alloc_plugObj *get_header_plug(uint32_t contactor_id);
ListObj *get_Cross_BtwnPlugNode(uint32_t node_id, uint32_t plug_id);
union PCU_RawData retriver_PCU_RawData(uint32_t id, PCURawData data_type);
#endif