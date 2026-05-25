#include "StdAfx.h"

#include "PythonNetworkStream.h"
#include "PythonNonPlayer.h"
#include "AbstractApplication.h"
#include "AbstractPlayer.h"
#include "AbstractCharacterManager.h"
#include "AbstractChat.h"

#include "InstanceBase.h"

#if defined(ENABLE_DEV_FILE_MODE)
#	include "../EterPack/EterPackManager.h"
#endif

#if defined(ENABLE_METINSTONE_SWAP)
	#include "MetinstoneSwapManager.h"
#endif

#if defined(ENABLE_EVENT_BANNER)
	#include "PythonInGameEventSystemManager.h"
#endif

#if defined(ENABLE_OX_RENDER_AREA)
	#include "PythonBackground.h"
#endif

#if defined(ENABLE_MONSTER_CARD)
#	include "PythonMonsterCardManager.h"
#	include <cstdlib>
#endif

#if defined(ENVIRONMENT_SYSTEM)
	#include <unordered_map>
#endif

#define ishan(ch) (((ch) & 0xE0) > 0x90)
#define ishanasc(ch) (isascii(ch) || ishan(ch))
#define ishanalp(ch) (isalpha(ch) || ishan(ch))
#define isnhdigit(ch) (!ishan(ch) && isdigit(ch))
#define isnhspace(ch) (!ishan(ch) && isspace(ch))

#define LOWER(c) (((c) >= 'A' && (c) <= 'Z') ? ((c) + ('a' - 'A')) : (c))
#define UPPER(c) (((c) >= 'a' && (c) <= 'z') ? ((c) + ('A' - 'a')) : (c))

void SkipSpaces(char** string)
{
	for (; **string != '\0' && isnhspace((unsigned char)**string); ++(*string));
}

char* OneArgument(char* argument, char* first_arg)
{
	char mark = FALSE;

	if (!argument)
	{
		*first_arg = '\0';
		return NULL;
	}

	SkipSpaces(&argument);

	while (*argument)
	{
		if (*argument == '\"')
		{
			mark = !mark;
			++argument;
			continue;
		}

		if (!mark && isnhspace((unsigned char)*argument))
			break;

		*(first_arg++) = LOWER(*argument);
		++argument;
	}

	*first_arg = '\0';

	SkipSpaces(&argument);
	return (argument);
}

void AppendMonsterList(const CPythonNonPlayer::TMobTableList& c_rMobTableList, const char* c_szHeader, int iType)
{
	DWORD dwMonsterCount = 0;
	std::string strMonsterList = c_szHeader;

	CPythonNonPlayer::TMobTableList::const_iterator itor = c_rMobTableList.begin();
	for (; itor != c_rMobTableList.end(); ++itor)
	{
		const CPythonNonPlayer::TMobTable* c_pMobTable = *itor;
		if (iType == c_pMobTable->bRank)
		{
			if (dwMonsterCount != 0)
				strMonsterList += ", ";
			strMonsterList += c_pMobTable->szLocaleName;
			if (++dwMonsterCount > 5)
				break;
		}
	}
	if (dwMonsterCount > 0)
	{
		IAbstractChat& rkChat = IAbstractChat::GetSingleton();
		rkChat.AppendChat(CHAT_TYPE_INFO, strMonsterList.c_str());
	}
}

#if defined(ENABLE_DEV_FILE_MODE)
// Forward declaration (defined below).
bool SplitToken(const char* c_szLine, CTokenVector* pstTokenVector, const char* c_szDelimeter = " ");

static void AppendPyExceptionToChat(const char* c_szPrefix)
{
	PyObject *ptype = NULL, *pvalue = NULL, *ptrace = NULL;
	PyErr_Fetch(&ptype, &pvalue, &ptrace);
	PyErr_NormalizeException(&ptype, &pvalue, &ptrace);

	std::string msg = c_szPrefix ? c_szPrefix : "python error";
	msg += ": ";

	if (pvalue)
	{
		PyObject* pStr = PyObject_Str(pvalue);
		if (pStr)
		{
			const char* c_sz = PyString_AsString(pStr);
			if (c_sz && *c_sz)
				msg += c_sz;
			Py_DECREF(pStr);
		}
	}

	IAbstractChat::GetSingleton().AppendChat(CHAT_TYPE_INFO, msg.c_str());

	Py_XDECREF(ptype);
	Py_XDECREF(pvalue);
	Py_XDECREF(ptrace);
}

