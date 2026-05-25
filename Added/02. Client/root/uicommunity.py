import app
import ui
import net
import grp
import guild
import player
import messenger
import localeInfo
import uiToolTip
import uiCommon
import uiScriptLocale
import chat

from _weakref import proxy

import uimessenger

import community
import wndMgr
import re


_COMMUNITY_NAME_COORD_RE = re.compile(r"[-+]?\d*\.\d+\s+[-+]?\d*\.\d+")


def IsCommunityGuildRenewalEnabled():
	if not getattr(app, "ENABLE_MESSENGER_RENEWAL", 0):
		return False
	return getattr(app, "ENABLE_COMMUNITY_GUILD_RENEWAL", 0) != 0


def NameKeyToPid(name):
	if not name:
		return 0
	h = 2166136261
	for ch in name:
		h ^= ord(ch)
		h = (h * 16777619) & 0xFFFFFFFF
	return h if h else 1


def IsPlausibleMemberDisplayName(name):
	if not name or not isinstance(name, basestring):
		return False
	stripped = name.strip()
	if not stripped or len(stripped) > 24:
		return False
	if "[LC;" in stripped or "[DONE" in stripped or "|name;" in stripped:
		return False
	if stripped.startswith("idx;") or (stripped.startswith(";") and ";" in stripped):
		return False
	if stripped.endswith("]") and "[" not in stripped:
		return False
	if re.match(r"^\d+\]$", stripped):
		return False
	if _COMMUNITY_NAME_COORD_RE.search(stripped):
		return False
	for ch in stripped:
		code = ord(ch)
		if code < 32 or code == 127:
			return False
	return True


def NormalizeCommunityMemberDisplayName(name, key=None):
	if IsPlausibleMemberDisplayName(name):
		return name.strip()
	if key and isinstance(key, basestring) and IsPlausibleMemberDisplayName(key):
		return key.strip()
	return ""


def NormalizeCommunityMemberKey(groupIndex, key, name):
	if groupIndex == FRIEND:
		if name:
			return NameKeyToPid(name)
		if isinstance(key, basestring):
			return NameKeyToPid(key)
	if app.ENABLE_MESSENGER_BLOCK and groupIndex == BLOCK:
		if name:
			return NameKeyToPid(name)
		if isinstance(key, basestring):
			return NameKeyToPid(key)
	return key


def GetCommunityMapName(mapIndex):
	if not mapIndex:
		return ""
	if localeInfo.MINIMAP_ZONE_NAME_DICT_BY_IDX.has_key(mapIndex):
		return localeInfo.MINIMAP_ZONE_NAME_DICT_BY_IDX[mapIndex]
	return ""

FRIEND = uimessenger.FRIEND
GUILD = uimessenger.GUILD
if app.ENABLE_MESSENGER_GM:
	GM = uimessenger.GM
if app.ENABLE_MESSENGER_BLOCK:
	BLOCK = uimessenger.BLOCK

ROOT_PATH = "d:/ymir work/ui/game/windows/"
TASKBAR_PATH = "d:/ymir work/ui/game/taskbar/"
PUBLIC_PATH = "d:/ymir work/ui/public/"

COMMUNITY_FAMILY_INFO_BAR_BG = ROOT_PATH + "community_info_bar_family_default.sub"
COMMUNITY_MEMBER_INFO_BAR_BG = ROOT_PATH + "community_info_bar_default.sub"
COMMUNITY_MEMBER_INFO_BAR_HOVER = ROOT_PATH + "community_info_bar_mouse_over.sub"
COMMUNITY_MY_INFO_BG = ROOT_PATH + "community_info_bar_player.sub"
COMMUNITY_MAIN_TAB_FRIEND_IMG = ROOT_PATH + "community_lower_tab_friend.sub"
COMMUNITY_MAIN_TAB_GUILD_IMG = ROOT_PATH + "community_lower_tab_guild.sub"
COMMUNITY_MAIN_TAB_CONFIG_IMG = ROOT_PATH + "community_lower_tab_config.sub"
COMMUNITY_SUB_TAB_FRIEND_IMG = ROOT_PATH + "community_friend_member.sub"
COMMUNITY_SUB_TAB_BLOCK_IMG = ROOT_PATH + "community_friend_block.sub"
COMMUNITY_SUB_TAB_REQUEST_IMG = ROOT_PATH + "community_friend_request.sub"
COMMUNITY_GUILD_SUB_TAB_IMG = ROOT_PATH + "community_guild_member.sub"
COMMUNITY_STATE_ONLINE = ROOT_PATH + "community_state_green.sub"
COMMUNITY_STATE_OFFLINE = ROOT_PATH + "community_state_red.sub"
COMMUNITY_STATE_DROP_TOP = ROOT_PATH + "community_drop_list_top.sub"
COMMUNITY_STATE_DROP_MIDDLE = ROOT_PATH + "community_drop_list_middle.sub"
COMMUNITY_STATE_DROP_BOTTOM = ROOT_PATH + "community_drop_list_bottom.sub"
COMMUNITY_STATE_DROP_OVER = ROOT_PATH + "community_drop_mouse_over.sub"
COMMUNITY_STATE_DROP_W = 170
COMMUNITY_STATE_DROP_ITEM_H = 16
COMMUNITY_STATE_DROP_X = 8
COMMUNITY_STATE_DROP_Y = 28
COMMUNITY_STATE_ICON_X = 8
COMMUNITY_STATE_TEXT_X = 24
COMMUNITY_STATE_TEXT_Y = 2
COMMUNITY_ROW_ICON_POSITION = ROOT_PATH + "community_icon_position_default.sub"
COMMUNITY_ROW_ICON_POSITION_OVER = ROOT_PATH + "community_icon_position_mouse_over.sub"
COMMUNITY_ROW_ICON_PARTY_INVITE = ROOT_PATH + "community_icon_party_invite_default.sub"
COMMUNITY_ROW_ICON_PARTY_INVITE_OVER = ROOT_PATH + "community_icon_party_invite_mouse_over.sub"
COMMUNITY_ROW_ICON_PARTY_INVITE_CLICK = ROOT_PATH + "community_icon_party_invite_click.sub"
COMMUNITY_ROW_ICON_WISHLIST = ROOT_PATH + "community_icon_wishlist_default.sub"
COMMUNITY_ROW_ICON_WISHLIST_OVER = ROOT_PATH + "community_icon_wishlist_mouse_over.sub"
COMMUNITY_ROW_ICON_WISHLIST_CLICK = ROOT_PATH + "community_icon_wishlist_click.sub"
COMMUNITY_ROW_ICON_DELETE = ROOT_PATH + "community_icon_x_default.sub"
COMMUNITY_ROW_ICON_DELETE_OVER = ROOT_PATH + "community_icon_x_mouse_over.sub"
COMMUNITY_ROW_ICON_DELETE_CLICK = ROOT_PATH + "community_icon_x_mouse_over.sub"
COMMUNITY_ROW_ICON_REQUEST_ACCEPT = ROOT_PATH + "community_icon_request_accept_default.sub"
COMMUNITY_ROW_ICON_REQUEST_ACCEPT_OVER = ROOT_PATH + "community_icon_request_accept_mouse_over.sub"
COMMUNITY_ROW_ICON_REQUEST_ACCEPT_CLICK = ROOT_PATH + "community_icon_request_accept_click.sub"
COMMUNITY_ROW_ICON_FAMILY = ROOT_PATH + "community_icon_family_default.sub"
COMMUNITY_ROW_ICON_FAMILY_OVER = ROOT_PATH + "community_icon_family_mouse_over.sub"

# Official uiCommunity dump (26.0.6.0) layout refs:
# MemberInfoBar.WIDTH=258, HEIGHT=34, BUTTON_INTERVAL_X=30
# FamilyInfoBar.ADJUST_POS_X=13
# SubTabMemberViewWindow.WIDTH=278, HEIGHT=255, MEMBER_INFO_BAR_INTERVAL_Y=36
# MessengerWindow.__family_info_bar (dump): bar sub + buttons; no uiscript EditLine
COMMUNITY_MEMBER_INFO_BAR_W = 258
COMMUNITY_MEMBER_INFO_BAR_H = 34
COMMUNITY_MEMBER_INFO_BAR_DEFAULT_X = 3
COMMUNITY_FAMILY_INFO_BAR_ADJUST_X = 13
COMMUNITY_MEMBER_INFO_BUTTON_INTERVAL_X = 30

COMMUNITY_LIST_PANEL_X = 11
COMMUNITY_LIST_PANEL_Y = 109
COMMUNITY_LIST_PANEL_W = 266
COMMUNITY_LIST_PANEL_H = 255
COMMUNITY_LIST_FRAME_W = 265
# Friend row entries only (AwengeR etc.): extra Y inside the list panel.
COMMUNITY_LIST_ROW_Y_OFFSET = 3

COMMUNITY_LIST_ROW_WIDTH = COMMUNITY_MEMBER_INFO_BAR_W
COMMUNITY_LIST_ROW_HEIGHT = COMMUNITY_MEMBER_INFO_BAR_H
COMMUNITY_LIST_START_Y = COMMUNITY_LIST_PANEL_Y + COMMUNITY_LIST_ROW_Y_OFFSET
COMMUNITY_LIST_LINE_HEIGHT = 36
COMMUNITY_LIST_X = 11 + COMMUNITY_MEMBER_INFO_BAR_DEFAULT_X

COMMUNITY_MY_INFO_X = 11
COMMUNITY_MY_INFO_Y = 35
# uiscript/communitywindow.py messenger_upper_outline_window
COMMUNITY_UPPER_OUTLINE_X = 11
COMMUNITY_UPPER_OUTLINE_Y = 4
COMMUNITY_UPPER_OUTLINE_W = 280
COMMUNITY_UPPER_OUTLINE_H = 76
COMMUNITY_FAMILY_INFO_BAR_X = 15
COMMUNITY_FAMILY_INFO_BAR_Y = 42
COMMUNITY_FAMILY_INFO_BTN_X_BASE = COMMUNITY_MEMBER_INFO_BAR_W - 15
COMMUNITY_FAMILY_INFO_BTN_POS_X = COMMUNITY_FAMILY_INFO_BTN_X_BASE - (COMMUNITY_MEMBER_INFO_BUTTON_INTERVAL_X * 2)
COMMUNITY_FAMILY_INFO_BTN_PARTY_X = COMMUNITY_FAMILY_INFO_BTN_X_BASE - COMMUNITY_MEMBER_INFO_BUTTON_INTERVAL_X
COMMUNITY_FAMILY_INFO_BTN_LOVE_X = COMMUNITY_FAMILY_INFO_BTN_X_BASE
COMMUNITY_FAMILY_INFO_BTN_Y = 4
COMMUNITY_SCROLL_X = 278
COMMUNITY_SCROLL_Y = COMMUNITY_LIST_PANEL_Y
COMMUNITY_SCROLL_SIZE = 253
COMMUNITY_MAIN_TAB_IMAGE_Y = 396
COMMUNITY_MAIN_TAB_IMAGE_X = (0.5, 0.5, 0.5)
COMMUNITY_SUB_TAB_IMAGE_Y = 82
COMMUNITY_FRIEND_ACTION_Y = 82
COMMUNITY_FRIEND_ACTION_X_BASE = 160
COMMUNITY_FRIEND_ACTION_INTERVAL_X = 30
COMMUNITY_FRIEND_ACTION_BTN_W = 24
COMMUNITY_FRIEND_ACTION_BTN_H = 24
COMMUNITY_FRIEND_ACTION_WHISPER_INDEX = 2
COMMUNITY_GUILD_ACTION_WHISPER_INDEX = 2
COMMUNITY_ACTION_ICON_FRIEND_INVITE = ROOT_PATH + "community_icon_friend_invite_default.sub"
COMMUNITY_ACTION_ICON_FRIEND_INVITE_OVER = ROOT_PATH + "community_icon_friend_invite_mouse_over.sub"
COMMUNITY_ACTION_ICON_FRIEND_INVITE_CLICK = ROOT_PATH + "community_icon_friend_invite_click.sub"
COMMUNITY_ACTION_ICON_BLOCK = ROOT_PATH + "community_icon_block_default.sub"
COMMUNITY_ACTION_ICON_BLOCK_OVER = ROOT_PATH + "community_icon_block_mouse_over.sub"
COMMUNITY_ACTION_ICON_BLOCK_CLICK = ROOT_PATH + "community_icon_block_click.sub"
COMMUNITY_ACTION_ICON_WHISPER = ROOT_PATH + "community_icon_whisper_default.sub"
COMMUNITY_ACTION_ICON_WHISPER_OVER = ROOT_PATH + "community_icon_whisper_mouse_over.sub"
COMMUNITY_ACTION_ICON_WHISPER_CLICK = ROOT_PATH + "community_icon_whisper_click.sub"
COMMUNITY_ACTION_ICON_DELETE = ROOT_PATH + "community_icon_delete_default.sub"
COMMUNITY_ACTION_ICON_DELETE_OVER = ROOT_PATH + "community_icon_delete_mouse_over.sub"
COMMUNITY_ACTION_ICON_DELETE_CLICK = ROOT_PATH + "community_icon_delete_click.sub"
COMMUNITY_SUB_TAB_RADIO_Y = 88
# Guild tab (coords local to guild_player_has_guild_window unless noted)
COMMUNITY_GUILD_PANEL_X = 10
COMMUNITY_GUILD_PANEL_Y = 45
COMMUNITY_GUILD_SUB_TAB_IMAGE_X = 0
COMMUNITY_GUILD_SUB_TAB_IMAGE_Y = 0
COMMUNITY_GUILD_SUB_TAB_RADIO_X = 3
COMMUNITY_GUILD_SUB_TAB_RADIO_Y = 5
COMMUNITY_GUILD_ACTION_X_BASE = 150
COMMUNITY_GUILD_ACTION_Y = 0
COMMUNITY_GUILD_ACTION_INTERVAL_X = 30
COMMUNITY_GUILD_INFO_BTN_X = 165
COMMUNITY_GUILD_INFO_BTN_Y = 295
COMMUNITY_GUILD_LIST_PANEL_X = 1
COMMUNITY_GUILD_LIST_PANEL_Y = 32
COMMUNITY_GUILD_LIST_PANEL_W = COMMUNITY_LIST_PANEL_W
COMMUNITY_GUILD_LIST_PANEL_H = 252
COMMUNITY_GUILD_LIST_FRAME_W = COMMUNITY_LIST_FRAME_W
COMMUNITY_GUILD_LIST_X = COMMUNITY_GUILD_LIST_PANEL_X + COMMUNITY_MEMBER_INFO_BAR_DEFAULT_X
COMMUNITY_GUILD_LIST_START_Y = COMMUNITY_GUILD_LIST_PANEL_Y + COMMUNITY_LIST_ROW_Y_OFFSET
COMMUNITY_GUILD_LIST_BOTTOM_PAD = 8
COMMUNITY_GUILD_SCROLL_X = 268
COMMUNITY_GUILD_SCROLL_Y = COMMUNITY_GUILD_LIST_PANEL_Y
COMMUNITY_GUILD_SCROLL_SIZE = 246
COMMUNITY_HIDE_TOOLBAR_BG = True
COMMUNITY_ROW_ICON_X_BASE = COMMUNITY_MEMBER_INFO_BAR_W - 28
COMMUNITY_ROW_ICON_POS_X = COMMUNITY_ROW_ICON_X_BASE - (COMMUNITY_MEMBER_INFO_BUTTON_INTERVAL_X * 2)
COMMUNITY_ROW_ICON_FAVORITE_X = COMMUNITY_ROW_ICON_X_BASE - COMMUNITY_MEMBER_INFO_BUTTON_INTERVAL_X
COMMUNITY_ROW_ICON_PARTY_X = COMMUNITY_ROW_ICON_X_BASE
COMMUNITY_ROW_ICON_Y = 4
# Block list row: single unblock (red X) button position (tune X/Y here)
COMMUNITY_ROW_BLOCK_ICON_W = 24
COMMUNITY_ROW_BLOCK_ICON_H = 24
# Horizontal center of the row bar (258px wide, 24px icon -> X=117)
COMMUNITY_ROW_BLOCK_UNBLOCK_X = (COMMUNITY_LIST_ROW_WIDTH - COMMUNITY_ROW_BLOCK_ICON_W) // 1.2
# Vertical center of the row bar (34px tall, 24px icon -> Y=5)
COMMUNITY_ROW_BLOCK_UNBLOCK_Y = (COMMUNITY_LIST_ROW_HEIGHT - COMMUNITY_ROW_BLOCK_ICON_H) // 2
COMMUNITY_ROW_STATE_X = 7
COMMUNITY_ROW_STATE_Y = 12
# Alt orta oyuncu bilgisi icin
COMMUNITY_ROW_NAME_X = 25
COMMUNITY_ROW_NAME_Y = 10
COMMUNITY_ROW_NAME_PICKER_W = COMMUNITY_ROW_ICON_POS_X - COMMUNITY_ROW_NAME_X
COMMUNITY_ROW_NAME_PICKER_H = 18
COMMUNITY_ROW_LEVEL_X = COMMUNITY_ROW_ICON_POS_X - 5
COMMUNITY_ROW_LEVEL_COLOR = -7751539
COMMUNITY_STATUS_COMMENT_SLOT_H = 20
COMMUNITY_STATUS_COMMENT_SLOT_PAD = 2
# Official my_info_main_player_name_collision shows alignment on name hover.
COMMUNITY_HDR_HIDE_NAME_COLLISION = False
# ------------------------------------------------------------
COMMUNITY_HDR_DROP_X = 26
COMMUNITY_HDR_DROP_Y = 9
COMMUNITY_HDR_DROP_W = 16
COMMUNITY_HDR_DROP_H = 16
COMMUNITY_HDR_GUILD_TEXT_X = 100
COMMUNITY_HDR_GUILD_TEXT_Y = 0
COMMUNITY_HDR_NAME_TEXT_X = 205
COMMUNITY_HDR_NAME_TEXT_Y = 0
COMMUNITY_HDR_NAME_COLLISION_X = 150
COMMUNITY_HDR_NAME_COLLISION_Y = 8
COMMUNITY_HDR_NAME_COLLISION_W = 110
COMMUNITY_HDR_NAME_COLLISION_H = 17
COMMUNITY_HDR_ALIGNMENT_TOOLTIP_MIN_W = 120
COMMUNITY_HDR_MESSAGE_BTN_X = 259
COMMUNITY_HDR_MESSAGE_BTN_Y = 8
# Ust sol durum iconu icin
COMMUNITY_HDR_STATE_X = 10
COMMUNITY_HDR_STATE_Y = 12
# ------------------------------------------------------------
COMMUNITY_SUB_TAB_TOOLTIP_Y = -19
COMMUNITY_SUB_TAB_TOOLTIP_MIN_W = 65
COMMUNITY_CONFIG_TOOLTIP_MIN_W = 220
COMMUNITY_SUB_TAB_PICKER_W = 46
COMMUNITY_SUB_TAB_PICKER_H = 30
COMMUNITY_SUB_TAB_PICKER_X = (13, 58, 103)
COMMUNITY_MAIN_TAB_PICKER_X = (10, 105, 200)
COMMUNITY_MAIN_TAB_PICKER_Y = 400
COMMUNITY_MAIN_TAB_PICKER_W = 95
COMMUNITY_MAIN_TAB_PICKER_H = 90
COMMUNITY_TOOLTIP_W_ACTION = 100
COMMUNITY_STATE_DIALOG_TOOLTIP_W = 280

COMMUNITY_VIEW_MESSENGER = 0
COMMUNITY_VIEW_GUILD = 1
COMMUNITY_VIEW_CONFIG = 2

COMMUNITY_CONFIG_SCROLL_STEP = 0.16666666666666666
COMMUNITY_CONFIG_PANEL_W = 266
COMMUNITY_CONFIG_PANEL_H = 325
COMMUNITY_CONFIG_SCROLLBAR_X = 278
COMMUNITY_CONFIG_SCROLLBAR_Y = 10
COMMUNITY_CONFIG_SCROLLBAR_SIZE = 335
COMMUNITY_CONFIG_BTN_Y = 335
COMMUNITY_CONFIG_SAVE_BTN_X = 175
COMMUNITY_CONFIG_INIT_BTN_X = 12
COMMUNITY_CONFIG_CLIP_X = 1
COMMUNITY_CONFIG_CLIP_Y = 1
COMMUNITY_CONFIG_CLIP_RIGHT_PAD = 0
COMMUNITY_CONFIG_CLIP_BOTTOM_PAD = 4
COMMUNITY_CONFIG_MAIN_TOPIC_W = 258
COMMUNITY_CONFIG_MAIN_TOPIC_TITLE_W = 258
COMMUNITY_CONFIG_MAIN_TOPIC_TITLE_H = 25
COMMUNITY_CONFIG_MAIN_TOPIC_TITLE_X = 0
COMMUNITY_CONFIG_MAIN_TOPIC_TITLE_Y = 0
COMMUNITY_CONFIG_SUB_TOPIC_W = 258
COMMUNITY_CONFIG_SUB_TOPIC_H = 65
COMMUNITY_CONFIG_SUB_TITLE_W = 202
COMMUNITY_CONFIG_SUB_TITLE_H = 25
COMMUNITY_CONFIG_ON_OFF_BTN_W = 43
COMMUNITY_CONFIG_ON_OFF_BTN_H = 21
COMMUNITY_CONFIG_ON_OFF_BTN_GAP = 5
COMMUNITY_CONFIG_ON_OFF_BTN_X = COMMUNITY_CONFIG_SUB_TITLE_W + COMMUNITY_CONFIG_ON_OFF_BTN_GAP
COMMUNITY_CONFIG_ON_OFF_BTN_Y = 2
COMMUNITY_CONFIG_ON_OFF_BTN_VISUAL = PUBLIC_PATH + "small_button_01.sub"
COMMUNITY_CONFIG_ON_OFF_BTN_VISUAL_OVER = PUBLIC_PATH + "small_button_02.sub"
COMMUNITY_CONFIG_ON_OFF_BTN_VISUAL_DOWN = PUBLIC_PATH + "small_button_03.sub"
COMMUNITY_CONFIG_SUB_TITLE_X = 0
COMMUNITY_CONFIG_SUB_TITLE_Y = -2
COMMUNITY_CONFIG_CHECK_ITEM_W = 50
COMMUNITY_CONFIG_CHECK_ITEM_H = 25
COMMUNITY_CONFIG_CHECK_BOX_X = 30
COMMUNITY_CONFIG_CHECK_BOX_Y = 5
COMMUNITY_CONFIG_CHECK_ITEM_INTERVAL_X = 77
COMMUNITY_CONFIG_CHECK_ROW_Y = 28
COMMUNITY_CONFIG_MAIN_TOPIC_GAP = 8
COMMUNITY_CONFIG_CONTENT_X = 2
COMMUNITY_CONFIG_MAIN_TOPIC_X = 2

CONFIG_BLOCK_EXCHANGE_ON = 1
CONFIG_BLOCK_EXCHANGE_GUILD = 2
CONFIG_BLOCK_EXCHANGE_FRIEND = 4
CONFIG_BLOCK_EXCHANGE_FAMILY = 8
CONFIG_BLOCK_PARTY_INVITE_ON = 16
CONFIG_BLOCK_PARTY_INVITE_GUILD = 32
CONFIG_BLOCK_PARTY_INVITE_FRIEND = 64
CONFIG_BLOCK_PARTY_INVITE_FAMILY = 128
CONFIG_BLOCK_PARTY_REQUEST_JOIN_ON = 256
CONFIG_BLOCK_PARTY_REQUEST_JOIN_GUILD = 512
CONFIG_BLOCK_PARTY_REQUEST_JOIN_FRIEND = 1024
CONFIG_BLOCK_PARTY_REQUEST_JOIN_FAMILY = 2048
CONFIG_BLOCK_WHISPER_ON = 4096
CONFIG_BLOCK_WHISPER_GUILD = 8192
CONFIG_BLOCK_WHISPER_FRIEND = 16384
CONFIG_BLOCK_WHISPER_FAMILY = 32768
CONFIG_BLOCK_FRIEND_REQUEST_ON = 65536
CONFIG_BLOCK_FRIEND_REQUEST_GUILD = 131072
CONFIG_BLOCK_FRIEND_REQUEST_FAMILY = 262144
CONFIG_BLOCK_GUILD_INVITE_ON = 524288
CONFIG_BLOCK_GUILD_INVITE_FRIEND = 1048576
CONFIG_BLOCK_GUILD_INVITE_FAMILY = 2097152
CONFIG_LOGIN_ALARM_ON = 4194304
CONFIG_LOGIN_ALARM_ON_GUILD = 8388608
CONFIG_LOGIN_ALARM_ON_FRIEND = 16777216
CONFIG_LOGIN_ALARM_ON_FAMILY = 33554432
CONFIG_MY_INFO_SHOW_ON = 67108864
CONFIG_MY_INFO_SHOW_ON_LEVEL = 134217728
CONFIG_MY_INFO_SHOW_ON_LOCATION = 268435456

CONFIG_CHECK_GUILD = 0
CONFIG_CHECK_GUILD_LOGIN = 1
CONFIG_CHECK_FRIEND = 2
CONFIG_CHECK_FRIEND_LOGIN = 3
CONFIG_CHECK_FAMILY = 4
CONFIG_CHECK_FAMILY_LOGIN = 5
CONFIG_CHECK_LEVEL = 6
CONFIG_CHECK_LOCATION = 7

CONFIG_CHECK_ICON_PATH = {
	CONFIG_CHECK_GUILD: ROOT_PATH + "community_icon_guild_member.sub",
	CONFIG_CHECK_GUILD_LOGIN: ROOT_PATH + "community_icon_guild_member.sub",
	CONFIG_CHECK_FRIEND: ROOT_PATH + "community_icon_friend.sub",
	CONFIG_CHECK_FRIEND_LOGIN: ROOT_PATH + "community_icon_friend.sub",
	CONFIG_CHECK_FAMILY: ROOT_PATH + "community_icon_family_default.sub",
	CONFIG_CHECK_FAMILY_LOGIN: ROOT_PATH + "community_icon_family_default.sub",
	CONFIG_CHECK_LEVEL: ROOT_PATH + "community_icon_level.sub",
	CONFIG_CHECK_LOCATION: ROOT_PATH + "community_icon_position_default.sub",
}

CONFIG_CHECK_TOOLTIP_KEY = {
	CONFIG_CHECK_GUILD: "COMMUNITY_CONFIG_INCLUDE_GUILD_MEMBER",
	CONFIG_CHECK_GUILD_LOGIN: "COMMUNITY_CONFIG_GUILD_MEMBER_LOGIN_ALARM",
	CONFIG_CHECK_FRIEND: "COMMUNITY_CONFIG_INCLUDE_FRIEND_MEMBER",
	CONFIG_CHECK_FRIEND_LOGIN: "COMMUNITY_CONFIG_FRIEND_MEMBER_LOGIN_ALARM",
	CONFIG_CHECK_FAMILY: "COMMUNITY_CONFIG_INCLUDE_FAMILY_MEMBER",
	CONFIG_CHECK_FAMILY_LOGIN: "COMMUNITY_CONFIG_FAMILY_MEMBER_LOGIN_ALARM",
	CONFIG_CHECK_LEVEL: "COMMUNITY_CONFIG_LEVEL_INFO_SHOW",
	CONFIG_CHECK_LOCATION: "COMMUNITY_CONFIG_LOCATION_INFO_SHOW",
}

CONFIG_SUB_TOPIC_TITLE = {
	0: "COMMUNITY_CONFIG_SUB_BLOCK_EXCHANGE",
	1: "COMMUNITY_CONFIG_SUB_BLOCK_PARTY_INVITE",
	2: "COMMUNITY_CONFIG_SUB_BLOCK_PARTY_REQUEST",
	3: "COMMUNITY_CONFIG_SUB_BLOCK_WHISPER",
	4: "COMMUNITY_CONFIG_SUB_BLOCK_FRIEND_REQUEST",
	5: "COMMUNITY_CONFIG_SUB_BLOCK_GUILD_INVITE",
	6: "COMMUNITY_CONFIG_SUB_LOGIN_ALARM",
	7: "COMMUNITY_CONFIG_SUB_MY_INFO_ALARM",
}

MESSENGER_VIEW_FRIEND = 0
MESSENGER_VIEW_BLOCK = 1
MESSENGER_VIEW_REQUEST = 2

REQUEST_TAB_TWINKLE_INTERVAL = 0.5

