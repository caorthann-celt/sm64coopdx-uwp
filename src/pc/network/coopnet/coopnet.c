#include <inttypes.h>
#include <stdarg.h>
#ifdef UWP_BUILD
#define _CRT_RAND_S
#endif
#include <stdlib.h>
#include <time.h>
#include "libcoopnet.h"
#include "coopnet.h"
#include "coopnet_id.h"
#include "pc/network/network.h"
#include "pc/network/version.h"
#include "pc/djui/djui_language.h"
#include "pc/djui/djui_popup.h"
#include "pc/mods/mods.h"
#include "pc/utils/misc.h"
#include "pc/debuglog.h"
#ifdef DISCORD_SDK
#include "pc/discord/discord.h"
#endif
#ifdef UWP_BUILD
#include <windows.h>
#endif

#ifdef COOPNET

#define MAX_COOPNET_DESCRIPTION_LENGTH 1024

#ifdef UWP_BUILD
static void coopnet_uwp_log(const char* message) {
    OutputDebugStringA(message);
    OutputDebugStringA("\n");
}
#else
static void coopnet_uwp_log(UNUSED const char* message) {
}
#endif

uint64_t gCoopNetDesiredLobby = 0;
char gCoopNetPassword[64] = "";
char sCoopNetDescription[MAX_COOPNET_DESCRIPTION_LENGTH] = "";

static uint64_t sLocalLobbyId = 0;
static uint64_t sLocalLobbyOwnerId = 0;
static enum NetworkType sNetworkType;
static bool sReconnecting = false;
static uint32_t sCoopNetTraceSequence = 0;

static CoopNetRc coopnet_initialize(void);

static const char* coopnet_network_type_name(enum NetworkType networkType) {
    switch (networkType) {
        case NT_NONE:   return "none";
        case NT_SERVER: return "server";
        case NT_CLIENT: return "client";
    }
    return "unknown";
}

static const char* coopnet_error_name(enum MPacketErrorNumber error) {
    switch (error) {
        case MERR_NONE:                     return "MERR_NONE";
        case MERR_COOPNET_VERSION:          return "MERR_COOPNET_VERSION";
        case MERR_LOBBY_NOT_FOUND:          return "MERR_LOBBY_NOT_FOUND";
        case MERR_LOBBY_JOIN_FULL:          return "MERR_LOBBY_JOIN_FULL";
        case MERR_LOBBY_JOIN_FAILED:        return "MERR_LOBBY_JOIN_FAILED";
        case MERR_LOBBY_PASSWORD_INCORRECT: return "MERR_LOBBY_PASSWORD_INCORRECT";
        case MERR_PEER_FAILED:              return "MERR_PEER_FAILED";
        case MERR_MAX:                      return "MERR_MAX";
    }
    return "MERR_UNKNOWN";
}

static void coopnet_trace(const char* event, const char* fmt, ...) {
    char detail[512] = "";
    if (fmt != NULL && fmt[0] != '\0') {
        va_list args;
        va_start(args, fmt);
        vsnprintf(detail, sizeof(detail), fmt, args);
        va_end(args);
    }

    char message[640];
    uint32_t sequence = ++sCoopNetTraceSequence;
    if (detail[0] != '\0') {
        snprintf(message, sizeof(message), "CoopNet Trace #%u: %s | %s", sequence, event, detail);
    } else {
        snprintf(message, sizeof(message), "CoopNet Trace #%u: %s", sequence, event);
    }

    LOG_INFO("%s", message);
    coopnet_uwp_log(message);
}

#ifdef UWP_BUILD
static bool coopnet_generate_dest_id(uint64_t* outDestId) {
    if (outDestId == NULL) { return false; }

    uint64_t value = 0;
    for (int i = 0; i < 4; i++) {
        unsigned int chunk = 0;
        if (rand_s(&chunk) != 0) {
            return false;
        }
        value ^= ((uint64_t)(chunk & 0xFFFF)) << (i * 16);
    }

    if (value == 0) {
        value = 1;
    }

    *outDestId = value;
    return true;
}

static bool coopnet_dest_id_looks_weak(uint64_t destId) {
    if (destId == 0) { return true; }

    // Older UWP builds generated destIds with MSVC rand(), which only fills
    // the low 47 bits and leaves the high 17 bits empty.
    return (destId >> 47) == 0;
}
#endif

