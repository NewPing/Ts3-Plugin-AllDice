/*
 * TeamSpeak 3 plugin
 *
 * Copyright (c) TeamSpeak Systems GmbH
 */

#if defined(WIN32) || defined(__WIN32__) || defined(_WIN32)
#pragma warning (disable : 4100)  /* Disable Unreferenced parameter warning */
#include <Windows.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "teamspeak/public_errors.h"
#include "teamspeak/public_errors_rare.h"
#include "teamspeak/public_definitions.h"
#include "teamspeak/public_rare_definitions.h"
#include "teamspeak/clientlib_publicdefinitions.h"
#include "ts3_functions.h"
#include "plugin.h"
#include <stdlib.h>     /* srand, rand */
#include <time.h>       /* time */

typedef int bool;
#define true 1
#define false 0

char subString[19999];
int randomNumberCounter;
char userColorColor[19999][19999];
char tmpUserColor[19999];
bool chatBotActive = false;

static struct TS3Functions ts3Functions;

#ifdef _WIN32
#define _strcpy(dest, destSize, src) strcpy_s(dest, destSize, src)
#define snprintf sprintf_s
#else
#define _strcpy(dest, destSize, src) { strncpy(dest, src, destSize-1); (dest)[destSize-1] = '\0'; }
#endif

#define PLUGIN_API_VERSION 23

#define PATH_BUFSIZE 512
#define COMMAND_BUFSIZE 128
#define INFODATA_BUFSIZE 128
#define SERVERINFO_BUFSIZE 256
#define CHANNELINFO_BUFSIZE 512
#define RETURNCODE_BUFSIZE 128

static char* pluginID = NULL;

#ifdef _WIN32
/* Helper function to convert wchar_T to Utf-8 encoded strings on Windows */
static int wcharToUtf8(const wchar_t* str, char** result) {
	int outlen = WideCharToMultiByte(CP_UTF8, 0, str, -1, 0, 0, 0, 0);
	*result = (char*)malloc(outlen);
	if(WideCharToMultiByte(CP_UTF8, 0, str, -1, *result, outlen, 0, 0) == 0) {
		*result = NULL;
		return -1;
	}
	return 0;
}
#endif

/*********************************** Required functions ************************************/
/*
 * If any of these required functions is not implemented, TS3 will refuse to load the plugin
 */

/* Unique name identifying this plugin */
const char* ts3plugin_name() {
#ifdef _WIN32
	/* TeamSpeak expects UTF-8 encoded characters. Following demonstrates a possibility how to convert UTF-16 wchar_t into UTF-8. */
	static char* result = NULL;  /* Static variable so it's allocated only once */
	if(!result) {
		const wchar_t* name = L"ZZW DiceBot";
		if(wcharToUtf8(name, &result) == -1) {  /* Convert name into UTF-8 encoded result */
			result = "ZZW DiceBot";  /* Conversion failed, fallback here */
		}
	}
	return result;
#else
	return "ZZW DiceBot";
#endif
}

/* Plugin version */
const char* ts3plugin_version() {
    return "0.17";
}

/* Plugin API version. Must be the same as the clients API major version, else the plugin fails to load. */
int ts3plugin_apiVersion() {
	return PLUGIN_API_VERSION;
}

/* Plugin author */
const char* ts3plugin_author() {
	/* If you want to use wchar_t, see ts3plugin_name() on how to use */
    return "Jan Roth";
}

/* Plugin description */
const char* ts3plugin_description() {
	/* If you want to use wchar_t, see ts3plugin_name() on how to use */
    return "ZZW Plugin, um das Wuerfeln per TS3-Chat zum ermoeglichen.";
}

/* Set TeamSpeak 3 callback functions */
void ts3plugin_setFunctionPointers(const struct TS3Functions funcs) {
    ts3Functions = funcs;
}

/*
 * Custom code called right after loading the plugin. Returns 0 on success, 1 on failure.
 * If the function returns 1 on failure, the plugin will be unloaded again.
 */
int ts3plugin_init() {
    char appPath[PATH_BUFSIZE];
    char resourcesPath[PATH_BUFSIZE];
    char configPath[PATH_BUFSIZE];
	char pluginPath[PATH_BUFSIZE];

    /* Your plugin init code here */
    //printf("PLUGIN: init\n");

    /* Example on how to query application, resources and configuration paths from client */
    /* Note: Console client returns empty string for app and resources path */
    ts3Functions.getAppPath(appPath, PATH_BUFSIZE);
    ts3Functions.getResourcesPath(resourcesPath, PATH_BUFSIZE);
    ts3Functions.getConfigPath(configPath, PATH_BUFSIZE);
	ts3Functions.getPluginPath(pluginPath, PATH_BUFSIZE, pluginID);

	//printf("PLUGIN: App path: %s\nResources path: %s\nConfig path: %s\nPlugin path: %s\n", appPath, resourcesPath, configPath, pluginPath);

    return 0;  /* 0 = success, 1 = failure, -2 = failure but client will not show a "failed to load" warning */
	/* -2 is a very special case and should only be used if a plugin displays a dialog (e.g. overlay) asking the user to disable
	 * the plugin again, avoiding the show another dialog by the client telling the user the plugin failed to load.
	 * For normal case, if a plugin really failed to load because of an error, the correct return value is 1. */
}

/* Custom code called right before the plugin is unloaded */
void ts3plugin_shutdown() {
    /* Your plugin cleanup code here */
    //printf("PLUGIN: shutdown\n");

	/*
	 * Note:
	 * If your plugin implements a settings dialog, it must be closed and deleted here, else the
	 * TeamSpeak client will most likely crash (DLL removed but dialog from DLL code still open).
	 */

	/* Free pluginID if we registered it */
	if(pluginID) {
		free(pluginID);
		pluginID = NULL;
	}
}

/****************************** Optional functions ********************************/
/*
 * Following functions are optional, if not needed you don't need to implement them.
 */

/* Tell client if plugin offers a configuration window. If this function is not implemented, it's an assumed "does not offer" (PLUGIN_OFFERS_NO_CONFIGURE). */
int ts3plugin_offersConfigure() {
	//printf("PLUGIN: offersConfigure\n");
	/*
	 * Return values:
	 * PLUGIN_OFFERS_NO_CONFIGURE         - Plugin does not implement ts3plugin_configure
	 * PLUGIN_OFFERS_CONFIGURE_NEW_THREAD - Plugin does implement ts3plugin_configure and requests to run this function in an own thread
	 * PLUGIN_OFFERS_CONFIGURE_QT_THREAD  - Plugin does implement ts3plugin_configure and requests to run this function in the Qt GUI thread
	 */
	return PLUGIN_OFFERS_NO_CONFIGURE;  /* In this case ts3plugin_configure does not need to be implemented */
}

/* Plugin might offer a configuration window. If ts3plugin_offersConfigure returns 0, this function does not need to be implemented. */
void ts3plugin_configure(void* handle, void* qParentWidget) {
    //printf("PLUGIN: configure\n");
}

/*
 * If the plugin wants to use error return codes, plugin commands, hotkeys or menu items, it needs to register a command ID. This function will be
 * automatically called after the plugin was initialized. This function is optional. If you don't use these features, this function can be omitted.
 * Note the passed pluginID parameter is no longer valid after calling this function, so you must copy it and store it in the plugin.
 */
void ts3plugin_registerPluginID(const char* id) {
	const size_t sz = strlen(id) + 1;
	pluginID = (char*)malloc(sz * sizeof(char));
	_strcpy(pluginID, sz, id);  /* The id buffer will invalidate after exiting this function */
	printf("PLUGIN: registerPluginID: %s\n", pluginID);
}

/* Plugin command keyword. Return NULL or "" if not used. */
const char* ts3plugin_commandKeyword() {
	return "";
}

static void print_and_free_bookmarks_list(struct PluginBookmarkList* list)
{
    /*int i;
    for (i = 0; i < list->itemcount; ++i) {
        if (list->items[i].isFolder) {
            printf("Folder: name=%s\n", list->items[i].name);
            print_and_free_bookmarks_list(list->items[i].folder);
            ts3Functions.freeMemory(list->items[i].name);
        } else {
            printf("Bookmark: name=%s uuid=%s\n", list->items[i].name, list->items[i].uuid);
            ts3Functions.freeMemory(list->items[i].name);
            ts3Functions.freeMemory(list->items[i].uuid);
        }
    }*/
    //ts3Functions.freeMemory(list);
}

