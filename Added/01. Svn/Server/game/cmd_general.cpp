#include "stdafx.h"
#ifdef __FreeBSD__
#include <md5.h>
#else
#include "../../libthecore/include/xmd5.h"
#endif

#include "utils.h"
#include "config.h"
#include "desc_client.h"
#include "desc_manager.h"
#include "char.h"
#include "char_manager.h"
#include "motion.h"
#include "packet.h"
#include "affect.h"
#include "pvp.h"
#include "start_position.h"
#include "party.h"
#include "guild_manager.h"
#include "p2p.h"
#include "dungeon.h"
#include "messenger_manager.h"
#include "war_map.h"
#include "questmanager.h"
#include "item_manager.h"
#include "monarch.h"
#include "mob_manager.h"
#include "dev_log.h"
#include "item.h"
#include "arena.h"
#include "buffer_manager.h"
#include "unique_item.h"
#include "threeway_war.h"
#include "log.h"
#include "../../common/VnumHelper.h"

#if defined(ENABLE_MONSTER_CARD)
#	include "MonstercardSystem.h"
#endif

extern int g_server_id;

extern int g_nPortalLimitTime;

ACMD(do_user_horse_ride)
{
	if (ch->IsObserverMode())
		return;

	if (ch->IsDead() || ch->IsStun())
		return;

	if (ch->IsHorseRiding() == false)
	{
		if (ch->GetMountVnum())
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_STRING("You're already riding. Get off first."));
			return;
		}

		if (ch->GetHorse() == NULL)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_STRING("Please call your Horse first."));
			return;
		}

		ch->StartRiding();
	}
	else
	{
		ch->StopRiding();
	}
}

ACMD(do_user_horse_back)
{
	if (ch->GetHorse() != NULL)
	{
		ch->HorseSummon(false);
		ch->ChatPacket(CHAT_TYPE_INFO, LC_STRING("You have sent your horse away."));
	}
	else if (ch->IsHorseRiding() == true)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_STRING("You have to get off your Horse."));
	}
	else
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_STRING("Please call your Horse first."));
	}
}

ACMD(do_user_horse_feed)
{
	if (ch->GetMyShop())
		return;

	if (!ch->GetHorse())
	{
		if (ch->IsHorseRiding() == false)
			ch->ChatPacket(CHAT_TYPE_INFO, LC_STRING("Please call your Horse first."));
		else
			ch->ChatPacket(CHAT_TYPE_INFO, LC_STRING("You cannot feed your Horse whilst sitting on it."));
		return;
	}

	DWORD dwFood = ch->GetHorseGrade() + 50054 - 1;

	if (ch->CountSpecifyItem(dwFood) > 0)
	{
		ch->RemoveSpecifyItem(dwFood, 1);
		ch->FeedHorse();

		const char* c_szConv = under_han(ITEM_MANAGER::instance().GetTable(dwFood)->szLocaleName) ? LC_STRING("the") : LC_STRING("the");
		ch->ChatPacket(CHAT_TYPE_INFO, LC_STRING("You have fed the Horse with %s%s.",
			LC_ITEM(ITEM_MANAGER::instance().GetTable(dwFood)->dwVnum), c_szConv));
	}
	else
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_STRING("You need %s.", LC_ITEM(ITEM_MANAGER::instance().GetTable(dwFood)->dwVnum)));
	}
}

#if defined(ENABLE_MONSTER_CARD)
ACMD(do_monstercard)
{
	const auto& sys = ch->GetMonsterCardSystem();
	if (sys == nullptr)
	{
		sys_err("FATAL! MonstercardSystem from %s was nullptr", ch->GetName());
		return;
	}
	sys->DispatchClientCommand(argument);
}

ACMD(do_cardmonster_1_full)
{
	const auto& sys = ch->GetMonsterCardSystem();
	if (sys == nullptr)
		return;
	sys->DebugCompleteCurrentMission();
}

ACMD(do_cardmonster_2_full)
{
	const auto& sys = ch->GetMonsterCardSystem();
	if (sys == nullptr)
		return;
	// NOTE: Server-side does not distinguish solo vs party collections; this boosts all known monster cards.
	sys->DebugIncreaseAllCollectionsStage();
}

ACMD(do_cardmonster_3_full)
{
	const auto& sys = ch->GetMonsterCardSystem();
	if (sys == nullptr)
		return;
	// NOTE: Server-side does not distinguish solo vs party collections; this boosts all known monster cards.
	sys->DebugIncreaseAllCollectionsStage();
}

ACMD(do_cardmonster_reset)
{
	const auto& sys = ch->GetMonsterCardSystem();
	if (sys == nullptr)
		return;
	sys->DebugResetAll();
}
#endif

#define MAX_REASON_LEN 128

EVENTINFO(TimedEventInfo)
{
	DynamicCharacterPtr ch;
	int subcmd;
	int left_second;
	char szReason[MAX_REASON_LEN];

	TimedEventInfo()
		: ch()
		, subcmd(0)
		, left_second(0)
	{
		::memset(szReason, 0, MAX_REASON_LEN);
	}
};

struct SendDisconnectFunc
{
	void operator () (LPDESC d)
	{
		if (d->GetCharacter())
		{
			if (d->GetCharacter()->GetGMLevel() == GM_PLAYER)
				d->GetCharacter()->ChatPacket(CHAT_TYPE_COMMAND, "quit Shutdown(SendDisconnectFunc)");
		}
	}
};

struct DisconnectFunc
{
	void operator () (LPDESC d)
	{
		if (d->GetType() == DESC_TYPE_CONNECTOR)
			return;

		if (d->IsPhase(PHASE_P2P))
			return;

		if (d->GetCharacter())
			d->GetCharacter()->Disconnect("Shutdown(DisconnectFunc)");

		d->SetPhase(PHASE_CLOSE);
	}
};

EVENTINFO(shutdown_event_data)
{
	int seconds;

	shutdown_event_data()
		: seconds(0)
	{
	}
};

EVENTFUNC(shutdown_event)
{
	shutdown_event_data* info = dynamic_cast<shutdown_event_data*>(event->info);

	if (info == NULL)
	{
		sys_err("shutdown_event> <Factor> Null pointer");
		return 0;
	}

	int* pSec = &(info->seconds);

	if (*pSec < 0)
	{
		sys_log(0, "shutdown_event sec %d", *pSec);

		if (--*pSec == -10)
		{
			const DESC_MANAGER::DESC_SET& c_set_desc = DESC_MANAGER::instance().GetClientSet();
			std::for_each(c_set_desc.begin(), c_set_desc.end(), DisconnectFunc());
			return passes_per_sec;
		}
		else if (*pSec < -10)
			return 0;

		return passes_per_sec;
	}
	else if (*pSec == 0)
	{
		const DESC_MANAGER::DESC_SET& c_set_desc = DESC_MANAGER::instance().GetClientSet();
		std::for_each(c_set_desc.begin(), c_set_desc.end(), SendDisconnectFunc());
		g_bNoMoreClient = true;
		--*pSec;
		return passes_per_sec;
	}
	else
	{
		char buf[64];
		snprintf(buf, sizeof(buf), LC_STRING("%d seconds until Exit.", *pSec));
		SendNotice(buf);

		--*pSec;
		return passes_per_sec;
	}
}

void Shutdown(int iSec)
{
	if (g_bNoMoreClient)
	{
		thecore_shutdown();
		return;
	}

	CWarMapManager::instance().OnShutdown();

	char buf[64];
	snprintf(buf, sizeof(buf), LC_STRING("The game will be closed in %d seconds.", iSec));

	SendNotice(buf);

	shutdown_event_data* info = AllocEventInfo<shutdown_event_data>();
	info->seconds = iSec;

	event_create(shutdown_event, info, 1);
}

ACMD(do_shutdown)
{
	TPacketGGShutdown p;
	p.bHeader = HEADER_GG_SHUTDOWN;
	P2P_MANAGER::instance().Send(&p, sizeof(TPacketGGShutdown));

	Shutdown(10);
}

EVENTFUNC(timed_event)
{
	TimedEventInfo* info = dynamic_cast<TimedEventInfo*>(event->info);

	if (info == NULL)
	{
		sys_err("timed_event> <Factor> Null pointer");
		return 0;
	}

	LPCHARACTER ch = info->ch;

	if (ch == NULL) // <Factor>
		return 0;

	LPDESC d = ch->GetDesc();

	if (info->left_second <= 0)
	{
		ch->m_pkTimedEvent = NULL;

		if (true == LC_IsEurope() || true == LC_IsYMIR() || true == LC_IsKorea())
		{
			switch (info->subcmd)
			{
				case SCMD_LOGOUT:
				case SCMD_QUIT:
				case SCMD_PHASE_SELECT:
#if defined(__LOCALE_CLIENT__)
				case SCMD_LANGUAGE_CHANGE:
#endif
				{
					TPacketNeedLoginLogInfo acc_info;
					acc_info.dwPlayerID = ch->GetDesc()->GetAccountTable().id;
					db_clientdesc->DBPacket(HEADER_GD_VALID_LOGOUT, 0, &acc_info, sizeof(acc_info));

					LogManager::instance().DetailLoginLog(false, ch);
				}
				break;
			}
		}

		switch (info->subcmd)
		{
			case SCMD_LOGOUT:
				if (d)
					d->SetPhase(PHASE_CLOSE);
				break;

			case SCMD_QUIT:
				ch->ChatPacket(CHAT_TYPE_COMMAND, "quit");
				break;

			case SCMD_PHASE_SELECT:
			{
				ch->Disconnect("timed_event - SCMD_PHASE_SELECT");

				if (d)
				{
					d->SetPhase(PHASE_SELECT);
				}
			}
			break;

#if defined(__LOCALE_CLIENT__)
			case SCMD_LANGUAGE_CHANGE:
			{
				ch->ChatPacket(CHAT_TYPE_COMMAND, "language_change");
				ch->Disconnect("timed_event - SCMD_LANGUAGE_CHANGE");

				if (d)
					d->SetPhase(PHASE_SELECT);
			}

			break;
#endif
		}

		return 0;
	}
	else
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_STRING("%d seconds until Exit.", info->left_second));
		--info->left_second;
	}

	return PASSES_PER_SEC(1);
}

ACMD(do_cmd)
{
	// RECALL_DELAY
	/*
	if (ch->m_pkRecallEvent != NULL)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_STRING("Your logout has been cancelled."));
		event_cancel(&ch->m_pkRecallEvent);
		return;
	}
	*/
	// END_OF_RECALL_DELAY

	if (ch->m_pkTimedEvent)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_STRING("Your logout has been cancelled."));
		event_cancel(&ch->m_pkTimedEvent);
		return;
	}

	switch (subcmd)
	{
		case SCMD_LOGOUT:
			ch->ChatPacket(CHAT_TYPE_INFO, LC_STRING("Back to login window. Please wait."));
			break;

		case SCMD_QUIT:
			ch->ChatPacket(CHAT_TYPE_INFO, LC_STRING("You have been disconnected from the server. Please wait."));
			break;

		case SCMD_PHASE_SELECT:
#if defined(__LOCALE_CLIENT__)
		case SCMD_LANGUAGE_CHANGE:
#endif
			ch->ChatPacket(CHAT_TYPE_INFO, LC_STRING("You are changing character. Please wait."));
			break;
	}

	int nExitLimitTime = 10;

	if (ch->IsHack(false, true, nExitLimitTime) &&
		false == CThreeWayWar::instance().IsSungZiMapIndex(ch->GetMapIndex()) &&
		(!ch->GetWarMap() || ch->GetWarMap()->GetType() == GUILD_WAR_TYPE_FLAG))
	{
		return;
	}

	switch (subcmd)
	{
		case SCMD_LOGOUT:
		case SCMD_QUIT:
		case SCMD_PHASE_SELECT:
#if defined(__LOCALE_CLIENT__)
		case SCMD_LANGUAGE_CHANGE:
#endif
		{
			TimedEventInfo* info = AllocEventInfo<TimedEventInfo>();

			{
				if (ch->IsPosition(POS_FIGHTING))
					info->left_second = 10;
				else
					info->left_second = 3;
			}

			info->ch = ch;
			info->subcmd = subcmd;
			strlcpy(info->szReason, argument, sizeof(info->szReason));

			ch->m_pkTimedEvent = event_create(timed_event, info, 1);
		}
		break;
	}
}

ACMD(do_mount)
{
	/*
	char arg1[256];
	struct action_mount_param param;

	// if already riding
	if (ch->GetMountingChr())
	{
		char arg2[256];
		two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

		if (!*arg1 || !*arg2)
			return;

		param.x = atoi(arg1);
		param.y = atoi(arg2);
		param.vid = ch->GetMountingChr()->GetVID();
		param.is_unmount = true;

		float distance = DISTANCE_SQRT(param.x - (DWORD)ch->GetX(), param.y - (DWORD)ch->GetY());

		if (distance > 600.0f)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_STRING("Get a little closer and get off."));
			return;
		}

		action_enqueue(ch, ACTION_TYPE_MOUNT, &param, 0.0f, true);
		return;
	}

	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
		return;

	LPCHARACTER tch = CHARACTER_MANAGER::instance().Find(atoi(arg1));

	if (!tch->IsNPC() || !tch->IsMountable())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_STRING("You can't ride there."));
		return;
	}

	float distance = DISTANCE_SQRT(tch->GetX() - ch->GetX(), tch->GetY() - ch->GetY());

	if (distance > 600.0f)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_STRING("Come a little closer and ride."));
		return;
	}

	param.vid = tch->GetVID();
	param.is_unmount = false;

	action_enqueue(ch, ACTION_TYPE_MOUNT, &param, 0.0f, true);
	*/
}

ACMD(do_fishing)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
		return;

	ch->SetRotation(atof(arg1));
	ch->fishing();
}

ACMD(do_console)
{
	ch->ChatPacket(CHAT_TYPE_COMMAND, "ConsoleEnable");
}

ACMD(do_restart)
{
	ch->Restart(subcmd);
}

ACMD(do_stat_reset)
{
	ch->PointChange(POINT_STAT_RESET_COUNT, 12 - ch->GetPoint(POINT_STAT_RESET_COUNT));
}