static PyObject* ReloadModuleFromEterPack(const std::string& modName)
{
	// Try to load "<mod>.py" via EterPackManager; with dev-file mode this can come from disk (pack/root/<mod>.py).
	const std::string fileName = modName + ".py";

	CMappedFile file;
	LPCVOID pvData = NULL;
	if (!CEterPackManager::Instance().Get(file, fileName.c_str(), &pvData) || !pvData || file.Size() == 0)
		return NULL;

	// If module already exists in sys.modules with different case, reuse it
	// so the user can type any case and still reload the live module.
	std::string actualName = modName;
	{
		PyObject* pModDict = PyImport_GetModuleDict(); // borrowed
		if (pModDict && PyDict_Check(pModDict))
		{
			PyObject* key = NULL;
			PyObject* value = NULL;
			Py_ssize_t pos = 0;
			std::string needle = modName;
			for (std::size_t i = 0; i < needle.size(); ++i)
			{
				char& ch = needle[i];
				if (ch >= 'A' && ch <= 'Z')
					ch = char(ch - 'A' + 'a');
			}

			while (PyDict_Next(pModDict, &pos, &key, &value))
			{
				if (!key)
					continue;
				const char* k = PyString_Check(key) ? PyString_AsString(key) : NULL;
				if (!k)
					continue;

				std::string kk = k;
				for (std::size_t i = 0; i < kk.size(); ++i)
				{
					char& ch = kk[i];
					if (ch >= 'A' && ch <= 'Z')
						ch = char(ch - 'A' + 'a');
				}
				if (kk == needle)
				{
					actualName = k;
					break;
				}
			}
		}
	}

	PyObject* pModule = PyImport_AddModule(actualName.c_str()); // borrowed
	if (!pModule)
		return NULL;

	PyObject* pDict = PyModule_GetDict(pModule); // borrowed
	if (!pDict)
		return NULL;

	// Ensure builtins are available (needed for import, etc.).
	// Without this, scripts can fail with "__import__ not found".
	PyObject* pBuiltins = PyEval_GetBuiltins(); // borrowed
	if (pBuiltins)
		PyDict_SetItemString(pDict, "__builtins__", pBuiltins);

	PyObject* pCode = Py_CompileString((const char*)pvData, fileName.c_str(), Py_file_input);
	if (!pCode)
		return NULL;

	PyObject* pFileStr = PyString_FromString(fileName.c_str());
	if (pFileStr)
	{
		PyDict_SetItemString(pDict, "__file__", pFileStr);
		Py_DECREF(pFileStr);
	}

	PyObject* pRes = PyEval_EvalCode((PyCodeObject*)pCode, pDict, pDict);
	Py_DECREF(pCode);

	if (!pRes)
		return NULL;

	Py_DECREF(pRes);
	Py_INCREF(pModule);
	return pModule;
}
#endif

bool CPythonNetworkStream::ClientCommand(const char* c_szCommand)
{
#if defined(ENABLE_DEV_FILE_MODE)
	// Dev-only: reload a root python module from disk without relogin.
	// Usage: /pyreload uimonstercard   OR   /pyreload uimonstercard.py
	if (c_szCommand && c_szCommand[0] == '/')
	{
		CTokenVector tok;
		if (SplitToken(c_szCommand, &tok) && tok.size() >= 2)
		{
			const std::string cmd = tok[0];
			if (cmd == "/pyreload")
			{
				std::string modName = tok[1];

				// normalize: strip optional ".py"
				if (modName.size() > 3)
				{
					const std::string ext = modName.substr(modName.size() - 3);
					if (ext == ".py" || ext == ".PY")
						modName.erase(modName.size() - 3);
				}

				// basic validation to avoid paths
				for (std::size_t i = 0; i < modName.size(); ++i)
				{
					const char ch = modName[i];
					const bool ok =
						(ch >= 'a' && ch <= 'z') ||
						(ch >= 'A' && ch <= 'Z') ||
						(ch >= '0' && ch <= '9') ||
						(ch == '_') ||
						(ch == '.');
					if (!ok)
					{
						IAbstractChat::GetSingleton().AppendChat(CHAT_TYPE_INFO, "pyreload: gecersiz modul adi");
						return true;
					}
				}

				PyObject* pReloaded = ReloadModuleFromEterPack(modName);
				if (!pReloaded)
				{
					AppendPyExceptionToChat("pyreload: reload basarisiz");
					return true;
				}

				// Best-effort refresh hook (optional).
				static const char* s_apcRefreshFns[] = { "__refresh__", "Refresh", "OnReload", "__reload__" };
				for (int i = 0; i < (int)(sizeof(s_apcRefreshFns) / sizeof(s_apcRefreshFns[0])); ++i)
				{
					PyObject* pFn = PyObject_GetAttrString(pReloaded, s_apcRefreshFns[i]);
					if (!pFn)
					{
						PyErr_Clear();
						continue;
					}
					if (PyCallable_Check(pFn))
					{
						PyObject* pRet = PyObject_CallObject(pFn, NULL);
						if (!pRet)
							PyErr_Clear();
						else
							Py_DECREF(pRet);
					}
					Py_DECREF(pFn);
				}

				// Let the game phase window decide how to recreate/refresh visible UIs.
				if (m_apoPhaseWnd[PHASE_WINDOW_GAME])
					PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "OnPyReload", Py_BuildValue("(s)", modName.c_str()));

				Py_DECREF(pReloaded);

				IAbstractChat::GetSingleton().AppendChat(CHAT_TYPE_INFO, ("pyreload: ok -> " + modName).c_str());
				return true;
			}
		}
	}
