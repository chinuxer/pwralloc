#ifndef PDUBROKER_H
#define PDUBROKER_H
#include "stm32f4xx.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdalign.h>
#include <stdarg.h>
#include <ctype.h>
#include "inttypes.h"
#include "math.h"
#include <float.h>
#include <limits.h>
#include "pdu_core.h"
typedef uint32_t ID_TYPE;
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
    CRITERION_PRIOR = 0b00000001,              // 车优先级
    CRITERION_MIN_COST = 0b00000010,           // 经济运行
    CRITERION_MAX_POWER = 0b00000100,          // 最大需求
    CRITERION_EQU_SANITY = 0b00001000,         // 均衡健康度
    CRITERION_LMT_CAPACITY = 0b00010000,       // 单铜排限电流容量
    CRITERION_MAX_EFFICIENCY = 0b00100000,     // 模块效率
    CRITERION_PEERS_FAIRNOFAVOR = 0b01000000,  // 同优先级内模块均分
    CRITERION_PEERS_EARLYPREFER = 0b10000000,  // 同优先级内先到先得
    CRITERION_ALLOWANCE_MINIMUM = 0b100000000, // 插枪后必须获取最低节点供应
} CRITERION;

typedef enum
{
    PCUDATA_OF_PLUG = 0x00100u,
    PCUDATA_OF_PLUG_DEMAND_CURRENT,
    PCUDATA_OF_PLUG_MAXIMUM_CURRENT,
    PCUDATA_OF_PLUG_STATUS,
    PCUDATA_OF_PLUG_PRIORITY,
    PCUDATA_OF_PLUG_VOLTAGE,
    PCUDATA_OF_NODE = 0x10000u,
    PCUDATA_OF_NODE_WORKHOURS,
    PCUDATA_OF_NODE_CURRENT,
    PCUDATA_OF_CONTACTORS = 0x1000000u,

} PCURawData;

union PCU_RawData
{
    int32_t int32_data;
    uint32_t uint32_data;
    float float_data;
};

typedef enum
{
    SET_VERBOSE = 0,
    SET_TACITMOD,
    GET_STATUS,
} VISUAL_PRINT_OPRT;

typedef enum
{
    NEITHERFLAG = 0,
    REALLOCATOR = 0xA0,
    REALLOCATEE,
    SUPRASWITCH,
    UNAVAILABLE = 0xF0,
    MALFUNCTION,
    CIRCUMSTANT
} PLUG_TACTIC_FLAG;

/*
******************************************************************************
* Definitely Needed Macros
******************************************************************************/
// Declare the related symbols exported from ICF
extern const unsigned char *const __PDU_CORE_RAM_start__;
extern const unsigned char *const __PDU_CORE_RAM_end__;
extern const unsigned char *const __PDU_CORE_HEAP_start__;
extern const unsigned char *const __PDU_CORE_HEAP_end__;
extern const unsigned int __PDU_CORE_RAM_size__;
// cstat !MISRAC2012-Rule-8.9_b
extern const unsigned int __PDU_CORE_HEAP_size__;

#define CYPHER (__COUNTER__)

#define IN_PDU_RAM_SECTION __attribute__((section(".pdu_ram_section"), aligned(4)))
#define IN_PCU_RAM_SECTION
#define IN_PDU_HEAP_SECTION __attribute__((section(".pdu_heap_section")))
#define RAM_CAPACITY (__PDU_CORE_RAM_size__ / sizeof(ptrdiff_t))