bool ns_coopnet_query(QueryCallbackPtr callback, QueryFinishCallbackPtr finishCallback, const char* password) {
    coopnet_trace("lobby-list-request", "passwordLen=%zu", strlen(password));
    gCoopNetCallbacks.OnLobbyListGot = callback;
    gCoopNetCallbacks.OnLobbyListFinish = finishCallback;
    CoopNetRc initRc = coopnet_initialize();
    if (initRc != COOPNET_OK) {
        coopnet_trace("lobby-list-request-failed", "stage=initialize rc=%d", initRc);
        return false;
    }
    CoopNetRc listRc = coopnet_lobby_list_get(GAME_NAME, password);
    if (listRc != COOPNET_OK) {
        coopnet_trace("lobby-list-request-failed", "stage=lobby_list_get rc=%d", listRc);
        return false;
    }
    coopnet_trace("lobby-list-request-sent", "game=%s", GAME_NAME);
    return true;
}

static void coopnet_on_connected(uint64_t userId) {
    coopnet_trace("connected", "userId=%" PRIu64, userId);
    coopnet_set_local_user_id(userId);
}

static void coopnet_on_disconnected(bool intentional) {
    coopnet_trace("disconnected", "intentional=%d", intentional ? 1 : 0);
    LOG_INFO("Coopnet shutdown!");
    if (!intentional) {
        djui_popup_create(DLANG(NOTIF, COOPNET_DISCONNECTED), 2);
    }
    coopnet_shutdown();
    gCoopNetCallbacks.OnLobbyListGot = NULL;
    gCoopNetCallbacks.OnLobbyListFinish = NULL;
}

static void coopnet_on_peer_disconnected(uint64_t peerId) {
    coopnet_trace("peer-disconnected", "peerId=%" PRIu64, peerId);
    u8 localIndex = coopnet_user_id_to_local_index(peerId);
    if (localIndex != UNKNOWN_LOCAL_INDEX && gNetworkPlayers[localIndex].connected) {
        network_player_disconnected(gNetworkPlayers[localIndex].globalIndex);
    }
}

static void coopnet_on_load_balance(const char* host, uint32_t port) {
    coopnet_trace("load-balance", "host=%s port=%u", host ? host : "", port);
    if (host && strlen(host) > 0) {
        snprintf(configCoopNetIp, MAX_CONFIG_STRING, "%s", host);
    }
    configCoopNetPort = port;
    configfile_save(configfile_name());
}

static void coopnet_on_receive(uint64_t userId, const uint8_t* data, uint64_t dataLength) {
    coopnet_trace("peer-data", "userId=%" PRIu64 " bytes=%" PRIu64, userId, dataLength);
    coopnet_set_user_id(0, userId);
    u8 localIndex = coopnet_user_id_to_local_index(userId);
    network_receive(localIndex, &userId, (u8*)data, dataLength);
}

static void coopnet_on_lobby_joined(uint64_t lobbyId, uint64_t userId, uint64_t ownerId, uint64_t destId) {
    coopnet_trace("lobby-joined-callback", "lobbyId=%" PRIu64 " userId=%" PRIu64 " ownerId=%" PRIu64 " destId=%" PRIu64,
        lobbyId, userId, ownerId, destId);
    LOG_INFO("coopnet_on_lobby_joined!");
    coopnet_set_user_id(0, ownerId);
    sLocalLobbyId = lobbyId;
    sLocalLobbyOwnerId = ownerId;

    if (userId == coopnet_get_local_user_id()) {
        coopnet_clear_dest_ids();
        snprintf(configDestId, MAX_CONFIG_STRING, "%" PRIu64 "", destId);
#ifdef UWP_BUILD
        coopnet_trace("lobby-joined-local", "ownerId=%" PRIu64, ownerId);
#endif
    }

    coopnet_save_dest_id(userId, destId);

    if (userId == coopnet_get_local_user_id() && gNetworkType == NT_CLIENT) {
        coopnet_trace("mod-list-request", "reason=local-lobby-joined");
        network_send_mod_list_request();
    }
#ifdef DISCORD_SDK
    if (gDiscordInitialized) {
        discord_activity_update();
    }
#endif
}