#endif
	return false;
}

bool SplitToken(const char* c_szLine, CTokenVector* pstTokenVector, const char* c_szDelimeter)
{
	pstTokenVector->reserve(10);
	pstTokenVector->clear();

	std::string stToken;
	std::string strLine = c_szLine;

	DWORD basePos = 0;

	do
	{
		int beginPos = strLine.find_first_not_of(c_szDelimeter, basePos);
		if (beginPos < 0)
			return false;

		int endPos;

		if (strLine[beginPos] == '"')
		{
			++beginPos;
			endPos = strLine.find_first_of("\"", beginPos);

			if (endPos < 0)
				return false;

			basePos = endPos + 1;
		}
		else
		{
			endPos = strLine.find_first_of(c_szDelimeter, beginPos);
			basePos = endPos;
		}

		pstTokenVector->push_back(strLine.substr(beginPos, endPos - beginPos));

		// ??? ???. ???? ???? ??? ???? ?????. - [levites]
		if (int(strLine.find_first_not_of(c_szDelimeter, basePos)) < 0)
			break;
	} while (basePos < strLine.length());

	return true;
}

void CPythonNetworkStream::ServerCommand(char* c_szCommand)
{
	// #0000811: [M2EU] ???? ??? ???? 
	if (strcmpi(c_szCommand, "ConsoleEnable") == 0)
		return;

	if (m_apoPhaseWnd[PHASE_WINDOW_GAME])
	{
		bool isTrue;
		if (PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME],
			"BINARY_ServerCommand_Run",
			Py_BuildValue("(s)", c_szCommand),
			&isTrue
		))
		{
			if (isTrue)
				return;
		}
	}
	else if (m_poSerCommandParserWnd)
	{
		bool isTrue;
		if (PyCallClassMemberFunc(m_poSerCommandParserWnd,
			"BINARY_ServerCommand_Run",
			Py_BuildValue("(s)", c_szCommand),
			&isTrue
		))
		{
			if (isTrue)
				return;
		}
	}

	CTokenVector TokenVector;
	if (!SplitToken(c_szCommand, &TokenVector))
		return;

	if (TokenVector.empty())
		return;

	const char* szCmd = TokenVector[0].c_str();