ACMD(do_stat_minus)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
		return;

	if (ch->IsPolymorphed())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_STRING("You cannot change your status while you are transformed."));
		return;
	}

	if (ch->GetPoint(POINT_STAT_RESET_COUNT) <= 0)
		return;

	if (!strcmp(arg1, "st"))
	{
		if (ch->GetRealPoint(POINT_ST) <= JobInitialPoints[ch->GetJob()].st)
			return;

		ch->SetRealPoint(POINT_ST, ch->GetRealPoint(POINT_ST) - 1);
		ch->SetPoint(POINT_ST, ch->GetPoint(POINT_ST) - 1);
		ch->ComputePoints();
		ch->PointChange(POINT_ST, 0);
	}
	else if (!strcmp(arg1, "dx"))
	{
		if (ch->GetRealPoint(POINT_DX) <= JobInitialPoints[ch->GetJob()].dx)
			return;

		ch->SetRealPoint(POINT_DX, ch->GetRealPoint(POINT_DX) - 1);
		ch->SetPoint(POINT_DX, ch->GetPoint(POINT_DX) - 1);
		ch->ComputePoints();
		ch->PointChange(POINT_DX, 0);
	}
	else if (!strcmp(arg1, "ht"))
	{
		if (ch->GetRealPoint(POINT_HT) <= JobInitialPoints[ch->GetJob()].ht)
			return;

		ch->SetRealPoint(POINT_HT, ch->GetRealPoint(POINT_HT) - 1);
		ch->SetPoint(POINT_HT, ch->GetPoint(POINT_HT) - 1);
		ch->ComputePoints();
		ch->PointChange(POINT_HT, 0);
		ch->PointChange(POINT_MAX_HP, 0);
	}
	else if (!strcmp(arg1, "iq"))
	{
		if (ch->GetRealPoint(POINT_IQ) <= JobInitialPoints[ch->GetJob()].iq)
			return;

		ch->SetRealPoint(POINT_IQ, ch->GetRealPoint(POINT_IQ) - 1);
		ch->SetPoint(POINT_IQ, ch->GetPoint(POINT_IQ) - 1);
		ch->ComputePoints();
		ch->PointChange(POINT_IQ, 0);
		ch->PointChange(POINT_MAX_SP, 0);
	}
	else
		return;

	ch->PointChange(POINT_STAT, +1);
	ch->PointChange(POINT_STAT_RESET_COUNT, -1);
	ch->ComputePoints();
}

ACMD(do_stat)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
		return;

	if (ch->IsPolymorphed())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_STRING("You cannot change your status while you are transformed."));
		return;
	}

	if (ch->GetPoint(POINT_STAT) <= 0)
		return;

	POINT_TYPE idx = 0;

	if (!strcmp(arg1, "st"))
		idx = POINT_ST;
	else if (!strcmp(arg1, "dx"))
		idx = POINT_DX;
	else if (!strcmp(arg1, "ht"))
		idx = POINT_HT;
	else if (!strcmp(arg1, "iq"))
		idx = POINT_IQ;
	else
		return;

	if (ch->GetRealPoint(idx) >= gPlayerMaxLevelStats)
		return;

	ch->SetRealPoint(idx, ch->GetRealPoint(idx) + 1);
	ch->SetPoint(idx, ch->GetPoint(idx) + 1);
	ch->ComputePoints();
	ch->PointChange(idx, 0);

	if (idx == POINT_IQ)
	{
		ch->PointChange(POINT_MAX_HP, 0);
	}
	else if (idx == POINT_HT)
	{
		ch->PointChange(POINT_MAX_SP, 0);
	}

	ch->PointChange(POINT_STAT, -1);
	ch->ComputePoints();
}

#if defined(__CONQUEROR_LEVEL__)
ACMD(do_conqueror_point)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
		return;

	if (ch->IsPolymorphed())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_STRING("You cannot change your status while you are transformed."));
		return;
	}

	if (ch->GetPoint(POINT_CONQUEROR_POINT) <= 0)
		return;

	POINT_VALUE idx = 0;
	if (!strcmp(arg1, "sstr"))
		idx = POINT_SUNGMA_STR;
	else if (!strcmp(arg1, "shp"))
		idx = POINT_SUNGMA_HP;
	else if (!strcmp(arg1, "smove"))
		idx = POINT_SUNGMA_MOVE;
	else if (!strcmp(arg1, "simmune"))
		idx = POINT_SUNGMA_IMMUNE;
	else
		return;

	if (ch->GetRealPoint(idx) >= gPlayerMaxLevelStats)
		return;

	ch->SetRealPoint(idx, ch->GetRealPoint(idx) + 1);
	ch->SetPoint(idx, ch->GetPoint(idx) + 1);
	ch->ComputePoints();
	ch->PointChange(idx, 0);

	ch->PointChange(POINT_CONQUEROR_POINT, -1);
	ch->ComputePoints();

	ch->PointsPacket(); // Refresh points.
}
#endif

ACMD(do_pvp)
{
	if (ch->GetArena() != NULL || CArenaManager::instance().IsArenaMap(ch->GetMapIndex()) == true)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_STRING("You cannot use this in the duel arena."));
		return;
	}

	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	DWORD vid = 0;
	str_to_number(vid, arg1);

	LPCHARACTER pkVictim = CHARACTER_MANAGER::instance().Find(vid);

	if (!pkVictim)
		return;

	if (pkVictim->IsNPC())
		return;

#if defined(__MESSENGER_BLOCK_SYSTEM__)
	if (CMessengerManager::instance().IsBlocked(ch->GetName(), pkVictim->GetName()))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_STRING("Unblock %s to continue.", pkVictim->GetName()));
		return;
	}
	else if (CMessengerManager::instance().IsBlocked(pkVictim->GetName(), ch->GetName()))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_STRING("%s has blocked you.", pkVictim->GetName()));
		return;
	}
#endif

	if (pkVictim->GetArena() != NULL)
	{
		pkVictim->ChatPacket(CHAT_TYPE_INFO, LC_STRING("This player is currently fighting."));
		return;
	}

	CPVPManager::instance().Insert(ch, pkVictim);
}

ACMD(do_guildskillup)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
		return;

	if (!ch->GetGuild())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_STRING("[Guild] It does not belong to the guild."));
		return;
	}

	CGuild* g = ch->GetGuild();
	TGuildMember* gm = g->GetMember(ch->GetPlayerID());
	if (gm->grade == GUILD_LEADER_GRADE)
	{
		DWORD vnum = 0;
		str_to_number(vnum, arg1);
		g->SkillLevelUp(vnum);
	}
	else
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_STRING("[Guild] You do not have the authority to change the level of the guild skills."));
	}
}

ACMD(do_skillup)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
		return;

	DWORD vnum = 0;
	str_to_number(vnum, arg1);

	if (true == ch->CanUseSkill(vnum))
	{
		ch->SkillLevelUp(vnum);
	}
	else
	{
		switch (vnum)
		{
			case SKILL_HORSE_WILDATTACK:
			case SKILL_HORSE_CHARGE:
			case SKILL_HORSE_ESCAPE:
			case SKILL_HORSE_WILDATTACK_RANGE:

			case SKILL_7_A_ANTI_TANHWAN:
			case SKILL_7_B_ANTI_AMSEOP:
			case SKILL_7_C_ANTI_SWAERYUNG:
			case SKILL_7_D_ANTI_YONGBI:

			case SKILL_8_A_ANTI_GIGONGCHAM:
			case SKILL_8_B_ANTI_YEONSA:
			case SKILL_8_C_ANTI_MAHWAN:
			case SKILL_8_D_ANTI_BYEURAK:

			case SKILL_ADD_HP:
			case SKILL_RESIST_PENETRATE:

#if defined(__7AND8TH_SKILLS__)
			case SKILL_ANTI_PALBANG:
			case SKILL_ANTI_AMSEOP:
			case SKILL_ANTI_SWAERYUNG:
			case SKILL_ANTI_YONGBI:
			case SKILL_ANTI_GIGONGCHAM:
			case SKILL_ANTI_HWAJO:
			case SKILL_ANTI_MARYUNG:
			case SKILL_ANTI_BYEURAK:
			case SKILL_ANTI_SALPOONG:
			case SKILL_HELP_PALBANG:
			case SKILL_HELP_AMSEOP:
			case SKILL_HELP_SWAERYUNG:
			case SKILL_HELP_YONGBI:
			case SKILL_HELP_GIGONGCHAM:
			case SKILL_HELP_HWAJO:
			case SKILL_HELP_MARYUNG:
			case SKILL_HELP_BYEURAK:
			case SKILL_HELP_SALPOONG:
#endif
				ch->SkillLevelUp(vnum);
				break;
		}
	}
}

//
// @version 05/06/20 Bang2ni - ??  Delegate to CHARACTER class
//
ACMD(do_safebox_close)
{
	ch->CloseSafebox();
}

//
// @version 05/06/20 Bang2ni - ??  Delegate to CHARACTER class
//
ACMD(do_safebox_password)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));
	ch->ReqSafeboxLoad(arg1);
}

ACMD(do_safebox_change_password)
{
	char arg1[256];
	char arg2[256];

	two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

	if (!*arg1 || strlen(arg1) > 6)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_STRING("[Storeroom] You have entered an incorrect password."));
		return;
	}

	if (!*arg2 || strlen(arg2) > 6)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_STRING("[Storeroom] You have entered an incorrect password."));
		return;
	}

	if (LC_IsBrazil() == true)
	{
		for (int i = 0; i < 6; ++i)
		{
			if (arg2[i] == '\0')
				break;

			if (isalpha(arg2[i]) == false)
			{
				ch->ChatPacket(CHAT_TYPE_INFO, LC_STRING("[Storeroom] You have entered an incorrect password."));
				return;
			}
		}
	}

	TSafeboxChangePasswordPacket p;

	p.dwID = ch->GetDesc()->GetAccountTable().id;
	strlcpy(p.szOldPassword, arg1, sizeof(p.szOldPassword));
	strlcpy(p.szNewPassword, arg2, sizeof(p.szNewPassword));

	db_clientdesc->DBPacket(HEADER_GD_SAFEBOX_CHANGE_PASSWORD, ch->GetDesc()->GetHandle(), &p, sizeof(p));
}

ACMD(do_mall_password)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1 || strlen(arg1) > 6)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_STRING("[Storeroom] You have entered an incorrect password."));
		return;
	}

	int iPulse = thecore_pulse();

	if (ch->GetMall())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_STRING("[Storeroom] The Storeroom is already open."));
		return;
	}

	if (iPulse - ch->GetMallLoadTime() < passes_per_sec * 10)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_STRING("[Storeroom] You have to wait 10 seconds before you can open the Storeroom again."));
		return;
	}

	ch->SetMallLoadTime(iPulse);

	TSafeboxLoadPacket p;
	p.dwID = ch->GetDesc()->GetAccountTable().id;
	strlcpy(p.szLogin, ch->GetDesc()->GetAccountTable().login, sizeof(p.szLogin));
	strlcpy(p.szPassword, arg1, sizeof(p.szPassword));

	db_clientdesc->DBPacket(HEADER_GD_MALL_LOAD, ch->GetDesc()->GetHandle(), &p, sizeof(p));
}

ACMD(do_mall_close)
{
	if (ch->GetMall())
	{
		ch->SetMallLoadTime(thecore_pulse());
		ch->CloseMall();
		ch->Save();
	}
}

ACMD(do_ungroup)
{
	if (!ch->GetParty())
		return;

	if (!CPartyManager::instance().IsEnablePCParty())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_STRING("[Group] The server cannot execute this group request."));
		return;
	}

	if (ch->GetDungeon()
#if defined(__GUILD_DRAGONLAIR_PARTY_SYSTEM__)
		|| ch->GetGuildDragonLair()
#endif
#if defined(__DEFENSE_WAVE__)
		|| ch->GetDefenseWave()
#endif
		)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_STRING("[Group] You cannot leave a group while you are in a dungeon."));
		return;
	}

	LPPARTY pParty = ch->GetParty();

	if (pParty->GetMemberCount() == 2)
	{
		// party disband
		CPartyManager::instance().DeleteParty(pParty);
	}
	else
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_STRING("[Group] You have left the group."));
		//pParty->SendPartyRemoveOneToAll(ch);
		pParty->Quit(ch->GetPlayerID());
		//pParty->SendPartyRemoveAllToOne(ch);
	}
}

ACMD(do_close_shop)
{
	if (ch->GetMyShop())
	{
		ch->CloseMyShop();
		return;
	}
}

ACMD(do_set_walk_mode)
{
	ch->SetNowWalking(true);
	ch->SetWalking(true);
}

ACMD(do_set_run_mode)
{
	ch->SetNowWalking(false);
	ch->SetWalking(false);
}

ACMD(do_war)
{
	//    
	CGuild* g = ch->GetGuild();

	if (!g)
		return;

	//  ??!
	if (g->UnderAnyWar())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_STRING("[Guild] Your guild is already participating in another war."));
		return;
	}

	char arg1[256], arg2[256];
	int type = GUILD_WAR_TYPE_FIELD;
	two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

	if (!*arg1)
		return;

	if (*arg2)
	{
		str_to_number(type, arg2);

		if (type < 0 || type >= GUILD_WAR_TYPE_MAX_NUM)
			type = GUILD_WAR_TYPE_FIELD;
	}

	DWORD gm_pid = g->GetMasterPID();

	if (gm_pid != ch->GetPlayerID())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_STRING("[Guild] No one is entitled to a guild war."));
		return;
	}

	CGuild* opp_g = CGuildManager::instance().FindGuildByName(arg1);

	if (!opp_g)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_STRING("[Guild] No guild with this name exists."));
		return;
	}

	switch (g->GetGuildWarState(opp_g->GetID()))
	{
		case GUILD_WAR_NONE:
		{
			if (opp_g->UnderAnyWar())
			{
				ch->ChatPacket(CHAT_TYPE_INFO, LC_STRING("[Guild] This guild is already participating in a war."));
				return;
			}

			int iWarPrice = KOR_aGuildWarInfo[type].iWarPrice;

			if (g->GetGuildMoney() < iWarPrice)
			{
				ch->ChatPacket(CHAT_TYPE_INFO, LC_STRING("[Guild] Not enough Yang to participate in a guild war."));
				return;
			}

			if (opp_g->GetGuildMoney() < iWarPrice)
			{
				ch->ChatPacket(CHAT_TYPE_INFO, LC_STRING("[Guild] The guild does not have enough Yang to participate in a guild war."));
				return;
			}
		}
		break;

		case GUILD_WAR_SEND_DECLARE:
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_STRING("You have declared war on this guild."));
			return;
		}
		break;

		case GUILD_WAR_RECV_DECLARE:
		{
			if (opp_g->UnderAnyWar())
			{
				ch->ChatPacket(CHAT_TYPE_INFO, LC_STRING("[Guild] This guild is already participating in a war."));
				g->RequestRefuseWar(opp_g->GetID());
				return;
			}
		}
		break;

		case GUILD_WAR_RESERVE:
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_STRING("[Guild] This Guild is already scheduled for another war."));
			return;
		}
		break;

		case GUILD_WAR_END:
			return;

		default:
			ch->ChatPacket(CHAT_TYPE_INFO, LC_STRING("[Guild] This guild is already participating in another war."));
			g->RequestRefuseWar(opp_g->GetID());
			return;
	}

	if (!g->CanStartWar(type))
	{
		//    ?  ?.
		if (g->GetLadderPoint() == 0)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_STRING("[Guild] Guild level is too low to participate in a guild war."));
			sys_log(0, "GuildWar.StartError.NEED_LADDER_POINT");
		}
		else if (g->GetMemberCount() < GUILD_WAR_MIN_MEMBER_COUNT)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_STRING("[Guild] A minimum of %d players are needed to participate in a guild war.", GUILD_WAR_MIN_MEMBER_COUNT));
			sys_log(0, "GuildWar.StartError.NEED_MINIMUM_MEMBER[%d]", GUILD_WAR_MIN_MEMBER_COUNT);
		}
		else
		{
			sys_log(0, "GuildWar.StartError.UNKNOWN_ERROR");
		}
		return;
	}

	// ? ? ?  ?  ? ?.
	if (!opp_g->CanStartWar(GUILD_WAR_TYPE_FIELD))
	{
		if (opp_g->GetLadderPoint() == 0)
			ch->ChatPacket(CHAT_TYPE_INFO, LC_STRING("[Guild] The guild does not have enough points to participate in a guild war."));
		else if (opp_g->GetMemberCount() < GUILD_WAR_MIN_MEMBER_COUNT)
			ch->ChatPacket(CHAT_TYPE_INFO, LC_STRING("[Guild] The guild does not have enough members to participate in a guild war."));
		return;
	}

	do
	{
		if (g->GetMasterCharacter() != NULL)
			break;

		CCI* pCCI = P2P_MANAGER::instance().FindByPID(g->GetMasterPID());

		if (pCCI != NULL)
			break;

		ch->ChatPacket(CHAT_TYPE_INFO, LC_STRING("[Guild] The enemy's guild leader is offline."));
		g->RequestRefuseWar(opp_g->GetID());
		return;

	} while (false);

	do
	{
		if (opp_g->GetMasterCharacter() != NULL)
			break;

		CCI* pCCI = P2P_MANAGER::instance().FindByPID(opp_g->GetMasterPID());

		if (pCCI != NULL)
			break;

		ch->ChatPacket(CHAT_TYPE_INFO, LC_STRING("[Guild] The enemy's guild leader is offline."));
		g->RequestRefuseWar(opp_g->GetID());
		return;

	} while (false);

	g->RequestDeclareWar(opp_g->GetID(), type);
}

