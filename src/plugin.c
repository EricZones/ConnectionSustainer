/*
 * ConnectionSustainer Teamspeak3 plugin
 * Copyright (c) EricZones
 */

#if defined(WIN32) || defined(__WIN32__) || defined(_WIN32)
#pragma warning(disable : 4100) /* Disable Unreferenced parameter warning */
#include <Windows.h>
#endif

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "teamspeak/public_definitions.h"
#include "teamspeak/public_errors.h"
#include "teamspeak/public_errors_rare.h"
#include "teamspeak/public_rare_definitions.h"
#include "ts3_functions.h"

#include "plugin.h"

static struct TS3Functions ts3Functions;

#ifdef _WIN32
#define _strcpy(dest, destSize, src) strcpy_s(dest, destSize, src)
#define snprintf sprintf_s
#else
#define _strcpy(dest, destSize, src)                                                                                                                                                                                                                           \
    {                                                                                                                                                                                                                                                          \
        strncpy(dest, src, destSize - 1);                                                                                                                                                                                                                      \
        (dest)[destSize - 1] = '\0';                                                                                                                                                                                                                           \
    }
#endif

#define PLUGIN_API_VERSION 26

#define PATH_BUFSIZE 512
#define COMMAND_BUFSIZE 128
#define INFODATA_BUFSIZE 128
#define SERVERINFO_BUFSIZE 256
#define CHANNELINFO_BUFSIZE 512
#define RETURNCODE_BUFSIZE 128

static char* pluginID = NULL;
static boolean enabled = false;

typedef struct {
    char* label;
    char* address;
    char* serverPassword;
    char* nickname;
    char* defaultChannel;
    char* defaultChannelPassword;
    uint64 channelID;
    anyID clientID;
} ClientInfo;

typedef struct KeyValueNode {
    uint64 key;
    ClientInfo value;
    struct KeyValueNode* next;
} KeyValueNode;

/*********************************** Connection list ************************************/

KeyValueNode* list = NULL;

KeyValueNode* createEntry(uint64 key, ClientInfo value) {
    KeyValueNode* newNode = (KeyValueNode*)malloc(sizeof(KeyValueNode));
    if (!newNode) {
        printf("Failed allocating memory\n");
        return NULL;
    }
    newNode->key = key;
    newNode->value = value;
    newNode->next = NULL;
    return newNode;
}

void addEntry(KeyValueNode** head, uint64 key, ClientInfo value) {
    KeyValueNode* newNode = createEntry(key, value);
    if (!newNode) return;

    newNode->next = *head;
    *head = newNode;
}

ClientInfo* findEntry(KeyValueNode* head, uint64 key) {
    KeyValueNode* current = head;
    while (current) {
        if (current->key == key) {
            return &current->value;
        }
        current = current->next;
    }
    return NULL;
}

void freeList(KeyValueNode** head) {
    KeyValueNode* current = *head;
    while (current) {
        KeyValueNode* next = current->next;
        free(current);
        current = next;
    }
    *head = NULL;
}

/*********************************** Required functions ************************************/

/* Plugin name */
const char* ts3plugin_name() {
    return "Connection Sustainer";
}

/* Plugin version */
const char* ts3plugin_version() {
    return "1.0";
}

/* Plugin API version */
int ts3plugin_apiVersion() {
    return PLUGIN_API_VERSION;
}

/* Plugin author */
const char* ts3plugin_author() {
    return "EricZones";
}

/* Plugin description */
const char* ts3plugin_description() {
    return "Stay connected on every server you are connected to\nProtection against kicks and one second bans";
}

/* Set callback functions */
void ts3plugin_setFunctionPointers(const struct TS3Functions funcs) {
    ts3Functions = funcs;
}

/* Plugin loading function */
int ts3plugin_init() {
    char appPath[PATH_BUFSIZE];
    char resourcesPath[PATH_BUFSIZE];
    char configPath[PATH_BUFSIZE];
    char pluginPath[PATH_BUFSIZE];

    printf("[%s] Loading plugin...\n", ts3plugin_name());

    ts3Functions.getAppPath(appPath, PATH_BUFSIZE);
    ts3Functions.getResourcesPath(resourcesPath, PATH_BUFSIZE);
    ts3Functions.getConfigPath(configPath, PATH_BUFSIZE);
    ts3Functions.getPluginPath(pluginPath, PATH_BUFSIZE, pluginID);

    return 0;
}

/* Plugin unloading function */
void ts3plugin_shutdown() {
    printf("[%s] Unloading plugin...\n", ts3plugin_name());

    if (pluginID) {
        free(pluginID);
        pluginID = NULL;
    }
    freeList(&list);
}

/*********************************** Optional functions ************************************/