#if defined(ENABLE_MONSTER_CARD)
	// MonsterCard server-side uses CHAT_TYPE_COMMAND text stream.
	// We intercept and translate it into data queried by official uimonstercard.py.
	if (0 == strncmp(szCmd, "monstercardsystem", 17) || 0 == strncmp(c_szCommand, "MONSTERCARDSYSTEM", 17))
	{
		// Expect "MONSTERCARDSYSTEM <payload>" or "MONSTERCARDSYSTEM <token>" etc.
		std::string cmdLine(c_szCommand);
		const std::string prefix("MONSTERCARDSYSTEM ");
		std::string payload = cmdLine;
		if (payload.find(prefix) == 0)
			payload = payload.substr(prefix.size());
		else
		{
			// lower-case path: TokenVector already lower-cased in OneArgument, but SplitToken doesn't.
			const std::string prefix2("monstercardsystem ");
			if (payload.find(prefix2) == 0)
				payload = payload.substr(prefix2.size());
		}

		CPythonMonsterCardManager& mgr = CPythonMonsterCardManager::Instance();
		mgr.LoadTable();

		// payload formats from server code:
		// ADD_DATA/Level/<n>
		// ADD_DATA/Cards/<22 ints>
		// NEW_MISSION/<16 ints>
		// REC_MAINCARDS/<c0>/<c1>/<c2>/... (indices ignored)
		// ADD_MOB_INFO/<vnum>/<collected>/<kills>/<need>/<stage>
		std::vector<std::string> parts;
		parts.reserve(32);
		std::string cur;
		for (std::size_t i = 0; i < payload.size(); ++i)
		{
			const char ch = payload[i];
			if (ch == '/')
			{
				parts.push_back(cur);
				cur.clear();
			}
			else
			{
				cur.push_back(ch);
			}
		}
		if (!cur.empty())
			parts.push_back(cur);

		if (!parts.empty())
		{
			if (parts[0] == "ADD_DATA" && parts.size() >= 3)
			{
				if (parts[1] == "Level")
					mgr.SetMissionStage(static_cast<int>(std::strtol(parts[2].c_str(), nullptr, 10)));
				// Payload: ADD_DATA/Cards/<22 ints> -> parts[0..1] + 22 numbers => parts.size() == 24
				else if (parts[1] == "Cards" && parts.size() >= 24)
				{
					// Cards payload is 22 values: 3 main + 16 deck + 3 killed
					DWORD main0 = static_cast<DWORD>(std::strtoul(parts[2 + 0].c_str(), nullptr, 10));
					DWORD main1 = static_cast<DWORD>(std::strtoul(parts[2 + 1].c_str(), nullptr, 10));
					DWORD main2 = static_cast<DWORD>(std::strtoul(parts[2 + 2].c_str(), nullptr, 10));
					mgr.SetMissionMainCards(main0, main1, main2);

					std::vector<DWORD> deck;
					deck.reserve(16);
					for (int d = 0; d < 16; ++d)
						deck.push_back(static_cast<DWORD>(std::strtoul(parts[2 + 3 + d].c_str(), nullptr, 10)));
					mgr.SetMissionDeck(deck);

					const int k0 = static_cast<int>(std::strtol(parts[2 + 3 + 16 + 0].c_str(), nullptr, 10));
					const int k1 = static_cast<int>(std::strtol(parts[2 + 3 + 16 + 1].c_str(), nullptr, 10));
					const int k2 = static_cast<int>(std::strtol(parts[2 + 3 + 16 + 2].c_str(), nullptr, 10));
					mgr.SetMissionKilledFlags(k0, k1, k2);
				}
				return;
			}
			if (parts[0] == "NEW_MISSION" && parts.size() >= 1 + 16)
			{
				std::vector<DWORD> deck;
				deck.reserve(16);
				for (int i = 0; i < 16; ++i)
					deck.push_back(static_cast<DWORD>(std::strtoul(parts[1 + i].c_str(), nullptr, 10)));
				mgr.SetMissionDeck(deck);
				mgr.SetMissionLoaded(true);
				return;
			}
			if (parts[0] == "REC_MAINCARDS" && parts.size() >= 4)
			{
				mgr.SetMissionMainCards(
					static_cast<DWORD>(std::strtoul(parts[1].c_str(), nullptr, 10)),
					static_cast<DWORD>(std::strtoul(parts[2].c_str(), nullptr, 10)),
					static_cast<DWORD>(std::strtoul(parts[3].c_str(), nullptr, 10))
				);
				mgr.SetMissionLoaded(true);
				if (m_apoPhaseWnd[PHASE_WINDOW_GAME])
					PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "RefreshMissionPage", Py_BuildValue("()"));
				return;
			}
			if (parts[0] == "SUCCES_KILL" && parts.size() >= 2)
			{
				// kill flags already come via ADD_DATA/Cards; ignore.
				mgr.SetMissionLoaded(true);
				return;
			}
			if (parts[0] == "ADD_MOB_INFO" && parts.size() >= 6)
			{
				CPythonMonsterCardManager::SMobInfo info {};
				const DWORD vnum = static_cast<DWORD>(std::strtoul(parts[1].c_str(), nullptr, 10));
				info.collectedCards = static_cast<int>(std::strtol(parts[2].c_str(), nullptr, 10));
				info.killCount = static_cast<int>(std::strtol(parts[3].c_str(), nullptr, 10));
				info.needCards = static_cast<int>(std::strtol(parts[4].c_str(), nullptr, 10));
				info.stage = static_cast<int>(std::strtol(parts[5].c_str(), nullptr, 10));
				info.lastTeleport = 0;
				info.lastPoly = 0;
				info.lastSpawn = 0;
				// Backward compatible: newer server sends timestamps too.
				if (parts.size() >= 9)
				{
					info.lastTeleport = static_cast<long long>(std::strtoll(parts[6].c_str(), nullptr, 10));
					info.lastPoly = static_cast<long long>(std::strtoll(parts[7].c_str(), nullptr, 10));
					info.lastSpawn = static_cast<long long>(std::strtoll(parts[8].c_str(), nullptr, 10));
				}
				mgr.UpsertMobInfo(vnum, info);
				mgr.SetIllustrationLoaded(true);
				// Refresh UI immediately so star count updates without reopening.
				if (m_apoPhaseWnd[PHASE_WINDOW_GAME])
					PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "MonsterCardIllustrationRefresh", Py_BuildValue("()"));
				return;
			}
			if (parts[0] == "NO_NEW_ORDER" && parts.size() >= 2)
			{
				const long long seconds = static_cast<long long>(std::strtoll(parts[1].c_str(), nullptr, 10));
				if (m_apoPhaseWnd[PHASE_WINDOW_GAME])
					PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "MonsterCardShowCooldown", Py_BuildValue("(L)", seconds));
				return;
			}
			if (parts[0] == "NO_NEED_STAGE" && parts.size() >= 2)
			{
				const int needStage = static_cast<int>(std::strtol(parts[1].c_str(), nullptr, 10));
				if (m_apoPhaseWnd[PHASE_WINDOW_GAME])
					PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "MonsterCardShowNeedStage", Py_BuildValue("(i)", needStage));
				return;
			}
			if (parts[0] == "ACHIEV_APPLY" && parts.size() >= 3)
			{
				const DWORD vnum = static_cast<DWORD>(std::strtoul(parts[1].c_str(), nullptr, 10));
				const int applied = static_cast<int>(std::strtol(parts[2].c_str(), nullptr, 10));
				mgr.SetAchievApplied(vnum, applied != 0);
				if (m_apoPhaseWnd[PHASE_WINDOW_GAME])
					PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "MonsterCardAchievRefresh", Py_BuildValue("()"));
				return;
			}
			if (parts[0] == "ACHIEV_REGIST" && parts.size() >= 3)
			{
				const DWORD vnum = static_cast<DWORD>(std::strtoul(parts[1].c_str(), nullptr, 10));
				const int rank = static_cast<int>(std::strtol(parts[2].c_str(), nullptr, 10));
				mgr.SetAchievRegistRank(vnum, rank);
				if (m_apoPhaseWnd[PHASE_WINDOW_GAME])
					PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "MonsterCardAchievRefresh", Py_BuildValue("()"));
				return;
			}
			if (parts[0] == "MISSION_FAIL" && parts.size() >= 3)
			{
				const int type = static_cast<int>(std::strtol(parts[1].c_str(), nullptr, 10));
				const int data = static_cast<int>(std::strtol(parts[2].c_str(), nullptr, 10));
				if (m_apoPhaseWnd[PHASE_WINDOW_GAME])
					PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "MonsterCardMissionFail", Py_BuildValue("(ii)", type, data));
				return;
			}
			if (parts[0] == "OPEN")
			{
				if (m_apoPhaseWnd[PHASE_WINDOW_GAME])
					PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "RefreshMissionPage", Py_BuildValue("()"));
				return;
			}
		}
		return;
	}