/* Plugin processes console command. Return 0 if plugin handled the command, 1 if not handled. */
int ts3plugin_processCommand(uint64 serverConnectionHandlerID, const char* command) {
//	char buf[COMMAND_BUFSIZE];
//	char *s, *param1 = NULL, *param2 = NULL;
//	int i = 0;
//	enum { CMD_NONE = 0, CMD_JOIN, CMD_COMMAND, CMD_SERVERINFO, CMD_CHANNELINFO, CMD_AVATAR, CMD_ENABLEMENU, CMD_SUBSCRIBE, CMD_UNSUBSCRIBE, CMD_SUBSCRIBEALL, CMD_UNSUBSCRIBEALL, CMD_BOOKMARKSLIST } cmd = CMD_NONE;
//#ifdef _WIN32
//	char* context = NULL;
//#endif
//
//	printf("PLUGIN: process command: '%s'\n", command);
//
//	_strcpy(buf, COMMAND_BUFSIZE, command);
//#ifdef _WIN32
//	s = strtok_s(buf, " ", &context);
//#else
//	s = strtok(buf, " ");
//#endif
//	while(s != NULL) {
//		if(i == 0) {
//			if(!strcmp(s, "join")) {
//				cmd = CMD_JOIN;
//			} else if(!strcmp(s, "command")) {
//				cmd = CMD_COMMAND;
//			} else if(!strcmp(s, "serverinfo")) {
//				cmd = CMD_SERVERINFO;
//			} else if(!strcmp(s, "channelinfo")) {
//				cmd = CMD_CHANNELINFO;
//			} else if(!strcmp(s, "avatar")) {
//				cmd = CMD_AVATAR;
//			} else if(!strcmp(s, "enablemenu")) {
//				cmd = CMD_ENABLEMENU;
//			} else if(!strcmp(s, "subscribe")) {
//				cmd = CMD_SUBSCRIBE;
//			} else if(!strcmp(s, "unsubscribe")) {
//				cmd = CMD_UNSUBSCRIBE;
//			} else if(!strcmp(s, "subscribeall")) {
//				cmd = CMD_SUBSCRIBEALL;
//			} else if(!strcmp(s, "unsubscribeall")) {
//				cmd = CMD_UNSUBSCRIBEALL;
//            } else if (!strcmp(s, "bookmarkslist")) {
//                cmd = CMD_BOOKMARKSLIST;
//            }
//		} else if(i == 1) {
//			param1 = s;
//		} else {
//			param2 = s;
//		}
//#ifdef _WIN32
//		s = strtok_s(NULL, " ", &context);
//#else
//		s = strtok(NULL, " ");
//#endif
//		i++;
//	}
//
//	switch(cmd) {
//		case CMD_NONE:
//			return 1;  /* Command not handled by plugin */
//		case CMD_JOIN:  /* /test join <channelID> [optionalCannelPassword] */
//			if(param1) {
//				uint64 channelID = (uint64)atoi(param1);
//				char* password = param2 ? param2 : "";
//				char returnCode[RETURNCODE_BUFSIZE];
//				anyID myID;
//
//				/* Get own clientID */
//				if(ts3Functions.getClientID(serverConnectionHandlerID, &myID) != ERROR_ok) {
//					ts3Functions.logMessage("Error querying client ID", LogLevel_ERROR, "Plugin", serverConnectionHandlerID);
//					break;
//				}
//
//				/* Create return code for requestClientMove function call. If creation fails, returnCode will be NULL, which can be
//				 * passed into the client functions meaning no return code is used.
//				 * Note: To use return codes, the plugin needs to register a plugin ID using ts3plugin_registerPluginID */
//				ts3Functions.createReturnCode(pluginID, returnCode, RETURNCODE_BUFSIZE);
//
//				/* In a real world plugin, the returnCode should be remembered (e.g. in a dynamic STL vector, if it's a C++ plugin).
//				 * onServerErrorEvent can then check the received returnCode, compare with the remembered ones and thus identify
//				 * which function call has triggered the event and react accordingly. */
//
//				/* Request joining specified channel using above created return code */
//				if(ts3Functions.requestClientMove(serverConnectionHandlerID, myID, channelID, password, returnCode) != ERROR_ok) {
//					ts3Functions.logMessage("Error requesting client move", LogLevel_INFO, "Plugin", serverConnectionHandlerID);
//				}
//			} else {
//				ts3Functions.printMessageToCurrentTab("Missing channel ID parameter.");
//			}
//			break;
//		case CMD_COMMAND:  /* /test command <command> */
//			if(param1) {
//				/* Send plugin command to all clients in current channel. In this case targetIds is unused and can be NULL. */
//				if(pluginID) {
//					/* See ts3plugin_registerPluginID for how to obtain a pluginID */
//					printf("PLUGIN: Sending plugin command to current channel: %s\n", param1);
//					ts3Functions.sendPluginCommand(serverConnectionHandlerID, pluginID, param1, PluginCommandTarget_CURRENT_CHANNEL, NULL, NULL);
//				} else {
//					printf("PLUGIN: Failed to send plugin command, was not registered.\n");
//				}
//			} else {
//				ts3Functions.printMessageToCurrentTab("Missing command parameter.");
//			}
//			break;
//		case CMD_SERVERINFO: {  /* /test serverinfo */
//			/* Query host, port and server password of current server tab.
//			 * The password parameter can be NULL if the plugin does not want to receive the server password.
//			 * Note: Server password is only available if the user has actually used it when connecting. If a user has
//			 * connected with the permission to ignore passwords (b_virtualserver_join_ignore_password) and the password,
//			 * was not entered, it will not be available.
//			 * getServerConnectInfo returns 0 on success, 1 on error or if current server tab is disconnected. */
//			char host[SERVERINFO_BUFSIZE];
//			/*char password[SERVERINFO_BUFSIZE];*/
//			char* password = NULL;  /* Don't receive server password */
//			unsigned short port;
//			if(!ts3Functions.getServerConnectInfo(serverConnectionHandlerID, host, &port, password, SERVERINFO_BUFSIZE)) {
//				char msg[SERVERINFO_BUFSIZE];
//				snprintf(msg, sizeof(msg), "Server Connect Info: %s:%d", host, port);
//				ts3Functions.printMessageToCurrentTab(msg);
//			} else {
//				ts3Functions.printMessageToCurrentTab("No server connect info available.");
//			}
//			break;
//		}
//		case CMD_CHANNELINFO: {  /* /test channelinfo */
//			/* Query channel path and password of current server tab.
//			 * The password parameter can be NULL if the plugin does not want to receive the channel password.
//			 * Note: Channel password is only available if the user has actually used it when entering the channel. If a user has
//			 * entered a channel with the permission to ignore passwords (b_channel_join_ignore_password) and the password,
//			 * was not entered, it will not be available.
//			 * getChannelConnectInfo returns 0 on success, 1 on error or if current server tab is disconnected. */
//			char path[CHANNELINFO_BUFSIZE];
//			/*char password[CHANNELINFO_BUFSIZE];*/
//			char* password = NULL;  /* Don't receive channel password */
//
//			/* Get own clientID and channelID */
//			anyID myID;
//			uint64 myChannelID;
//			if(ts3Functions.getClientID(serverConnectionHandlerID, &myID) != ERROR_ok) {
//				ts3Functions.logMessage("Error querying client ID", LogLevel_ERROR, "Plugin", serverConnectionHandlerID);
//				break;
//			}
//			/* Get own channel ID */
//			if(ts3Functions.getChannelOfClient(serverConnectionHandlerID, myID, &myChannelID) != ERROR_ok) {
//				ts3Functions.logMessage("Error querying channel ID", LogLevel_ERROR, "Plugin", serverConnectionHandlerID);
//				break;
//			}
//
//			/* Get channel connect info of own channel */
//			if(!ts3Functions.getChannelConnectInfo(serverConnectionHandlerID, myChannelID, path, password, CHANNELINFO_BUFSIZE)) {
//				char msg[CHANNELINFO_BUFSIZE];
//				snprintf(msg, sizeof(msg), "Channel Connect Info: %s", path);
//				ts3Functions.printMessageToCurrentTab(msg);
//			} else {
//				ts3Functions.printMessageToCurrentTab("No channel connect info available.");
//			}
//			break;
//		}
//		case CMD_AVATAR: {  /* /test avatar <clientID> */
//			char avatarPath[PATH_BUFSIZE];
//			anyID clientID = (anyID)atoi(param1);
//			unsigned int error;
//
//			memset(avatarPath, 0, PATH_BUFSIZE);
//			error = ts3Functions.getAvatar(serverConnectionHandlerID, clientID, avatarPath, PATH_BUFSIZE);
//			if(error == ERROR_ok) {  /* ERROR_ok means the client has an avatar set. */
//				if(strlen(avatarPath)) {  /* Avatar path contains the full path to the avatar image in the TS3Client cache directory */
//					printf("Avatar path: %s\n", avatarPath);
//				} else { /* Empty avatar path means the client has an avatar but the image has not yet been cached. The TeamSpeak
//						  * client will automatically start the download and call onAvatarUpdated when done */
//					printf("Avatar not yet downloaded, waiting for onAvatarUpdated...\n");
//				}
//			} else if(error == ERROR_database_empty_result) {  /* Not an error, the client simply has no avatar set */
//				printf("Client has no avatar\n");
//			} else { /* Other error occured (invalid server connection handler ID, invalid client ID, file io error etc) */
//				printf("Error getting avatar: %d\n", error);
//			}
//			break;
//		}
//		case CMD_ENABLEMENU:  /* /test enablemenu <menuID> <0|1> */
//			if(param1) {
//				int menuID = atoi(param1);
//				int enable = param2 ? atoi(param2) : 0;
//				ts3Functions.setPluginMenuEnabled(pluginID, menuID, enable);
//			} else {
//				ts3Functions.printMessageToCurrentTab("Usage is: /test enablemenu <menuID> <0|1>");
//			}
//			break;
//		case CMD_SUBSCRIBE:  /* /test subscribe <channelID> */
//			if(param1) {
//				char returnCode[RETURNCODE_BUFSIZE];
//				uint64 channelIDArray[2];
//				channelIDArray[0] = (uint64)atoi(param1);
//				channelIDArray[1] = 0;
//				ts3Functions.createReturnCode(pluginID, returnCode, RETURNCODE_BUFSIZE);
//				if(ts3Functions.requestChannelSubscribe(serverConnectionHandlerID, channelIDArray, returnCode) != ERROR_ok) {
//					ts3Functions.logMessage("Error subscribing channel", LogLevel_INFO, "Plugin", serverConnectionHandlerID);
//				}
//			}
//			break;
//		case CMD_UNSUBSCRIBE:  /* /test unsubscribe <channelID> */
//			if(param1) {
//				char returnCode[RETURNCODE_BUFSIZE];
//				uint64 channelIDArray[2];
//				channelIDArray[0] = (uint64)atoi(param1);
//				channelIDArray[1] = 0;
//				ts3Functions.createReturnCode(pluginID, returnCode, RETURNCODE_BUFSIZE);
//				if(ts3Functions.requestChannelUnsubscribe(serverConnectionHandlerID, channelIDArray, NULL) != ERROR_ok) {
//					ts3Functions.logMessage("Error unsubscribing channel", LogLevel_INFO, "Plugin", serverConnectionHandlerID);
//				}
//			}
//			break;
//		case CMD_SUBSCRIBEALL: {  /* /test subscribeall */
//			char returnCode[RETURNCODE_BUFSIZE];
//			ts3Functions.createReturnCode(pluginID, returnCode, RETURNCODE_BUFSIZE);
//			if(ts3Functions.requestChannelSubscribeAll(serverConnectionHandlerID, returnCode) != ERROR_ok) {
//				ts3Functions.logMessage("Error subscribing channel", LogLevel_INFO, "Plugin", serverConnectionHandlerID);
//			}
//			break;
//		}
//		case CMD_UNSUBSCRIBEALL: {  /* /test unsubscribeall */
//			char returnCode[RETURNCODE_BUFSIZE];
//			ts3Functions.createReturnCode(pluginID, returnCode, RETURNCODE_BUFSIZE);
//			if(ts3Functions.requestChannelUnsubscribeAll(serverConnectionHandlerID, returnCode) != ERROR_ok) {
//				ts3Functions.logMessage("Error subscribing channel", LogLevel_INFO, "Plugin", serverConnectionHandlerID);
//			}
//			break;
//		}
//        case CMD_BOOKMARKSLIST: {  /* test bookmarkslist */
//            struct PluginBookmarkList* list;
//            unsigned int error = ts3Functions.getBookmarkList(&list);
//            if (error == ERROR_ok) {
//                print_and_free_bookmarks_list(list);
//            }
//            else {
//                ts3Functions.logMessage("Error getting bookmarks list", LogLevel_ERROR, "Plugin", serverConnectionHandlerID);
//            }
//            break;
//        }
//	}

	return 0;  /* Plugin handled command */
}