def GetAlignmentGradeColor(grade):
	import colorInfo
	colorDict = {
		0: colorInfo.TITLE_RGB_GOOD_4,
		1: colorInfo.TITLE_RGB_GOOD_3,
		2: colorInfo.TITLE_RGB_GOOD_2,
		3: colorInfo.TITLE_RGB_GOOD_1,
		4: colorInfo.TITLE_RGB_NORMAL,
		5: colorInfo.TITLE_RGB_EVIL_1,
		6: colorInfo.TITLE_RGB_EVIL_2,
		7: colorInfo.TITLE_RGB_EVIL_3,
		8: colorInfo.TITLE_RGB_EVIL_4,
	}
	colorList = colorDict.get(grade, colorInfo.TITLE_RGB_NORMAL)
	return ui.GenerateColor(colorList[0], colorList[1], colorList[2])


def GetUiScriptLocaleText(name, default=""):
	text = getattr(uiScriptLocale, name, default)
	if text:
		return text
	return default

def GetCommunityConfigToolTipMinWidth(text):
	if not text:
		return COMMUNITY_CONFIG_TOOLTIP_MIN_W
	return max(COMMUNITY_CONFIG_TOOLTIP_MIN_W, min(360, len(text) * 6 + 24))

def IsMouseInWindowRect(window):
	if not window or not window.IsShow():
		return False
	mx, my = wndMgr.GetMousePosition()
	wx, wy = window.GetGlobalPosition()
	return wx <= mx <= wx + window.GetWidth() and wy <= my <= wy + window.GetHeight()

def ApplyCommunityClipMask(widget, clipWindow):
	if not app.ENABLE_CLIP_MASK or not clipWindow or not widget:
		return
	if hasattr(widget, "SetClippingMaskWindow"):
		widget.SetClippingMaskWindow(clipWindow)
	if isinstance(widget, ui.Button):
		buttonText = getattr(widget, "ButtonText", None)
		if buttonText:
			ApplyCommunityClipMask(buttonText, clipWindow)

def GetConnectionStateImage(state):
	if state == community.LEFT_SEAT:
		return ROOT_PATH + "community_state_deep_red.sub"
	if state == community.AUTO_HUNT:
		return ROOT_PATH + "community_state_blue.sub"
	if state == community.SHOP_OPEN:
		return ROOT_PATH + "community_state_yellow.sub"
	if state == community.DISCONNECT:
		return COMMUNITY_STATE_OFFLINE
	return COMMUNITY_STATE_ONLINE

def GetConnectionStateLocaleKey(state):
	keys = {
		community.CONNECT: "COMMUNITY_CONNECT",
		community.LEFT_SEAT: "COMMUNITY_LEFT_SEAT",
		community.AUTO_HUNT: "COMMUNITY_AUTO_HUNT",
		community.SHOP_OPEN: "COMMUNITY_SHOP_OPEN",
		community.DISCONNECT: "COMMUNITY_DISCONNECT",
	}
	return keys.get(state, "COMMUNITY_CONNECT")

def GetConnectionStateLabel(state):
	key = GetConnectionStateLocaleKey(state)
	text = GetUiScriptLocaleText(key)
	if text:
		return text
	fallbacks = {
		community.CONNECT: "Online",
		community.LEFT_SEAT: "AFK",
		community.AUTO_HUNT: "Auto-Hunt",
		community.SHOP_OPEN: "Shop",
		community.DISCONNECT: "Offline",
	}
	return fallbacks.get(state, key)

def GetConnectionStateRowDialogToolTip(state):
	dialogKeys = {
		community.LEFT_SEAT: "COMMUNITY_LEFT_SEAT_DIALOG",
		community.DISCONNECT: "COMMUNITY_DISCONNECT_DIALOG",
	}
	key = dialogKeys.get(state)
	if not key:
		return ""
	return GetUiScriptLocaleText(key)

COMMUNITY_STATE_OPTIONS = (
	community.CONNECT,
	community.LEFT_SEAT,
	community.AUTO_HUNT,
	community.SHOP_OPEN,
	community.DISCONNECT,
)

class CommunityConnectionStateBox(ui.Window):
	def __init__(self):
		ui.Window.__init__(self)
		self.AddFlag("not_pick")
		ui.Window.SetSize(self, 10, 10)
		self.currentState = community.CONNECT
		self.stateImage = ui.ImageBox()
		self.stateImage.AddFlag("not_pick")
		self.stateImage.SetParent(self)
		self.stateImage.LoadImage(GetConnectionStateImage(self.currentState))
		self.stateImage.Show()

	def SetConnectionState(self, state):
		self.currentState = state
		self.stateImage.LoadImage(GetConnectionStateImage(state))

	def GetConnectionState(self):
		return self.currentState

class CommunityStateDropDownList(ui.Window):
	def __init__(self):
		ui.Window.__init__(self)
		self.AddFlag("float")
		self.selectEvent = None
		self.itemButtons = []
		self.itemOverImages = []
		self.itemLabels = []
		self.itemIcons = []
		self.hoverIndex = -1
		self._built = False
		self.toolTipHelper = None

	def __del__(self):
		ui.Window.__del__(self)

	def __GetRowBgPath(self, index):
		if index == 0:
			return COMMUNITY_STATE_DROP_TOP
		if index == len(COMMUNITY_STATE_OPTIONS) - 1:
			return COMMUNITY_STATE_DROP_BOTTOM
		return COMMUNITY_STATE_DROP_MIDDLE

	def __EnsureBuilt(self):
		if self._built:
			return
		self._built = True
		self.__BuildItems()

	def __BuildItems(self):
		self.SetSize(COMMUNITY_STATE_DROP_W, COMMUNITY_STATE_DROP_ITEM_H * len(COMMUNITY_STATE_OPTIONS))
		for index, state in enumerate(COMMUNITY_STATE_OPTIONS):
			yPos = index * COMMUNITY_STATE_DROP_ITEM_H
			rowPath = self.__GetRowBgPath(index)

			rowBtn = ui.Button()
			rowBtn.SetParent(self)
			rowBtn.SetPosition(0, yPos)
			rowBtn.SetUpVisual(rowPath)
			rowBtn.SetOverVisual(rowPath)
			rowBtn.SetDownVisual(rowPath)
			rowBtn.SetOverEvent(ui.__mem_func__(self.__OnOverInItem), index)
			rowBtn.SetOverOutEvent(ui.__mem_func__(self.__OnOverOutItem))
			rowBtn.SetEvent(ui.__mem_func__(self.__OnSelectItem), state)
			rowBtn.Show()
			self.itemButtons.append(rowBtn)

			overImage = ui.ImageBox()
			overImage.AddFlag("not_pick")
			overImage.SetParent(rowBtn)
			overImage.LoadImage(COMMUNITY_STATE_DROP_OVER)
			overImage.SetPosition(0, 0)
			overImage.Hide()
			self.itemOverImages.append(overImage)

			iconBox = ui.ImageBox()
			iconBox.AddFlag("not_pick")
			iconBox.SetParent(rowBtn)
			iconBox.LoadImage(GetConnectionStateImage(state))
			iconBox.SetPosition(COMMUNITY_STATE_ICON_X, COMMUNITY_STATE_TEXT_Y)
			iconBox.Show()
			self.itemIcons.append(iconBox)

			label = ui.TextLine()
			label.AddFlag("not_pick")
			label.SetParent(rowBtn)
			label.SetPosition(COMMUNITY_STATE_TEXT_X, COMMUNITY_STATE_TEXT_Y)
			label.SetPackedFontColor(0xFFCFCFCF)
			label.SetText(GetConnectionStateLabel(state))
			label.Show()
			self.itemLabels.append(label)

	def SetSelectItemEvent(self, event):
		self.selectEvent = event

	def SetToolTipHelper(self, helper):
		self.toolTipHelper = helper

	def __OnOverInItem(self, index):
		self.hoverIndex = index
		for idx, overImage in enumerate(self.itemOverImages):
			if idx == index:
				overImage.Show()
			else:
				overImage.Hide()
		if self.toolTipHelper:
			tip = GetConnectionStateRowDialogToolTip(COMMUNITY_STATE_OPTIONS[index])
			if tip:
				self.toolTipHelper.Show(tip, COMMUNITY_STATE_DIALOG_TOOLTIP_W)
			else:
				self.toolTipHelper.Hide()

	def __OnOverOutItem(self):
		self.hoverIndex = -1
		for overImage in self.itemOverImages:
			overImage.Hide()
		if self.toolTipHelper:
			self.toolTipHelper.Hide()

	def __OnSelectItem(self, state):
		if self.selectEvent:
			self.selectEvent(state)
		self.Hide()

	def Open(self):
		self.__EnsureBuilt()
		self.__OnOverOutItem()
		self.Show()
		self.SetTop()

	def Close(self):
		self.__OnOverOutItem()
		self.Hide()

class StatusMessageEditLine(ui.EditLine):
	GUIDE_TEXT_COLOR = grp.GenerateColor(0.6, 0.6, 0.6, 1.0)
	NORMAL_TEXT_COLOR = grp.GenerateColor(1.0, 1.0, 1.0, 1.0)

	def __init__(self):
		ui.EditLine.__init__(self)
		self.guideText = ""
		self.isGuideMode = False
		self.guideClearEvent = None
		self.max = community.MAX_STATUS_MESSAGE_LENGTH

	def __del__(self):
		ui.EditLine.__del__(self)

	def SetGuideText(self, text):
		self.guideText = text
		self.__ShowGuide()

	def SetGuideMessageClearEvent(self, event):
		self.guideClearEvent = event

	def IsGuideEventExist(self):
		return self.guideClearEvent is not None

	def __ShowGuide(self):
		self.isGuideMode = True
		self.SetPackedFontColor(self.GUIDE_TEXT_COLOR)
		self.SetText(self.guideText)

	def __ClearGuide(self):
		if not self.isGuideMode:
			return
		self.isGuideMode = False
		self.SetPackedFontColor(self.NORMAL_TEXT_COLOR)
		self.SetText("")
		if self.guideClearEvent:
			self.guideClearEvent()

	def SetFocus(self):
		self.__ClearGuide()
		ui.EditLine.SetFocus(self)

	def OnMouseLeftButtonDown(self):
		if self.isGuideMode:
			self.__ClearGuide()
		self.SetFocus()
		return True

	def OnSetFocus(self):
		self.__ClearGuide()
		ui.EditLine.OnSetFocus(self)

	def OnKeyDown(self, key):
		if self.isGuideMode:
			self.__ClearGuide()
		return ui.EditLine.OnKeyDown(self, key)

	def OnKeyUp(self, key):
		return ui.EditLine.OnKeyUp(self, key)

	def GetInputText(self):
		if self.isGuideMode:
			return ""
		text = self.GetText()
		if text == self.guideText:
			return ""
		return text

	def OnKillFocus(self):
		ui.EditLine.OnKillFocus(self)
		text = self.GetText()
		if not text or text == self.guideText:
			self.__ShowGuide()

class StatusMessageInputDialog(ui.BoardWithTitleBar):
	WIDTH = 380
	HEIGHT = 85
	SLOT_H = 18
	BTN_SIZE = 24
	PAD_X = 11
	PAD_Y = 33
	PAD_Y2 = 55
	MY_INFO_BAR_H = 35

	def __init__(self):
		self.myMessageTextLine = None
		self.myMessageSlotBar = None
		self.deleteButton = None
		self.registerEditLine = None
		self.registerSlotBar = None
		self.registerButton = None
		self.currentStatusMessage = ""
		self.slotWidth = self.WIDTH - (self.PAD_X * 2) - self.BTN_SIZE - 6
		ui.BoardWithTitleBar.__init__(self)
		self.AddFlag("float")
		self.AddFlag("movable")
		self.__LoadWindow()

	def __del__(self):
		ui.BoardWithTitleBar.__del__(self)

	def __LoadWindow(self):
		self.SetSize(self.WIDTH, self.HEIGHT)
		self.SetTitleName(GetUiScriptLocaleText("COMMUNITY_MY_STATUS_MESSAGE"))
		self.SetCloseEvent(ui.__mem_func__(self.Hide))

		slotBar1 = ui.SlotBar()
		slotBar1.SetParent(self)
		slotBar1.SetPosition(self.PAD_X, self.PAD_Y)
		slotBar1.SetSize(self.slotWidth, self.SLOT_H)
		slotBar1.Show()
		self.myMessageSlotBar = slotBar1

		textLine = ui.TextLine()
		textLine.SetParent(slotBar1)
		textLine.SetPosition(3, 3)
		textLine.SetLimitWidth(self.slotWidth - 6)
		textLine.Show()
		self.myMessageTextLine = textLine

		deleteBtn = ui.Button()
		deleteBtn.SetParent(self)
		deleteBtn.SetPosition(self.PAD_X + self.slotWidth + 15, self.PAD_Y - 0)
		deleteBtn.SetUpVisual(PUBLIC_PATH + "close_button_01.sub")
		deleteBtn.SetOverVisual(PUBLIC_PATH + "close_button_02.sub")
		deleteBtn.SetDownVisual(PUBLIC_PATH + "close_button_03.sub")
		deleteBtn.SetToolTipText(localeInfo.COMMUNITY_DELETE)
		deleteBtn.SetEvent(ui.__mem_func__(self.__OnSelectDeleteButton))
		deleteBtn.Show()
		self.deleteButton = deleteBtn

		slotBar2 = ui.SlotBar()
		slotBar2.SetParent(self)
		slotBar2.SetPosition(self.PAD_X, self.PAD_Y2)
		slotBar2.SetSize(self.slotWidth, self.SLOT_H)
		slotBar2.Show()
		self.registerSlotBar = slotBar2

		editLine = StatusMessageEditLine()
		editLine.SetParent(slotBar2)
		editLine.SetPosition(3, 2)
		editLine.SetSize(self.slotWidth - 6, self.SLOT_H - 2)
		editLine.SetLimitWidth(self.slotWidth - 6)
		editLine.SetMax(community.MAX_STATUS_MESSAGE_LENGTH)
		editLine.SetGuideText(GetUiScriptLocaleText("COMMUNITY_STATUS_MESSAGE_GUILD_TEXT"))
		editLine.SetReturnEvent(ui.__mem_func__(self.__OnSelectRegisterButton))
		editLine.SetEscapeEvent(ui.__mem_func__(self.Hide))
		editLine.Show()
		self.registerEditLine = editLine

		registerBtn = ui.Button()
		registerBtn.SetParent(self)
		registerBtn.SetPosition(self.PAD_X + self.slotWidth + 10, self.PAD_Y2 - 0)
		registerBtn.SetUpVisual(TASKBAR_PATH + "send_chat_button_01.sub")
		registerBtn.SetOverVisual(TASKBAR_PATH + "send_chat_button_02.sub")
		registerBtn.SetDownVisual(TASKBAR_PATH + "send_chat_button_03.sub")
		registerBtn.SetToolTipText(localeInfo.COMMUNITY_REGISTER_COMMENT)
		registerBtn.SetEvent(ui.__mem_func__(self.__OnSelectRegisterButton))
		registerBtn.Show()
		self.registerButton = registerBtn

	def LoadMyStatusMessage(self, status_message):
		self.currentStatusMessage = status_message or ""
		if self.myMessageTextLine:
			self.myMessageTextLine.SetText(self.currentStatusMessage)

	def Show(self):
		self.__ClearEditLineText()
		ui.BoardWithTitleBar.Show(self)
		self.SetTop()
		if self.registerEditLine:
			self.registerEditLine.SetFocus()

	def OnKeyDown(self, key):
		if self.registerEditLine and self.registerEditLine.IsFocus():
			if self.registerEditLine.OnKeyDown(key):
				return True
		return False

	def OnKeyUp(self, key):
		if self.registerEditLine and self.registerEditLine.IsFocus():
			if self.registerEditLine.OnKeyUp(key):
				return True
		return False

	def Hide(self):
		if self.registerEditLine and self.registerEditLine.IsFocus():
			self.registerEditLine.KillFocus()
		ui.BoardWithTitleBar.Hide(self)

	def OnPressEscapeKey(self):
		self.Hide()
		return True

	def __ClearEditLineText(self):
		if not self.registerEditLine:
			return
		self.registerEditLine.SetText("")
		self.registerEditLine.SetGuideText(
			GetUiScriptLocaleText("COMMUNITY_STATUS_MESSAGE_GUILD_TEXT"))

	def __OnSelectDeleteButton(self):
		if hasattr(community, "CanChangeMyStatusMessageTime"):
			if not community.CanChangeMyStatusMessageTime():
				return
		if hasattr(community, "SendDeleteMyStatusMessage"):
			community.SendDeleteMyStatusMessage()
		else:
			self.LoadMyStatusMessage("")

	def __OnSelectRegisterButton(self):
		if not self.registerEditLine:
			return
		message = self.registerEditLine.GetInputText()
		if message:
			message = message.strip()
		if not message:
			return
		if hasattr(community, "CanChangeMyStatusMessageTime"):
			if not community.CanChangeMyStatusMessageTime():
				return
		if hasattr(community, "SendRegisterMyStatusMessage"):
			community.SendRegisterMyStatusMessage(message)
		else:
			self.LoadMyStatusMessage(message)
		self.__ClearEditLineText()

class CommunityUiToolTipHelper(object):
	def __init__(self):
		self._toolTip = None

	def Hide(self):
		if self._toolTip:
			self._toolTip.HideToolTip()
			self._toolTip = None

	def Show(self, text, minWidth=COMMUNITY_SUB_TAB_TOOLTIP_MIN_W):
		self.Hide()
		if not text:
			return
		tooltip = uiToolTip.ToolTip(minWidth)
		tooltip.AutoAppendTextLine(text)
		tooltip.AlignTextLineHorizonalCenter()
		tooltip.ResizeToolTip()
		tooltip.ShowToolTip()
		self._toolTip = tooltip

	def ShowLines(self, lines, minWidth=COMMUNITY_SUB_TAB_TOOLTIP_MIN_W):
		self.Hide()
		if not lines:
			return
		tooltip = uiToolTip.ToolTip(minWidth)
		for line in lines:
			if line:
				tooltip.AutoAppendTextLine(line)
		tooltip.AlignTextLineHorizonalCenter()
		tooltip.ResizeToolTip()
		tooltip.ShowToolTip()
		self._toolTip = tooltip

	def __OnOverIn(self, text, minWidth=COMMUNITY_SUB_TAB_TOOLTIP_MIN_W):
		self.Show(text, minWidth)

	def Bind(self, button, text, minWidth=COMMUNITY_SUB_TAB_TOOLTIP_MIN_W):
		if not button or not text:
			return
		if hasattr(button, "SetOverEvent"):
			button.SetOverEvent(ui.__mem_func__(self.__OnOverIn), text, minWidth)
			button.SetOverOutEvent(ui.__mem_func__(self.Hide))
		elif hasattr(button, "SetShowToolTipEvent"):
			button.SetShowToolTipEvent(ui.__mem_func__(self.__OnOverIn), text, minWidth)
			button.SetHideToolTipEvent(ui.__mem_func__(self.Hide))
		elif hasattr(button, "SetEvent"):
			button.SetEvent(ui.__mem_func__(self.__OnOverIn), "mouse_over_in", text, minWidth)
			button.SetEvent(ui.__mem_func__(self.Hide), "mouse_over_out")

	def BindShowToolTip(self, button, text, minWidth=COMMUNITY_SUB_TAB_TOOLTIP_MIN_W):
		if not button or not text:
			return
		if hasattr(button, "SetShowToolTipEvent"):
			button.SetShowToolTipEvent(ui.__mem_func__(self.__OnOverIn), text, minWidth)
			button.SetHideToolTipEvent(ui.__mem_func__(self.Hide))
		else:
			self.Bind(button, text, minWidth)

class CommunityFamilyInfoBar(ui.Window):
	"""Official FamilyInfoBar: _default.sub = dim/disabled, _mouse_over.sub = active hover."""

	def __init__(self):
		ui.Window.__init__(self)
		self.AddFlag("not_pick")
		ui.Window.SetSize(self, COMMUNITY_MEMBER_INFO_BAR_W, COMMUNITY_MEMBER_INFO_BAR_H)

		self.infoBar = ui.ImageBox()
		self.infoBar.AddFlag("not_pick")
		self.infoBar.SetParent(self)
		self.infoBar.LoadImage(COMMUNITY_FAMILY_INFO_BAR_BG)
		self.infoBar.SetPosition(0, 0)
		self.infoBar.Show()

		self.locationButton = self.__CreateActionButton(
			COMMUNITY_FAMILY_INFO_BTN_POS_X,
			COMMUNITY_FAMILY_INFO_BTN_Y,
			COMMUNITY_ROW_ICON_POSITION,
			COMMUNITY_ROW_ICON_POSITION_OVER,
			COMMUNITY_ROW_ICON_POSITION,
		)
		self.partyInviteButton = self.__CreateActionButton(
			COMMUNITY_FAMILY_INFO_BTN_PARTY_X,
			COMMUNITY_FAMILY_INFO_BTN_Y,
			COMMUNITY_ROW_ICON_PARTY_INVITE,
			COMMUNITY_ROW_ICON_PARTY_INVITE_OVER,
			COMMUNITY_ROW_ICON_PARTY_INVITE_CLICK,
		)
		self.lovePointButton = self.__CreateActionButton(
			COMMUNITY_FAMILY_INFO_BTN_LOVE_X,
			COMMUNITY_FAMILY_INFO_BTN_Y,
			COMMUNITY_ROW_ICON_FAMILY,
			COMMUNITY_ROW_ICON_FAMILY_OVER,
			COMMUNITY_ROW_ICON_FAMILY,
		)

		self.handler = None
		self._toolTipHelper = CommunityUiToolTipHelper()
		self.partyInviteButton.SetEvent(ui.__mem_func__(self.__OnClickPartyInviteButton))
		self.__BindToolTips()

		self.DisableFamilyInfoBar()

	def __CreateActionButton(self, x, y, defaultPath, overPath, downPath):
		button = ui.Button()
		button.SetParent(self)
		button.SetPosition(x, y)
		button.SetUpVisual(defaultPath)
		button.SetOverVisual(overPath)
		button.SetDownVisual(downPath)
		button.SetDisableVisual(defaultPath)
		button.Show()
		button._communityDefaultPath = defaultPath
		button._communityOverPath = overPath
		button._communityDownPath = downPath
		return button

	def __SetActionButtonEnabled(self, button, isEnabled):
		if not button:
			return
		defaultPath = button._communityDefaultPath
		overPath = button._communityOverPath
		downPath = button._communityDownPath
		button.SetUpVisual(defaultPath)
		button.SetDisableVisual(defaultPath)
		if isEnabled:
			button.SetOverVisual(overPath)
			button.SetDownVisual(downPath)
			button.Enable()
		else:
			button.SetOverVisual(defaultPath)
			button.SetDownVisual(defaultPath)
			button.Disable()

	def DisableFamilyInfoBar(self):
		self.__SetActionButtonEnabled(self.locationButton, False)
		self.__SetActionButtonEnabled(self.partyInviteButton, False)
		self.__SetActionButtonEnabled(self.lovePointButton, False)

	def EnableFamilyInfoBar(self):
		self.__SetActionButtonEnabled(self.locationButton, True)
		self.__SetActionButtonEnabled(self.partyInviteButton, True)
		self.__SetActionButtonEnabled(self.lovePointButton, True)

	def DisableLocationInfoButton(self):
		self.__SetActionButtonEnabled(self.locationButton, False)

	def EnableLocationInfoButton(self):
		self.__SetActionButtonEnabled(self.locationButton, True)

	def DisablePartyInviteButton(self):
		self.__SetActionButtonEnabled(self.partyInviteButton, False)

	def EnablePartyInviteButton(self):
		self.__SetActionButtonEnabled(self.partyInviteButton, True)

	def SetHandler(self, handler):
		self.handler = handler

	def __BindToolTips(self):
		helper = self._toolTipHelper
		locationTip = GetUiScriptLocaleText("COMMUNITY_MAP_INFO")
		if locationTip and "%" in locationTip:
			locationTip = locationTip.split("%", 1)[0].rstrip(" :")
		helper.Bind(self.locationButton, locationTip, COMMUNITY_TOOLTIP_W_ACTION)
		helper.Bind(
			self.partyInviteButton,
			GetUiScriptLocaleText("COMMUNITY_PARTY_INVITE"),
			COMMUNITY_TOOLTIP_W_ACTION)
		self.lovePointButton.SetOverEvent(ui.__mem_func__(self.__OnOverLovePointButton))
		self.lovePointButton.SetOverOutEvent(ui.__mem_func__(self.__OnOutToolTip))

	def __OnOutToolTip(self):
		self._toolTipHelper.Hide()

	def __OnOverLovePointButton(self):
		if not self.handler:
			return
		lovePoint = self.handler.GetFamilyLovePoint()
		if lovePoint <= 0:
			return
		fmt = GetUiScriptLocaleText("COMMUNITY_TOOLTIP_LOVE_POINT")
		if fmt:
			self._toolTipHelper.Show(fmt % lovePoint, COMMUNITY_TOOLTIP_W_ACTION)

	def __OnClickPartyInviteButton(self):
		if self.handler:
			self.handler.OnFamilyPartyInviteClick()

	def __del__(self):
		ui.Window.__del__(self)

class CommunityListPanel(ui.Window):
	def __init__(self, width, height):
		ui.Window.__init__(self)
		self.AddFlag("not_pick")
		ui.Window.SetSize(self, width, height)
		yPos = 0
		while yPos < height:
			tile = ui.ImageBox()
			tile.AddFlag("not_pick")
			tile.SetParent(self)
			tile.LoadImage(COMMUNITY_MEMBER_INFO_BAR_BG)
			tile.SetPosition(0, yPos)
			tile.Show()
			yPos += COMMUNITY_LIST_LINE_HEIGHT

	def __del__(self):
		ui.Window.__del__(self)