#endif

#if defined(ENABLE_EVENT_BANNER)
	// NOTE : Instead of creating an additional packet to enable events, we will just
	// take advantage of the command packet and filter the existing events, making it
	// easier to control. However, this code below is subject to change in the future
	// to its own packet.
	BYTE bInGameEventType = CPythonInGameEventSystemManager::Instance().GetInGameEventType(szCmd);
	if (bInGameEventType != CPythonInGameEventSystemManager::INGAME_EVENT_TYPE_NONE)
	{
		if (2 != TokenVector.size())
		{
			TraceError("CPythonNetworkStream::ServerCommand(c_szCommand=%s) - Strange Parameter Count : %d", c_szCommand, TokenVector.size());
			return;
		}

		int iFlag = atoi(TokenVector[1].c_str());
		CPythonInGameEventSystemManager::Instance().SetInGameEventEnable(bInGameEventType, iFlag);
		return;
	}
#endif

	if (!strcmpi(szCmd, "quit"))
	{
		PostQuitMessage(0);
	}
	else if (!strcmpi(szCmd, "BettingMoney"))
	{
		if (2 != TokenVector.size())
		{
			TraceError("CPythonNetworkStream::ServerCommand(c_szCommand=%s) - Strange Parameter Count : %s", c_szCommand);
			return;
		}

		// UINT uMoney = atoi(TokenVector[1].c_str());
	}
#if defined(ENABLE_SEND_TARGET_INFO)
	else if (!strcmpi(szCmd, "RefreshMonsterDropInfo"))
	{
		if (TokenVector.size() != 2)
		{
			TraceError("CPythonNetworkStream::ServerCommand(c_szCommand=%s) - Strange Parameter Count : %s", c_szCommand);
			return;
		}

		const DWORD dwRace = atoi(TokenVector[1].c_str());
		PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "BINARY_RefreshTargetMonsterDropInfo", Py_BuildValue("(i)", dwRace));
	}
#endif
	// GIFT NOTIFY
	else if (!strcmpi(szCmd, "gift"))
	{
		PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "Gift_Show", Py_BuildValue("()"));
	}