/* Client changed current server connection handler */
void ts3plugin_currentServerConnectionChanged(uint64 serverConnectionHandlerID) {
}

/*
 * Implement the following three functions when the plugin should display a line in the server/channel/client info.
 * If any of ts3plugin_infoTitle, ts3plugin_infoData or ts3plugin_freeMemory is missing, the info text will not be displayed.
 */

/* Static title shown in the left column in the info frame */
const char* ts3plugin_infoTitle() {
	return "ZZW DiceBot";
}

/*
 * Dynamic content shown in the right column in the info frame. Memory for the data string needs to be allocated in this
 * function. The client will call ts3plugin_freeMemory once done with the string to release the allocated memory again.
 * Check the parameter "type" if you want to implement this feature only for specific item types. Set the parameter
 * "data" to NULL to have the client ignore the info data.
 */
void ts3plugin_infoData(uint64 serverConnectionHandlerID, uint64 id, enum PluginItemType type, char** data) {
	//char* name;

	///* For demonstration purpose, display the name of the currently selected server, channel or client. */
	//switch(type) {
	//	case PLUGIN_SERVER:
	//		if(ts3Functions.getServerVariableAsString(serverConnectionHandlerID, VIRTUALSERVER_NAME, &name) != ERROR_ok) {
	//			printf("Error getting virtual server name\n");
	//			return;
	//		}
	//		break;
	//	case PLUGIN_CHANNEL:
	//		if(ts3Functions.getChannelVariableAsString(serverConnectionHandlerID, id, CHANNEL_NAME, &name) != ERROR_ok) {
	//			printf("Error getting channel name\n");
	//			return;
	//		}
	//		break;
	//	case PLUGIN_CLIENT:
	//		if(ts3Functions.getClientVariableAsString(serverConnectionHandlerID, (anyID)id, CLIENT_NICKNAME, &name) != ERROR_ok) {
	//			printf("Error getting client nickname\n");
	//			return;
	//		}
	//		break;
	//	default:
	//		printf("Invalid item type: %d\n", type);
	//		data = NULL;  /* Ignore */
	//		return;
	//}

	//*data = (char*)malloc(INFODATA_BUFSIZE * sizeof(char));  /* Must be allocated in the plugin! */
	//snprintf(*data, INFODATA_BUFSIZE, "The nickname is [I]\"%s\"[/I]", name);  /* bbCode is supported. HTML is not supported */
	//ts3Functions.freeMemory(name);
}

/* Required to release the memory for parameter "data" allocated in ts3plugin_infoData and ts3plugin_initMenus */
void ts3plugin_freeMemory(void* data) {
	free(data);
}

/*
 * Plugin requests to be always automatically loaded by the TeamSpeak 3 client unless
 * the user manually disabled it in the plugin dialog.
 * This function is optional. If missing, no autoload is assumed.
 */
int ts3plugin_requestAutoload() {
	return 0;  /* 1 = request autoloaded, 0 = do not request autoload */
}

///* Helper function to create a menu item */
//static struct PluginMenuItem* createMenuItem(enum PluginMenuType type, int id, const char* text, const char* icon) {
//	struct PluginMenuItem* menuItem = (struct PluginMenuItem*)malloc(sizeof(struct PluginMenuItem));
//	menuItem->type = type;
//	menuItem->id = id;
//	_strcpy(menuItem->text, PLUGIN_MENU_BUFSZ, text);
//	_strcpy(menuItem->icon, PLUGIN_MENU_BUFSZ, icon);
//	return menuItem;
//}
//
///* Some makros to make the code to create menu items a bit more readable */
//#define BEGIN_CREATE_MENUS(x) const size_t sz = x + 1; size_t n = 0; *menuItems = (struct PluginMenuItem**)malloc(sizeof(struct PluginMenuItem*) * sz);
//#define CREATE_MENU_ITEM(a, b, c, d) (*menuItems)[n++] = createMenuItem(a, b, c, d);
//#define END_CREATE_MENUS (*menuItems)[n++] = NULL; assert(n == sz);
//
///*
// * Menu IDs for this plugin. Pass these IDs when creating a menuitem to the TS3 client. When the menu item is triggered,
// * ts3plugin_onMenuItemEvent will be called passing the menu ID of the triggered menu item.
// * These IDs are freely choosable by the plugin author. It's not really needed to use an enum, it just looks prettier.
// */
//enum {
//	MENU_ID_CLIENT_1 = 1,
//	MENU_ID_CLIENT_2,
//	MENU_ID_CHANNEL_1,
//	MENU_ID_CHANNEL_2,
//	MENU_ID_CHANNEL_3,
//	MENU_ID_GLOBAL_1,
//	MENU_ID_GLOBAL_2
//};
//
///*
// * Initialize plugin menus.
// * This function is called after ts3plugin_init and ts3plugin_registerPluginID. A pluginID is required for plugin menus to work.
// * Both ts3plugin_registerPluginID and ts3plugin_freeMemory must be implemented to use menus.
// * If plugin menus are not used by a plugin, do not implement this function or return NULL.
// */
//void ts3plugin_initMenus(struct PluginMenuItem*** menuItems, char** menuIcon) {
//	/*
//	 * Create the menus
//	 * There are three types of menu items:
//	 * - PLUGIN_MENU_TYPE_CLIENT:  Client context menu
//	 * - PLUGIN_MENU_TYPE_CHANNEL: Channel context menu
//	 * - PLUGIN_MENU_TYPE_GLOBAL:  "Plugins" menu in menu bar of main window
//	 *
//	 * Menu IDs are used to identify the menu item when ts3plugin_onMenuItemEvent is called
//	 *
//	 * The menu text is required, max length is 128 characters
//	 *
//	 * The icon is optional, max length is 128 characters. When not using icons, just pass an empty string.
//	 * Icons are loaded from a subdirectory in the TeamSpeak client plugins folder. The subdirectory must be named like the
//	 * plugin filename, without dll/so/dylib suffix
//	 * e.g. for "test_plugin.dll", icon "1.png" is loaded from <TeamSpeak 3 Client install dir>\plugins\test_plugin\1.png
//	 */
//
//	BEGIN_CREATE_MENUS(7);  /* IMPORTANT: Number of menu items must be correct! */
//	CREATE_MENU_ITEM(PLUGIN_MENU_TYPE_CLIENT,  MENU_ID_CLIENT_1,  "Client item 1",  "1.png");
//	CREATE_MENU_ITEM(PLUGIN_MENU_TYPE_CLIENT,  MENU_ID_CLIENT_2,  "Client item 2",  "2.png");
//	CREATE_MENU_ITEM(PLUGIN_MENU_TYPE_CHANNEL, MENU_ID_CHANNEL_1, "Channel item 1", "1.png");
//	CREATE_MENU_ITEM(PLUGIN_MENU_TYPE_CHANNEL, MENU_ID_CHANNEL_2, "Channel item 2", "2.png");
//	CREATE_MENU_ITEM(PLUGIN_MENU_TYPE_CHANNEL, MENU_ID_CHANNEL_3, "Channel item 3", "3.png");
//	CREATE_MENU_ITEM(PLUGIN_MENU_TYPE_GLOBAL,  MENU_ID_GLOBAL_1,  "Global item 1",  "1.png");
//	CREATE_MENU_ITEM(PLUGIN_MENU_TYPE_GLOBAL,  MENU_ID_GLOBAL_2,  "Global item 2",  "2.png");
//	END_CREATE_MENUS;  /* Includes an assert checking if the number of menu items matched */
//
//	/*
//	 * Specify an optional icon for the plugin. This icon is used for the plugins submenu within context and main menus
//	 * If unused, set menuIcon to NULL
//	 */
//	*menuIcon = (char*)malloc(PLUGIN_MENU_BUFSZ * sizeof(char));
//	_strcpy(*menuIcon, PLUGIN_MENU_BUFSZ, "t.png");
//
//	/*
//	 * Menus can be enabled or disabled with: ts3Functions.setPluginMenuEnabled(pluginID, menuID, 0|1);
//	 * Test it with plugin command: /test enablemenu <menuID> <0|1>
//	 * Menus are enabled by default. Please note that shown menus will not automatically enable or disable when calling this function to
//	 * ensure Qt menus are not modified by any thread other the UI thread. The enabled or disable state will change the next time a
//	 * menu is displayed.
//	 */
//	/* For example, this would disable MENU_ID_GLOBAL_2: */
//	/* ts3Functions.setPluginMenuEnabled(pluginID, MENU_ID_GLOBAL_2, 0); */
//
//	/* All memory allocated in this function will be automatically released by the TeamSpeak client later by calling ts3plugin_freeMemory */
//}
//
///* Helper function to create a hotkey */
//static struct PluginHotkey* createHotkey(const char* keyword, const char* description) {
//	struct PluginHotkey* hotkey = (struct PluginHotkey*)malloc(sizeof(struct PluginHotkey));
//	_strcpy(hotkey->keyword, PLUGIN_HOTKEY_BUFSZ, keyword);
//	_strcpy(hotkey->description, PLUGIN_HOTKEY_BUFSZ, description);
//	return hotkey;
//}
//
///* Some makros to make the code to create hotkeys a bit more readable */
//#define BEGIN_CREATE_HOTKEYS(x) const size_t sz = x + 1; size_t n = 0; *hotkeys = (struct PluginHotkey**)malloc(sizeof(struct PluginHotkey*) * sz);
//#define CREATE_HOTKEY(a, b) (*hotkeys)[n++] = createHotkey(a, b);
//#define END_CREATE_HOTKEYS (*hotkeys)[n++] = NULL; assert(n == sz);
//
///*
// * Initialize plugin hotkeys. If your plugin does not use this feature, this function can be omitted.
// * Hotkeys require ts3plugin_registerPluginID and ts3plugin_freeMemory to be implemented.
// * This function is automatically called by the client after ts3plugin_init.
// */
//void ts3plugin_initHotkeys(struct PluginHotkey*** hotkeys) {
//	/* Register hotkeys giving a keyword and a description.
//	 * The keyword will be later passed to ts3plugin_onHotkeyEvent to identify which hotkey was triggered.
//	 * The description is shown in the clients hotkey dialog. */
//	BEGIN_CREATE_HOTKEYS(3);  /* Create 3 hotkeys. Size must be correct for allocating memory. */
//	CREATE_HOTKEY("keyword_1", "Test hotkey 1");
//	CREATE_HOTKEY("keyword_2", "Test hotkey 2");
//	CREATE_HOTKEY("keyword_3", "Test hotkey 3");
//	END_CREATE_HOTKEYS;
//
//	/* The client will call ts3plugin_freeMemory to release all allocated memory */
//}
//
///************************** TeamSpeak callbacks ***************************/
///*
// * Following functions are optional, feel free to remove unused callbacks.
// * See the clientlib documentation for details on each function.
// */
//
///* Clientlib */