/* Registering plugin id */
void ts3plugin_registerPluginID(const char* id) {
    const size_t sz = strlen(id) + 1;
    pluginID        = (char*)malloc(sz * sizeof(char));
    _strcpy(pluginID, sz, id);
}

/* Handler current server connection changed */
void ts3plugin_currentServerConnectionChanged(uint64 serverConnectionHandlerID) {
    // printf("PLUGIN: currentServerConnectionChanged %llu (%llu)\n", (long long unsigned int)serverConnectionHandlerID, (long long unsigned int)ts3Functions.getCurrentServerConnectionHandlerID());
}

/* Required to release memory allocated in ts3plugin_initMenus */
void ts3plugin_freeMemory(void* data) {
    free(data);
}

/* Menu item creation */
static struct PluginMenuItem* createMenuItem(enum PluginMenuType type, int id, const char* text, const char* icon) {
    struct PluginMenuItem* menuItem = (struct PluginMenuItem*)malloc(sizeof(struct PluginMenuItem));
    menuItem->type                  = type;
    menuItem->id                    = id;
    _strcpy(menuItem->text, PLUGIN_MENU_BUFSZ, text);
    _strcpy(menuItem->icon, PLUGIN_MENU_BUFSZ, icon);
    return menuItem;
}

/* Makros for creating menu items */
#define BEGIN_CREATE_MENUS(x)                                                                                                                                                                                                                                  \
    const size_t sz = x + 1;                                                                                                                                                                                                                                   \
    size_t       n  = 0;                                                                                                                                                                                                                                       \
    *menuItems      = (struct PluginMenuItem**)malloc(sizeof(struct PluginMenuItem*) * sz);
#define CREATE_MENU_ITEM(a, b, c, d) (*menuItems)[n++] = createMenuItem(a, b, c, d);
#define END_CREATE_MENUS                                                                                                                                                                                                                                       \
    (*menuItems)[n++] = NULL;                                                                                                                                                                                                                                  \
    assert(n == sz);

/* Menu IDs for menu items */
enum {
    MENU_ID_GLOBAL_1,
    MENU_ID_GLOBAL_2
};

/* Initialize plugin menus */
void ts3plugin_initMenus(struct PluginMenuItem*** menuItems, char** menuIcon) {
    BEGIN_CREATE_MENUS(2);
    CREATE_MENU_ITEM(PLUGIN_MENU_TYPE_GLOBAL, MENU_ID_GLOBAL_1, "Enable Plugin", "enable.png");
    CREATE_MENU_ITEM(PLUGIN_MENU_TYPE_GLOBAL, MENU_ID_GLOBAL_2, "Disable Plugin", "disable.png");
    END_CREATE_MENUS;

    /* Plugin menu icon */
    *menuIcon = (char*)malloc(PLUGIN_MENU_BUFSZ * sizeof(char));
    _strcpy(*menuIcon, PLUGIN_MENU_BUFSZ, "plugin.png");

    ts3Functions.setPluginMenuEnabled(pluginID, MENU_ID_GLOBAL_2, 0);
}

/*********************************** TeamSpeak callbacks ************************************/

void ts3plugin_onConnectStatusChangeEvent(uint64 serverConnectionHandlerID, int newStatus, unsigned int errorNumber) {
    if (newStatus != STATUS_CONNECTION_ESTABLISHED) return;

    ClientInfo* found = findEntry(list, serverConnectionHandlerID);
    if (found) {
        anyID myID;
        ts3Functions.getClientID(serverConnectionHandlerID, &myID);
        found->clientID = myID;
        ts3Functions.requestClientMove(serverConnectionHandlerID, myID, found->channelID, "", NULL);

    } else {
        anyID myID;
        char* label;
        char* serverPassword;
        char* nickname;
        char* defaultChannel;
        char* defaultChannelPassword;

        uint64 port;
        char* ip;
        ts3Functions.getClientID(serverConnectionHandlerID, &myID);
        ts3Functions.getConnectionVariableAsString(serverConnectionHandlerID, myID, CONNECTION_SERVER_IP, &ip);
        ts3Functions.getConnectionVariableAsUInt64(serverConnectionHandlerID, myID, CONNECTION_SERVER_PORT, &port);

        /* Combining port and ip */
        char buffer[20];
        sprintf(buffer, "%llu", port);
        size_t newSize = strlen(ip) + strlen(buffer) + 2;
        char* address = malloc(newSize);
        strcpy(address, ip);
        strcat(address, ":");
        strcat(address, buffer);

        ts3Functions.getServerVariableAsString(serverConnectionHandlerID, VIRTUALSERVER_NAME, &label);
        ts3Functions.getClientSelfVariableAsString(serverConnectionHandlerID, CLIENT_NICKNAME, &nickname);
        ts3Functions.getClientSelfVariableAsString(serverConnectionHandlerID, CLIENT_DEFAULT_CHANNEL, &defaultChannel);
        ts3Functions.getClientSelfVariableAsString(serverConnectionHandlerID, CLIENT_DEFAULT_CHANNEL_PASSWORD, &defaultChannelPassword);
        ts3Functions.getClientSelfVariableAsString(serverConnectionHandlerID, CLIENT_SERVER_PASSWORD, &serverPassword);

        ClientInfo client = {
            .label = label,
            .address = address,
            .serverPassword = serverPassword,
            .nickname = nickname,
            .defaultChannel = defaultChannel,
            .defaultChannelPassword = defaultChannelPassword,
            .clientID = myID
        };
        addEntry(&list, serverConnectionHandlerID, client);
    }
}

