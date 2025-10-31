#include "pdu_main.h"
#include "pdu_broker.h"
#define COMMAND_OUT_SHOWTOPO "topo"
#define COMMAND_IN_PREFIX "in("
#define COMMAND_OUT_PREFIX "ex("
#define COMMAND_SUFFIX ")"

#define CLI_PRINTF(...) SEGGER_RTT_printf(1, __VA_ARGS__)
// 命令参数结构体
typedef struct
{
    int charger_id; // 充电桩序号
    int current_A;  // 所需电流 (A)
    int priority;   // 优先级 (0-3)
    int valid;      // 参数是否有效
} PluginCommand;

enum
{
    CLI_CMD_INVALID = 0,
    CLI_CMD_PLUGOUT = 1,
    CLI_CMD_PLUGIN = 2,
};

void handle_plugin(int id, int current_A, int priority)
{
    if (id < 1 || id > sizeof(PwrDemandObj) / sizeof(PwrDemandObj[0]))
    {
        return;
    }
    id -= 1;
    float dummy_V = 400.0f;
    PwrDemandObj[id].power_req = dummy_V * current_A * 0.001f;
    PwrDemandObj[id].priority = priority;
    PwrDemandObj[id].status = PLUGIN_PLUGGED;
}
void handle_plugout(int id)
{
    if (id < 1 || id > sizeof(PwrDemandObj) / sizeof(PwrDemandObj[0]))
    {
        return;
    }
    id -= 1;
    PwrDemandObj[id].power_req = 0.0f;
    PwrDemandObj[id].priority = 0;
    PwrDemandObj[id].status = PLUGIN_UNPLUGGED;
}

// 解析收到的命令
PluginCommand parse_command_plugout(const char *cmd)
{
    PluginCommand result = {0};

    int prefix_len = strlen(COMMAND_OUT_PREFIX);
    int suffix_len = strlen(COMMAND_SUFFIX);
    int cmd_len = strlen(cmd);

    // 检查命令格式是否正确
    if (cmd_len < prefix_len + suffix_len + 2)
    {
        return result;
    }

    // 检查命令前缀和后缀
    if (strncmp(cmd, COMMAND_OUT_PREFIX, prefix_len) != 0 || strncmp(&cmd[cmd_len - suffix_len], COMMAND_SUFFIX, suffix_len) != 0)
    {
        return result;
    }

    // 提取括号内的内容
    char params_str[RTT_RX_BUFFER_SIZE];
    int params_len = cmd_len - prefix_len - suffix_len;
    strncpy(params_str, &cmd[prefix_len], params_len);
    params_str[params_len] = '\0';

    // 解析第一个参数: 充电桩序号
    char *token = strtok(params_str, ",");
    if (token == NULL || token[0] != '#')
    {
        return result;
    }
    result.charger_id = atoi(&token[1]);
    if (result.charger_id <= 0)
    {
        return result;
    }
    // 所有参数解析成功
    result.valid = CLI_CMD_PLUGOUT;
    return result;
}
PluginCommand parse_command_plugin(const char *cmd)
{
    PluginCommand result = {0, 0, 0, 0};
    char temp[32];
    int prefix_len = strlen(COMMAND_IN_PREFIX);
    int suffix_len = strlen(COMMAND_SUFFIX);
    int cmd_len = strlen(cmd);

    // 检查命令格式是否正确
    if (cmd_len < prefix_len + suffix_len + 5)
    {
        return result;
    }

    // 检查命令前缀和后缀
    if (strncmp(cmd, COMMAND_IN_PREFIX, prefix_len) != 0 || strncmp(&cmd[cmd_len - suffix_len], COMMAND_SUFFIX, suffix_len) != 0)
    {
        return result;
    }

    // 提取括号内的内容
    char params_str[RTT_RX_BUFFER_SIZE];
    int params_len = cmd_len - prefix_len - suffix_len;
    strncpy(params_str, &cmd[prefix_len], params_len);
    params_str[params_len] = '\0';

    // 解析第一个参数: 充电桩序号
    char *token = strtok(params_str, ",");
    if (token == NULL || token[0] != '#')
    {
        return result;
    }
    result.charger_id = atoi(&token[1]);
    if (result.charger_id <= 0)
    {
        return result;
    }

    // 解析第二个参数: 需求电流 (A)
    token = strtok(NULL, ",");
    if (token == NULL)
    {
        return result;
    }
    int A_len = strlen("A");
    int power_str_len = strlen(token);
    if (power_str_len < A_len + 1)
    {
        return result;
    }
    if (strncmp(&token[power_str_len - A_len], "A", A_len) != 0)
    {
        return result;
    }
    strncpy(temp, token, power_str_len - A_len);
    temp[power_str_len - A_len] = '\0';
    result.current_A = atoi(temp);
    if (result.current_A <= 0)
    {
        return result;
    }

    // 解析第三个参数: 优先级 (0-3)
    token = strtok(NULL, ",");
    if (token == NULL)
    {
        return result;
    }
    result.priority = atoi(token);
    if (result.priority < PRIOR_VAIN || result.priority > PRIOR_EXTREME)
    {
        return result;
    }

    // 所有参数解析成功
    result.valid = CLI_CMD_PLUGIN;
    return result;
}

