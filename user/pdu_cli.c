#include "pdu_main.h"

#define COMMAND_OUT_SHOWTOPO "topo"
#define COMMAND_IN_PREFIX "in("
#define COMMAND_OUT_PREFIX "ex("
#define COMMAND_VAR_PREFIX "va("
#define COMMAND_SUFFIX ")"

#define CLI_PRINTF(...) SEGGER_RTT_printf(1, __VA_ARGS__)
/**  1.统一命令解析架构 */
// 命令参数结构体
typedef struct
{
    int charger_id; // 充电桩序号
    int current_A;  // 所需电流 (A)
    int priority;   // 优先级 (0-3)
    int valid;      // 命令类型标识
} PluginCommand;

enum
{
    CLI_CMD_INVALID = 0,
    CLI_CMD_PLUGOUT,
    CLI_CMD_PLUGIN,
    CLI_CMD_VARIATION,
};

// 命令前缀配置表
typedef struct
{
    const char *prefix;
    int cmd_type;
    int param_count; // 参数数量：1=只有id，2=id+电流，3=id+电流+优先级
} CommandConfig;

static const CommandConfig cmd_configs[] = {
    {COMMAND_IN_PREFIX, CLI_CMD_PLUGIN, 3},
    {COMMAND_OUT_PREFIX, CLI_CMD_PLUGOUT, 1},
    {COMMAND_VAR_PREFIX, CLI_CMD_VARIATION, 2},
    {NULL, CLI_CMD_INVALID, 0} // 结束标记
};

/** 2.公共可重用工具函数 */
// 提取括号内的参数字符串
static bool extract_parameters(const char *cmd, const char *prefix, char *params, size_t max_len)
{
    size_t prefix_len = strlen(prefix);
    size_t cmd_len = strlen(cmd);
    size_t suffix_len = strlen(COMMAND_SUFFIX);

    if (cmd_len < prefix_len + suffix_len + 2)
    {
        return false;
    }
    if (strncmp(&cmd[cmd_len - suffix_len], COMMAND_SUFFIX, suffix_len) != 0)
    {
        return false;
    }

    size_t params_len = cmd_len - prefix_len - suffix_len;
    if (params_len >= max_len)
    {
        return false;
    }

    strncpy(params, &cmd[prefix_len], params_len);
    params[params_len] = '\0';
    return true;
}

// 解析充电桩ID (#1格式)
static int parse_charger_id(const char *params)
{
    char *token = strtok((char *)params, ",");
    if (!token || token[0] != '#')
    {
        return 0;
    }

    return atoi(&token[1]);
}

// 解析电流参数 (10A格式)
static int parse_current_param(const char *params)
{
    char temp[32];
    char *saveptr;
    char *token = strtok_r((char *)params, ",", &saveptr);

    // 跳过第一个参数(ID)
    token = strtok_r(NULL, ",", &saveptr);
    if (!token)
    {
        return 0;
    }

    int A_len = strlen("A");
    int token_len = strlen(token);
    if (token_len < A_len + 1)
    {
        return 0;
    }
    if (strncmp(&token[token_len - A_len], "A", A_len) != 0)
    {
        return 0;
    }

    strncpy(temp, token, token_len - A_len);
    temp[token_len - A_len] = '\0';
    return atoi(temp);
}

// 解析优先级参数
static int parse_priority_param(const char *params)
{
    char *saveptr;
    char *token = strtok_r((char *)params, ",", &saveptr);

    // 跳过前两个参数
    token = strtok_r(NULL, ",", &saveptr); // 电流
    token = strtok_r(NULL, ",", &saveptr); // 优先级

    return token ? atoi(token) : 0;
}