#if defined(ENVIRONMENT_SYSTEM)
	else if (!strcmpi(szCmd, "SnowTexture"))
	{
		if (2 != TokenVector.size()) {
			TraceError("CPythonNetworkStream::ServerCommand(c_szCommand=%s) - Strange Parameter Count", c_szCommand);
			return;
		}
		
		if (!CPythonBackground::Instance().IsMapOutdoor())
			return;
		
		static const std::unordered_map<std::string, std::pair<std::string, std::string>> TextureList = {
			{ "metin2_map_a1", { "textureset/metin2_a1.txt", "textureset/snow/snow_metin2_a1.txt" }},
			{ "map_a2",	{ "textureset/metin2_a2.txt", "textureset/snow/snow_metin2_a2.txt" }},
			{ "metin2_map_a3", { "textureset/metin2_a3.txt", "textureset/snow/snow_metin2_a3.txt" }},
			{ "metin2_map_b1", { "textureset/metin2_b1.txt", "textureset/snow/snow_metin2_b1.txt" }},
			{ "metin2_map_b3", { "textureset/metin2_b3.txt", "textureset/snow/snow_metin2_b3.txt" }},
			{ "metin2_map_c1", { "textureset/metin2_c1.txt", "textureset/snow/snow_metin2_c1.txt" }},
			{ "metin2_map_c3", { "textureset/metin2_c3.txt", "textureset/snow/snow_metin2_c3.txt" }},
			{ "metin2_map_milgyo", { "textureset/metin2_milgyo.txt", "textureset/snow/snow_metin2_milgyo.txt" }},
			{ "metin2_map_n_flame_01", { "textureset/metin2_n_flame_01.txt", "textureset/snow/snow_metin2_n_flame_01.txt" }},
			{ "metin2_map_n_desert_01", { "textureset/metin2_n_desert1.txt", "textureset/snow/snow_metin2_n_desert_01.txt" }},
			{ "season1/metin2_map_oxevent", { "textureset/metin2_map_oxevent.txt", "textureset/snow/snow_metin2_map_oxevent.txt" }},
		};
		
		const char* pszMap = CPythonBackground::Instance().GetWarpMapName();
		if (!pszMap)
			return;

		auto it = TextureList.find(std::string(pszMap));
		if (it == TextureList.end())
			return;

		// pair: .first = normal textureset, .second = snow textureset
		if (!TokenVector.at(1).compare("enable"))
			CPythonBackground::Instance().GetMapOutdoorRef().ChangeTextureset(it->second.second);
		else
			CPythonBackground::Instance().GetMapOutdoorRef().ChangeTextureset(it->second.first);
	}
#endif
#if defined(ENABLE_GEM_SYSTEM)
	else if (!strcmpi(szCmd, "BINARY_OpenSelectItemWindowEx"))
	{
		PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "BINARY_OpenSelectItemWindowEx", Py_BuildValue("()"));
	}
	else if (!strcmpi(szCmd, "BINARY_RefreshSelectItemWindowEx"))
	{
		PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "BINARY_RefreshSelectItemWindowEx", Py_BuildValue("()"));
	}
