// Before
				if (ch->GetPremiumRemainSeconds(PREMIUM_AUTO_USE) > 0)
				{
					if (!ch->IsAffectFlag(AFF_AUTO_USE)) {
						ch->AddAffect(AFFECT_AUTO, POINT_NONE, 0, AFF_AUTO_USE, INFINITE_AFFECT_DURATION, 0, false);
					}
				}

// After
				if (ch->GetPremiumRemainSeconds(PREMIUM_AUTO_USE) > 0)
				{
					if (!ch->IsAffectFlag(AFF_AUTO_USE)) {
						ch->AddAffect(AFFECT_AUTO, POINT_NONE, 0, AFF_AUTO_USE, INFINITE_AFFECT_DURATION, 0, false);
					}
#if defined(__MESSENGER_RENEWAL__)
					if (ch->GetMessengerConnectionState() != MESSENGER_CONNECTION_STATE_LEFT_SEAT)
						CMessengerManager::instance().SetConnectionState(ch, MESSENGER_CONNECTION_STATE_AUTO_HUNT);
#endif
				}

// Before
		case 'd':
		{
			if (ch->IsAffectFlag(AFF_AUTO_USE))
				ch->RemoveAffect(AFFECT_AUTO);

		}

// After
		case 'd':
		{
			if (ch->IsAffectFlag(AFF_AUTO_USE))
				ch->RemoveAffect(AFFECT_AUTO);

#if defined(__MESSENGER_RENEWAL__)
			if (ch->GetMessengerConnectionState() == MESSENGER_CONNECTION_STATE_AUTO_HUNT)
			{
				BYTE bRestoreState = MESSENGER_CONNECTION_STATE_CONNECT;
#if defined(__LEFT_SEAT__)
				if (ch->LeftSeat())
					bRestoreState = MESSENGER_CONNECTION_STATE_LEFT_SEAT;
#endif
				CMessengerManager::instance().SetConnectionState(ch, bRestoreState);
			}
#endif
		}
