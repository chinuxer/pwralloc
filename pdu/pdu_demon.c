
#include "pdu_broker.h"
#include "pdu_tactic.h"
#define PRELITTER "pdu >:"
#define GET_POS -1, -1
#define RIGHTSHIFT(x) (x + (int)strlen("┃┃") + 2)
typedef enum
{
    CURSOR_INC = 1,
    CURSOR_FIX,
    CURSOR_CLR
} CURSOR_OPRT;

typedef struct
{
    int32_t x;
    int32_t y;

} ULTIMUM_POS;

bool opt_Visual_Print_Flag(VISUAL_PRINT_OPRT flag)
{
    static bool Visual_Print_Flag IN_PDU_RAM_SECTION = false;
    if (SET_VERBOSE == flag)
    {
        Visual_Print_Flag = true;
    }
    if (SET_TACITMOD == flag)
    {
        Visual_Print_Flag = false;
    }
    return Visual_Print_Flag;
}

static ULTIMUM_POS opt_ultimum_demonzone(int32_t x, int32_t y)
{
    static ULTIMUM_POS ultimum_pos IN_PDU_RAM_SECTION = {0};
    if (-1 == x && -1 == y)
    {
        return ultimum_pos;
    }

    ultimum_pos.x = x > 0 ? x : ultimum_pos.x;
    ultimum_pos.y = y > 0 ? y : ultimum_pos.y;
    return ultimum_pos;
}

#define BEGINROW_PRINTLOG 6
static int fetch_current_cursor_raw(CURSOR_OPRT oprt)
{
    int max_line = opt_ultimum_demonzone(GET_POS).y - BEGINROW_PRINTLOG;
    static int current_line IN_PDU_RAM_SECTION = 0;
    if (CURSOR_CLR == oprt)
    {
        current_line = 0;
        return 0;
    }
    else if (CURSOR_FIX == oprt)
    {
        return current_line;
    }
    else
    {
        current_line = (current_line + 1) % (0 == max_line ? 1 : max_line);
        return current_line;
    }
}
static void sleep(uint32_t ms)
{
    while ((bool)(ms--))
    {
        SysTick->CTRL = 0;
        SysTick->LOAD = 72000 - 1;
        SysTick->VAL = 0;
        SysTick->CTRL = 5;
        while (!(bool)(SysTick->CTRL & 0x00010000u))
        {
            ;
        }
        SysTick->CTRL = 0;
    }
}

void print_oneliner(const char *format, ...)
{
    if (false == opt_Visual_Print_Flag(GET_STATUS))
    {
        return;
    }
    static int line_lasttime IN_PDU_RAM_SECTION = 0;
    int current_line = fetch_current_cursor_raw(CURSOR_INC);
    if (current_line != line_lasttime)
    {
        if (current_line < line_lasttime)
        {
            int colmax = opt_ultimum_demonzone(GET_POS).x;
            int rowmax = opt_ultimum_demonzone(GET_POS).y;
            printf("\033[%d;%dH", BEGINROW_PRINTLOG, RIGHTSHIFT(colmax));
            for (int i = 5 + current_line; i <= rowmax; i++)
            {
                printf("\033[K");
                printf("\033[1B");
            }
        }
        line_lasttime = current_line;
    }
    // Handle variadic arguments like printf
    printf("\033[u");
    printf("\033[%dB", current_line);
    // printf("\033[J");

    // Print the prompt
    printf(PRELITTER);

    // Handle variadic arguments and print the message
    if (format != NULL && strlen(format) > 0)
    {
        va_list args;
        va_start(args, format);
        vprintf(format, args);
        va_end(args);
        printf("\033[K");

        // current_line = fetch_current_cursor_raw(CURSOR_INC);
        // printf("\033[1E");
        // printf("\033[%dC", RIGHTSHIFT(colmax));
        // printf(PRELITTER);
    }
}

