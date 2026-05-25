// In `int CInputMain::Messenger(const LPCHARACTER c_lpChar, const char* c_pData, std::size_t uiBytes)`, extend the switch statement with:
#if defined(__MESSENGER_RENEWAL__)
		case MESSENGER_SUBHEADER_CG_SET_CONNECTION_STATE:
		{
			if (uiBytes < sizeof(TPacketCGMessengerSetConnectionState))
				return -1;

			const TPacketCGMessengerSetConnectionState* c_pStatePacket =
				reinterpret_cast<const TPacketCGMessengerSetConnectionState*>(c_pData);

			CMessengerManager::instance().SetConnectionState(c_lpChar, c_pStatePacket->bConnectionState);
		}
		return sizeof(TPacketCGMessengerSetConnectionState);

		case MESSENGER_SUBHEADER_CG_SET_STATUS_MESSAGE:
		{
			if (uiBytes < sizeof(TPacketCGMessengerSetStatusMessage))
				return -1;

			const TPacketCGMessengerSetStatusMessage* c_pStatusPacket =
				reinterpret_cast<const TPacketCGMessengerSetStatusMessage*>(c_pData);

			CMessengerManager::instance().SetStatusMessage(c_lpChar, c_pStatusPacket->szStatusMessage);
		}
		return sizeof(TPacketCGMessengerSetStatusMessage);

		case MESSENGER_SUBHEADER_CG_DELETE_STATUS_MESSAGE:
			CMessengerManager::instance().DeleteStatusMessage(c_lpChar);
			return 0;
#endif