static void coopnet_on_lobby_left(uint64_t lobbyId, uint64_t userId) {
    LOG_INFO("coopnet_on_lobby_left!");
    coopnet_trace("lobby-left-callback", "lobbyId=%" PRIu64 " userId=%" PRIu64, lobbyId, userId);
    coopnet_clear_dest_id(userId);
    if (lobbyId == sLocalLobbyId && userId == coopnet_get_local_user_id()) {
        network_shutdown(false, false, true, false);
    }
}

static void coopnet_on_error(enum MPacketErrorNumber error, uint64_t tag) {
    coopnet_trace("error-callback", "error=%s tag=%" PRIu64, coopnet_error_name(error), tag);
    switch (error) {
        case MERR_COOPNET_VERSION:
            djui_popup_create(DLANG(NOTIF, COOPNET_VERSION), 2);
            network_shutdown(false, false, false, false);
            break;
        case MERR_PEER_FAILED:
            {
                char built[256] = { 0 };
                u8 localIndex = coopnet_user_id_to_local_index(tag);
                char* name = DLANG(NOTIF, UNKNOWN);
                if (localIndex == 0) {
                    name = DLANG(NOTIF, LOBBY_HOST);
                } else if (localIndex != UNKNOWN_LOCAL_INDEX && gNetworkPlayers[localIndex].connected) {
                    name = gNetworkPlayers[localIndex].name;
                }
                djui_language_replace(DLANG(NOTIF, PEER_FAILED), built, 256, '@', name);
                djui_popup_create(built, 2);
            }
            break;
        case MERR_LOBBY_NOT_FOUND:
            djui_popup_create(DLANG(NOTIF, LOBBY_NOT_FOUND), 2);
            network_shutdown(false, false, false, false);
            break;
        case MERR_LOBBY_JOIN_FULL:
            djui_popup_create(DLANG(NOTIF, DISCONNECT_FULL), 2);
            network_shutdown(false, false, false, false);
            break;
        case MERR_LOBBY_JOIN_FAILED:
            djui_popup_create(DLANG(NOTIF, LOBBY_JOIN_FAILED), 2);
            network_shutdown(false, false, false, false);
            break;
        case MERR_LOBBY_PASSWORD_INCORRECT:
            djui_popup_create(DLANG(NOTIF, LOBBY_PASSWORD_INCORRECT), 2);
            network_shutdown(false, false, false, false);
            break;
        case MERR_NONE:
        case MERR_MAX:
            break;
    }
}

static bool ns_coopnet_initialize(enum NetworkType networkType, bool reconnecting) {
    sNetworkType = networkType;
    sReconnecting = reconnecting;
    coopnet_trace("network-initialize", "networkType=%s reconnecting=%d alreadyConnected=%d",
        coopnet_network_type_name(networkType), reconnecting ? 1 : 0, coopnet_is_connected() ? 1 : 0);
    if (reconnecting) { return true; }
    return coopnet_is_connected()
        ? true
        : (coopnet_initialize() == COOPNET_OK);
}

static char* ns_coopnet_get_id_str(u8 localIndex) {
    static char id_str[32] = { 0 };
    if (localIndex == UNKNOWN_LOCAL_INDEX) {
        snprintf(id_str, 32, "???");
    } else {
        uint64_t userId = ns_coopnet_get_id(localIndex);
        uint64_t destId = coopnet_get_dest_id(userId);
        snprintf(id_str, 32, "%" PRIu64 "", destId);
    }
    return id_str;
}

static bool ns_coopnet_match_addr(void* addr1, void* addr2) {
    return !memcmp(addr1, addr2, sizeof(u64));
}

bool ns_coopnet_is_connected(void) {
    return coopnet_is_connected();
}

static void coopnet_populate_description(void) {
    char* buffer = sCoopNetDescription;
    int bufferLength = MAX_COOPNET_DESCRIPTION_LENGTH;
    // get version
    const char* version = get_version();
    int versionLength = strlen(version);
    snprintf(buffer, bufferLength, "%s", version);
    buffer += versionLength;
    bufferLength -= versionLength;

    // get mod strings
    if (gActiveMods.entryCount <= 0) { return; }
    char** strings = calloc(gActiveMods.entryCount, sizeof(char*));
    if (strings == NULL) { return; }
    for (int i = 0; i < gActiveMods.entryCount; i++) {
        struct Mod* mod = gActiveMods.entries[i];
        strings[i] = mod->name;
    }

    // add seperator
    char* sep = "\n\nMods:\n";
    snprintf(buffer, bufferLength, "%s", sep);
    buffer += strlen(sep);
    bufferLength -= strlen(sep);

    // concat mod strings
    str_seperator_concat(buffer, bufferLength, strings, gActiveMods.entryCount, "\\#dcdcdc\\\n");
    free(strings);
}

