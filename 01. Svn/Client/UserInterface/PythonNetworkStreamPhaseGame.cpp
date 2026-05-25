// Add to includes:
#if defined(ENABLE_MESSENGER_RENEWAL)
#include "PythonCommunity.h"
#endif

// Add the following `CPythonNetworkStream::RequestMessengerRefresh` function anywhere in this file:
#if defined(ENABLE_MESSENGER_RENEWAL)
void CPythonNetworkStream::RequestMessengerRefresh()
{
	__RefreshMessengerWindow();
}
#endif

// Add the following `CPythonNetworkStream::SendMessengerSetConnectionStatePacket` function anywhere in this file:
#if defined(ENABLE_MESSENGER_RENEWAL)
bool CPythonNetworkStream::SendMessengerSetConnectionStatePacket(BYTE bConnectionState)
{
	if (bConnectionState > MESSENGER_CONNECTION_STATE_DISCONNECT)
		return false;

	TPacketCGMessenger Packet;
	Packet.bHeader = HEADER_CG_MESSENGER;
	Packet.bSubHeader = MESSENGER_SUBHEADER_CG_SET_CONNECTION_STATE;
	if (!Send(sizeof(Packet), &Packet))
		return false;

	TPacketCGMessengerSetConnectionState StatePacket;
	StatePacket.bConnectionState = bConnectionState;
	if (!Send(sizeof(StatePacket), &StatePacket))
		return false;

	return SendSequence();
}

bool CPythonNetworkStream::SendMessengerSetStatusMessagePacket(const char* c_szMessage)
{
	if (!c_szMessage || !*c_szMessage)
		return false;

	size_t len = strlen(c_szMessage);
	if (len == 0 || len > MESSENGER_STATUS_MESSAGE_MAX_LEN)
		return false;

	TPacketCGMessenger Packet;
	Packet.bHeader = HEADER_CG_MESSENGER;
	Packet.bSubHeader = MESSENGER_SUBHEADER_CG_SET_STATUS_MESSAGE;
	if (!Send(sizeof(Packet), &Packet))
		return false;

	TPacketCGMessengerSetStatusMessage StatusPacket;
	memset(&StatusPacket, 0, sizeof(StatusPacket));
	strncpy(StatusPacket.szStatusMessage, c_szMessage, MESSENGER_STATUS_MESSAGE_MAX_LEN);
	StatusPacket.szStatusMessage[MESSENGER_STATUS_MESSAGE_MAX_LEN] = '\0';
	if (!Send(sizeof(StatusPacket), &StatusPacket))
		return false;

	return SendSequence();
}

bool CPythonNetworkStream::SendMessengerDeleteStatusMessagePacket()
{
	TPacketCGMessenger Packet;
	Packet.bHeader = HEADER_CG_MESSENGER;
	Packet.bSubHeader = MESSENGER_SUBHEADER_CG_DELETE_STATUS_MESSAGE;
	if (!Send(sizeof(Packet), &Packet))
		return false;

	return SendSequence();
}
#endif

// Add the following `CPythonCommunity::Instance` function anywhere in this file:
#if defined(ENABLE_MESSENGER_RENEWAL)
	if (Packet.bSubHeader == MESSENGER_SUBHEADER_GC_MY_STATUS_MESSAGE)
	{
		if (uiPacketSize < sizeof(TPacketGCMessengerMyStatusMessage))
			return false;

		TPacketGCMessengerMyStatusMessage StatusPacket;
		if (!Recv(sizeof(StatusPacket), &StatusPacket))
			return false;

		CPythonCommunity::Instance().NotifyMyStatusMessage(StatusPacket.szStatusMessage);
		return true;
	}

	if (Packet.bSubHeader == MESSENGER_SUBHEADER_GC_STATUS_MESSAGE)
	{
		while (uiPacketSize >= sizeof(TPacketGCMessengerStatusMessage))
		{
			TPacketGCMessengerStatusMessage StatusPacket;
			if (!Recv(sizeof(StatusPacket), &StatusPacket))
				return false;

			uiPacketSize -= sizeof(StatusPacket);
			CPythonCommunity::Instance().OnFriendStatusMessage(
				StatusPacket.szName, StatusPacket.szStatusMessage);
		}
		return true;
	}
#endif

// Add the following `CPythonCommunity::Instance` function anywhere in this file:
#if defined(ENABLE_MESSENGER_RENEWAL)
		auto NotifyFriendRenewalDetails = [&ListPacket]()
		{
			if (ListPacket.bConnected & MESSENGER_CONNECTED_STATE_ONLINE)
			{
				CPythonCommunity::Instance().OnFriendRenewalDetails(
					ListPacket.szName,
					ListPacket.bLevel,
					ListPacket.bIsConqueror,
					ListPacket.bChannel,
					ListPacket.lMapIndex);
			}
		};

		auto NotifyFriendConnectionState = [&ListPacket]()
		{
			CPythonCommunity::Instance().OnFriendConnectionState(
				ListPacket.szName, ListPacket.bConnectionState);
		};

		auto NotifyFriendStatusMessage = [&ListPacket]()
		{
			CPythonCommunity::Instance().OnFriendStatusMessage(
				ListPacket.szName, ListPacket.szStatusMessage);
		};
#endif