// 处理解析后的命令
void handle_command(const PluginCommand *cmd)
{
    if (!cmd->valid)
    {
        print_oneliner("Invalid formatted input");
        return;
    }

    // 在这里添加命令处理逻辑
    // printf("解析到有效命令:\n");
    // printf("  充电桩序号: %d\n", cmd->charger_id);
    // printf("  所需电流: %d A\n", cmd->current_A);
    // printf("  优先级: %d\n", cmd->priority);

    switch (cmd->valid)
    {
    case CLI_CMD_PLUGOUT:
        // print_oneliner("plug#%d out\r\n", cmd->charger_id);
        handle_plugout(cmd->charger_id);
        break;
    case CLI_CMD_PLUGIN:
        // print_oneliner("plug#%d in\r\n", cmd->charger_id);
        handle_plugin(cmd->charger_id, cmd->current_A, cmd->priority);
        break;
    default:
        break;
    }
}

void set_Cmd_of_PDU(PDU_CMD cmd);

void rtt_cli_task(void)
{
    static char rtt_rx_buffer[RTT_RX_BUFFER_SIZE] = {0};
    static int rx_index = 0;
    int bytes_read;
    char c;
    static bool cmd_processed = false; // 新增：标记命令是否已处理，防止重复处理

    // 读取RTT数据
    bytes_read = SEGGER_RTT_Read(0, &c, 1);

    if (bytes_read > 0)
    {
        // 处理换行符，表示命令输入结束
        if (c == '\r' || c == '\n')

        {
            /* 确保只在遇到第一个换行符时处理命令，避免\r\n序列导致重复处理 */
            if (!cmd_processed && rx_index > 0)
            {
                rtt_rx_buffer[rx_index] = '\0'; // 确保字符串终止
                                                // print_oneliner("\n收到命令: %s\r\n", rtt_rx_buffer); // 换行并打印接收到的命令

                // 解析并处理命令
                PluginCommand (*parsefunc)(const char *) = NULL;

                if (strncmp(rtt_rx_buffer, COMMAND_IN_PREFIX, strnlen(COMMAND_IN_PREFIX, RTT_RX_BUFFER_SIZE)) == 0)
                {
                    parsefunc = parse_command_plugin;
                }
                else if (strncmp(rtt_rx_buffer, COMMAND_OUT_PREFIX, strnlen(COMMAND_OUT_PREFIX, RTT_RX_BUFFER_SIZE)) == 0)
                {
                    parsefunc = parse_command_plugout;
                }
                else if (strncmp(rtt_rx_buffer, COMMAND_OUT_SHOWTOPO, strnlen(COMMAND_OUT_SHOWTOPO, RTT_RX_BUFFER_SIZE)) == 0)
                {
                    parsefunc = NULL;
                    set_Cmd_of_PDU(PDU_CMD_VISUAL);
                }
                else
                {
                    print_oneliner("[PDU] Unknown command 👉: %s", rtt_rx_buffer);
                }

                if (parsefunc)
                {
                    PluginCommand cmd = parsefunc(rtt_rx_buffer);
                    handle_command(&cmd);

                    cmd_processed = true; // 标记命令已处理
                }
            }

            // 无论是否处理了命令，遇到换行符都重置缓冲区和索引，准备接收新命令
            // if (!cmd_processed)
            if (c == '\n' && 1 >= rx_index)
            {
                print_oneliner(NULL);
            }
            rx_index = 0;
            memset(rtt_rx_buffer, 0, RTT_RX_BUFFER_SIZE);
        }
        // 处理退格键
        else if (c == '\b')
        {
            if (rx_index > 0)
            {
                rx_index--;
                rtt_rx_buffer[rx_index] = '\0';
                // 通常这里也需要在终端回显退格操作，更新显示，但根据你的注释，回显可能被屏蔽了
            }
        }
        // 正常字符，加入缓冲区
        else
        {
            if (rx_index < RTT_RX_BUFFER_SIZE - 1)
            {
                rtt_rx_buffer[rx_index++] = c;
                cmd_processed = false; // 收到新字符，重置命令处理标记
                // 如果你需要回显字符，可以在这里添加，例如：SEGGER_RTT_Write(0, &c, 1);
            }
            else
            {
                // 缓冲区已满，可以添加处理，例如：打印错误信息或忽略输入
                // print_oneliner("\n错误：输入过长!\r\n");
                // rx_index = 0; // 可选择重置缓冲区
                // memset(rtt_rx_buffer, 0, RTT_RX_BUFFER_SIZE);
                // print_oneliner("");
            }
        }
    }
}