class CommunityMemberItem(ui.Window):
	def __init__(self, getHandlerEvent, getListParentEvent):
		ui.Window.__init__(self)
		self.getParentEvent = getHandlerEvent
		parent = getListParentEvent()
		if parent:
			self.SetParent(parent)
		self.AddFlag("float")
		self.SetSize(COMMUNITY_LIST_ROW_WIDTH, COMMUNITY_LIST_ROW_HEIGHT)

		self.name = ""
		self.key = None
		self.state = 0
		self.connectionState = community.DISCONNECT
		self.level = 0
		self.isSelected = False
		self.isHovered = False
		self.offlineTime = 0
		self.offlineTimeToolTip = None
		self.countryFlagImage = None
		self.channel = 0
		self.mapIndex = 0
		self.hasLocationInfo = False
		self.isFavorite = False
		self.iconToolTip = None
		self.statusMessage = ""

		self.renewalBackground = ui.ImageBox()
		self.renewalBackground.AddFlag("not_pick")
		self.renewalBackground.SetParent(self)
		self.renewalBackground.LoadImage(COMMUNITY_MEMBER_INFO_BAR_BG)
		self.renewalBackground.Show()

		self.image = ui.ImageBox()
		self.image.AddFlag("not_pick")
		self.image.SetParent(self)
		self.image.SetPosition(COMMUNITY_ROW_STATE_X, COMMUNITY_ROW_STATE_Y)
		self.image.Show()

		self.stateIconPicker = ui.Window()
		self.stateIconPicker.SetParent(self)
		self.stateIconPicker.SetPosition(COMMUNITY_ROW_STATE_X, COMMUNITY_ROW_STATE_Y)
		self.stateIconPicker.SetSize(10, 10)
		self.stateIconPicker.SetOverEvent(ui.__mem_func__(self.__OnOverStateIcon))
		self.stateIconPicker.SetOverOutEvent(ui.__mem_func__(self.__OnOutRowButtonToolTip))
		self.stateIconPicker.Show()

		self.text = ui.TextLine()
		self.text.AddFlag("not_pick")
		self.text.SetParent(self)
		if app.WJ_MULTI_TEXTLINE:
			self.text.DisableEnterToken()
		self.text.SetPosition(COMMUNITY_ROW_NAME_X, COMMUNITY_ROW_NAME_Y)
		self.text.Show()

		self.nameHoverPicker = ui.Window()
		self.nameHoverPicker.SetParent(self)
		self.nameHoverPicker.SetPosition(COMMUNITY_ROW_NAME_X, COMMUNITY_ROW_NAME_Y - 2)
		self.nameHoverPicker.SetSize(COMMUNITY_ROW_NAME_PICKER_W, COMMUNITY_ROW_NAME_PICKER_H)
		self.nameHoverPicker.SetOverEvent(ui.__mem_func__(self.__OnOverName))
		self.nameHoverPicker.SetOverOutEvent(ui.__mem_func__(self.__OnOutRowButtonToolTip))
		self.nameHoverPicker.Show()

		self.levelText = ui.TextLine()
		self.levelText.AddFlag("not_pick")
		self.levelText.SetParent(self)
		if app.WJ_MULTI_TEXTLINE:
			self.levelText.DisableEnterToken()
		self.levelText.SetPosition(COMMUNITY_ROW_LEVEL_X, COMMUNITY_ROW_NAME_Y)
		self.levelText.SetHorizontalAlignRight()
		self.levelText.SetPackedFontColor(COMMUNITY_ROW_LEVEL_COLOR)
		self.levelText.Hide()

		self.locationButton = self.__CreateRowButton(
			COMMUNITY_ROW_ICON_POS_X,
			COMMUNITY_ROW_ICON_Y,
			COMMUNITY_ROW_ICON_POSITION,
			COMMUNITY_ROW_ICON_POSITION_OVER,
			COMMUNITY_ROW_ICON_POSITION,
		)
		self.favoriteButton = self.__CreateRowButton(
			COMMUNITY_ROW_ICON_FAVORITE_X,
			COMMUNITY_ROW_ICON_Y,
			COMMUNITY_ROW_ICON_WISHLIST,
			COMMUNITY_ROW_ICON_WISHLIST_OVER,
			COMMUNITY_ROW_ICON_WISHLIST_CLICK,
		)
		self.partyInviteButton = self.__CreateRowButton(
			COMMUNITY_ROW_ICON_PARTY_X,
			COMMUNITY_ROW_ICON_Y,
			COMMUNITY_ROW_ICON_PARTY_INVITE,
			COMMUNITY_ROW_ICON_PARTY_INVITE_OVER,
			COMMUNITY_ROW_ICON_PARTY_INVITE_CLICK,
		)

		self.locationButton.SetOverEvent(ui.__mem_func__(self.__OnOverLocationButton))
		self.locationButton.SetOverOutEvent(ui.__mem_func__(self.__OnOutRowButtonToolTip))
		self.favoriteButton.SetOverEvent(ui.__mem_func__(self.__OnOverFavoriteButton))
		self.favoriteButton.SetOverOutEvent(ui.__mem_func__(self.__OnOutRowButtonToolTip))
		self.favoriteButton.SetEvent(ui.__mem_func__(self.__OnClickFavoriteButton))
		self.partyInviteButton.SetOverEvent(ui.__mem_func__(self.__OnOverPartyInviteButton))
		self.partyInviteButton.SetOverOutEvent(ui.__mem_func__(self.__OnOutRowButtonToolTip))
		self.partyInviteButton.SetEvent(ui.__mem_func__(self.__OnClickPartyInviteButton))

		self.Offline()

	def __CreateRowButton(self, x, y, defaultPath, overPath, downPath):
		button = ui.Button()
		button.SetParent(self)
		button.SetPosition(x, y)
		button.SetUpVisual(defaultPath)
		button.SetOverVisual(overPath)
		button.SetDownVisual(downPath)
		button.SetDisableVisual(defaultPath)
		button.Show()
		button._communityDefaultPath = defaultPath
		button._communityOverPath = overPath
		button._communityDownPath = downPath
		return button

	def __SetRowButtonEnabled(self, button, isEnabled):
		if not button:
			return
		defaultPath = button._communityDefaultPath
		overPath = button._communityOverPath
		downPath = button._communityDownPath
		button.SetUpVisual(defaultPath)
		button.SetDisableVisual(defaultPath)
		if isEnabled:
			button.SetOverVisual(overPath)
			button.SetDownVisual(downPath)
			button.Enable()
		else:
			button.SetOverVisual(defaultPath)
			button.SetDownVisual(defaultPath)
			button.Disable()

	def __RefreshFavoriteVisual(self):
		if self.isFavorite:
			self.favoriteButton._communityDefaultPath = COMMUNITY_ROW_ICON_WISHLIST_OVER
			self.favoriteButton._communityOverPath = COMMUNITY_ROW_ICON_WISHLIST_CLICK
			self.favoriteButton._communityDownPath = COMMUNITY_ROW_ICON_WISHLIST_CLICK
		else:
			self.favoriteButton._communityDefaultPath = COMMUNITY_ROW_ICON_WISHLIST
			self.favoriteButton._communityOverPath = COMMUNITY_ROW_ICON_WISHLIST_OVER
			self.favoriteButton._communityDownPath = COMMUNITY_ROW_ICON_WISHLIST_CLICK

	def __RefreshRowButtons(self):
		self.__RefreshFavoriteVisual()
		if self.IsOnline():
			self.__SetRowButtonEnabled(
				self.locationButton, self.hasLocationInfo and (self.channel > 0 or self.mapIndex != 0))
			self.__SetRowButtonEnabled(self.favoriteButton, True)
			self.__SetRowButtonEnabled(self.partyInviteButton, True)
		else:
			self.__SetRowButtonEnabled(self.locationButton, False)
			self.__SetRowButtonEnabled(self.favoriteButton, False)
			self.__SetRowButtonEnabled(self.partyInviteButton, False)

	def __HideIconToolTip(self):
		if self.iconToolTip:
			self.iconToolTip.HideToolTip()
			self.iconToolTip = None

	def __HideOfflineTimeToolTip(self):
		if self.offlineTimeToolTip:
			self.offlineTimeToolTip.HideToolTip()
			self.offlineTimeToolTip = None

	def __HideAllRowToolTips(self):
		self.__HideIconToolTip()
		self.__HideOfflineTimeToolTip()

	def __ShowThinToolTip(self, lines):
		self.__HideAllRowToolTips()
		tooltip = uiToolTip.ToolTip(COMMUNITY_SUB_TAB_TOOLTIP_MIN_W)
		for line in lines:
			tooltip.AutoAppendTextLine(line)
		tooltip.AlignTextLineHorizonalCenter()
		tooltip.ResizeToolTip()
		tooltip.ShowToolTip()
		self.iconToolTip = tooltip

	def __OnOverLocationButton(self):
		if not self.IsOnline():
			return
		lines = []
		channelFmt = getattr(uiScriptLocale, "COMMUNITY_CHANNEL_INFO", None)
		if channelFmt and self.channel > 0:
			lines.append(channelFmt % self.channel)
		mapFmt = getattr(uiScriptLocale, "COMMUNITY_MAP_INFO", None)
		mapName = GetCommunityMapName(self.mapIndex)
		if mapFmt and mapName:
			lines.append(mapFmt % mapName)
		if not lines:
			fallback = getattr(localeInfo, "CHANNEL_NOT_FIND_INFO", None)
			if fallback:
				lines.append(fallback)
		if lines:
			self.__ShowThinToolTip(lines)

	def __OnOverFavoriteButton(self):
		text = GetUiScriptLocaleText("COMMUNITY_FAVORITE_BUTTON")
		if text:
			self.__ShowThinToolTip((text,))

	def __OnOverPartyInviteButton(self):
		text = GetUiScriptLocaleText("COMMUNITY_PARTY_INVITE")
		if text:
			self.__ShowThinToolTip((text,))

	def __GetConnectionStateToolTipText(self):
		state = community.DISCONNECT
		if self.IsOnline():
			state = self.connectionState
		return GetConnectionStateLabel(state)

	def __OnOverStateIcon(self):
		text = self.__GetConnectionStateToolTipText()
		if text:
			self.__ShowThinToolTip((text,))

	def __ShowStatusMessageToolTip(self):
		if not self.statusMessage:
			return False
		msg = self.statusMessage.strip()
		if not msg:
			return False
		self.__HideAllRowToolTips()
		minW = max(COMMUNITY_SUB_TAB_TOOLTIP_MIN_W, min(280, len(msg) * 6 + 40))
		tooltip = uiToolTip.ToolTip(minW)
		tooltip.SetFollow(True)
		tooltip.AutoAppendTextLine(msg)
		tooltip.AlignTextLineHorizonalCenter()
		tooltip.ResizeToolTip()
		tooltip.ShowToolTip()
		self.iconToolTip = tooltip
		return True

	def __ShowOfflineTimeToolTip(self):
		if not app.ENABLE_MESSENGER_DETAILS:
			return False
		if self.IsOnline() or self.offlineTime == 0:
			return False
		offlineTime = app.GetGlobalTimeStamp() - self.offlineTime
		if offlineTime <= 60:
			return False
		offlineTimeText = localeInfo.MESSENGER_OFFLINE_TIME_TOOLTIP % (
			localeInfo.SecondToDHM(offlineTime))
		self.__HideAllRowToolTips()
		tooltip = uiToolTip.ToolTip(11 * len(offlineTimeText))
		tooltip.SetTitle(self.name)
		tooltip.AppendTextLine(offlineTimeText)
		tooltip.ResizeToolTip()
		tooltip.ShowToolTip()
		self.offlineTimeToolTip = tooltip
		return True

	def __OnOverName(self):
		if self.__ShowStatusMessageToolTip():
			return
		self.__ShowOfflineTimeToolTip()

	def __OnOutRowButtonToolTip(self):
		self.__HideAllRowToolTips()

	def __OnClickFavoriteButton(self):
		if not self.IsOnline():
			return
		handler = self.getParentEvent()
		if handler:
			handler.OnMemberFavoriteClick(self)

	def __OnClickPartyInviteButton(self):
		if not self.IsOnline():
			return
		handler = self.getParentEvent()
		if handler:
			handler.OnMemberPartyInviteClick(self)

	def __del__(self):
		ui.Window.__del__(self)

	def SetKey(self, key):
		self.key = key

	def IsSameKey(self, key):
		if self.key is not None and key is not None:
			try:
				if (int(self.key) & 0xFFFFFFFF) == (int(key) & 0xFFFFFFFF):
					return True
			except (TypeError, ValueError):
				pass
		if self.key == key:
			return True
		if isinstance(key, basestring) and self.name and key == self.name:
			return True
		if self.name and key == NameKeyToPid(self.name):
			return True
		return False

	def SetName(self, name):
		if name and not IsPlausibleMemberDisplayName(name):
			if IsPlausibleMemberDisplayName(self.name):
				return
			name = ""
		elif not name:
			name = ""
		else:
			name = name.strip()
		self.name = name
		self.__RefreshNameText()

	def SetLevel(self, level):
		self.level = level
		self.__RefreshNameText()

	def SetLocationInfo(self, channel, mapIndex):
		self.channel = channel
		self.mapIndex = mapIndex
		self.hasLocationInfo = True
		self.__RefreshRowButtons()

	def SetFavorite(self, isFavorite):
		self.isFavorite = isFavorite
		self.__RefreshRowButtons()

	def SetStatusMessage(self, statusMessage):
		self.statusMessage = statusMessage or ""

	def GetStatusMessage(self):
		return self.statusMessage

	def __RefreshNameText(self):
		if not self.name:
			return
		self.text.SetText(self.name)
		if self.IsOnline() and self.level > 0:
			fmt = getattr(localeInfo, "COMMUNITY_MEMBER_LEVEL", None)
			if fmt:
				levelText = fmt % self.level
			else:
				levelText = "(Sv. %d)" % self.level
			self.levelText.SetText(levelText)
			self.levelText.Show()
		else:
			self.levelText.Hide()

	if app.ENABLE_MESSENGER_DETAILS:
		def SetOfflineTime(self, offlineTime):
			self.offlineTime = offlineTime

		if app.ENABLE_MULTI_LANGUAGE_SYSTEM:
			def SetLanguage(self, country):
				pass

	def GetName(self):
		return self.name

	def GetKey(self):
		return self.key

	def GetStepWidth(self):
		return 0

	def IsOnline(self):
		return self.state == 1 and self.connectionState != community.DISCONNECT

	def __ApplyConnectionStateImage(self):
		self.image.LoadImage(GetConnectionStateImage(self.connectionState))

	def SetConnectionState(self, connectionState):
		self.connectionState = connectionState
		if connectionState == community.DISCONNECT:
			self.state = 0
		else:
			self.state = 1
		self.__ApplyConnectionStateImage()
		self.__RefreshNameText()
		self.__RefreshRowButtons()

	def Online(self, connectionState=community.CONNECT):
		self.state = 1
		self.connectionState = connectionState
		self.__ApplyConnectionStateImage()
		self.__RefreshNameText()
		self.__RefreshRowButtons()

	def Offline(self):
		self.state = 0
		self.connectionState = community.DISCONNECT
		self.image.LoadImage(COMMUNITY_STATE_OFFLINE)
		self.__RefreshNameText()
		self.__RefreshRowButtons()

	def __RefreshRowHighlight(self):
		if not self.renewalBackground:
			return
		if self.isSelected or self.isHovered:
			self.renewalBackground.LoadImage(COMMUNITY_MEMBER_INFO_BAR_HOVER)
		else:
			self.renewalBackground.LoadImage(COMMUNITY_MEMBER_INFO_BAR_BG)

	def Select(self):
		self.isSelected = True
		self.__RefreshRowHighlight()

	def UnSelect(self):
		self.isSelected = False
		self.__RefreshRowHighlight()

	def CanWhisper(self):
		return self.IsOnline()

	def CanRemove(self):
		return True

	def OnWhisper(self):
		if self.IsOnline():
			handler = self.getParentEvent()
			if handler and handler.whisperButtonEvent:
				handler.whisperButtonEvent(self.GetName())

	def OnRemove(self):
		removeName = self.GetName()
		if removeName:
			messenger.RemoveFriend(removeName)
		community.SendDeleteMember(removeName)
		return True

	def OnMouseLeftButtonDown(self):
		self.getParentEvent().OnSelectItem(self)

	def OnMouseLeftButtonDoubleClick(self):
		self.getParentEvent().OnDoubleClickItem(self)

	def OnMouseOverIn(self):
		self.isHovered = True
		self.__RefreshRowHighlight()

		if self.stateIconPicker and self.stateIconPicker.IsIn():
			return
		if self.nameHoverPicker and self.nameHoverPicker.IsIn():
			return

		if self.__ShowStatusMessageToolTip():
			return

		self.__ShowOfflineTimeToolTip()

	def OnMouseOverOut(self):
		self.isHovered = False
		self.__RefreshRowHighlight()
		self.__HideAllRowToolTips()

class CommunityFriendGroup(object):
	def __init__(self, getHandlerEvent, getListParentEvent):
		self.getHandlerEvent = getHandlerEvent
		self.getListParentEvent = getListParentEvent
		self.memberList = []

	def AppendMember(self, key, name):
		item = CommunityMemberItem(self.getHandlerEvent, self.getListParentEvent)
		item.SetKey(key)
		item.SetName(name)
		self.memberList.append(item)
		return item

	def RemoveMember(self, item):
		if not item:
			return
		item.Hide()
		for i in xrange(len(self.memberList)):
			if item == self.memberList[i]:
				del self.memberList[i]
				return

	def ClearMember(self):
		self.memberList = []

	def FindMember(self, key):
		for member in self.memberList:
			if member.IsSameKey(key):
				return member
		return None

	def FindMemberByName(self, name):
		if not name:
			return None
		for member in self.memberList:
			if member.GetName() == name:
				return member
		return None

	def MergeMemberData(self, target, source):
		if source.IsOnline():
			target.Online()
		if source.level > target.level:
			target.SetLevel(source.level)
		if source.hasLocationInfo:
			target.SetLocationInfo(source.channel, source.mapIndex)
		if source.isFavorite:
			target.SetFavorite(True)
		if source.statusMessage:
			target.SetStatusMessage(source.statusMessage)
		if app.ENABLE_MESSENGER_DETAILS and source.offlineTime:
			target.SetOfflineTime(source.offlineTime)

	def GetLoginMemberList(self):
		return filter(lambda m: m.IsOnline(), self.memberList)

	def GetLogoutMemberList(self):
		return filter(lambda m: not m.IsOnline(), self.memberList)

if app.ENABLE_MESSENGER_BLOCK:
	class CommunityBlockMemberItem(CommunityMemberItem):
		def __init__(self, getHandlerEvent, getListParentEvent):
			CommunityMemberItem.__init__(self, getHandlerEvent, getListParentEvent)
			self.locationButton.Hide()
			self.favoriteButton.Hide()
			self.partyInviteButton.Hide()
			self.unblockButton = CommunityMemberItem._CommunityMemberItem__CreateRowButton(
				self,
				COMMUNITY_ROW_BLOCK_UNBLOCK_X,
				COMMUNITY_ROW_BLOCK_UNBLOCK_Y,
				COMMUNITY_ROW_ICON_DELETE,
				COMMUNITY_ROW_ICON_DELETE_OVER,
				COMMUNITY_ROW_ICON_DELETE_CLICK,
			)
			self.unblockButton.SetOverEvent(ui.__mem_func__(self.__OnOverUnblockButton))
			self.unblockButton.SetOverOutEvent(ui.__mem_func__(self._OnOutRowButtonToolTip))
			self.unblockButton.SetEvent(ui.__mem_func__(self.__OnClickUnblockButton))
			self.nameHoverPicker.SetSize(
				COMMUNITY_ROW_BLOCK_UNBLOCK_X - COMMUNITY_ROW_NAME_X - 4,
				COMMUNITY_ROW_NAME_PICKER_H,
			)

		def _OnOutRowButtonToolTip(self):
			CommunityMemberItem._CommunityMemberItem__OnOutRowButtonToolTip(self)

		def __RefreshRowButtons(self):
			CommunityMemberItem._CommunityMemberItem__SetRowButtonEnabled(self, self.unblockButton, True)

		def __OnOverUnblockButton(self):
			text = GetUiScriptLocaleText("COMMUNITY_MESSENGER_BLOCK_DELETE")
			if text:
				CommunityMemberItem._CommunityMemberItem__ShowThinToolTip(self, (text,))

		def __OnClickUnblockButton(self):
			removeName = self.GetName()
			if not removeName:
				return
			handler = self.getParentEvent()
			if handler:
				handler.RemoveBlockMemberLocally(self)
			net.SendMessengerBlockRemovePacket(removeName, removeName)

		def OnRemove(self):
			removeName = self.GetName()
			if removeName:
				net.SendMessengerBlockRemovePacket(removeName, removeName)
			return True

	class CommunityBlockGroup(CommunityFriendGroup):
		def AppendMember(self, key, name):
			item = CommunityBlockMemberItem(self.getHandlerEvent, self.getListParentEvent)
			item.SetKey(key)
			item.SetName(name)
			self.memberList.append(item)
			return item

class CommunityRequestMemberItem(CommunityMemberItem):
	def __init__(self, getHandlerEvent, getListParentEvent):
		CommunityMemberItem.__init__(self, getHandlerEvent, getListParentEvent)
		self.favoriteButton.Hide()
		self.partyInviteButton.Hide()
		self.locationButton.SetOverEvent(ui.__mem_func__(self._OnOverLocationButton))
		self.locationButton.SetOverOutEvent(ui.__mem_func__(self._OnOutRowButtonToolTip))
		self.acceptButton = CommunityMemberItem._CommunityMemberItem__CreateRowButton(
			self,
			COMMUNITY_ROW_ICON_FAVORITE_X,
			COMMUNITY_ROW_ICON_Y,
			COMMUNITY_ROW_ICON_REQUEST_ACCEPT,
			COMMUNITY_ROW_ICON_REQUEST_ACCEPT_OVER,
			COMMUNITY_ROW_ICON_REQUEST_ACCEPT_CLICK,
		)
		self.denyButton = CommunityMemberItem._CommunityMemberItem__CreateRowButton(
			self,
			COMMUNITY_ROW_ICON_PARTY_X,
			COMMUNITY_ROW_ICON_Y,
			COMMUNITY_ROW_ICON_DELETE,
			COMMUNITY_ROW_ICON_DELETE_OVER,
			COMMUNITY_ROW_ICON_DELETE_CLICK,
		)
		self.acceptButton.SetOverEvent(ui.__mem_func__(self._OnOverAcceptButton))
		self.acceptButton.SetOverOutEvent(ui.__mem_func__(self._OnOutRowButtonToolTip))
		self.acceptButton.SetEvent(ui.__mem_func__(self.__OnClickAcceptButton))
		self.denyButton.SetOverEvent(ui.__mem_func__(self._OnOverDenyButton))
		self.denyButton.SetOverOutEvent(ui.__mem_func__(self._OnOutRowButtonToolTip))
		self.denyButton.SetEvent(ui.__mem_func__(self.__OnClickDenyButton))
		self.nameHoverPicker.SetSize(
			COMMUNITY_ROW_ICON_POS_X - COMMUNITY_ROW_NAME_X - 4,
			COMMUNITY_ROW_NAME_PICKER_H,
		)
		self.Online()

	def _OnOutRowButtonToolTip(self):
		CommunityMemberItem._CommunityMemberItem__OnOutRowButtonToolTip(self)

	def _OnOverLocationButton(self):
		CommunityMemberItem._CommunityMemberItem__OnOverLocationButton(self)

	def __RefreshRowButtons(self):
		if self.IsOnline():
			CommunityMemberItem._CommunityMemberItem__SetRowButtonEnabled(
				self, self.locationButton, self.hasLocationInfo and (self.channel > 0 or self.mapIndex != 0))
			CommunityMemberItem._CommunityMemberItem__SetRowButtonEnabled(self, self.acceptButton, True)
			CommunityMemberItem._CommunityMemberItem__SetRowButtonEnabled(self, self.denyButton, True)
		else:
			CommunityMemberItem._CommunityMemberItem__SetRowButtonEnabled(self, self.locationButton, False)
			CommunityMemberItem._CommunityMemberItem__SetRowButtonEnabled(self, self.acceptButton, False)
			CommunityMemberItem._CommunityMemberItem__SetRowButtonEnabled(self, self.denyButton, False)

	def _OnOverAcceptButton(self):
		text = GetUiScriptLocaleText("COMMUNITY_ACCEPT")
		if text:
			CommunityMemberItem._CommunityMemberItem__ShowThinToolTip(self, (text,))

	def _OnOverDenyButton(self):
		text = GetUiScriptLocaleText("COMMUNITY_MESSENGER_FRIEND_DELETE")
		if text:
			CommunityMemberItem._CommunityMemberItem__ShowThinToolTip(self, (text,))

	def __OnClickAcceptButton(self):
		removeName = self.GetName()
		if not removeName:
			return
		net.SendCommandPacket("/messenger_auth y " + removeName)
		handler = self.getParentEvent()
		if handler:
			handler.RemoveFriendRequest(removeName)

	def __OnClickDenyButton(self):
		removeName = self.GetName()
		if not removeName:
			return
		net.SendCommandPacket("/messenger_auth n " + removeName)
		handler = self.getParentEvent()
		if handler:
			handler.RemoveFriendRequest(removeName)

	def CanWhisper(self):
		return False

	def CanRemove(self):
		return True

	def OnRemove(self):
		self.__OnClickDenyButton()
		return True

class CommunityRequestGroup(CommunityFriendGroup):
	def AppendMember(self, key, name):
		item = CommunityRequestMemberItem(self.getHandlerEvent, self.getListParentEvent)
		item.SetKey(key)
		item.SetName(name)
		self.memberList.append(item)
		return item

