
#define __IMPORT_PDU_MALLOC__
#define __IMPORT_GLOBALVAR__

#include "pdu_broker.h"

#define __INSTANT__PARAM__
#include "pdu_param.h"

void list_init(ListObj *const list)
{
    list->next_linker = list->prev_linker = list;
}

void list_insert_after(ListObj *const list, ListObj *const node)
{
    list->next_linker->prev_linker = node;
    node->next_linker = list->next_linker;

    list->next_linker = node;
    node->prev_linker = list;
}

void list_insert_before(ListObj *const list, ListObj *const node)
{
    list->prev_linker->next_linker = node;
    node->prev_linker = list->prev_linker;

    list->prev_linker = node;
    node->next_linker = list;
}

void list_remove(ListObj *const node)
{
    node->next_linker->prev_linker = node->prev_linker;
    node->prev_linker->next_linker = node->next_linker;

    node->next_linker = node->prev_linker = node;
}

bool list_isempty(const ListObj *list)
{
    return list->next_linker == list;
}

uint32_t list_len(const ListObj *list)
{
    uint32_t len = 0;
    const ListObj *p = list;
    while (p->next_linker != list)
    {
        p = p->next_linker;
        len++;
    }

    return len;
}
uint32_t list_len_safe(const ListObj *list, uint32_t max)
{
    uint32_t len = 0;
    const ListObj *p = list;

    while (p->next_linker != list)
    {
        p = p->next_linker;

        if (++len >= max)
        {
            break;
        }
    }

    return len;
}

void gear_clear(struct Alloc_plugObj *plug)
{
    struct Alloc_nodeObj *p = plug->next_charger;
    struct Alloc_nodeObj *next; // Temporary pointer to hold next node

    while (NODE_VAIN != p && p != NULL)
    {
        p->chargingplug_Id = 0;
        p->priority = PRIOR_VAIN;

        next = p->next_charger; // Save the next pointer before overwriting
        p->next_charger = NULL; // Set current node's next_charger to NULL

        p = next; // Move to next node

        // Optional: keep a safety check to avoid infinite loops
        static uint32_t num = 0;
        if (++num > NODE_MAX)
        {
            num = 0; // Reset counter
            break;
        }
    }

    plug->next_charger = NODE_VAIN;
    return;
}
uint32_t gear_num(const struct Alloc_plugObj *plug)
{
    const struct Alloc_nodeObj *p = plug->next_charger;
    uint32_t num = (NODE_VAIN != p);
    while (NODE_VAIN != p->next_charger && NODE_VAIN != p)
    {
        p = p->next_charger;

        if (++num > NODE_MAX)
        {
            break;
        }
    }
    return num;
}
bool gear_insert(struct Alloc_plugObj *plug, struct Alloc_nodeObj *target)
{
    if (!plug || !target || NULL != target->next_charger) // or target is already in a plug charging chain
    {
        return false;
    }

    target->chargingplug_Id = plug->id;
    target->priority = plug->strategy_info.priority;
    target->next_charger = NODE_VAIN;
    struct Alloc_nodeObj **current = &plug->next_charger; // Double pointer: pointer to the "head pointer"
    int cnt = 1;
    while (*current != NODE_VAIN)
    {
        current = &(*current)->next_charger;
        if (++cnt > NODE_MAX)
        {
            return false;
        }
    }
    *current = target; // Assign target to the found NULL pointer position
    return true;
}

MATCHURE gear_remove(struct Alloc_plugObj *plug, struct Alloc_nodeObj *target)
{
    if (!plug || !target)
    {
        return FUTILE;
    }

    // Use double pointer to traverse the list and find the target node
    struct Alloc_nodeObj **current = &plug->next_charger;
    int cnt = 1;
    while (*current != NODE_VAIN)
    {
        if (*current == target)
        {
            // Found the target node, remove it from the chain
            *current = target->next_charger;
            target->next_charger = NULL; // Clear the target's next pointer,even not a NODE_VAIN
            target->chargingplug_Id = 0;
            target->priority = PRIOR_VAIN;
            return EUREKA;
        }
        current = &(*current)->next_charger;
        if (cnt++ > NODE_MAX)
        {
            break;
        }
    }

    return FUTILE;
}