void ts3plugin_onConnectStatusChangeEvent(uint64 serverConnectionHandlerID, int newStatus, unsigned int errorNumber) {
    /* Some example code following to show how to use the information query functions. */

  //  if(newStatus == STATUS_CONNECTION_ESTABLISHED) {  /* connection established and we have client and channels available */
  //      char* s;
  //      char msg[1024];
  //      anyID myID;
  //      uint64* ids;
  //      size_t i;
		//unsigned int error;

  //      /* Print clientlib version */
  //      if(ts3Functions.getClientLibVersion(&s) == ERROR_ok) {
  //          printf("PLUGIN: Client lib version: %s\n", s);
  //          ts3Functions.freeMemory(s);  /* Release string */
  //      } else {
  //          ts3Functions.logMessage("Error querying client lib version", LogLevel_ERROR, "Plugin", serverConnectionHandlerID);
  //          return;
  //      }

		///* Write plugin name and version to log */
  //      snprintf(msg, sizeof(msg), "Plugin %s, Version %s, Author: %s", ts3plugin_name(), ts3plugin_version(), ts3plugin_author());
  //      ts3Functions.logMessage(msg, LogLevel_INFO, "Plugin", serverConnectionHandlerID);

  //      /* Print virtual server name */
  //      if((error = ts3Functions.getServerVariableAsString(serverConnectionHandlerID, VIRTUALSERVER_NAME, &s)) != ERROR_ok) {
		//	if(error != ERROR_not_connected) {  /* Don't spam error in this case (failed to connect) */
		//		ts3Functions.logMessage("Error querying server name", LogLevel_ERROR, "Plugin", serverConnectionHandlerID);
		//	}
  //          return;
  //      }
  //      printf("PLUGIN: Server name: %s\n", s);
  //      ts3Functions.freeMemory(s);

  //      /* Print virtual server welcome message */
  //      if(ts3Functions.getServerVariableAsString(serverConnectionHandlerID, VIRTUALSERVER_WELCOMEMESSAGE, &s) != ERROR_ok) {
  //          ts3Functions.logMessage("Error querying server welcome message", LogLevel_ERROR, "Plugin", serverConnectionHandlerID);
  //          return;
  //      }
  //      printf("PLUGIN: Server welcome message: %s\n", s);
  //      ts3Functions.freeMemory(s);  /* Release string */

  //      /* Print own client ID and nickname on this server */
  //      if(ts3Functions.getClientID(serverConnectionHandlerID, &myID) != ERROR_ok) {
  //          ts3Functions.logMessage("Error querying client ID", LogLevel_ERROR, "Plugin", serverConnectionHandlerID);
  //          return;
  //      }
  //      if(ts3Functions.getClientSelfVariableAsString(serverConnectionHandlerID, CLIENT_NICKNAME, &s) != ERROR_ok) {
  //          ts3Functions.logMessage("Error querying client nickname", LogLevel_ERROR, "Plugin", serverConnectionHandlerID);
  //          return;
  //      }
  //      printf("PLUGIN: My client ID = %d, nickname = %s\n", myID, s);
  //      ts3Functions.freeMemory(s);

  //      /* Print list of all channels on this server */
  //      if(ts3Functions.getChannelList(serverConnectionHandlerID, &ids) != ERROR_ok) {
  //          ts3Functions.logMessage("Error getting channel list", LogLevel_ERROR, "Plugin", serverConnectionHandlerID);
  //          return;
  //      }
  //      printf("PLUGIN: Available channels:\n");
  //      for(i=0; ids[i]; i++) {
  //          /* Query channel name */
  //          if(ts3Functions.getChannelVariableAsString(serverConnectionHandlerID, ids[i], CHANNEL_NAME, &s) != ERROR_ok) {
  //              ts3Functions.logMessage("Error querying channel name", LogLevel_ERROR, "Plugin", serverConnectionHandlerID);
  //              return;
  //          }
  //          printf("PLUGIN: Channel ID = %llu, name = %s\n", (long long unsigned int)ids[i], s);
  //          ts3Functions.freeMemory(s);
  //      }
  //      ts3Functions.freeMemory(ids);  /* Release array */

  //      /* Print list of existing server connection handlers */
  //      printf("PLUGIN: Existing server connection handlers:\n");
  //      if(ts3Functions.getServerConnectionHandlerList(&ids) != ERROR_ok) {
  //          ts3Functions.logMessage("Error getting server list", LogLevel_ERROR, "Plugin", serverConnectionHandlerID);
  //          return;
  //      }
  //      for(i=0; ids[i]; i++) {
  //          if((error = ts3Functions.getServerVariableAsString(ids[i], VIRTUALSERVER_NAME, &s)) != ERROR_ok) {
		//		if(error != ERROR_not_connected) {  /* Don't spam error in this case (failed to connect) */
		//			ts3Functions.logMessage("Error querying server name", LogLevel_ERROR, "Plugin", serverConnectionHandlerID);
		//		}
  //              continue;
  //          }
  //          printf("- %llu - %s\n", (long long unsigned int)ids[i], s);
  //          ts3Functions.freeMemory(s);
  //      }
  //      ts3Functions.freeMemory(ids);
  //  }
}

void ts3plugin_onNewChannelEvent(uint64 serverConnectionHandlerID, uint64 channelID, uint64 channelParentID) {
}

void ts3plugin_onNewChannelCreatedEvent(uint64 serverConnectionHandlerID, uint64 channelID, uint64 channelParentID, anyID invokerID, const char* invokerName, const char* invokerUniqueIdentifier) {
}

void ts3plugin_onDelChannelEvent(uint64 serverConnectionHandlerID, uint64 channelID, anyID invokerID, const char* invokerName, const char* invokerUniqueIdentifier) {
}

void ts3plugin_onChannelMoveEvent(uint64 serverConnectionHandlerID, uint64 channelID, uint64 newChannelParentID, anyID invokerID, const char* invokerName, const char* invokerUniqueIdentifier) {
}

void ts3plugin_onUpdateChannelEvent(uint64 serverConnectionHandlerID, uint64 channelID) {
}

void ts3plugin_onUpdateChannelEditedEvent(uint64 serverConnectionHandlerID, uint64 channelID, anyID invokerID, const char* invokerName, const char* invokerUniqueIdentifier) {
}

void ts3plugin_onUpdateClientEvent(uint64 serverConnectionHandlerID, anyID clientID, anyID invokerID, const char* invokerName, const char* invokerUniqueIdentifier) {
}

void ts3plugin_onClientMoveEvent(uint64 serverConnectionHandlerID, anyID clientID, uint64 oldChannelID, uint64 newChannelID, int visibility, const char* moveMessage) {
}

void ts3plugin_onClientMoveSubscriptionEvent(uint64 serverConnectionHandlerID, anyID clientID, uint64 oldChannelID, uint64 newChannelID, int visibility) {
}

void ts3plugin_onClientMoveTimeoutEvent(uint64 serverConnectionHandlerID, anyID clientID, uint64 oldChannelID, uint64 newChannelID, int visibility, const char* timeoutMessage) {
}

void ts3plugin_onClientMoveMovedEvent(uint64 serverConnectionHandlerID, anyID clientID, uint64 oldChannelID, uint64 newChannelID, int visibility, anyID moverID, const char* moverName, const char* moverUniqueIdentifier, const char* moveMessage) {
}

void ts3plugin_onClientKickFromChannelEvent(uint64 serverConnectionHandlerID, anyID clientID, uint64 oldChannelID, uint64 newChannelID, int visibility, anyID kickerID, const char* kickerName, const char* kickerUniqueIdentifier, const char* kickMessage) {
}

void ts3plugin_onClientKickFromServerEvent(uint64 serverConnectionHandlerID, anyID clientID, uint64 oldChannelID, uint64 newChannelID, int visibility, anyID kickerID, const char* kickerName, const char* kickerUniqueIdentifier, const char* kickMessage) {
}

void ts3plugin_onClientIDsEvent(uint64 serverConnectionHandlerID, const char* uniqueClientIdentifier, anyID clientID, const char* clientName) {
}

void ts3plugin_onClientIDsFinishedEvent(uint64 serverConnectionHandlerID) {
}

void ts3plugin_onServerEditedEvent(uint64 serverConnectionHandlerID, anyID editerID, const char* editerName, const char* editerUniqueIdentifier) {
}

void ts3plugin_onServerUpdatedEvent(uint64 serverConnectionHandlerID) {
}

int ts3plugin_onServerErrorEvent(uint64 serverConnectionHandlerID, const char* errorMessage, unsigned int error, const char* returnCode, const char* extraMessage) {
	//printf("PLUGIN: onServerErrorEvent %llu %s %d %s\n", (long long unsigned int)serverConnectionHandlerID, errorMessage, error, (returnCode ? returnCode : ""));
	//if(returnCode) {
	//	/* A plugin could now check the returnCode with previously (when calling a function) remembered returnCodes and react accordingly */
	//	/* In case of using a a plugin return code, the plugin can return:
	//	 * 0: Client will continue handling this error (print to chat tab)
	//	 * 1: Client will ignore this error, the plugin announces it has handled it */
	//	return 1;
	//}
	return 0;  /* If no plugin return code was used, the return value of this function is ignored */
}

