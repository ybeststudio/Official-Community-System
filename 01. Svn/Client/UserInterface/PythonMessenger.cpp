// Add to includes:
#if defined(ENABLE_MESSENGER_RENEWAL)
#include "PythonCommunity.h"
#endif

// Add the following `if` function anywhere in this file:
#if defined(ENABLE_MESSENGER_RENEWAL)
	if (CPythonCommunity::Instance().HasMessengerHandler())
	{
#if defined(ENABLE_MESSENGER_DETAILS)
#if defined(ENABLE_MULTI_LANGUAGE_SYSTEM)
		CPythonCommunity::Instance().OnFriendOnline(c_szName, 0, c_szCountry);
#else
		CPythonCommunity::Instance().OnFriendOnline(c_szName, 0);
#endif
#else
		CPythonCommunity::Instance().OnFriendOnline(c_szName);
#endif
		return;
	}
#endif

// Add the following `if` function anywhere in this file:
#if defined(ENABLE_MESSENGER_RENEWAL)
	if (CPythonCommunity::Instance().HasMessengerHandler())
	{
#if defined(ENABLE_MESSENGER_DETAILS)
#if defined(ENABLE_MULTI_LANGUAGE_SYSTEM)
		CPythonCommunity::Instance().OnFriendOffline(c_szName, c_dwLastPlayTime, c_szCountry);
#else
		CPythonCommunity::Instance().OnFriendOffline(c_szName, c_dwLastPlayTime);
#endif
#else
		CPythonCommunity::Instance().OnFriendOffline(c_szName);
#endif
		return;
	}
#endif

// Add the following `if` function anywhere in this file:
#if defined(ENABLE_MESSENGER_RENEWAL)
	if (CPythonCommunity::Instance().HasMessengerHandler())
	{
		CPythonCommunity::Instance().OnBlockOnline(c_szName);
		return;
	}
#endif

// Add the following `if` function anywhere in this file:
#if defined(ENABLE_MESSENGER_RENEWAL)
	if (CPythonCommunity::Instance().HasMessengerHandler())
	{
		CPythonCommunity::Instance().OnBlockOffline(c_szName);
		return;
	}
#endif

// Add the following `if` function anywhere in this file:
#if defined(ENABLE_MESSENGER_RENEWAL)
	if (CPythonCommunity::Instance().HasMessengerHandler())
	{
		CPythonCommunity::Instance().OnBlockRemove(c_szName);
		return;
	}
#endif