#if 0
void linkage_print(void)
{
    printf("********** PwrPlugin linkage **********\r\n");
    for (int i = 1; i <= gpPlugsArray->length; i++)
    {
        const ListObj *list = PLUG_LINKER_REF(gpPlugsArray, i);
        if (NULL==list)
        {
            continue;
        }

        printf("Plugin %d joined (%d) nodes:", i, list_len_safe(list, CONTACTOR_MAX));

        const ListObj *pos;
        list_for_each(pos, list)
        {

            ID_TYPE contactor_id = ID_OF(pos);
            ID_TYPE pwrnode_id = ID_OF(NODEREF_FROM_CONTACTOR(ID_OF(pos)));
            ID_TYPE pool_id = (pwrnode_id - 1) / NODES_PER_POOL + 1;
           printf(" %" PRIu32 "(%" PRIu32 ",%" PRIu32 ")", contactor_id, pool_id, pwrnode_id);
        }
        printf("\r\n");
    }
    /*
        gear_insert(PLUG_REF(1), NODE_REF(1));
        gear_insert(PLUG_REF(1), NODE_REF(3));
        gear_insert(PLUG_REF(1), NODE_REF(4));
        gear_insert(PLUG_REF(1), NODE_REF(8));
        gear_insert(PLUG_REF(2), NODE_REF(2));
        gear_insert(PLUG_REF(2), NODE_REF(7));
        gear_insert(PLUG_REF(6), NODE_REF(11));
        gear_insert(PLUG_REF(6), NODE_REF(15));
        gear_insert(PLUG_REF(6), NODE_REF(9));
        gear_insert(PLUG_REF(11), NODE_REF(14));
        gear_insert(PLUG_REF(12), NODE_REF(13));
        gear_insert(PLUG_REF(12), NODE_REF(16));

        const struct Alloc_nodeObj *pos;
        printf("plug1 is charging by %d nodes:  ", gear_num(PLUG_REF(1)));
        gear_for_each(pos, PLUG_REF(1))
        {
            printf("%d ,", ID_OF(pos));
        }
        printf("\r\n");
        printf("plug2 is charging by %d nodes:  ", gear_num(PLUG_REF(2)));
        gear_for_each(pos, PLUG_REF(2))
        {
            printf("%d ,", ID_OF(pos));
        }
        printf("\r\n");
        printf("plug6 is charging by %d nodes:  ", gear_num(PLUG_REF(6)));
        gear_for_each(pos, PLUG_REF(6))
        {
            printf("%d ,", ID_OF(pos));
        }
        printf("\r\n");
        printf("plug11 is charging by %d nodes:  ", gear_num(PLUG_REF(11)));
        gear_for_each(pos, PLUG_REF(11))
        {
            printf("%d ,", ID_OF(pos));
        }
        printf("\r\n");
        printf("plug12 is charging by %d nodes:  ", gear_num(PLUG_REF(12)));
        gear_for_each(pos, PLUG_REF(12))
        {
            printf("%d ,", ID_OF(pos));
        }
        printf("\r\n*******************************\r\n");
        gear_remove(PLUG_REF(2), NODE_REF(7));
        gear_remove(PLUG_REF(1), NODE_REF(3));
        printf("plug1 is charging by %d nodes:  ", gear_num(PLUG_REF(1)));
        gear_for_each(pos, PLUG_REF(1))
        {
            printf("%d ,", ID_OF(pos));
        }
        printf("\r\n");
        printf("plug2 is charging by %d nodes\r\n", gear_num(PLUG_REF(2)));
        gear_for_each(pos, PLUG_REF(2))
        {
            printf("%d ,", ID_OF(pos));
        }
            */
}