ACMD(do_nowar)
{
	CGuild* g = ch->GetGuild();
	if (!g)
		return;

	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
		return;

	DWORD gm_pid = g->GetMasterPID();

	if (gm_pid != ch->GetPlayerID())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_STRING("[Guild] No one is entitled to a guild war."));
		return;
	}

	CGuild* opp_g = CGuildManager::instance().FindGuildByName(arg1);

	if (!opp_g)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_STRING("[Guild] No guild with this name exists."));
		return;
	}

	g->RequestRefuseWar(opp_g->GetID());
}

ACMD(do_detaillog)
{
	ch->DetailLog();
}

ACMD(do_monsterlog)
{
	ch->ToggleMonsterLog();
}

ACMD(do_pkmode)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
		return;

	BYTE mode = 0;
	str_to_number(mode, arg1);

	if (mode == PK_MODE_PROTECT)
		return;

	if (ch->GetLevel() < PK_PROTECT_LEVEL && mode != 0)
		return;

	ch->SetPKMode(mode);
}

ACMD(do_messenger_auth)
{
	if (ch->GetArena())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_STRING("You cannot use this in the duel arena."));
		return;
	}

	char arg1[256], arg2[256];
	two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

	if (!*arg1 || !*arg2)
		return;

	char answer = LOWER(*arg1);

	if (answer != 'y')
	{
		LPCHARACTER tch = CHARACTER_MANAGER::instance().FindPC(arg2);

		if (tch)
			tch->ChatPacket(CHAT_TYPE_INFO, LC_STRING("%s declined the invitation.", ch->GetName()));
	}

	CMessengerManager::instance().AuthToAdd(ch->GetName(), arg2, answer == 'y' ? false : true); // DENY
}

ACMD(do_setblockmode)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (*arg1)
	{
		BYTE flag = 0;
		str_to_number(flag, arg1);
		ch->SetBlockMode(flag);
	}
}

ACMD(do_unmount)
{
#if defined(__MOUNT_COSTUME_SYSTEM__)
	if (const LPITEM pCostumeMount = ch->GetWear(WEAR_COSTUME_MOUNT))
		if (ch->UnequipItem(pCostumeMount) == false)
			return;
#endif
	ch->UnMount(true);
}

ACMD(do_observer_exit)
{
	if (ch->IsObserverMode())
	{
		if (ch->GetWarMap())
			ch->SetWarMap(NULL);

		if (ch->GetArena() != NULL || ch->GetArenaObserverMode() == true)
		{
			ch->SetArenaObserverMode(false);

			if (ch->GetArena() != NULL)
				ch->GetArena()->RemoveObserver(ch->GetPlayerID());

			ch->SetArena(NULL);
			ch->WarpSet(ARENA_RETURN_POINT_X(ch->GetEmpire()), ARENA_RETURN_POINT_Y(ch->GetEmpire()));
		}
		else
		{
			ch->ExitToSavedLocation();
		}
		ch->SetObserverMode(false);
	}
}

ACMD(do_party_request)
{
	if (ch->GetArena())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_STRING("You cannot use this in the duel arena."));
		return;
	}

	if (ch->GetParty())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_STRING("You cannot accept the invitation because you are already in the group."));
		return;
	}

	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
		return;

	DWORD vid = 0;
	str_to_number(vid, arg1);
	LPCHARACTER tch = CHARACTER_MANAGER::instance().Find(vid);

	if (tch)
		if (!ch->RequestToParty(tch))
			ch->ChatPacket(CHAT_TYPE_COMMAND, "PartyRequestDenied");
}

ACMD(do_party_request_accept)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
		return;

	DWORD vid = 0;
	str_to_number(vid, arg1);
	LPCHARACTER tch = CHARACTER_MANAGER::instance().Find(vid);

	if (tch)
		ch->AcceptToParty(tch);
}

ACMD(do_party_request_deny)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
		return;

	DWORD vid = 0;
	str_to_number(vid, arg1);
	LPCHARACTER tch = CHARACTER_MANAGER::instance().Find(vid);

	if (tch)
		ch->DenyToParty(tch);
}

ACMD(do_monarch_warpto)
{
	if (true == LC_IsYMIR() || true == LC_IsKorea())
		return;

	if (!CMonarch::instance().IsMonarch(ch->GetPlayerID(), ch->GetEmpire()))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_STRING("This function can only be used by the emperor."));
		return;
	}

	if (!ch->IsMCOK(CHARACTER::MI_WARP))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_STRING("Cooldown time for approximately %d seconds", ch->GetMCLTime(CHARACTER::MI_WARP)));
		return;
	}

	const int WarpPrice = 10000;

	if (!CMonarch::instance().IsMoneyOk(WarpPrice, ch->GetEmpire()))
	{
		int NationMoney = CMonarch::instance().GetMoney(ch->GetEmpire());
		ch->ChatPacket(CHAT_TYPE_INFO, LC_STRING("Lack of Taxes. Current Capital : %u Missing Capital : %u", NationMoney, WarpPrice));
		return;
	}

	int x = 0, y = 0;
	char arg1[256];

	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_STRING("Command: warpto <character name>"));
		return;
	}

	LPCHARACTER tch = CHARACTER_MANAGER::instance().FindPC(arg1);

	if (!tch)
	{
		CCI* pkCCI = P2P_MANAGER::instance().Find(arg1);

		if (pkCCI)
		{
			if (pkCCI->bEmpire != ch->GetEmpire())
			{
				ch->ChatPacket(CHAT_TYPE_INFO, LC_STRING("You cannot be warped to an unknown player."));
				return;
			}

			if (pkCCI->bChannel != g_bChannel)
			{
				ch->ChatPacket(CHAT_TYPE_INFO, LC_STRING("Adding player %d into the channel. (Present channel %d)", pkCCI->bChannel, g_bChannel));
				return;
			}
			if (!IsMonarchWarpZone(pkCCI->lMapIndex))
			{
				ch->ChatPacket(CHAT_TYPE_INFO, LC_STRING("You cannot move to that area."));
				return;
			}

			PIXEL_POSITION pos;

			if (!SECTREE_MANAGER::instance().GetCenterPositionOfMap(pkCCI->lMapIndex, pos))
				ch->ChatPacket(CHAT_TYPE_INFO, "Cannot find map (index %d)", pkCCI->lMapIndex);
			else
			{
				//ch->ChatPacket(CHAT_TYPE_INFO, "You warp to (%d, %d)", pos.x, pos.y);
				ch->ChatPacket(CHAT_TYPE_INFO, LC_STRING("Warp to player %s.", arg1));
				ch->WarpSet(pos.x, pos.y);

				//  ?
				CMonarch::instance().SendtoDBDecMoney(WarpPrice, ch->GetEmpire(), ch);

				ch->SetMC(CHARACTER::MI_WARP);
			}
		}
		else if (NULL == CHARACTER_MANAGER::instance().FindPC(arg1))
		{
			ch->ChatPacket(CHAT_TYPE_INFO, "There is no one by that name");
		}

		return;
	}
	else
	{
		if (tch->GetEmpire() != ch->GetEmpire())
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_STRING("You cannot be warped to an unknown player."));
			return;
		}
		if (!IsMonarchWarpZone(tch->GetMapIndex()))
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_STRING("You cannot move to that area."));
			return;
		}
		x = tch->GetX();
		y = tch->GetY();
	}

	ch->ChatPacket(CHAT_TYPE_INFO, LC_STRING("Warp to player %s.", arg1));
	ch->WarpSet(x, y);
	ch->Stop();

	CMonarch::instance().SendtoDBDecMoney(WarpPrice, ch->GetEmpire(), ch);

	ch->SetMC(CHARACTER::MI_WARP);
}

ACMD(do_monarch_transfer)
{
	if (true == LC_IsYMIR() || true == LC_IsKorea())
		return;

	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_STRING("Use: transfer <name>"));
		return;
	}

	if (!CMonarch::instance().IsMonarch(ch->GetPlayerID(), ch->GetEmpire()))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_STRING("This function can only be used by the emperor."));
		return;
	}

	if (!ch->IsMCOK(CHARACTER::MI_TRANSFER))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_STRING("Cooldown time for approximately %d seconds", ch->GetMCLTime(CHARACTER::MI_TRANSFER)));
		return;
	}

	const int WarpPrice = 10000;

	if (!CMonarch::instance().IsMoneyOk(WarpPrice, ch->GetEmpire()))
	{
		int NationMoney = CMonarch::instance().GetMoney(ch->GetEmpire());
		ch->ChatPacket(CHAT_TYPE_INFO, LC_STRING("Lack of Taxes. Current Capital : %u Missing Capital : %u", NationMoney, WarpPrice));
		return;
	}

	LPCHARACTER tch = CHARACTER_MANAGER::instance().FindPC(arg1);

	if (!tch)
	{
		CCI* pkCCI = P2P_MANAGER::instance().Find(arg1);

		if (pkCCI)
		{
			if (pkCCI->bEmpire != ch->GetEmpire())
			{
				ch->ChatPacket(CHAT_TYPE_INFO, LC_STRING("You cannot recruit players from another kingdom."));
				return;
			}
			if (pkCCI->bChannel != g_bChannel)
			{
				ch->ChatPacket(CHAT_TYPE_INFO, LC_STRING("The player %s is on channel %d at the moment. (Your channel: %d)", arg1, pkCCI->bChannel, g_bChannel));
				return;
			}
			if (!IsMonarchWarpZone(pkCCI->lMapIndex))
			{
				ch->ChatPacket(CHAT_TYPE_INFO, LC_STRING("You cannot move to that area."));
				return;
			}
			if (!IsMonarchWarpZone(ch->GetMapIndex()))
			{
				ch->ChatPacket(CHAT_TYPE_INFO, LC_STRING("You cannot move to that area."));
				return;
			}

			TPacketGGTransfer pgg;

			pgg.bHeader = HEADER_GG_TRANSFER;
			strlcpy(pgg.szName, arg1, sizeof(pgg.szName));
			pgg.lX = ch->GetX();
			pgg.lY = ch->GetY();

			P2P_MANAGER::instance().Send(&pgg, sizeof(TPacketGGTransfer));
			ch->ChatPacket(CHAT_TYPE_INFO, LC_STRING("You have recruited %s players.", arg1));

			CMonarch::instance().SendtoDBDecMoney(WarpPrice, ch->GetEmpire(), ch);
			ch->SetMC(CHARACTER::MI_TRANSFER);
		}
		else
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_STRING("There is no user with this name."));
		}

		return;
	}

	if (ch == tch)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_STRING("You cannot recruit yourself."));
		return;
	}

	if (tch->GetEmpire() != ch->GetEmpire())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_STRING("You cannot recruit players from another kingdom."));
		return;
	}
	if (!IsMonarchWarpZone(tch->GetMapIndex()))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_STRING("You cannot move to that area."));
		return;
	}
	if (!IsMonarchWarpZone(ch->GetMapIndex()))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_STRING("You cannot move to that area."));
		return;
	}

	//tch->Show(ch->GetMapIndex(), ch->GetX(), ch->GetY(), ch->GetZ());
	tch->WarpSet(ch->GetX(), ch->GetY(), ch->GetMapIndex());

	CMonarch::instance().SendtoDBDecMoney(WarpPrice, ch->GetEmpire(), ch);
	ch->SetMC(CHARACTER::MI_TRANSFER);
}

ACMD(do_monarch_info)
{
	if (CMonarch::instance().IsMonarch(ch->GetPlayerID(), ch->GetEmpire()))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_STRING("My information about the emperor"));
		TMonarchInfo* p = CMonarch::instance().GetMonarch();
		for (int n = 1; n < 4; ++n)
		{
			if (n == ch->GetEmpire())
				ch->ChatPacket(CHAT_TYPE_INFO, LC_STRING("[%s] : %s Gold %lld ", EMPIRE_NAME(n), p->name[n], p->money[n]));
			else
				ch->ChatPacket(CHAT_TYPE_INFO, LC_STRING("[%s] : %s  ", EMPIRE_NAME(n), p->name[n]));

		}
	}
	else
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_STRING("Information about the emperor"));
		TMonarchInfo* p = CMonarch::instance().GetMonarch();
		for (int n = 1; n < 4; ++n)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_STRING("[%s] : %s  ", EMPIRE_NAME(n), p->name[n]));

		}
	}

}

ACMD(do_elect)
{
	db_clientdesc->DBPacketHeader(HEADER_GD_COME_TO_VOTE, ch->GetDesc()->GetHandle(), 0);
}

// LUA_ADD_GOTO_INFO
struct GotoInfo
{
	std::string st_name;

	BYTE empire;
	int mapIndex;
	DWORD x, y;

	GotoInfo()
	{
		st_name = "";
		empire = 0;
		mapIndex = 0;

		x = 0;
		y = 0;
	}

	GotoInfo(const GotoInfo& c_src)
	{
		__copy__(c_src);
	}

	void operator = (const GotoInfo& c_src)
	{
		__copy__(c_src);
	}

	void __copy__(const GotoInfo& c_src)
	{
		st_name = c_src.st_name;
		empire = c_src.empire;
		mapIndex = c_src.mapIndex;

		x = c_src.x;
		y = c_src.y;
	}
};

extern void BroadcastNotice(const char* c_pszBuf, const bool c_bBigFont);

ACMD(do_monarch_tax)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "Usage: monarch_tax <1-50>");
		return;
	}

	if (!ch->IsMonarch())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_STRING("Only an emperor can use this."));
		return;
	}

	int tax = 0;
	str_to_number(tax, arg1);

	if (tax < 1 || tax > 50)
		ch->ChatPacket(CHAT_TYPE_INFO, LC_STRING("Choose a number between 1 and 50."));

	quest::CQuestManager::instance().SetEventFlag("trade_tax", tax);

	ch->ChatPacket(CHAT_TYPE_INFO, LC_STRING("Taxes are set to %d %%."));

	char szMsg[1024];

	snprintf(szMsg, sizeof(szMsg), "do_monarch_tax_text1", tax);
	BroadcastNotice(szMsg);

	snprintf(szMsg, sizeof(szMsg), "do_monarch_tax_text2", tax);
	BroadcastNotice(szMsg);

	ch->SetMC(CHARACTER::MI_TAX);
}