void ns_coopnet_update(void) {
    if (!coopnet_is_connected()) { return; }

    coopnet_update();
    if (gNetworkType != NT_NONE && sNetworkType != NT_NONE) {
        if (sNetworkType == NT_SERVER) {
            char mode[64] = "";
            mods_get_main_mod_name(mode, 64);
            if (sReconnecting) {
                LOG_INFO("Update lobby");
                coopnet_trace("lobby-update-request", "lobbyId=%" PRIu64 " mode=%s", sLocalLobbyId, mode);
                coopnet_populate_description();
                coopnet_lobby_update(sLocalLobbyId, GAME_NAME, get_version(), configPlayerName, mode, sCoopNetDescription);
            } else {
                LOG_INFO("Create lobby");
                snprintf(gCoopNetPassword, 64, "%s", configPassword);
                coopnet_trace("lobby-create-request", "mode=%s maxPlayers=%d passwordLen=%zu",
                    mode, configAmountOfPlayers, strlen(gCoopNetPassword));
                coopnet_populate_description();
                coopnet_lobby_create(GAME_NAME, get_version(), configPlayerName, mode, (uint16_t)configAmountOfPlayers, gCoopNetPassword, sCoopNetDescription);
            }
        } else if (sNetworkType == NT_CLIENT) {
            coopnet_trace("lobby-join-request", "lobbyId=%" PRIu64 " passwordLen=%zu",
                gCoopNetDesiredLobby, strlen(gCoopNetPassword));
            LOG_INFO("Join lobby");
            CoopNetRc rc = coopnet_lobby_join(gCoopNetDesiredLobby, gCoopNetPassword);
            coopnet_trace("lobby-join-request-result", "lobbyId=%" PRIu64 " rc=%d", gCoopNetDesiredLobby, rc);
        }
        sNetworkType = NT_NONE;
    }
}

static int ns_coopnet_network_send(u8 localIndex, void* address, u8* data, u16 dataLength) {
    if (!coopnet_is_connected()) { return 1; }
    //if (gCurLobbyId == 0) { return 2; }
    u64 userId = coopnet_raw_get_id(localIndex);
    if (localIndex == 0 && address != NULL) { userId = *(u64*)address; }
    CoopNetRc rc = coopnet_send_to(userId, data, dataLength);
#ifdef UWP_BUILD
    if (rc != COOPNET_OK) {
        char message[192];
        snprintf(message, sizeof(message), "CoopNet UWP: peer send failed localIndex=%u user=%" PRIu64 " bytes=%u rc=%d", localIndex, userId, dataLength, rc);
        coopnet_uwp_log(message);
    }
#endif
    if (rc != COOPNET_OK) { return -1; }

    return 0;
}

static bool coopnet_allow_invite(void) {
    if (sLocalLobbyId == 0) { return false; }
    return (sLocalLobbyOwnerId == coopnet_get_local_user_id()) || (strlen(gCoopNetPassword) == 0);
}

static void ns_coopnet_get_lobby_id(UNUSED char* destination, UNUSED u32 destLength) {
    if (sLocalLobbyId == 0) {
        snprintf(destination, destLength, "%s", "");
    } else {
        snprintf(destination, destLength, "coopnet:%" PRIu64 "", sLocalLobbyId);
    }
}

static void ns_coopnet_get_lobby_secret(UNUSED char* destination, UNUSED u32 destLength) {
    if (sLocalLobbyId == 0 || !coopnet_allow_invite()) {
        snprintf(destination, destLength, "%s", "");
    } else {
        snprintf(destination, destLength, "coopnet:%" PRIu64":%s", sLocalLobbyId, gCoopNetPassword);
    }
}