void ts3plugin_onServerStopEvent(uint64 serverConnectionHandlerID, const char* shutdownMessage) {
}

int generateRandomNumber(int startFrom, int span) {
	if (span != 0) {
		int number;

		/* initialize random seed: */
		unsigned long seed = mix(clock(), time(NULL) + randomNumberCounter, getpid());
		randomNumberCounter++;
		if (randomNumberCounter > 5000) {
			randomNumberCounter = 0;
		}


		srand(seed);

		/* generate secret number between 1 and 10: */
		number = rand() % span + startFrom + 1;

		return number;
	}

	return -1;
}

unsigned long mix(unsigned long a, unsigned long b, unsigned long c)
{
	a = a - b;  a = a - c;  a = a ^ (c >> 13);
	b = b - c;  b = b - a;  b = b ^ (a << 8);
	c = c - a;  c = c - b;  c = c ^ (b >> 13);
	a = a - b;  a = a - c;  a = a ^ (c >> 12);
	b = b - c;  b = b - a;  b = b ^ (a << 16);
	c = c - a;  c = c - b;  c = c ^ (b >> 5);
	a = a - b;  a = a - c;  a = a ^ (c >> 3);
	b = b - c;  b = b - a;  b = b ^ (a << 10);
	c = c - a;  c = c - b;  c = c ^ (b >> 15);
	return c;
}

int sizeOf(char* s) {
	int i = 0;
	while (s[i] != NULL && s[i] != ' ') {
		i++;
	}
	return i;
}

void getSubStringOf(char* pString, int startChar, int span) {
	memcpy(subString, &pString[startChar], span);
	subString[span] = '\0';
}

bool isCommand(char* msg) {
	if (msg[0] == '!') {
		return true;
	}
	else {
		return false;
	}
}

bool isDice(char* msg) {
	if (contains(msg, 'w', 1) && !(contains(msg, 's', 1))) {
		return true;
	}
	return false;
}

bool isPlusSomething(char* msg) {
	return contains(msg, '+', 3);
}

bool isMinusSomething(char* msg) {
	return contains(msg, '-', 3);
}

bool isMultipleDice(char* msg) {
	if (contains(msg, 'w', 0) && msg[1] != 'w' && msg[1] != 's') {
		return true;
	}
	return false;
}

bool contains(char* msg, char c, int startPos) {
	for (int i = startPos; i < sizeOf(msg); i++) {
		if (msg[i] == c) {
			return true;
		}
	}
	return false;
}

bool isSWWDice(char* msg) {
	if (msg[1] == 's') {
		if (msg[2] == 'w') {
			if (msg[3] == 'w') {
				return true;
			}
		}
	}
	return false;
}

int getPosOf(char* message, char c) {
	for (int i = 0; i < sizeOf(message); i++) {
		if (message[i] == c) {
			return i;
		}
	}
	return -1;
}

int explodingDice(int span) {
	if (span == 1) {
		return -1;
	}

	int result = 0;
	int tmpR = 0;
	do {
		tmpR = generateRandomNumber(0, span);
		result = result + tmpR;
	} while (tmpR == span);

	return result;
}

bool isSetColor(char* msg) {
	if (msg[1] == 'f') {
		if (msg[2] == 'a') {
			if (msg[3] == 'r') {
				if (msg[4] == 'b') {
					if (msg[5] == 'e') {
						if (msg[6] == ' ') {
							return true;
						}
					}
				}
			}
		}
	}
	return false;
}

bool isFate(char* msg) {
	if (msg[1] == 'f') {
		return true;
	}
	return false;
}

void getUserColor(int userID) {
	if (!strcmp(userColorColor[userID], "")) {
		char tmp[19999] = "black";
		memcpy(tmpUserColor, &tmp[0], sizeOf(tmp));
		subString[sizeOf(tmp)] = '\0';
		return;
	}
	else {
		memcpy(tmpUserColor, &userColorColor[userID][0], sizeOf(userColorColor[userID]));
		tmpUserColor[sizeOf(userColorColor[userID])] = '\0';
	}
}

void setUserColor(int userID, char* color) {
	memcpy(userColorColor[userID], &color[0], sizeOf(color));
	userColorColor[userID][sizeOf(color)] = '\0';
}

bool setAn(char* msg) {
	if (msg[0] == '!') {
		if (msg[1] == 'a') {
			if (msg[2] == 'n') {
				return true;
			}
		}
	}
	return false;
}

bool setAus(char* msg) {
	if (msg[0] == '!') {
		if (msg[1] == 'a') {
			if (msg[2] == 'u') {
				if (msg[3] == 's') {
					return true;
				}
			}
		}
	}
	return false;
}

bool isGetVersion(char* msg) {
	if (msg[0] == '!') {
		if (msg[1] == 'v') {
			if (msg[2] == 'e') {
				if (msg[3] == 'r') {
					if (msg[4] == 's') {
						if (msg[5] == 'i') {
							if (msg[6] == 'o') {
								if (msg[7] == 'n') {
									return true;
								}
							}
						}
					}
				}
			}
		}
	}
	return false;
}

bool isHelp(char* msg) {
	if (msg[0] == '!') {
		if (msg[1] == 'h') {
			if (msg[2] == 'e') {
				if (msg[3] == 'l') {
					if (msg[4] == 'p') {
						return true;
					}
				}
			}
		}
	}
	return false;
}

bool isOpenPrivatChat(char* msg) {
	if (msg[0] == '!') {
		if (msg[1] == 'p') {
			if (msg[2] == 'm') {
				return true;
			}
		}
	}
	return false;
}

void sendMessage(uint64 serverConnectionHandlerID, char* msg, anyID channelID, anyID fromID, bool isSendPrivate) {
	if (isSendPrivate) {
		ts3Functions.requestSendPrivateTextMsg(serverConnectionHandlerID, msg, fromID, 0);
	}
	else {
		ts3Functions.requestSendChannelTextMsg(serverConnectionHandlerID, msg, channelID, 0);
	}
}