static const DWORD cs_dwMonarchMobVnums[] =
{
	191,
	192,
	193,
	194,
	391,
	392,
	393,
	394,
	491,
	492,
	493,
	494,
	591,
	691,
	791,
	1304,
	1901,
	2091,
	2191,
	2206,
	0,
};

ACMD(do_monarch_mob)
{
	char arg1[256];
	LPCHARACTER tch;

	one_argument(argument, arg1, sizeof(arg1));

	if (!ch->IsMonarch())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_STRING("Only an emperor can use this."));
		return;
	}

	if (!*arg1)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "Usage: mmob <mob name>");
		return;
	}

	BYTE pcEmpire = ch->GetEmpire();
	BYTE mapEmpire = SECTREE_MANAGER::instance().GetEmpireFromMapIndex(ch->GetMapIndex());

	if (LC_IsYMIR() == true || LC_IsKorea() == true)
	{
		if (mapEmpire != pcEmpire && mapEmpire != 0)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_STRING("You can use this skill only in your land."));
			return;
		}
	}

	const int SummonPrice = 5000000;

	if (!ch->IsMCOK(CHARACTER::MI_SUMMON))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_STRING("Cooldown time for approximately %d seconds", ch->GetMCLTime(CHARACTER::MI_SUMMON)));
		return;
	}

	//   ? 
	if (!CMonarch::instance().IsMoneyOk(SummonPrice, ch->GetEmpire()))
	{
		int NationMoney = CMonarch::instance().GetMoney(ch->GetEmpire());
		ch->ChatPacket(CHAT_TYPE_INFO, LC_STRING("Lack of Taxes. Current Capital : %u Missing Capital : %u", NationMoney, SummonPrice));
		return;
	}

	const CMob* pkMob = nullptr;
	DWORD vnum = 0;

	if (isdigit(*arg1))
	{
		str_to_number(vnum, arg1);

		if ((pkMob = CMobManager::instance().Get(vnum)) == NULL)
			vnum = 0;
	}
	else
	{
		pkMob = CMobManager::Instance().Get(arg1, true);

		if (pkMob)
			vnum = pkMob->m_table.dwVnum;
	}

	if (pkMob == nullptr)
		return;

	DWORD count;

	for (count = 0; cs_dwMonarchMobVnums[count] != 0; ++count)
		if (cs_dwMonarchMobVnums[count] == vnum)
			break;

	if (0 == cs_dwMonarchMobVnums[count])
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_STRING("The Monster cannot be called. Check the Mob Number."));
		return;
	}

	tch = CHARACTER_MANAGER::instance().SpawnMobRange(vnum,
		ch->GetMapIndex(),
		ch->GetX() - number(200, 750),
		ch->GetY() - number(200, 750),
		ch->GetX() + number(200, 750),
		ch->GetY() + number(200, 750),
		true,
		pkMob->m_table.bType == CHAR_TYPE_STONE,
		true);

	if (tch)
	{
		CMonarch::instance().SendtoDBDecMoney(SummonPrice, ch->GetEmpire(), ch);
		ch->SetMC(CHARACTER::MI_SUMMON);
	}
}

static const char* FN_point_string(DWORD dwApplyType)
{
	switch (dwApplyType)
	{
		case POINT_MAX_HP: return LC_STRING("?  +%d");
		case POINT_MAX_SP: return LC_STRING("? ? +%d");
		case POINT_HT: return LC_STRING(" +%d");
		case POINT_IQ: return LC_STRING(" +%d");
		case POINT_ST: return LC_STRING("? +%d");
		case POINT_DX: return LC_STRING(" +%d");
		case POINT_ATT_SPEED: return LC_STRING("?? +%d");
		case POINT_MOV_SPEED: return LC_STRING("?? %d");
		case POINT_CASTING_SPEED: return LC_STRING(" -%d");
		case POINT_HP_REGEN: return LC_STRING(" ? +%d");
		case POINT_SP_REGEN: return LC_STRING("? ? +%d");
		case POINT_POISON_PCT: return LC_STRING(" %d");
		case POINT_BLEEDING_PCT: return LC_STRING(" %d");
		case POINT_STUN_PCT: return LC_STRING(" +%d");
		case POINT_SLOW_PCT: return LC_STRING("? +%d");
		case POINT_CRITICAL_PCT: return LC_STRING("%d%% ? ? ");
		case POINT_RESIST_CRITICAL: return LC_STRING(" ? ? %d%% ");
		case POINT_PENETRATE_PCT: return LC_STRING("%d%% ?  ");
		case POINT_RESIST_PENETRATE: return LC_STRING("   ? %d%% ");
		case POINT_ATTBONUS_HUMAN: return LC_STRING("?  ? +%d%%");
		case POINT_ATTBONUS_ANIMAL: return LC_STRING("  ? +%d%%");
		case POINT_ATTBONUS_ORC: return LC_STRING(" ? +%d%%");
		case POINT_ATTBONUS_MILGYO: return LC_STRING("? ? +%d%%");
		case POINT_ATTBONUS_UNDEAD: return LC_STRING(" ? +%d%%");
		case POINT_ATTBONUS_DEVIL: return LC_STRING("? ? +%d%%");
		case POINT_STEAL_HP: return LC_STRING("? %d%%   ");
		case POINT_STEAL_SP: return LC_STRING("? %d%%  ? ");
		case POINT_MANA_BURN_PCT: return LC_STRING("%d%% ? ?  ? ?");
		case POINT_DAMAGE_SP_RECOVER: return LC_STRING("%d%% ? ? ? ?");
		case POINT_BLOCK: return LC_STRING("?  ? %d%%");
		case POINT_DODGE: return LC_STRING("?  ? ? %d%%");
		case POINT_RESIST_SWORD: return LC_STRING("??  %d%%");
		case POINT_RESIST_TWOHAND: return LC_STRING("?  %d%%");
		case POINT_RESIST_DAGGER: return LC_STRING("??  %d%%");
		case POINT_RESIST_BELL: return LC_STRING("  %d%%");
		case POINT_RESIST_FAN: return LC_STRING("  %d%%");
		case POINT_RESIST_BOW: return LC_STRING("?  %d%%");
		case POINT_RESIST_CLAW: return LC_STRING("??  %d%%");
		case POINT_RESIST_FIRE: return LC_STRING("?  %d%%");
		case POINT_RESIST_ELEC: return LC_STRING("  %d%%");
		case POINT_RESIST_MAGIC: return LC_STRING("  %d%%");
#if defined(__MAGIC_REDUCTION__)
		case POINT_RESIST_MAGIC_REDUCTION:	return LC_STRING("  %d%%");
#endif
		case POINT_RESIST_WIND: return LC_STRING("?  %d%%");
		case POINT_RESIST_ICE: return LC_STRING("  %d%%");
		case POINT_RESIST_EARTH: return LC_STRING("  %d%%");
		case POINT_RESIST_DARK: return LC_STRING("  %d%%");
		case POINT_REFLECT_MELEE: return LC_STRING(" ? ? ? : %d%%");
		case POINT_REFLECT_CURSE: return LC_STRING(" ? ? %d%%");
		case POINT_POISON_REDUCE: return LC_STRING("  %d%%");
		case POINT_BLEEDING_REDUCE:	return LC_STRING("  %d%%");
		case POINT_KILL_SP_RECOVER: return LC_STRING("%d%% ? ? ? ?");
		case POINT_EXP_DOUBLE_BONUS: return LC_STRING("%d%% ? ? ? ? ");
		case POINT_GOLD_DOUBLE_BONUS: return LC_STRING("%d%% ? ?  2 ");
		case POINT_ITEM_DROP_BONUS: return LC_STRING("%d%% ? ?  2 ");
		case POINT_POTION_BONUS: return LC_STRING("  %d%%  ");
		case POINT_KILL_HP_RECOVERY: return LC_STRING("%d%% ? ?  ?");
			//case POINT_IMMUNE_STUN: return LC_STRING("  %d%%");
			//case POINT_IMMUNE_SLOW: return LC_STRING("  %d%%");
			//case POINT_IMMUNE_FALL: return LC_STRING("?  %d%%");
			//case POINT_SKILL: return LC_STRING("");
			//case POINT_BOW_DISTANCE: return LC_STRING("");
		case POINT_ATT_GRADE_BONUS: return LC_STRING("? +%d");
		case POINT_DEF_GRADE_BONUS: return LC_STRING(" +%d");
		case POINT_MAGIC_ATT_GRADE: return LC_STRING(" ? +%d");
		case POINT_MAGIC_DEF_GRADE: return LC_STRING("  +%d");
			//case POINT_CURSE_PCT: return LC_STRING("");
		case POINT_MAX_STAMINA: return LC_STRING("?  +%d");
		case POINT_ATTBONUS_WARRIOR: return LC_STRING("?  +%d%%");
		case POINT_ATTBONUS_ASSASSIN: return LC_STRING("?  +%d%%");
		case POINT_ATTBONUS_SURA: return LC_STRING("??  +%d%%");
		case POINT_ATTBONUS_SHAMAN: return LC_STRING("?  +%d%%");
		case POINT_ATTBONUS_WOLFMAN: return LC_STRING("?  +%d%%");
		case POINT_ATTBONUS_MONSTER: return LC_STRING("?  +%d%%");
		case POINT_MALL_ATTBONUS: return LC_STRING("? +%d%%");
		case POINT_MALL_DEFBONUS: return LC_STRING(" +%d%%");
		case POINT_MALL_EXPBONUS: return LC_STRING("? %d%%");
		case POINT_MALL_ITEMBONUS: return LC_STRING("  %.1f");
		case POINT_MALL_GOLDBONUS: return LC_STRING("  %.1f");
		case POINT_MAX_HP_PCT: return LC_STRING("?  +%d%%");
		case POINT_MAX_SP_PCT: return LC_STRING("? ? +%d%%");
		case POINT_SKILL_DAMAGE_BONUS: return LC_STRING("?  %d%%");
		case POINT_NORMAL_HIT_DAMAGE_BONUS: return LC_STRING("  %d%%");
		case POINT_SKILL_DEFEND_BONUS: return LC_STRING("?   %d%%");
		case POINT_NORMAL_HIT_DEFEND_BONUS: return LC_STRING("   %d%%");
			//case POINT_PC_BANG_EXP_BONUS: return LC_STRING("");
			//case POINT_PC_BANG_DROP_BONUS: return LC_STRING("");
			//case POINT_EXTRACT_HP_PCT: return LC_STRING("");
		case POINT_RESIST_WARRIOR: return LC_STRING("? %d%% ");
		case POINT_RESIST_ASSASSIN: return LC_STRING("?? %d%% ");
		case POINT_RESIST_SURA: return LC_STRING("? %d%% ");
		case POINT_RESIST_SHAMAN: return LC_STRING("? %d%% ");
		case POINT_RESIST_WOLFMAN: return LC_STRING("? %d%% ");
#if defined(__ELEMENT_SYSTEM__)
		case POINT_ENCHANT_ELECT: return LC_STRING("Lightning Power + %d%%");
		case POINT_ENCHANT_FIRE: return LC_STRING("Fire Power + %d%%");
		case POINT_ENCHANT_ICE: return LC_STRING("Ice Power + %d%%");
		case POINT_ENCHANT_WIND: return LC_STRING("Wind Power + %d%%");
		case POINT_ENCHANT_EARTH: return LC_STRING("Earth Power + %d%%");
		case POINT_ENCHANT_DARK: return LC_STRING("Dark Power + %d%%");
		case POINT_ATTBONUS_CZ: return LC_STRING("Strong against Zodiac Monsters + %d%%");
		case POINT_ATTBONUS_INSECT: return LC_STRING("Strong against Insects + %d%%");
		case POINT_ATTBONUS_DESERT: return LC_STRING("Strong against Desert Monsters + %d%%");
#endif
		case POINT_ATTBONUS_STONE: return LC_STRING("Strong against Metin Stones +%d%%");
		default:
			return LC_STRING("Unkown apply_number %d", dwApplyType);
	}
}

static bool FN_hair_affect_string(LPCHARACTER ch, char* buf, size_t bufsiz)
{
	if (NULL == ch || NULL == buf)
		return false;

	CAffect* aff = NULL;
	time_t expire = 0;
	struct tm ltm;
	int year, mon, day;
	int offset = 0;

	aff = ch->FindAffect(AFFECT_HAIR);

	if (NULL == aff)
		return false;

	expire = ch->GetQuestFlag("hair.limit_time");

	if (expire < get_global_time())
		return false;

	// set apply string
	offset = snprintf(buf, bufsiz, FN_point_string(aff->wApplyOn), aff->lApplyValue);

	if (offset < 0 || offset >= (int)bufsiz)
		offset = bufsiz - 1;

	localtime_r(&expire, &ltm);

	year = ltm.tm_year + 1900;
	mon = ltm.tm_mon + 1;
	day = ltm.tm_mday;

	snprintf(buf + offset, bufsiz - offset, LC_STRING("(Procedure: %d y- %d m - %d d)", year, mon, day));

	return true;
}

ACMD(do_costume)
{
	char buf[1024];
	const size_t bufferSize = sizeof(buf);

	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	ch->ChatPacket(CHAT_TYPE_INFO, "COSTUME status:");

	CItem* pBody = ch->GetWear(WEAR_COSTUME_BODY);
	if (pBody)
	{
		const char* itemName = pBody->GetName();

		ch->ChatPacket(CHAT_TYPE_INFO, "   BODY : %s", itemName);

		if (pBody->IsEquipped() && arg1[0] == 'b')
			ch->UnequipItem(pBody);
	}

	CItem* pHair = ch->GetWear(WEAR_COSTUME_HAIR);
	if (pHair)
	{
		const char* itemName = pHair->GetName();

		ch->ChatPacket(CHAT_TYPE_INFO, "   HAIR : %s", itemName);

		for (int i = 0; i < pHair->GetAttributeCount(); ++i)
		{
			const TPlayerItemAttribute& attr = pHair->GetAttribute(i);
			if (0 < attr.wType)
			{
				snprintf(buf, bufferSize, FN_point_string(attr.wType), attr.lValue);
				ch->ChatPacket(CHAT_TYPE_INFO, "     %s", buf);
			}
		}

		if (pHair->IsEquipped() && arg1[0] == 'h')
			ch->UnequipItem(pHair);
	}

#if defined(__MOUNT_COSTUME_SYSTEM__)
	CItem* pMount = ch->GetWear(WEAR_COSTUME_MOUNT);
	if (pMount)
	{
		const char* itemName = pMount->GetName();

		ch->ChatPacket(CHAT_TYPE_INFO, "   MOUNT : %s", itemName);

		if (pMount->IsEquipped() && arg1[0] == 'm')
			ch->UnequipItem(pMount);
	}
#endif

#if defined(__ACCE_COSTUME_SYSTEM__)
	CItem* pAcce = ch->GetWear(WEAR_COSTUME_ACCE);
	if (pAcce)
	{
		const char* itemName = pAcce->GetName();

		ch->ChatPacket(CHAT_TYPE_INFO, "   ACCE : %s", itemName);

		if (pAcce->IsEquipped() && arg1[0] == 'a')
			ch->UnequipItem(pAcce);
	}
#endif

#if defined(__WEAPON_COSTUME_SYSTEM__)
	CItem* pWeapon = ch->GetWear(WEAR_COSTUME_WEAPON);
	if (pWeapon)
	{
		const char* itemName = pWeapon->GetName();
		ch->ChatPacket(CHAT_TYPE_INFO, "   WEAPON : %s", itemName);
		if (pWeapon->IsEquipped() && arg1[0] == 'w')
			ch->UnequipItem(pWeapon);
	}
#endif

#if defined(__AURA_COSTUME_SYSTEM__)
	CItem* pAura = ch->GetWear(WEAR_COSTUME_AURA);
	if (pAura)
	{
		const char* itemName = pAura->GetName();
		ch->ChatPacket(CHAT_TYPE_INFO, "  AURA : %s", itemName);
		if (pAura->IsEquipped() && arg1[0] == 'a')
			ch->UnequipItem(pAura);
	}
#endif
}