void ts3plugin_onClientKickFromServerEvent(uint64 serverConnectionHandlerID, anyID clientID, uint64 oldChannelID, uint64 newChannelID, int visibility, anyID kickerID, const char* kickerName, const char* kickerUniqueIdentifier, const char* kickMessage) {
    if (!enabled) return;
    ClientInfo* found = findEntry(list, serverConnectionHandlerID);
    if (!found) return;
    if (found->clientID != clientID) return;

    found->channelID = oldChannelID;
    ts3Functions.guiConnect(PLUGIN_CONNECT_TAB_CURRENT, found->label, found->address, found->serverPassword, found->nickname, found->defaultChannel, found->defaultChannelPassword, "", "", "", "", found->nickname, "", "", &serverConnectionHandlerID);
}

void ts3plugin_onClientBanFromServerEvent(uint64 serverConnectionHandlerID, anyID clientID, uint64 oldChannelID, uint64 newChannelID, int visibility, anyID kickerID, const char* kickerName, const char* kickerUniqueIdentifier, uint64 time,
                                          const char* kickMessage) {
    if (!enabled) return;
    ClientInfo* found = findEntry(list, serverConnectionHandlerID);
    if (!found) return;
    if (found->clientID != clientID) return;

    found->channelID = oldChannelID;
    ts3Functions.guiConnect(PLUGIN_CONNECT_TAB_CURRENT, found->label, found->address, found->serverPassword, found->nickname, found->defaultChannel, found->defaultChannelPassword, "", "", "", "", found->nickname, "", "", &serverConnectionHandlerID);
}

/* Client UI callback */
void ts3plugin_onMenuItemEvent(uint64 serverConnectionHandlerID, enum PluginMenuType type, int menuItemID, uint64 selectedItemID) {
    switch (type) {
        case PLUGIN_MENU_TYPE_GLOBAL:
            switch (menuItemID) {
                case MENU_ID_GLOBAL_1:
                    if (enabled) {
                        sendMessage(serverConnectionHandlerID, "[color=black]<[b]Connection Sustainer[/b]> The [color=red]Plugin[/color] is already enabled");
                    } else {
                        enabled = true;
                        ts3Functions.setPluginMenuEnabled(pluginID, menuItemID, 0);
                        ts3Functions.setPluginMenuEnabled(pluginID, MENU_ID_GLOBAL_2, 1);
                        sendMessage(serverConnectionHandlerID, "[color=black]<[b]Connection Sustainer[/b]> The [color=#00aaff]Plugin[/color] has been [color=green]enabled[/color]");
                    }
                    break;
                case MENU_ID_GLOBAL_2:
                    if (enabled) {
                        enabled = false;
                        ts3Functions.setPluginMenuEnabled(pluginID, menuItemID, 0);
                        ts3Functions.setPluginMenuEnabled(pluginID, MENU_ID_GLOBAL_1, 1);
                        sendMessage(serverConnectionHandlerID, "[color=black]<[b]Connection Sustainer[/b]> The [color=#00aaff]Plugin[/color] has been [color=red]disabled[/color]");
                    }
                    else {
                        sendMessage(serverConnectionHandlerID, "[color=black]<[b]Connection Sustainer[/b]> The [color=red]Plugin[/color] is already disabled");
                    }
                    break;
                default:
                    break;
            }
        default:
            break;
    }
}

/* Message to client */
void sendMessage(uint64 serverConnectionHandlerID, const char* message) {
    int status;
    ts3Functions.getConnectionStatus(serverConnectionHandlerID, &status);
    if (status == 0) {
        ts3Functions.printMessageToCurrentTab(message);
        return;
    }
    anyID ownID;
    if (ts3Functions.getClientID(serverConnectionHandlerID, &ownID) != ERROR_ok) return;
    ts3Functions.requestSendPrivateTextMsg(serverConnectionHandlerID, message, ownID, NULL);
}