int ts3plugin_onTextMessageEvent(uint64 serverConnectionHandlerID, anyID targetMode, anyID toID, anyID fromID, const char* fromName, const char* fromUniqueIdentifier, const char* message, int ffIgnored) {
	double version = 0.15;
	anyID myID;
	bool isCommandAlreadyTriggered = false;

	if (setAn(message)) {
		ts3Functions.getClientID(serverConnectionHandlerID, &myID);  /* Get own client ID */
		if (myID == fromID) {
			if (isCommandAlreadyTriggered == false) {
				chatBotActive = true;
				sendMessage(serverConnectionHandlerID, "[ZZW DiceBot] An", ts3Functions.getChannelOfClient, fromID, false);
				isCommandAlreadyTriggered = true;
			}
		}
	}
	if (setAus(message)) {
		ts3Functions.getClientID(serverConnectionHandlerID, &myID);  /* Get own client ID */
		if (myID == fromID) {
			if (isCommandAlreadyTriggered == false) {
				chatBotActive = false;
				sendMessage(serverConnectionHandlerID, "[ZZW DiceBot] Aus", ts3Functions.getChannelOfClient, fromID, false);
				isCommandAlreadyTriggered = true;
			}
		}
	}
	if (isGetVersion(message)) {
		ts3Functions.getClientID(serverConnectionHandlerID, &myID);  /* Get own client ID */
		if (myID == fromID) {
			if (isCommandAlreadyTriggered == false) {
				char ausgabe[19999] = "[ZZW DiceBot] Installierte Version des ZZW-DiceBots: 0.17 - [url=https://www.dropbox.com/sh/sh85x3ta6zkx2y3/AAAHuqGE_UjCQjrIQa5363QKa?dl=0]Hier der Link zum Download";
				sendMessage(serverConnectionHandlerID, ausgabe, ts3Functions.getChannelOfClient, fromID, false);
				isCommandAlreadyTriggered = true;
			}
		}
	}
	if (isOpenPrivatChat(message) && chatBotActive) {
		if (myID == fromID) {
			if (isCommandAlreadyTriggered == false) {
				char ausgabe[19999] = "[ZZW DiceBot] Schreibe hier um privat zu Wuerfeln!";
				sendMessage(serverConnectionHandlerID, ausgabe, ts3Functions.getChannelOfClient, fromID, true);
				isCommandAlreadyTriggered = true;
			}
		}
		else {
			if (isCommandAlreadyTriggered == false) {
				char ausgabe[19999] = "[ZZW DiceBot] Schreibe hier um privat zu Wuerfeln! - Lediglich der SL kann deine Nachrichten lesen...";
				sendMessage(serverConnectionHandlerID, ausgabe, ts3Functions.getChannelOfClient, fromID, true);
				isCommandAlreadyTriggered = true;
			}
		}
	}
	if (isHelp(message)) {
		if (isCommandAlreadyTriggered == false) {
			char ausgabe[19999] = "[ZZW DiceBot] Liste moeglicher Befehle:\n";
			char commands[9][19999] = { 
				"!an - Aktiviert den Dicebot", 
				"!aus - Deaktiviert den Dicebot", 
				"!help - Gibt eine Hilfsseite aus",
				"!version - Gibt die aktuelle Version und einen Downloadlink aus",
				"!pm - Oeffnet ein Fenster zum privaten Wuerfeln",
				"!farbe [farbe] - Ermoeglicht das setzen einer Ausgabefarbe", 
				"!f - Fate Wurf",
				"![zahl]w[zahl]+/-[zahl] - Wuerfelt die angegebene Zahl an Wuerfeln",
				"!sww[zahl]+/-[zahl] - Savage Worlds Wurf"
			};
			for (int i = 0; i < 9; i++) {
				strcat(ausgabe, commands[i]);
				strcat(ausgabe, "\n");
			}
			sendMessage(serverConnectionHandlerID, ausgabe, ts3Functions.getChannelOfClient, fromID, false);
			isCommandAlreadyTriggered = true;
		}
	}

	if (chatBotActive == true && isCommandAlreadyTriggered == false) {
		if (isCommand(message)) {
			getUserColor(fromID);
			char ausgabe[19999] = "\n[color=";
			strcat(ausgabe, tmpUserColor);
			strcat(ausgabe, "][");
			strcat(ausgabe, fromName);
			strcat(ausgabe, "]");
			char tmp[19999];

			int multi = 1;
			int pRandomNumber;
			int randomNumber;
			int addNumber = 0;
			int result = 0;

			bool error = true;

			bool pm = false; //gibt an ob es sich um eine privaten Wurf handelt
			if (targetMode == TextMessageTarget_CLIENT) {
				pm = true;
			}

			if (isMultipleDice(message)) {
				getSubStringOf(message, 1, getPosOf(message, 'w') - 1);
				multi = atoi(subString);
			}
			if (isDice(message)) {
				if (isCommandAlreadyTriggered == false) {
					error = false;
					strcat(ausgabe, " wuerfelt einen ");
					getSubStringOf(message, 1, sizeOf(message));
					strcat(ausgabe, subString);
					strcat(ausgabe, "\n Ergebnis: ");

					if (isPlusSomething(message)) {
						getSubStringOf(message, getPosOf(message, 'w') + 1, getPosOf(message, '+') - 2);
						pRandomNumber = atoi(subString);
						getSubStringOf(message, getPosOf(message, '+') + 1, sizeOf(message) - getPosOf(message, 'w'));
						addNumber = atoi(subString);

						getSubStringOf(message, 1, getPosOf(message, '+') - 1);
						strcat(ausgabe, subString);
					}
					else if (isMinusSomething(message)) {
						getSubStringOf(message, getPosOf(message, 'w') + 1, getPosOf(message, '-') - 2);
						pRandomNumber = atoi(subString);
						getSubStringOf(message, getPosOf(message, '-') + 1, sizeOf(message) - getPosOf(message, 'w'));
						addNumber = atoi(subString);
						addNumber = addNumber * (-1);

						getSubStringOf(message, 1, getPosOf(message, '-') - 1);
						strcat(ausgabe, subString);
					}
					else {
						getSubStringOf(message, getPosOf(message, 'w') + 1, sizeOf(message) - getPosOf(message, 'w'));
						pRandomNumber = atoi(subString);

						getSubStringOf(message, 1, sizeOf(message));
						strcat(ausgabe, subString);
					}
					strcat(ausgabe, "(");

					randomNumber = generateRandomNumber(0, pRandomNumber);
					result = result + randomNumber;

					sprintf(subString, "%d", randomNumber);
					strcat(ausgabe, subString);

					for (int i = 1; i < multi; i++) {
						randomNumber = generateRandomNumber(0, pRandomNumber);
						result = result + randomNumber;

						strcat(ausgabe, "+");
						sprintf(subString, "%d", randomNumber);
						strcat(ausgabe, subString);
					}

					strcat(ausgabe, ") Summe: ( ");
					sprintf(subString, "%d", result);
					strcat(ausgabe, subString);

					result = result + addNumber;

					if (isPlusSomething(message)) {
						getSubStringOf(message, getPosOf(message, '+'), sizeOf(message) - getPosOf(message, 'w'));
						strcat(ausgabe, subString);
					}
					else if (isMinusSomething(message)) {
						getSubStringOf(message, getPosOf(message, '-'), sizeOf(message) - getPosOf(message, 'w'));
						strcat(ausgabe, subString);
					}

					strcat(ausgabe, " ) = ");
					sprintf(subString, "%d", result);
					strcat(ausgabe, subString);

					sendMessage(serverConnectionHandlerID, ausgabe, ts3Functions.getChannelOfClient, fromID, pm);
					isCommandAlreadyTriggered = true;
				}
			}
			if (isSWWDice(message)) { //funktioniert
				if (isCommandAlreadyTriggered == false) {
					error = false;
					int tmp3;
					double tmp4;
					int randomNumberTmp;
					strcat(ausgabe, " Wildcard Eigenschafts Probe: \nProbewuerfel		W");

					if (isPlusSomething(message)) {
						getSubStringOf(message, getPosOf(message, 'w') + 2, getPosOf(message, '+') - 1);
						pRandomNumber = atoi(subString);
						getSubStringOf(message, getPosOf(message, '+') + 1, sizeOf(message) - getPosOf(message, 'w') + 1);
						addNumber = atoi(subString);

						getSubStringOf(message, 4, getPosOf(message, '+') - 4);
						strcat(ausgabe, subString);
					}
					else if (isMinusSomething(message)) {
						getSubStringOf(message, getPosOf(message, 'w') + 2, getPosOf(message, '-') - 1);
						pRandomNumber = atoi(subString);
						getSubStringOf(message, getPosOf(message, '-') + 1, sizeOf(message) - getPosOf(message, 'w') + 1);
						addNumber = atoi(subString);
						addNumber = addNumber * (-1);

						getSubStringOf(message, 4, getPosOf(message, '-') - 4);
						strcat(ausgabe, subString);
					}
					else {
						getSubStringOf(message, getPosOf(message, 'w') + 2, sizeOf(message) - getPosOf(message, 'w'));
						pRandomNumber = atoi(subString);
						addNumber = 0;

						strcat(ausgabe, subString);
					}

					strcat(ausgabe, "	(");

					//norm würfelwurf mit explosion
					randomNumber = explodingDice(pRandomNumber);
					if (randomNumber != -1) {
						result = randomNumber + addNumber;
					}
					randomNumberTmp = randomNumber;

					sprintf(subString, "%d", randomNumber);
					strcat(ausgabe, subString);
					strcat(ausgabe, ") ");
					strcat(ausgabe, "	");
					sprintf(subString, "%d", randomNumber);
					strcat(ausgabe, subString);
					if (isPlusSomething(message)) {
						getSubStringOf(message, getPosOf(message, '+'), sizeOf(message) - (getPosOf(message, 'w') + 1));
						strcat(ausgabe, subString);
					}
					else if (isMinusSomething(message)) {
						getSubStringOf(message, getPosOf(message, '-'), sizeOf(message) - (getPosOf(message, 'w') + 1));
						strcat(ausgabe, subString);
					}
					strcat(ausgabe, "=");
					sprintf(subString, "%d", result);
					strcat(ausgabe, subString);

					tmp3 = result / 4;
					tmp4 = result / 4;
					if (tmp4 < 1) { //ausgabe für fehlgeschlagen um: tmp3
						tmp3 = 4 - result;
						strcat(ausgabe, " Fehlschlag um ");
						sprintf(subString, "%d", tmp3);
						strcat(ausgabe, subString);
						strcat(ausgabe, " Punkt(e)\n");
					}
					else if (tmp4 == 1) { //ausgabe erfolgreich
						strcat(ausgabe, " Erfolg\n");
					}
					else if (tmp4 > 1) { //ausgabe für erfolg um: tmp3
						tmp3 = (result - 4) / 4;
						strcat(ausgabe, " Erfolg mit ");
						sprintf(subString, "%d", tmp3);
						strcat(ausgabe, subString);
						strcat(ausgabe, " Steigerung(en)\n");
					}


					//würfelwurf mit w6 und explosion (Wildcardwürfel)
					randomNumber = explodingDice(6);
					result = randomNumber + addNumber;

					strcat(ausgabe, "Wildcardwuerfel	W6	(");
					sprintf(subString, "%d", randomNumber);
					strcat(ausgabe, subString);
					strcat(ausgabe, ") ");
					strcat(ausgabe, "	");
					sprintf(subString, "%d", randomNumber);
					strcat(ausgabe, subString);
					if (isPlusSomething(message)) {
						getSubStringOf(message, getPosOf(message, '+'), sizeOf(message) - (getPosOf(message, 'w') + 1));
						strcat(ausgabe, subString);
					}
					else if (isMinusSomething(message)) {
						getSubStringOf(message, getPosOf(message, '-'), sizeOf(message) - (getPosOf(message, 'w') + 1));
						strcat(ausgabe, subString);
					}
					strcat(ausgabe, "=");
					sprintf(subString, "%d", result);
					strcat(ausgabe, subString);

					tmp3 = result / 4;
					tmp4 = result / 4;
					if (tmp4 < 1) { //ausgabe für fehlgeschlagen um: tmp3
						tmp3 = 4 - result;
						strcat(ausgabe, " Fehlschlag um ");
						sprintf(subString, "%d", tmp3);
						strcat(ausgabe, subString);
						strcat(ausgabe, " Punkt(e)");

						if (randomNumberTmp == randomNumber && randomNumber == 1) {
							int iFehlschlag = generateRandomNumber(0, 3);
							switch (iFehlschlag)
							{
							case 0:
								strcat(ausgabe, "\n-Fehlschlag!-");
								break;
							case 1:
								strcat(ausgabe, "\n-Kritischer Fehlschlag!-");
								break;
							case 2:
								strcat(ausgabe, "\n-Schwerer Kritischer Fehlschlag!-");
								break;
							default:
								strcat(ausgabe, "\n-Fehlschlag!-");
								break;
							}
						}
					}
					else if (tmp4 == 1) { //ausgabe erfolgreich
						strcat(ausgabe, " Erfolg");
					}
					else if (tmp4 > 1) { //ausgabe für erfolg um: tmp3
						tmp3 = (result - 4) / 4;
						strcat(ausgabe, " Erfolg mit ");
						sprintf(subString, "%d", tmp3);
						strcat(ausgabe, subString);
						strcat(ausgabe, " Steigerung(en)");
					}

					sendMessage(serverConnectionHandlerID, ausgabe, ts3Functions.getChannelOfClient, fromID, pm);
					isCommandAlreadyTriggered = true;
				}
			}
			if (isSetColor(message)) {
				if (isCommandAlreadyTriggered == false) {
					error = false;
					getSubStringOf(message, getPosOf(message, '!') + 7, sizeOf(message) - getPosOf(message, '!') + 6);
					setUserColor(fromID, subString);

					getUserColor(fromID);
					char ausgabe[19999] = "[color=";
					strcat(ausgabe, tmpUserColor);
					strcat(ausgabe, "]");
					strcat(ausgabe, " Farbe gesetzt...");
					sendMessage(serverConnectionHandlerID, ausgabe, ts3Functions.getChannelOfClient, fromID, pm);
					isCommandAlreadyTriggered = true;
				}
			}
			if (isFate(message)) {
				if (isCommandAlreadyTriggered == false) {
					error = false;
					int ri, ri1, ri2, ri3, ri4, mod, ergi;
					char rs[19999] = "";
					char rs1[19999] = "";
					char rs2[19999] = "";
					char rs3[19999] = "";
					char rs4[19999] = "";
					char ergs[19999] = "";

					//4w3 fuerfeln (geht von -1 bis +1) und dann zusammen rechnen
					ri1 = generateRandomNumber(-2, 3);
					ri2 = generateRandomNumber(-2, 3);
					ri3 = generateRandomNumber(-2, 3);
					ri4 = generateRandomNumber(-2, 3);

					ri = ri1 + ri2 + ri3 + ri4;

					sprintf(rs, "%d", ri);
					sprintf(rs1, "%d", ri1);
					sprintf(rs2, "%d", ri2);
					sprintf(rs3, "%d", ri3);
					sprintf(rs4, "%d", ri4);

					//ausgabe zusammen stellen
					getUserColor(fromID);
					char ausgabe[19999] = "\n[color=";
					strcat(ausgabe, tmpUserColor);
					strcat(ausgabe, "][");
					strcat(ausgabe, fromName);
					strcat(ausgabe, "]");
					strcat(ausgabe, " Fate Fertigkeitsprobe: \nWurf: ");
					strcat(ausgabe, rs1);
					strcat(ausgabe, " ");
					strcat(ausgabe, rs2);
					strcat(ausgabe, " ");
					strcat(ausgabe, rs3);
					strcat(ausgabe, " ");
					strcat(ausgabe, rs4);
					strcat(ausgabe, "  >>  ");
					strcat(ausgabe, rs);
					strcat(ausgabe, "  >>  ");
					strcat(ausgabe, rs);
					strcat(ausgabe, "+");
					//modifikator (!f4 ==> 4) addieren = erg
					getSubStringOf(message, getPosOf(message, '!') + 2, sizeOf(message) - getPosOf(message, '!') + 1);
					strcat(ausgabe, subString);
					mod = atoi(subString);
					ergi = ri + mod;
					sprintf(ergs, "%d", ergi);
					strcat(ausgabe, "=");
					strcat(ausgabe, ergs);


					sendMessage(serverConnectionHandlerID, ausgabe, ts3Functions.getChannelOfClient, fromID, pm);
					isCommandAlreadyTriggered = true;
				}
			}

			if (error == true && (!setAn(message))) {
				//If no case is true...
				strcat(ausgabe, " Syntax fehler...");
				sendMessage(serverConnectionHandlerID, ausgabe, ts3Functions.getChannelOfClient, fromID, pm);
			}
		}
	}

	///// http://www2.hs-fulda.de/~klingebiel/c-stdlib/string.htm
	return 0;  /* 0 = handle normally, 1 = client will ignore the text message */
}