ACMD(do_hair)
{
	char buf[256];

	if (false == FN_hair_affect_string(ch, buf, sizeof(buf)))
		return;

	ch->ChatPacket(CHAT_TYPE_INFO, buf);
}

ACMD(do_inventory)
{
	int index = 0;
	int count = 1;

	char arg1[256];
	char arg2[256];

	LPITEM item;

	two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

	if (!*arg1)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "Usage: inventory <start_index> <count>");
		return;
	}

	if (!*arg2)
	{
		index = 0;
		str_to_number(count, arg1);
	}
	else
	{
		str_to_number(index, arg1); index = MIN(index, INVENTORY_MAX_NUM);
		str_to_number(count, arg2); count = MIN(count, INVENTORY_MAX_NUM);
	}

	for (int i = 0; i < count; ++i)
	{
#if defined(__EXTEND_INVEN_SYSTEM__)
		if (index >= ch->GetExtendInvenMax())
#else
		if (index >= INVENTORY_MAX_NUM)
#endif
			break;

		item = ch->GetInventoryItem(index);

		ch->ChatPacket(CHAT_TYPE_INFO, "inventory [%d] = %s", index, item ? item->GetName() : "<NONE>");
		++index;
	}
}

// gift notify quest command
ACMD(do_gift)
{
	ch->ChatPacket(CHAT_TYPE_COMMAND, "gift");
}

#if !defined(__CUBE_RENEWAL__)
ACMD(do_cube)
{
	if (!ch->CanDoCube())
		return;

	dev_log(LOG_DEB0, "CUBE COMMAND <%s>: %s", ch->GetName(), argument);
	int cube_index = 0, inven_index = 0;
	const char* line;

	char arg1[256], arg2[256], arg3[256];

	line = two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));
	one_argument(line, arg3, sizeof(arg3));

	if (0 == arg1[0])
	{
		// print usage
		ch->ChatPacket(CHAT_TYPE_INFO, "Usage: cube open");
		ch->ChatPacket(CHAT_TYPE_INFO, "       cube close");
		ch->ChatPacket(CHAT_TYPE_INFO, "       cube add <inveltory_index>");
		ch->ChatPacket(CHAT_TYPE_INFO, "       cube delete <cube_index>");
		ch->ChatPacket(CHAT_TYPE_INFO, "       cube list");
		ch->ChatPacket(CHAT_TYPE_INFO, "       cube cancel");
		ch->ChatPacket(CHAT_TYPE_INFO, "       cube make [all]");
		return;
	}

	const std::string& strArg1 = std::string(arg1);

	// r_info (request information)
	// /cube r_info ==> (Client -> Server)  NPC   ?  
	//					(Server -> Client) /cube r_list npcVNUM resultCOUNT 123,1/125,1/128,1/130,5
	//
	// /cube r_info 3 ==> (Client -> Server)  NPC  ?   3    ?  
	// /cube r_info 3 5 ==> (Client -> Server)  NPC  ?   3 ?  5    ?   
	//					(Server -> Client) /cube m_info startIndex count 125,1|126,2|127,2|123,5&555,5&555,4/120000@125,1|126,2|127,2|123,5&555,5&555,4/120000
	//
	if (strArg1 == "r_info")
	{
		if (0 == arg2[0])
			Cube_request_result_list(ch);
		else
		{
			if (isdigit(*arg2))
			{
				int listIndex = 0, requestCount = 1;
				str_to_number(listIndex, arg2);

				if (0 != arg3[0] && isdigit(*arg3))
					str_to_number(requestCount, arg3);

				Cube_request_material_info(ch, listIndex, requestCount);
			}
		}

		return;
	}

	switch (LOWER(arg1[0]))
	{
		case 'o': // open
			Cube_open(ch);
			break;

		case 'c': // close
			Cube_close(ch);
			break;

		case 'l': // list
			Cube_show_list(ch);
			break;

		case 'a': // add cue_index inven_index
		{
			if (0 == arg2[0] || !isdigit(*arg2) ||
				0 == arg3[0] || !isdigit(*arg3))
				return;

			str_to_number(cube_index, arg2);
			str_to_number(inven_index, arg3);
			Cube_add_item(ch, cube_index, inven_index);
		}
		break;

		case 'd': // delete
		{
			if (0 == arg2[0] || !isdigit(*arg2))
				return;

			str_to_number(cube_index, arg2);
			Cube_delete_item(ch, cube_index);
		}
		break;

		case 'm': // make
			if (0 != arg2[0])
			{
				while (true == Cube_make(ch))
					dev_log(LOG_DEB0, "cube make success");
			}
			else
				Cube_make(ch);
			break;

		default:
			return;
	}
}
#endif

ACMD(do_in_game_mall)
{
	if (LC_IsYMIR() == true || LC_IsKorea() == true)
	{
		ch->ChatPacket(CHAT_TYPE_COMMAND, "mall http://metin2.co.kr/04_mall/mall/login.htm");
		return;
	}

	if (true == LC_IsTaiwan())
	{
		ch->ChatPacket(CHAT_TYPE_COMMAND, "mall http://203.69.141.203/mall/mall/item_main.htm");
		return;
	}

	if (LC_IsJapan() == true)
	{
		ch->ChatPacket(CHAT_TYPE_COMMAND, "mall http://mt2.oge.jp/itemmall/itemList.php");
		return;
	}

	if (LC_IsNewCIBN() == true && test_server)
	{
		ch->ChatPacket(CHAT_TYPE_COMMAND, "mall http://218.99.6.51/04_mall/mall/login.htm");
		return;
	}

	if (LC_IsSingapore() == true)
	{
		ch->ChatPacket(CHAT_TYPE_COMMAND, "mall http://www.metin2.sg/ishop.php");
		return;
	}

	/*
	if (LC_IsCanada() == true)
	{
		ch->ChatPacket(CHAT_TYPE_COMMAND, "mall http://mall.z8games.com/mall_entry.aspx?tb=m2");
		return;
	}
	*/

	if (LC_IsEurope() == true)
	{
		char country_code[3];

		switch (LC_GetLocalType())
		{
			case LC_GERMANY: country_code[0] = 'd'; country_code[1] = 'e'; country_code[2] = '\0'; break;
			case LC_FRANCE: country_code[0] = 'f'; country_code[1] = 'r'; country_code[2] = '\0'; break;
			case LC_ITALY: country_code[0] = 'i'; country_code[1] = 't'; country_code[2] = '\0'; break;
			case LC_SPAIN: country_code[0] = 'e'; country_code[1] = 's'; country_code[2] = '\0'; break;
			case LC_UK: country_code[0] = 'e'; country_code[1] = 'n'; country_code[2] = '\0'; break;
			case LC_TURKEY: country_code[0] = 't'; country_code[1] = 'r'; country_code[2] = '\0'; break;
			case LC_POLAND: country_code[0] = 'p'; country_code[1] = 'l'; country_code[2] = '\0'; break;
			case LC_PORTUGAL: country_code[0] = 'p'; country_code[1] = 't'; country_code[2] = '\0'; break;
			case LC_GREEK: country_code[0] = 'g'; country_code[1] = 'r'; country_code[2] = '\0'; break;
			case LC_RUSSIA: country_code[0] = 'r'; country_code[1] = 'u'; country_code[2] = '\0'; break;
			case LC_DENMARK: country_code[0] = 'd'; country_code[1] = 'k'; country_code[2] = '\0'; break;
			case LC_BULGARIA: country_code[0] = 'b'; country_code[1] = 'g'; country_code[2] = '\0'; break;
			case LC_CROATIA: country_code[0] = 'h'; country_code[1] = 'r'; country_code[2] = '\0'; break;
			case LC_MEXICO: country_code[0] = 'm'; country_code[1] = 'x'; country_code[2] = '\0'; break;
			case LC_ARABIA: country_code[0] = 'a'; country_code[1] = 'e'; country_code[2] = '\0'; break;
			case LC_CZECH: country_code[0] = 'c'; country_code[1] = 'z'; country_code[2] = '\0'; break;
			case LC_ROMANIA: country_code[0] = 'r'; country_code[1] = 'o'; country_code[2] = '\0'; break;
			case LC_HUNGARY: country_code[0] = 'h'; country_code[1] = 'u'; country_code[2] = '\0'; break;
			case LC_NETHERLANDS: country_code[0] = 'n'; country_code[1] = 'l'; country_code[2] = '\0'; break;
			case LC_USA: country_code[0] = 'u'; country_code[1] = 's'; country_code[2] = '\0'; break;
			case LC_CANADA: country_code[0] = 'c'; country_code[1] = 'a'; country_code[2] = '\0'; break;
			default:
			{
				if (test_server)
				{
					country_code[0] = 'u'; country_code[1] = 's'; country_code[2] = '\0';
				}
				break;
			}
		}

#if defined(__LOCALE_CLIENT__)
		snprintf(country_code, sizeof(country_code),
			"%s", LocaleService_GetCountry(ch->GetCountry()));
#endif

		char buf[512 + 1];
		char sas[33];
		MD5_CTX ctx;
		const char sas_key[] = "GF9001";

		snprintf(buf, sizeof(buf), "%u%u%s", ch->GetPlayerID(), ch->GetAID(), sas_key);

		MD5Init(&ctx);
		MD5Update(&ctx, (const unsigned char*)buf, strlen(buf));
#ifdef __FreeBSD__
		MD5End(&ctx, sas);
#else
		static const char hex[] = "0123456789abcdef";
		unsigned char digest[16];
		MD5Final(digest, &ctx);
		int i;
		for (i = 0; i < 16; ++i)
		{
			sas[i + i] = hex[digest[i] >> 4];
			sas[i + i + 1] = hex[digest[i] & 0x0f];
		}
		sas[i + i] = '\0';
#endif

		snprintf(buf, sizeof(buf), "mall http://%s/ishop?pid=%u&c=%s&sid=%d&sas=%s",
			g_strWebMallURL.c_str(), ch->GetPlayerID(), country_code, g_server_id, sas);

		ch->ChatPacket(CHAT_TYPE_COMMAND, buf);
	}
}

// ?
ACMD(do_dice)
{
	char arg1[256], arg2[256];
	int start = 1, end = 100;

	two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

	if (*arg1 && *arg2)
	{
		start = atoi(arg1);
		end = atoi(arg2);
	}
	else if (*arg1 && !*arg2)
	{
		start = 1;
		end = atoi(arg1);
	}

	end = MAX(start, end);
	start = MIN(start, end);

	int n = number(start, end);

#if defined(__DICE_SYSTEM__)
	if (ch->GetParty())
		ch->GetParty()->ChatPacketToAllMember(CHAT_TYPE_INFO, LC_STRING("%s ?  %d ??. (%d-%d)", ch->GetName(), n, start, end));
	else
		ch->ChatPacket(CHAT_TYPE_INFO, LC_STRING(" ?  %d ??. (%d-%d)", n, start, end));
#else
	if (ch->GetParty())
		ch->GetParty()->ChatPacketToAllMember(CHAT_TYPE_INFO, LC_STRING("%s ?  %d ??. (%d-%d)", ch->GetName(), n, start, end));
	else
		ch->ChatPacket(CHAT_TYPE_INFO, LC_STRING(" ?  %d ??. (%d-%d)", n, start, end));
#endif
}

ACMD(do_click_mall)
{
	ch->ChatPacket(CHAT_TYPE_COMMAND, "ShowMeMallPassword");
}

ACMD(do_ride)
{
	dev_log(LOG_DEB0, "[DO_RIDE] start");
	if (ch->IsDead() || ch->IsStun())
		return;

	if (ch->IsFishing())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_STRING("You cannot carry out this action while fishing."));
		return;
	}

#if defined(__MOUNT_COSTUME_SYSTEM__)
	if (SECTREE_MANAGER::Instance().IsBlockFilterMapIndex(SECTREE_MANAGER::MOUNT_BLOCK_MAP_INDEX, ch->GetMapIndex()))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_STRING("You cannot summon your mount/pet right now."));
		return;
	}
#endif

	if (ch->IsWearingDress())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_STRING("You cannot ride while you are wearing a Wedding Dress or a Tuxedo."));
		return;
	}

	{
		if (ch->IsHorseRiding())
		{
			dev_log(LOG_DEB0, "[DO_RIDE] stop riding");
			ch->StopRiding();
			return;
		}

		if (ch->GetMountVnum())
		{
			dev_log(LOG_DEB0, "[DO_RIDE] unmount");
			do_unmount(ch, NULL, 0, 0);
			return;
		}
	}

	{
		if (ch->GetHorse() != NULL)
		{
			dev_log(LOG_DEB0, "[DO_RIDE] start riding");
			ch->StartRiding();
			return;
		}

		for (BYTE i = 0; i < INVENTORY_MAX_NUM; ++i)
		{
			LPITEM item = ch->GetInventoryItem(i);
			if (NULL == item)
				continue;

			if (item->IsRideItem())
			{
				if (NULL == ch->GetWear(WEAR_UNIQUE1)
					|| NULL == ch->GetWear(WEAR_UNIQUE2)
#if defined(__MOUNT_COSTUME_SYSTEM__)
					|| NULL == ch->GetWear(WEAR_COSTUME_MOUNT)
#endif
					)
				{
					dev_log(LOG_DEB0, "[DO_RIDE] USE UNIQUE ITEM");
					//ch->EquipItem(item);
					ch->UseItem(TItemPos(INVENTORY, i));
					return;
				}
			}

			switch (item->GetVnum())
			{
				case 71114:
				case 71116:
				case 71118:
				case 71120:
					dev_log(LOG_DEB0, "[DO_RIDE] USE QUEST ITEM");
					ch->UseItem(TItemPos(INVENTORY, i));
					return;
			}

			// GF mantis #113524, 52001~52090
			if ((item->GetVnum() > 52000) && (item->GetVnum() < 52091))
			{
				dev_log(LOG_DEB0, "[DO_RIDE] USE QUEST ITEM");
				ch->UseItem(TItemPos(INVENTORY, i));
				return;
			}
		}
	}

	ch->ChatPacket(CHAT_TYPE_INFO, LC_STRING("Please call your Horse first."));
}

