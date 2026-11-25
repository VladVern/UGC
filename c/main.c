#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "cJSON.h"

#define MAX_COMMANDS 128

typedef struct {
    UINT key;
    char *key_name;
    char *command;
    int registered;
    DWORD last_execution;
} Command;

static Command commands[MAX_COMMANDS];
static int command_count = 0;

static UINT key_open_chat = 0;
static UINT key_input_chat = 0;
static UINT key_close_chat = 0;

static int close_chat = 0;
static int cooldown = 0;

int parse_key(const char *key) {
    if (!key || !*key)
        return 0;

    size_t len = strlen(key);

    if (len == 4 && memcmp(key, "Key", 3) == 0) {
        if (key[3] >= 'A' && key[3] <= 'Z')
            return key[3];
    }

    if (len == 6 && memcmp(key, "Digit", 5) == 0) {
        if (key[5] >= '0' && key[5] <= '9')
            return key[5];
    }

    if ((key[0] == 'F' || key[0] == 'f') && key[1]) {
        char *end;
        long n = strtol(key + 1, &end, 10);

        if (*end == '\0' && n >= 1 && n <= 24)
            return VK_F1 + (int)n - 1;
    }

    if (len == 7 && memcmp(key, "Numpad", 6) == 0) {
        if (key[6] >= '0' && key[6] <= '9')
            return VK_NUMPAD0 + key[6] - '0';
    }

    static const struct {
        const char *code;
        int vk;
    } keys[] = {
        {"Escape", VK_ESCAPE},
        {"Enter", VK_RETURN},
        {"Space", VK_SPACE},
        {"Tab", VK_TAB},
        {"Backspace", VK_BACK},
        {"Backspac", VK_BACK},
        {"Delete", VK_DELETE},
        {"Insert", VK_INSERT},

        {"ShiftLeft", VK_LSHIFT},
        {"ShiftRight", VK_RSHIFT},
        {"ControlLeft", VK_LCONTROL},
        {"ControlRight", VK_RCONTROL},
        {"AltLeft", VK_LMENU},
        {"AltRight", VK_RMENU},
        {"MetaLeft", VK_LWIN},
        {"MetaRight", VK_RWIN},

        {"ArrowUp", VK_UP},
        {"ArrowDown", VK_DOWN},
        {"ArrowLeft", VK_LEFT},
        {"ArrowRight", VK_RIGHT},
        {"Home", VK_HOME},
        {"End", VK_END},
        {"PageUp", VK_PRIOR},
        {"PageDown", VK_NEXT},

        {"CapsLock", VK_CAPITAL},
        {"NumLock", VK_NUMLOCK},
        {"ScrollLock", VK_SCROLL},

        {"Pause", VK_PAUSE},
        {"PrintScreen", VK_SNAPSHOT},
        {"ContextMenu", VK_APPS},

        {"NumpadAdd", VK_ADD},
        {"NumpadSubtract", VK_SUBTRACT},
        {"NumpadMultiply", VK_MULTIPLY},
        {"NumpadDivide", VK_DIVIDE},
        {"NumpadDecimal", VK_DECIMAL},
        {"NumpadEnter", VK_RETURN},
        {"NumpadEqual", VK_OEM_NEC_EQUAL},
        {"NumpadComma", VK_SEPARATOR},

        {"Backquote", VK_OEM_3},
        {"Minus", VK_OEM_MINUS},
        {"Equal", VK_OEM_PLUS},
        {"BracketLeft", VK_OEM_4},
        {"BracketRight", VK_OEM_6},
        {"Backslash", VK_OEM_5},
        {"Semicolon", VK_OEM_1},
        {"Quote", VK_OEM_7},
        {"Comma", VK_OEM_COMMA},
        {"Period", VK_OEM_PERIOD},
        {"Slash", VK_OEM_2},

        {"IntlBackslash", VK_OEM_102},
        {"IntlRo", VK_OEM_102},
        {"IntlYen", VK_OEM_5},

        {"Convert", VK_CONVERT},
        {"NonConvert", VK_NONCONVERT},
        {"KanaMode", VK_KANA},
        {"Lang1", VK_KANA},
        {"Lang2", VK_HANJA},
        {"Lang3", VK_JUNJA},
        {"Lang4", VK_FINAL},
        {"Lang5", VK_HANJA},

        {"AudioVolumeMute", VK_VOLUME_MUTE},
        {"AudioVolumeDown", VK_VOLUME_DOWN},
        {"AudioVolumeUp", VK_VOLUME_UP},
        {"MediaTrackNext", VK_MEDIA_NEXT_TRACK},
        {"MediaTrackPrevious", VK_MEDIA_PREV_TRACK},
        {"MediaStop", VK_MEDIA_STOP},
        {"MediaPlayPause", VK_MEDIA_PLAY_PAUSE},

        {"BrowserBack", VK_BROWSER_BACK},
        {"BrowserForward", VK_BROWSER_FORWARD},
        {"BrowserRefresh", VK_BROWSER_REFRESH},
        {"BrowserStop", VK_BROWSER_STOP},
        {"BrowserSearch", VK_BROWSER_SEARCH},
        {"BrowserFavorites", VK_BROWSER_FAVORITES},
        {"BrowserHome", VK_BROWSER_HOME},

        {"LaunchMail", VK_LAUNCH_MAIL},
        {"MediaSelect", VK_LAUNCH_MEDIA_SELECT},
        {"LaunchApp1", VK_LAUNCH_APP1},
        {"LaunchApp2", VK_LAUNCH_APP2},

        {"Help", VK_HELP},
        {"Select", VK_SELECT},
        {"Clear", VK_CLEAR},
        {"Execute", VK_EXECUTE}
    };

    for (size_t i = 0; i < sizeof(keys) / sizeof(keys[0]); i++) {
        if (strcmp(key, keys[i].code) == 0)
            return keys[i].vk;
    }

    return 0;
}