#endif
#if !defined(ENABLE_CUBE_RENEWAL)
	// CUBE
	else if (!strcmpi(szCmd, "cube"))
	{
		if (TokenVector.size() < 2)
		{
			TraceError("CPythonNetworkStream::ServerCommand(c_szCommand=%s) - Strange Parameter Count : %s", c_szCommand);
			return;
		}

		if ("open" == TokenVector[1])
		{
			if (4 > TokenVector.size())
			{
				TraceError("CPythonNetworkStream::ServerCommand(c_szCommand=%s) - Strange Parameter Count : %s", c_szCommand);
				return;
			}

			DWORD npcVNUM = (DWORD)atoi(TokenVector[2].c_str());
			PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "BINARY_Cube_Open", Py_BuildValue("(ii)", npcVNUM, std::stoi(TokenVector[3])));
		}
		else if ("close" == TokenVector[1])
		{
			PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "BINARY_Cube_Close", Py_BuildValue("()"));
		}
		else if ("info" == TokenVector[1])
		{
			if (5 != TokenVector.size())
			{
				TraceError("CPythonNetworkStream::ServerCommand(c_szCommand=%s) - Strange Parameter Count : %s", c_szCommand);
				return;
			}

			UINT gold = atoi(TokenVector[2].c_str());
			UINT itemVnum = atoi(TokenVector[3].c_str());
			UINT count = atoi(TokenVector[4].c_str());
			PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "BINARY_Cube_UpdateInfo", Py_BuildValue("(iii)", gold, itemVnum, count));
		}
		else if ("success" == TokenVector[1])
		{
			if (4 != TokenVector.size())
			{
				TraceError("CPythonNetworkStream::ServerCommand(c_szCommand=%s) - Strange Parameter Count : %s", c_szCommand);
				return;
			}

			UINT itemVnum = atoi(TokenVector[2].c_str());
			UINT count = atoi(TokenVector[3].c_str());
			PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "BINARY_Cube_Succeed", Py_BuildValue("(ii)", itemVnum, count));
		}
		else if ("fail" == TokenVector[1])
		{
			PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "BINARY_Cube_Failed", Py_BuildValue("()"));
		}
		else if ("r_list" == TokenVector[1])
		{
			// result list (/cube r_list npcVNUM resultCount resultText)
			// 20383 4 72723,1/72725,1/72730.1/50001,5 <- ????????? "/" ????? ?????? ??????? ??
			if (5 != TokenVector.size())
			{
				TraceError("CPythonNetworkStream::ServerCommand(c_szCommand=%s) - Strange Parameter Count : %d", c_szCommand, 5);
				return;
			}

			DWORD npcVNUM = (DWORD)atoi(TokenVector[2].c_str());

			PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "BINARY_Cube_ResultList", Py_BuildValue("(is)", npcVNUM, TokenVector[4].c_str()));
		}
		else if ("m_info" == TokenVector[1])
		{
			// material list (/cube m_info requestStartIndex resultCount MaterialText)
			// ex) requestStartIndex: 0, resultCount : 5 - ??? NPC?? ????? ??? ?????? ?? 0~4?????? ?????? ???????? ????? ?? ????? ??? ?????? MaterialText?? ???????
			// ?? ??????? ???????? ????? ??? ?????? "@" ????? ???
			// 0 5 125,1|126,2|127,2|123,5&555,5&555,4/120000 <- ????????? ???????? ???? ??????? ??

			if (5 != TokenVector.size())
			{
				TraceError("CPythonNetworkStream::ServerCommand(c_szCommand=%s) - Strange Parameter Count : %d", c_szCommand, 5);
				return;
			}

			UINT requestStartIndex = (UINT)atoi(TokenVector[2].c_str());
			UINT resultCount = (UINT)atoi(TokenVector[3].c_str());

			PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "BINARY_Cube_MaterialInfo", Py_BuildValue("(iis)", requestStartIndex, resultCount, TokenVector[4].c_str()));
		}
	}
	// CUEBE_END
#endif
	else if (!strcmpi(szCmd, "ObserverCount"))
	{
		if (2 != TokenVector.size())
		{
			TraceError("CPythonNetworkStream::ServerCommand(c_szCommand=%s) - Strange Parameter Count : %s", c_szCommand);
			return;
		}

		UINT uObserverCount = atoi(TokenVector[1].c_str());

		PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME],
			"BINARY_BettingGuildWar_UpdateObserverCount",
			Py_BuildValue("(i)", uObserverCount)
		);
	}
	else if (!strcmpi(szCmd, "ObserverMode"))
	{
		if (2 != TokenVector.size())
		{
			TraceError("CPythonNetworkStream::ServerCommand(c_szCommand=%s) - Strange Parameter Count : %s", c_szCommand);
			return;
		}

		UINT uMode = atoi(TokenVector[1].c_str());

		IAbstractPlayer& rkPlayer = IAbstractPlayer::GetSingleton();
		rkPlayer.SetObserverMode(uMode ? true : false);

		PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME],
			"BINARY_BettingGuildWar_SetObserverMode",
			Py_BuildValue("(i)", uMode)
		);
	}
	else if (!strcmpi(szCmd, "ObserverTeamInfo"))
	{
	}
#if defined(ENABLE_OX_RENDER_AREA)
	else if (!strcmpi(szCmd, "OXAreaRender"))
	{
		if (2 != TokenVector.size())
		{
			TraceError("CPythonNetworkStream::ServerCommand(c_szCommand=%s) - Strange Parameter Count : %s", c_szCommand);
			return;
		}

		CPythonBackground::Instance().RenderOXEventArea(static_cast<CPythonBackground::OXArea>(atoi(TokenVector[1].c_str())));
	}
#endif
#if defined(ENABLE_LOCALE_CLIENT)
	else if (!strcmpi(szCmd, "language_change"))
	{
		PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "LanguageChange", Py_BuildValue("()"));
	}
#endif
#if defined(ENABLE_METINSTONE_SWAP)
	else if (!strcmpi(szCmd, "metinstone_swap"))
	{
		if (2 != TokenVector.size())
		{
			TraceError("CPythonNetworkStream::ServerCommand(c_szCommand=%s) - Strange Parameter Count : %s", c_szCommand);
			return;
		}

		MetinStoneSwap::Manager::Instance().SetEventFlag(atoi(TokenVector[1].c_str()));
	}