#endif
#define WHITE_ON_GREEN "\033[97;42m"
#define WHITE_ON_RED "\033[37;41m"
#define BLACK_ON_YELLOW "\033[30;43m"
#define BLACK_ON_WHITE "\033[30;47m"
#define COLOR_RESET "\033[0m"
#define COLOR_GREEN "\033[32m"
#define COLOR_RED "\033[31m"
#define COLOR_YELLOW "\033[33m"
#define COLOR_WHITE "\033[37m"
#define COLOR_BLUE "\033[34m"
#define COLPOS_OF_CONTACTOR1 26
#define COLPOS_OF_CHARGEE1 21
#define COLPOS_OF_LEFTFRAME 15
#define WIDTH_OF_CHARGEE 11
#define HEIGHT_OF_LOGO 10
#define LEFT_MARGIN_BASE 28
#define TOP_MARGIN_BASE 2
#define HEIGHT_OF_TOPFRAME (HEIGHT_OF_LOGO + TOP_MARGIN_BASE * 2)

static void draw_logo(void)
{
    printf("\033[H");
    printf("\033[2J");
    printf("\r\n\r\n%s", COLOR_BLUE);
    printf("██╗███╗   ██╗███████╗██╗   ██╗██████╗  ██████╗ ██╗    ██╗███████╗██████╗        \r\n");
    printf("██║████╗  ██║██╔════╝╚██╗ ██╔╝██╔══██╗██╔═══██╗██║    ██║██╔════╝██╔══██╗       \r\n");
    printf("██║██╔██╗ ██║█████╗   ╚████╔╝ ██████╔╝██║   ██║██║ █╗ ██║█████╗  ██████╔╝       \r\n");
    printf("██║██║╚██╗██║██╔══╝    ╚██╔╝  ██╔═══╝ ██║   ██║██║███╗██║██╔══╝  ██╔══██╗       \r\n");
    printf("██║██║ ╚████║██║        ██║   ██║     ╚██████╔╝╚███╔███╔╝███████╗██║  ██║       \r\n");
    printf("╚═╝╚═╝  ╚═══╝╚═╝        ╚═╝   ╚═╝      ╚═════╝  ╚══╝╚══╝ ╚══════╝╚═╝  ╚═╝       \r\n%s", COLOR_RESET);
}
static void draw_frameline_pool(ID_TYPE matrix)
{
    if (matrix < 1 || matrix > POOL_MAX)
    {
        return;
    }
    printf("\033[H");
    printf("\033[%dB", (int)(HEIGHT_OF_LOGO + (CONTACTORS_PER_NODE * 2 + 6) * (matrix - 1)));
    printf("%s POOL⚡", COLOR_GREEN);
    printf("%" PRIu32, matrix);
    printf("%s\n", COLOR_RESET);
    printf("\r\n\r\n");
    printf("\033[%dG", COLPOS_OF_LEFTFRAME);
    printf("┏━━━━");

    for (int contactor = 1; contactor <= CONTACTORS_PER_NODE; contactor++)
    {
        printf("━━━━━━┻━━━━━━━━");
    }
    printf("┓");
    for (int node = 0; node <= NODES_PER_POOL * 2; node++)
    {
        printf("\r\n\033[%dG", COLPOS_OF_LEFTFRAME);
        printf("┃");

        printf("%*s", (int)(strlen("┏━━━━") / 3 - 1 + strlen("━━━━━━┻━━━━━━━━") / 3 * CONTACTORS_PER_NODE), " ");
        printf("┃");
    }
    printf("\r\n\033[%dG", COLPOS_OF_LEFTFRAME);
    printf("┗━━━━");
    for (int contactor = 1; contactor <= CONTACTORS_PER_NODE; contactor++)
    {
        printf("━━━━━━━━━━━━━━━");
    }
    printf("┛");
}