void press_key(UINT key) {
    if (!key)
        return;

    INPUT input[2] = {0};

    UINT scanCode = MapVirtualKeyA(key, MAPVK_VK_TO_VSC);

    if (!scanCode)
        return;

    input[0].type = INPUT_KEYBOARD;
    input[0].ki.wScan = (WORD)scanCode;
    input[0].ki.dwFlags = KEYEVENTF_SCANCODE;

    input[1].type = INPUT_KEYBOARD;
    input[1].ki.wScan = (WORD)scanCode;
    input[1].ki.dwFlags = KEYEVENTF_SCANCODE | KEYEVENTF_KEYUP;

    SendInput(2, input, sizeof(INPUT));
}

void type_text(const char *text) {
    if (!text)
        return;

    int len = MultiByteToWideChar(
        CP_UTF8,
        0,
        text,
        -1,
        NULL,
        0
    );

    if (len <= 1)
        return;

    WCHAR *wide = malloc(len * sizeof(WCHAR));

    if (!wide)
        return;

    MultiByteToWideChar(
        CP_UTF8,
        0,
        text,
        -1,
        wide,
        len
    );

    for (int i = 0; i < len - 1; i++) {
        INPUT input[2] = {0};

        input[0].type = INPUT_KEYBOARD;
        input[0].ki.wScan = wide[i];
        input[0].ki.dwFlags = KEYEVENTF_UNICODE;

        input[1].type = INPUT_KEYBOARD;
        input[1].ki.wScan = wide[i];
        input[1].ki.dwFlags =
            KEYEVENTF_UNICODE | KEYEVENTF_KEYUP;

        SendInput(2, input, sizeof(INPUT));
    }

    free(wide);
}

void execute_command(int index) {
    if (index < 0 || index >= command_count)
        return;

    Command *cmd = &commands[index];

    press_key(key_open_chat);

    Sleep(80);

    type_text(cmd->command);

    if (cooldown > 0)
        Sleep((DWORD)cooldown);

    press_key(key_input_chat);

    if (close_chat)
        press_key(key_close_chat);
}