if IsCommunityGuildRenewalEnabled():

	class CommunityGuildMemberItem(CommunityMemberItem):
		def __init__(self, getHandlerEvent, getListParentEvent):
			CommunityMemberItem.__init__(self, getHandlerEvent, getListParentEvent)
			self.locationButton.SetEvent(ui.__mem_func__(self.__OnClickLocationButton))
	
		def __OnClickLocationButton(self):
			if not self.IsOnline():
				return
			net.SendGuildUseSkillPacket(155, self.key)
	
		def CanRemove(self):
			for i in xrange(guild.ENEMY_GUILD_SLOT_MAX_COUNT):
				if guild.GetEnemyGuildName(i) != "":
					return False
			if guild.MainPlayerHasAuthority(guild.AUTH_REMOVE_MEMBER):
				if guild.IsMemberByName(self.name):
					return True
			return False
	
		def OnRemove(self):
			net.SendGuildRemoveMemberPacket(self.key)
			return True
	
		def OnWhisper(self):
			if self.IsOnline():
				handler = self.getParentEvent()
				if handler and handler.whisperButtonEvent:
					handler.whisperButtonEvent(self.GetName())

	class CommunityGuildGroup(CommunityFriendGroup):
		def AppendMember(self, key, name):
			item = CommunityGuildMemberItem(self.getHandlerEvent, self.getListParentEvent)
			item.SetKey(key)
			item.SetName(name)
			self.memberList.append(item)
			return item

	class CommunityGuildWindow(object):
		def __init__(self, owner):
			self.owner = owner
			self.whisperButtonEvent = lambda *arg: None
			self.listParent = None
			self.group = None
			self.showingItemList = []
			self.selectedItem = None
			self.startLine = 0
			self.scrollBar = None
			self.listBg = None
			self.listFrame = None
			self.listClipWindow = None
			self._scrollBarSize = 0
			self._uiReady = False
			self.questionDialog = None
	
			self.guildInviteButton = None
			self.guildBlockButton = None
			self.guildWhisperButton = None
			self.guildDeleteButton = None
	
			getHandler = ui.__mem_func__(self.GetMemberEventHandler)
			getListParent = ui.__mem_func__(self.GetListParent)
			self.group = CommunityGuildGroup(getHandler, getListParent)
	
		def Destroy(self):
			self._uiReady = False
			self.listParent = None
			self.showingItemList = []
			self.selectedItem = None
			if self.group:
				self.group.ClearMember()
			self.scrollBar = None
			self.listBg = None
			self.listFrame = None
			self.listClipWindow = None
	
		def SetWhisperButtonEvent(self, event):
			self.whisperButtonEvent = event
	
		def GetMemberEventHandler(self):
			return self
	
		def GetListParent(self):
			return self.listParent
	
		def BindWidgets(self, listParent, inviteBtn, blockBtn, whisperBtn, deleteBtn):
			self.listParent = listParent
			self.guildInviteButton = inviteBtn
			self.guildBlockButton = blockBtn
			self.guildWhisperButton = whisperBtn
			self.guildDeleteButton = deleteBtn
			self.__EnsureScrollBar()
			self.__EnsureListPanel()
			self.__BindActionButtons()
			if self.guildWhisperButton:
				self.guildWhisperButton.Disable()
			if self.guildDeleteButton:
				self.guildDeleteButton.Disable()
			self._uiReady = True
			self.OnRefreshList()
	
		def __BindActionButtons(self):
			if self.guildInviteButton:
				self.guildInviteButton.SetEvent(ui.__mem_func__(self.OnPressAddFriendButton))
			if self.guildBlockButton:
				self.guildBlockButton.SetEvent(ui.__mem_func__(self.OnPressAddBlockButton))
			if self.guildWhisperButton:
				self.guildWhisperButton.SetEvent(ui.__mem_func__(self.OnPressWhisperButton))
			if self.guildDeleteButton:
				self.guildDeleteButton.SetEvent(ui.__mem_func__(self.OnPressRemoveButton))
	
		def __EnsureScrollBar(self):
			if self.scrollBar or not self.listParent:
				return
			bar = ui.ScrollBar()
			bar.SetParent(self.listParent)
			bar.SetPosition(COMMUNITY_GUILD_SCROLL_X, COMMUNITY_GUILD_SCROLL_Y)
			bar.SetScrollBarSize(COMMUNITY_GUILD_SCROLL_SIZE)
			bar.SetScrollEvent(ui.__mem_func__(self.OnScroll))
			bar.Show()
			self.scrollBar = bar
	
		def __EnsureListPanel(self):
			if not self.listParent:
				return
			if not self.listBg:
				self.listBg = CommunityListPanel(
					COMMUNITY_GUILD_LIST_PANEL_W, COMMUNITY_GUILD_LIST_PANEL_H)
				self.listBg.SetParent(self.listParent)
				self.listBg.SetPosition(
					COMMUNITY_GUILD_LIST_PANEL_X, COMMUNITY_GUILD_LIST_PANEL_Y)
				self.listBg.Show()
	
				self.listFrame = ui.OutlineWindow()
				self.listFrame.SetParent(self.listParent)
				self.listFrame.MakeOutlineWindow(
					COMMUNITY_GUILD_LIST_FRAME_W, COMMUNITY_GUILD_LIST_PANEL_H)
				self.listFrame.SetPosition(
					COMMUNITY_GUILD_LIST_PANEL_X, COMMUNITY_GUILD_LIST_PANEL_Y)
				self.listFrame.Show()
				self.listFrame.SetTop()
	
			if not self.listClipWindow:
				self.listClipWindow = ui.Window()
				self.listClipWindow.AddFlag("not_pick")
				self.listClipWindow.SetParent(self.listParent)
				self.listClipWindow.Show()
	
			self.__UpdateListClipWindow()
	
		def __GetListPanelRect(self):
			if self.listFrame:
				panelX, panelY = self.listFrame.GetLocalPosition()
				panelW = self.listFrame.GetWidth()
				panelH = self.listFrame.GetHeight()
			else:
				panelX = COMMUNITY_GUILD_LIST_PANEL_X
				panelY = COMMUNITY_GUILD_LIST_PANEL_Y
				panelW = COMMUNITY_GUILD_LIST_PANEL_W
				panelH = COMMUNITY_GUILD_LIST_PANEL_H
			return panelX, panelY, panelW, panelH
	
		def __UpdateListClipWindow(self):
			if not self.listClipWindow:
				return
			panelX, panelY, panelW, panelH = self.__GetListPanelRect()
			clipY = panelY + COMMUNITY_LIST_ROW_Y_OFFSET
			clipH = max(1, panelH - COMMUNITY_LIST_ROW_Y_OFFSET)
			self.listClipWindow.SetPosition(panelX, clipY)
			self.listClipWindow.SetSize(panelW, clipH)
	
		def __GetListMetrics(self):
			panelX, panelY, panelW, panelH = self.__GetListPanelRect()
			startY = panelY + COMMUNITY_LIST_ROW_Y_OFFSET
			bottomY = COMMUNITY_GUILD_INFO_BTN_Y - COMMUNITY_GUILD_LIST_BOTTOM_PAD
			if bottomY <= startY:
				bottomY = panelY + panelH
	
			yProbe = startY
			visibleCount = 0
			while yProbe + COMMUNITY_LIST_ROW_HEIGHT <= bottomY:
				visibleCount += 1
				yProbe += COMMUNITY_LIST_LINE_HEIGHT
	
			return {
				"visibleCount": visibleCount,
				"startY": startY,
				"bottomY": bottomY,
				"listX": COMMUNITY_GUILD_LIST_X,
				"scrollY": COMMUNITY_GUILD_SCROLL_Y,
				"scrollSize": COMMUNITY_GUILD_SCROLL_SIZE,
			}
	
		def __SyncScrollBar(self, metrics, preservePos=False):
			if not self.scrollBar:
				return
			scrollSize = metrics["scrollSize"]
			if scrollSize != self._scrollBarSize:
				self._scrollBarSize = scrollSize
				self.scrollBar.SetScrollBarSize(scrollSize)
			self.scrollBar.SetPosition(COMMUNITY_GUILD_SCROLL_X, metrics["scrollY"])
	
		def __ApplyMemberRowClip(self, item):
			if app.ENABLE_CLIP_MASK and hasattr(item, "SetClippingMaskWindow") and self.listClipWindow:
				item.SetClippingMaskWindow(self.listClipWindow)
	
		def __FindMember(self, key, name=None):
			if not self.group:
				return None
			member = self.group.FindMember(key)
			if member:
				return member
			if name:
				return self.group.FindMemberByName(name)
			if isinstance(key, basestring):
				return self.group.FindMemberByName(key)
			return None
	
		def __IsLocalGuildMember(self, name=None, key=None):
			try:
				myName = player.GetName()
			except:
				return False
			if not myName:
				return False
			if name and name == myName:
				return True
			if isinstance(key, basestring) and key == myName:
				return True
			return False
	
		def __PurgeLocalGuildMember(self):
			if not self.group:
				return
			try:
				myName = player.GetName()
			except:
				return
			if not myName:
				return
			member = self.group.FindMemberByName(myName)
			if not member:
				return
			if self.selectedItem == member:
				member.UnSelect()
				self.selectedItem = None
				if self.guildWhisperButton:
					self.guildWhisperButton.Disable()
				if self.guildDeleteButton:
					self.guildDeleteButton.Disable()
			self.group.RemoveMember(member)
	
		def __AddMember(self, key, name):
			if not self.group:
				return None
			if not name and isinstance(key, basestring):
				name = key
			if self.__IsLocalGuildMember(name, key):
				return None
			member = self.__FindMember(key, name)
			if not member:
				member = self.group.AppendMember(key, name or key)
				if self._uiReady:
					self.OnSelectItem(None)
			elif name:
				member.SetName(name)
				member.SetKey(key)
			return member
	
		def __SyncGuildMemberMeta(self):
			if not self.group:
				return
			try:
				count = guild.GetMemberCount()
			except:
				return
			for i in xrange(count):
				try:
					pid, name, grade, race, level, offer, general = guild.GetMemberData(i)
				except:
					continue
				if not name:
					continue
				if self.__IsLocalGuildMember(name, pid):
					continue
				member = self.group.FindMemberByName(name)
				if not member:
					member = self.group.FindMember(pid)
				if not member:
					member = self.__AddMember(pid, name)
				if not member:
					continue
				if IsPlausibleMemberDisplayName(name):
					member.SetName(name)
				if pid >= 0:
					member.SetKey(pid)
				if level > 0:
					member.SetLevel(level)
			self.__PurgeLocalGuildMember()
			self.__SyncGuildMemberFavoriteState()
			self.__SyncGuildMemberLocationFromFriends()
	
		def __SyncGuildMemberFavoriteState(self):
			if not self.group or not self.owner or not self.owner.messengerWindow:
				return
			friendGroup = self.owner.messengerWindow.groupList[FRIEND]
			if not friendGroup:
				return
			for member in self.group.memberList:
				friendMember = friendGroup.FindMemberByName(member.GetName())
				if friendMember:
					member.SetFavorite(friendMember.isFavorite)
	
		def __SyncGuildMemberLocationFromFriends(self):
			if not self.group or not self.owner or not self.owner.messengerWindow:
				return
			friendGroup = self.owner.messengerWindow.groupList[FRIEND]
			if not friendGroup:
				return
			for member in self.group.memberList:
				if member.hasLocationInfo:
					continue
				friendMember = friendGroup.FindMemberByName(member.GetName())
				if not friendMember:
					friendMember = friendGroup.FindMember(member.GetKey())
				if friendMember and friendMember.hasLocationInfo:
					member.SetLocationInfo(friendMember.channel, friendMember.mapIndex)
	
		def SetGuildMemberLocationInfo(self, pid, channel, mapIndex):
			if not self.group:
				return
			member = self.__FindMember(pid)
			if not member:
				member = self.group.FindMember(pid)
			if member and (channel > 0 or mapIndex != 0):
				member.SetLocationInfo(channel, mapIndex)
	
		def UpdateGuildMemberLocationInfo(self, pid, channel, mapIndex):
			self.SetGuildMemberLocationInfo(pid, channel, mapIndex)
	
		def ClearMember(self):
			if self.group:
				self.group.ClearMember()
			self.showingItemList = []
			self.selectedItem = None
			self.startLine = 0
			if self._uiReady:
				self.OnRefreshList()
	
		if app.ENABLE_MESSENGER_DETAILS:
			def OnLogin(self, key, offlineTime=0, country=""):
				if self.__IsLocalGuildMember(key, key):
					return
				member = self.__AddMember(key, key)
				if not member:
					return
				member.SetName(key)
				member.SetOfflineTime(offlineTime)
				member.Online()
				if self._uiReady:
					self.OnRefreshList()
	
			def OnLogout(self, key, offlineTime=0, country=""):
				if self.__IsLocalGuildMember(key, key):
					return
				member = self.__AddMember(key, key)
				if not member:
					return
				member.SetName(key)
				member.SetOfflineTime(offlineTime)
				member.Offline()
				if self._uiReady:
					self.OnRefreshList()
		else:
			def OnLogin(self, key, name=None):
				if not name:
					name = key
				if self.__IsLocalGuildMember(name, key):
					return
				member = self.__AddMember(key, name)
				if not member:
					return
				member.SetName(name)
				member.Online()
				if self._uiReady:
					self.OnRefreshList()
	
			def OnLogout(self, key, name=None):
				if not name:
					name = key
				if self.__IsLocalGuildMember(name, key):
					return
				member = self.__AddMember(key, name)
				if not member:
					return
				member.SetName(name)
				member.Offline()
				if self._uiReady:
					self.OnRefreshList()
	
		def OnRemoveMember(self, key):
			if not self.group:
				return
			member = self.__FindMember(key)
			if member and self.selectedItem == member:
				member.UnSelect()
				self.selectedItem = None
				if self.guildWhisperButton:
					self.guildWhisperButton.Disable()
				if self.guildDeleteButton:
					self.guildDeleteButton.Disable()
			if member:
				self.group.RemoveMember(member)
			if self._uiReady:
				self.OnRefreshList()
	
		def OnRefreshList(self):
			if not self._uiReady or not self.listParent or not self.group:
				return
	
			for item in self.showingItemList:
				item.Hide()
	
			self.__SyncGuildMemberMeta()
			self.__PurgeLocalGuildMember()
			self.showingItemList = []
			self.startLine = 0
			self._scrollBarSize = 0
			if self.scrollBar:
				self.scrollBar.SetPos(0, False)
	
			loginMemberList = self.group.GetLoginMemberList()
			logoutMemberList = self.group.GetLogoutMemberList()
			for member in loginMemberList:
				if not self.__IsLocalGuildMember(member.GetName(), member.GetKey()):
					self.showingItemList.append(member)
			for member in logoutMemberList:
				if not self.__IsLocalGuildMember(member.GetName(), member.GetKey()):
					self.showingItemList.append(member)
	
			self.__LocateMember()
	
		def __LocateMember(self):
			self.__LocateMemberImpl(syncScrollBar=True)
	
		def __LocateMemberFromScroll(self):
			self.__LocateMemberImpl(syncScrollBar=False)
	
		def __LocateMemberImpl(self, syncScrollBar=True):
			if not self.listParent:
				return
	
			self.__UpdateListClipWindow()
			metrics = self.__GetListMetrics()
			visibleCount = metrics["visibleCount"]
			startY = metrics["startY"]
			bottomY = metrics["bottomY"]
			listX = metrics["listX"]
	
			if syncScrollBar:
				self.__SyncScrollBar(metrics, preservePos=False)
	
			scrollLineCount = max(0, len(self.showingItemList) - visibleCount)
			if self.startLine > scrollLineCount:
				self.startLine = scrollLineCount
	
			if self.scrollBar:
				if scrollLineCount <= 0:
					self.scrollBar.Hide()
					self.startLine = 0
				else:
					self.scrollBar.SetMiddleBarSize(
						float(visibleCount) / float(len(self.showingItemList)))
					self.scrollBar.SetTop()
					self.scrollBar.Show()
	
			map(ui.Window.Hide, self.showingItemList)
	
			yPos = startY
			for item in self.showingItemList[self.startLine:]:
				if yPos + COMMUNITY_LIST_ROW_HEIGHT > bottomY:
					break
				item.SetParent(self.listParent)
				item.SetPosition(listX + item.GetStepWidth(), yPos)
				self.__ApplyMemberRowClip(item)
				item.SetTop()
				item.Show()
				yPos += COMMUNITY_LIST_LINE_HEIGHT
	
		def OnSelectItem(self, item):
			if self.selectedItem and item != self.selectedItem:
				self.selectedItem.UnSelect()
			self.selectedItem = item
			if self.selectedItem:
				self.selectedItem.Select()
				if self.guildWhisperButton:
					if self.selectedItem.CanWhisper():
						self.guildWhisperButton.Enable()
					else:
						self.guildWhisperButton.Disable()
				if self.guildDeleteButton:
					if self.selectedItem.CanRemove():
						self.guildDeleteButton.Enable()
					else:
						self.guildDeleteButton.Disable()
			else:
				if self.guildWhisperButton:
					self.guildWhisperButton.Disable()
				if self.guildDeleteButton:
					self.guildDeleteButton.Disable()
	
		def OnDoubleClickItem(self, item):
			if self.selectedItem and self.selectedItem.IsOnline():
				self.OnPressWhisperButton()
	
		def OnScroll(self):
			if not self.scrollBar or not self.listParent:
				return
			metrics = self.__GetListMetrics()
			visibleCount = metrics["visibleCount"]
			scrollLineCount = len(self.showingItemList) - visibleCount
			if scrollLineCount <= 0:
				return
			startLine = int(scrollLineCount * self.scrollBar.GetPos())
			if startLine == self.startLine:
				return
			self.startLine = startLine
			self.__LocateMemberFromScroll()
	
		def __IsMouseOverScrollBar(self):
			if not self.scrollBar or not self.scrollBar.IsShow():
				return False
			return IsMouseInWindowRect(self.scrollBar)
	
		def __IsMouseOverListArea(self):
			if self.__IsMouseOverScrollBar():
				return True
			if IsMouseInWindowRect(self.listClipWindow):
				return True
			if IsMouseInWindowRect(self.listFrame):
				return True
			for item in self.showingItemList[self.startLine:]:
				if item.IsShow() and item.IsIn():
					return True
			return False
	
		def OnMouseWheelScrollLines(self, lineDelta):
			if not self._uiReady or not self.listParent:
				return False
			if not self.__IsMouseOverListArea():
				return False
			if not self.scrollBar or not self.scrollBar.IsShow():
				return False
	
			metrics = self.__GetListMetrics()
			visibleCount = metrics["visibleCount"]
			scrollLineCount = len(self.showingItemList) - visibleCount
			if scrollLineCount <= 0:
				return False
	
			newStartLine = self.startLine + lineDelta
			if newStartLine < 0:
				newStartLine = 0
			elif newStartLine > scrollLineCount:
				newStartLine = scrollLineCount
	
			if newStartLine == self.startLine:
				return False
	
			self.startLine = newStartLine
			self.scrollBar.SetPos(float(newStartLine) / float(scrollLineCount), False)
			self.__LocateMemberFromScroll()
			return True
	
		def OnPressWhisperButton(self):
			if self.selectedItem:
				self.selectedItem.OnWhisper()
	
		def OnPressRemoveButton(self):
			if self.selectedItem and self.selectedItem.CanRemove():
				self.questionDialog = uiCommon.QuestionDialog()
				self.questionDialog.SetText(localeInfo.MESSENGER_DO_YOU_DELETE)
				self.questionDialog.SetAcceptEvent(ui.__mem_func__(self.OnRemove))
				self.questionDialog.SetCancelEvent(ui.__mem_func__(self.OnCloseQuestionDialog))
				self.questionDialog.Open()
	
		def OnCloseQuestionDialog(self):
			if self.questionDialog:
				self.questionDialog.Close()
			self.questionDialog = None
			return True
	
		def OnRemove(self):
			if self.selectedItem and self.selectedItem.CanRemove():
				self.selectedItem.OnRemove()
				if self.group:
					self.group.RemoveMember(self.selectedItem)
				self.selectedItem.UnSelect()
				self.selectedItem = None
				self.OnRefreshList()
			self.OnCloseQuestionDialog()
	
		def OnPressAddFriendButton(self):
			name = ""
			if self.selectedItem:
				name = self.selectedItem.GetName()
			friendNameBoard = uiCommon.InputDialog()
			friendNameBoard.SetTitle(localeInfo.MESSENGER_ADD_FRIEND)
			if name:
				friendNameBoard.inputValue.SetText(name)
			friendNameBoard.SetAcceptEvent(ui.__mem_func__(self.OnAddFriend))
			friendNameBoard.SetCancelEvent(ui.__mem_func__(self.OnCancelAddFriend))
			friendNameBoard.Open()
			self.friendNameBoard = friendNameBoard
	
		def OnAddFriend(self):
			text = self.friendNameBoard.GetText()
			if text:
				community.SendRequestFriend(text)
			self.friendNameBoard.Close()
			self.friendNameBoard = None
			return True
	
		def OnCancelAddFriend(self):
			self.friendNameBoard.Close()
			self.friendNameBoard = None
			return True
	
		def OnPressAddBlockButton(self):
			if not app.ENABLE_MESSENGER_BLOCK:
				return
			name = ""
			if self.selectedItem:
				name = self.selectedItem.GetName()
			blockNameBoard = uiCommon.InputDialog()
			blockNameBoard.SetTitle(localeInfo.MESSENGER_ADD_BLOCK_FRIEND)
			if name:
				blockNameBoard.inputValue.SetText(name)
			blockNameBoard.SetAcceptEvent(ui.__mem_func__(self.OnBlockFriend))
			blockNameBoard.SetCancelEvent(ui.__mem_func__(self.OnCancelBlockFriend))
			blockNameBoard.Open()
			self.blockFriendNameBoard = blockNameBoard
	
		def OnBlockFriend(self):
			text = self.blockFriendNameBoard.GetText()
			if text:
				community.SendAddBlock(text)
			self.blockFriendNameBoard.Close()
			self.blockFriendNameBoard = None
			return True
	
		def OnCancelBlockFriend(self):
			self.blockFriendNameBoard.Close()
			self.blockFriendNameBoard = None
			return True
	
		def OnMemberPartyInviteClick(self, member):
			if not member or not member.IsOnline():
				return
			name = member.GetName()
			if not name:
				return
			if hasattr(community, "CanPartyInviteTime") and not community.CanPartyInviteTime():
				return
			if hasattr(community, "SendPartyInvite"):
				community.SendPartyInvite(name)
	
		def OnMemberFavoriteClick(self, member):
			if not member or not member.IsOnline():
				return
			if self.owner and self.owner.messengerWindow:
				self.owner.messengerWindow.OnMemberFavoriteClick(member)