static inline void recover_pos(void)
{
    printf("\033[u");
    printf("\033[%dB", fetch_current_cursor_raw(CURSOR_FIX));
    printf("\033[%dC", (int)strlen(PRELITTER));
}
void draw_pwrnode(ID_TYPE node, bool post_scrip)
{
    // Validate input range early
    if (node < 1 || node > NODE_MAX)
    {
        return;
    }

    // Early exit based on visual flag
    if (false == opt_Visual_Print_Flag(GET_STATUS))
    {
        return;
    }
// ！！No compromising with cstat static detection rules！！(｀ﾍ´)=3
#ifndef __CSTAT__

    const ID_TYPE node_index = node - 1;

    ID_TYPE matrix = node_index / NODES_PER_POOL;
    ID_TYPE node_per_matrix = node_index % NODES_PER_POOL;
    struct Alloc_nodeObj *ref = NODE_REF(node);

    if (ref == NULL)
    {
        return;
    }

    ID_TYPE id = ID_OF(ref);

    // 4 constant var, clearly, Move complex formatting logic out of printf(...) call ●|￣|＿
    const float dummy_I = ref->value_Iset + (rand() % 100) / 100.0f;
    const float dummy_V = ref->value_Vset + (rand() % 100) / 50.0f;
    const char *color_code = ref->discarded ? BLACK_ON_WHITE : (dummy_I > 10.0f ? BLACK_ON_YELLOW : WHITE_ON_GREEN);
    const int row_offset = HEIGHT_OF_TOPFRAME + (CONTACTORS_PER_NODE * 2 + 6) * matrix + node_per_matrix * 2;

    printf("\033[H");               // Home cursor
    printf("\033[%dB", row_offset); // Move down rows
    printf("%s%02u%6.1fA,%5.1fV %s",
           color_code,
           id,
           (double)dummy_I,
           (double)dummy_V,
           COLOR_RESET);
#endif

    if (post_scrip)
    {
        recover_pos();
    }
}

void draw_plug(ID_TYPE plug, bool post_scrip)
{
    if (plug < 1 || plug > POOL_MAX * CONTACTORS_PER_NODE)
    {
        return;
    }
    if (false == opt_Visual_Print_Flag(GET_STATUS))
    {
        return;
    }
    ID_TYPE matrix = (plug - 1) / CONTACTORS_PER_NODE;
    ID_TYPE contactor = (plug - 1) % CONTACTORS_PER_NODE;
    printf("\033[H");
    printf("\033[%dB", (int)(HEIGHT_OF_LOGO + (CONTACTORS_PER_NODE * 2 + 6) * matrix));
    uint32_t col_start = COLPOS_OF_CHARGEE1 + contactor * (WIDTH_OF_CHARGEE + 4);
    printf("\033[%" PRIu32 "G", col_start);
    ID_TYPE id = matrix * CONTACTORS_PER_NODE * NODES_PER_POOL + contactor + 1;
    struct Alloc_plugObj *header = get_header_plug(id);

    id = (NULL != header ? ID_OF(header) : 99);
    if (PRIOR_VAIN != PLUG_REF(id)->strategy_info.priority)
    {
        if (NODE_VAIN == PLUG_REF(id)->next_charger)
        {
            printf("%s⏳", BLACK_ON_WHITE);
            printf("%sPLUG#%02" PRIu32 "  %s", BLACK_ON_YELLOW, id, COLOR_RESET);
        }
        else
        {
            printf("%s%01d%s PLUG#%02" PRIu32 "  %s", BLACK_ON_WHITE, PLUG_REF(id)->strategy_info.priority, BLACK_ON_YELLOW, id, COLOR_RESET);
        }
    }
    else
    {
        printf("%s  PLUG#%02" PRIu32 "  %s", WHITE_ON_GREEN, id, COLOR_RESET);
    }

    printf("\033[1B");
    printf("\033[%" PRIu32 "G", col_start);
    printf("%s  %5.1fkW  %s", (PRIOR_VAIN != (PLUG_REF(id))->strategy_info.priority ? BLACK_ON_YELLOW : WHITE_ON_GREEN), (double)PLUG_REF(id)->strategy_info.demand*0.4, COLOR_RESET);
    printf("\033[1A");
    if (post_scrip)
    {
        recover_pos();
    }
}
void draw_contactor(ID_TYPE contactor, bool post_scrip)
{
    if (contactor < 1 || contactor > CONTACTOR_MAX)
    {
        return;
    }
    if (false == opt_Visual_Print_Flag(GET_STATUS))
    {
        return;
    }

    const ID_TYPE idx = contactor - 1;

    ID_TYPE matrix = idx / CONTACTORS_PER_NODE / NODES_PER_POOL;
    ID_TYPE node = idx / CONTACTORS_PER_NODE % NODES_PER_POOL;
    ID_TYPE contactor_per_matrix = idx % CONTACTORS_PER_NODE;

    printf("\033[H");
    int row_offset = HEIGHT_OF_TOPFRAME + (CONTACTORS_PER_NODE * 2 + 6) * matrix + node * 2;
    printf("\033[%dB", row_offset);

    uint32_t col_start = COLPOS_OF_CHARGEE1 + 5 + contactor_per_matrix * (WIDTH_OF_CHARGEE + 4);
    printf("\033[%" PRIu32 "G", col_start);

    // Refactored color selection logic
    const char *color_code;
    if (CONTACTOR_REF(contactor)->discarded)
    {
        color_code = COLOR_WHITE;
    }
    else if (CONTACTOR_REF(contactor)->ia_contacted)
    {
        color_code = COLOR_RED;
    }
    else
    {
        color_code = COLOR_GREEN;
    }
    printf("%s%s%s", color_code, "◼", COLOR_RESET);

    if (post_scrip)
    {
        recover_pos();
    }
}