int load_config(const char *filename) {
    FILE *file = fopen(filename, "rb");

    if (!file) {

        printf("Cannot open config: %s\n", filename);

        return 0;
    }

    fseek(file, 0, SEEK_END);

    long size = ftell(file);

    rewind(file);

    if (size <= 0) {

        fclose(file);

        printf("Config is empty\n");

        return 0;
    }

    char *buffer = malloc((size_t)size + 1);

    if (!buffer) {

        fclose(file);

        return 0;
    }

    size_t bytes_read = fread(
        buffer,
        1,
        (size_t)size,
        file
    );

    fclose(file);

    if (bytes_read != (size_t)size) {

        free(buffer);

        printf("Cannot read config\n");

        return 0;
    }

    buffer[size] = '\0';

    cJSON *root = cJSON_Parse(buffer);

    free(buffer);

    if (!root) {

        printf("Invalid JSON\n");

        return 0;
    }

    cJSON *selected = cJSON_GetObjectItemCaseSensitive(
        root,
        "selected-preset"
    );

    cJSON *presets = cJSON_GetObjectItemCaseSensitive(
        root,
        "presets"
    );

    if (!cJSON_IsString(selected) ||
        !cJSON_IsArray(presets)) {

        printf("Invalid config structure\n");

        cJSON_Delete(root);

        return 0;
    }

    char selected_name[256];

    snprintf(
        selected_name,
        sizeof(selected_name),
        "%s",
        selected->valuestring
    );

    cJSON *preset = NULL;
    cJSON *current = NULL;

    cJSON_ArrayForEach(current, presets) {

        cJSON *name = cJSON_GetObjectItemCaseSensitive(
            current,
            "name"
        );

        if (cJSON_IsString(name) &&
            strcmp(name->valuestring, selected_name) == 0) {

            preset = current;

            break;
        }
    }

    if (!preset) {

        printf("Selected preset not found: %s\n", selected_name);

        cJSON_Delete(root);

        return 0;
    }

    cJSON *cooldown_json = cJSON_GetObjectItemCaseSensitive(
        preset,
        "cooldown"
    );

    if (cJSON_IsNumber(cooldown_json)) {

        cooldown = cooldown_json->valueint;

        if (cooldown < 0)
            cooldown = 0;
    }

    cJSON *open = cJSON_GetObjectItemCaseSensitive(
        preset,
        "key-open-chat"
    );

    cJSON *input = cJSON_GetObjectItemCaseSensitive(
        preset,
        "key-input-chat"
    );

    if (!cJSON_IsString(open) ||
        !cJSON_IsString(input)) {

        printf("Invalid chat keys\n");

        cJSON_Delete(root);

        return 0;
    }

    key_open_chat = parse_key(open->valuestring);

    key_input_chat = parse_key(input->valuestring);

    cJSON *close = cJSON_GetObjectItemCaseSensitive(
        preset,
        "key-close-chat"
    );

    if (cJSON_IsObject(close)) {

        cJSON *is_close = cJSON_GetObjectItemCaseSensitive(
            close,
            "is-close"
        );

        cJSON *close_key = cJSON_GetObjectItemCaseSensitive(
            close,
            "key"
        );

        close_chat = cJSON_IsTrue(is_close);

        if (close_chat && cJSON_IsString(close_key)) {

            key_close_chat = parse_key(close_key->valuestring);
        }
    }

    if (!key_open_chat || !key_input_chat ||
        (close_chat && !key_close_chat)) {

        printf("Invalid chat key configuration\n");

        cJSON_Delete(root);

        return 0;
    }

    cJSON *list = cJSON_GetObjectItemCaseSensitive(
        preset,
        "commands"
    );

    if (!cJSON_IsArray(list)) {

        printf("Commands must be an array\n");

        cJSON_Delete(root);

        return 0;
    }

    cJSON *item = NULL;

    cJSON_ArrayForEach(item, list) {

        if (command_count >= MAX_COMMANDS)
            break;

        cJSON *key = cJSON_GetObjectItemCaseSensitive(
            item,
            "key"
        );

        cJSON *command = cJSON_GetObjectItemCaseSensitive(
            item,
            "command"
        );

        if (!cJSON_IsString(key) ||
            !cJSON_IsString(command))
            continue;

        UINT vk = parse_key(key->valuestring);

        if (!vk) {

            printf("Unknown key: %s\n", key->valuestring);

            continue;
        }

        int index = command_count;

        commands[index].key = vk;

        commands[index].key_name = strdup(
            key->valuestring
        );
        
        commands[index].command = strdup(
            command->valuestring
        );
        
        commands[index].registered = 0;
        commands[index].last_execution = 0;

        if (!commands[index].key_name ||
            !commands[index].command) {

            free(commands[index].key_name);
            free(commands[index].command);

            commands[index].key_name = NULL;
            commands[index].command = NULL;

            continue;
        }

        command_count++;
    }

    cJSON_Delete(root);

    printf("\n");
    printf("Loaded preset: %s\n", selected_name);
    printf("Loaded commands: %d\n", command_count);
    printf("Cooldown: %d ms\n", cooldown);

    return command_count > 0;
}