void ts3plugin_onTalkStatusChangeEvent(uint64 serverConnectionHandlerID, int status, int isReceivedWhisper, anyID clientID) {
	/* Demonstrate usage of getClientDisplayName */
	/*char name[512];
	if(ts3Functions.getClientDisplayName(serverConnectionHandlerID, clientID, name, 512) == ERROR_ok) {
		if(status == STATUS_TALKING) {
			printf("--> %s starts talking\n", name);
		} else {
			printf("--> %s stops talking\n", name);
		}
	}*/
}

void ts3plugin_onConnectionInfoEvent(uint64 serverConnectionHandlerID, anyID clientID) {
}

void ts3plugin_onServerConnectionInfoEvent(uint64 serverConnectionHandlerID) {
}

void ts3plugin_onChannelSubscribeEvent(uint64 serverConnectionHandlerID, uint64 channelID) {
}

void ts3plugin_onChannelSubscribeFinishedEvent(uint64 serverConnectionHandlerID) {
}

void ts3plugin_onChannelUnsubscribeEvent(uint64 serverConnectionHandlerID, uint64 channelID) {
}

void ts3plugin_onChannelUnsubscribeFinishedEvent(uint64 serverConnectionHandlerID) {
}

void ts3plugin_onChannelDescriptionUpdateEvent(uint64 serverConnectionHandlerID, uint64 channelID) {
}

void ts3plugin_onChannelPasswordChangedEvent(uint64 serverConnectionHandlerID, uint64 channelID) {
}

void ts3plugin_onPlaybackShutdownCompleteEvent(uint64 serverConnectionHandlerID) {
}

void ts3plugin_onSoundDeviceListChangedEvent(const char* modeID, int playOrCap) {
}

void ts3plugin_onEditPlaybackVoiceDataEvent(uint64 serverConnectionHandlerID, anyID clientID, short* samples, int sampleCount, int channels) {
}

void ts3plugin_onEditPostProcessVoiceDataEvent(uint64 serverConnectionHandlerID, anyID clientID, short* samples, int sampleCount, int channels, const unsigned int* channelSpeakerArray, unsigned int* channelFillMask) {
}

void ts3plugin_onEditMixedPlaybackVoiceDataEvent(uint64 serverConnectionHandlerID, short* samples, int sampleCount, int channels, const unsigned int* channelSpeakerArray, unsigned int* channelFillMask) {
}

void ts3plugin_onEditCapturedVoiceDataEvent(uint64 serverConnectionHandlerID, short* samples, int sampleCount, int channels, int* edited) {
}

void ts3plugin_onCustom3dRolloffCalculationClientEvent(uint64 serverConnectionHandlerID, anyID clientID, float distance, float* volume) {
}

void ts3plugin_onCustom3dRolloffCalculationWaveEvent(uint64 serverConnectionHandlerID, uint64 waveHandle, float distance, float* volume) {
}

void ts3plugin_onUserLoggingMessageEvent(const char* logMessage, int logLevel, const char* logChannel, uint64 logID, const char* logTime, const char* completeLogString) {
}

/* Clientlib rare */

void ts3plugin_onClientBanFromServerEvent(uint64 serverConnectionHandlerID, anyID clientID, uint64 oldChannelID, uint64 newChannelID, int visibility, anyID kickerID, const char* kickerName, const char* kickerUniqueIdentifier, uint64 time, const char* kickMessage) {
}

int ts3plugin_onClientPokeEvent(uint64 serverConnectionHandlerID, anyID fromClientID, const char* pokerName, const char* pokerUniqueIdentity, const char* message, int ffIgnored) {
 //   anyID myID;

 //   //printf("PLUGIN onClientPokeEvent: %llu %d %s %s %d\n", (long long unsigned int)serverConnectionHandlerID, fromClientID, pokerName, message, ffIgnored);

	///* Check if the Friend/Foe manager has already blocked this poke */
	//if(ffIgnored) {
	//	return 0;  /* Client will block anyways, doesn't matter what we return */
	//}

 //   /* Example code: Send text message back to poking client */
 //   if(ts3Functions.getClientID(serverConnectionHandlerID, &myID) != ERROR_ok) {  /* Get own client ID */
 //       //ts3Functions.logMessage("Error querying own client id", LogLevel_ERROR, "Plugin", serverConnectionHandlerID);
 //       return 0;
 //   }
 //   if(fromClientID != myID) {  /* Don't reply when source is own client */
 //       if(ts3Functions.requestSendPrivateTextMsg(serverConnectionHandlerID, "Received your poke!", fromClientID, NULL) != ERROR_ok) {
 //           //ts3Functions.logMessage("Error requesting send text message", LogLevel_ERROR, "Plugin", serverConnectionHandlerID);
 //       }
 //   }

    return 0;  /* 0 = handle normally, 1 = client will ignore the poke */
}

void ts3plugin_onClientSelfVariableUpdateEvent(uint64 serverConnectionHandlerID, int flag, const char* oldValue, const char* newValue) {
}

void ts3plugin_onFileListEvent(uint64 serverConnectionHandlerID, uint64 channelID, const char* path, const char* name, uint64 size, uint64 datetime, int type, uint64 incompletesize, const char* returnCode) {
}

void ts3plugin_onFileListFinishedEvent(uint64 serverConnectionHandlerID, uint64 channelID, const char* path) {
}

void ts3plugin_onFileInfoEvent(uint64 serverConnectionHandlerID, uint64 channelID, const char* name, uint64 size, uint64 datetime) {
}

void ts3plugin_onServerGroupListEvent(uint64 serverConnectionHandlerID, uint64 serverGroupID, const char* name, int type, int iconID, int saveDB) {
}

void ts3plugin_onServerGroupListFinishedEvent(uint64 serverConnectionHandlerID) {
}

void ts3plugin_onServerGroupByClientIDEvent(uint64 serverConnectionHandlerID, const char* name, uint64 serverGroupList, uint64 clientDatabaseID) {
}

void ts3plugin_onServerGroupPermListEvent(uint64 serverConnectionHandlerID, uint64 serverGroupID, unsigned int permissionID, int permissionValue, int permissionNegated, int permissionSkip) {
}

void ts3plugin_onServerGroupPermListFinishedEvent(uint64 serverConnectionHandlerID, uint64 serverGroupID) {
}

void ts3plugin_onServerGroupClientListEvent(uint64 serverConnectionHandlerID, uint64 serverGroupID, uint64 clientDatabaseID, const char* clientNameIdentifier, const char* clientUniqueID) {
}

void ts3plugin_onChannelGroupListEvent(uint64 serverConnectionHandlerID, uint64 channelGroupID, const char* name, int type, int iconID, int saveDB) {
}

void ts3plugin_onChannelGroupListFinishedEvent(uint64 serverConnectionHandlerID) {
}

void ts3plugin_onChannelGroupPermListEvent(uint64 serverConnectionHandlerID, uint64 channelGroupID, unsigned int permissionID, int permissionValue, int permissionNegated, int permissionSkip) {
}

void ts3plugin_onChannelGroupPermListFinishedEvent(uint64 serverConnectionHandlerID, uint64 channelGroupID) {
}

void ts3plugin_onChannelPermListEvent(uint64 serverConnectionHandlerID, uint64 channelID, unsigned int permissionID, int permissionValue, int permissionNegated, int permissionSkip) {
}

void ts3plugin_onChannelPermListFinishedEvent(uint64 serverConnectionHandlerID, uint64 channelID) {
}

void ts3plugin_onClientPermListEvent(uint64 serverConnectionHandlerID, uint64 clientDatabaseID, unsigned int permissionID, int permissionValue, int permissionNegated, int permissionSkip) {
}

void ts3plugin_onClientPermListFinishedEvent(uint64 serverConnectionHandlerID, uint64 clientDatabaseID) {
}

void ts3plugin_onChannelClientPermListEvent(uint64 serverConnectionHandlerID, uint64 channelID, uint64 clientDatabaseID, unsigned int permissionID, int permissionValue, int permissionNegated, int permissionSkip) {
}

void ts3plugin_onChannelClientPermListFinishedEvent(uint64 serverConnectionHandlerID, uint64 channelID, uint64 clientDatabaseID) {
}