#define FRONT_MAGICWORD 0xDEADCAFEu
#define REAR_MAGICWORD 0xBABEFACEu
// in case of VLA enabled in IAR within C99 standard
#define VLA_INSTANT(type, name, length) \
    type name[(length)];                \
    memset(name, 0, sizeof(type) * (length));

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
#define NODE_OF_CONTACTOR(x) ((ID_TYPE)(((x) - 1) / CONTACTORS_PER_NODE + 1))
#define POOL_OF_CONTACTOR(x) ((ID_TYPE)(((x) - 1) / CONTACTORS_PER_NODE / NODES_PER_POOL + 1))
#define COPPERBAR_OF_CONTACTOR(x) ((ID_TYPE)((POOL_OF_CONTACTOR(x) - 1) * CONTACTORS_PER_NODE + (x) % (CONTACTORS_PER_NODE + 1)))
#define REF(pointer, seq) ((pointer)->obj_array + ((seq) - 1 < (pointer)->length ? (seq) - 1 : 0))
#define NODE_REF(node) (REF(gpNodesArray, node))
#define PLUG_REF(plug) (REF(gpPlugsArray, plug))
#define CONTACTOR_REF(knob) (REF(gpContactorsArray, knob))
#define COPBAR_REF(bar) (REF(gpCopBarArray, bar))
#define POOL_REF(pool) (REF(gpPoolArray, pool))
#define SETTLER_REF(plug) (REF(gpReqSettlerArray, plug))
#define PLUG_LINKER_REF_INCOGNITA(pointer, plug) ((ListObj *)&REF(pointer, plug)->copula)
#define PLUG_LINKER_REF_NONYM(plug) ((ListObj *)&PLUG_REF(plug)->copula)
#define CONTACTOR_LINKER_REF_INCOGNITA(pointer, knob) ((ListObj *)REF(pointer, knob))
#define CONTACTOR_LINKER_REF_NONYM(knob) ((ListObj *)CONTACTOR_REF(knob))
#define MACRO_PICKOUT(ARG1ST, ARG2ND, PICKED, ...) PICKED
#define PLUG_LINKER_REF(...) MACRO_PICKOUT(__VA_ARGS__, PLUG_LINKER_REF_INCOGNITA, PLUG_LINKER_REF_NONYM)(__VA_ARGS__)
#define CONTACTOR_LINKER_REF(...) MACRO_PICKOUT(__VA_ARGS__, CONTACTOR_LINKER_REF_INCOGNITA, CONTACTOR_LINKER_REF_NONYM)(__VA_ARGS__)

#define ID_OF(x) ((x)->id)
#define NODEREF_FROM_CONTACTOR(knob) (NODE_REF(((knob) - 1) / CONTACTORS_PER_NODE % NODE_MAX + 1))
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

#define CRITERIA_ASSIGNMENT ((size_t)CRITERION_EQU_SANITY | (size_t)CRITERION_PRIOR | (size_t)CRITERION_PEERS_FAIRNOFAVOR | (size_t)CRITERION_ALLOWANCE_MINIMUM)
#define DEMAND_CURRENT_SAMPLENUM 6
/*******************************************************************************
 * Data Structures
 ******************************************************************************/
typedef struct
{
    float demand;
    float longing;
    float value_Vset;
    uint32_t workhours_topnode;
    uint32_t workhours_bottomnode;
    int32_t shortage_demand;
    uint32_t soc;
    uint32_t duration;
    PLUG_TACTIC_FLAG tag_realloc;
    PRIOR priority;
} STRAGTEGY_BASIS;
typedef struct Proto_listObj
{
    struct Proto_listObj *next_linker;
    struct Proto_listObj *prev_linker;
    ID_TYPE id;
    bool ia_contacted;
    bool discarded;
} ListObj, *ListRef;

struct Alloc_nodeObj
{
    struct Alloc_nodeObj *next_charger;
    ID_TYPE id;
    float value_Iset;
    float value_Vset;
    uint32_t chargingplug_Id;
    PRIOR priority;
    bool discarded;
};
struct Alloc_plugObj
{
    struct Alloc_nodeObj *next_charger;
    ListObj copula;
    ID_TYPE id;
    STRAGTEGY_BASIS strategy_info;
};
struct Tactic_CopBarObj
{
    ID_TYPE id;
    const float current_max;
};

struct Tactic_PwrPoolObj
{
    ID_TYPE id;
    uint32_t nodes_worked;
};

struct Tactic_ReqCurrentObj
{
    uint32_t index;
    int32_t counter;
    uint32_t last_stand_value;
    float sample_current_pool[DEMAND_CURRENT_SAMPLENUM];
};
typedef struct
{
    uint32_t front_canary; // absolutely required to be top element
    size_t length;
    size_t pools_num;
    size_t forks_num;
    struct Alloc_nodeObj obj_array[];
} Alloc_NodesArray;

typedef struct
{
    uint32_t front_canary; // absolutely required to be top element
    size_t length;
    size_t forks_num;
    ListObj obj_array[];
} Alloc_ContactorsArray;

typedef struct
{
    uint32_t front_canary; // absolutely required to be top element
    size_t length;
    size_t criteria;
    struct Alloc_plugObj obj_array[];
} Alloc_PlugsArray;

typedef struct
{
    uint32_t front_canary; // absolutely required to be top element
    size_t length;

    struct Tactic_CopBarObj obj_array[];
} Alloc_CopBarArray;

typedef struct
{
    uint32_t front_canary; // absolutely required to be top element
    size_t length;

    struct Tactic_PwrPoolObj obj_array[];
} Alloc_PoolArray;