int register_hotkeys(void) {
    int registered_count = 0;

    for (int i = 0; i < command_count; i++) {
        int duplicate = 0;

        for (int j = 0; j < i; j++) {
            if (commands[j].key == commands[i].key) {

                duplicate = 1;

                break;
            }
        }

        if (duplicate)
            continue;

        if (RegisterHotKey(
            NULL,
            i + 1,
            MOD_NOREPEAT,
            commands[i].key
        )) {
            commands[i].registered = 1;

            registered_count++;

            printf(
                "Registered: %s\n",
                commands[i].key_name
            );

        } else {
            printf(
                "Not registered: %s (error %lu)\n",
                commands[i].key_name,
                GetLastError()
            );
        }
    }

    printf("\nRegistered hotkeys: %d\n", registered_count);

    return registered_count;
}

void unregister_hotkeys(void) {
    for (int i = 0; i < command_count; i++) {
        if (commands[i].registered) {
            UnregisterHotKey(NULL, i + 1);

            commands[i].registered = 0;
        }
    }
}

void free_commands(void) {
    for (int i = 0; i < command_count; i++) {
        free(commands[i].key_name);
        free(commands[i].command);

        commands[i].key_name = NULL;
        commands[i].command = NULL;
    }

    command_count = 0;
}

int main(void) {
    SetConsoleOutputCP(CP_UTF8);

    SetConsoleTitleA("Universal Game Commander");

    printf("====================================\n");
    printf("     UNIVERSAL GAME COMMANDER\n");
    printf("====================================\n\n");

    char exe_path[MAX_PATH];

    GetModuleFileNameA(NULL, exe_path, MAX_PATH);

    char *slash = strrchr(exe_path, '\\');

    if (slash) {
        *(slash + 1) = '\0';
    }

    strcat(exe_path, "config\\config.json");

    if (!load_config(exe_path)) {   
        printf("\nConfig loading failed\n");

        free_commands();

        system("pause");

        return 1;
    }

    printf("\nRegistering hotkeys...\n\n");

    int registered = register_hotkeys();

    if (registered == 0) {
        printf("\nNo hotkeys registered\n");

        free_commands();

        system("pause");

        return 1;
    }

    printf("\n====================================\n");
    printf("Commander is working\n");
    printf("Press your hotkeys to execute commands\n");
    printf("====================================\n\n");

    MSG msg;

    while (GetMessage(&msg, NULL, 0, 0) > 0) {
        if (msg.message != WM_HOTKEY)
            continue;

        int index = (int)msg.wParam - 1;

        if (index < 0 || index >= command_count)
            continue;

        execute_command(index);
    }

    unregister_hotkeys();

    free_commands();

    printf("\nCommander stopped\n");

    return 0;
}

// Compile
// /mingw64/bin/gcc main.c cJSON.c -o commander.exe -luser32 -static -static-libgcc