class CommunityMessengerWindow(object):
	"""Inner messenger panel (official uiCommunity.MessengerWindow)."""

	def __init__(self, owner):
		self.owner = owner
		self.interface = None
		self.whisperButtonEvent = lambda *arg: None

		self.listParent = None
		self.groupList = [None, None, None, None]
		self.showingItemList = []
		self.selectedItem = None
		self.startLine = 0
		self.currentSubTab = MESSENGER_VIEW_FRIEND
		self.familyGroup = None

		self.messengerViewWindow = None
		self.messengerUpperOutlineWindow = None
		self.familyInfoBar = None
		self.scrollBar = None
		self.messengerToolbarBg = None
		self.messengerPart3ListBg = None
		self.messengerPart3ListFrame = None
		self.messengerListClipWindow = None
		self._messengerScrollBarSize = 0

		self.friendTabButton = None
		self.blockTabButton = None
		self.requestTabButton = None
		self.friendTabImage = None
		self.blockTabImage = None
		self.requestTabImage = None
		self.requestTabTwinkle = None
		self._isRequestTwinkleOn = False
		self._lastRequestTwinkleTime = 0.0
		self._friendActionButtonsOverlay = False
		self._subTabPickers = []
		self._subTabPickersReady = False

		self.friendInviteButton = None
		self.friendBlockButton = None
		self.friendWhisperButton = None
		self.friendDeleteButton = None

		self.questionDialog = None
		self.favoriteQuestionDialog = None
		self.favoriteQuestionMember = None
		self.friendNameBoard = None
		self.blockFriendNameBoard = None
		self._engineActive = False
		self._uiReady = False
		self._pendingFriendStatusByKey = {}
		self.statusMessageCommentSlot = None
		self.statusMessageCommentText = None
		self._pendingLover = None
		self._pendingLovePoint = None
		self._pendingLoverLogin = False
		self.requestGroup = None

		self.__ActivateEngine()

	def __ActivateEngine(self):
		if self._engineActive:
			return
		self._engineActive = True
		messenger.SetMessengerHandler(self)
		community.SetMessengerHandler(self)
		self.__AddGroup()

	def __DeactivateEngine(self):
		if not self._engineActive:
			return
		messenger.SetMessengerHandler(None)
		community.ClearMessengerHandler()
		self._engineActive = False

	def __del__(self):
		self.__DeactivateEngine()

	def Destroy(self):
		self.__DeactivateEngine()
		self._uiReady = False
		self.listParent = None
		self.groupList = [None, None, None, None]
		self.showingItemList = []
		self.selectedItem = None
		self.familyGroup = None
		self.familyInfoBar = None
		self.requestGroup = None
		self._pendingLover = None
		self._pendingLovePoint = None
		self._pendingLoverLogin = False
		self._pendingFriendStatusByKey = {}
		self.statusMessageCommentSlot = None
		self.statusMessageCommentText = None
		self._isRequestTwinkleOn = False
		self._lastRequestTwinkleTime = 0.0

	def BindInterface(self, interface):
		self.interface = interface

	def SetWhisperButtonEvent(self, event):
		self.whisperButtonEvent = event

	def __GetFriendActionButtonParent(self):
		if self.owner:
			return self.owner
		return self.messengerViewWindow

	def __GetFriendActionButtonPosition(self, index):
		x = COMMUNITY_FRIEND_ACTION_X_BASE + (COMMUNITY_FRIEND_ACTION_INTERVAL_X * index)
		y = COMMUNITY_FRIEND_ACTION_Y
		parent = self.__GetFriendActionButtonParent()
		if parent != self.messengerViewWindow and self.messengerViewWindow:
			mvX, mvY = self.messengerViewWindow.GetLocalPosition()
			boardX, boardY = 0, 0
			if self.owner and self.owner.board:
				boardX, boardY = self.owner.board.GetLocalPosition()
			x += boardX + mvX
			y += boardY + mvY
		return x, y

	def __GetFriendActionButtonLocalPosition(self, index):
		x = COMMUNITY_FRIEND_ACTION_X_BASE + (COMMUNITY_FRIEND_ACTION_INTERVAL_X * index)
		y = COMMUNITY_FRIEND_ACTION_Y
		return x, y

	def __SyncFriendActionButtonLayout(self):
		parent = self.__GetFriendActionButtonParent()
		if not parent:
			return
		shouldShow = self.__IsFriendActionBarAllowed()
		for index, attrName in enumerate((
			"friendInviteButton",
			"friendBlockButton",
			"friendWhisperButton",
			"friendDeleteButton",
		)):
			button = getattr(self, attrName, None)
			if not button or not getattr(button, "_communityOverlayAction", False):
				continue
			x, y = self.__GetFriendActionButtonPosition(index)
			if getattr(button, "_communityActionParent", None) != parent:
				button.SetParent(parent)
				button._communityActionParent = parent
			button.AddFlag("float")
			button.SetPosition(x, y)
			if shouldShow:
				button.Show()

	def __IsFriendActionBarAllowed(self):
		if not self.owner:
			return True
		return self.owner.currentViewState == COMMUNITY_VIEW_MESSENGER

	def __CreateMessengerOverlayActionButton(self, x, y, defaultPath, overPath, downPath):
		button = ui.Button()
		button.AddFlag("float")
		parent = self.__GetFriendActionButtonParent()
		if not parent:
			parent = self.messengerViewWindow
		button.SetParent(parent)
		button.SetPosition(x, y)
		button.SetUpVisual(defaultPath)
		button.SetOverVisual(overPath)
		button.SetDownVisual(downPath)
		button.SetDisableVisual(defaultPath)
		button._communityDefaultPath = defaultPath
		button._communityOverPath = overPath
		button._communityDownPath = downPath
		button._communityOverlayAction = True
		button.Show()
		return button

	def __SetMessengerOverlayActionButtonEnabled(self, button, isEnabled):
		if not button:
			return
		defaultPath = button._communityDefaultPath
		overPath = button._communityOverPath
		downPath = button._communityDownPath
		button.SetUpVisual(defaultPath)
		button.SetDisableVisual(defaultPath)
		button._communityActionDisabled = not isEnabled
		if isEnabled:
			button.SetOverVisual(overPath)
			button.SetDownVisual(downPath)
		else:
			button.SetOverVisual(defaultPath)
			button.SetDownVisual(defaultPath)
		button.Enable()
		if self.owner and button in (self.friendWhisperButton, self.friendDeleteButton):
			self.__RefreshActionBarTooltips()

	def __BuildFriendActionButtons(self):
		if not self.messengerViewWindow:
			return

		specs = (
			("friendInviteButton",
			 COMMUNITY_ACTION_ICON_FRIEND_INVITE,
			 COMMUNITY_ACTION_ICON_FRIEND_INVITE_OVER,
			 COMMUNITY_ACTION_ICON_FRIEND_INVITE_CLICK),
			("friendBlockButton",
			 COMMUNITY_ACTION_ICON_BLOCK,
			 COMMUNITY_ACTION_ICON_BLOCK_OVER,
			 COMMUNITY_ACTION_ICON_BLOCK_CLICK),
			("friendWhisperButton",
			 COMMUNITY_ACTION_ICON_WHISPER,
			 COMMUNITY_ACTION_ICON_WHISPER_OVER,
			 COMMUNITY_ACTION_ICON_WHISPER_CLICK),
			("friendDeleteButton",
			 COMMUNITY_ACTION_ICON_DELETE,
			 COMMUNITY_ACTION_ICON_DELETE_OVER,
			 COMMUNITY_ACTION_ICON_DELETE_CLICK),
		)

		for index, (attrName, defaultPath, overPath, downPath) in enumerate(specs):
			x, y = self.__GetFriendActionButtonPosition(index)
			button = getattr(self, attrName, None)
			if button and getattr(button, "_communityOverlayAction", False):
				self.__SyncFriendActionButtonLayout()
				continue
			if button:
				button.Hide()
				button.AddFlag("not_pick")
			button = self.__CreateMessengerOverlayActionButton(
				x, y, defaultPath, overPath, downPath)
			setattr(self, attrName, button)

		self._friendActionButtonsOverlay = True
		self.__SyncFriendActionButtonLayout()
		self.RaiseFriendActionButtonsToFront()

	def __OptionalGetChild(self, getChild, name):
		try:
			return getChild(name)
		except:
			return None

	def __EnsureFamilyInfoBar(self):
		if self.familyInfoBar or not self.listParent:
			return

		bar = CommunityFamilyInfoBar()
		bar.SetParent(self.listParent)
		bar.SetPosition(COMMUNITY_FAMILY_INFO_BAR_X, COMMUNITY_FAMILY_INFO_BAR_Y)
		bar.SetHandler(self)
		bar.Show()
		self.familyInfoBar = bar

	def __EnsureScrollBar(self):
		if self.scrollBar or not self.listParent:
			return

		bar = ui.ScrollBar()
		bar.SetParent(self.listParent)
		bar.SetPosition(COMMUNITY_SCROLL_X, COMMUNITY_SCROLL_Y)
		bar.SetScrollBarSize(COMMUNITY_SCROLL_SIZE)
		bar.SetScrollEvent(ui.__mem_func__(self.OnScroll))
		bar.Show()
		self.scrollBar = bar

	def __FixMessengerImagesForLocale(self):
		if localeInfo.IsARABIC():
			return
		if self.friendTabImage:
			self.friendTabImage.LoadImage(COMMUNITY_SUB_TAB_FRIEND_IMG)
		if self.blockTabImage:
			self.blockTabImage.LoadImage(COMMUNITY_SUB_TAB_BLOCK_IMG)
		if self.requestTabImage:
			self.requestTabImage.LoadImage(COMMUNITY_SUB_TAB_REQUEST_IMG)

	def BindScript(self, getChild):
		self.messengerViewWindow = getChild("messenger_view_window")
		self.listParent = self.messengerViewWindow
		self.messengerUpperOutlineWindow = getChild("messenger_upper_outline_window")
		self.scrollBar = self.__OptionalGetChild(getChild, "messenger_scroll_bar")
		self.messengerToolbarBg = self.__OptionalGetChild(getChild, "messenger_toolbar_bg")

		self.friendTabButton = getChild("messenger_friend_member_tab_button")
		self.blockTabButton = getChild("messenger_block_member_tab_button")
		self.requestTabButton = getChild("messenger_request_member_tab_button")
		self.friendTabImage = getChild("messenger_friend_member_tab_image")
		self.blockTabImage = getChild("messenger_block_member_tab_image")
		self.requestTabImage = getChild("messenger_request_member_tab_image")
		self.requestTabTwinkle = getChild("community_request_button_twinkle")
		if self.requestTabTwinkle:
			self.requestTabTwinkle.Hide()

		self.friendInviteButton = getChild("friend_invite")
		self.friendBlockButton = getChild("friend_block")
		self.friendWhisperButton = getChild("friend_whisper")
		self.friendDeleteButton = getChild("friend_delete")
		self._uiReady = True

		self.__FixMessengerImagesForLocale()
		self.__BuildFriendActionButtons()
		self.__EnsureFamilyInfoBar()
		self.__EnsureScrollBar()

		if self.scrollBar:
			self.scrollBar.SetScrollEvent(ui.__mem_func__(self.OnScroll))

		self.friendTabButton.SetEvent(ui.__mem_func__(self.OnSelectFriendTab))
		self.blockTabButton.SetEvent(ui.__mem_func__(self.OnSelectBlockTab))
		self.requestTabButton.SetEvent(ui.__mem_func__(self.OnSelectRequestTab))

		self.friendInviteButton.SetEvent(ui.__mem_func__(self.OnPressAddFriendButton))
		self.friendBlockButton.SetEvent(ui.__mem_func__(self.OnPressAddBlockButton))
		self.friendWhisperButton.SetEvent(ui.__mem_func__(self.OnPressWhisperButton))
		self.friendDeleteButton.SetEvent(ui.__mem_func__(self.OnPressRemoveButton))

		if getattr(self.friendWhisperButton, "_communityOverlayAction", False):
			self.__SetMessengerOverlayActionButtonEnabled(self.friendWhisperButton, False)
		else:
			self.friendWhisperButton.Disable()
		if getattr(self.friendDeleteButton, "_communityOverlayAction", False):
			self.__SetMessengerOverlayActionButtonEnabled(self.friendDeleteButton, False)
		else:
			self.friendDeleteButton.Disable()

		self.__BindMessengerToolTips()
		self.__RefreshActionBarTooltips()

		self.__ActivateEngine()
		self.__SetupLayout()
		self.__EnsurePart3ListThinboard()
		messenger.RefreshGuildMember()
		self.__SyncFriendsFromEngine()
		if app.ENABLE_MESSENGER_BLOCK:
			self.__SyncBlocksFromEngine()
		self.__FlushPendingLoverState()
		self.OnSelectFriendTab()

	def __FlushPendingLoverState(self):
		if self._pendingLover:
			name, lovePoint = self._pendingLover
			self._pendingLover = None
			self.__ApplyLover(name, lovePoint)
		if self._pendingLovePoint is not None:
			lovePoint = self._pendingLovePoint
			self._pendingLovePoint = None
			if self.familyGroup:
				lover = self.familyGroup.GetLover()
				if lover:
					lover.SetLovePoint(lovePoint)
		if self._pendingLoverLogin:
			self._pendingLoverLogin = False
			if self.familyGroup:
				lover = self.familyGroup.GetLover()
				if lover:
					lover.Online()

	def GetMemberEventHandler(self):
		return self

	def GetFamilyLovePoint(self):
		if not self.familyGroup:
			return 0
		lover = self.familyGroup.GetLover()
		if not lover:
			return 0
		return getattr(lover, "lovePoint", 0)

	def __BindMessengerToolTips(self):
		if not self.owner:
			return
		helper = self.owner._uiToolTipHelper
		self.__EnsureSubTabTooltipPickers(helper)
		helper.Bind(
			self.friendInviteButton,
			GetUiScriptLocaleText("MESSENGER_ADD_FRIEND"),
			COMMUNITY_TOOLTIP_W_ACTION)
		helper.Bind(
			self.friendBlockButton,
			GetUiScriptLocaleText("MESSENGER_BLOCK"),
			COMMUNITY_TOOLTIP_W_ACTION)
		if self.friendWhisperButton:
			helper.BindShowToolTip(
				self.friendWhisperButton,
				GetUiScriptLocaleText("COMMUNITY_MESSENGER_WHISPER"),
				COMMUNITY_TOOLTIP_W_ACTION)

	def __EnsureSubTabTooltipPickers(self, helper):
		if self._subTabPickersReady or not helper or not self.messengerViewWindow:
			return

		specs = (
			(COMMUNITY_SUB_TAB_PICKER_X[0], ui.__mem_func__(self.OnSelectFriendTab),
			 GetUiScriptLocaleText("COMMUNITY_MESSENGER_SUB_TAB_FRIEND")),
			(COMMUNITY_SUB_TAB_PICKER_X[1], ui.__mem_func__(self.OnSelectBlockTab),
			 GetUiScriptLocaleText("COMMUNITY_MESSENGER_SUB_TAB_BLOCK")),
			(COMMUNITY_SUB_TAB_PICKER_X[2], ui.__mem_func__(self.OnSelectRequestTab),
			 GetUiScriptLocaleText("COMMUNITY_MESSENGER_SUB_TAB_REQUEST")),
		)

		for x, clickHandler, tooltipText in specs:
			picker = ui.Window()
			picker.SetParent(self.messengerViewWindow)
			picker.SetPosition(x, COMMUNITY_SUB_TAB_IMAGE_Y)
			picker.SetSize(COMMUNITY_SUB_TAB_PICKER_W, COMMUNITY_SUB_TAB_PICKER_H)
			picker.SetOnMouseLeftButtonUpEvent(clickHandler)
			helper.Bind(picker, tooltipText)
			picker.Show()
			picker.SetTop()
			self._subTabPickers.append(picker)

		self._subTabPickersReady = True
		self.RaiseFriendActionButtonsToFront()

	def __RefreshActionBarTooltips(self):
		if not self.owner:
			return
		helper = self.owner._uiToolTipHelper
		if self.friendWhisperButton:
			helper.BindShowToolTip(
				self.friendWhisperButton,
				GetUiScriptLocaleText("COMMUNITY_MESSENGER_WHISPER"),
				COMMUNITY_TOOLTIP_W_ACTION)
		if not self.friendDeleteButton:
			return
		deleteText = GetUiScriptLocaleText("COMMUNITY_MESSENGER_FRIEND_DELETE")
		if self.currentSubTab == MESSENGER_VIEW_BLOCK:
			deleteText = GetUiScriptLocaleText("COMMUNITY_MESSENGER_BLOCK_DELETE")
		helper.Bind(
			self.friendDeleteButton, deleteText, COMMUNITY_TOOLTIP_W_ACTION)

	def GetSelf(self):
		return self.owner

	def GetListParent(self):
		if self.listParent:
			return self.listParent
		return self.owner

	def __SyncFriendsFromEngine(self):
		group = self.groupList[FRIEND]
		if not group:
			return
		self.__NormalizeFriendMemberKeys()
		friendNames = community.GetFriendsNameByTuple()
		if not friendNames:
			friendNames = ()

		nameSet = set()
		for i in xrange(len(friendNames)):
			name = friendNames[i]
			if IsPlausibleMemberDisplayName(name):
				nameSet.add(name)

		changed = False
		keptMembers = []
		for member in group.memberList:
			name = member.GetName()
			if name and name not in nameSet:
				changed = True
				continue
			keptMembers.append(member)
		if changed:
			group.memberList = keptMembers

		for name in nameSet:
			key = NameKeyToPid(name)
			member = group.FindMember(key)
			if not member:
				member = group.FindMemberByName(name)
				if member:
					member.SetKey(key)
			if member:
				continue
			member = group.AppendMember(key, name)
			member.SetName(name)
			member.Offline()
			changed = True

		if self.__DedupeFriendMembers():
			changed = True

		if changed and self.listParent:
			self.RefreshWindow()
		self.__FlushPendingFriendStatus()

	def __FlushPendingFriendStatus(self):
		group = self.groupList[FRIEND]
		if not group or not self._pendingFriendStatusByKey:
			return
		for member in group.memberList:
			key = member.GetKey()
			if key not in self._pendingFriendStatusByKey:
				continue
			member.SetStatusMessage(self._pendingFriendStatusByKey[key])
			del self._pendingFriendStatusByKey[key]

	def __NormalizeFriendMemberKeys(self):
		group = self.groupList[FRIEND]
		if not group:
			return
		for member in group.memberList:
			name = member.GetName()
			if not name:
				continue
			member.SetKey(NameKeyToPid(name))

	def __DedupeFriendMembers(self):
		group = self.groupList[FRIEND]
		if not group:
			return False
		byName = {}
		newList = []
		changed = False
		for member in group.memberList:
			name = member.GetName()
			if not name:
				newList.append(member)
				continue
			if name not in byName:
				byName[name] = member
				member.SetKey(NameKeyToPid(name))
				newList.append(member)
				continue
			changed = True
			existing = byName[name]
			group.MergeMemberData(existing, member)
			existing.SetKey(NameKeyToPid(name))
		if changed:
			group.memberList = newList
		return changed

	def __FindMemberByName(self, group, name):
		if not group or not name:
			return None
		findByName = getattr(group, "FindMemberByName", None)
		if findByName:
			return findByName(name)
		for member in group.memberList:
			if member.GetName() == name:
				return member
		return None

	def __FindGroupMember(self, group, key, name=None):
		if not group:
			return None
		normKey = key
		if name:
			normKey = NameKeyToPid(name)
		elif isinstance(key, basestring):
			normKey = NameKeyToPid(key)
		member = group.FindMember(normKey)
		if member:
			return member
		lookupName = name
		if not lookupName and isinstance(key, basestring):
			lookupName = key
		if lookupName:
			member = self.__FindMemberByName(group, lookupName)
			if member:
				member.SetKey(normKey)
				return member
		return None

	if app.ENABLE_MESSENGER_BLOCK:
		def __SyncBlocksFromEngine(self):
			group = self.groupList[BLOCK]
			if not group:
				return
			blockNames = community.GetBlocksNameByTuple()
			if not blockNames:
				blockNames = ()

			nameSet = set()
			for i in xrange(len(blockNames)):
				name = blockNames[i]
				if IsPlausibleMemberDisplayName(name):
					nameSet.add(name)

			changed = False
			keptMembers = []
			for member in group.memberList:
				name = member.GetName()
				if name and name not in nameSet:
					changed = True
					continue
				keptMembers.append(member)
			if changed:
				group.memberList = keptMembers

			for i in xrange(len(blockNames)):
				name = blockNames[i]
				if not IsPlausibleMemberDisplayName(name):
					continue
				key = NameKeyToPid(name)
				if group.FindMember(key):
					continue
				member = group.AppendMember(key, name)
				member.SetName(name)
				member.Offline()
				changed = True

			if changed and self.listParent:
				self.RefreshWindow()

	def SyncFriendsFromEngine(self):
		self.__SyncFriendsFromEngine()

	if app.ENABLE_MESSENGER_BLOCK:
		def SyncBlocksFromEngine(self):
			self.__SyncBlocksFromEngine()

	def __ApplyUpperOutlineBorderOnly(self):
		outline = self.messengerUpperOutlineWindow
		if not outline:
			return
		outline.SetPosition(
			COMMUNITY_UPPER_OUTLINE_X, COMMUNITY_UPPER_OUTLINE_Y)
		outline.SetSize(
			COMMUNITY_UPPER_OUTLINE_W, COMMUNITY_UPPER_OUTLINE_H)
		center = getattr(outline, "center_img", None)
		if center:
			center.Hide()
		outline.Show()

	def __SetupLayout(self):
		self.__ApplyUpperOutlineBorderOnly()
		if COMMUNITY_HIDE_TOOLBAR_BG and self.messengerToolbarBg:
			self.messengerToolbarBg.Hide()

		self.__FixMessengerImagesForLocale()
		if self.friendTabImage:
			self.friendTabImage.SetPosition(10, COMMUNITY_SUB_TAB_IMAGE_Y)
		if self.blockTabImage:
			self.blockTabImage.SetPosition(10, COMMUNITY_SUB_TAB_IMAGE_Y)
			self.blockTabImage.Hide()
		if self.requestTabImage:
			self.requestTabImage.SetPosition(10, COMMUNITY_SUB_TAB_IMAGE_Y)
			self.requestTabImage.Hide()
		for tabImage in (self.friendTabImage, self.blockTabImage, self.requestTabImage):
			if tabImage:
				tabImage.AddFlag("not_pick")

		friendActionButtons = (
			self.friendInviteButton,
			self.friendBlockButton,
			self.friendWhisperButton,
			self.friendDeleteButton,
		)
		if not self._friendActionButtonsOverlay:
			self.__BuildFriendActionButtons()
		else:
			self.RaiseFriendActionButtonsToFront()

		if self.messengerUpperOutlineWindow:
			self.messengerUpperOutlineWindow.AddFlag("not_pick")

		owner = self.owner
		if owner:
			owner.RaiseMyInfoChromeToFront()

	def __EnsurePart3ListThinboard(self):
		if not self.listParent:
			return
		if not self.messengerPart3ListBg:
			self.messengerPart3ListBg = CommunityListPanel(COMMUNITY_LIST_PANEL_W, COMMUNITY_LIST_PANEL_H)
			self.messengerPart3ListBg.SetParent(self.listParent)
			self.messengerPart3ListBg.SetPosition(COMMUNITY_LIST_PANEL_X, COMMUNITY_LIST_PANEL_Y)
			self.messengerPart3ListBg.Show()

			self.messengerPart3ListFrame = ui.OutlineWindow()
			self.messengerPart3ListFrame.SetParent(self.listParent)
			self.messengerPart3ListFrame.MakeOutlineWindow(COMMUNITY_LIST_FRAME_W, COMMUNITY_LIST_PANEL_H)
			self.messengerPart3ListFrame.SetPosition(COMMUNITY_LIST_PANEL_X, COMMUNITY_LIST_PANEL_Y)
			self.messengerPart3ListFrame.Show()
			self.messengerPart3ListFrame.SetTop()

		if not self.messengerListClipWindow:
			self.messengerListClipWindow = ui.Window()
			self.messengerListClipWindow.AddFlag("not_pick")
			self.messengerListClipWindow.SetParent(self.listParent)
			self.messengerListClipWindow.Show()

		self.__UpdateMessengerListClipWindow()
		self.RaiseFriendActionButtonsToFront()

	def __GetStatusCommentReserve(self):
		return 0

	def __ApplyPendingFriendStatus(self, member):
		if not member:
			return
		key = member.GetKey()
		if key not in self._pendingFriendStatusByKey:
			return
		member.SetStatusMessage(self._pendingFriendStatusByKey[key])
		del self._pendingFriendStatusByKey[key]

	def __GetMessengerListPanelRect(self):
		frame = self.messengerPart3ListFrame
		if frame:
			panelX, panelY = frame.GetLocalPosition()
			panelW = frame.GetWidth()
			panelH = frame.GetHeight()
		else:
			panelX = COMMUNITY_LIST_PANEL_X
			panelY = COMMUNITY_LIST_PANEL_Y
			panelW = COMMUNITY_LIST_PANEL_W
			panelH = COMMUNITY_LIST_PANEL_H
		return panelX, panelY, panelW, panelH

	def __UpdateMessengerListClipWindow(self):
		if not self.messengerListClipWindow:
			return
		panelX, panelY, panelW, panelH = self.__GetMessengerListPanelRect()
		clipY = panelY + COMMUNITY_LIST_ROW_Y_OFFSET
		commentReserve = self.__GetStatusCommentReserve()
		clipH = max(1, panelH - COMMUNITY_LIST_ROW_Y_OFFSET - commentReserve)
		self.messengerListClipWindow.SetPosition(panelX, clipY)
		self.messengerListClipWindow.SetSize(panelW, clipH)

	def __GetMessengerListMetrics(self):
		panelX, panelY, panelW, panelH = self.__GetMessengerListPanelRect()
		commentReserve = self.__GetStatusCommentReserve()
		startY = panelY + COMMUNITY_LIST_ROW_Y_OFFSET
		bottomY = panelY + panelH - commentReserve
		viewportH = max(1, bottomY - startY)

		visibleCount = 0
		yProbe = startY
		while yProbe + COMMUNITY_LIST_ROW_HEIGHT <= bottomY:
			visibleCount += 1
			yProbe += COMMUNITY_LIST_LINE_HEIGHT
		visibleCount = max(1, visibleCount)

		return {
			"startY": startY,
			"bottomY": bottomY,
			"listX": COMMUNITY_LIST_X,
			"visibleCount": visibleCount,
			"scrollY": panelY,
			"scrollSize": viewportH,
		}

	def __SyncMessengerScrollBar(self, metrics, preservePos=True):
		if not self.scrollBar:
			return

		scrollSize = metrics["scrollSize"]
		scrollY = metrics["scrollY"]
		savedPos = self.scrollBar.GetPos() if preservePos else 0.0

		self.scrollBar.SetPosition(COMMUNITY_SCROLL_X, scrollY)

		if self._messengerScrollBarSize != scrollSize:
			self._messengerScrollBarSize = scrollSize
			self.scrollBar.SetScrollBarSize(scrollSize)
			if preservePos:
				self.scrollBar.SetPos(savedPos, False)

	def __ApplyMemberRowClip(self, item):
		if not self.messengerListClipWindow:
			return
		if app.ENABLE_CLIP_MASK and hasattr(item, "SetClippingMaskWindow"):
			item.SetClippingMaskWindow(self.messengerListClipWindow)

	def __UpdateSubTabImages(self):
		if self.currentSubTab == MESSENGER_VIEW_FRIEND:
			if self.friendTabImage:
				self.friendTabImage.Show()
			if self.blockTabImage:
				self.blockTabImage.Hide()
			if self.requestTabImage:
				self.requestTabImage.Hide()
		elif self.currentSubTab == MESSENGER_VIEW_BLOCK:
			if self.friendTabImage:
				self.friendTabImage.Hide()
			if self.blockTabImage:
				self.blockTabImage.Show()
			if self.requestTabImage:
				self.requestTabImage.Hide()
		else:
			if self.friendTabImage:
				self.friendTabImage.Hide()
			if self.blockTabImage:
				self.blockTabImage.Hide()
			if self.requestTabImage:
				self.requestTabImage.Show()

	def __AddGroup(self):
		getHandler = ui.__mem_func__(self.GetMemberEventHandler)
		getListParent = ui.__mem_func__(self.GetListParent)
		getSelf = ui.__mem_func__(self.GetSelf)

		if not self.groupList[FRIEND]:
			self.groupList[FRIEND] = CommunityFriendGroup(getHandler, getListParent)
		if not self.groupList[GUILD]:
			self.groupList[GUILD] = None

		if app.ENABLE_MESSENGER_GM and not self.groupList[GM]:
			self.groupList[GM] = uimessenger.MessengerGMGroup(getSelf)

		if app.ENABLE_MESSENGER_BLOCK and not self.groupList[BLOCK]:
			self.groupList[BLOCK] = CommunityBlockGroup(getHandler, getListParent)

		if not self.requestGroup:
			self.requestGroup = CommunityRequestGroup(getHandler, getListParent)

	def RaiseFriendActionButtonsToFront(self):
		if not self.__IsFriendActionBarAllowed():
			return
		self.__SyncFriendActionButtonLayout()
		# Sub-tab band .sub is full width; action icons must render above it.
		for tabImage in (self.friendTabImage, self.blockTabImage, self.requestTabImage):
			if tabImage and tabImage.IsShow():
				tabImage.SetTop()

		for tabButton in (self.friendTabButton, self.blockTabButton, self.requestTabButton):
			if tabButton:
				tabButton.SetTop()

		if self.requestTabTwinkle and self.requestTabTwinkle.IsShow():
			self.requestTabTwinkle.SetTop()

		for picker in self._subTabPickers:
			if picker:
				picker.SetTop()

		for button in (
			self.friendInviteButton,
			self.friendBlockButton,
			self.friendWhisperButton,
			self.friendDeleteButton,
		):
			if button and button.IsShow():
				button.SetTop()
		self.__RefreshActionBarTooltips()

	def SetFriendActionButtonsVisible(self, visible):
		if visible and not self.__IsFriendActionBarAllowed():
			visible = False
		for button in (
			self.friendInviteButton,
			self.friendBlockButton,
			self.friendWhisperButton,
			self.friendDeleteButton,
		):
			if not button:
				continue
			if visible:
				button.Show()
			else:
				button.Hide()
		if visible:
			self.RaiseFriendActionButtonsToFront()

	def __ShouldShowRequestTwinkle(self):
		if self.currentSubTab == MESSENGER_VIEW_REQUEST:
			return False
		if not self.requestGroup or len(self.requestGroup.memberList) <= 0:
			return False
		owner = self.owner
		if owner and getattr(owner, "currentViewState", COMMUNITY_VIEW_MESSENGER) != COMMUNITY_VIEW_MESSENGER:
			return False
		return True

	def __RefreshRequestTwinkle(self):
		if not self.requestTabTwinkle:
			return
		if self.__ShouldShowRequestTwinkle():
			self._isRequestTwinkleOn = True
			self._lastRequestTwinkleTime = 0.0
			self.requestTabTwinkle.Show()
		else:
			self._isRequestTwinkleOn = False
			self._lastRequestTwinkleTime = 0.0
			self.requestTabTwinkle.Hide()

	def OnUpdate(self):
		if not self._isRequestTwinkleOn or not self.requestTabTwinkle:
			return
		if not self.__ShouldShowRequestTwinkle():
			self.__RefreshRequestTwinkle()
			return
		currentTime = app.GetTime()
		if currentTime - self._lastRequestTwinkleTime < REQUEST_TAB_TWINKLE_INTERVAL:
			return
		self._lastRequestTwinkleTime = currentTime
		if self.requestTabTwinkle.IsShow():
			self.requestTabTwinkle.Hide()
		else:
			self.requestTabTwinkle.Show()

	def AddFriendRequest(self, name, level=0, channel=0, mapIndex=0):
		if not name:
			return False
		if not self.requestGroup:
			self.__AddGroup()
		if not self.requestGroup:
			return False
		name = name.strip()
		if not IsPlausibleMemberDisplayName(name):
			return False
		if self.requestGroup.FindMemberByName(name):
			return False
		key = NameKeyToPid(name)
		member = self.requestGroup.AppendMember(key, name)
		member.SetName(name)
		member.SetKey(key)
		member.Online()
		if level > 0:
			member.SetLevel(level)
		if channel > 0 or mapIndex != 0:
			member.SetLocationInfo(channel, mapIndex)
		member.__RefreshRowButtons()
		self.__RefreshRequestTwinkle()
		if self._uiReady:
			if self.currentSubTab == MESSENGER_VIEW_REQUEST:
				self.OnRefreshList()
		notifyText = getattr(localeInfo, "COMMUNITY_REQUEST_FRIEND", None)
		if notifyText:
			chat.AppendChat(chat.CHAT_TYPE_INFO, notifyText % name)
		return True

	def RemoveFriendRequest(self, name):
		if not name or not self.requestGroup:
			return
		member = self.requestGroup.FindMemberByName(name.strip())
		if not member:
			member = self.requestGroup.FindMember(NameKeyToPid(name))
		if not member:
			return
		if self.selectedItem == member:
			member.UnSelect()
			self.selectedItem = None
		self.requestGroup.RemoveMember(member)
		self.__RefreshRequestTwinkle()
		if self._uiReady:
			self.OnRefreshList()

	def DeleteRequestMember(self, pid):
		if not self.requestGroup:
			return
		member = self.requestGroup.FindMember(pid)
		if member:
			self.RemoveFriendRequest(member.GetName())

	def __FilterMemberItems(self, itemList):
		return itemList

	def __HidePreviousShowingItems(self):
		for item in self.showingItemList:
			item.Hide()

	def __LocateMember(self):
		self.__LocateMemberImpl(syncScrollBar=True)

	def __LocateMemberFromScroll(self):
		self.__LocateMemberImpl(syncScrollBar=False)

	def __LocateMemberImpl(self, syncScrollBar=True):
		if not self.listParent:
			return

		self.__UpdateMessengerListClipWindow()
		metrics = self.__GetMessengerListMetrics()
		visibleCount = metrics["visibleCount"]
		startY = metrics["startY"]
		bottomY = metrics["bottomY"]
		listX = metrics["listX"]

		if syncScrollBar:
			self.__SyncMessengerScrollBar(metrics, preservePos=False)

		scrollLineCount = max(0, len(self.showingItemList) - visibleCount)
		if self.startLine > scrollLineCount:
			self.startLine = scrollLineCount

		if self.scrollBar:
			if scrollLineCount <= 0:
				self.scrollBar.Hide()
				self.startLine = 0
			else:
				self.scrollBar.SetMiddleBarSize(float(visibleCount) / float(len(self.showingItemList)))
				self.scrollBar.SetTop()
				self.scrollBar.Show()

		map(ui.Window.Hide, self.showingItemList)

		yPos = startY
		for item in self.showingItemList[self.startLine:]:
			if yPos + COMMUNITY_LIST_ROW_HEIGHT > bottomY:
				break
			item.SetParent(self.listParent)
			item.SetPosition(listX + item.GetStepWidth(), yPos)
			self.__ApplyMemberRowClip(item)
			item.SetTop()
			item.Show()
			yPos += COMMUNITY_LIST_LINE_HEIGHT

		self.RaiseFriendActionButtonsToFront()

	def RefreshWindow(self, is_member_sorting=False):
		if not self._uiReady:
			return
		self.__DedupeFriendMembers()
		self.__FlushPendingFriendStatus()
		self.OnRefreshList()
		self.RaiseFriendActionButtonsToFront()
		owner = self.owner
		if owner:
			owner.RaiseMyInfoChromeToFront()

	def OnRefreshList(self):
		if not self._uiReady or not self.listParent:
			return

		self.__HidePreviousShowingItems()
		self.showingItemList = []
		self.startLine = 0
		self._messengerScrollBarSize = 0
		if self.scrollBar:
			self.scrollBar.SetPos(0, False)

		if self.currentSubTab == MESSENGER_VIEW_FRIEND:
			group = self.groupList[FRIEND]
			if group:
				loginMemberList = group.GetLoginMemberList()
				logoutMemberList = group.GetLogoutMemberList()
				for member in loginMemberList:
					self.showingItemList.append(member)
				for member in logoutMemberList:
					self.showingItemList.append(member)

			self.showingItemList = self.__FilterMemberItems(self.showingItemList)

		elif self.currentSubTab == MESSENGER_VIEW_BLOCK and app.ENABLE_MESSENGER_BLOCK:
			group = self.groupList[BLOCK]
			if group:
				for member in group.GetLoginMemberList():
					self.showingItemList.append(member)
				for member in group.GetLogoutMemberList():
					self.showingItemList.append(member)

		elif self.currentSubTab == MESSENGER_VIEW_REQUEST:
			if self.requestGroup:
				for member in self.requestGroup.memberList:
					self.showingItemList.append(member)

		self.__LocateMember()
		self.__RefreshRequestTwinkle()
		self.SetFriendActionButtonsVisible(True)
		self.RaiseFriendActionButtonsToFront()

	def OnSelectFriendTab(self):
		self.currentSubTab = MESSENGER_VIEW_FRIEND
		self.friendTabButton.Down()
		self.blockTabButton.SetUp()
		self.requestTabButton.SetUp()
		self.__UpdateSubTabImages()
		self.SetFriendActionButtonsVisible(True)
		self.RaiseFriendActionButtonsToFront()
		self.OnSelectItem(None)
		self.__RefreshActionBarTooltips()
		self.OnRefreshList()

	def OnSelectBlockTab(self):
		self.currentSubTab = MESSENGER_VIEW_BLOCK
		self.friendTabButton.SetUp()
		self.blockTabButton.Down()
		self.requestTabButton.SetUp()
		self.__UpdateSubTabImages()
		self.SetFriendActionButtonsVisible(True)
		self.RaiseFriendActionButtonsToFront()
		self.OnSelectItem(None)
		self.__RefreshActionBarTooltips()
		self.OnRefreshList()

	def OnSelectRequestTab(self):
		self.currentSubTab = MESSENGER_VIEW_REQUEST
		self.friendTabButton.SetUp()
		self.blockTabButton.SetUp()
		self.requestTabButton.Down()
		self.__UpdateSubTabImages()
		self.SetFriendActionButtonsVisible(True)
		self.RaiseFriendActionButtonsToFront()
		self.OnSelectItem(None)
		self.__RefreshActionBarTooltips()
		self.OnRefreshList()

	def OnSelectItem(self, item):
		if self.selectedItem and item != self.selectedItem:
			self.selectedItem.UnSelect()
			self.selectedItem.__HideAllRowToolTips()
		self.selectedItem = item
		if self.selectedItem:
			self.selectedItem.Select()
			if self.friendWhisperButton:
				canWhisper = self.selectedItem.CanWhisper()
				if getattr(self.friendWhisperButton, "_communityOverlayAction", False):
					self.__SetMessengerOverlayActionButtonEnabled(
						self.friendWhisperButton, canWhisper)
				elif canWhisper:
					self.friendWhisperButton.Enable()
				else:
					self.friendWhisperButton.Disable()
			if self.friendDeleteButton:
				canRemove = self.selectedItem.CanRemove()
				if getattr(self.friendDeleteButton, "_communityOverlayAction", False):
					self.__SetMessengerOverlayActionButtonEnabled(
						self.friendDeleteButton, canRemove)
				elif canRemove:
					self.friendDeleteButton.Enable()
				else:
					self.friendDeleteButton.Disable()
		else:
			if self.friendWhisperButton:
				if getattr(self.friendWhisperButton, "_communityOverlayAction", False):
					self.__SetMessengerOverlayActionButtonEnabled(
						self.friendWhisperButton, False)
				else:
					self.friendWhisperButton.Disable()
			if self.friendDeleteButton:
				if getattr(self.friendDeleteButton, "_communityOverlayAction", False):
					self.__SetMessengerOverlayActionButtonEnabled(
						self.friendDeleteButton, False)
				else:
					self.friendDeleteButton.Disable()

	def OnDoubleClickItem(self, item):
		if self.selectedItem and self.selectedItem.IsOnline():
			self.OnPressWhisperButton()

	def OnScroll(self):
		if not self.scrollBar or not self.listParent:
			return
		metrics = self.__GetMessengerListMetrics()
		visibleCount = metrics["visibleCount"]
		scrollLineCount = len(self.showingItemList) - visibleCount
		if scrollLineCount <= 0:
			return
		startLine = int(scrollLineCount * self.scrollBar.GetPos())
		if startLine == self.startLine:
			return
		self.startLine = startLine
		self.__LocateMemberFromScroll()

	def __IsMouseOverMessengerScrollBar(self):
		if not self.scrollBar or not self.scrollBar.IsShow():
			return False
		return IsMouseInWindowRect(self.scrollBar)

	def __IsMouseOverMessengerListArea(self):
		if self.__IsMouseOverMessengerScrollBar():
			return True
		if IsMouseInWindowRect(self.messengerListClipWindow):
			return True
		if IsMouseInWindowRect(self.messengerPart3ListFrame):
			return True
		for item in self.showingItemList[self.startLine:]:
			if item.IsShow() and item.IsIn():
				return True
		return False

	def OnMouseWheelScrollLines(self, lineDelta):
		if not self._uiReady or not self.listParent:
			return False
		if not self.__IsMouseOverMessengerListArea():
			return False
		if not self.scrollBar or not self.scrollBar.IsShow():
			return False

		metrics = self.__GetMessengerListMetrics()
		visibleCount = metrics["visibleCount"]
		scrollLineCount = len(self.showingItemList) - visibleCount
		if scrollLineCount <= 0:
			return False

		newStartLine = self.startLine + lineDelta
		if newStartLine < 0:
			newStartLine = 0
		elif newStartLine > scrollLineCount:
			newStartLine = scrollLineCount

		if newStartLine == self.startLine:
			return False

		self.startLine = newStartLine
		self.scrollBar.SetPos(float(newStartLine) / float(scrollLineCount), False)
		self.__LocateMemberFromScroll()
		return True

	def OnPressWhisperButton(self):
		button = self.friendWhisperButton
		if button and getattr(button, "_communityOverlayAction", False):
			if getattr(button, "_communityActionDisabled", False):
				return
		if not self.selectedItem or not self.selectedItem.CanWhisper():
			return
		self.selectedItem.OnWhisper()

	def OnPressRemoveButton(self):
		button = self.friendDeleteButton
		if button and getattr(button, "_communityOverlayAction", False):
			if getattr(button, "_communityActionDisabled", False):
				return
		if self.selectedItem and self.selectedItem.CanRemove():
			self.questionDialog = uiCommon.QuestionDialog()
			self.questionDialog.SetText(localeInfo.MESSENGER_DO_YOU_DELETE)
			self.questionDialog.SetAcceptEvent(ui.__mem_func__(self.OnRemove))
			self.questionDialog.SetCancelEvent(ui.__mem_func__(self.OnCloseQuestionDialog))
			self.questionDialog.Open()

	def OnCloseQuestionDialog(self):
		if self.questionDialog:
			self.questionDialog.Close()
		self.questionDialog = None
		return True

	def OnRemove(self):
		if self.selectedItem and self.selectedItem.CanRemove():
			for group in self.groupList:
				if group:
					group.RemoveMember(self.selectedItem)
			self.selectedItem.OnRemove()
			self.selectedItem.UnSelect()
			self.selectedItem = None
			self.OnRefreshList()
		self.OnCloseQuestionDialog()

	def OnPressAddFriendButton(self):
		friendNameBoard = uiCommon.InputDialog()
		friendNameBoard.SetTitle(localeInfo.MESSENGER_ADD_FRIEND)
		friendNameBoard.SetAcceptEvent(ui.__mem_func__(self.OnAddFriend))
		friendNameBoard.SetCancelEvent(ui.__mem_func__(self.OnCancelAddFriend))
		friendNameBoard.Open()
		self.friendNameBoard = friendNameBoard

	def OnAddFriend(self):
		text = self.friendNameBoard.GetText()
		if text:
			community.SendRequestFriend(text)
		self.friendNameBoard.Close()
		self.friendNameBoard = None
		return True

	def OnCancelAddFriend(self):
		self.friendNameBoard.Close()
		self.friendNameBoard = None
		return True

	def OnPressAddBlockButton(self):
		if not app.ENABLE_MESSENGER_BLOCK:
			return
		blockNameBoard = uiCommon.InputDialog()
		blockNameBoard.SetTitle(localeInfo.MESSENGER_ADD_BLOCK_FRIEND)
		blockNameBoard.SetAcceptEvent(ui.__mem_func__(self.OnBlockFriend))
		blockNameBoard.SetCancelEvent(ui.__mem_func__(self.OnCancelBlockFriend))
		blockNameBoard.Open()
		self.blockFriendNameBoard = blockNameBoard

	def OnBlockFriend(self):
		text = self.blockFriendNameBoard.GetText()
		if text:
			community.SendAddBlock(text)
		self.blockFriendNameBoard.Close()
		self.blockFriendNameBoard = None
		return True

	def OnCancelBlockFriend(self):
		self.blockFriendNameBoard.Close()
		self.blockFriendNameBoard = None
		return True

	def __AddList(self, groupIndex, key, name):
		if groupIndex < 0 or groupIndex >= len(self.groupList):
			return None
		if not self.groupList[groupIndex]:
			self.__AddGroup()
		group = self.groupList[groupIndex]
		if not group:
			return None
		if not name and isinstance(key, basestring):
			name = key
		displayName = NormalizeCommunityMemberDisplayName(name, key)
		normKey = NormalizeCommunityMemberKey(groupIndex, key, displayName or name)
		member = self.__FindGroupMember(group, normKey, displayName or name)
		if not member:
			if not displayName and groupIndex == FRIEND:
				return None
			member = group.AppendMember(normKey, displayName or key)
			if self._uiReady:
				self.OnSelectItem(None)
		elif displayName:
			member.SetName(displayName)
			member.SetKey(normKey)
		elif name and not IsPlausibleMemberDisplayName(name):
			member.SetKey(normKey)
		if groupIndex == FRIEND:
			self.__DedupeFriendMembers()
			self.__ApplyPendingFriendStatus(member)
		return member

	def OnRemoveList(self, groupIndex, key):
		if groupIndex == GUILD:
			if IsCommunityGuildRenewalEnabled() and self.owner and self.owner.guildWindow:
				self.owner.guildWindow.OnRemoveMember(key)
			return
		if groupIndex < 0 or groupIndex >= len(self.groupList):
			return
		group = self.groupList[groupIndex]
		if group:
			member = self.__FindGroupMember(group, key)
			if member and self.selectedItem == member:
				member.UnSelect()
				self.selectedItem = None
				if self.friendWhisperButton:
					self.friendWhisperButton.Disable()
				if self.friendDeleteButton:
					self.friendDeleteButton.Disable()
			group.RemoveMember(member)
		self.OnRefreshList()

	def OnRemoveAllList(self, groupIndex):
		if groupIndex == GUILD:
			self.ClearGuildMember()
			return
		if groupIndex < 0 or groupIndex >= len(self.groupList):
			return
		group = self.groupList[groupIndex]
		if group:
			group.ClearMember()
		self.OnRefreshList()

	def ClearGuildMember(self):
		if IsCommunityGuildRenewalEnabled() and self.owner and self.owner.guildWindow:
			self.owner.guildWindow.ClearMember()

	if app.ENABLE_MESSENGER_DETAILS:
		def OnLogin(self, groupIndex, key, offlineTime=0, country=""):
			if groupIndex == GUILD:
				if IsCommunityGuildRenewalEnabled() and self.owner and self.owner.guildWindow:
					self.owner.guildWindow.OnLogin(key, offlineTime, country)
				return
			if groupIndex != FRIEND:
				return
			name = key
			member = self.__AddList(groupIndex, key, name)
			if not member:
				return
			member.SetName(name)
			member.SetOfflineTime(offlineTime)
			if app.ENABLE_MULTI_LANGUAGE_SYSTEM:
				member.SetLanguage(country)
			member.Online()
			if groupIndex == FRIEND:
				self.__DedupeFriendMembers()
			self.OnRefreshList()

		def OnLogout(self, groupIndex, key, offlineTime=0, country=""):
			if groupIndex == GUILD:
				if IsCommunityGuildRenewalEnabled() and self.owner and self.owner.guildWindow:
					self.owner.guildWindow.OnLogout(key, offlineTime, country)
				return
			if groupIndex != FRIEND:
				return
			name = key
			member = self.__AddList(groupIndex, key, name)
			if not member:
				return
			member.SetName(name)
			member.SetOfflineTime(offlineTime)
			if app.ENABLE_MULTI_LANGUAGE_SYSTEM:
				member.SetLanguage(country)
			member.Offline()
			if groupIndex == FRIEND:
				self.__DedupeFriendMembers()
			self.OnRefreshList()
	else:
		def OnLogin(self, groupIndex, key, name=None):
			if groupIndex == GUILD:
				if IsCommunityGuildRenewalEnabled() and self.owner and self.owner.guildWindow:
					self.owner.guildWindow.OnLogin(key, name)
				return
			if groupIndex != FRIEND:
				return
			if not name:
				name = key
			member = self.__AddList(groupIndex, key, name)
			if not member:
				return
			member.SetName(name)
			member.Online()
			if groupIndex == FRIEND:
				self.__DedupeFriendMembers()
			self.OnRefreshList()

		def OnLogout(self, groupIndex, key, name=None):
			if groupIndex == GUILD:
				if IsCommunityGuildRenewalEnabled() and self.owner and self.owner.guildWindow:
					self.owner.guildWindow.OnLogout(key, name)
				return
			if groupIndex != FRIEND:
				return
			if not name:
				name = key
			member = self.__AddList(groupIndex, key, name)
			if not member:
				return
			member.SetName(name)
			member.Offline()
			if groupIndex == FRIEND:
				self.__DedupeFriendMembers()
			self.OnRefreshList()

	def AddOnlineFriendFromBridge(self, pid, name, offlineTime=0, country=""):
		if not IsPlausibleMemberDisplayName(name):
			return
		member = self.__AddList(FRIEND, pid, name)
		if not member:
			return
		member.SetName(name)
		if app.ENABLE_MESSENGER_DETAILS:
			member.SetOfflineTime(offlineTime)
			if app.ENABLE_MULTI_LANGUAGE_SYSTEM:
				member.SetLanguage(country)
		member.Online(community.CONNECT)
		self.__DedupeFriendMembers()
		if self._uiReady:
			self.OnRefreshList()

	def ChangeFriendConnectionState(self, pid, connectionState):
		group = self.groupList[FRIEND]
		if not group:
			return
		member = self.__FindGroupMember(group, pid)
		if not member:
			return
		if connectionState != community.DISCONNECT and member.state == 0:
			return
		member.SetConnectionState(connectionState)
		if self._uiReady:
			self.OnRefreshList()

	def LoginFriendFromBridge(self, pid, name, offlineTime=0, country=""):
		if not IsPlausibleMemberDisplayName(name):
			return
		member = self.__AddList(FRIEND, pid, name)
		if not member:
			return
		member.SetName(name)
		if app.ENABLE_MESSENGER_DETAILS:
			member.SetOfflineTime(offlineTime)
			if app.ENABLE_MULTI_LANGUAGE_SYSTEM:
				member.SetLanguage(country)
		member.Offline()
		self.__DedupeFriendMembers()
		if self._uiReady:
			self.OnRefreshList()

	def DeleteFriendMember(self, pid):
		group = self.groupList[FRIEND]
		if group:
			group.RemoveMember(self.__FindGroupMember(group, pid))
		self.OnRefreshList()

	def SetFriendLevelInfo(self, pid, isConqueror, level):
		group = self.groupList[FRIEND]
		if not group:
			return
		member = self.__FindGroupMember(group, pid)
		if member and level > 0:
			member.SetLevel(level)
			if self._uiReady:
				self.OnRefreshList()

	def SetFriendLocationInfo(self, pid, channel, mapIndex):
		group = self.groupList[FRIEND]
		if not group:
			return
		member = self.__FindGroupMember(group, pid)
		if member:
			member.SetLocationInfo(channel, mapIndex)
			if self._uiReady:
				self.OnRefreshList()
		if IsCommunityGuildRenewalEnabled() and self.owner and self.owner.guildWindow:
			self.owner.guildWindow.SetGuildMemberLocationInfo(pid, channel, mapIndex)

	def SetGuildMemberLocationInfo(self, pid, channel, mapIndex):
		if not IsCommunityGuildRenewalEnabled():
			return
		if self.owner and self.owner.guildWindow:
			self.owner.guildWindow.SetGuildMemberLocationInfo(pid, channel, mapIndex)

	def UpdateGuildMemberLocationInfo(self, pid, channel, mapIndex):
		self.SetGuildMemberLocationInfo(pid, channel, mapIndex)

	def SetFriendStatusMessage(self, pid, statusMessage):
		group = self.groupList[FRIEND]
		if not group:
			return
		statusMessage = statusMessage or ""
		normKey = pid
		if isinstance(pid, basestring):
			normKey = NameKeyToPid(pid)
		member = self.__FindGroupMember(group, normKey)
		if not member:
			self._pendingFriendStatusByKey[normKey] = statusMessage
			return
		member.SetStatusMessage(statusMessage)
		self._pendingFriendStatusByKey.pop(normKey, None)
		if self._uiReady:
			self.OnRefreshList()

	def UpdateFriendLocationInfo(self, pid, channel, mapIndex):
		self.SetFriendLocationInfo(pid, channel, mapIndex)

	def EnableFriendFavorite(self, pid, isEnable):
		group = self.groupList[FRIEND]
		if not group:
			return
		member = self.__FindGroupMember(group, pid)
		if member:
			member.SetFavorite(isEnable)
			self.OnRefreshList()

	def OnMemberFavoriteClick(self, member):
		if not member or not member.IsOnline():
			return
		if member.isFavorite:
			self.__AskDeleteFavorite(member)
		else:
			self.__AskAddFavorite(member)

	def __AskAddFavorite(self, member):
		self.favoriteQuestionMember = member
		self.favoriteQuestionDialog = uiCommon.QuestionDialog()
		text = getattr(uiScriptLocale, "COMMUNITY_ADD_FAVORITE_QUESTION", "Add to favorites?")
		self.favoriteQuestionDialog.SetText(text)
		self.favoriteQuestionDialog.SetAcceptEvent(ui.__mem_func__(self.__OnAcceptAddFavorite))
		self.favoriteQuestionDialog.SetCancelEvent(ui.__mem_func__(self.__OnCloseFavoriteQuestion))
		self.favoriteQuestionDialog.Open()

	def __AskDeleteFavorite(self, member):
		self.favoriteQuestionMember = member
		self.favoriteQuestionDialog = uiCommon.QuestionDialog()
		text = getattr(uiScriptLocale, "COMMUNITY_DELETE_FAVORITE_QUESTION", "Remove from favorites?")
		self.favoriteQuestionDialog.SetText(text)
		self.favoriteQuestionDialog.SetAcceptEvent(ui.__mem_func__(self.__OnAcceptDeleteFavorite))
		self.favoriteQuestionDialog.SetCancelEvent(ui.__mem_func__(self.__OnCloseFavoriteQuestion))
		self.favoriteQuestionDialog.Open()

	def __OnAcceptAddFavorite(self):
		member = self.favoriteQuestionMember
		if member:
			if hasattr(community, "SendAddFavorite"):
				try:
					community.SendAddFavorite(member.GetKey())
				except TypeError:
					community.SendAddFavorite()
			else:
				member.SetFavorite(True)
		self.__OnCloseFavoriteQuestion()
		return True

	def __OnAcceptDeleteFavorite(self):
		member = self.favoriteQuestionMember
		if member:
			if hasattr(community, "SendDeleteFavorite"):
				try:
					community.SendDeleteFavorite(member.GetKey())
				except TypeError:
					community.SendDeleteFavorite()
			else:
				member.SetFavorite(False)
		self.__OnCloseFavoriteQuestion()
		return True

	def __OnCloseFavoriteQuestion(self):
		if self.favoriteQuestionDialog:
			self.favoriteQuestionDialog.Close()
		self.favoriteQuestionDialog = None
		self.favoriteQuestionMember = None
		return True

	def OnMemberPartyInviteClick(self, member):
		if not member or not member.IsOnline():
			return
		name = member.GetName()
		if not name:
			return
		if hasattr(community, "CanPartyInviteTime") and not community.CanPartyInviteTime():
			return
		if hasattr(community, "SendPartyInvite"):
			community.SendPartyInvite(name)

	def OnFamilyPartyInviteClick(self):
		if not self.familyGroup:
			return
		lover = self.familyGroup.GetLover()
		if lover:
			self.OnMemberPartyInviteClick(lover)

	if app.ENABLE_MESSENGER_BLOCK:
		def RemoveBlockMemberLocally(self, member):
			if not member:
				return
			group = self.groupList[BLOCK]
			if not group:
				return
			if self.selectedItem == member:
				member.UnSelect()
				self.selectedItem = None
				if self.friendWhisperButton:
					self.friendWhisperButton.Disable()
				if self.friendDeleteButton:
					self.friendDeleteButton.Disable()
			group.RemoveMember(member)
			if self._uiReady:
				self.OnRefreshList()

		def AddBlockFromBridge(self, pid, name):
			if not self.groupList[BLOCK]:
				return
			member = self.__AddList(BLOCK, pid, name)
			if not member:
				return
			member.SetName(name)
			member.Offline()
			self.OnRefreshList()

		def DeleteBlockMember(self, pid):
			group = self.groupList[BLOCK]
			if group:
				member = self.__FindGroupMember(group, pid)
				if member:
					if self.selectedItem == member:
						member.UnSelect()
						self.selectedItem = None
					group.RemoveMember(member)
			if self._uiReady:
				self.OnRefreshList()

	def OnAddLover(self, name, lovePoint):
		if not self._uiReady:
			self._pendingLover = (name, lovePoint)
			return
		self.__ApplyLover(name, lovePoint)

	def __ApplyLover(self, name, lovePoint):
		if not self.familyGroup:
			self.familyGroup = uimessenger.MessengerFamilyGroup(ui.__mem_func__(self.GetSelf))
			self.familyGroup.Open()
		member = self.familyGroup.AppendMember(0, name)
		member.SetName(name)
		member.SetLovePoint(lovePoint)
		member.Offline()
		if self.familyInfoBar:
			self.familyInfoBar.EnableFamilyInfoBar()
		self.OnRefreshList()

	def OnUpdateLovePoint(self, lovePoint):
		if not self._uiReady:
			self._pendingLovePoint = lovePoint
			return
		if not self.familyGroup:
			return
		lover = self.familyGroup.GetLover()
		if lover:
			lover.SetLovePoint(lovePoint)

	def OnLoginLover(self):
		if not self._uiReady:
			self._pendingLoverLogin = True
			return
		if not self.familyGroup:
			return
		lover = self.familyGroup.GetLover()
		if lover:
			lover.Online()

	def OnLogoutLover(self):
		if not self._uiReady:
			self._pendingLoverLogin = False
			return
		if not self.familyGroup:
			return
		lover = self.familyGroup.GetLover()
		if lover:
			lover.Offline()

	def ClearLoverInfo(self):
		if not self._uiReady:
			self._pendingLover = None
			self._pendingLovePoint = None
			self._pendingLoverLogin = False
			return
		if not self.familyGroup:
			return
		self.familyGroup.ClearMember()
		self.familyGroup = None
		if self.familyInfoBar:
			self.familyInfoBar.DisableFamilyInfoBar()
		self.OnRefreshList()