void print_TopoGraph(void)
{
    fetch_current_cursor_raw(CURSOR_CLR);
    draw_logo();
    sleep(250);
    for (ID_TYPE matrix = 1; matrix <= POOL_MAX; matrix++)
    {
        draw_frameline_pool(matrix);
        sleep(150);
    }
    for (ID_TYPE node = 1; node <= NODE_MAX; node++)
    {
        draw_pwrnode(node, false);
    }
    sleep(150);
    for (ID_TYPE plug = 1; plug <= POOL_MAX * CONTACTORS_PER_NODE; plug++)
    {
        draw_plug(plug, false);
    }
    sleep(150);
    for (ID_TYPE contactor = 1; contactor <= CONTACTOR_MAX; contactor++)
    {
        draw_contactor(contactor, false);
    }
    opt_ultimum_demonzone((int32_t)(LEFT_MARGIN_BASE + COLPOS_OF_LEFTFRAME * CONTACTORS_PER_NODE), (int32_t)(HEIGHT_OF_LOGO + (CONTACTORS_PER_NODE * 2 + 6) * POOL_MAX));
    sleep(250);
    int linemax = opt_ultimum_demonzone(GET_POS).y;
    int colmax = opt_ultimum_demonzone(GET_POS).x;
    printf("\033[H");
    printf("\033[%dC", colmax);
    printf("\033[%dB", TOP_MARGIN_BASE);
    for (int i = TOP_MARGIN_BASE; i < linemax; i++)
    {
        printf("┃┃");
        printf("\033[1B");
        printf("\033[2D");
    }
    printf("\033[H");
    printf("\033[%dC", RIGHTSHIFT(colmax));
    printf("\033[%dB", TOP_MARGIN_BASE);
    printf(COLOR_YELLOW);
    printf("Usage: in(#{plug id},{required current}A,{priority})   | Such as: in(#1,250A,2)    //No.1 charger plug in as VIP,250A needed \r\n");
    printf("\033[%dC", RIGHTSHIFT(colmax));
    printf("       ex(#{plug id})                                  | Such as: ex(#2)           //No.2 charger plug out\r\n");
    printf("\033[%dC", RIGHTSHIFT(colmax));
    printf("       va(#{plug id},{changed current}A)               | Such as: va(#3,80A)       //No.3 charger demand downsized to 80A\r\n");
    printf("\033[%dC", RIGHTSHIFT(colmax));
    printf(COLOR_RESET);
    printf("\033[s");
    // printf(PRELITTER);
    //  printf("\033[?25l");
}
