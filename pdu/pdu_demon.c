
#include "pdu_broker.h"

void sleep(uint32_t ms)
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
}

#define WHITE_ON_GREEN "\033[97;42m"
#define WHITE_ON_RED "\033[37;41m"
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
const struct Alloc_plugObj *get_header_plug(uint32_t contactor_id);
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

static void draw_pwrnode(int node)
{
    if (node < 1 || node > NODE_MAX)
    {
        return;
    }
    int matrix = (node - 1) / NODES_PER_POOL;
    int node_per_matrix = (node - 1) % NODES_PER_POOL;
    int id = NODE_REF(node)->id;
    float dummy_I = NODE_REF(node)->value_Iset + rand() % 100 / 10.0f;
    float dummy_V = NODE_REF(node)->value_Vset + rand() % 100 / 10.0f;
    printf("\033[H");
    printf("\033[%dB", (int)(HEIGHT_OF_TOPFRAME + (CONTACTORS_PER_NODE * 2 + 6) * matrix + node_per_matrix * 2));
    printf("%s%02d%6.1fA,%5.1fV %s", (dummy_I > 5 ? WHITE_ON_RED : WHITE_ON_GREEN), id, (double)dummy_I, (double)dummy_V, COLOR_RESET);
}

static void draw_plug(int plug)
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
    const struct Alloc_plugObj *header = get_header_plug(id);

    id = header ? ID_OF(header) : 99;
    printf("%s  PLUG#%02d  %s", (PLUG_REF(plug)->energon ? WHITE_ON_RED : WHITE_ON_GREEN), id, COLOR_RESET);
    printf("\033[1B");
    printf("\033[%dG", col_start);
    printf("%s  %5.1fkW  %s", (PLUG_REF(plug)->energon ? WHITE_ON_RED : WHITE_ON_GREEN), (double)PLUG_REF(plug)->demand, COLOR_RESET);
    printf("\033[1A");
}
static void draw_contactor(int contactor)
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
    printf("%s%s%s", (1 ? COLOR_RED : COLOR_GREEN), "◼", COLOR_RESET);
}

void print_TopoGraph()
{
    draw_logo();
    sleep(250);
    for (int matrix = 1; matrix <= POOL_MAX; matrix++)
    {
        draw_frameline_pool(matrix);
        sleep(250);
    }
    for (int node = 1; node <= NODE_MAX; node++)
    {
        draw_pwrnode(node);
    }
    sleep(250);
    for (int plug = 1; plug <= POOL_MAX * CONTACTORS_PER_NODE; plug++)
    {
        draw_plug(plug);
    }
    sleep(250);
    for (int contactor = 1; contactor <= CONTACTOR_MAX; contactor++)
    {
        draw_contactor(contactor);
    }
    printf("\r\n\r\n");
    printf("\033[?25l");
}