ListObj *get_Cross_BtwnPlugNode(uint32_t node_id, uint32_t plug_id)
{
    struct Alloc_plugObj *plug = PLUG_REF(plug_id);
    struct Alloc_nodeObj *node = NODE_REF(node_id);
    ListObj *pos;
    list_for_each(pos, &plug->copula)
    {
        if (NODEREF_FROM_CONTACTOR(ID_OF(pos)) == node)
        {
            return (pos);
        }
    }
}
struct Alloc_plugObj *get_header_plug(uint32_t contactor_id)
{
    if (contactor_id < 1 || contactor_id > CONTACTOR_MAX)
    {
        return NULL;
    }
    const ListObj *list = CONTACTOR_LINKER_REF(contactor_id);
    const ListObj *pos;
    list_for_each(pos, list)
    {
        if (ID_OF(pos) == *(uint32_t *)(char[]){"PLUG"})
        {
            return list_entry(pos, struct Alloc_plugObj, copula);
        }
    }
    return NULL;
}

#define CREATE_FLEXSTRUCT_ARRAY(type, count) \
    (type *)create_FlexStruct_Array(sizeof(type), sizeof(((type *)0)->obj_array[0]), count, type##_Init)

#define IS_FRONT_CANARY_INTACT(ptr, type) \
    ((ptr) ? (((const type *)(ptr))->front_canary == (FRONT_MAGICWORD)) : false)
#define GET_REAR_CANARY_PTR(ptr, type) \
    ((uint32_t *)((char *)(ptr) + sizeof(type) + (ptr)->length * sizeof(((type *)0)->obj_array[0])))
#define IS_REAR_CANARY_INTACT(ptr, type) \
    ((ptr) ? (*GET_REAR_CANARY_PTR(ptr, type) == (REAR_MAGICWORD)) : false)

void Alloc_nodeArray_Init(void *const ptr, size_t n)
{
    if (!ptr)
    {
        return;
    }
    Alloc_nodeArray *p = (Alloc_nodeArray *)ptr;
    p->length = n;
    p->front_canary = FRONT_MAGICWORD;
    *GET_REAR_CANARY_PTR(p, Alloc_nodeArray) = REAR_MAGICWORD;
    for (int i = 1; i <= p->length; i++)
    {
        ID_OF(REF(p, i)) = i;
        REF(p, i)->next_charger = NULL;
    }
}
void Alloc_plugArray_Init(void *const ptr, size_t n)
{
    if (!ptr)
    {
        return;
    }
    Alloc_plugArray *p = (Alloc_plugArray *)ptr;
    p->length = n;
    p->front_canary = FRONT_MAGICWORD;
    p->criteria = CRITERION_EQU_SANITY | CRITERION_LMT_CAPACITY | CRITERION_PRIOR | CRITERION_PRIOR_FAIRNOFAVOR;
    *GET_REAR_CANARY_PTR(p, Alloc_plugArray) = REAR_MAGICWORD;
    for (int i = 1; i <= p->length; i++)
    {
        list_init(PLUG_LINKER_REF(p, i));
        ID_OF(REF(p, i)) = i;
        ID_OF(PLUG_LINKER_REF(p, i)) = *(uint32_t *)(char[]){"PLUG"};
        REF(p, i)->next_charger = NODE_VAIN;
    }
}
void Alloc_contactorArray_Init(void *const ptr, size_t n)
{
    if (!ptr)
    {
        return;
    }
    Alloc_contactorArray *p = (Alloc_contactorArray *)ptr;
    p->length = n;
    p->front_canary = FRONT_MAGICWORD;
    *GET_REAR_CANARY_PTR(p, Alloc_contactorArray) = REAR_MAGICWORD;
    for (int i = 1; i <= p->length; i++)
    {
        ID_OF(REF(p, i)) = i;
        list_init(CONTACTOR_LINKER_REF(p, i));
    }
}

void Tactic_copbarArray_Init(void *const ptr, size_t n)
{
    if (!ptr)
    {
        return;
    }
    Tactic_copbarArray *p = (Tactic_copbarArray *)ptr;
    p->length = n;
    p->front_canary = FRONT_MAGICWORD;
    *GET_REAR_CANARY_PTR(p, Alloc_contactorArray) = REAR_MAGICWORD;
    for (int i = 1; i <= p->length; i++)
    {
        ID_OF(REF(p, i)) = i;
    }
}
void Tactic_pwrpoolArray_Init(void *const ptr, size_t n)
{
    if (!ptr)
    {
        return;
    }
    Tactic_pwrpoolArray *p = (Tactic_pwrpoolArray *)ptr;
    p->length = n;
    p->front_canary = FRONT_MAGICWORD;
    *GET_REAR_CANARY_PTR(p, Alloc_contactorArray) = REAR_MAGICWORD;
    for (int i = 1; i <= p->length; i++)
    {
        ID_OF(REF(p, i)) = i;
    }
}
void *create_FlexStruct_Array(size_t header_size, size_t element_size, int count, void (*init_func)(void *, size_t))
{
    // Validate parameters
    if (count <= 0 || 0 == header_size || 0 == element_size)
    {
        return NULL;
    }

    // Calculate total required memory
    size_t total_size = sizeof(uint32_t) + header_size + count * element_size;

    // Allocate and zero-initialize memory
    void *ptr = pdu_calloc(total_size);
    if (ptr == NULL)
    {
        printf("!!!flexible array mem allocation failed.\n");
        return NULL;
    }
#pragma calls = Alloc_plugArray_Init, Alloc_nodeArray_Init, Alloc_contactorArray_Init, Tactic_copbarArray_Init, Tactic_pwrpoolArray_Init
    init_func(ptr, count);
    return ptr;
}
#if 0
static inline void find_plug_join_(const Verbose_TextPool *textpool, size_t combo_max, Combo_Koinon *combo_array)
{
    for (int i = 1; i <= PLUG_MAX; i++)
    {
        scan_KeyValue_of_PluginJoined(combo_array, combo_max, textpool, i);
    }
}
#endif
static inline void find_plug_join(const KeyValue_Array *array, uint32_t max, Combo_Koinon *valuesArray)
{
    for (int i = 1; i <= max; i++)
    {
        scan_KeyValue(valuesArray, array, i);
    }
}
static inline uint32_t top_within_col_of(uint32_t contactor_id)
{
    contactor_id--;
    return ((contactor_id / CONTACTORS_PER_NODE / NODES_PER_POOL + 1) * (NODES_PER_POOL * CONTACTORS_PER_NODE) - CONTACTORS_PER_NODE + contactor_id % CONTACTORS_PER_NODE + 1);
}
static bool list_plug_join(const Combo_Koinon *array, uint32_t max)
{
    if (!array)
    {
        return false;
    }
    for (uint8_t i = 0; i < max; i++)
    {
        if (0 == array[i].id_of.plug || 0 == array[i].id_of.contactor)
        {
            continue;
        }
        if (array[i].id_of.plug > max)
        {
            printf("!!!plugjoin_array[%d] is out of range.\n", i);
            return false;
        }

        uint32_t top = top_within_col_of(array[i].id_of.contactor);

        list_insert_after(PLUG_LINKER_REF(gpPlugsArray, array[i].id_of.plug), CONTACTOR_LINKER_REF(top));
        for (uint8_t row = 0; row < (NODES_PER_POOL - 1); row++)
        {
            uint32_t prev = top - row * CONTACTORS_PER_NODE;
            uint32_t next = top - (row + 1) * CONTACTORS_PER_NODE;
            list_insert_after(CONTACTOR_LINKER_REF(prev), CONTACTOR_LINKER_REF(next));
        }
    }
    return true;
}
bool linkup_PlgnNdInit(KeyValue_Array *param_ref)
{
#define PARSE_KEYVALUE_ARRAY(token) uint32_t token = PICKOUT_KEYVALUE(param_ref, #token)
    if (!param_ref)
    {
        return false;
    }
    PARSE_KEYVALUE_ARRAY(u32pwrnodes_max);
    if (!u32pwrnodes_max)
    {
        return false;
    }
    pdu_calloc(0);
    gpNodesArray = CREATE_FLEXSTRUCT_ARRAY(Alloc_nodeArray, u32pwrnodes_max);
    PARSE_KEYVALUE_ARRAY(u32pwrguns_max);
    if (!u32pwrguns_max)
    {
        return false;
    }
    gpPlugsArray = CREATE_FLEXSTRUCT_ARRAY(Alloc_plugArray, u32pwrguns_max);
    PARSE_KEYVALUE_ARRAY(u32pwrcontactors_max);
    if (!u32pwrcontactors_max)
    {
        return false;
    }
    gpContactorsArray = CREATE_FLEXSTRUCT_ARRAY(Alloc_contactorArray, u32pwrcontactors_max);
    if (!gpNodesArray || !gpPlugsArray || !gpContactorsArray)
    {
        printf("!!!flexible array mem allocation failed.\n");
        return false;
    }
    if (u32pwrcontactors_max % u32pwrnodes_max != 0)
    {
        printf("!!!contactors is not a multiple of nodes.\n");
        return false;
    }
    PARSE_KEYVALUE_ARRAY(u32pool_max);
    if (!u32pool_max)
    {
        return false;
    }
    POOL_MAX = u32pool_max;
    CONTACTORS_PER_NODE = u32pwrcontactors_max / u32pwrnodes_max;
    NODES_PER_POOL = u32pwrcontactors_max / CONTACTORS_PER_NODE / POOL_MAX;
    gpCopBarArray = CREATE_FLEXSTRUCT_ARRAY(Tactic_copbarArray, POOL_MAX * CONTACTORS_PER_NODE);
    gpPoolArray = CREATE_FLEXSTRUCT_ARRAY(Tactic_pwrpoolArray, POOL_MAX);
    VLA_INSTANT(Combo_Koinon, koinon_array, POOL_MAX * CONTACTORS_PER_NODE); // in case of VLA enabled in IAR within C99 standard
    scan_KeyValue(NULL, NULL, 0);
    find_plug_join(param_ref, u32pwrguns_max, koinon_array);
    return (list_plug_join(koinon_array, POOL_MAX * CONTACTORS_PER_NODE));
}
bool hear_Canaries_Twittering(void)
{
    if (!IS_FRONT_CANARY_INTACT(gpNodesArray, Alloc_nodeArray))
    {
        printf("gpNodesArray Front canary corrupted!\r\n");
        return false;
    }

    if (!IS_REAR_CANARY_INTACT(gpNodesArray, Alloc_nodeArray))
    {
        printf("gpNodesArray Rear canary corrupted!\r\n");
        return false;
    }

    if (!IS_FRONT_CANARY_INTACT(gpPlugsArray, Alloc_plugArray))
    {

        printf("gpPlugsArray Front canary corrupted!\r\n");
        return false;
    }
    if (!IS_REAR_CANARY_INTACT(gpPlugsArray, Alloc_plugArray))
    {
        printf("gpPlugsArray Rear canary corrupted!\r\n");
        return false;
    }
    if (!IS_FRONT_CANARY_INTACT(gpContactorsArray, Alloc_contactorArray))
    {

        printf("gpContactorsArray Front canary corrupted!\r\n");
        return false;
    }
    if (!IS_REAR_CANARY_INTACT(gpContactorsArray, Alloc_contactorArray))
    {
        printf("gpContactorsArray Rear canary corrupted!\r\n");
        return false;
    }
    return true;
}
bool chk_registered_symbol(void);
void print_TopoGraph();
void linkage_print(void);
void allocating_Reception(void);

PDU_STA FSM_mainEntry_PDU(PDU_CMD cmd)
{
    switch (cmd)
    {
    case PDU_CMD_INITIAT:
    {
        KeyValue_Array *Param = oprt_TopoParam(NULL);
        if (!chk_registered_symbol())
        {
            return PDU_STA_DISCARD;
        }
        if (!Param || !linkup_PlgnNdInit(Param))
        {
            return PDU_STA_DISCARD;
        }

        linkage_print();
        // print_TopoGraph();
        return PDU_STA_WORKING;
    }
    case PDU_CMD_STANDBY:
    {
        return PDU_STA_STANDBY;
    }
    case PDU_CMD_VISUAL:
    {
        print_TopoGraph();
        return PDU_STA_TOPOGRAPH;
    }
    case PDU_CMD_WORKON:
    {
        if (!hear_Canaries_Twittering())
        {
            return PDU_STA_DISCARD;
        }
        allocating_Reception();
        return PDU_STA_WORKING;
    }
    default:
        return PDU_STA_UNKNOWN;
    }
}