#endif
	else if (!strcmpi(szCmd, "StoneDetect"))
	{
		if (4 != TokenVector.size())
		{
			TraceError("CPythonNetworkStream::ServerCommand(c_szCommand=%s) - Strange Parameter Count : %s", c_szCommand);
			return;
		}

		// vid distance(1-3) angle(0-360)
		DWORD dwVID = atoi(TokenVector[1].c_str());
		BYTE byDistance = atoi(TokenVector[2].c_str());
		float fAngle = atof(TokenVector[3].c_str());
		fAngle = fmod(540.0f - fAngle, 360.0f);
		Tracef("StoneDetect [VID:%d] [Distance:%d] [Angle:%d->%f]\n", dwVID, byDistance, atoi(TokenVector[3].c_str()), fAngle);

		IAbstractCharacterManager& rkChrMgr = IAbstractCharacterManager::GetSingleton();

		CInstanceBase* pInstance = rkChrMgr.GetInstancePtr(dwVID);
		if (!pInstance)
		{
			TraceError("CPythonNetworkStream::ServerCommand(c_szCommand=%s) - Not Exist Instance", c_szCommand);
			return;
		}

		TPixelPosition PixelPosition;
		D3DXVECTOR3 v3Rotation(0.0f, 0.0f, fAngle);
		pInstance->NEW_GetPixelPosition(&PixelPosition);

		PixelPosition.y *= -1.0f;

		switch (byDistance)
		{
		case 0:
			CEffectManager::Instance().RegisterEffect("d:/ymir work/effect/etc/firecracker/find_out.mse");
			CEffectManager::Instance().CreateEffect("d:/ymir work/effect/etc/firecracker/find_out.mse", PixelPosition, v3Rotation);
			break;
		case 1:
			CEffectManager::Instance().RegisterEffect("d:/ymir work/effect/etc/compass/appear_small.mse");
			CEffectManager::Instance().CreateEffect("d:/ymir work/effect/etc/compass/appear_small.mse", PixelPosition, v3Rotation);
			break;
		case 2:
			CEffectManager::Instance().RegisterEffect("d:/ymir work/effect/etc/compass/appear_middle.mse");
			CEffectManager::Instance().CreateEffect("d:/ymir work/effect/etc/compass/appear_middle.mse", PixelPosition, v3Rotation);
			break;
		case 3:
			CEffectManager::Instance().RegisterEffect("d:/ymir work/effect/etc/compass/appear_large.mse");
			CEffectManager::Instance().CreateEffect("d:/ymir work/effect/etc/compass/appear_large.mse", PixelPosition, v3Rotation);
			break;
		default:
			TraceError("CPythonNetworkStream::ServerCommand(c_szCommand=%s) - Strange Distance", c_szCommand);
			break;
		}

#ifdef _DEBUG
		IAbstractChat& rkChat = IAbstractChat::GetSingleton();
		rkChat.AppendChat(CHAT_TYPE_INFO, c_szCommand);
#endif
	}
	else if (!strcmpi(szCmd, "StartStaminaConsume"))
	{
		if (3 != TokenVector.size())
		{
			TraceError("CPythonNetworkStream::ServerCommand(c_szCommand=%s) - Strange Parameter Count : %s", c_szCommand);
			return;
		}

		DWORD dwConsumePerSec = atoi(TokenVector[1].c_str());
		DWORD dwCurrentStamina = atoi(TokenVector[2].c_str());

		IAbstractPlayer& rPlayer = IAbstractPlayer::GetSingleton();
		rPlayer.StartStaminaConsume(dwConsumePerSec, dwCurrentStamina);
	}
	else if (!strcmpi(szCmd, "StopStaminaConsume"))
	{
		if (2 != TokenVector.size())
		{
			TraceError("CPythonNetworkStream::ServerCommand(c_szCommand=%s) - Strange Parameter Count : %d", c_szCommand, TokenVector.size());
			return;
		}

		DWORD dwCurrentStamina = atoi(TokenVector[1].c_str());

		IAbstractPlayer& rPlayer = IAbstractPlayer::GetSingleton();
		rPlayer.StopStaminaConsume(dwCurrentStamina);
	}
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
	else if (!strcmpi(szCmd, "combo"))
	{
		int iFlag = atoi(TokenVector[1].c_str());
		IAbstractPlayer& rPlayer = IAbstractPlayer::GetSingleton();
		rPlayer.SetComboSkillFlag(iFlag);
		m_bComboSkillFlag = iFlag ? true : false;
	}
	else if (!strcmpi(szCmd, "setblockmode"))
	{
		int iFlag = atoi(TokenVector[1].c_str());
		PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "OnBlockMode", Py_BuildValue("(i)", iFlag));
	}
	else
	{
		TraceError("Unknown Server Command %s | %s", c_szCommand, szCmd);
	}
}