// Before
				if (ListPacket.bConnected & MESSENGER_CONNECTED_STATE_ONLINE)
				{
#if defined(ENABLE_MESSENGER_DETAILS)
					CPythonMessenger::Instance().OnFriendLogin(ListPacket.szName
#if defined(ENABLE_MULTI_LANGUAGE_SYSTEM)
						, ListPacket.szCountry
#endif
					);
#else
					CPythonMessenger::Instance().OnFriendLogin(ListPacket.szName);
#endif
				}

// After
				if (ListPacket.bConnected & MESSENGER_CONNECTED_STATE_ONLINE)
				{
#if defined(ENABLE_MESSENGER_DETAILS)
					CPythonMessenger::Instance().OnFriendLogin(ListPacket.szName
#if defined(ENABLE_MULTI_LANGUAGE_SYSTEM)
						, ListPacket.szCountry
#endif
					);
#else
					CPythonMessenger::Instance().OnFriendLogin(ListPacket.szName);
#endif
#if defined(ENABLE_MESSENGER_RENEWAL)
					NotifyFriendRenewalDetails();
					NotifyFriendConnectionState();
					NotifyFriendStatusMessage();
#endif
				}

// Before
				else
				{
#if defined(ENABLE_MESSENGER_DETAILS)
					CPythonMessenger::Instance().OnFriendLogout(ListPacket.szName, ListPacket.dwLastPlayTime
#if defined(ENABLE_MULTI_LANGUAGE_SYSTEM)
						, ListPacket.szCountry
#endif
					);
#else
					CPythonMessenger::Instance().OnFriendLogout(ListPacket.szName);
#endif
				}

// After
				else
				{
#if defined(ENABLE_MESSENGER_DETAILS)
					CPythonMessenger::Instance().OnFriendLogout(ListPacket.szName, ListPacket.dwLastPlayTime
#if defined(ENABLE_MULTI_LANGUAGE_SYSTEM)
						, ListPacket.szCountry
#endif
					);
#else
					CPythonMessenger::Instance().OnFriendLogout(ListPacket.szName);
#endif
#if defined(ENABLE_MESSENGER_RENEWAL)
					NotifyFriendConnectionState();
					NotifyFriendStatusMessage();
#endif
				}

// Before
			case MESSENGER_SUBHEADER_GC_LOGIN:
			{
#if defined(ENABLE_MESSENGER_DETAILS)
				CPythonMessenger::Instance().OnFriendLogin(ListPacket.szName
#if defined(ENABLE_MULTI_LANGUAGE_SYSTEM)
					, ListPacket.szCountry
#endif
				);
#else
				CPythonMessenger::Instance().OnFriendLogin(ListPacket.szName);
#endif
				__RefreshTargetBoardByName(ListPacket.szName);
			}

// After
			case MESSENGER_SUBHEADER_GC_LOGIN:
			{
#if defined(ENABLE_MESSENGER_DETAILS)
				CPythonMessenger::Instance().OnFriendLogin(ListPacket.szName
#if defined(ENABLE_MULTI_LANGUAGE_SYSTEM)
					, ListPacket.szCountry
#endif
				);
#else
				CPythonMessenger::Instance().OnFriendLogin(ListPacket.szName);
#endif
#if defined(ENABLE_MESSENGER_RENEWAL)
				NotifyFriendRenewalDetails();
				NotifyFriendConnectionState();
				NotifyFriendStatusMessage();
#endif
				__RefreshTargetBoardByName(ListPacket.szName);
			}

// Add the following `CPythonMessenger::Instance` function anywhere in this file:
#if defined(ENABLE_MESSENGER_RENEWAL)
			case MESSENGER_SUBHEADER_GC_CONNECTION_STATE:
			{
				if (ListPacket.bConnected & MESSENGER_CONNECTED_STATE_ONLINE)
				{
#if defined(ENABLE_MESSENGER_DETAILS)
					CPythonMessenger::Instance().OnFriendLogin(ListPacket.szName
#if defined(ENABLE_MULTI_LANGUAGE_SYSTEM)
						, ListPacket.szCountry
#endif
					);
#else
					CPythonMessenger::Instance().OnFriendLogin(ListPacket.szName);
#endif
					NotifyFriendRenewalDetails();
				}
				else
				{
#if defined(ENABLE_MESSENGER_DETAILS)
					CPythonMessenger::Instance().OnFriendLogout(ListPacket.szName, ListPacket.dwLastPlayTime
#if defined(ENABLE_MULTI_LANGUAGE_SYSTEM)
						, ListPacket.szCountry
#endif
					);
#else
					CPythonMessenger::Instance().OnFriendLogout(ListPacket.szName);
#endif
				}
				NotifyFriendConnectionState();
				NotifyFriendStatusMessage();
			}
			break;
#endif

// In `bool CPythonNetworkStream::RecvGuild()`, extend the switch statement with:
#if defined(ENABLE_MESSENGER_RENEWAL) && defined(ENABLE_COMMUNITY_GUILD_RENEWAL)
		case GUILD_SUBHEADER_GC_MEMBER_LOCATION:
		{
			TPacketGCGuildMemberLocation loc;
			if (!Recv(sizeof(loc), &loc))
				return false;

			CPythonCommunity::Instance().OnGuildMemberLocation(loc.dwPID, loc.bChannel, loc.lMapIndex);
			break;
		}
#endif
