// Add to includes:
#if defined(__MESSENGER_RENEWAL__)
#	include "messenger_manager.h"
#endif

// In `bool CHARACTER::RemoveAffect(CAffect* pkAff)`, find this block:
if (IsPC())
	{
		SendAffectRemovePacket(GetDesc(), GetPlayerID(), pkAff->dwType, pkAff->wApplyOn);
	}

// Add after it:
#if defined(__MESSENGER_RENEWAL__) && defined(ENABLE_AUTO_SYSTEM)
	if (IsPC() && pkAff->dwType == AFFECT_AUTO)
	{
		if (GetMessengerConnectionState() == MESSENGER_CONNECTION_STATE_AUTO_HUNT)
		{
			BYTE bRestoreState = MESSENGER_CONNECTION_STATE_CONNECT;
#if defined(__LEFT_SEAT__)
			if (LeftSeat())
				bRestoreState = MESSENGER_CONNECTION_STATE_LEFT_SEAT;
#endif
			CMessengerManager::instance().SetConnectionState(this, bRestoreState);
		}
	}
#endif