#if defined(__MOVE_CHANNEL__)
ACMD(do_move_channel)
{
	if (!ch)
		return;

	if (ch->m_pkTimedEvent)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_STRING("Your logout has been cancelled."));
		event_cancel(&ch->m_pkTimedEvent);
		return;
	}

	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_STRING("Usage: channel <new channel>"));
		return;
	}

	short channel;
	str_to_number(channel, arg1);

	if (channel < 1 || channel > 4)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_STRING("Please enter a valid channel."));
		return;
	}

	if (channel == g_bChannel)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_STRING("You are already on channel %d.", g_bChannel));
		return;
	}

	if (g_bChannel == 99)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_STRING("You cannot change your channel."));
		return;
	}

	if (ch->GetDungeon() || ch->GetMapIndex() >= 10000)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_STRING("You cannot change your channel."));
		return;
	}

	TPacketChangeChannel p;
	p.iChannel = channel;
	p.lMapIndex = ch->GetMapIndex();

	db_clientdesc->DBPacket(HEADER_GD_FIND_CHANNEL, ch->GetDesc()->GetHandle(), &p, sizeof(p));
}
#endif

#if defined(__POPUP_NOTICE__)
ACMD(do_popup_notice_check)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
		return;

	int enable;
	str_to_number(enable, arg1);

	ch->SetQuestFlag("popup_notice.checkbox", enable);
}
#endif

#if defined(__GAME_OPTION_ESCAPE__)
ACMD(do_escape)
{
	if (ch->IsDead() || ch->IsPolymorphed())
		return;

	if (ch->GetEscapeCooltime() > thecore_pulse() && !ch->IsGM())
		return;

	const BYTE bEmpire = ch->GetEmpire();
	const long lMapIndex = ch->GetMapIndex();
	const PIXEL_POSITION& rkPos = ch->GetXYZ();

	const LPSECTREE_MAP pSectreeMap = SECTREE_MANAGER::instance().GetMap(lMapIndex);
	if (pSectreeMap == nullptr)
	{
		ch->WarpSet(g_start_position[bEmpire][0], g_start_position[bEmpire][1]);
		return;
	}

	const LPSECTREE pSectree = pSectreeMap->Find(rkPos.x, rkPos.y);
	const DWORD dwAttr = pSectree->GetAttribute(rkPos.x, rkPos.y);

	int iEscapeDistance = quest::CQuestManager::instance().GetEventFlag("escape_distance");
	iEscapeDistance = (iEscapeDistance > 0) ? iEscapeDistance : 300;

	int iEscapeCooltime = quest::CQuestManager::instance().GetEventFlag("escape_cooltime");
	iEscapeCooltime = (iEscapeCooltime > 0) ? iEscapeCooltime : 10;

	if (IS_SET(dwAttr, ATTR_BLOCK /*| ATTR_OBJECT*/))
	{
		/*
		* NOTE : If an object doesn't have a blocked area, players can still be blocked if they get inside it.
		* The problem is that bridges are treated as objects too, and we don't want players to use the escape feature through them.
		* The tricky part is figuring out whether a specific object is a bridge or not.
		* In the current state, we are only checking blocked areas.
		*/

		PIXEL_POSITION kNewPos;
		if (SECTREE_MANAGER::instance().GetRandomLocation(lMapIndex, kNewPos, rkPos.x, rkPos.y, iEscapeDistance))
		{
			char szBuf[255 + 1];
			snprintf(szBuf, sizeof(szBuf), "%ld, (%d, %d) -> (%d, %d)",
				lMapIndex, rkPos.x, rkPos.y, kNewPos.x, kNewPos.y);
			LogManager::instance().CharLog(ch, lMapIndex, "ESCAPE", szBuf);

			ch->ChatPacket(CHAT_TYPE_INFO, LC_STRING("You have successfully freed yourself."));
			ch->Show(lMapIndex, kNewPos.x, kNewPos.y, rkPos.z);
		}
		else
		{
			char szBuf[255 + 1];
			snprintf(szBuf, sizeof(szBuf), "%ld, (%d, %d) -> EMPIRE START POSITION",
				lMapIndex, rkPos.x, rkPos.y);
			LogManager::instance().CharLog(ch, lMapIndex, "ESCAPE", szBuf);

			ch->WarpSet(g_start_position[bEmpire][0], g_start_position[bEmpire][1]);
		}
	}

	ch->SetEscapeCooltime(thecore_pulse() + PASSES_PER_SEC(10));
}
#endif

#if defined(__HIDE_COSTUME_SYSTEM__)
ACMD(do_hide_costume_part)
{
	char szArg1[256], szArg2[256];
	two_arguments(argument, szArg1, sizeof(szArg1), szArg2, sizeof(szArg2));

	if (!*szArg1 || !*szArg2)
		return;

	BYTE bCostumeSubType = 0, bHidden = 0;
	str_to_number(bCostumeSubType, szArg1);
	str_to_number(bHidden, szArg2);

	ch->SetHiddenCostumePart(bCostumeSubType, bHidden);
}
#endif

#if defined(ENABLE_PASSIVE_SYSTEM)
namespace
{
	enum EPassiveRelicSockets
	{
		PASSIVE_RELIC_SOCKET_REMAIN = ITEM_SOCKET_REMAIN_SEC,
		PASSIVE_RELIC_SOCKET_ACTIVE = 1,
		PASSIVE_RELIC_SOCKET_PROC = 2,
	};

	enum EPassiveRelicMaterialIndex
	{
		PASSIVE_RELIC_MATERIAL_WEAPON,
		PASSIVE_RELIC_MATERIAL_ELEMENT,
		PASSIVE_RELIC_MATERIAL_ARMOR,
		PASSIVE_RELIC_MATERIAL_ACCE,
		PASSIVE_RELIC_MATERIAL_MAX,
	};

	struct TPassiveRelicMaterialInfo
	{
		BYTE bSubType;
		DWORD dwVnum;
	};

	struct TPassiveRelicBonusInfo
	{
		BYTE bBonusType;
		WORD wApply;
		BYTE bValueCount;
		short sValues[10];
	};

	enum EPassiveRelicBonusTypes
	{
		PASSIVE_RELIC_BONUS_DIRECT,
	};

	const DWORD PASSIVE_RELIC_VNUM_MIN = 30272;
	const DWORD PASSIVE_RELIC_VNUM_MAX = 30276;
	const char* PASSIVE_RELIC_COOLDOWN_FLAG = "passive_relic.cooldown";
	const char* PASSIVE_RELIC_DECK_SELECTED_FLAG = "passive_relic.deck.selected";
	const char* PASSIVE_RELIC_DECK_INIT_FLAG = "passive_relic.deck.init";
	const char* PASSIVE_RELIC_DECK_ATTR_TYPE_FLAG_FMT = "passive_relic.deck%d.attr.type%d";
	const char* PASSIVE_RELIC_DECK_ATTR_VALUE_FLAG_FMT = "passive_relic.deck%d.attr.value%d";
	const char* PASSIVE_RELIC_DECK_PROC_STONE_FLAG_FMT = "passive_relic.deck%d.proc.stone";
	const char* PASSIVE_RELIC_DECK_PROC_DISMOUNT_FLAG_FMT = "passive_relic.deck%d.proc.dismount";
	const BYTE PASSIVE_RELIC_MAX_BONUS_COUNT = 4;
	const int PASSIVE_RELIC_DIRECT_UNEQUIP_SUCCESS_CHANCE = 20;
	const int PASSIVE_RELIC_FALLBACK_DURATION = 60 * 60 * 24 * 7;

	enum EPassiveRelicDeck
	{
		PASSIVE_RELIC_DECK_EARTH = 0,
		PASSIVE_RELIC_DECK_SKY = 1,
		PASSIVE_RELIC_DECK_MAX = 2,
	};

	const TPassiveRelicMaterialInfo kPassiveRelicMaterials[PASSIVE_RELIC_MATERIAL_MAX] =
	{
		{ MATERIAL_PASSIVE_WEAPON, 30255 },
		{ MATERIAL_PASSIVE_ELEMENT, 30258 },
		{ MATERIAL_PASSIVE_ARMOR, 30256 },
		{ MATERIAL_PASSIVE_ACCE, 30257 },
	};

	// https://tr-wiki.metin2.gameforge.com/index.php/Kalynty_Sistemi
	// Bonus de?erleri resmi oyuna gre dzenledim de?i?tirmek isterseniz a?a?ydaki de?erleri de?i?tirin
	/*
	SungMa STR/RES/VIT/INT iin sabit 15
	Metin savunma iin 1,2,3,4,5,6,8,10
	Metin ta?yna kar?y g iin 1,2,3,4,5,6,7,8,10
	Patron drop ?ansy iin 1,2,3,4,5,6,7,8,10
	Patronlara kar?y gc iin 1,2,3,4,5,6,7,8,10
	*/

	const TPassiveRelicBonusInfo kPassiveRelicBonusPool[] =
	{
		{ PASSIVE_RELIC_BONUS_DIRECT, APPLY_SUNGMA_STR, 1, { 15 } },
		{ PASSIVE_RELIC_BONUS_DIRECT, APPLY_SUNGMA_HP, 1, { 15 } },
		{ PASSIVE_RELIC_BONUS_DIRECT, APPLY_SUNGMA_MOVE, 1, { 15 } },
		{ PASSIVE_RELIC_BONUS_DIRECT, APPLY_SUNGMA_IMMUNE, 1, { 15 } },
		{ PASSIVE_RELIC_BONUS_DIRECT, APPLY_HIT_STONE_ATTBONUS_STONE, 9, { 1, 2, 3, 4, 5, 6, 7, 8, 10 } },
		{ PASSIVE_RELIC_BONUS_DIRECT, APPLY_KILL_BOSS_ITEM_BONUS, 9, { 1, 2, 3, 4, 5, 6, 7, 8, 10 } },
		{ PASSIVE_RELIC_BONUS_DIRECT, APPLY_ITEM_DROP_BONUS, 9, { 1, 2, 3, 4, 5, 6, 7, 8, 10 } },
	};

	LPITEM GetEquippedPassiveRelic(LPCHARACTER ch)
	{
		if (!ch)
			return NULL;

		return ch->GetWear(WEAR_PASSIVE);
	}

	bool IsPassiveRelicActive(LPITEM pkItem)
	{
		return pkItem && pkItem->GetSocket(PASSIVE_RELIC_SOCKET_ACTIVE) != 0;
	}
	int GetPassiveRelicDurationSeconds(LPITEM pkItem)
	{
		if (!pkItem)
			return 0;

		const int iDuration = pkItem->GetDuration();
		if (iDuration > 0)
			return iDuration;

		const int iRemain = pkItem->GetSocket(PASSIVE_RELIC_SOCKET_REMAIN);
		if (iRemain > 0)
			return iRemain;

		return PASSIVE_RELIC_FALLBACK_DURATION;
	}

	long GetPassiveRelicProcSocket(LPITEM pkItem)
	{
		return pkItem ? pkItem->GetSocket(PASSIVE_RELIC_SOCKET_PROC) : 0;
	}

	short GetPassiveRelicStoneDefProcValue(LPITEM pkItem)
	{
		return static_cast<short>(GetPassiveRelicProcSocket(pkItem) & 0xFFFF);
	}

	short GetPassiveRelicDismountMoveProcValue(LPITEM pkItem)
	{
		return static_cast<short>((GetPassiveRelicProcSocket(pkItem) >> 16) & 0xFFFF);
	}

	void SetPassiveRelicStoneDefProcValue(LPITEM pkItem, short sValue)
	{
		if (!pkItem)
			return;

		const long lCurrent = GetPassiveRelicProcSocket(pkItem);
		const long lUpdated = (lCurrent & 0xFFFF0000L) | (static_cast<long>(sValue) & 0xFFFFL);
		pkItem->SetSocket(PASSIVE_RELIC_SOCKET_PROC, lUpdated);
	}

	void SetPassiveRelicDismountMoveProcValue(LPITEM pkItem, short sValue)
	{
		if (!pkItem)
			return;

		const long lCurrent = GetPassiveRelicProcSocket(pkItem);
		const long lUpdated = (lCurrent & 0x0000FFFFL) | ((static_cast<long>(sValue) & 0xFFFFL) << 16);
		pkItem->SetSocket(PASSIVE_RELIC_SOCKET_PROC, lUpdated);
	}

	int GetPassiveRelicBonusCount(LPITEM pkItem)
	{
		if (!pkItem)
			return 0;

		return pkItem->GetAttributeCount();
	}

	void RefreshPassiveRelicState(LPCHARACTER ch, LPITEM pkItem);

	int ClampPassiveRelicDeckIndex(int iDeck)
	{
		if (iDeck < PASSIVE_RELIC_DECK_EARTH || iDeck >= PASSIVE_RELIC_DECK_MAX)
			return PASSIVE_RELIC_DECK_EARTH;
		return iDeck;
	}

	std::string BuildPassiveRelicDeckAttrTypeFlag(int iDeck, int iAttrIndex)
	{
		char szFlag[64];
		snprintf(szFlag, sizeof(szFlag), PASSIVE_RELIC_DECK_ATTR_TYPE_FLAG_FMT, iDeck, iAttrIndex);
		return szFlag;
	}

	std::string BuildPassiveRelicDeckAttrValueFlag(int iDeck, int iAttrIndex)
	{
		char szFlag[64];
		snprintf(szFlag, sizeof(szFlag), PASSIVE_RELIC_DECK_ATTR_VALUE_FLAG_FMT, iDeck, iAttrIndex);
		return szFlag;
	}

	std::string BuildPassiveRelicDeckProcStoneFlag(int iDeck)
	{
		char szFlag[64];
		snprintf(szFlag, sizeof(szFlag), PASSIVE_RELIC_DECK_PROC_STONE_FLAG_FMT, iDeck);
		return szFlag;
	}

	std::string BuildPassiveRelicDeckProcDismountFlag(int iDeck)
	{
		char szFlag[64];
		snprintf(szFlag, sizeof(szFlag), PASSIVE_RELIC_DECK_PROC_DISMOUNT_FLAG_FMT, iDeck);
		return szFlag;
	}

	int GetPassiveRelicSelectedDeck(LPCHARACTER ch)
	{
		if (!ch)
			return PASSIVE_RELIC_DECK_EARTH;

		return ClampPassiveRelicDeckIndex(ch->GetQuestFlag(PASSIVE_RELIC_DECK_SELECTED_FLAG));
	}

	void SetPassiveRelicSelectedDeck(LPCHARACTER ch, int iDeck)
	{
		if (!ch)
			return;

		ch->SetQuestFlag(PASSIVE_RELIC_DECK_SELECTED_FLAG, ClampPassiveRelicDeckIndex(iDeck));
	}