static void ns_coopnet_shutdown(bool reconnecting) {
    coopnet_trace("network-shutdown", "reconnecting=%d", reconnecting ? 1 : 0);
    if (reconnecting) { return; }
    LOG_INFO("Coopnet shutdown!");
    coopnet_shutdown();
    gCoopNetCallbacks.OnLobbyListGot = NULL;
    gCoopNetCallbacks.OnLobbyListFinish = NULL;

    gCoopNetCallbacks.OnConnected = NULL;
    gCoopNetCallbacks.OnDisconnected = NULL;
    gCoopNetCallbacks.OnReceive = NULL;
    gCoopNetCallbacks.OnLobbyJoined = NULL;
    gCoopNetCallbacks.OnLobbyLeft = NULL;
    gCoopNetCallbacks.OnError = NULL;
    gCoopNetCallbacks.OnPeerDisconnected = NULL;
    gCoopNetCallbacks.OnLoadBalance = NULL;

    sLocalLobbyId = 0;
    sLocalLobbyOwnerId = 0;
}

static CoopNetRc coopnet_initialize(void) {
    gCoopNetCallbacks.OnConnected = coopnet_on_connected;
    gCoopNetCallbacks.OnDisconnected = coopnet_on_disconnected;
    gCoopNetCallbacks.OnReceive = coopnet_on_receive;
    gCoopNetCallbacks.OnLobbyJoined = coopnet_on_lobby_joined;
    gCoopNetCallbacks.OnLobbyLeft = coopnet_on_lobby_left;
    gCoopNetCallbacks.OnError = coopnet_on_error;
    gCoopNetCallbacks.OnPeerConnected = NULL;
    gCoopNetCallbacks.OnPeerDisconnected = coopnet_on_peer_disconnected;
    gCoopNetCallbacks.OnLoadBalance = coopnet_on_load_balance;

    if (coopnet_is_connected()) { return COOPNET_OK; }

    char* endptr = NULL;
    uint64_t destId = strtoull(configDestId, &endptr, 10);

    bool shouldRefreshDestId = (destId == 0);
#ifdef UWP_BUILD
    if (!shouldRefreshDestId && coopnet_dest_id_looks_weak(destId)) {
        shouldRefreshDestId = true;
        coopnet_uwp_log("CoopNet UWP: upgrading weak persistent destId");
    }
#endif

    if (shouldRefreshDestId) {
#ifdef UWP_BUILD
        if (!coopnet_generate_dest_id(&destId)) {
            coopnet_trace("dest-id-generate-failed", NULL);
            return COOPNET_FAILED;
        }
#else
        // I'll seed the random number generator so we don't get the same ID every time.
        srand(time(NULL));
        destId = ((uint64_t)rand() << 32) | (uint64_t)rand();
        if (destId == 0) { destId = 1; } // Just in case!
#endif
        snprintf(configDestId, MAX_CONFIG_STRING, "%" PRIu64, destId);
        configfile_save(configfile_name());
        coopnet_trace("dest-id-ready", "value=%" PRIu64 " refreshed=1", destId);
    } else {
        coopnet_trace("dest-id-ready", "value=%" PRIu64 " refreshed=0", destId);
    }

    coopnet_trace("begin-request", "host=%s port=%u name=%s destId=%" PRIu64,
        configCoopNetIp, configCoopNetPort, configPlayerName, destId);

    CoopNetRc rc = coopnet_begin(configCoopNetIp, configCoopNetPort, configPlayerName, destId);
    if (rc == COOPNET_FAILED) {
        coopnet_trace("begin-result", "rc=%d", rc);
        djui_popup_create(DLANG(NOTIF, COOPNET_CONNECTION_FAILED), 2);
    } else {
        coopnet_trace("begin-result", "rc=%d", rc);
    }
    return rc;
}

struct NetworkSystem gNetworkSystemCoopNet = {
    .initialize       = ns_coopnet_initialize,
    .get_id           = ns_coopnet_get_id,
    .get_id_str       = ns_coopnet_get_id_str,
    .save_id          = ns_coopnet_save_id,
    .clear_id         = ns_coopnet_clear_id,
    .dup_addr         = ns_coopnet_dup_addr,
    .match_addr       = ns_coopnet_match_addr,
    .update           = ns_coopnet_update,
    .send             = ns_coopnet_network_send,
    .get_lobby_id     = ns_coopnet_get_lobby_id,
    .get_lobby_secret = ns_coopnet_get_lobby_secret,
    .shutdown         = ns_coopnet_shutdown,
    .requireServerBroadcast = false,
    .name             = "CoopNet",
};

#endif