def IsCommunityConfigFlagSet(settingFlag, flagBit):
	return (int(settingFlag) & int(flagBit)) != 0

class CommunityConfigCheckItem(ui.Window):
	def __init__(self, owner, parentSubTopic, itemMode, toggleFlag, mirrorParentOn=False):
		ui.Window.__init__(self)
		self.owner = owner
		self.parentSubTopic = parentSubTopic
		self.itemMode = itemMode
		self.toggleFlag = toggleFlag
		self.mirrorParentOn = mirrorParentOn
		self.itemImage = None
		self.checkImage = None
		self.unCheckImage = None
		self.toolTip = None
		self.toolTipText = ""
		self.isChecked = False
		self.SetSize(COMMUNITY_CONFIG_CHECK_ITEM_W, COMMUNITY_CONFIG_CHECK_ITEM_H)
		self.__LoadWindow()

	def __LoadWindow(self):
		iconPath = CONFIG_CHECK_ICON_PATH.get(self.itemMode, "")
		if iconPath:
			self.itemImage = ui.ImageBox()
			self.itemImage.SetParent(self)
			self.itemImage.LoadImage(iconPath)
			self.itemImage.SetPosition(0, 2)
			self.itemImage.Show()

			tooltipKey = CONFIG_CHECK_TOOLTIP_KEY.get(self.itemMode, "")
			self.toolTipText = GetUiScriptLocaleText(tooltipKey)

		self.checkImage = ui.ImageBox()
		self.checkImage.SetParent(self)
		self.checkImage.Hide()
		self.checkImage.LoadImage(ROOT_PATH + "check_box.sub")
		self.checkImage.SetPosition(COMMUNITY_CONFIG_CHECK_BOX_X, COMMUNITY_CONFIG_CHECK_BOX_Y)
		self.checkImage.SetEvent(ui.__mem_func__(self.__OnClickCheck), "mouse_click")
		self.checkImage.Show()

		self.unCheckImage = ui.ImageBox()
		self.unCheckImage.SetParent(self)
		self.unCheckImage.LoadImage(ROOT_PATH + "uncheck_box.sub")
		self.unCheckImage.SetPosition(COMMUNITY_CONFIG_CHECK_BOX_X, COMMUNITY_CONFIG_CHECK_BOX_Y)
		self.unCheckImage.SetEvent(ui.__mem_func__(self.__OnClickCheck), "mouse_click")
		self.unCheckImage.Show()

		if self.toolTipText:
			self.__BindToolTipHover(self.itemImage)
			self.__BindToolTipHover(self.checkImage)
			self.__BindToolTipHover(self.unCheckImage)

	def __BindToolTipHover(self, widget):
		if not widget:
			return
		widget.SetEvent(ui.__mem_func__(self.__OnMouseOverIn), "mouse_over_in")
		widget.SetEvent(ui.__mem_func__(self.__OnMouseOverOut), "mouse_over_out")

	def ApplyClipMask(self, clipWindow):
		ApplyCommunityClipMask(self, clipWindow)
		for widget in (self.itemImage, self.checkImage, self.unCheckImage):
			ApplyCommunityClipMask(widget, clipWindow)

	def __OnMouseOverIn(self):
		if not self.toolTipText:
			return
		self.HideToolTip()
		minWidth = GetCommunityConfigToolTipMinWidth(self.toolTipText)
		tooltip = uiToolTip.ToolTip(minWidth)
		tooltip.AutoAppendTextLine(self.toolTipText)
		tooltip.AlignTextLineHorizonalCenter()
		tooltip.ResizeToolTip()
		tooltip.ShowToolTip()
		self.toolTip = tooltip

	def __OnMouseOverOut(self):
		self.HideToolTip()

	def HideToolTip(self):
		if self.toolTip:
			self.toolTip.HideToolTip()
			self.toolTip = None

	def __OnClickCheck(self):
		if not self.parentSubTopic or not self.parentSubTopic.IsOn():
			return
		if self.mirrorParentOn:
			return
		self.owner.ToggleConfigFlag(self.toggleFlag)

	def UpdateByFlag(self, settingFlag):
		if self.mirrorParentOn:
			self.isChecked = self.parentSubTopic.IsOn()
		else:
			self.isChecked = IsCommunityConfigFlagSet(settingFlag, self.toggleFlag)
		if self.isChecked:
			if self.checkImage:
				self.checkImage.Show()
			if self.unCheckImage:
				self.unCheckImage.Hide()
		else:
			if self.checkImage:
				self.checkImage.Hide()
			if self.unCheckImage:
				self.unCheckImage.Show()

	def InitConfig(self, settingFlag):
		self.UpdateByFlag(settingFlag)

