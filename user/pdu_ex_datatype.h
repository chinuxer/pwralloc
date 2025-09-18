#ifndef _EXPORT_DATATYPE_H_
#define _EXPORT_DATATYPE_H_

typedef enum
{
    MOD_INIT = 0,   // 模块初始化
    MOD_STANDBY,    // 模块待机
    MOD_CHARGING,   // 模块充电中
    MOD_FAULT,      // 模块故障
    MOD_MAINTENANCE // 模块维护
} ModuleStatus;

typedef struct
{
    uint8_t pwrnode_id;  // 模块标识
    float max_voltage;   // 最大输出电压
    float max_current;   // 最大输出电流
    float max_power;     // 最大输出功率
    float current_power; // 当前输出功率
    float temperature;   // 模块温度
    ModuleStatus status; // 模块状态
} PowerSupply;

typedef enum
{
    PLUGIN_UNPLUGGED = 0, // 未插入
    PLUGIN_PLUGGED,       // 已插入未充电
    PLUGIN_CHARGING,      // 充电中
    PLUGIN_SUSPENDED,     // 充电暂停
    PLUGIN_FAULT          // 故障
} PLuginStatus;

typedef struct
{
    uint8_t plugin_id;   // 充电枪标识
    float voltage_req;   // 电压需求
    float current_req;   // 电流需求
    float power_req;     // 功率需求
    uint8_t priority;    // 优先级
    PLuginStatus status; // 充电枪状态
} PowerDemand;

typedef struct
{
    uint8_t contactor_id; // 充电枪标识
    bool onoff;
} Contactor;
#endif // !1