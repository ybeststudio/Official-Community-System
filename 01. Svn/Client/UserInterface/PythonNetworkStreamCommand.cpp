// Before
	else if (!strcmpi(szCmd, "messenger_auth"))
	{
		if (TokenVector.size() < 2)
			return;

		const std::string& c_rstrName = TokenVector[1].c_str();
	}

// After
	else if (!strcmpi(szCmd, "messenger_auth"))
	{
		if (TokenVector.size() < 2)
			return;

		const std::string& c_rstrName = TokenVector[1].c_str();
#if defined(ENABLE_MESSENGER_RENEWAL)
		if (TokenVector.size() >= 5)
		{
			const int iLevel = atoi(TokenVector[2].c_str());
			const int iChannel = atoi(TokenVector[3].c_str());
			const long lMapIndex = atol(TokenVector[4].c_str());
			PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "OnMessengerAddFriendQuestion",
				Py_BuildValue("(siii)", c_rstrName.c_str(), iLevel, iChannel, lMapIndex));
		}
		else
		{
			PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "OnMessengerAddFriendQuestion",
				Py_BuildValue("(siii)", c_rstrName.c_str(), 0, 0, 0));
		}
#else
		PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "OnMessengerAddFriendQuestion", Py_BuildValue("(s)", c_rstrName.c_str()));
#endif
	}