	void SavePassiveRelicDeckState(LPCHARACTER ch, LPITEM pkRelic, int iDeck)
	{
		if (!ch || !pkRelic)
			return;

		iDeck = ClampPassiveRelicDeckIndex(iDeck);

		for (int i = 0; i < ITEM_ATTRIBUTE_MAX_NUM; ++i)
		{
			const TPlayerItemAttribute& rAttr = pkRelic->GetAttribute(i);
			ch->SetQuestFlag(BuildPassiveRelicDeckAttrTypeFlag(iDeck, i), rAttr.wType);
			ch->SetQuestFlag(BuildPassiveRelicDeckAttrValueFlag(iDeck, i), rAttr.lValue);
		}

		ch->SetQuestFlag(BuildPassiveRelicDeckProcStoneFlag(iDeck), GetPassiveRelicStoneDefProcValue(pkRelic));
		ch->SetQuestFlag(BuildPassiveRelicDeckProcDismountFlag(iDeck), GetPassiveRelicDismountMoveProcValue(pkRelic));
	}

	void ClearPassiveRelicBonuses(LPITEM pkRelic)
	{
		if (!pkRelic)
			return;

		while (pkRelic->GetAttributeCount() > 0)
		{
			if (!pkRelic->RemoveAttributeAt(pkRelic->GetAttributeCount() - 1))
				break;
		}

		SetPassiveRelicStoneDefProcValue(pkRelic, 0);
		SetPassiveRelicDismountMoveProcValue(pkRelic, 0);
	}

	void LoadPassiveRelicDeckState(LPCHARACTER ch, LPITEM pkRelic, int iDeck)
	{
		if (!ch || !pkRelic)
			return;

		iDeck = ClampPassiveRelicDeckIndex(iDeck);
		ClearPassiveRelicBonuses(pkRelic);

		for (int i = 0; i < ITEM_ATTRIBUTE_MAX_NUM; ++i)
		{
			const int iType = ch->GetQuestFlag(BuildPassiveRelicDeckAttrTypeFlag(iDeck, i));
			const int iValue = ch->GetQuestFlag(BuildPassiveRelicDeckAttrValueFlag(iDeck, i));
			if (iType <= 0 || iValue == 0)
				continue;

			pkRelic->AddAttribute(static_cast<WORD>(iType), static_cast<short>(iValue));
		}

		SetPassiveRelicStoneDefProcValue(pkRelic, static_cast<short>(ch->GetQuestFlag(BuildPassiveRelicDeckProcStoneFlag(iDeck))));
		SetPassiveRelicDismountMoveProcValue(pkRelic, static_cast<short>(ch->GetQuestFlag(BuildPassiveRelicDeckProcDismountFlag(iDeck))));
	}

	void ResetPassiveRelicDeckState(LPCHARACTER ch, int iDeck)
	{
		if (!ch)
			return;

		iDeck = ClampPassiveRelicDeckIndex(iDeck);
		for (int i = 0; i < ITEM_ATTRIBUTE_MAX_NUM; ++i)
		{
			ch->SetQuestFlag(BuildPassiveRelicDeckAttrTypeFlag(iDeck, i), 0);
			ch->SetQuestFlag(BuildPassiveRelicDeckAttrValueFlag(iDeck, i), 0);
		}

		ch->SetQuestFlag(BuildPassiveRelicDeckProcStoneFlag(iDeck), 0);
		ch->SetQuestFlag(BuildPassiveRelicDeckProcDismountFlag(iDeck), 0);
	}

	void InitializePassiveRelicDeckState(LPCHARACTER ch, LPITEM pkRelic)
	{
		if (!ch || !pkRelic)
			return;

		if (ch->GetQuestFlag(PASSIVE_RELIC_DECK_INIT_FLAG) != 0)
			return;

		SavePassiveRelicDeckState(ch, pkRelic, PASSIVE_RELIC_DECK_EARTH);
		ResetPassiveRelicDeckState(ch, PASSIVE_RELIC_DECK_SKY);
		SetPassiveRelicSelectedDeck(ch, PASSIVE_RELIC_DECK_EARTH);
		ch->SetQuestFlag(PASSIVE_RELIC_DECK_INIT_FLAG, 1);
	}

	void SwitchPassiveRelicDeck(LPCHARACTER ch, LPITEM pkRelic, int iNextDeck)
	{
		if (!ch || !pkRelic)
			return;

		InitializePassiveRelicDeckState(ch, pkRelic);

		const int iCurrentDeck = GetPassiveRelicSelectedDeck(ch);
		iNextDeck = ClampPassiveRelicDeckIndex(iNextDeck);

		if (iCurrentDeck == iNextDeck)
			return;

		const bool bWasActive = pkRelic->IsEquipped() && IsPassiveRelicActive(pkRelic);
		if (bWasActive)
			pkRelic->ModifyPoints(false);

		SavePassiveRelicDeckState(ch, pkRelic, iCurrentDeck);
		LoadPassiveRelicDeckState(ch, pkRelic, iNextDeck);
		SetPassiveRelicSelectedDeck(ch, iNextDeck);

		if (bWasActive)
		{
			ch->RemoveAffect(AFFECT_PASSIVE_RELIC_STONE_DEF);
			ch->RemoveAffect(AFFECT_PASSIVE_RELIC_DISMOUNT_SPEED);
			pkRelic->ModifyPoints(true);
		}

		RefreshPassiveRelicState(ch, pkRelic);
	}

	bool CanUsePassiveRelicCommand(LPCHARACTER ch)
	{
		if (!ch || !ch->IsPC())
			return false;

		if (ch->IsDead() || ch->IsStun())
			return false;

		if (ch->GetExchange() || ch->GetMyShop() || ch->GetShopOwner() || ch->IsOpenSafebox() || ch->IsCubeOpen())
		{
			ch->ChatPacket(CHAT_TYPE_INFO, "You cannot use the relic right now.");
			return false;
		}

		if (ch->GetQuestFlag(PASSIVE_RELIC_COOLDOWN_FLAG) > get_global_time())
		{
			ch->ChatPacket(CHAT_TYPE_INFO, "Please wait a moment and try again.");
			return false;
		}

		return true;
	}

	void TouchPassiveRelicCooldown(LPCHARACTER ch)
	{
		if (ch)
			ch->SetQuestFlag(PASSIVE_RELIC_COOLDOWN_FLAG, get_global_time() + 1);
	}

	void RefreshPassiveRelicState(LPCHARACTER ch, LPITEM pkItem)
	{
		if (!ch || !pkItem)
			return;

		if (pkItem->IsEquipped())
		{
			ch->ComputeBattlePoints();
			ch->UpdatePacket();
		}

		pkItem->UpdatePacket();
	}

	void SetPassiveRelicActive(LPCHARACTER ch, LPITEM pkItem, bool bActive)
	{
		if (!ch || !pkItem)
			return;

		const bool bWasActive = IsPassiveRelicActive(pkItem);
		if (bWasActive == bActive)
			return;

		if (pkItem->IsEquipped() && bWasActive)
			pkItem->ModifyPoints(false);

		pkItem->SetSocket(PASSIVE_RELIC_SOCKET_ACTIVE, bActive ? 1 : 0);

		if (pkItem->IsEquipped() && bActive)
		{
			pkItem->ModifyPoints(true);
			ch->EffectPacket(SE_PASSIVE_EFFECT);
			ch->SpecificEffectPacket("d:/ymir work/effect/etc/buff/buff_passive_01.mse");
		}
		else if (!bActive)
		{
			ch->RemoveAffect(AFFECT_PASSIVE_RELIC_STONE_DEF);
			ch->RemoveAffect(AFFECT_PASSIVE_RELIC_DISMOUNT_SPEED);
		}

		RefreshPassiveRelicState(ch, pkItem);
	}

	bool ParsePassiveRelicMaterialCells(const char* argument, int aiCells[PASSIVE_RELIC_MATERIAL_MAX])
	{
		if (!argument)
			return false;

		for (int i = 0; i < PASSIVE_RELIC_MATERIAL_MAX; ++i)
		{
			char arg[256];
			argument = one_argument(argument, arg, sizeof(arg));
			if (!*arg)
				return false;

			str_to_number(aiCells[i], arg);
		}

		return true;
	}

	LPITEM GetPassiveRelicMaterialItem(LPCHARACTER ch, int iCell, const TPassiveRelicMaterialInfo& rInfo)
	{
		if (!ch)
			return NULL;

		if (iCell < 0 || iCell >= INVENTORY_MAX_NUM)
			return NULL;

		LPITEM pkItem = ch->GetInventoryItem(iCell);
		if (!pkItem)
			return NULL;

		if (pkItem->GetType() != ITEM_MATERIAL)
			return NULL;

		if (pkItem->GetSubType() != rInfo.bSubType)
			return NULL;

		if (pkItem->GetVnum() != rInfo.dwVnum)
			return NULL;

		if (pkItem->GetCount() <= 0)
			return NULL;

		return pkItem;
	}

	bool CollectPassiveRelicMaterialItems(LPCHARACTER ch, const int aiCells[PASSIVE_RELIC_MATERIAL_MAX], LPITEM apItems[PASSIVE_RELIC_MATERIAL_MAX])
	{
		for (int i = 0; i < PASSIVE_RELIC_MATERIAL_MAX; ++i)
		{
			for (int j = i + 1; j < PASSIVE_RELIC_MATERIAL_MAX; ++j)
			{
				if (aiCells[i] == aiCells[j])
					return false;
			}

			apItems[i] = GetPassiveRelicMaterialItem(ch, aiCells[i], kPassiveRelicMaterials[i]);
			if (!apItems[i])
				return false;
		}

		return true;
	}

	void ConsumePassiveRelicMaterialItems(LPITEM apItems[PASSIVE_RELIC_MATERIAL_MAX])
	{
		for (int i = 0; i < PASSIVE_RELIC_MATERIAL_MAX; ++i)
		{
			if (!apItems[i])
				continue;

			if (apItems[i]->GetCount() > 1)
				apItems[i]->SetCount(apItems[i]->GetCount() - 1);
			else
				apItems[i]->SetCount(0);
		}
	}

	bool AddRandomPassiveRelicBonus(LPITEM pkRelic)
	{
		if (!pkRelic)
			return false;

		std::vector<int> vecAvailableIndexes;
		for (int i = 0; i < static_cast<int>(sizeof(kPassiveRelicBonusPool) / sizeof(kPassiveRelicBonusPool[0])); ++i)
		{
			const TPassiveRelicBonusInfo& rBonusInfo = kPassiveRelicBonusPool[i];
			switch (rBonusInfo.bBonusType)
			{
				case PASSIVE_RELIC_BONUS_DIRECT:
					if (!pkRelic->HasAttr(rBonusInfo.wApply))
						vecAvailableIndexes.push_back(i);
					break;
			}
		}

		if (vecAvailableIndexes.empty())
			return false;

		const int iSelectedIndex = vecAvailableIndexes[number(0, vecAvailableIndexes.size() - 1)];
		const TPassiveRelicBonusInfo& rBonusInfo = kPassiveRelicBonusPool[iSelectedIndex];
		const short sValue = rBonusInfo.sValues[number(0, rBonusInfo.bValueCount - 1)];
		switch (rBonusInfo.bBonusType)
		{
			case PASSIVE_RELIC_BONUS_DIRECT:
				pkRelic->AddAttribute(rBonusInfo.wApply, sValue);
				return true;
		}

		return false;
	}
}