/** 3.命令解析参数流 */
// 统一的命令解析-分流函数
PluginCommand parse_split_cmdstrem(const char *cmd)
{
    PluginCommand result = {0};

    if (!cmd || strlen(cmd) < 5)
        {return result;}

    // 识别命令类型
    const CommandConfig *config = NULL;
    for (int i = 0; cmd_configs[i].prefix != NULL; i++)
    {
        if (strncmp(cmd, cmd_configs[i].prefix, strlen(cmd_configs[i].prefix)) == 0)
        {
            config = &cmd_configs[i];
            break;
        }
    }

    if (!config)
       { return result;}

    // 提取括号内的参数
    char params[BUFFER_SIZE_DOWN];

    if (!extract_parameters(cmd, config->prefix, params, sizeof(params)))
    {
        return result;
    }

    // 根据参数数量解析
    switch (config->param_count)
    {
    case 1: // 只有ID：ex(#1)
        result.charger_id = parse_charger_id(params);
        break;

    case 2: // ID + 电流：va(#1,10A)
    {
        char *str_copy = strdup(params);
        result.charger_id = parse_charger_id(str_copy);
        free(str_copy);
    }
        result.current_A = parse_current_param(params);
        break;

    case 3: // ID + 电流 + 优先级：in(#1,10A,2)
    {
        char *str_copy = strdup(params);
        result.charger_id = parse_charger_id(str_copy);
        free(str_copy);
        str_copy = strdup(params);
        result.current_A = parse_current_param(str_copy);
        free(str_copy);
    }
        result.priority = parse_priority_param(params);
        break;
    }

    // 验证必要参数
    if (result.charger_id > 0 &&
        (config->param_count == 1 ||
         (config->param_count >= 2 && result.current_A > 0) &&
             (config->param_count != 3 ||
              (result.priority >= PRIOR_VAIN && result.priority <= PRIOR_EXTREME))))
    {
        result.valid = config->cmd_type;
    }

    return result;
}
/** 4.命令处理逻辑过程 */
// 统一的命令处理器
void handle_command_unified(const PluginCommand *cmd)
{
    if (!cmd->valid)
    {
        print_oneliner("Invalid formatted input");
        return;
    }

    // 参数边界检查
    if (cmd->charger_id < 1 || cmd->charger_id > sizeof(PwrDemandObj) / sizeof(PwrDemandObj[0]))
    {
        return;
    }

    int array_index = cmd->charger_id - 1;
    struct PDU_RET_T ret;

    switch (cmd->valid)
    {
    case CLI_CMD_PLUGIN:
        ret = FSM_mainEntry_PDU(PDU_CMD_PLUGIN, cmd->charger_id, cmd->priority);
        PwrDemandObj[array_index] = (PowerDemand){
            .power_req = 400.0f * cmd->current_A * 0.001f,
            .max_req = cmd->current_A,
            .priority = cmd->priority,
            .voltage_req = 400.0f,
            .status = PLUGIN_PLUGGED};
        break;

    case CLI_CMD_PLUGOUT:
        ret = FSM_mainEntry_PDU(PDU_CMD_PLUGOUT, cmd->charger_id);
        PwrDemandObj[array_index] = (PowerDemand){
            .power_req = 0.0f, .priority = 0, .max_req = 0, .current_req = 0.0f, .status = PLUGIN_UNPLUGGED};
        break;

    case CLI_CMD_VARIATION:
        // 电流变化处理
        PwrDemandObj[array_index].current_req = cmd->current_A;
        PwrDemandObj[array_index].power_req = 400.0f * cmd->current_A * 0.001f;
        break;
    }
}

// 处理完整命令
static void process_complete_command(const char *command)
{
    // 特殊命令处理
    if (strncmp(command, COMMAND_OUT_SHOWTOPO, strlen(COMMAND_OUT_SHOWTOPO)) == 0)
    {
        FSM_mainEntry_PDU(PDU_CMD_VISUAL, true);
        return;
    }

    PluginCommand cmd = parse_split_cmdstrem(command);
    if (cmd.valid)
    {
        handle_command_unified(&cmd);
    }
    else
    {
        print_oneliner("[PDU] Unknown command 👉: %s", command);
    }
}
/** 任务函数定期被main调用 */
void rtt_cli_task(void)
{
    static char rtt_rx_buffer[BUFFER_SIZE_DOWN] = {0};
    static int rx_index = 0;
    static bool cmd_processed = false;
    int bytes_read;
    char c;

    bytes_read = SEGGER_RTT_Read(0, &c, 1);
    if (bytes_read <= 0)
        return;

    switch (c)
    {
    case '\r':
    case '\n': // 命令结束
        if (!cmd_processed && rx_index > 0)
        {
            rtt_rx_buffer[rx_index] = '\0';
            process_complete_command(rtt_rx_buffer);
            cmd_processed = true;
        }
        if (c == '\n' && rx_index <= 1)
        {
            print_oneliner(NULL);
        }
        rx_index = 0;
        memset(rtt_rx_buffer, 0, BUFFER_SIZE_DOWN);
        break;

    case '\b': // 退格处理
        if (rx_index > 0)
            rtt_rx_buffer[--rx_index] = '\0';
        break;

    default: // 正常字符
        if (rx_index < BUFFER_SIZE_DOWN - 1)
        {
            rtt_rx_buffer[rx_index++] = c;
            cmd_processed = false;
        }
        break;
    }
}
