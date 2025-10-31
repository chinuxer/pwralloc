#ifndef PDUBROKER_H
#define PDUBROKER_H
#include "stm32f4xx.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdatomic.h>
#include <stdarg.h>
#include <ctype.h>
#include "inttypes.h"
#include "math.h"

typedef enum
{
    FUTILE = -1,
    EUREKA,
} MATCHURE;

typedef enum
{
    FAIL = -1,
    DONE,
} RESULTANT;

typedef enum
{
    CRITERION_NONE = 0,
    CRITERION_PRIOR = 0b00000001,             // 车优先级
    CRITERION_MIN_COST = 0b00000010,          // 经济运行
    CRITERION_MAX_POWER = 0b00000100,         // 最大需求
    CRITERION_EQU_SANITY = 0b00001000,        // 均衡健康度
    CRITERION_LMT_CAPACITY = 0b00010000,      // 单铜排限电流容量
    CRITERION_MAX_EFFICIENCY = 0b00100000,    // 模块效率
    CRITERION_PRIOR_FAIRNOFAVOR = 0b01000000, // 同优先级内模块均分
    CRITERION_PRIOR_EARLYPREFER = 0b10000000, // 同优先级内先到先得
} CRITERION;
typedef enum
{
    PRIOR_VAIN = 0, // 没有置优先级=枪没有在工作
    PRIOR_BASE,     // 会被PRIOR_SVIP褫夺功率配额
    PRIOR_VIP,      // 不会被PRIOR_SVIP抢褫夺功率配额,会分摊PRIOR_BASE的功率配额
    PRIOR_SVIP,     // 会褫夺PRIOR_BASE的功率配合
    PRIOR_EXTREME,  // 会褫夺所有非PRIO_EXTREME功率配合
} PRIOR;

typedef enum
{
    PCUDATA_OF_PLUG = 0x00100,
    PCUDATA_OF_NODE = 0x10000,
    PCUDATA_OF_NODE_WORKHOURS,
    PCUDATA_OF_CONTACTORS = 0x1000000,

} PCURawData;

union PCU_RawData
{
    int32_t int32_data;
    float float_data;
};
/*
******************************************************************************
* Definitely Needed Macros
******************************************************************************/
// Declare the related symbols exported from ICF
extern uint32_t __PDU_CORE_RAM_start__;
extern uint32_t __PDU_CORE_RAM_end__;
extern uint32_t __PDU_CORE_RAM_size__;
extern uint32_t __PDU_CORE_HEAP_start__;
extern uint32_t __PDU_CORE_HEAP_end__;
extern uint32_t __PDU_CORE_HEAP_size__;

#define CYPHER (__COUNTER__)
#define NODEARRAY_ATTRIBUTE (0x2000000 + sizeof(ptrdiff_t) * 1)
#define CONTACTORARRAY_ATTRIBUTE (0x2000000 + sizeof(ptrdiff_t) * 2)
#define PLUGARRAY_ATTRIBUTE (0x2000000 + sizeof(ptrdiff_t) * 3)

#define IN_PDU_RAM_SECTION __attribute__((section(".pdu_ram_section"), aligned(4)))
#define IN_PDU_HEAP_SECTION __attribute__((section(".pdu_heap_section")))
#define RAM_CAPACITY (__PDU_CORE_RAM_size__ / sizeof(ptrdiff_t))

#define FRONT_MAGICWORD 0xDEADCAFE
#define REAR_MAGICWORD 0xBABEFACE

#define VLA_INSTANT(type, name, length) \
    type name[length];                  \
    memset(name, 0, sizeof(name));

#define NODE_VAIN ((struct Alloc_nodeObj *)(intptr_t)(-1))
#define patriptr(PTR, TYPE, MEMBER) ((TYPE *)((char *)PTR - offsetof(TYPE, MEMBER)))
#define LIST_HEAD_INIT(name) {&(name), &(name)}
#define LIST_HEAD(name) ListObj name = LIST_HEAD_INIT(name)
#define list_entry(node, type, member) \
    patriptr(node, type, member)

