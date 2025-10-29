
#include "pdu_broker.h"
#include "pdu_ex_datatype.h"
#define PRELITTER "pdu >:"

typedef enum
{
    CURSOR_INC = 1,
    CURSOR_FIX,
    CURSOR_CLR
} CURSOR_OPRT;
static int fetch_current_cursor_raw(CURSOR_OPRT oprt)
{
#define MAX_LINE 20
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
    else if (CURSOR_INC == oprt)
    {
        current_line = (current_line + 1) % MAX_LINE;
        return current_line;
    }
    else
    {
        return 0;
    }
}
static void sleep(uint32_t ms)
{
    while (ms--)
    {
        SysTick->CTRL = 0;
        SysTick->LOAD = 72000 - 1;
        SysTick->VAL = 0;
        SysTick->CTRL = 5;
        while (!(SysTick->CTRL & 0x00010000))
            ;
        SysTick->CTRL = 0;
    }
}

bool gear_insert(struct Alloc_plugObj *plug, struct Alloc_nodeObj *target);
uint32_t gear_num(const struct Alloc_plugObj *plug);
MATCHURE gear_remove(struct Alloc_plugObj *plug, struct Alloc_nodeObj *target);

void print_oneliner(const char *format, ...)
{

    int current_line = fetch_current_cursor_raw(CURSOR_INC);
    // Handle variadic arguments like printf
    printf("\033[u");
    printf("\033[%dB", current_line);
    printf("\033[J");

    // Print the prompt
    printf(PRELITTER);

    // Handle variadic arguments and print the message
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
    printf("\033[K");
    if (format != NULL && strlen(format) > 0)
    {
        current_line = fetch_current_cursor_raw(CURSOR_INC);
        printf("\033[1E");
        printf(PRELITTER);
    }
}
void linkage_print(void)
{
    printf("********** PwrPlugin linkage **********\r\n");
    for (int i = 1; i <= gpPlugsArray->length; i++)
    {
        const ListObj *list = PLUG_LINKER_REF(gpPlugsArray, i);
        if (!list)
        {
            continue;
        }

        printf("Plugin %d joined (%d) nodes:", i, list_len_safe(list, CONTACTOR_MAX));

        const ListObj *pos;
        list_for_each(pos, list)
        {

            uint32_t contactor_id = ID_OF(pos);
            uint32_t pwrnode_id = ID_OF(NODEREF_FROM_CONTACTOR(ID_OF(pos)));
            uint32_t pool_id = (pwrnode_id - 1) / NODES_PER_POOL + 1;
            printf(" %d(%d,%d)", contactor_id, pool_id, pwrnode_id);
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

#define WHITE_ON_GREEN "\033[97;42m"
#define WHITE_ON_RED "\033[37;41m"
#define BLACK_ON_YELLOW "\033[30;43m"
#define BLACK_ON_WHITE "\033[30;47m"
#define COLOR_RESET "\033[0m"
#define COLOR_GREEN "\033[32m"
#define COLOR_RED "\033[31m"
#define COLOR_YELLOW "\033[33m"
#define COLOR_BLUE "\033[34m"
#define COLPOS_OF_CONTACTOR1 26
#define COLPOS_OF_CHARGEE1 21
#define COLPOS_OF_LEFTFRAME 15
#define WIDTH_OF_CHARGEE 11
#define HEIGHT_OF_LOGO 10
#define HEIGHT_OF_TOPFRAME (HEIGHT_OF_LOGO + 4)
struct Alloc_plugObj *get_header_plug(uint32_t contactor_id);
static void draw_logo()
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
static void draw_frameline_pool(int matrix)
{
    if (matrix < 1 || matrix > POOL_MAX)
    {
        return;
    }
    printf("\033[H");
    printf("\033[%dB", (int)(HEIGHT_OF_LOGO + (CONTACTORS_PER_NODE * 2 + 6) * (matrix - 1)));
    printf("%s POOL⚡%d%s", COLOR_GREEN, matrix, COLOR_RESET);
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
    printf("\033[%dC", strnlen(PRELITTER, 6));
}
void draw_pwrnode(int node, bool post_scrip)
{
    if (node < 1 || node > NODE_MAX)
    {
        return;
    }
    int matrix = (node - 1) / NODES_PER_POOL;
    int node_per_matrix = (node - 1) % NODES_PER_POOL;
    int id = ID_OF(NODE_REF(node));
    float dummy_I = NODE_REF(node)->value_Iset + rand() % 100 / 100.0f;
    float dummy_V = NODE_REF(node)->value_Vset + rand() % 100 / 50.0f;
    printf("\033[H");
    printf("\033[%dB", (int)(HEIGHT_OF_TOPFRAME + (CONTACTORS_PER_NODE * 2 + 6) * matrix + node_per_matrix * 2));
    printf("%s%02d%6.1fA,%5.1fV %s", (dummy_I > 10.0f ? BLACK_ON_YELLOW : WHITE_ON_GREEN), id, (double)dummy_I, (double)dummy_V, COLOR_RESET);
    if (post_scrip)
    {
        recover_pos();
    }
}

void draw_plug(int plug, bool post_scrip)
{
    if (plug < 1 || plug > POOL_MAX * CONTACTORS_PER_NODE)
    {
        return;
    }
    int matrix = (plug - 1) / CONTACTORS_PER_NODE;
    int contactor = (plug - 1) % CONTACTORS_PER_NODE;
    printf("\033[H");
    printf("\033[%dB", (int)(HEIGHT_OF_LOGO + (CONTACTORS_PER_NODE * 2 + 6) * matrix));
    int col_start = COLPOS_OF_CHARGEE1 + contactor * (WIDTH_OF_CHARGEE + 4);
    printf("\033[%dG", col_start);
    int id = matrix * CONTACTORS_PER_NODE * NODES_PER_POOL + contactor + 1;
    struct Alloc_plugObj *header = get_header_plug(id);

    id = header ? ID_OF(header) : 99;
    if (PRIOR_VAIN != PLUG_REF(id)->strategy_info.priority)
    {
        printf("%s%01d%s PLUG#%02d  %s", BLACK_ON_WHITE, PLUG_REF(id)->strategy_info.priority, BLACK_ON_YELLOW, id, COLOR_RESET);
    }
    else
    {
        printf("%s  PLUG#%02d  %s", WHITE_ON_GREEN, id, COLOR_RESET);
    }

    printf("\033[1B");
    printf("\033[%dG", col_start);
    printf("%s  %5.1fkW  %s", (PLUG_REF(id)->strategy_info.priority ? BLACK_ON_YELLOW : WHITE_ON_GREEN), (double)PLUG_REF(id)->strategy_info.demand, COLOR_RESET);
    printf("\033[1A");
    if (post_scrip)
    {
        recover_pos();
    }
}
void draw_contactor(int contactor, bool post_scrip)
{
    if (contactor < 1 || contactor > CONTACTOR_MAX)
    {
        return;
    }
    int matrix = (contactor - 1) / CONTACTORS_PER_NODE / NODES_PER_POOL;
    int node = (contactor - 1) / CONTACTORS_PER_NODE % NODES_PER_POOL;
    int contactor_per_matrix = (contactor - 1) % CONTACTORS_PER_NODE;
    printf("\033[H");
    printf("\033[%dB", (int)(HEIGHT_OF_TOPFRAME + (CONTACTORS_PER_NODE * 2 + 6) * matrix + node * 2));
    int col_start = COLPOS_OF_CHARGEE1 + 5 + contactor_per_matrix * (WIDTH_OF_CHARGEE + 4);
    printf("\033[%dG", col_start);
    printf("%s%s%s", (CONTACTOR_REF(contactor)->ia_contacted ? COLOR_RED : COLOR_GREEN), "◼", COLOR_RESET);
    if (post_scrip)
    {
        recover_pos();
    }
}

void print_TopoGraph()
{
    fetch_current_cursor_raw(CURSOR_CLR);
    draw_logo();
    sleep(250);
    for (int matrix = 1; matrix <= POOL_MAX; matrix++)
    {
        draw_frameline_pool(matrix);
        sleep(150);
    }
    for (int node = 1; node <= NODE_MAX; node++)
    {
        draw_pwrnode(node, false);
    }
    sleep(150);
    for (int plug = 1; plug <= POOL_MAX * CONTACTORS_PER_NODE; plug++)
    {
        draw_plug(plug, false);
    }
    sleep(150);
    for (int contactor = 1; contactor <= CONTACTOR_MAX; contactor++)
    {
        draw_contactor(contactor, false);
    }
    sleep(250);
    printf("\r\n\r\n\r\n\r\n");
    printf(COLOR_YELLOW);
    printf("支持命令格式: in(#{充电桩序号},{电流}A,{优先级})   | 示例: in(#1,250A,2)    //插入充电枪头\r\n");
    printf("              ex(#{充电桩序号})                    | 示例: ex(#2)           //拔出充电枪头\r\n\r\n");
    printf(COLOR_RESET);
    printf("\033[s");
    printf(PRELITTER);
    printf("\033[?25l");
}