ACMD(do_passive_relic)
{
	char arg1[256];
	argument = one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
		return;

	const bool bIsUnequipCommand = !str_cmp(arg1, "unequip");
	if (!bIsUnequipCommand && !CanUsePassiveRelicCommand(ch))
		return;

	LPITEM pkRelic = GetEquippedPassiveRelic(ch);
	if (!pkRelic)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "Equip your passive relic first.");
		return;
	}

	InitializePassiveRelicDeckState(ch, pkRelic);

	if (!str_cmp(arg1, "earth") || !str_cmp(arg1, "sky"))
	{
		const int iNextDeck = !str_cmp(arg1, "sky") ? PASSIVE_RELIC_DECK_SKY : PASSIVE_RELIC_DECK_EARTH;
		SwitchPassiveRelicDeck(ch, pkRelic, iNextDeck);
		TouchPassiveRelicCooldown(ch);
		ch->ChatPacket(CHAT_TYPE_INFO, iNextDeck == PASSIVE_RELIC_DECK_SKY ? "Sky relic deck selected." : "Earth relic deck selected.");
		return;
	}

	if (!str_cmp(arg1, "extract"))
	{
		char argCell[256];
		one_argument(argument, argCell, sizeof(argCell));
		if (!*argCell)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, "Place a valid pincer item on the relic.");
			return;
		}

		int iExtractCell = -1;
		str_to_number(iExtractCell, argCell);
		if (iExtractCell < 0 || iExtractCell >= INVENTORY_MAX_NUM)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, "Invalid extraction item slot.");
			return;
		}

		LPITEM pkExtractItem = ch->GetInventoryItem(iExtractCell);
		if (!pkExtractItem || (pkExtractItem->GetVnum() != 100100 && pkExtractItem->GetVnum() != 100101))
		{
			ch->ChatPacket(CHAT_TYPE_INFO, "You need Dragon Pincer or Dragon Pincer+.");
			return;
		}

		const int iSuccessChance = (pkExtractItem->GetVnum() == 100101) ? 100 : 30;
		const int iEmptyPos = ch->GetEmptyInventory(pkRelic->GetSize());
		if (iEmptyPos < 0)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, "You need an empty inventory slot to extract the relic.");
			return;
		}

		pkExtractItem->SetCount(pkExtractItem->GetCount() - 1);

		if (IsPassiveRelicActive(pkRelic))
			SetPassiveRelicActive(ch, pkRelic, false);

		if (number(1, 100) > iSuccessChance)
		{
			SavePassiveRelicDeckState(ch, pkRelic, GetPassiveRelicSelectedDeck(ch));
			pkRelic->RemoveFromCharacter();
			pkRelic->SetCount(0);
			ch->ChatPacket(CHAT_TYPE_INFO, "Relic extraction failed. The relic was destroyed.");
			TouchPassiveRelicCooldown(ch);
			return;
		}

		SavePassiveRelicDeckState(ch, pkRelic, GetPassiveRelicSelectedDeck(ch));
		pkRelic->RemoveFromCharacter();
		pkRelic->AddToCharacter(ch, TItemPos(INVENTORY, iEmptyPos));
		ch->ChatPacket(CHAT_TYPE_INFO, "Relic extraction succeeded.");
		TouchPassiveRelicCooldown(ch);
		return;
	}

	if (!str_cmp(arg1, "unequip"))
	{
		if (IsPassiveRelicActive(pkRelic))
			SetPassiveRelicActive(ch, pkRelic, false);

		if (number(1, 100) > PASSIVE_RELIC_DIRECT_UNEQUIP_SUCCESS_CHANCE)
		{
			SavePassiveRelicDeckState(ch, pkRelic, GetPassiveRelicSelectedDeck(ch));
			pkRelic->RemoveFromCharacter();
			pkRelic->SetCount(0);
			ch->ChatPacket(CHAT_TYPE_INFO, "Direct relic extraction failed. The relic was destroyed.");
			return;
		}

		int iEmptyPos = ch->GetEmptyInventory(pkRelic->GetSize());
		if (iEmptyPos < 0)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, "You need an empty inventory slot to unequip the relic.");
			return;
		}

		SavePassiveRelicDeckState(ch, pkRelic, GetPassiveRelicSelectedDeck(ch));
		pkRelic->RemoveFromCharacter();
		pkRelic->AddToCharacter(ch, TItemPos(INVENTORY, iEmptyPos));
		ch->ChatPacket(CHAT_TYPE_INFO, "The relic has been removed from the slot.");
		return;
	}

	if (!str_cmp(arg1, "charge"))
	{
		int aiCells[PASSIVE_RELIC_MATERIAL_MAX];
		LPITEM apItems[PASSIVE_RELIC_MATERIAL_MAX] = {};
		if (!ParsePassiveRelicMaterialCells(argument, aiCells) || !CollectPassiveRelicMaterialItems(ch, aiCells, apItems))
		{
			ch->ChatPacket(CHAT_TYPE_INFO, "Place the four spirit items into the relic slots first.");
			return;
		}

		const int iDuration = GetPassiveRelicDurationSeconds(pkRelic);
		if (iDuration <= 0)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, "This relic cannot be charged.");
			return;
		}

		if (pkRelic->GetSocket(PASSIVE_RELIC_SOCKET_REMAIN) >= iDuration)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, "The relic is already fully charged.");
			return;
		}

		TouchPassiveRelicCooldown(ch);

		if (pkRelic->IsEquipped() && -1 != pkRelic->GetProto()->cLimitTimerBasedOnWearIndex)
			pkRelic->StopTimerBasedOnWearExpireEvent();

		pkRelic->SetSocket(PASSIVE_RELIC_SOCKET_REMAIN, iDuration);

		if (pkRelic->IsEquipped() && -1 != pkRelic->GetProto()->cLimitTimerBasedOnWearIndex)
			pkRelic->StartTimerBasedOnWearExpireEvent();

		ConsumePassiveRelicMaterialItems(apItems);
		RefreshPassiveRelicState(ch, pkRelic);
		ch->ChatPacket(CHAT_TYPE_INFO, "The relic time has been restored.");
		return;
	}

	if (!str_cmp(arg1, "add"))
	{
		int aiCells[PASSIVE_RELIC_MATERIAL_MAX];
		LPITEM apItems[PASSIVE_RELIC_MATERIAL_MAX] = {};
		if (!ParsePassiveRelicMaterialCells(argument, aiCells) || !CollectPassiveRelicMaterialItems(ch, aiCells, apItems))
		{
			ch->ChatPacket(CHAT_TYPE_INFO, "Place the four spirit items into the relic slots first.");
			return;
		}

		if (GetPassiveRelicBonusCount(pkRelic) >= PASSIVE_RELIC_MAX_BONUS_COUNT)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, "The relic cannot receive more bonuses.");
			return;
		}

		const bool bWasActive = pkRelic->IsEquipped() && IsPassiveRelicActive(pkRelic);
		if (bWasActive)
			pkRelic->ModifyPoints(false);

		const bool bAdded = AddRandomPassiveRelicBonus(pkRelic);

		if (bWasActive)
			pkRelic->ModifyPoints(true);

		if (!bAdded)
		{
			RefreshPassiveRelicState(ch, pkRelic);
			ch->ChatPacket(CHAT_TYPE_INFO, "The relic already has every available bonus.");
			return;
		}

		TouchPassiveRelicCooldown(ch);
		ConsumePassiveRelicMaterialItems(apItems);
		RefreshPassiveRelicState(ch, pkRelic);
		ch->ChatPacket(CHAT_TYPE_INFO, "A random relic bonus has been added.");
		return;
	}

	if (!str_cmp(arg1, "activate"))
	{
		if (GetPassiveRelicBonusCount(pkRelic) <= 0)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, "Add a relic bonus first.");
			return;
		}

		const int iDuration = GetPassiveRelicDurationSeconds(pkRelic);
		if (iDuration > 0 && pkRelic->GetSocket(PASSIVE_RELIC_SOCKET_REMAIN) <= 0)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, "Charge the relic before activation.");
			return;
		}

		TouchPassiveRelicCooldown(ch);

		const bool bNextState = !IsPassiveRelicActive(pkRelic);
		SetPassiveRelicActive(ch, pkRelic, bNextState);
		ch->ChatPacket(CHAT_TYPE_INFO, bNextState ? "The relic is now active." : "The relic is now inactive.");
		return;
	}
}
#endif

#if defined(ENABLE_TITLE_SYSTEM)
namespace
{
	struct STitleSystemDef
	{
		int iTitleID;
		const char* c_szName;
		const char* c_szEffect;
	};

	const STitleSystemDef kTitleSystemDefs[] =
	{
		{ 1000, "Sansli", "d:/ymir work/effect/etc/title/title_05_medal.mse" },
		{ 1001, "Metin+", "d:/ymir work/effect/etc/title/title_06_banner_gold.mse" },
		{ 1002, "Destansi", "d:/ymir work/effect/etc/title/title_07_banner_red.mse" },
		{ 1003, "Efsanevi", "d:/ymir work/effect/etc/title/title_08_banner_blue.mse" },
		{ 1004, "Mistik", "d:/ymir work/effect/etc/title/title_02_dragon.mse" },
		{ 1005, "Gunessever", "d:/ymir work/effect/etc/title/title_01_shield.mse" },
		{ 2000, "Son kurtulan", "d:/ymir work/effect/etc/title/title_04_trophy.mse" },
		{ 2001, "Yenilmez", "d:/ymir work/effect/etc/title/title_03_fist.mse" },
		{ 2002, "Savas habercisi", "d:/ymir work/effect/etc/title/title_02_dragon.mse" },
		{ 2003, "Kasap", "d:/ymir work/effect/etc/title/title_01_shield.mse" },
		{ 3000, "Metin+", "d:/ymir work/effect/etc/title/title_06_banner_gold.mse" },
		{ 3001, "Destansi", "d:/ymir work/effect/etc/title/title_07_banner_red.mse" },
		{ 3002, "Efsanevi", "d:/ymir work/effect/etc/title/title_08_banner_blue.mse" },
		{ 3003, "Mistik", "d:/ymir work/effect/etc/title/title_02_dragon.mse" },
		{ 3004, "Sansli", "d:/ymir work/effect/etc/title/title_05_medal.mse" },
	};

	const char* TITLE_SYSTEM_CLEAR_EFFECT_TOKEN = "__TITLE_EFFECT_CLEAR__";
	const char* TITLE_SYSTEM_ACTIVE_FLAG = "title_system.active";
	const char* TITLE_SYSTEM_OWNED_FLAG_FMT = "title_system.owned.%d";
	const char* TITLE_SYSTEM_EXPIRE_FLAG_FMT = "title_system.expire.%d";

	void BuildTitleSystemOwnedFlag(int iTitleID, char* szFlag, size_t stSize)
	{
		snprintf(szFlag, stSize, TITLE_SYSTEM_OWNED_FLAG_FMT, iTitleID);
	}

	void BuildTitleSystemExpireFlag(int iTitleID, char* szFlag, size_t stSize)
	{
		snprintf(szFlag, stSize, TITLE_SYSTEM_EXPIRE_FLAG_FMT, iTitleID);
	}

	const STitleSystemDef* FindTitleSystemDef(int iTitleID)
	{
		for (size_t i = 0; i < sizeof(kTitleSystemDefs) / sizeof(kTitleSystemDefs[0]); ++i)
		{
			if (kTitleSystemDefs[i].iTitleID == iTitleID)
				return &kTitleSystemDefs[i];
		}

		return NULL;
	}

	int GetTitleSystemExpireTime(LPCHARACTER ch, int iTitleID)
	{
		if (!ch)
			return 0;

		char szFlag[64];
		BuildTitleSystemExpireFlag(iTitleID, szFlag, sizeof(szFlag));
		return ch->GetQuestFlag(szFlag);
	}

	bool IsTitleSystemOwned(LPCHARACTER ch, int iTitleID)
	{
		if (!ch)
			return false;

		char szFlag[64];
		BuildTitleSystemOwnedFlag(iTitleID, szFlag, sizeof(szFlag));
		return ch->GetQuestFlag(szFlag) > 0;
	}

	void ClearTitleSystemOwnership(LPCHARACTER ch, int iTitleID)
	{
		if (!ch)
			return;

		char szOwned[64];
		char szExpire[64];
		BuildTitleSystemOwnedFlag(iTitleID, szOwned, sizeof(szOwned));
		BuildTitleSystemExpireFlag(iTitleID, szExpire, sizeof(szExpire));
		ch->SetQuestFlag(szOwned, 0);
		ch->SetQuestFlag(szExpire, 0);

		if (ch->GetQuestFlag(TITLE_SYSTEM_ACTIVE_FLAG) == iTitleID)
			ch->SetQuestFlag(TITLE_SYSTEM_ACTIVE_FLAG, 0);
	}

	bool IsTitleSystemExpired(LPCHARACTER ch, int iTitleID)
	{
		const int iExpireAt = GetTitleSystemExpireTime(ch, iTitleID);
		if (iExpireAt <= 0)
			return false;

		if (iExpireAt > get_global_time())
			return false;

		ClearTitleSystemOwnership(ch, iTitleID);
		return true;
	}

	void SetTitleSystemActive(LPCHARACTER ch, int iTitleID)
	{
		if (!ch)
			return;

		ch->SetQuestFlag(TITLE_SYSTEM_ACTIVE_FLAG, iTitleID);
	}
}

ACMD(do_title)
{
	char arg1[256];
	argument = one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "Title commands: /title list, /title equip <id>, /title clear, /title status");
		return;
	}

	if (!str_cmp(arg1, "list"))
	{
		ch->ChatPacket(CHAT_TYPE_COMMAND, "TitleSyncReset");
		ch->ChatPacket(CHAT_TYPE_INFO, "Unlocked titles:");
		for (size_t i = 0; i < sizeof(kTitleSystemDefs) / sizeof(kTitleSystemDefs[0]); ++i)
		{
			const int iTitleID = kTitleSystemDefs[i].iTitleID;
			if (!IsTitleSystemOwned(ch, iTitleID))
				continue;

			if (IsTitleSystemExpired(ch, iTitleID))
				continue;

			const int iExpireAt = GetTitleSystemExpireTime(ch, iTitleID);
			ch->ChatPacket(CHAT_TYPE_COMMAND, "TitleSyncAdd %d", iTitleID);
			if (iExpireAt > 0)
				ch->ChatPacket(CHAT_TYPE_INFO, "- %d: %s (expires in %d sec)", iTitleID, kTitleSystemDefs[i].c_szName, iExpireAt - get_global_time());
			else
				ch->ChatPacket(CHAT_TYPE_INFO, "- %d: %s", iTitleID, kTitleSystemDefs[i].c_szName);
		}
		return;
	}

	if (!str_cmp(arg1, "status"))
	{
		const int iActive = ch->GetQuestFlag(TITLE_SYSTEM_ACTIVE_FLAG);
		ch->ChatPacket(CHAT_TYPE_COMMAND, "TitleSyncActive %d", iActive);
		if (iActive <= 0)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, "No active title.");
			return;
		}

		const STitleSystemDef* pkDef = FindTitleSystemDef(iActive);
		if (!pkDef)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, "Active title id: %d", iActive);
			return;
		}

		ch->ChatPacket(CHAT_TYPE_INFO, "Active title: %d (%s)", iActive, pkDef->c_szName);
		return;
	}


	if (!str_cmp(arg1, "sync"))
	{
		ch->ChatPacket(CHAT_TYPE_COMMAND, "TitleSyncReset");
		for (size_t i = 0; i < sizeof(kTitleSystemDefs) / sizeof(kTitleSystemDefs[0]); ++i)
		{
			const int iTitleID = kTitleSystemDefs[i].iTitleID;
			if (!IsTitleSystemOwned(ch, iTitleID))
				continue;

			if (IsTitleSystemExpired(ch, iTitleID))
				continue;

			ch->ChatPacket(CHAT_TYPE_COMMAND, "TitleSyncAdd %d", iTitleID);
		}
		ch->ChatPacket(CHAT_TYPE_COMMAND, "TitleSyncActive %d", ch->GetQuestFlag(TITLE_SYSTEM_ACTIVE_FLAG));
		return;
	}

	if (!str_cmp(arg1, "clear"))
	{
		SetTitleSystemActive(ch, 0);
		ch->SpecificEffectPacket(TITLE_SYSTEM_CLEAR_EFFECT_TOKEN);
		// no restart for title clear (client sync command handles UI state)
		ch->ChatPacket(CHAT_TYPE_COMMAND, "TitleSyncActive 0");
		ch->ChatPacket(CHAT_TYPE_INFO, "Title unequipped.");
		return;
	}

	if (!str_cmp(arg1, "equip"))
	{
		char arg2[256];
		one_argument(argument, arg2, sizeof(arg2));
		if (!*arg2)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, "Usage: /title equip <id>");
			return;
		}

		int iTitleID = 0;
		str_to_number(iTitleID, arg2);
		const STitleSystemDef* pkDef = FindTitleSystemDef(iTitleID);
		if (!pkDef)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, "Unknown title id.");
			return;
		}

		if (!IsTitleSystemOwned(ch, iTitleID) || IsTitleSystemExpired(ch, iTitleID))
		{
			ch->ChatPacket(CHAT_TYPE_INFO, "You do not own this title or it has expired.");
			return;
		}
		const int iActiveTitle = ch->GetQuestFlag(TITLE_SYSTEM_ACTIVE_FLAG);
		if (iActiveTitle == iTitleID)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, "This title is already active.");
			return;
		}
		if (iActiveTitle > 0)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, "Remove your current title first.");
			return;
		}
		ch->SpecificEffectPacket(TITLE_SYSTEM_CLEAR_EFFECT_TOKEN);
		SetTitleSystemActive(ch, iTitleID);
		ch->ChatPacket(CHAT_TYPE_COMMAND, "TitleSyncActive %d", iTitleID);
		if (pkDef->c_szEffect && *pkDef->c_szEffect) ch->SpecificEffectPacket(pkDef->c_szEffect);
		ch->ChatPacket(CHAT_TYPE_INFO, "%s was equipped.", pkDef->c_szName);
		return;
	}

	ch->ChatPacket(CHAT_TYPE_INFO, "Unknown title command.");
}
#endif

#if defined(ENABLE_AUTO_SYSTEM)
ACMD(do_autohunt)
{
	/*if (quest::CQuestManager::instance().GetEventFlag("DISABLE_AUTO_HUNT") == 1)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "AutoHunt Disabled");
		return;
	}*/

	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));
	switch (LOWER(arg1[0]))
	{
		case 'b':
		{
			if (ch->IsRiding() || ch->GetHorse())
			{
				ch->ChatPacket(CHAT_TYPE_INFO, "Mount unsummon!");
			}
			else
			{
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
			}
		}
		break;

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
		break;

		default:
			break;
	}
}
#endif

#if defined(ENABLE_AUTO_RESTART_EVENT)
ACMD(do_auto_restart)
{
	if (!ch)
	{
		return;
	}

	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
		return;

	uint8_t type;
	str_to_number(type, arg1);

	if (type)
	{
		ch->autohunt_restart = true;
	}
	else
	{
		ch->autohunt_restart = false;
	}
}
#endif
