
#define __IMPORT_PDU_MALLOC__
#define __IMPORT_GLOBALVAR__
#include "pdu_broker.h"
#define __INSTANT__PARAM__
#include "pdu_param.h"

static void list_init(ListObj *const list)
{
    list->next_linker = list->prev_linker = list;
}

static void list_insert_after(ListObj *const list, ListObj *const node)
{
    list->next_linker->prev_linker = node;
    node->next_linker = list->next_linker;

    list->next_linker = node;
    node->prev_linker = list;
}
/*
static void list_insert_before(ListObj *const list, ListObj *const node)
{
    list->prev_linker->next_linker = node;
    node->prev_linker = list->prev_linker;

    list->prev_linker = node;
    node->next_linker = list;
}

static void list_remove(ListObj *const node)
{
    node->next_linker->prev_linker = node->prev_linker;
    node->prev_linker->next_linker = node->next_linker;

    node->next_linker = node->prev_linker = node;
}

static uint32_t list_len(const ListObj *list)
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
*/
static bool list_isempty(const ListObj *list)
{
    return list->next_linker == list;
}

uint32_t list_len_safe(const ListObj *list, uint32_t max)
{
    uint32_t len = 0;
    if (list_isempty(list))
    {
        return len;
    }
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

/*
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
        static uint32_t num IN_PDU_RAM_SECTION = 0;
        if (++num > NODE_MAX)
        {
            num = 0; // Reset counter
            break;
        }
    }

    plug->next_charger = NODE_VAIN;
    return;
}
*/
uint32_t gear_num(const struct Alloc_plugObj *plug)
{
    const struct Alloc_nodeObj *p = plug->next_charger;
    uint32_t num = (uint32_t)(NODE_VAIN != p);
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
    if (NULL == plug || NULL == target || NULL != target->next_charger) // or target is already in a plug charging chain
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
    if (!(bool)plug || !(bool)target)
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

ListObj *get_Cross_BtwnPlugNode(ID_TYPE node_id, ID_TYPE plug_id)
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
    return NULL;
}
struct Alloc_plugObj *get_header_plug(ID_TYPE contactor_id)
{
    if (contactor_id < 1 || contactor_id > CONTACTOR_MAX)
    {
        return NULL;
    }
    const ListObj *list = CONTACTOR_LINKER_REF(contactor_id);
    const ListObj *pos;
    list_for_each(pos, list)
    {
        //cstat #CERT-EXP36-C_a #CERT-EXP39-C_d
        if (ID_OF(pos) == *(uint32_t *)(char[]){"PLUG"})
        {
            return list_entry(pos, struct Alloc_plugObj, copula);
        }
    }
    return NULL;
}

#define CREATE_FLEXSTRUCT_ARRAY(type, count) \
    (type *)create_FlexStruct_Array(sizeof(type), sizeof(((type *)0)->obj_array[0]), (count), type##_Init)

#define IS_FRONT_CANARY_INTACT(ptr, type) \
    (NULL != (ptr) ? (((const type *)(ptr))->front_canary == (FRONT_MAGICWORD)) : false)
#define GET_REAR_CANARY_PTR(ptr, type) \
    ((uint32_t *)((char *)(ptr) + sizeof(type) + (ptr)->length * sizeof(((type *)0)->obj_array[0])))
#define IS_REAR_CANARY_INTACT(ptr, type) \
    (NULL != (ptr) ? (*GET_REAR_CANARY_PTR(ptr, type) == (REAR_MAGICWORD)) : false)

static void Alloc_NodesArray_Init(void *const ptr, size_t n)
{
    if (!(bool)ptr)
    {
        return;
    }
    //cstat #CERT-EXP36-C_a #CERT-EXP36-C_b #CERT-EXP39-C_d
    Alloc_NodesArray *p = (Alloc_NodesArray *)ptr;
    p->length = n;
    p->front_canary = FRONT_MAGICWORD;
    *GET_REAR_CANARY_PTR(p, Alloc_NodesArray) = REAR_MAGICWORD;
    for (ID_TYPE i = 1; i <= p->length; i++)
    {
        ID_OF(REF(p, i)) = i;
        REF(p, i)->next_charger = NULL;
    }
}
static void Alloc_PlugsArray_Init(void *const ptr, size_t n)
{
    if (!(bool)ptr)
    {
        return;
    }
    //cstat #CERT-EXP36-C_a #CERT-EXP36-C_b #CERT-EXP39-C_d
    Alloc_PlugsArray *p = (Alloc_PlugsArray *)ptr;
    p->length = n;
    p->front_canary = FRONT_MAGICWORD;
    p->criteria = CRITERIA_ASSIGNMENT;
    *GET_REAR_CANARY_PTR(p, Alloc_PlugsArray) = REAR_MAGICWORD;
    for (ID_TYPE i = 1; i <= p->length; i++)
    {
        list_init(PLUG_LINKER_REF(p, i));
        ID_OF(REF(p, i)) = i;
        ID_OF(PLUG_LINKER_REF(p, i)) = *(uint32_t *)(char[]){"PLUG"};
        REF(p, i)->next_charger = NODE_VAIN;
    }
}
static void Alloc_ContactorsArray_Init(void *const ptr, size_t n)
{
    if (!(bool)ptr)
    {
        return;
    }
    //cstat #CERT-EXP36-C_a #CERT-EXP36-C_b #CERT-EXP39-C_d
    Alloc_ContactorsArray *p = (Alloc_ContactorsArray *)ptr;
    p->length = n;
    p->front_canary = FRONT_MAGICWORD;
    *GET_REAR_CANARY_PTR(p, Alloc_ContactorsArray) = REAR_MAGICWORD;
    for (ID_TYPE i = 1; i <= p->length; i++)
    {
        ID_OF(REF(p, i)) = i;
        list_init(CONTACTOR_LINKER_REF(p, i));
    }
}

static void Alloc_CopBarArray_Init(void *const ptr, size_t n)
{
    if (!(bool)ptr)
    {
        return;
    }
    //cstat #CERT-EXP36-C_a #CERT-EXP36-C_b #CERT-EXP39-C_d
    Alloc_CopBarArray *p = (Alloc_CopBarArray *)ptr;
    p->length = n;
    p->front_canary = FRONT_MAGICWORD;
    *GET_REAR_CANARY_PTR(p, Alloc_CopBarArray) = REAR_MAGICWORD;
    for (ID_TYPE i = 1; i <= p->length; i++)
    {
        ID_OF(REF(p, i)) = i;
    }
}
static void Alloc_PoolArray_Init(void *const ptr, size_t n)
{
    if (!(bool)ptr)
    {
        return;
    }
    //cstat #CERT-EXP36-C_a #CERT-EXP36-C_b #CERT-EXP39-C_d
    Alloc_PoolArray *p = (Alloc_PoolArray *)ptr;
    p->length = n;
    p->front_canary = FRONT_MAGICWORD;
    *GET_REAR_CANARY_PTR(p, Alloc_PoolArray) = REAR_MAGICWORD;
    for (ID_TYPE i = 1; i <= p->length; i++)
    {
        ID_OF(REF(p, i)) = i;
    }
}

static void Alloc_ReqSettlerArray_Init(void *const ptr, size_t n)
{
    if (!(bool)ptr)
    {
        return;
    }
    //cstat #CERT-EXP36-C_a #CERT-EXP36-C_b #CERT-EXP39-C_d
    Alloc_ReqSettlerArray *p = (Alloc_ReqSettlerArray *)ptr;
    p->length = n;
    p->front_canary = FRONT_MAGICWORD;
    *GET_REAR_CANARY_PTR(p, Alloc_ReqSettlerArray) = REAR_MAGICWORD;
    for (int i = 0; i < p->length; i++)
    {
        memset((void *)(p->obj_array + i), 0, sizeof(p->obj_array[0]));
    }
}

static void *create_FlexStruct_Array(size_t header_size, size_t element_size, size_t count, void (*init_func)(void *, size_t))
{
    // Validate parameters
    if (count == 0u || 0u == header_size || 0u == element_size)
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
#pragma calls = Alloc_PlugsArray_Init, Alloc_NodesArray_Init, Alloc_ContactorsArray_Init, \
        Alloc_CopBarArray_Init, Alloc_PoolArray_Init, Alloc_ReqSettlerArray_Init
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
    for (ID_TYPE i = 1; i <= max; i++)
    {
        scan_KeyValue(valuesArray, array, i);
    }
}
static inline ID_TYPE top_within_col_of(ID_TYPE contactor_id)
{
    contactor_id--;
    return ((contactor_id / CONTACTORS_PER_NODE / NODES_PER_POOL + 1) * (NODES_PER_POOL * CONTACTORS_PER_NODE) - CONTACTORS_PER_NODE + contactor_id % CONTACTORS_PER_NODE + 1);
}
static bool list_plug_join(const Combo_Koinon *array, uint32_t max)
{
    for (uint8_t i = 0; i < max; i++)
    {
        if (0u == array[i].id_of.plug || 0u == array[i].id_of.contactor)
        {
            continue;
        }
        if (array[i].id_of.plug > max)
        {
            printf("!!!plugjoin_array[%" PRIu8 "] is out of range.\n", i);
            return false;
        }

        ID_TYPE top = top_within_col_of(array[i].id_of.contactor);

        list_insert_after(PLUG_LINKER_REF(gpPlugsArray, array[i].id_of.plug), CONTACTOR_LINKER_REF(top));
       
        for (uint8_t row = 0; row < (NODES_PER_POOL - 1); row++)
        {
            ID_TYPE prev = top - row * CONTACTORS_PER_NODE;
            if (prev > array[i].id_of.contactor)
            {
                CONTACTOR_REF(prev)->discarded = true;
            }
            ID_TYPE next = top - (row + 1) * CONTACTORS_PER_NODE;
            list_insert_after(CONTACTOR_LINKER_REF(prev), CONTACTOR_LINKER_REF(next));
        }
       
    }
    return true;
}

static void check_Discarded_Node(void)
{
#define FOREACH_IDX(idx,max) for (ID_TYPE idx = 1; idx <= (max); idx++)
    // Iterate through all nodes in the array
    FOREACH_IDX(node_idx, NODE_MAX)
    {
        struct Alloc_nodeObj* node = NODE_REF(node_idx);
        
        // Skip already discarded nodes
        if (node->discarded) 
        {
            continue;
        }
        
        bool all_contactors_discarded = true;
        
        // Check all contactors connected to this node
        FOREACH_IDX(cont_idx, CONTACTORS_PER_NODE)
        {
            ID_TYPE contactor_id = (node_idx-1) * CONTACTORS_PER_NODE + cont_idx;
            
            // Verify the contactor ID is valid before accessing
            if (contactor_id >= 1 && contactor_id <= CONTACTOR_MAX) 
            {
                // If any contactor is not discarded, the node should not be marked as discarded
                if (!CONTACTOR_REF(contactor_id)->discarded) 
                {
                    all_contactors_discarded = false;
                    break;
                }
            }
            else 
            {
                // Invalid contactor ID, treat as not discarded
                all_contactors_discarded = false;
                break;
            }
        }
        
        // If all contactors are discarded, mark the node as discarded
        if (all_contactors_discarded) 
        {
            node->discarded = true;
        }
    }
}
static bool linkup_PlgnNdInit(KeyValue_Array *param_ref)
{
#define PARSE_KEYVALUE_ARRAY(token) uint32_t token = PICKOUT_KEYVALUE(param_ref, #token)
    //cstat #CERT-EXP36-C_b
    PARSE_KEYVALUE_ARRAY(u32pwrnodes_max);
    if (0==u32pwrnodes_max)
    {
        return false;
    }
    PARSE_KEYVALUE_ARRAY(u32pwrguns_max);
    if (0==u32pwrguns_max)
    {
        return false;
    }
    PARSE_KEYVALUE_ARRAY(u32pwrcontactors_max);
    if (0==u32pwrcontactors_max)
    {
        return false;
    }
    PARSE_KEYVALUE_ARRAY(u32pool_max);
    if (0==u32pool_max)
    {
        return false;
    }
    VLA_INSTANT(Combo_Koinon, koinon_array, u32pool_max * u32pwrcontactors_max / u32pwrnodes_max);
    scan_KeyValue(NULL, NULL, 0);
    find_plug_join(param_ref, u32pwrguns_max, koinon_array);
    pdu_calloc(0);
    gpNodesArray = CREATE_FLEXSTRUCT_ARRAY(Alloc_NodesArray, u32pwrnodes_max);    
    gpPlugsArray = CREATE_FLEXSTRUCT_ARRAY(Alloc_PlugsArray, u32pwrguns_max);    
    gpContactorsArray = CREATE_FLEXSTRUCT_ARRAY(Alloc_ContactorsArray, u32pwrcontactors_max);    
    POOL_MAX = u32pool_max;
    CONTACTORS_PER_NODE = u32pwrcontactors_max / u32pwrnodes_max;
    NODES_PER_POOL = u32pwrcontactors_max / CONTACTORS_PER_NODE / POOL_MAX;
    //cstat !MISRAC2012-Rule-20.7
    gpCopBarArray = CREATE_FLEXSTRUCT_ARRAY(Alloc_CopBarArray, POOL_MAX * CONTACTORS_PER_NODE);
    gpPoolArray = CREATE_FLEXSTRUCT_ARRAY(Alloc_PoolArray, POOL_MAX);
    gpReqSettlerArray = CREATE_FLEXSTRUCT_ARRAY(Alloc_ReqSettlerArray, PLUG_MAX);
    if (!(bool)gpNodesArray || !(bool)gpPlugsArray || !(bool)gpContactorsArray || !(bool)gpCopBarArray || !(bool)gpPoolArray || !(bool)gpReqSettlerArray)
    {
        printf("!!!flexible array mem allocation failed.\n");
        return false;
    }
    if (u32pwrcontactors_max % u32pwrnodes_max != 0)
    {
        printf("!!!contactors is not a multiple of nodes.\n");
        return false;
    }    
    return (list_plug_join(koinon_array, POOL_MAX * CONTACTORS_PER_NODE));
}
bool hear_Canaries_Twittering(void)
{
#define CONTEXT_EXPANDER_PROJ_GLOBAL(x)                                        \
    do                                                                         \
    {                                                                          \
        if (!IS_FRONT_CANARY_INTACT(VAR_NAME(x), VAR_TYPE(x)))                 \
        {                                                                      \
            printf("allocated object of " #x ": Front canary corrupted!\r\n"); \
            return false;                                                      \
        }                                                                      \
        if (!IS_REAR_CANARY_INTACT(VAR_NAME(x), VAR_TYPE(x)))                  \
        {                                                                      \
            printf("allocated object of " #x ": Rear canary corrupted!\r\n");  \
            return false;                                                      \
        }                                                                      \
    } while (0)
    //cstat #CERT-EXP39-C_d #CERT-EXP39-C_a #CERT-EXP36-C_a
    VARIABLE_LIST_PENDING_EXPANDED
    return true;
#undef CONTEXT_EXPANDER_PROJ_GLOBAL
}

bool chk_registered_symbol(void);
// void linkage_print(void);
bool init_PDU_Broker(void)
{
    KeyValue_Array *Param = oprt_TopoParam(NULL);
    if (!chk_registered_symbol())
    {
        return false;
    }
    //cstat !MISRAC2012-Rule-13.5
    if (!(bool)Param || !(bool)linkup_PlgnNdInit(Param))
    {
        return false;
    }
     check_Discarded_Node();
    // linkage_print();
    return true;
}