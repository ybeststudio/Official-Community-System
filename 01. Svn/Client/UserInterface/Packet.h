// Add the following packet definitions in the related packet section:
#if defined(ENABLE_MESSENGER_RENEWAL)
enum EMessengerConnectionState
{
	MESSENGER_CONNECTION_STATE_CONNECT = 0,
	MESSENGER_CONNECTION_STATE_LEFT_SEAT = 1,
	MESSENGER_CONNECTION_STATE_AUTO_HUNT = 2,
	MESSENGER_CONNECTION_STATE_SHOP_OPEN = 3,
	MESSENGER_CONNECTION_STATE_DISCONNECT = 4,
};

enum
{
	MESSENGER_STATUS_MESSAGE_MAX_LEN = 50,
};
#endif

// Add the following packet definitions in the related packet section:
#if defined(ENABLE_MESSENGER_RENEWAL)
	MESSENGER_SUBHEADER_GC_CONNECTION_STATE,
	MESSENGER_SUBHEADER_GC_MY_STATUS_MESSAGE,
	MESSENGER_SUBHEADER_GC_STATUS_MESSAGE,
#endif

// Add the following packet definitions in the related packet section:
#if defined(ENABLE_MESSENGER_RENEWAL)
	BYTE bLevel;
	BYTE bIsConqueror;
	BYTE bChannel;
	long lMapIndex;
	BYTE bConnectionState;
	char szStatusMessage[MESSENGER_STATUS_MESSAGE_MAX_LEN + 1];
#endif

// Add the following packet definitions in the related packet section:
#if defined(ENABLE_MESSENGER_RENEWAL)
	MESSENGER_SUBHEADER_CG_SET_CONNECTION_STATE,
	MESSENGER_SUBHEADER_CG_SET_STATUS_MESSAGE,
	MESSENGER_SUBHEADER_CG_DELETE_STATUS_MESSAGE,
#endif

// Add the following packet definitions in the related packet section:
#if defined(ENABLE_MESSENGER_RENEWAL)
typedef struct command_messenger_set_connection_state
{
	BYTE bConnectionState;
} TPacketCGMessengerSetConnectionState;

typedef struct packet_messenger_my_status_message
{
	char szStatusMessage[MESSENGER_STATUS_MESSAGE_MAX_LEN + 1];
} TPacketGCMessengerMyStatusMessage;

typedef struct packet_messenger_status_message
{
	char szName[CHARACTER_NAME_MAX_LEN + 1];
	char szStatusMessage[MESSENGER_STATUS_MESSAGE_MAX_LEN + 1];
} TPacketGCMessengerStatusMessage;

typedef struct command_messenger_set_status_message
{
	char szStatusMessage[MESSENGER_STATUS_MESSAGE_MAX_LEN + 1];
} TPacketCGMessengerSetStatusMessage;
#endif

// Add the following packet definitions in the related packet section:
#if defined(ENABLE_COMMUNITY_GUILD_RENEWAL)
	GUILD_SUBHEADER_GC_MEMBER_LOCATION,
#endif

// Add the following packet definitions in the related packet section:
#if defined(ENABLE_COMMUNITY_GUILD_RENEWAL)
typedef struct packet_guild_member_location
{
	DWORD dwPID;
	BYTE bChannel;
	long lMapIndex;
} TPacketGCGuildMemberLocation;
#endif
