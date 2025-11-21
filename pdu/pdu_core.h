#ifndef PDUCORE_H
#define PDUCORE_H



enum PDU_STA_T
{
    PDU_STA_NOREADY = 0, // PDU未就绪
    PDU_STA_DISCARD,     // PDU故障
    PDU_STA_WORKING,     // PDU运行
    PDU_STA_STANDBY,     // PDU旁路
    PDU_STA_PENDING,     // 有节点或接触器操作
    PDU_STA_FAILURE,     // 命令失败
    PDU_STA_UNKNOWN      // 未知状态
} ;

enum PDU_CMD_T
{
    PDU_CMD_INITIAT = 0, // 模块初始
    PDU_CMD_STANDBY,     // 模块旁路    
    PDU_CMD_PLUGIN,// 插枪准备绝缘检测
    PDU_CMD_CHARGING,//绝缘检测成功准备充电
    PDU_CMD_PLUGOUT,//准备拔枪
    PDU_CMD_VISUAL,   // 模块可视 
    PDU_CMD_WORKING,
    PDU_CMD_UNKNOWN
} ;

typedef enum
{
    PRIOR_VAIN = 0, // 没有置优先级=枪没有在工作
    PRIOR_BASE,     // 会被PRIOR_SVIP褫夺功率配额
    PRIOR_VIP,      // 不会被PRIOR_SVIP抢褫夺功率配额,会分摊PRIOR_BASE的功率配额
    PRIOR_SVIP,     // 会褫夺PRIOR_BASE的功率配合
    PRIOR_EXTREME,  // 会褫夺所有非PRIO_EXTREME功率配合
} PRIOR;

typedef struct node_oprt
{
    uint32_t node_id;
    uint32_t contactor_on_id;
    uint32_t contactor_off_id;
    struct node_oprt *next;
} NODE_OPRATION;

typedef struct PDU_RET_T
{
    enum PDU_STA_T retcode;
    NODE_OPRATION *retdata;
} PDU_RET;

#endif