class CommunityConfigSubTopic(ui.Window):
	def __init__(self, owner, subTopicType, toggleFlag):
		ui.Window.__init__(self)
		self.owner = owner
		self.subTopicType = subTopicType
		self.toggleFlag = toggleFlag
		self.titleBg = None
		self.titleText = None
		self.onOffButton = None
		self.checkItemList = []
		self.isOn = False
		self.baseY = 0
		self.SetSize(COMMUNITY_CONFIG_SUB_TOPIC_W, COMMUNITY_CONFIG_SUB_TOPIC_H)
		self.__LoadWindow()

	def __LoadWindow(self):
		self.titleBg = ui.ImageBox()
		self.titleBg.SetParent(self)
		self.titleBg.AddFlag("not_pick")
		self.titleBg.LoadImage(ROOT_PATH + "community_config_sub_title.sub")
		self.titleBg.SetPosition(COMMUNITY_CONFIG_SUB_TITLE_X, COMMUNITY_CONFIG_SUB_TITLE_Y)
		self.titleBg.Show()

		self.titleText = ui.TextLine()
		self.titleText.SetParent(self.titleBg)
		self.titleText.SetPosition(self.titleBg.GetWidth() // 2, self.titleBg.GetHeight() // 2)
		self.titleText.SetVerticalAlignCenter()
		self.titleText.SetHorizontalAlignCenter()
		titleKey = CONFIG_SUB_TOPIC_TITLE.get(self.subTopicType, "")
		self.titleText.SetText(GetUiScriptLocaleText(titleKey))
		self.titleText.Show()

		self.onOffButton = ui.ToggleButton()
		self.onOffButton.SetParent(self)
		self.onOffButton.SetUpVisual(COMMUNITY_CONFIG_ON_OFF_BTN_VISUAL)
		self.onOffButton.SetOverVisual(COMMUNITY_CONFIG_ON_OFF_BTN_VISUAL_OVER)
		self.onOffButton.SetDownVisual(COMMUNITY_CONFIG_ON_OFF_BTN_VISUAL_DOWN)
		self.onOffButton.SetPosition(COMMUNITY_CONFIG_ON_OFF_BTN_X, COMMUNITY_CONFIG_ON_OFF_BTN_Y)
		self.onOffButton.SetToggleUpEvent(ui.__mem_func__(self.__OnClickOnOff))
		self.onOffButton.SetToggleDownEvent(ui.__mem_func__(self.__OnClickOnOff))
		self.onOffButton.Show()

	def ApplyClipMask(self, clipWindow):
		ApplyCommunityClipMask(self, clipWindow)
		ApplyCommunityClipMask(self.titleBg, clipWindow)
		ApplyCommunityClipMask(self.titleText, clipWindow)
		ApplyCommunityClipMask(self.onOffButton, clipWindow)
		for checkItem in self.checkItemList:
			checkItem.ApplyClipMask(clipWindow)

	def AppendCheckItem(self, itemMode, toggleFlag, mirrorParentOn=False):
		checkItem = CommunityConfigCheckItem(self.owner, self, itemMode, toggleFlag, mirrorParentOn)
		checkItem.SetParent(self)
		index = len(self.checkItemList)
		checkItem.SetPosition(2 + (COMMUNITY_CONFIG_CHECK_ITEM_INTERVAL_X * index), COMMUNITY_CONFIG_CHECK_ROW_Y)
		checkItem.Show()
		self.checkItemList.append(checkItem)
		return checkItem

	def __OnClickOnOff(self):
		self.owner.ToggleConfigFlag(self.toggleFlag)

	def __RefreshOnOffButton(self):
		if not self.onOffButton:
			return
		if self.isOn:
			self.onOffButton.SetText(GetUiScriptLocaleText("COMMUNITY_CONFIG_ON", "ON"))
			self.onOffButton.Down()
		else:
			self.onOffButton.SetText(GetUiScriptLocaleText("COMMUNITY_CONFIG_OFF", "OFF"))
			self.onOffButton.SetUp()

	def IsOn(self):
		return self.isOn

	def UpdateByFlag(self, settingFlag):
		self.isOn = IsCommunityConfigFlagSet(settingFlag, self.toggleFlag)
		self.__RefreshOnOffButton()
		for checkItem in self.checkItemList:
			checkItem.UpdateByFlag(settingFlag)

	def InitConfig(self, settingFlag):
		self.UpdateByFlag(settingFlag)

class CommunityConfigMainTopic(ui.Window):
	def __init__(self, owner, titleText):
		ui.Window.__init__(self)
		self.owner = owner
		self.titleTextValue = titleText
		self.titleBg = None
		self.titleText = None
		self.subTopicList = []
		self.baseY = 0
		self.SetSize(COMMUNITY_CONFIG_MAIN_TOPIC_W, COMMUNITY_CONFIG_MAIN_TOPIC_TITLE_H)
		self.__LoadWindow()

	def __LoadWindow(self):
		self.titleBg = ui.ImageBox()
		self.titleBg.SetParent(self)
		self.titleBg.AddFlag("not_pick")
		self.titleBg.LoadImage(ROOT_PATH + "community_config_main_title.sub")
		self.titleBg.SetPosition(
			COMMUNITY_CONFIG_MAIN_TOPIC_TITLE_X,
			COMMUNITY_CONFIG_MAIN_TOPIC_TITLE_Y,
		)
		self.titleBg.Show()

		self.titleText = ui.TextLine()
		self.titleText.SetParent(self.titleBg)
		self.titleText.SetPosition(self.titleBg.GetWidth() // 2, self.titleBg.GetHeight() // 2)
		self.titleText.SetVerticalAlignCenter()
		self.titleText.SetHorizontalAlignCenter()
		self.titleText.SetText(self.titleTextValue)
		self.titleText.Show()

	def ApplyClipMask(self, clipWindow):
		ApplyCommunityClipMask(self, clipWindow)
		ApplyCommunityClipMask(self.titleBg, clipWindow)
		ApplyCommunityClipMask(self.titleText, clipWindow)
		for subTopic in self.subTopicList:
			subTopic.ApplyClipMask(clipWindow)

	def AppendSubTopic(self, subTopicType, toggleFlag):
		subTopic = CommunityConfigSubTopic(self.owner, subTopicType, toggleFlag)
		subTopic.SetParent(self)
		subTopic.baseY = COMMUNITY_CONFIG_MAIN_TOPIC_TITLE_H + 5 + (len(self.subTopicList) * COMMUNITY_CONFIG_SUB_TOPIC_H)
		subTopic.SetPosition(2, subTopic.baseY)
		subTopic.Show()
		self.subTopicList.append(subTopic)
		self.__UpdateHeight()
		return subTopic

	def AppendCheckItem(self, itemMode, toggleFlag, mirrorParentOn=False):
		if not self.subTopicList:
			return None
		return self.subTopicList[-1].AppendCheckItem(itemMode, toggleFlag, mirrorParentOn)

	def GetHeight(self):
		height = COMMUNITY_CONFIG_MAIN_TOPIC_TITLE_H + 5
		height += len(self.subTopicList) * COMMUNITY_CONFIG_SUB_TOPIC_H
		return height

	def __UpdateHeight(self):
		self.SetSize(COMMUNITY_CONFIG_MAIN_TOPIC_W, self.GetHeight())

	def UpdateByFlag(self, settingFlag):
		for subTopic in self.subTopicList:
			subTopic.UpdateByFlag(settingFlag)

	def InitConfig(self, settingFlag):
		for subTopic in self.subTopicList:
			subTopic.InitConfig(settingFlag)

class CommunityConfigWindow(object):
	def __init__(self, owner):
		self.owner = owner
		self.configInfoBg = None
		self.scrollBar = None
		self.saveButton = None
		self.initButton = None
		self.contentParent = None
		self.configClipWindow = None
		self.mainTopicList = []
		self.configFlag = community.CONFIG_DEFAULT
		self.savedConfigFlag = community.CONFIG_DEFAULT
		self.scrollValue = 0
		self._uiReady = False
		self.questionDialog = None

	def Destroy(self):
		self._uiReady = False
		self.mainTopicList = []
		self.contentParent = None
		self.configClipWindow = None
		self.scrollBar = None
		self.configInfoBg = None
		self.saveButton = None
		self.initButton = None
		self.questionDialog = None

	def BindWidgets(self, configInfoBg, scrollBar, saveButton, initButton):
		self.configInfoBg = configInfoBg
		self.scrollBar = scrollBar
		self.saveButton = saveButton
		self.initButton = initButton
		if self.scrollBar:
			self.scrollBar.SetScrollEvent(ui.__mem_func__(self.__OnScroll))
			self.scrollBar.SetScrollStep(COMMUNITY_CONFIG_SCROLL_STEP)
		if self.saveButton:
			self.saveButton.SetEvent(ui.__mem_func__(self.__OnClickSaveButton))
		if self.initButton:
			self.initButton.SetEvent(ui.__mem_func__(self.__OnClickInitButton))
		self.__BuildContent()
		self.__ApplyConfigPanelLayout()
		self._uiReady = True
		self.LoadConfigFromServer()
		self.__ApplyClipMask()

	def __GetConfigPanelSize(self):
		return (COMMUNITY_CONFIG_PANEL_W, COMMUNITY_CONFIG_PANEL_H)

	def __GetConfigClipSize(self):
		panelW, panelH = self.__GetConfigPanelSize()
		clipX = max(0, COMMUNITY_CONFIG_CLIP_X)
		clipY = max(0, COMMUNITY_CONFIG_CLIP_Y)
		rightPad = max(0, COMMUNITY_CONFIG_CLIP_RIGHT_PAD)
		bottomPad = max(0, COMMUNITY_CONFIG_CLIP_BOTTOM_PAD)
		clipW = max(1, panelW - clipX - rightPad)
		clipH = max(1, panelH - clipY - bottomPad)
		return (clipW, clipH)

	def __RaiseConfigContentToFront(self):
		if self.configClipWindow:
			self.configClipWindow.SetTop()
		if self.contentParent:
			self.contentParent.Show()
			self.contentParent.SetTop()

	def __ApplyConfigPanelLayout(self):
		# uiscript already builds config_info_bg outline; MakeOutlineWindow here
		# stacks a new center_img over clip/content and hides every settings row.
		if self.scrollBar:
			self.scrollBar.SetPosition(
				COMMUNITY_CONFIG_SCROLLBAR_X, COMMUNITY_CONFIG_SCROLLBAR_Y)
			self.scrollBar.SetScrollBarSize(COMMUNITY_CONFIG_SCROLLBAR_SIZE)
			self.scrollBar.SetTop()
		if self.saveButton:
			self.saveButton.SetPosition(COMMUNITY_CONFIG_SAVE_BTN_X, COMMUNITY_CONFIG_BTN_Y)
		if self.initButton:
			self.initButton.SetPosition(COMMUNITY_CONFIG_INIT_BTN_X, COMMUNITY_CONFIG_BTN_Y)
		if self.configClipWindow:
			clipW, clipH = self.__GetConfigClipSize()
			self.configClipWindow.SetSize(clipW, clipH)
		self.__RaiseConfigContentToFront()

	def __EnsureClipWindow(self):
		if self.configClipWindow or not self.configInfoBg:
			return
		clipW, clipH = self.__GetConfigClipSize()
		clipWindow = ui.Window()
		clipWindow.AddFlag("not_pick")
		clipWindow.SetParent(self.configInfoBg)
		clipWindow.SetPosition(COMMUNITY_CONFIG_CLIP_X, COMMUNITY_CONFIG_CLIP_Y)
		clipWindow.SetSize(clipW, clipH)
		clipWindow.Show()
		clipWindow.SetTop()
		self.configClipWindow = clipWindow

	def __BuildContent(self):
		if not self.configInfoBg:
			return

		self.__EnsureClipWindow()
		clipParent = self.configClipWindow if self.configClipWindow else self.configInfoBg

		if self.contentParent:
			self.contentParent.Hide()
		self.contentParent = ui.Window()
		self.contentParent.SetParent(clipParent)
		self.contentParent.SetPosition(COMMUNITY_CONFIG_CONTENT_X, 0)
		self.contentParent.SetSize(COMMUNITY_CONFIG_MAIN_TOPIC_W, COMMUNITY_CONFIG_PANEL_H)
		self.contentParent.Show()

		self.mainTopicList = []

		blockTopic = CommunityConfigMainTopic(
			self, GetUiScriptLocaleText("COMMUNITY_CONFIG_MAIN_BLOCK"))
		blockTopic.SetParent(self.contentParent)
		blockTopic.Show()
		self.mainTopicList.append(blockTopic)

		blockTopic.AppendSubTopic(0, CONFIG_BLOCK_EXCHANGE_ON)
		blockTopic.AppendCheckItem(CONFIG_CHECK_GUILD, CONFIG_BLOCK_EXCHANGE_GUILD)
		blockTopic.AppendCheckItem(CONFIG_CHECK_FRIEND, CONFIG_BLOCK_EXCHANGE_FRIEND)
		blockTopic.AppendCheckItem(CONFIG_CHECK_FAMILY, CONFIG_BLOCK_EXCHANGE_FAMILY)

		blockTopic.AppendSubTopic(1, CONFIG_BLOCK_PARTY_INVITE_ON)
		blockTopic.AppendCheckItem(CONFIG_CHECK_GUILD, CONFIG_BLOCK_PARTY_INVITE_GUILD)
		blockTopic.AppendCheckItem(CONFIG_CHECK_FRIEND, CONFIG_BLOCK_PARTY_INVITE_FRIEND)
		blockTopic.AppendCheckItem(CONFIG_CHECK_FAMILY, CONFIG_BLOCK_PARTY_INVITE_FAMILY)

		blockTopic.AppendSubTopic(2, CONFIG_BLOCK_PARTY_REQUEST_JOIN_ON)
		blockTopic.AppendCheckItem(CONFIG_CHECK_GUILD, CONFIG_BLOCK_PARTY_REQUEST_JOIN_GUILD)
		blockTopic.AppendCheckItem(CONFIG_CHECK_FRIEND, CONFIG_BLOCK_PARTY_REQUEST_JOIN_FRIEND)
		blockTopic.AppendCheckItem(CONFIG_CHECK_FAMILY, CONFIG_BLOCK_PARTY_REQUEST_JOIN_FAMILY)

		blockTopic.AppendSubTopic(3, CONFIG_BLOCK_WHISPER_ON)
		blockTopic.AppendCheckItem(CONFIG_CHECK_GUILD, CONFIG_BLOCK_WHISPER_GUILD)
		blockTopic.AppendCheckItem(CONFIG_CHECK_FRIEND, CONFIG_BLOCK_WHISPER_FRIEND)
		blockTopic.AppendCheckItem(CONFIG_CHECK_FAMILY, CONFIG_BLOCK_WHISPER_FAMILY)

		blockTopic.AppendSubTopic(4, CONFIG_BLOCK_FRIEND_REQUEST_ON)
		blockTopic.AppendCheckItem(CONFIG_CHECK_GUILD, CONFIG_BLOCK_FRIEND_REQUEST_GUILD)
		blockTopic.AppendCheckItem(CONFIG_CHECK_FRIEND, 0, True)
		blockTopic.AppendCheckItem(CONFIG_CHECK_FAMILY, CONFIG_BLOCK_FRIEND_REQUEST_FAMILY)

		blockTopic.AppendSubTopic(5, CONFIG_BLOCK_GUILD_INVITE_ON)
		blockTopic.AppendCheckItem(CONFIG_CHECK_FRIEND, CONFIG_BLOCK_GUILD_INVITE_FRIEND)
		blockTopic.AppendCheckItem(CONFIG_CHECK_FAMILY, CONFIG_BLOCK_GUILD_INVITE_FAMILY)

		loginTopic = CommunityConfigMainTopic(
			self, GetUiScriptLocaleText("COMMUNITY_CONFIG_MAIN_LOGIN_ALARM"))
		loginTopic.SetParent(self.contentParent)
		loginTopic.Show()
		self.mainTopicList.append(loginTopic)

		loginTopic.AppendSubTopic(6, CONFIG_LOGIN_ALARM_ON)
		loginTopic.AppendCheckItem(CONFIG_CHECK_GUILD_LOGIN, CONFIG_LOGIN_ALARM_ON_GUILD)
		loginTopic.AppendCheckItem(CONFIG_CHECK_FRIEND_LOGIN, CONFIG_LOGIN_ALARM_ON_FRIEND)
		loginTopic.AppendCheckItem(CONFIG_CHECK_FAMILY_LOGIN, CONFIG_LOGIN_ALARM_ON_FAMILY)

		myInfoTopic = CommunityConfigMainTopic(
			self, GetUiScriptLocaleText("COMMUNITY_CONFIG_MAIN_MY_INFO_ALARM"))
		myInfoTopic.SetParent(self.contentParent)
		myInfoTopic.Show()
		self.mainTopicList.append(myInfoTopic)

		myInfoTopic.AppendSubTopic(7, CONFIG_MY_INFO_SHOW_ON)
		myInfoTopic.AppendCheckItem(CONFIG_CHECK_LEVEL, CONFIG_MY_INFO_SHOW_ON_LEVEL)
		myInfoTopic.AppendCheckItem(CONFIG_CHECK_LOCATION, CONFIG_MY_INFO_SHOW_ON_LOCATION)

		self.__ArrangeTopics()
		self.__RefreshScrollMetrics()

	def __ApplyClipMask(self):
		clipWindow = self.configClipWindow if self.configClipWindow else self.configInfoBg
		if not clipWindow:
			return
		ApplyCommunityClipMask(self.contentParent, clipWindow)
		for mainTopic in self.mainTopicList:
			mainTopic.ApplyClipMask(clipWindow)

	def __ArrangeTopics(self):
		_, clipH = self.__GetConfigClipSize()
		posY = 5
		for mainTopic in self.mainTopicList:
			mainTopic.baseY = posY
			mainTopic.SetPosition(COMMUNITY_CONFIG_MAIN_TOPIC_X, posY)
			posY += mainTopic.GetHeight() + COMMUNITY_CONFIG_MAIN_TOPIC_GAP
		if self.contentParent:
			self.contentParent.SetSize(COMMUNITY_CONFIG_MAIN_TOPIC_W, max(posY, clipH))

	def __RefreshScrollMetrics(self):
		_, clipH = self.__GetConfigClipSize()
		totalHeight = 5
		for mainTopic in self.mainTopicList:
			totalHeight += mainTopic.GetHeight() + COMMUNITY_CONFIG_MAIN_TOPIC_GAP
		self.scrollValue = max(0, totalHeight - clipH)
		if self.scrollBar:
			if self.scrollValue > 0:
				pageScale = float(clipH) / float(totalHeight)
				pageScale = max(0.08, min(1.0, pageScale))
				self.scrollBar.SetMiddleBarSize(pageScale)
				self.scrollBar.Show()
				self.scrollBar.SetTop()
			else:
				self.scrollBar.SetPos(0.0)
				self.scrollBar.Hide()
		self.__ApplyScrollOffset()

	def __ApplyScrollOffset(self):
		offsetY = 0
		if self.scrollBar and self.scrollValue > 0:
			offsetY = int(self.scrollBar.GetPos() * self.scrollValue)
		if self.contentParent:
			self.contentParent.SetPosition(COMMUNITY_CONFIG_CONTENT_X, -offsetY)

	def __OnScroll(self):
		self.HideToolTips()
		self.__ApplyScrollOffset()

	def HideToolTips(self):
		for mainTopic in self.mainTopicList:
			for subTopic in mainTopic.subTopicList:
				for checkItem in subTopic.checkItemList:
					checkItem.HideToolTip()

	def LoadConfigFromServer(self):
		try:
			self.savedConfigFlag = community.GetLastSavedConfigFlag()
		except:
			self.savedConfigFlag = community.CONFIG_DEFAULT
		self.configFlag = self.savedConfigFlag
		self.__ApplyConfigFlag()

	def __ApplyConfigFlag(self):
		for mainTopic in self.mainTopicList:
			mainTopic.UpdateByFlag(self.configFlag)
		try:
			community.SetMyConfigFlag(self.configFlag)
		except:
			pass
		self.__ApplyClipMask()

	def ToggleConfigFlag(self, flagBit):
		if IsCommunityConfigFlagSet(self.configFlag, flagBit):
			self.configFlag &= ~int(flagBit)
		else:
			self.configFlag |= int(flagBit)
		self.__ApplyConfigFlag()

	def HasUnsavedChanges(self):
		return int(self.configFlag) != int(self.savedConfigFlag)

	def __OnClickSaveButton(self):
		try:
			community.SetMyConfigFlag(self.configFlag)
			community.SendSaveConfig()
		except:
			pass
		self.savedConfigFlag = self.configFlag

	def __OnClickInitButton(self):
		self.questionDialog = uiCommon.QuestionDialog()
		self.questionDialog.SetText(GetUiScriptLocaleText("COMMUNITY_CONFIG_INIT_QUESTION"))
		self.questionDialog.SetAcceptEvent(ui.__mem_func__(self.__OnAcceptInit))
		self.questionDialog.SetCancelEvent(ui.__mem_func__(self.__OnCloseQuestionDialog))
		self.questionDialog.Open()

	def __OnAcceptInit(self):
		try:
			community.SendInitConfig()
		except:
			pass
		self.configFlag = community.CONFIG_DEFAULT
		self.savedConfigFlag = community.CONFIG_DEFAULT
		self.__ApplyConfigFlag()
		if self.scrollBar:
			self.scrollBar.SetPos(0.0)
		self.__ApplyScrollOffset()
		self.__OnCloseQuestionDialog()

	def __OnCloseQuestionDialog(self):
		if self.questionDialog:
			self.questionDialog.Close()
		self.questionDialog = None
		return True

	def ShowWindow(self):
		if self._uiReady:
			self.__ApplyConfigPanelLayout()
			self.LoadConfigFromServer()
			self.__RefreshScrollMetrics()
			if self.scrollBar:
				self.scrollBar.SetPos(0.0)
			self.__ApplyScrollOffset()
			self.__ApplyClipMask()

	def __IsMouseOverConfigArea(self):
		if self.configClipWindow and self.configClipWindow.IsShow() and IsMouseInWindowRect(self.configClipWindow):
			return True
		if self.configInfoBg and self.configInfoBg.IsShow() and IsMouseInWindowRect(self.configInfoBg):
			return True
		if self.scrollBar and self.scrollBar.IsShow() and IsMouseInWindowRect(self.scrollBar):
			return True
		return False

	def OnMouseWheelScrollLines(self, lineDelta):
		if not self._uiReady or not self.scrollBar or not self.scrollBar.IsShow():
			return False
		if self.scrollValue <= 0:
			return False
		if not self.__IsMouseOverConfigArea():
			return False

		step = COMMUNITY_CONFIG_SCROLL_STEP
		if lineDelta < 0:
			newPos = self.scrollBar.GetPos() - step
		else:
			newPos = self.scrollBar.GetPos() + step
		if newPos < 0.0:
			newPos = 0.0
		elif newPos > 1.0:
			newPos = 1.0
		if newPos == self.scrollBar.GetPos():
			return False
		self.HideToolTips()
		self.scrollBar.SetPos(newPos)
		return True

class CommunityWindow(ui.ScriptWindow):
	COMMUNITY_VIEW_MESSENGER = COMMUNITY_VIEW_MESSENGER
	COMMUNITY_VIEW_GUILD = COMMUNITY_VIEW_GUILD
	COMMUNITY_VIEW_CONFIG = COMMUNITY_VIEW_CONFIG

	def __init__(self):
		ui.ScriptWindow.__init__(self)
		self.interface = None
		self.isLoaded = 0
		self.currentViewState = COMMUNITY_VIEW_MESSENGER
		self.whisperButtonEvent = lambda *arg: None
		self.guildButtonEvent = lambda *arg: None

		self.board = None
		self.myInfoBg = None
		self.myInfoPlayerNameText = None
		self.myInfoGuildNameText = None
		self.myInfoStateDropButton = None
		self.myInfoMessageButton = None
		self.myInfoNameCollision = None
		self._alignmentToolTip = None
		self.connectionStateBox = None
		self.connectionStateToolTipPicker = None
		self.stateDropAnchor = None
		self.stateDropListWindow = None
		self._stateDropListOpen = False
		self._ignoreNextMouseUp = False
		self.statusMessageInputDialog = None

		self.messengerViewWindow = None
		self.guildViewWindow = None
		self.configViewWindow = None

		self.guildPlayerHasGuildWindow = None
		self.guildPlayerNoGuildWindow = None
		self.guildMemberTabImage = None
		self.guildMemberTabButton = None
		self.guildMemberTabTooltipPicker = None
		self._guildSubTabPickerReady = False
		self._guildWhisperTooltipPicker = None
		self._guildWhisperTooltipPickerReady = False
		self.guildInviteButton = None
		self.guildBlockButton = None
		self.guildWhisperButton = None
		self.guildDeleteButton = None
		self.guildInfoButton = None

		self.mainTabMessengerButton = None
		self.mainTabGuildButton = None
		self.mainTabConfigButton = None
		self.mainTabMessengerImage = None
		self.mainTabGuildImage = None
		self.mainTabConfigImage = None
		self._mainTabPickers = []
		self._mainTabPickersReady = False

		self._uiToolTipHelper = CommunityUiToolTipHelper()
		self.messengerWindow = CommunityMessengerWindow(self)
		self.configWindow = CommunityConfigWindow(self)
		self.guildWindow = None
		if IsCommunityGuildRenewalEnabled():
			self.guildWindow = CommunityGuildWindow(self)

	def __del__(self):
		ui.ScriptWindow.__del__(self)

	def BindInterface(self, interface):
		self.interface = interface
		self.messengerWindow.BindInterface(interface)

	def SetWhisperButtonEvent(self, event):
		self.whisperButtonEvent = event
		self.messengerWindow.SetWhisperButtonEvent(event)
		if self.guildWindow:
			self.guildWindow.SetWhisperButtonEvent(event)

	def SetGuildButtonEvent(self, event):
		self.guildButtonEvent = event

	def GetMessengerWindow(self):
		return self.messengerWindow

	def AddFriendRequest(self, name, level=0, channel=0, mapIndex=0):
		if self.messengerWindow:
			return self.messengerWindow.AddFriendRequest(name, level, channel, mapIndex)
		return False

	def Show(self):
		if self.isLoaded == 0:
			self.isLoaded = 1
			self.__LoadWindow()
		else:
			self.messengerWindow.SyncFriendsFromEngine()
			if app.ENABLE_MESSENGER_BLOCK:
				self.messengerWindow.SyncBlocksFromEngine()
		community.RequestMessengerInfo()
		ui.ScriptWindow.Show(self)
		if app.ENABLE_MOUSE_WHEEL_TOP_WINDOW:
			wndMgr.SetWheelTopWindow(self.hWnd)
		if self.isLoaded:
			self.__RefreshMyInfo()
			self.__RefreshGuildViewState()
			self.RaiseMyInfoChromeToFront()

	def Close(self):
		self._uiToolTipHelper.Hide()
		self.__HideAlignmentToolTip()
		if self.configWindow:
			self.configWindow.HideToolTips()
		self.__CloseStateDropList()
		self.__CloseStatusMessageDialog()
		if app.ENABLE_MOUSE_WHEEL_TOP_WINDOW:
			wndMgr.ClearWheelTopWindow(self.hWnd)
		self.Hide()

	def Destroy(self):
		community.ClearCommunityHandler()
		if self.messengerWindow:
			self.messengerWindow.Destroy()
		self.messengerWindow = None
		if self.guildWindow:
			self.guildWindow.Destroy()
		self.guildWindow = None
		if self.statusMessageInputDialog:
			self.statusMessageInputDialog.Hide()
		self.statusMessageInputDialog = None
		self.board = None
		self.isLoaded = 0
		self.ClearDictionary()

	def RefreshMessenger(self):
		if not self.messengerWindow._engineActive:
			return
		self.messengerWindow.SyncFriendsFromEngine()
		if app.ENABLE_MESSENGER_BLOCK:
			self.messengerWindow.SyncBlocksFromEngine()
		messenger.RefreshGuildMember()
		if self.isLoaded and self.messengerWindow._uiReady:
			self.messengerWindow.RefreshWindow()
		if self.isLoaded and IsCommunityGuildRenewalEnabled() and self.guildWindow and self.guildWindow._uiReady:
			self.guildWindow.OnRefreshList()
		if self.isLoaded:
			self.__RefreshMyInfo()
			self.__RefreshGuildViewState()
			self.RaiseMyInfoChromeToFront()

	def ClearGuildMember(self):
		if self.guildWindow:
			self.guildWindow.ClearMember()

	def OnAddLover(self, name, lovePoint):
		self.messengerWindow.OnAddLover(name, lovePoint)

	def OnSelectItem(self, item):
		if self.currentViewState == COMMUNITY_VIEW_GUILD and self.guildWindow:
			self.guildWindow.OnSelectItem(item)
		elif self.messengerWindow:
			self.messengerWindow.OnSelectItem(item)

	def OnDoubleClickItem(self, item):
		if self.currentViewState == COMMUNITY_VIEW_GUILD and self.guildWindow:
			self.guildWindow.OnDoubleClickItem(item)
		elif self.messengerWindow:
			self.messengerWindow.OnDoubleClickItem(item)

	def OnUpdateLovePoint(self, lovePoint):
		self.messengerWindow.OnUpdateLovePoint(lovePoint)

	def OnUpdate(self):
		if self.messengerWindow:
			self.messengerWindow.OnUpdate()

	def OnLoginLover(self):
		self.messengerWindow.OnLoginLover()

	def OnLogoutLover(self):
		self.messengerWindow.OnLogoutLover()

	def ClearLoverInfo(self):
		self.messengerWindow.ClearLoverInfo()

	def __LoadWindow(self):
		pyScrLoader = ui.PythonScriptLoader()
		pyScrLoader.LoadScriptFile(self, "UIScript/communitywindow.py")

		try:
			self.board = self.GetChild("main_bg")
			self.myInfoBg = self.GetChild("my_info_bg")
			self.myInfoPlayerNameText = self.GetChild("my_info_main_player_name")
			self.myInfoGuildNameText = self.GetChild("my_info_main_player_guild_name")
			self.myInfoStateDropButton = self.GetChild("my_info_state_drop_list_button")
			self.myInfoMessageButton = self.GetChild("my_info_my_message_button")
			self.myInfoNameCollision = self.GetChild("my_info_main_player_name_collision")

			self.messengerViewWindow = self.GetChild("messenger_view_window")
			self.guildViewWindow = self.GetChild("guild_view_window")
			self.configViewWindow = self.GetChild("config_view_window")

			self.mainTabMessengerButton = self.GetChild("community_messenger_tab_button")
			self.mainTabGuildButton = self.GetChild("community_guild_tab_button")
			self.mainTabConfigButton = self.GetChild("community_config_tab_button")
			self.mainTabMessengerImage = self.GetChild("community_messenger_tab_image")
			self.mainTabGuildImage = self.GetChild("community_guild_tab_image")
			self.mainTabConfigImage = self.GetChild("community_config_tab_image")
		except:
			import exception
			exception.Abort("CommunityWindow.__LoadWindow")

		self.board.SetCloseEvent(ui.__mem_func__(self.Close))
		self.__FixHeaderForLocale()
		self.__LayoutHeader()
		self.__ReparentMyInfoBar()
		self.__BindMainTabs()
		self.__EnsureStateDropList()
		self.__BindStateDropButton()
		self.__BindStatusMessageButton()
		self.messengerWindow.BindScript(self.GetChild)
		self.__BindGuildViewWidgets()
		self.__BindConfigViewWidgets()
		self.__BindGuildViewEvents()
		self.__LayoutGuildView()
		self.__BindCommunityToolTips()
		self.messengerWindow.SyncFriendsFromEngine()
		self.__RefreshMyInfo()
		self.OnSelectMainTabMessenger()
		community.SetCommunityHandler(self)

	def __FixHeaderForLocale(self):
		if localeInfo.IsARABIC():
			return
		if self.myInfoBg:
			self.myInfoBg.LoadImage(COMMUNITY_MY_INFO_BG)
		if self.mainTabMessengerImage:
			self.mainTabMessengerImage.LoadImage(COMMUNITY_MAIN_TAB_FRIEND_IMG)
		if self.mainTabGuildImage:
			self.mainTabGuildImage.LoadImage(COMMUNITY_MAIN_TAB_GUILD_IMG)
		if self.mainTabConfigImage:
			self.mainTabConfigImage.LoadImage(COMMUNITY_MAIN_TAB_CONFIG_IMG)

	def __LayoutHeader(self):
		if self.myInfoBg:
			self.myInfoBg.AddFlag("float")
			self.myInfoBg.SetPosition(COMMUNITY_MY_INFO_X, COMMUNITY_MY_INFO_Y)
		if self.myInfoStateDropButton:
			self.myInfoStateDropButton.SetPosition(COMMUNITY_HDR_DROP_X, COMMUNITY_HDR_DROP_Y)
		if self.myInfoGuildNameText:
			self.myInfoGuildNameText.SetPosition(COMMUNITY_HDR_GUILD_TEXT_X, COMMUNITY_HDR_GUILD_TEXT_Y)
		if self.myInfoPlayerNameText:
			self.myInfoPlayerNameText.SetPosition(COMMUNITY_HDR_NAME_TEXT_X, COMMUNITY_HDR_NAME_TEXT_Y)
		if self.myInfoMessageButton:
			self.myInfoMessageButton.SetPosition(COMMUNITY_HDR_MESSAGE_BTN_X, COMMUNITY_HDR_MESSAGE_BTN_Y)
		if self.myInfoNameCollision:
			self.myInfoNameCollision.SetPosition(
				COMMUNITY_HDR_NAME_COLLISION_X,
				COMMUNITY_HDR_NAME_COLLISION_Y)
			if not COMMUNITY_HDR_HIDE_NAME_COLLISION:
				self.myInfoNameCollision.Show()
			else:
				self.myInfoNameCollision.Hide()

		if not self.connectionStateBox and self.myInfoBg:
			self.connectionStateBox = CommunityConnectionStateBox()
			self.connectionStateBox.SetParent(self.myInfoBg)
			self.connectionStateBox.SetPosition(COMMUNITY_HDR_STATE_X, COMMUNITY_HDR_STATE_Y)
			self.connectionStateBox.Show()

		for image in (self.mainTabMessengerImage, self.mainTabGuildImage, self.mainTabConfigImage):
			if image:
				image.AddFlag("not_pick")

	def __BindConfigViewWidgets(self):
		configInfoBg = self.__OptionalGetChildByName("config_info_bg")
		configScrollBar = self.__OptionalGetChildByName("config_scroll_bar")
		configSaveButton = self.__OptionalGetChildByName("config_setting_save_button")
		configInitButton = self.__OptionalGetChildByName("config_setting_init_button")
		if self.configWindow and configInfoBg:
			self.configWindow.BindWidgets(
				configInfoBg,
				configScrollBar,
				configSaveButton,
				configInitButton,
			)

	def __RefreshHeaderVisibility(self):
		showHeader = self.currentViewState != COMMUNITY_VIEW_CONFIG
		if self.myInfoBg:
			if showHeader:
				self.myInfoBg.Show()
			else:
				self.myInfoBg.Hide()
		if not showHeader:
			self.__CloseStateDropList()
			self.__CloseStatusMessageDialog()

	def __BindGuildViewWidgets(self):
		self.guildPlayerHasGuildWindow = self.__OptionalGetChildByName("guild_player_has_guild_window")
		self.guildPlayerNoGuildWindow = self.__OptionalGetChildByName("guild_player_no_guild_window")
		self.guildMemberTabImage = self.__OptionalGetChildByName("guild_member_tab_image")
		self.guildMemberTabButton = self.__OptionalGetChildByName("guild_member_tab_button")
		self.guildInviteButton = self.__OptionalGetChildByName("guild_invite")
		self.guildBlockButton = self.__OptionalGetChildByName("guild_block")
		self.guildWhisperButton = self.__OptionalGetChildByName("guild_whisper")
		self.guildDeleteButton = self.__OptionalGetChildByName("guild_delete")
		self.guildInfoButton = self.__OptionalGetChildByName("guild_info_button")
		if IsCommunityGuildRenewalEnabled() and self.guildWindow and self.guildPlayerHasGuildWindow:
			self.guildWindow.BindWidgets(
				self.guildPlayerHasGuildWindow,
				self.guildInviteButton,
				self.guildBlockButton,
				self.guildWhisperButton,
				self.guildDeleteButton,
			)

	def __BindGuildViewEvents(self):
		if self.guildInfoButton:
			self.guildInfoButton.SetEvent(ui.__mem_func__(self.OnPressGuildInfoButton))

	def OnPressGuildInfoButton(self):
		if not self.__PlayerHasGuild():
			return
		if self.guildButtonEvent:
			self.guildButtonEvent()

	def __LayoutGuildView(self):
		if self.guildPlayerHasGuildWindow:
			self.guildPlayerHasGuildWindow.SetPosition(COMMUNITY_GUILD_PANEL_X, COMMUNITY_GUILD_PANEL_Y)

		if self.guildMemberTabImage:
			if not localeInfo.IsARABIC():
				self.guildMemberTabImage.LoadImage(COMMUNITY_GUILD_SUB_TAB_IMG)
			self.guildMemberTabImage.SetPosition(
				COMMUNITY_GUILD_SUB_TAB_IMAGE_X, COMMUNITY_GUILD_SUB_TAB_IMAGE_Y)
			self.guildMemberTabImage.AddFlag("not_pick")

		if self.guildMemberTabButton:
			self.guildMemberTabButton.SetPosition(
				COMMUNITY_GUILD_SUB_TAB_RADIO_X, COMMUNITY_GUILD_SUB_TAB_RADIO_Y)

		if self.guildMemberTabTooltipPicker:
			self.guildMemberTabTooltipPicker.SetPosition(
				COMMUNITY_GUILD_SUB_TAB_IMAGE_X,
				COMMUNITY_GUILD_SUB_TAB_IMAGE_Y)
			self.guildMemberTabTooltipPicker.SetTop()

		guildActionButtons = (
			self.guildInviteButton,
			self.guildBlockButton,
			self.guildWhisperButton,
			self.guildDeleteButton,
		)
		for index, button in enumerate(guildActionButtons):
			if button:
				button.SetPosition(
					COMMUNITY_GUILD_ACTION_X_BASE + (COMMUNITY_GUILD_ACTION_INTERVAL_X * index),
					COMMUNITY_GUILD_ACTION_Y,
				)

		if self.guildInfoButton:
			self.guildInfoButton.SetPosition(COMMUNITY_GUILD_INFO_BTN_X, COMMUNITY_GUILD_INFO_BTN_Y)

		self.__RefreshGuildViewState()

	def __PlayerHasGuild(self):
		try:
			if player.GetGuildID() != 0:
				return True
		except:
			pass
		try:
			if guild.GetGuildID() != 0:
				return True
		except:
			pass
		try:
			guildName = guild.GetGuildName()
			if guildName:
				return True
		except:
			pass
		return False

	def __RefreshGuildViewState(self):
		if not self.isLoaded:
			return
		hasGuild = self.__PlayerHasGuild()
		if self.guildPlayerHasGuildWindow:
			if hasGuild:
				self.guildPlayerHasGuildWindow.Show()
			else:
				self.guildPlayerHasGuildWindow.Hide()
		if self.guildPlayerNoGuildWindow:
			if hasGuild:
				self.guildPlayerNoGuildWindow.Hide()
			else:
				self.guildPlayerNoGuildWindow.Show()
		if hasGuild and IsCommunityGuildRenewalEnabled() and self.guildWindow and self.guildWindow._uiReady:
			self.guildWindow.OnRefreshList()

	def __ReparentMyInfoBar(self):
		if not self.myInfoBg:
			return
		self.myInfoBg.SetParent(self)
		self.myInfoBg.AddFlag("float")
		self.myInfoBg.SetPosition(COMMUNITY_MY_INFO_X, COMMUNITY_MY_INFO_Y)
		self.myInfoBg.Show()
		self.myInfoBg.SetTop()

	def __EnsureStateDropAnchor(self):
		if self.stateDropAnchor:
			return
		anchorHeight = COMMUNITY_STATE_DROP_Y + COMMUNITY_STATE_DROP_ITEM_H * len(COMMUNITY_STATE_OPTIONS)
		anchor = ui.Window()
		anchor.AddFlag("float")
		anchor.AddFlag("not_pick")
		anchor.SetParent(self)
		anchor.SetPosition(COMMUNITY_MY_INFO_X, COMMUNITY_MY_INFO_Y)
		anchor.SetSize(COMMUNITY_UPPER_OUTLINE_W, anchorHeight)
		anchor.Hide()
		self.stateDropAnchor = anchor

	def __BindStateDropButton(self):
		if self.myInfoStateDropButton:
			self.myInfoStateDropButton.SetEvent(ui.__mem_func__(self.__OnSelectStateDropButton))

	def __BindStatusMessageButton(self):
		if self.myInfoMessageButton:
			self.myInfoMessageButton.SetEvent(ui.__mem_func__(self.__OnSelectStatusMessageButton))

	def __EnsureStatusMessageDialog(self):
		if self.statusMessageInputDialog:
			return
		dialog = StatusMessageInputDialog()
		dialog.Hide()
		self.statusMessageInputDialog = dialog

	def __PositionStatusMessageDialog(self):
		if not self.statusMessageInputDialog:
			return
		(gx, gy) = self.GetGlobalPosition()
		dialogX = gx + COMMUNITY_MY_INFO_X + COMMUNITY_HDR_MESSAGE_BTN_X + 410 - StatusMessageInputDialog.WIDTH
		dialogY = gy + COMMUNITY_MY_INFO_Y + StatusMessageInputDialog.MY_INFO_BAR_H + 8
		self.statusMessageInputDialog.SetPosition(dialogX, dialogY)

	def __CloseStatusMessageDialog(self):
		if self.statusMessageInputDialog and self.statusMessageInputDialog.IsShow():
			self.statusMessageInputDialog.Hide()

	def __OnSelectStatusMessageButton(self):
		self.__EnsureStatusMessageDialog()
		if not self.statusMessageInputDialog:
			return
		if self.statusMessageInputDialog.IsShow():
			self.statusMessageInputDialog.Hide()
			return
		self._ignoreNextMouseUp = True
		self.__CloseStateDropList()
		self._uiToolTipHelper.Hide()
		self.__PositionStatusMessageDialog()
		self.statusMessageInputDialog.Show()
		self.statusMessageInputDialog.SetTop()

	def LoadMyStatusMessage(self, status_message):
		self.__EnsureStatusMessageDialog()
		if self.statusMessageInputDialog:
			self.statusMessageInputDialog.LoadMyStatusMessage(status_message)

	def __EnsureStateDropList(self):
		self.__EnsureStateDropAnchor()
		if self.stateDropListWindow:
			return
		dropList = CommunityStateDropDownList()
		dropList.SetParent(self.stateDropAnchor)
		dropList.SetPosition(COMMUNITY_STATE_DROP_X, COMMUNITY_STATE_DROP_Y)
		dropList.SetSelectItemEvent(ui.__mem_func__(self.OnSelectConnectionState))
		dropList.SetToolTipHelper(self._uiToolTipHelper)
		dropList.Hide()
		self.stateDropListWindow = dropList

	def __OpenStateDropList(self):
		if not self.stateDropListWindow:
			self.__EnsureStateDropList()
		if not self.stateDropListWindow or not self.stateDropAnchor:
			return
		self._uiToolTipHelper.Hide()
		self.stateDropAnchor.Show()
		self.stateDropAnchor.SetTop()
		self.stateDropListWindow.Open()
		self._stateDropListOpen = True
		self.RaiseMyInfoChromeToFront()

	def __OnSelectStateDropButton(self):
		if not self.stateDropListWindow:
			self.__EnsureStateDropList()
		if not self.stateDropListWindow:
			return
		if self.stateDropListWindow.IsShow():
			self.__CloseStateDropList()
			self.RaiseMyInfoChromeToFront()
			return
		self._ignoreNextMouseUp = True
		self.__OpenStateDropList()

	def __CloseStateDropList(self):
		self._stateDropListOpen = False
		self._uiToolTipHelper.Hide()
		if self.stateDropListWindow:
			self.stateDropListWindow.Close()
		if self.stateDropAnchor:
			self.stateDropAnchor.Hide()

	def __IsMouseInStateDropChrome(self):
		if self.stateDropAnchor and self.stateDropAnchor.IsShow() and self.stateDropAnchor.IsIn():
			return True
		if self.stateDropListWindow and self.stateDropListWindow.IsShow() and self.stateDropListWindow.IsIn():
			return True
		if self.myInfoStateDropButton and self.myInfoStateDropButton.IsIn():
			return True
		if self.myInfoBg and self.myInfoBg.IsIn():
			return True
		return False

	def OnSelectConnectionState(self, connectionState):
		self.__CloseStateDropList()
		if hasattr(community, "CanChangeMyConnectionStateTime"):
			if not community.CanChangeMyConnectionStateTime():
				return
		if hasattr(community, "SendChangeConnectionState"):
			community.SendChangeConnectionState(connectionState)
		self.ChangeMyConnectionState(connectionState)

	def ChangeMyConnectionState(self, connectionState, isForceChangeByServer=False):
		if self.connectionStateBox:
			self.connectionStateBox.SetConnectionState(connectionState)
		if hasattr(community, "SetMainCharacterConnectionState"):
			community.SetMainCharacterConnectionState(connectionState)

	def OnMoveWindow(self, x, y):
		return

	def OnMouseLeftButtonDown(self):
		return False

	def OnMouseLeftButtonUp(self):
		if self._ignoreNextMouseUp:
			self._ignoreNextMouseUp = False
			return False
		if self.statusMessageInputDialog and self.statusMessageInputDialog.IsShow():
			if self.statusMessageInputDialog.IsIn():
				return False
			if self.myInfoMessageButton and self.myInfoMessageButton.IsIn():
				return False
			self.statusMessageInputDialog.Hide()
			return False
		if not self.stateDropListWindow or not self.stateDropListWindow.IsShow():
			return False
		if self.__IsMouseInStateDropChrome():
			return False
		self.__CloseStateDropList()
		return False

	def OnPressEscapeKey(self):
		if self.statusMessageInputDialog and self.statusMessageInputDialog.IsShow():
			self.statusMessageInputDialog.Hide()
			return True
		if self.stateDropListWindow and self.stateDropListWindow.IsShow():
			self.__CloseStateDropList()
			return True
		self.Close()
		return True

	def __RefreshMyConnectionState(self):
		state = community.CONNECT
		if hasattr(community, "GetMainCharacterConnectionState"):
			state = community.GetMainCharacterConnectionState()
		self.ChangeMyConnectionState(state, True)

	def __BindMainTabs(self):
		if self.mainTabMessengerButton:
			self.mainTabMessengerButton.SetEvent(ui.__mem_func__(self.OnSelectMainTabMessenger))
		if self.mainTabGuildButton:
			self.mainTabGuildButton.SetEvent(ui.__mem_func__(self.OnSelectMainTabGuild))
		if self.mainTabConfigButton:
			self.mainTabConfigButton.SetEvent(ui.__mem_func__(self.OnSelectMainTabConfig))

	def __BindCommunityToolTips(self):
		helper = self._uiToolTipHelper
		self.__EnsureMainTabTooltipPickers(helper)
		self.__BindMyInfoNameAlignmentToolTip()
		helper.Bind(
			self.myInfoMessageButton,
			GetUiScriptLocaleText("COMMUNITY_MOUSE_OVER_IN_STATUS_MESSAGE_BUTTON"),
			COMMUNITY_TOOLTIP_W_ACTION)
		self.__BindConnectionStateIconToolTip(helper)
		self.__EnsureGuildSubTabTooltipPicker(helper)
		self.__BindOptionalChildToolTips(helper)
		self.__EnsureGuildWhisperTooltipPicker()

	def __EnsureMainTabTooltipPickers(self, helper):
		if self._mainTabPickersReady or not helper or not self.board:
			return
		specs = (
			(COMMUNITY_MAIN_TAB_PICKER_X[0], ui.__mem_func__(self.OnSelectMainTabMessenger),
			 GetUiScriptLocaleText("COMMUNITY_MAIN_TAB_FRIEND")),
			(COMMUNITY_MAIN_TAB_PICKER_X[1], ui.__mem_func__(self.OnSelectMainTabGuild),
			 GetUiScriptLocaleText("COMMUNITY_MAIN_TAB_GUILD")),
			(COMMUNITY_MAIN_TAB_PICKER_X[2], ui.__mem_func__(self.OnSelectMainTabConfig),
			 GetUiScriptLocaleText("COMMUNITY_MAIN_TAB_CONFIG")),
		)
		for x, clickHandler, tooltipText in specs:
			if not tooltipText:
				continue
			picker = ui.Window()
			picker.SetParent(self.board)
			picker.SetPosition(x, COMMUNITY_MAIN_TAB_PICKER_Y)
			picker.SetSize(COMMUNITY_MAIN_TAB_PICKER_W, COMMUNITY_MAIN_TAB_PICKER_H)
			picker.SetOnMouseLeftButtonUpEvent(clickHandler)
			helper.Bind(picker, tooltipText, COMMUNITY_SUB_TAB_TOOLTIP_MIN_W)
			picker.Show()
			picker.SetTop()
			self._mainTabPickers.append(picker)
		self._mainTabPickersReady = True
		self.__RaiseMainTabTooltipPickersToFront()

	def __RaiseMainTabTooltipPickersToFront(self):
		for picker in self._mainTabPickers:
			if picker:
				picker.SetTop()

	def __EnsureAlignmentToolTip(self):
		if self._alignmentToolTip:
			return
		tooltip = uiToolTip.ToolTip(COMMUNITY_HDR_ALIGNMENT_TOOLTIP_MIN_W)
		tooltip.HideToolTip()
		self._alignmentToolTip = tooltip

	def __RefreshAlignmentToolTipContent(self):
		self.__EnsureAlignmentToolTip()
		point, grade = player.GetAlignmentData()
		titleList = localeInfo.TITLE_NAME_LIST
		if grade < 0 or grade >= len(titleList):
			grade = 4
		gradeColor = GetAlignmentGradeColor(grade)
		tooltip = self._alignmentToolTip
		tooltip.ClearToolTip()
		tooltip.AutoAppendTextLine(titleList[grade], gradeColor)
		tooltip.AutoAppendTextLine(localeInfo.ALIGNMENT_NAME + str(point))
		tooltip.AlignHorizonalCenter()

	def __OnOverInMyInfoNameAlignment(self):
		self.__RefreshAlignmentToolTipContent()
		if self._alignmentToolTip:
			self._alignmentToolTip.SetFollow(True)
			self._alignmentToolTip.ShowToolTip()

	def __OnOverOutMyInfoNameAlignment(self):
		self.__HideAlignmentToolTip()

	def __HideAlignmentToolTip(self):
		if self._alignmentToolTip:
			self._alignmentToolTip.HideToolTip()

	def __BindMyInfoNameAlignmentToolTip(self):
		if not self.myInfoNameCollision:
			return
		if hasattr(self.myInfoNameCollision, "SetOverEvent"):
			self.myInfoNameCollision.SetOverEvent(
				ui.__mem_func__(self.__OnOverInMyInfoNameAlignment))
			self.myInfoNameCollision.SetOverOutEvent(
				ui.__mem_func__(self.__OnOverOutMyInfoNameAlignment))

	def RefreshAlignment(self):
		if self._alignmentToolTip and self._alignmentToolTip.IsShow():
			self.__RefreshAlignmentToolTipContent()

	def __EnsureGuildWhisperTooltipPicker(self):
		if self._guildWhisperTooltipPickerReady or not self.guildPlayerHasGuildWindow:
			return
		tooltipText = GetUiScriptLocaleText("COMMUNITY_MESSENGER_WHISPER")
		if not tooltipText:
			return
		x = (
			COMMUNITY_GUILD_ACTION_X_BASE
			+ (COMMUNITY_GUILD_ACTION_INTERVAL_X * COMMUNITY_GUILD_ACTION_WHISPER_INDEX)
		)
		picker = ui.Window()
		picker.SetParent(self.guildPlayerHasGuildWindow)
		picker.SetPosition(x, COMMUNITY_GUILD_ACTION_Y)
		picker.SetSize(COMMUNITY_FRIEND_ACTION_BTN_W, COMMUNITY_FRIEND_ACTION_BTN_H)
		if self.guildWindow:
			picker.SetOnMouseLeftButtonUpEvent(
				ui.__mem_func__(self.guildWindow.OnPressWhisperButton))
		self._uiToolTipHelper.Bind(picker, tooltipText, COMMUNITY_TOOLTIP_W_ACTION)
		picker.Hide()
		self._guildWhisperTooltipPicker = picker
		self._guildWhisperTooltipPickerReady = True

	def __SyncGuildWhisperTooltipPicker(self):
		picker = self._guildWhisperTooltipPicker
		if not picker:
			return
		if self.guildWhisperButton:
			gx, gy = self.guildWhisperButton.GetLocalPosition()
			picker.SetPosition(gx, gy)
		if (
			self.currentViewState == COMMUNITY_VIEW_GUILD
			and self.__PlayerHasGuild()
			and self.guildViewWindow
			and self.guildViewWindow.IsShow()
		):
			picker.Show()
			picker.SetTop()
		else:
			picker.Hide()

	def __EnsureGuildSubTabTooltipPicker(self, helper):
		if self._guildSubTabPickerReady or not helper or not self.guildPlayerHasGuildWindow:
			return
		tooltipText = GetUiScriptLocaleText("COMMUNITY_MESSENGER_SUB_TAB_GUILD_MEMBER")
		if not tooltipText:
			return
		picker = ui.Window()
		picker.SetParent(self.guildPlayerHasGuildWindow)
		picker.SetPosition(
			COMMUNITY_GUILD_SUB_TAB_IMAGE_X,
			COMMUNITY_GUILD_SUB_TAB_IMAGE_Y)
		picker.SetSize(COMMUNITY_SUB_TAB_PICKER_W, COMMUNITY_SUB_TAB_PICKER_H)
		helper.Bind(picker, tooltipText, COMMUNITY_SUB_TAB_TOOLTIP_MIN_W)
		picker.Show()
		picker.SetTop()
		self.guildMemberTabTooltipPicker = picker
		self._guildSubTabPickerReady = True

	def __BindConnectionStateIconToolTip(self, helper):
		if not self.connectionStateBox or not self.myInfoBg or self.connectionStateToolTipPicker:
			return
		picker = ui.Window()
		picker.SetParent(self.myInfoBg)
		picker.SetPosition(COMMUNITY_HDR_STATE_X, COMMUNITY_HDR_STATE_Y)
		picker.SetSize(10, 10)
		picker.SetOverEvent(ui.__mem_func__(self.__OnOverInConnectionStateTooltip))
		picker.SetOverOutEvent(ui.__mem_func__(helper.Hide))
		picker.Show()
		self.connectionStateToolTipPicker = picker

	def __OnOverInConnectionStateTooltip(self):
		if not self.connectionStateBox:
			return
		text = GetConnectionStateLabel(self.connectionStateBox.GetConnectionState())
		if text:
			self._uiToolTipHelper.Show(text, COMMUNITY_SUB_TAB_TOOLTIP_MIN_W)

	def __BindOptionalChildToolTips(self, helper):
		optionalBindings = (
			("guild_invite", "MESSENGER_ADD_FRIEND", COMMUNITY_TOOLTIP_W_ACTION),
			("guild_block", "MESSENGER_BLOCK", COMMUNITY_TOOLTIP_W_ACTION),
			("guild_whisper", "COMMUNITY_MESSENGER_WHISPER", COMMUNITY_TOOLTIP_W_ACTION),
			("guild_delete", "COMMUNITY_MESSENGER_FRIEND_DELETE", COMMUNITY_TOOLTIP_W_ACTION),
			("guild_info_button", "COMMUNITY_GUILD_INFO", COMMUNITY_TOOLTIP_W_ACTION),
			("config_setting_save_button", "COMMUNITY_CONFIG_SAVE", COMMUNITY_TOOLTIP_W_ACTION),
			("config_setting_init_button", "COMMUNITY_CONFIG_INIT", COMMUNITY_TOOLTIP_W_ACTION),
		)
		for childName, localeKey, width in optionalBindings:
			button = self.__OptionalGetChildByName(childName)
			if button:
				helper.Bind(button, GetUiScriptLocaleText(localeKey), width)

	def __OptionalGetChildByName(self, name):
		try:
			return self.GetChild(name)
		except:
			return None

	def __RefreshMyInfo(self):
		try:
			if self.myInfoPlayerNameText:
				self.myInfoPlayerNameText.SetText(player.GetName())
			if self.myInfoGuildNameText:
				guildName = guild.GetGuildName()
				if guildName:
					self.myInfoGuildNameText.SetText(guildName)
				else:
					self.myInfoGuildNameText.SetText(uiScriptLocale.COMMUNITY_NO_GUILD)
			self.__RefreshMyConnectionState()
			self.__RefreshGuildViewState()
		except:
			pass

	def __HideAllMainViews(self):
		if self.messengerViewWindow:
			self.messengerViewWindow.Hide()
		if self.guildViewWindow:
			self.guildViewWindow.Hide()
		if self.configViewWindow:
			self.configViewWindow.Hide()

	def __HideAllMainTabImages(self):
		for image in (self.mainTabMessengerImage, self.mainTabGuildImage, self.mainTabConfigImage):
			if image:
				image.Hide()

	def RaiseMyInfoChromeToFront(self):
		statusDialogOpen = self.statusMessageInputDialog and self.statusMessageInputDialog.IsShow()
		if self.myInfoBg and self.currentViewState != COMMUNITY_VIEW_CONFIG:
			self.myInfoBg.AddFlag("float")
			if not statusDialogOpen:
				self.myInfoBg.SetTop()
		if self.stateDropAnchor and self.stateDropAnchor.IsShow():
			self.stateDropAnchor.SetTop()
		if self.stateDropListWindow and self.stateDropListWindow.IsShow():
			self.stateDropListWindow.SetTop()
		if statusDialogOpen:
			self.statusMessageInputDialog.SetTop()
		if self.myInfoNameCollision and self.myInfoNameCollision.IsShow():
			self.myInfoNameCollision.SetTop()
		if self.messengerWindow and self.currentViewState == COMMUNITY_VIEW_MESSENGER:
			self.messengerWindow.RaiseFriendActionButtonsToFront()
		if self.currentViewState == COMMUNITY_VIEW_GUILD:
			self.__SyncGuildWhisperTooltipPicker()
		self.__RaiseMainTabTooltipPickersToFront()

	def OnSelectMainTabMessenger(self):
		if self.configWindow:
			self.configWindow.HideToolTips()
		self.currentViewState = COMMUNITY_VIEW_MESSENGER
		self.__HideAllMainViews()
		self.__HideAllMainTabImages()
		if self.messengerViewWindow:
			self.messengerViewWindow.Show()
		if self.mainTabMessengerImage:
			self.mainTabMessengerImage.Show()
		if self.mainTabMessengerButton:
			self.mainTabMessengerButton.Down()
		if self.mainTabGuildButton:
			self.mainTabGuildButton.SetUp()
		if self.mainTabConfigButton:
			self.mainTabConfigButton.SetUp()
		if self.messengerWindow:
			self.messengerWindow.SetFriendActionButtonsVisible(True)
		self.messengerWindow.RefreshWindow()
		self.__RefreshHeaderVisibility()
		self.__SyncGuildWhisperTooltipPicker()
		self.RaiseMyInfoChromeToFront()

	def OnSelectMainTabGuild(self):
		if self.configWindow:
			self.configWindow.HideToolTips()
		self.currentViewState = COMMUNITY_VIEW_GUILD
		if self.messengerWindow:
			self.messengerWindow.SetFriendActionButtonsVisible(False)
		self.__HideAllMainViews()
		self.__HideAllMainTabImages()
		if self.guildViewWindow:
			self.guildViewWindow.Show()
		if self.mainTabGuildImage:
			self.mainTabGuildImage.Show()
		if self.mainTabMessengerButton:
			self.mainTabMessengerButton.SetUp()
		if self.mainTabGuildButton:
			self.mainTabGuildButton.Down()
		if self.mainTabConfigButton:
			self.mainTabConfigButton.SetUp()
		self.__RefreshGuildViewState()
		if IsCommunityGuildRenewalEnabled():
			messenger.RefreshGuildMember()
			if self.guildWindow and self.guildWindow._uiReady:
				self.guildWindow.OnRefreshList()
		self.__RefreshHeaderVisibility()
		self.__SyncGuildWhisperTooltipPicker()
		self.RaiseMyInfoChromeToFront()
		self.__RaiseMainTabTooltipPickersToFront()

	def OnSelectMainTabConfig(self):
		self.currentViewState = COMMUNITY_VIEW_CONFIG
		if self.messengerWindow:
			self.messengerWindow.SetFriendActionButtonsVisible(False)
		self.__HideAllMainViews()
		self.__HideAllMainTabImages()
		if self.configViewWindow:
			self.configViewWindow.Show()
		if self.mainTabConfigImage:
			self.mainTabConfigImage.Show()
		if self.mainTabMessengerButton:
			self.mainTabMessengerButton.SetUp()
		if self.mainTabGuildButton:
			self.mainTabGuildButton.SetUp()
		if self.mainTabConfigButton:
			self.mainTabConfigButton.Down()
		if self.configWindow:
			self.configWindow.ShowWindow()
		self.__RefreshHeaderVisibility()
		self.__SyncGuildWhisperTooltipPicker()
		self.__RaiseMainTabTooltipPickersToFront()

	if app.ENABLE_MOUSE_WHEEL_TOP_WINDOW:
		def OnMouseWheelButtonUp(self):
			if self.currentViewState == COMMUNITY_VIEW_CONFIG:
				if self.configWindow:
					return self.configWindow.OnMouseWheelScrollLines(-1)
				return False
			if self.currentViewState == COMMUNITY_VIEW_GUILD:
				if IsCommunityGuildRenewalEnabled() and self.guildWindow:
					return self.guildWindow.OnMouseWheelScrollLines(-1)
				return False
			if self.currentViewState != COMMUNITY_VIEW_MESSENGER:
				return False
			if not self.messengerWindow:
				return False
			return self.messengerWindow.OnMouseWheelScrollLines(-1)

		def OnMouseWheelButtonDown(self):
			if self.currentViewState == COMMUNITY_VIEW_CONFIG:
				if self.configWindow:
					return self.configWindow.OnMouseWheelScrollLines(1)
				return False
			if self.currentViewState == COMMUNITY_VIEW_GUILD:
				if IsCommunityGuildRenewalEnabled() and self.guildWindow:
					return self.guildWindow.OnMouseWheelScrollLines(1)
				return False
			if self.currentViewState != COMMUNITY_VIEW_MESSENGER:
				return False
			if not self.messengerWindow:
				return False
			return self.messengerWindow.OnMouseWheelScrollLines(1)