#define PLUG_MAX (gpPlugsArray->length)
#define NODE_MAX (gpNodesArray->length)
#define CONTACTOR_MAX (gpContactorsArray->length)
#define NODES_PER_POOL (gpNodesArray->forks_num)
#define CONTACTORS_PER_NODE (gpContactorsArray->forks_num)
#define CRITERIA (gpPlugsArray->criteria)
#define POOL_MAX (gpNodesArray->pools_num)
#define NODE_OF_CONTACTOR(x) (((x) - 1) / CONTACTORS_PER_NODE + 1)
#define POOL_OF_CONTACTOR(x) (((x) - 1) / CONTACTORS_PER_NODE / NODES_PER_POOL + 1)
#define COPPERBAR_OF_CONTACTOR(x) (((x) - 1) / NODES_PER_POOL + 1)
#define REF(pointer, seq) ((pointer)->obj_array + ((seq) - 1 < pointer->length ? seq - 1 : 0))
#define NODE_REF(node) (REF(gpNodesArray, node))
#define PLUG_REF(plug) (REF(gpPlugsArray, plug))
#define CONTACTOR_REF(knob) (REF(gpContactorsArray, knob))
#define COPBAR_REF(bar) (REF(gpCopBarArray, bar))
#define POOL_REF(pool) (REF(gpPoolArray, pool))
#define PLUG_LINKER_REF_INCOGNITA(pointer, plug) ((ListObj *)&REF(pointer, plug)->copula)
#define PLUG_LINKER_REF_NONYM(plug) ((ListObj *)&PLUG_REF(plug)->copula)
#define CONTACTOR_LINKER_REF_INCOGNITA(pointer, knob) ((ListObj *)REF(pointer, knob))
#define CONTACTOR_LINKER_REF_NONYM(knob) ((ListObj *)CONTACTOR_REF(knob))
#define MACRO_PICKOUT(ARG1ST, ARG2ND, PICKED, ...) PICKED
#define PLUG_LINKER_REF(...) MACRO_PICKOUT(__VA_ARGS__, PLUG_LINKER_REF_INCOGNITA, PLUG_LINKER_REF_NONYM)(__VA_ARGS__)
#define CONTACTOR_LINKER_REF(...) MACRO_PICKOUT(__VA_ARGS__, CONTACTOR_LINKER_REF_INCOGNITA, CONTACTOR_LINKER_REF_NONYM)(__VA_ARGS__)

#define ID_OF(x) ((x)->id)
#define NODEREF_FROM_CONTACTOR(knob) (NODE_REF((knob - 1) / CONTACTORS_PER_NODE % NODE_MAX + 1))
#define POOL_OF_NODE(node) ((ID_OF(NODE_REF(node)) - 1) / NODES_PER_POOL % POOL_MAX + 1)
#define gear_for_each(pos, head) \
    for (pos = (head)->next_charger; pos != NODE_VAIN; pos = pos->next_charger)
#define list_for_each(pos, head) \
    for (pos = (head)->next_linker; pos != (head); pos = pos->next_linker)

/*
 *  list_for_each_safe - iterate over a list safe against removal of list entry
 * @pos:    the &struct list_head to use as a loop cursor.
 * @n:      another &struct list_head to use as temporary storage
 * @head:   the head for your list.
 */
#define list_for_each_safe(pos, n, head)                                 \
    for (pos = (head)->next_linker, n = pos->next_linker; pos != (head); \
         pos = n, n = pos->next_linker)

/*******************************************************************************
 * Data Structures
 ******************************************************************************/
typedef struct
{
    PRIOR priority;
    float demand;
    uint32_t workhours_topnode; //
    uint32_t workhours_bottomnode;
    uint32_t shortage_demand;

} STRAGTEGY_BASIS;
typedef struct Proto_listObj
{
    uint32_t id;
    bool ia_contacted;
    struct Proto_listObj *next_linker;
    struct Proto_listObj *prev_linker;
} ListObj, *ListRef;

struct Alloc_nodeObj
{
    uint32_t id;
    float value_Iset;
    float value_Vset;
    PRIOR priority;

    uint32_t chargingplug_Id;
    struct Alloc_nodeObj *next_charger;
    //...
};
struct Alloc_plugObj
{
    uint32_t id;
    STRAGTEGY_BASIS strategy_info;
    //...
    struct Alloc_nodeObj *next_charger;
    ListObj copula;
};
struct Tactic_CopBarObj
{
    uint32_t id;
    float current_max;
    struct Tactic_CopBarObj *next_bar;
};