typedef struct
{
    uint32_t front_canary; // absolutely required to be top element
    size_t length;

    struct Tactic_ReqCurrentObj obj_array[];
} Alloc_ReqSettlerArray;
#define offset_Malloc_Addr(x) (__PDU_CORE_RAM_start__ + sizeof(ptrdiff_t) * (x))

#define VARIABLE_LIST_PENDING_EXPANDED        \
    CONTEXT_EXPANDER_PROJ_GLOBAL(Nodes);      \
    CONTEXT_EXPANDER_PROJ_GLOBAL(Plugs);      \
    CONTEXT_EXPANDER_PROJ_GLOBAL(Contactors); \
    CONTEXT_EXPANDER_PROJ_GLOBAL(CopBar);     \
    CONTEXT_EXPANDER_PROJ_GLOBAL(Pool);       \
    CONTEXT_EXPANDER_PROJ_GLOBAL(ReqSettler);
// cstat -DEFINE-hash-multiple
#define VAR_NAME(x) gp##x##Array
#define VAR_TYPE(x) Alloc_##x##Array
// cstat +DEFINE-hash-multiple
#ifdef __IMPORT_GLOBALVAR__
#define CONTEXT_EXPANDER_PROJ_GLOBAL(x) VAR_TYPE(x) * VAR_NAME(x) IN_PDU_RAM_SECTION = NULL
VARIABLE_LIST_PENDING_EXPANDED
#undef CONTEXT_EXPANDER_PROJ_GLOBAL
#else
#define CONTEXT_EXPANDER_PROJ_GLOBAL(x) extern VAR_TYPE(x) * VAR_NAME(x)
VARIABLE_LIST_PENDING_EXPANDED
#undef CONTEXT_EXPANDER_PROJ_GLOBAL
#endif // __IMPORT_GLOBALVAR__
#ifdef __IMPORT_PDU_MALLOC__

static void *pdu_mem_alloc(size_t size, void *ptr)
{
#define ALIGN_SIZE(s) (((s) + 3u) & ~3u)
    static uint8_t custom_mem_pool[CUSTOM_HEAP_SIZE] IN_PDU_HEAP_SECTION = {0};
    static size_t custom_mem_offset IN_PDU_RAM_SECTION = 0;
    static void *last_alloc_ptr IN_PDU_RAM_SECTION = NULL; // record last allocation pointer
    static size_t last_alloc_size IN_PDU_RAM_SECTION = 0;  //...size

    // reset default
    if (0 == size && ptr == NULL)
    {
        custom_mem_offset = 0;
        last_alloc_ptr = (void*)custom_mem_pool;
        last_alloc_size = 0;
        return NULL;
    }

    // allign!! _(:з」∠)_
    size = ALIGN_SIZE(size);

    // REALLOC branch
    if (ptr != NULL)
    {
        // verify last pointer
        if (ptr != last_alloc_ptr)
        {
            return NULL;
        }

        if (size == last_alloc_size)
        {
            return ptr;
        }
        else if (size < last_alloc_size)
        {
            // shrink
            size_t shrink_size = last_alloc_size - size;
            custom_mem_offset -= shrink_size;
            last_alloc_size = size;
            return ptr;
        }
        else
        {
            // expand
            size_t expand_size = size - last_alloc_size;
            if (custom_mem_offset + expand_size > CUSTOM_HEAP_SIZE)
            {
                return NULL;
            }
            custom_mem_offset += expand_size;
            memset((uint8_t *)ptr + last_alloc_size, 0, expand_size);
            last_alloc_size = size;
            return ptr;
        }
    }
    // CALLOC branch
    else
    {

        if (custom_mem_offset + size > CUSTOM_HEAP_SIZE)
        {
            return NULL;
        }

        void *new_ptr = (void *)(custom_mem_pool + custom_mem_offset);
        custom_mem_offset += size;
        memset(new_ptr, 0, size);

        last_alloc_ptr = new_ptr;
        last_alloc_size = size;
        return new_ptr;
    }
}
void *pdu_calloc(size_t size)
{
    return pdu_mem_alloc(size, NULL); // ptr=NULL --> calloc
}

static void *pdu_realloc(void *ptr, size_t size)
{
    return pdu_mem_alloc(size, ptr); // ptr≠NULL --> realloc
}
#endif // pdu_malloc
uint32_t list_len_safe(const ListObj *list, uint32_t max);
// cstat !CERT-EXP37-C_b
// cstat !MISRAC2012-Rule-8.3
// cstat !CERT-DCL40-C
void print_oneliner(const char *format, ...);

typedef enum PDU_STA_T PDU_STA;
typedef enum PDU_CMD_T PDU_CMD;
#endif // PDUBROKER_H