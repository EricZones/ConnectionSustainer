/*
 * ConnectionSustainer Teamspeak3 plugin
 * Copyright (c) EricZones
 */

#ifndef PLUGIN_H
#define PLUGIN_H

#if defined(WIN32) || defined(__WIN32__) || defined(_WIN32)
#define PLUGINS_EXPORTDLL __declspec(dllexport)
#else
#define PLUGINS_EXPORTDLL __attribute__((visibility("default")))
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* Required functions */
PLUGINS_EXPORTDLL const char* ts3plugin_name();
PLUGINS_EXPORTDLL const char* ts3plugin_version();
PLUGINS_EXPORTDLL int         ts3plugin_apiVersion();
PLUGINS_EXPORTDLL const char* ts3plugin_author();
PLUGINS_EXPORTDLL const char* ts3plugin_description();
PLUGINS_EXPORTDLL void        ts3plugin_setFunctionPointers(const struct TS3Functions funcs);
PLUGINS_EXPORTDLL int         ts3plugin_init();
PLUGINS_EXPORTDLL void        ts3plugin_shutdown();

/* Optional functions */
PLUGINS_EXPORTDLL void        ts3plugin_registerPluginID(const char* id);
PLUGINS_EXPORTDLL void        ts3plugin_currentServerConnectionChanged(uint64 serverConnectionHandlerID);
PLUGINS_EXPORTDLL void        ts3plugin_freeMemory(void* data);
PLUGINS_EXPORTDLL void        ts3plugin_initMenus(struct PluginMenuItem*** menuItems, char** menuIcon);

/* Clientlib */
PLUGINS_EXPORTDLL void ts3plugin_onConnectStatusChangeEvent(uint64 serverConnectionHandlerID, int newStatus, unsigned int errorNumber);
PLUGINS_EXPORTDLL void ts3plugin_onClientKickFromServerEvent(uint64 serverConnectionHandlerID, anyID clientID, uint64 oldChannelID, uint64 newChannelID, int visibility, anyID kickerID, const char* kickerName, const char* kickerUniqueIdentifier,
                                                             const char* kickMessage);
PLUGINS_EXPORTDLL void ts3plugin_onClientBanFromServerEvent(uint64 serverConnectionHandlerID, anyID clientID, uint64 oldChannelID, uint64 newChannelID, int visibility, anyID kickerID, const char* kickerName, const char* kickerUniqueIdentifier, uint64 time,
                                                            const char* kickMessage);

/* Client UI callbacks */
PLUGINS_EXPORTDLL void ts3plugin_onMenuItemEvent(uint64 serverConnectionHandlerID, enum PluginMenuType type, int menuItemID, uint64 selectedItemID);

/* Custom methods */
void sendMessage(uint64 serverConnectionHandlerID, const char* message);

#ifdef __cplusplus
}
#endif

#endif