struct Tactic_PwrPoolObj
{
    uint32_t id;
    uint32_t plugs_linked;
    uint32_t nodes_worked;
};
typedef struct
{
    uint32_t front_canary; // absolutely required to be top element
    size_t length;
    size_t pools_num;
    size_t forks_num;
    struct Alloc_nodeObj obj_array[];
} Alloc_nodeArray;

typedef struct
{
    uint32_t front_canary; // absolutely required to be top element
    size_t length;
    size_t forks_num;
    ListObj obj_array[];
} Alloc_contactorArray;

typedef struct
{
    uint32_t front_canary; // absolutely required to be top element
    size_t length;
    size_t criteria;
    struct Alloc_plugObj obj_array[];
} Alloc_plugArray;

typedef struct
{
    uint32_t front_canary; // absolutely required to be top element
    size_t length;

    struct Tactic_CopBarObj obj_array[];
} Tactic_copbarArray;

typedef struct
{
    uint32_t front_canary; // absolutely required to be top element
    size_t length;

    struct Tactic_PwrPoolObj obj_array[];
} Tactic_pwrpoolArray;
#define offset_Malloc_Addr (__PDU_CORE_RAM_start__ + sizeof(ptrdiff_t) * 4)

#ifdef __IMPORT_GLOBALVAR__
// refrence to instant of contactor,plugin and pwrnode object
// Alloc_nodeArray *gpNodesArray @NODEARRAY_ATTRIBUTE = NULL;
// Alloc_plugArray *gpPlugsArray @PLUGARRAY_ATTRIBUTE = NULL;
// Alloc_contactorArray *gpContactorsArray @CONTACTORARRAY_ATTRIBUTE = NULL;

Alloc_nodeArray *gpNodesArray IN_PDU_RAM_SECTION = NULL;
Alloc_plugArray *gpPlugsArray IN_PDU_RAM_SECTION = NULL;
Alloc_contactorArray *gpContactorsArray IN_PDU_RAM_SECTION = NULL;
Tactic_copbarArray *gpCopBarArray IN_PDU_RAM_SECTION = NULL;
Tactic_pwrpoolArray *gpPoolArray IN_PDU_RAM_SECTION = NULL;
#else
extern Alloc_nodeArray *gpNodesArray;
extern Alloc_plugArray *gpPlugsArray;
extern Alloc_contactorArray *gpContactorsArray;
extern Tactic_copbarArray *gpCopBarArray;
extern Tactic_pwrpoolArray *gpPoolArray;
#endif // __IMPORT_GLOBALVAR__
#ifdef __IMPORT_PDU_MALLOC__
#define CUSTOM_MEM_POOL_SIZE 4 * 1024
static uint8_t custom_mem_pool[CUSTOM_MEM_POOL_SIZE] IN_PDU_HEAP_SECTION = {0};
void *pdu_calloc(size_t size)
{
    static size_t custom_mem_offset IN_PDU_RAM_SECTION = 0;
    if (!size)
    {
        custom_mem_offset = 0;
        return NULL;
    }

    size = (size + 3) & ~3; // allign

    if (custom_mem_offset + size > CUSTOM_MEM_POOL_SIZE)
    {
        return NULL;
    }

    void *ptr = (void *)(custom_mem_pool + custom_mem_offset);

    custom_mem_offset += size;
    memset(ptr, 0, size);

    return ptr;
}
#endif // pdu_malloc
uint32_t list_len_safe(const ListObj *list, uint32_t max);
void print_oneliner(const char *format, ...);
typedef enum
{
    UNAVAILABLE = 0,
    MALFUNCTION,
    CIRCUMSTANT
} PDU_DATA_STATE;
typedef enum
{
    PDU_STA_NOREADY = 0, // PDU未就绪

    PDU_STA_DISCARD, // PDU故障
    PDU_STA_WORKING, // PDU运行
    PDU_STA_STANDBY, // PDU旁路
    PDU_STA_TOPOGRAPH,
    PDU_STA_UNKNOWN
} PDU_STA;
typedef enum
{
    PDU_CMD_INITIAT = 0, // 模块初始
    PDU_CMD_STANDBY,     // 模块旁路
    PDU_CMD_WORKON,      // 模块工作
    PDU_CMD_VISUAL,      // 模块可视
    PDU_CMD_UNKNOWN
} PDU_CMD;

#endif // PDUBROKER_H