void ts3plugin_onClientChannelGroupChangedEvent(uint64 serverConnectionHandlerID, uint64 channelGroupID, uint64 channelID, anyID clientID, anyID invokerClientID, const char* invokerName, const char* invokerUniqueIdentity) {
}

int ts3plugin_onServerPermissionErrorEvent(uint64 serverConnectionHandlerID, const char* errorMessage, unsigned int error, const char* returnCode, unsigned int failedPermissionID) {
	return 0;  /* See onServerErrorEvent for return code description */
}

void ts3plugin_onPermissionListGroupEndIDEvent(uint64 serverConnectionHandlerID, unsigned int groupEndID) {
}

void ts3plugin_onPermissionListEvent(uint64 serverConnectionHandlerID, unsigned int permissionID, const char* permissionName, const char* permissionDescription) {
}

void ts3plugin_onPermissionListFinishedEvent(uint64 serverConnectionHandlerID) {
}

void ts3plugin_onPermissionOverviewEvent(uint64 serverConnectionHandlerID, uint64 clientDatabaseID, uint64 channelID, int overviewType, uint64 overviewID1, uint64 overviewID2, unsigned int permissionID, int permissionValue, int permissionNegated, int permissionSkip) {
}

void ts3plugin_onPermissionOverviewFinishedEvent(uint64 serverConnectionHandlerID) {
}

void ts3plugin_onServerGroupClientAddedEvent(uint64 serverConnectionHandlerID, anyID clientID, const char* clientName, const char* clientUniqueIdentity, uint64 serverGroupID, anyID invokerClientID, const char* invokerName, const char* invokerUniqueIdentity) {
}

void ts3plugin_onServerGroupClientDeletedEvent(uint64 serverConnectionHandlerID, anyID clientID, const char* clientName, const char* clientUniqueIdentity, uint64 serverGroupID, anyID invokerClientID, const char* invokerName, const char* invokerUniqueIdentity) {
}

void ts3plugin_onClientNeededPermissionsEvent(uint64 serverConnectionHandlerID, unsigned int permissionID, int permissionValue) {
}

void ts3plugin_onClientNeededPermissionsFinishedEvent(uint64 serverConnectionHandlerID) {
}

void ts3plugin_onFileTransferStatusEvent(anyID transferID, unsigned int status, const char* statusMessage, uint64 remotefileSize, uint64 serverConnectionHandlerID) {
}

void ts3plugin_onClientChatClosedEvent(uint64 serverConnectionHandlerID, anyID clientID, const char* clientUniqueIdentity) {
}

void ts3plugin_onClientChatComposingEvent(uint64 serverConnectionHandlerID, anyID clientID, const char* clientUniqueIdentity) {
}

void ts3plugin_onServerLogEvent(uint64 serverConnectionHandlerID, const char* logMsg) {
}

void ts3plugin_onServerLogFinishedEvent(uint64 serverConnectionHandlerID, uint64 lastPos, uint64 fileSize) {
}

void ts3plugin_onMessageListEvent(uint64 serverConnectionHandlerID, uint64 messageID, const char* fromClientUniqueIdentity, const char* subject, uint64 timestamp, int flagRead) {
}

void ts3plugin_onMessageGetEvent(uint64 serverConnectionHandlerID, uint64 messageID, const char* fromClientUniqueIdentity, const char* subject, const char* message, uint64 timestamp) {
}

void ts3plugin_onClientDBIDfromUIDEvent(uint64 serverConnectionHandlerID, const char* uniqueClientIdentifier, uint64 clientDatabaseID) {
}

void ts3plugin_onClientNamefromUIDEvent(uint64 serverConnectionHandlerID, const char* uniqueClientIdentifier, uint64 clientDatabaseID, const char* clientNickName) {
}

void ts3plugin_onClientNamefromDBIDEvent(uint64 serverConnectionHandlerID, const char* uniqueClientIdentifier, uint64 clientDatabaseID, const char* clientNickName) {
}

void ts3plugin_onComplainListEvent(uint64 serverConnectionHandlerID, uint64 targetClientDatabaseID, const char* targetClientNickName, uint64 fromClientDatabaseID, const char* fromClientNickName, const char* complainReason, uint64 timestamp) {
}

void ts3plugin_onBanListEvent(uint64 serverConnectionHandlerID, uint64 banid, const char* ip, const char* name, const char* uid, const char* mytsid, uint64 creationTime, uint64 durationTime, const char* invokerName,
							  uint64 invokercldbid, const char* invokeruid, const char* reason, int numberOfEnforcements, const char* lastNickName) {
}

void ts3plugin_onClientServerQueryLoginPasswordEvent(uint64 serverConnectionHandlerID, const char* loginPassword) {
}

void ts3plugin_onPluginCommandEvent(uint64 serverConnectionHandlerID, const char* pluginName, const char* pluginCommand, anyID invokerClientID, const char* invokerName, const char* invokerUniqueIdentity) {
	printf("ON PLUGIN COMMAND: %s %s %d %s %s\n", pluginName, pluginCommand, invokerClientID, invokerName, invokerUniqueIdentity);
}

void ts3plugin_onIncomingClientQueryEvent(uint64 serverConnectionHandlerID, const char* commandText) {
}

void ts3plugin_onServerTemporaryPasswordListEvent(uint64 serverConnectionHandlerID, const char* clientNickname, const char* uniqueClientIdentifier, const char* description, const char* password, uint64 timestampStart, uint64 timestampEnd, uint64 targetChannelID, const char* targetChannelPW) {
}

/* Client UI callbacks */

/*
 * Called from client when an avatar image has been downloaded to or deleted from cache.
 * This callback can be called spontaneously or in response to ts3Functions.getAvatar()
 */
void ts3plugin_onAvatarUpdated(uint64 serverConnectionHandlerID, anyID clientID, const char* avatarPath) {
	///* If avatarPath is NULL, the avatar got deleted */
	///* If not NULL, avatarPath contains the path to the avatar file in the TS3Client cache */
	//if(avatarPath != NULL) {
	//	printf("onAvatarUpdated: %llu %d %s\n", (long long unsigned int)serverConnectionHandlerID, clientID, avatarPath);
	//} else {
	//	printf("onAvatarUpdated: %llu %d - deleted\n", (long long unsigned int)serverConnectionHandlerID, clientID);
	//}
}

/*
 * Called when a plugin menu item (see ts3plugin_initMenus) is triggered. Optional function, when not using plugin menus, do not implement this.
 *
 * Parameters:
 * - serverConnectionHandlerID: ID of the current server tab
 * - type: Type of the menu (PLUGIN_MENU_TYPE_CHANNEL, PLUGIN_MENU_TYPE_CLIENT or PLUGIN_MENU_TYPE_GLOBAL)
 * - menuItemID: Id used when creating the menu item
 * - selectedItemID: Channel or Client ID in the case of PLUGIN_MENU_TYPE_CHANNEL and PLUGIN_MENU_TYPE_CLIENT. 0 for PLUGIN_MENU_TYPE_GLOBAL.
 */
void ts3plugin_onMenuItemEvent(uint64 serverConnectionHandlerID, enum PluginMenuType type, int menuItemID, uint64 selectedItemID) {
	//printf("PLUGIN: onMenuItemEvent: serverConnectionHandlerID=%llu, type=%d, menuItemID=%d, selectedItemID=%llu\n", (long long unsigned int)serverConnectionHandlerID, type, menuItemID, (long long unsigned int)selectedItemID);
	//switch(type) {
	//	case PLUGIN_MENU_TYPE_GLOBAL:
	//		/* Global menu item was triggered. selectedItemID is unused and set to zero. */
	//		switch(menuItemID) {
	//			case MENU_ID_GLOBAL_1:
	//				/* Menu global 1 was triggered */
	//				break;
	//			case MENU_ID_GLOBAL_2:
	//				/* Menu global 2 was triggered */
	//				break;
	//			default:
	//				break;
	//		}
	//		break;
	//	case PLUGIN_MENU_TYPE_CHANNEL:
	//		/* Channel contextmenu item was triggered. selectedItemID is the channelID of the selected channel */
	//		switch(menuItemID) {
	//			case MENU_ID_CHANNEL_1:
	//				/* Menu channel 1 was triggered */
	//				break;
	//			case MENU_ID_CHANNEL_2:
	//				/* Menu channel 2 was triggered */
	//				break;
	//			case MENU_ID_CHANNEL_3:
	//				/* Menu channel 3 was triggered */
	//				break;
	//			default:
	//				break;
	//		}
	//		break;
	//	case PLUGIN_MENU_TYPE_CLIENT:
	//		/* Client contextmenu item was triggered. selectedItemID is the clientID of the selected client */
	//		switch(menuItemID) {
	//			case MENU_ID_CLIENT_1:
	//				/* Menu client 1 was triggered */
	//				break;
	//			case MENU_ID_CLIENT_2:
	//				/* Menu client 2 was triggered */
	//				break;
	//			default:
	//				break;
	//		}
	//		break;
	//	default:
	//		break;
	//}
}

/* This function is called if a plugin hotkey was pressed. Omit if hotkeys are unused. */
void ts3plugin_onHotkeyEvent(const char* keyword) {
	//printf("PLUGIN: Hotkey event: %s\n", keyword);
	/* Identify the hotkey by keyword ("keyword_1", "keyword_2" or "keyword_3" in this example) and handle here... */
}

/* Called when recording a hotkey has finished after calling ts3Functions.requestHotkeyInputDialog */
void ts3plugin_onHotkeyRecordedEvent(const char* keyword, const char* key) {
}

// This function receives your key Identifier you send to notifyKeyEvent and should return
// the friendly device name of the device this hotkey originates from. Used for display in UI.
const char* ts3plugin_keyDeviceName(const char* keyIdentifier) {
	return NULL;
}

// This function translates the given key identifier to a friendly key name for display in the UI
const char* ts3plugin_displayKeyText(const char* keyIdentifier) {
	return NULL;
}

// This is used internally as a prefix for hotkeys so we can store them without collisions.
// Should be unique across plugins.
const char* ts3plugin_keyPrefix() {
	return NULL;
}

/* Called when client custom nickname changed */
void ts3plugin_onClientDisplayNameChanged(uint64 serverConnectionHandlerID, anyID clientID, const char* displayName, const char* uniqueClientIdentifier) {
}
