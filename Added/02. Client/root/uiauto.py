import ui
import app
import localeInfo
import constInfo
import player
import net
import chrmgr
import wndMgr
import mouseModule
import guild
import skill
import item
import chr
import uiToolTip

class AutoWindow(ui.ScriptWindow):
	# C++: beceri 0..11, iksir 13..AUTO_POSITINO_SLOT_MAX-1 (12 arada UI yok).
	AUTO_POTION_CPP_SLOT_MIN = 13
	AUTO_COOLTIME_POS_Y = 4
	AUTO_COOLTIME_POS_X = 4
	AUTO_COOLTIME_MAX = AUTO_COOLTIME_POS_Y * AUTO_COOLTIME_POS_X
	AUTO_ONOFF_START = 1
	AUTO_ONOFF_ATTACK = 2
	AUTO_ONOFF_SKILL = 3
	AUTO_ONOFF_POSITION = 4
	AUTO_ONOFF_AUTO_RANGE = 5	

	def __init__(self):
		ui.ScriptWindow.__init__(self)
		self.isloded = 0
		self.isOpen = 0
		self.tooltipSkill = 0
		self.tooltipItem = 0
		self.autostartonoff = 0
		self.autoslotindex = {}
		self.timeeditlist = {}
		self.autoonoffbuttonlist =[]
		self.autoslot = None
		self.AutoSkillClearButton = None
		self.AutoPositionClearButton = None
		self.AutoAllClearButton = None
		self.AutoToolTipButton = None
		self.AutoToolTip = None
		self.__useToggleButtons = False
		self.__toggleButtons = {}
		self.__cooltimeEditCount = 0
		self.__layoutHunting = True
		self.slotSkill = None
		self.slotPotion = None
		self.__restartToggleButton = None
		self.__communityAutoHuntSyncHandler = None
		for i in xrange(player.AUTO_SKILL_SLOT_MAX):
			self.autoslotindex[i] = 0
		
		for i in range(player.AUTO_POSITINO_SLOT_START,player.AUTO_POSITINO_SLOT_MAX):
			self.autoslotindex[i] = player.ITEM_SLOT_COUNT
			
		self.AutoSystemToolTipList = [
			localeInfo.AUTO_TOOLTIP_LINE1,
			localeInfo.AUTO_TOOLTIP_LINE2,
			localeInfo.AUTO_TOOLTIP_LINE3,
			localeInfo.AUTO_TOOLTIP_LINE4,
			localeInfo.AUTO_TOOLTIP_LINE5,
		]
		for _ln in (6, 7, 8):
			_s = getattr(localeInfo, "AUTO_TOOLTIP_LINE%d" % _ln, "")
			if _s:
				self.AutoSystemToolTipList.append(_s)
		self.closegame = False
		self._attackOptDlg = None
		self._rangeOptDlg = None
		self.__lastStartStopVisualSync = 0.0
		self.LoadAutoWindow()
		self.isFirstReadFile = False

	def __GetChildOptional(self, childName):
		try:
			return self.GetChild(childName)
		except:
			return None

	def __GetChildFromOptional(self, parent, childName):
		try:
			return parent.GetChild(childName)
		except:
			return None

	def __del__(self):
		ui.ScriptWindow.__del__(self)
		self.isloded = 0
		self.isOpen = 0
		self.tooltipSkill = 0
		self.tooltipItem = 0
		self.autostartonoff = 0
		self.autoslotindex = {}
		self.timeeditlist = {}
		self.autoonoffbuttonlist =[]
		self.autoslot = None
		self.AutoSkillClearButton = None
		self.AutoPositionClearButton = None
		self.AutoAllClearButton = None
		self.AutoToolTipButton = None
		self.AutoToolTip = None
		self.closegame = False
		self.isFirstReadFile = False
		self.__useToggleButtons = False
		self.__toggleButtons = {}
		self.__cooltimeEditCount = 0
		self.__layoutHunting = True
		self.slotSkill = None
		self.slotPotion = None
		self.__restartToggleButton = None
		self.__lastStartStopVisualSync = 0.0

	def __GlobalAutoSlotToWindow(self, globalSlotIndex):
		# C++: skill 0..11, pozisyon 12..24; Autohuntingwindow iksir slotlari 0..11 (oyuncu 13..24, 12 UI yok).
		if globalSlotIndex < player.AUTO_SKILL_SLOT_MAX:
			return self.slotSkill, globalSlotIndex
		if globalSlotIndex >= 13:
			return self.slotPotion, globalSlotIndex - 13
		return None, -1

	def __CreateHuntingCooltimeFallbackRow(self, parent, startIndex):
		# UIScript'te hunt_* yoksa (eski epk): autowindow ile ayni gorunum.
		imgY = 2
		editX = 3
		editY = 3
		editW = 32
		editH = 18
		imgDx = 36
		imgPath = "d:/ymir work/ui/game/windows/auto_system_edit_line.sub"
		for col in xrange(6):
			idx = startIndex + col
			bg = ui.ExpandedImageBox()
			bg.SetParent(parent)
			bg.SetPosition(3 + imgDx * col, imgY)
			bg.LoadImage(imgPath)
			bg.Show()
			el = ui.EditLine()
			el.SetParent(bg)
			el.SetPosition(editX, editY)
			el.SetSize(editW, editH)
			el.SetMax(4)
			el.SetNumberMode()
			el.SetEscapeEvent(ui.__mem_func__(self.Close))
			el.Show()
			self.timeeditlist[idx] = el

	def __BindHuntingCooltimeEditLines(self, parentName, prefix, startIndex):
		parent = self.GetChild(parentName)
		if not self.__GetChildFromOptional(parent, "%s_i0" % prefix):
			self.__CreateHuntingCooltimeFallbackRow(parent, startIndex)
			return
		for col in xrange(6):
			idx = startIndex + col
			bg = parent.GetChild("%s_i%d" % (prefix, col))
			el = bg.GetChild("%s_e%d" % (prefix, col))
			el.SetMax(4)
			el.SetNumberMode()
			el.SetEscapeEvent(ui.__mem_func__(self.Close))
			self.timeeditlist[idx] = el

	def __SetupHuntingCooltimeEditLines(self):
		# PythonScriptLoader: tum adli kontroller kok AutoWindow uzerinde (autowindow.py / guncel autohuntingwindow).
		self.timeeditlist = {}
		if self.__GetChildOptional("editline0"):
			for i in xrange(24):
				el = self.__GetChildOptional("editline%d" % i)
				if not el:
					self.timeeditlist = {}
					break
				el.SetMax(4)
				el.SetNumberMode()
				el.SetEscapeEvent(ui.__mem_func__(self.Close))
				self.timeeditlist[i] = el
			if len(self.timeeditlist) == 24:
				self.__cooltimeEditCount = 24
				return
			self.timeeditlist = {}
		wSkill1 = self.__GetChildOptional("SkillCoolTimeEditLineWindow_1")
		if wSkill1 and self.__GetChildFromOptional(wSkill1, "hunt_s1_i0"):
			self.__BindHuntingCooltimeEditLines("SkillCoolTimeEditLineWindow_1", "hunt_s1", 0)
			self.__BindHuntingCooltimeEditLines("SkillCoolTimeEditLineWindow_2", "hunt_s2", 6)
			self.__BindHuntingCooltimeEditLines("PotionCoolTimeEditLineWindow_1", "hunt_p1", 12)
			self.__BindHuntingCooltimeEditLines("PotionCoolTimeEditLineWindow_2", "hunt_p2", 18)
			self.__cooltimeEditCount = 24
			return
		if wSkill1:
			self.__CreateHuntingCooltimeFallbackRow(wSkill1, 0)
			wSkill2 = self.__GetChildOptional("SkillCoolTimeEditLineWindow_2")
			if wSkill2:
				self.__CreateHuntingCooltimeFallbackRow(wSkill2, 6)
			wPot1 = self.__GetChildOptional("PotionCoolTimeEditLineWindow_1")
			if wPot1:
				self.__CreateHuntingCooltimeFallbackRow(wPot1, 12)
			wPot2 = self.__GetChildOptional("PotionCoolTimeEditLineWindow_2")
			if wPot2:
				self.__CreateHuntingCooltimeFallbackRow(wPot2, 18)
			self.__cooltimeEditCount = 24

	def __LoadWindow(self):
		try:
			pyScrLoader = ui.PythonScriptLoader()
			pyScrLoader.LoadScriptFile(self, "UIScript/Autohuntingwindow.py")
			board = self.GetChild("board")
			board.SetCloseEvent(ui.__mem_func__(self.Close))
			# Pencere tasima/drag bitince Start/End gorunumu bozulabiliyor; tek sefer senkronla.
			board.SetOnMouseLeftButtonUpEvent(ui.__mem_func__(self.__OnBoardMouseLeftUp))

			self.__layoutHunting = True
			self.__useToggleButtons = True
			self.autoonoffbuttonlist = []

			startBtn = self.GetChild("StartButton")
			startBtn.SetEvent(ui.__mem_func__(self.AutoOnOff), 1, self.AUTO_ONOFF_START, 0)
			if hasattr(startBtn, "SetOverOutEvent"):
				startBtn.SetOverOutEvent(ui.__mem_func__(self.__OnStartStopHoverOut))
			self.autoonoffbuttonlist.append(startBtn)
			endBtn = self.GetChild("EndButton")
			endBtn.SetEvent(ui.__mem_func__(self.AutoOnOff), 0, self.AUTO_ONOFF_START, 1)
			if hasattr(endBtn, "SetOverOutEvent"):
				endBtn.SetOverOutEvent(ui.__mem_func__(self.__OnStartStopHoverOut))
			self.autoonoffbuttonlist.append(endBtn)

			saveBtn = self.__GetChildOptional("SaveButton")
			if saveBtn:
				saveBtn.SetEvent(ui.__mem_func__(self.__OnSaveAutoButton))

			autoAttackToggle = self.__GetChildOptional("AutoAttackButton")
			if autoAttackToggle:
				self.__toggleButtons[self.AUTO_ONOFF_ATTACK] = autoAttackToggle
				autoAttackToggle.SetToggleDownEvent(ui.__mem_func__(self.AutoOnOff), 1, self.AUTO_ONOFF_ATTACK, 0)
				autoAttackToggle.SetToggleUpEvent(ui.__mem_func__(self.AutoOnOff), 0, self.AUTO_ONOFF_ATTACK, 0)

			autoRangeToggle = self.__GetChildOptional("AutoRangeButton")
			if autoRangeToggle:
				self.__toggleButtons[self.AUTO_ONOFF_AUTO_RANGE] = autoRangeToggle
				autoRangeToggle.SetToggleDownEvent(ui.__mem_func__(self.AutoOnOff), 1, self.AUTO_ONOFF_AUTO_RANGE, 0)
				autoRangeToggle.SetToggleUpEvent(ui.__mem_func__(self.AutoOnOff), 0, self.AUTO_ONOFF_AUTO_RANGE, 0)

			autoPotionToggle = self.__GetChildOptional("AutoPotionButton")
			if autoPotionToggle:
				self.__toggleButtons[self.AUTO_ONOFF_POSITION] = autoPotionToggle
				autoPotionToggle.SetToggleDownEvent(ui.__mem_func__(self.AutoOnOff), 1, self.AUTO_ONOFF_POSITION, 0)
				autoPotionToggle.SetToggleUpEvent(ui.__mem_func__(self.AutoOnOff), 0, self.AUTO_ONOFF_POSITION, 0)

			autoSkillToggle = self.__GetChildOptional("AutoSkillButton")
			if autoSkillToggle:
				self.__toggleButtons[self.AUTO_ONOFF_SKILL] = autoSkillToggle
				autoSkillToggle.SetToggleDownEvent(ui.__mem_func__(self.AutoOnOff), 1, self.AUTO_ONOFF_SKILL, 0)
				autoSkillToggle.SetToggleUpEvent(ui.__mem_func__(self.AutoOnOff), 0, self.AUTO_ONOFF_SKILL, 0)

			self.__restartToggleButton = None
			restartToggle = self.__GetChildOptional("AutoRestartHereButton")
			if restartToggle:
				self.__restartToggleButton = restartToggle
				restartToggle.SetToggleDownEvent(ui.__mem_func__(self.__OnAutoRestartToggle), True)
				restartToggle.SetToggleUpEvent(ui.__mem_func__(self.__OnAutoRestartToggle), False)

			self.AutoSkillClearButton = self.GetChild("SkillSettingClearButton")
			self.AutoSkillClearButton.SetEvent(ui.__mem_func__(self.AutoSkillClear))
			self.AutoPositionClearButton = self.GetChild("PotionSettingClearButton")
			self.AutoPositionClearButton.SetEvent(ui.__mem_func__(self.AutoPositionClear))
			self.AutoAllClearButton = self.GetChild("SettingAllClearButton")
			self.AutoAllClearButton.SetEvent(ui.__mem_func__(self.AutoAllClear))

			self.__SetupHuntingCooltimeEditLines()

			self.slotSkill = self.GetChild("AutoHuntingSkillSlot")
			self.slotPotion = self.GetChild("AutoHuntingPotionSlot")
			self.autoslot = self.slotSkill

			self.slotSkill.SetSlotStyle(wndMgr.SLOT_STYLE_NONE)
			self.slotSkill.SetSelectEmptySlotEvent(ui.__mem_func__(self.__HuntingSkillEmptySlot))
			self.slotSkill.SetSelectItemSlotEvent(ui.__mem_func__(self.__HuntingSkillSelectSlot))
			self.slotSkill.SetOverInItemEvent(ui.__mem_func__(self.__HuntingSkillOverIn))
			self.slotSkill.SetOverOutItemEvent(ui.__mem_func__(self.OverSkillSlotOutItem))
			self.slotSkill.Show()

			self.slotPotion.SetSlotStyle(wndMgr.SLOT_STYLE_NONE)
			self.slotPotion.SetSelectEmptySlotEvent(ui.__mem_func__(self.__HuntingPotionEmptySlot))
			self.slotPotion.SetSelectItemSlotEvent(ui.__mem_func__(self.__HuntingPotionSelectSlot))
			self.slotPotion.SetOverInItemEvent(ui.__mem_func__(self.__HuntingPotionOverIn))
			self.slotPotion.SetOverOutItemEvent(ui.__mem_func__(self.OverSkillSlotOutItem))
			self.slotPotion.Show()

			self.AutoToolTipButton = self.__GetChildOptional("HelpButton")
			if not self.AutoToolTipButton:
				self.AutoToolTipButton = self.__GetChildOptional("AutoToolTipButton")
			if not self.AutoToolTipButton:
				self.AutoToolTipButton = self.__GetChildOptional("AutoToolTIpButton")
			self.AutoToolTip = self.__CreateGameTypeToolTip(localeInfo.AUTO_TOOLTIP_TITLE,self.AutoSystemToolTipList)
			self.AutoToolTip.SetTop()
			if self.AutoToolTipButton:
				self.AutoToolTipButton.SetToolTipWindow(self.AutoToolTip)

			atkOptBtn = self.__GetChildOptional("AutoAttackOptionButton")
			if atkOptBtn:
				atkOptBtn.SetEvent(ui.__mem_func__(self.__OpenAutoAttackOption))
			rngSelBtn = self.__GetChildOptional("AutoRangeSelectButton")
			if rngSelBtn:
				rngSelBtn.SetEvent(ui.__mem_func__(self.__OpenAutoRangeOption))

			self.__RefreshHuntingToggleUiFromPlayer()
			self.__RefreshStartStopButtonsVisual()
		
		except:
			import exception
			exception.Abort("AutoWindow.__LoadWindow.UIScript/Autohuntingwindow.py")

	def __RefreshStartStopButtonsVisual(self):
		if not self.__layoutHunting or len(self.autoonoffbuttonlist) < 2:
			return
		startBtn = self.autoonoffbuttonlist[0]
		endBtn = self.autoonoffbuttonlist[1]
		if self.autostartonoff:
			startBtn.Down()
			endBtn.SetUp()
		else:
			endBtn.Down()
			startBtn.SetUp()

	def __OnBoardMouseLeftUp(self):
		self.__RefreshStartStopButtonsVisual()

	def __OnStartStopHoverOut(self):
		self.__RefreshStartStopButtonsVisual()

	def __OnSaveAutoButton(self):
		self.SaveAutoInfo()

	def __AutohuntExtraPath(self):
		n = chr.GetName()
		if str(n) == "0":
			return None
		return "UserData/" + str(n) + ".autohunt_opt"

	def LoadAutohuntExtraOptions(self):
		p = self.__AutohuntExtraPath()
		if not p:
			return
		handle = None
		try:
			handle = app.OpenTextFile(p)
			cnt = app.GetTextFileLineCount(handle)
			if cnt < 4:
				return
			px = float(app.GetTextFileLine(handle, 0).strip())
			mn = int(app.GetTextFileLine(handle, 1).strip())
			mx = int(app.GetTextFileLine(handle, 2).strip())
			mk = int(app.GetTextFileLine(handle, 3).strip())
			fn = getattr(player, "SetAutoHuntFocusRadiusPx", None)
			if fn:
				fn(px)
			fn2 = getattr(player, "SetAutoHuntAttackFilter", None)
			if fn2:
				fn2(mn, mx, mk)
		except IOError:
			pass
		except ValueError:
			pass
		except RuntimeError:
			pass
		finally:
			if handle is not None:
				try:
					app.CloseTextFile(handle)
				except:
					pass

	def SaveAutohuntExtraOptions(self):
		p = self.__AutohuntExtraPath()
		if not p:
			return
		try:
			import os
			d = os.path.dirname(p)
			if d and not os.path.exists(d):
				os.makedirs(d)
			gpx = getattr(player, "GetAutoHuntFocusRadiusRawPx", None)
			px = gpx() if gpx else 0.0
			gf = getattr(player, "GetAutoHuntAttackFilter", None)
			if gf:
				mn, mx, mk = gf()
			else:
				mn, mx, mk = -999, 999, 0x7F
			f = open(p, "w")
			f.write(str(px) + "\n")
			f.write(str(mn) + "\n")
			f.write(str(mx) + "\n")
			f.write(str(mk) + "\n")
			if hasattr(f, "close"):
				f.close()
		except IOError:
			pass

	def __OpenAutoAttackOption(self):
		if not getattr(player, "SetAutoHuntAttackFilter", None):
			return
		if not self._attackOptDlg:
			import uiautooptionwindows
			self._attackOptDlg = uiautooptionwindows.AutoAttackOptionDialog(self)
		self._attackOptDlg.Open()

	def __OpenAutoRangeOption(self):
		if not getattr(player, "SetAutoHuntFocusRadiusPx", None):
			return
		if not self._rangeOptDlg:
			import uiautooptionwindows
			self._rangeOptDlg = uiautooptionwindows.AutoHuntingRangeDialog(self)
		self._rangeOptDlg.Open()

	def __OnAutoRestartToggle(self, state):
		fn = getattr(player, "SetAutoRestart", None)
		if fn:
			fn(state)
		self.__ApplyRestartToggleVisual(bool(state))

	def __AutoToggleLabelOn(self):
		# UIScript locale_interface (uiScriptLocale); locale_game'de olmayabilir.
		import uiScriptLocale
		return getattr(uiScriptLocale, "AUTO_ON", None) or getattr(localeInfo, "AUTO_ON", None) or "A?"

	def __AutoToggleLabelOff(self):
		import uiScriptLocale
		return getattr(uiScriptLocale, "AUTO_OFF", None) or getattr(localeInfo, "AUTO_OFF", None) or "Kapal??"

	def __ApplyHuntingToggleVisual(self, typeKey, onoff):
		if not self.__useToggleButtons:
			return
		btn = self.__toggleButtons.get(typeKey)
		if not btn:
			return
		on = bool(onoff)
		if on:
			btn.SetText(self.__AutoToggleLabelOn())
			btn.Down()
		else:
			btn.SetText(self.__AutoToggleLabelOff())
			btn.SetUp()

	def __ApplyRestartToggleVisual(self, onoff):
		btn = self.__restartToggleButton
		if not btn:
			return
		on = bool(onoff)
		if on:
			btn.SetText(self.__AutoToggleLabelOn())
			btn.Down()
		else:
			btn.SetText(self.__AutoToggleLabelOff())
			btn.SetUp()

	def __RefreshHuntingToggleUiFromPlayer(self):
		if not (self.__layoutHunting and self.__useToggleButtons):
			return
		pairs = (
			(self.AUTO_ONOFF_ATTACK, "GetAutoAttackOnOff"),
			(self.AUTO_ONOFF_AUTO_RANGE, "GetAutoRangeOnOff"),
			(self.AUTO_ONOFF_POSITION, "GetAutoPositionOnOff"),
			(self.AUTO_ONOFF_SKILL, "GetAutoSkillOnOff"),
		)
		for key, name in pairs:
			fn = getattr(player, name, None)
			if not fn:
				continue
			try:
				self.__ApplyHuntingToggleVisual(key, fn() != 0)
			except:
				pass
		fnR = getattr(player, "GetAutoRestart", None)
		if fnR and self.__restartToggleButton:
			try:
				self.__ApplyRestartToggleVisual(fnR() != 0)
			except:
				pass

	def __HuntingSkillEmptySlot(self, localIndex):
		self.SelectActiveSkillEmptySlot(localIndex)

	def __HuntingSkillSelectSlot(self, localIndex):
		self.SelectActiveSkillSlot(localIndex)

	def __HuntingSkillOverIn(self, localIndex):
		self.OverActiveSkillSlot(localIndex)

	def __HuntingPotionEmptySlot(self, localIndex):
		self.SelectActiveSkillEmptySlot(localIndex + 13)

	def __HuntingPotionSelectSlot(self, localIndex):
		self.SelectActiveSkillSlot(localIndex + 13)

	def __HuntingPotionOverIn(self, localIndex):
		self.OverActiveSkillSlot(localIndex + 13)

	def __CreateGameTypeToolTip(self, title, descList):
		toolTip = uiToolTip.ToolTip()
		toolTip.SetTitle(title)
		toolTip.AppendSpace(5)

		for desc in descList:
			toolTip.AutoAppendTextLine(desc)

		toolTip.AlignHorizonalCenter()
		toolTip.SetTop()
		return toolTip

	def AutoSkillClear(self):
		if self.GetAutoStartonoff() == False:
			player.ClearAutoSKillSlot()
			self.RefreshAutoSkillSlot()
			for i in xrange(player.AUTO_SKILL_SLOT_MAX):
				self.autoslotindex[i] = 0

	def AutoPositionClear(self):
		if self.GetAutoStartonoff() == False:
			player.ClearAutoPositionSlot()
			self.RefreshAutoPositionSlot()
			for i in range(player.AUTO_POSITINO_SLOT_START,player.AUTO_POSITINO_SLOT_MAX):
				self.autoslotindex[i] = player.ITEM_SLOT_COUNT
	
	def AutoAllClear(self):
		if self.GetAutoStartonoff() == False:
			player.ClearAutoAllSlot()	
			self.RefreshAutoSkillSlot()
			self.RefreshAutoPositionSlot()
			for i in xrange(player.AUTO_SKILL_SLOT_MAX):
				self.autoslotindex[i] = 0
			for i in range(player.AUTO_POSITINO_SLOT_START,player.AUTO_POSITINO_SLOT_MAX):
				self.autoslotindex[i] = player.ITEM_SLOT_COUNT

	def IsNumberic(self, text) :
		try :
			int(text)
			return True
		except ValueError :
			return False

	def CheckCooltimeText(self, cooltime):
			if cooltime == "":
				return 0
			if not self.IsNumberic(cooltime):
				return 0
			return cooltime
	
	## ??? OnOff ????.
	def AutoOnOff(self, onoff,type,number,command = False):
		
		if not self.isloded:
			return

		if type == self.AUTO_ONOFF_START:
			if player.CanStartAuto() == False:
				return
			try:
				if onoff == 1:
					## ??? ????
					for i in xrange(player.AUTO_SKILL_SLOT_MAX):
						el = self.timeeditlist.get(i)
						if not el:
							continue
						cooltime = el.GetText()
						cooltime = self.CheckCooltimeText(cooltime)
						cooltime = player.CheckSkillSlotCoolTime(i,self.autoslotindex[i],int(cooltime))
						if self.autoslotindex[i] == 0:
							el.SetText("")
						if not cooltime == 0:
							player.SetAutoSlotCoolTime(i,int(cooltime))
							el.SetText(str(cooltime))

					## ???? ????
					for i in range(self.AUTO_POTION_CPP_SLOT_MIN, player.AUTO_POSITINO_SLOT_MAX):
						el = self.timeeditlist.get(i - 1)
						if not el:
							continue
						cooltime = el.GetText()
						cooltime = self.CheckCooltimeText(cooltime)
						cooltime = player.CheckPositionSlotCoolTime(i, self.autoslotindex[i], int(cooltime))
						if not cooltime == 0:
							player.SetAutoSlotCoolTime(i, int(cooltime))
							el.SetText(str(cooltime))
				else:
					for i in range(self.AUTO_POTION_CPP_SLOT_MIN, player.AUTO_POSITINO_SLOT_MAX):
						self.SetAutoCooltime(i, 0)

				player.AutoStartOnOff(onoff)
				self.autostartonoff = onoff
				if app.ENABLE_MESSENGER_RENEWAL and self.__communityAutoHuntSyncHandler:
					self.__communityAutoHuntSyncHandler(onoff != 0)
				self.__RefreshStartStopButtonsVisual()
			finally:
				pass
			
		elif type == self.AUTO_ONOFF_ATTACK:
			player.AutoAttackOnOff(onoff)

		elif type == self.AUTO_ONOFF_SKILL:
			player.AutoSkillOnOff(onoff)
		elif type == self.AUTO_ONOFF_POSITION:
			player.AutoPositionOnOff(onoff)
		elif type == self.AUTO_ONOFF_AUTO_RANGE:
			player.AutoRangeOnOff(onoff)

		if command == True:
			if onoff == False:
				self.Close()
				return

		if self.__layoutHunting:
			if type != self.AUTO_ONOFF_START:
				self.__ApplyHuntingToggleVisual(type, onoff != 0)
			return

		if self.__useToggleButtons and type != self.AUTO_ONOFF_START:
			return

		self.autoonoffbuttonlist[number].Down()
		self.autoonoffbuttonlist[number].Disable()
		if onoff == 1:
			number = number+1
		else:
			number = number-1
		self.autoonoffbuttonlist[number].SetUp()
		self.autoonoffbuttonlist[number].Enable()

	if app.ENABLE_MESSENGER_RENEWAL:
		def SetCommunityAutoHuntSyncHandler(self, handler):
			self.__communityAutoHuntSyncHandler = handler
		
	def LoadAutoWindow(self):
		if self.isloded == 0:
			self.isloded = 1
			self.__LoadWindow()
			self.SetCenterPosition()
			self.ReadAutoInfo()
	
	def Show(self):
		if self.isloded == 0:
			self.isloded = 1
			self.__LoadWindow()
			self.SetCenterPosition()

		self.SetTop()
		self.ReadAutoInfo()
		self.RefreshAutoPositionSlot()
		self.RefreshAutoSkillSlot()
		self.isOpen = 1
		
		if not item.CheckAffect(chr.NEW_AFFECT_AUTO_USE,0):
			if (not self.__useToggleButtons) and len(self.autoonoffbuttonlist) > 6:
				for i in range(4,7):
					self.autoonoffbuttonlist[i].Down()
					self.autoonoffbuttonlist[i].Disable()
			player.AutoSkillOnOff(0)
			player.AutoPositionOnOff(0)

		# K / gorev cubugu ile ayar paneli her zaman acilmali; sunucu auto_on gondermese bile
		# chrmgr.GetAutoOnOff() False kalir ve eski kod pencereyi hic Show etmiyordu.
		ui.ScriptWindow.Show(self)
		self.__RefreshHuntingToggleUiFromPlayer()
		self.__RefreshStartStopButtonsVisual()

	def ReadAutoInfo(self):
	
		if (str)(chr.GetName()) == "0":
			return

		import os
		p = 'UserData/' + chr.GetName()
		if os.path.exists(p) is False:
			return

		lines = []
		try:
			f = open(p, 'r')
			lines = f.readlines()
			f.close()
		except:
			return

		count = len(lines) / 2
		index = 0

		if count > 0:
			for slotindex in xrange(count):
				slotline = lines[index].strip()

				if slotindex < player.AUTO_SKILL_SLOT_MAX:
					player.SetAutoSkillSlotIndex(slotindex,int(slotline))
				else:
					cppSlot = slotindex + 1
					if cppSlot >= player.AUTO_POSITINO_SLOT_MAX:
						index += 2
						continue
					invPos = int(slotline)
					if invPos > 0 and invPos < player.ITEM_SLOT_COUNT:
						player.SetAutoPositionSlotIndex(cppSlot, invPos)

				line = lines[index + 1].strip() if (index + 1) < len(lines) else ""
				if not line == "":
					if slotindex < player.AUTO_SKILL_SLOT_MAX:
						cooltime = player.CheckSkillSlotCoolTime(slotindex,int(slotline),int(line))
						player.SetAutoSlotCoolTime(slotindex,int(cooltime))
						el = self.timeeditlist.get(slotindex)
						if el:
							el.SetText(str(cooltime))
					else:
						invPos2 = int(slotline)
						cppSlot = slotindex + 1
						if invPos2 > 0 and invPos2 < player.ITEM_SLOT_COUNT:
							cooltime = player.CheckPositionSlotCoolTime(cppSlot, invPos2, int(line))
							player.SetAutoSlotCoolTime(cppSlot, int(cooltime))
							el = self.timeeditlist.get(slotindex)
							if el:
								el.SetText(str(cooltime))

				index +=2
				
		self.isFirstReadFile = True

		self.LoadAutohuntExtraOptions()

		self.RefreshAutoPositionSlot()
		self.RefreshAutoSkillSlot()
		
				
	def SaveAutoInfo(self):
	
		if (str)(chr.GetName()) == "0":
			return
			
		import os
		if os.path.exists('UserData') is False:
			os.makedirs('UserData')

		output_AutoSystemFile = open('UserData/'+chr.GetName(), 'w')

		for slotindex in xrange(player.AUTO_SKILL_SLOT_MAX):
			linestr = str( self.autoslotindex[slotindex] ) + '\n'
			output_AutoSystemFile.write(linestr)
			
			el = self.timeeditlist.get(slotindex)
			if el and not el.GetText() == "":
				cooltime = player.CheckSkillSlotCoolTime(slotindex,self.autoslotindex[slotindex],int(el.GetText()))
				linestr = str(cooltime) + '\n'
			else:
				linestr = el.GetText() + '\n' if el else '\n'
			output_AutoSystemFile.write(linestr)
			

		for cppSlot in range(self.AUTO_POTION_CPP_SLOT_MIN, player.AUTO_POSITINO_SLOT_MAX):
			pos = self.autoslotindex.get(cppSlot, player.ITEM_SLOT_COUNT)
			el = self.timeeditlist.get(cppSlot - 1)
			if pos == 0 or pos == player.ITEM_SLOT_COUNT:
				output_AutoSystemFile.write('0\n')
				output_AutoSystemFile.write('\n')
				continue
			output_AutoSystemFile.write(str(pos) + '\n')
			if el and el.GetText() != "":
				cooltime = player.CheckPositionSlotCoolTime(cppSlot, pos, int(el.GetText()))
				output_AutoSystemFile.write(str(cooltime) + '\n')
			else:
				output_AutoSystemFile.write(el.GetText() + '\n' if el else '\n')
			
		output_AutoSystemFile.close()
		self.SaveAutohuntExtraOptions()

	def Close(self):
		self.Hide()
		self.isOpen = 0
		if self._attackOptDlg:
			self._attackOptDlg.Hide()
		if self._rangeOptDlg:
			self._rangeOptDlg.Hide()
		self.SaveAutoInfo()
		self.EditLineKillFocus()
		
	def EditLineKillFocus(self):
		for x in xrange(self.__cooltimeEditCount):
			el = self.timeeditlist.get(x)
			if el:
				el.KillFocus()

	def Destroy(self):
		self.isloded = 0
		self.Hide()
		if self._attackOptDlg:
			self._attackOptDlg.Destroy()
			self._attackOptDlg = None
		if self._rangeOptDlg:
			self._rangeOptDlg.Destroy()
			self._rangeOptDlg = None
		if 0 != self.tooltipSkill:
			self.tooltipSkill.HideToolTip()
			
	## ??? ???? ???? ##
	def OnActivateSkill(self):
		if self.isOpen:
			self.RefreshAutoSkillSlot()
	
	def OnDeactivateSkill(self, slotindex):
		if self.isOpen:
			for i in xrange(player.AUTO_SKILL_SLOT_MAX):
				(Position) = player.GetAutoSlotIndex(i)
				if slotindex == Position:
					if self.slotSkill:
						self.slotSkill.DeactivateSlot(i)
			
	def OnUseSkill(self, slotindex, coolTime):
		if self.isOpen:
			self.RefreshAutoSkillSlot()

	def SetSkillToolTip(self, tooltip):
		self.tooltipSkill = tooltip
	
	def SetItemToolTip(self, tooltip):
		self.tooltipItem = tooltip

	def SetAutoCooltime(self, slotindex, cooltime):
		wnd, localIdx = self.__GlobalAutoSlotToWindow(slotindex)
		if wnd and localIdx >= 0:
			wnd.SetSlotCoolTime(localIdx, cooltime, 0)
		
	def SetCloseGame(self):
		self.closegame = True
		
	def GetAutoStartonoff(self):
		return self.autostartonoff
		
	def RefreshAutoPositionSlot(self):

		if not self.slotSkill or not self.slotPotion:
			return
		
		if self.closegame:
			return

		for slotindex in range(self.AUTO_POTION_CPP_SLOT_MIN, player.AUTO_POSITINO_SLOT_MAX):
			wnd, localIdx = self.__GlobalAutoSlotToWindow(slotindex)
			if not wnd or localIdx < 0:
				continue
		
			Position = player.GetAutoSlotIndex(slotindex)
			editIdx = slotindex - 1
			editLine = self.timeeditlist.get(editIdx)
			# Bos oto slot: C++ slotPos==0; Python eski dosyada ITEM_SLOT_COUNT yazilmis olabilir.
			if Position == 0 or Position == player.ITEM_SLOT_COUNT:
				wnd.ClearSlot(localIdx)
				if editLine:
					editLine.SetText("")
				self.autoslotindex[slotindex] = player.ITEM_SLOT_COUNT
				continue

			if app.ENABLE_EXTEND_INVEN_SYSTEM:
				itemIndex = player.GetItemIndex(player.SLOT_TYPE_INVENTORY, Position)
				itemCount = player.GetItemCount(player.SLOT_TYPE_INVENTORY, Position)
			else:
				itemIndex = player.GetItemIndex(Position)
				itemCount = player.GetItemCount(Position)

			if itemCount <= 1:
				itemCount = 0
				
			wnd.SetItemSlot(localIdx, itemIndex, itemCount)
			self.autoslotindex[slotindex] = Position

			coolTime = player.GetAutoSlotCoolTime(slotindex)
			if editLine and editLine.GetText() == "":
				editLine.SetText(str(coolTime))
				
			if itemIndex == 0:
				wnd.ClearSlot(localIdx)
				if editLine:
					editLine.SetText("")
				player.SetAutoPositionSlotIndex(slotindex, 0)
				self.autoslotindex[slotindex] = player.ITEM_SLOT_COUNT
				continue

		self.slotSkill.RefreshSlot()
		self.slotPotion.RefreshSlot()
		
		if self.isFirstReadFile:
			self.SaveAutoInfo()

	def RefreshAutoSkillSlot(self):

		if not self.slotSkill:
			return

		for slotindex in xrange(player.AUTO_SKILL_SLOT_MAX):
		
			Position = player.GetAutoSlotIndex(slotindex)
			
			if Position == 0:
				self.autoslot.ClearSlot(slotindex)
				el = self.timeeditlist.get(slotindex)
				if el:
					el.SetText("")
				self.autoslotindex[slotindex] = 0
				continue
	
			skillIndex = player.GetSkillIndex(Position)
			if 0 == skillIndex:
				self.autoslot.ClearSlot(slotindex)
				el = self.timeeditlist.get(slotindex)
				if el:
					el.SetText("")
				player.SetAutoSkillSlotIndex(slotindex, 0)
				self.autoslotindex[slotindex] = 0
				continue

			skillType = skill.GetSkillType(skillIndex)
			if skill.SKILL_TYPE_GUILD == skillType:
				import guild
				skillGrade = 0
				skillLevel = guild.GetSkillLevel(Position)
			else:
				skillGrade = player.GetSkillGrade(Position)
				skillLevel = player.GetSkillLevel(Position)

			self.autoslot.SetSkillSlotNew(slotindex, skillIndex, skillGrade, skillLevel)
			self.autoslot.SetSlotCountNew(slotindex, skillGrade, skillLevel)
			self.autoslot.SetCoverButton(slotindex)

			## NOTE : CoolTime ??
			if player.IsSkillCoolTime(Position):
				(coolTime, elapsedTime) = player.GetSkillCoolTime(Position)
				self.autoslot.SetSlotCoolTime(slotindex, coolTime, elapsedTime)

			## NOTE : Activate ??? ???? ??????? ???????
			if player.IsSkillActive(Position):
				self.autoslot.ActivateSlot(slotindex)

			self.autoslotindex[slotindex] = Position

			## ????? ????
			coolTime = player.GetAutoSlotCoolTime(slotindex)
			el = self.timeeditlist.get(slotindex)
			if el and el.GetText() == "":
				el.SetText(str(coolTime))
			
		self.autoslot.RefreshSlot()

	def AddAutoSlot(self, slotindex):
		AttachedSlotType = mouseModule.mouseController.GetAttachedType()
		AttachedSlotNumber = mouseModule.mouseController.GetAttachedSlotNumber()
		AttachedItemIndex = mouseModule.mouseController.GetAttachedItemIndex()
		
		if slotindex < player.AUTO_SKILL_SLOT_MAX:
			if player.SLOT_TYPE_SKILL == AttachedSlotType:
				player.SetAutoSkillSlotIndex(slotindex,AttachedSlotNumber)
				self.RefreshAutoSkillSlot()
			elif player.SLOT_TYPE_AUTO == AttachedSlotType:
				if slotindex == AttachedSlotNumber:
					return
				if AttachedSlotNumber >= player.AUTO_SKILL_SLOT_MAX:
					return
				player.SetAutoSkillSlotIndex(slotindex,AttachedItemIndex)
				self.RefreshAutoSkillSlot()
		else:
			if player.SLOT_TYPE_INVENTORY == AttachedSlotType:
				itemIndex = player.GetItemIndex(AttachedSlotNumber)
				item.SelectItem(itemIndex)
				ItemType		= item.GetItemType()
				ItemSubType	= item.GetItemSubType()
				itemRemaintime = 0

				if not ItemType == item.ITEM_TYPE_USE:
					return;
					
				if ItemSubType == item.USE_ABILITY_UP:
					itemRemaintime = item.GetValue(1)
				elif ItemSubType == item.USE_AFFECT:
					itemRemaintime = item.GetValue(3)

				if ItemSubType == item.USE_POTION \
				or ItemSubType == item.USE_ABILITY_UP \
				or ItemSubType == item.USE_POTION_NODELAY \
				or ItemSubType == item.USE_AFFECT:
					if itemRemaintime < 9999:
						player.SetAutoPositionSlotIndex(slotindex,AttachedSlotNumber)
						self.RefreshAutoPositionSlot()

			elif player.SLOT_TYPE_AUTO == AttachedSlotType:
				if slotindex == AttachedSlotNumber:
					return
				if AttachedSlotNumber < player.AUTO_SKILL_SLOT_MAX:
					return
				player.SetAutoPositionSlotIndex(slotindex,AttachedItemIndex)
				self.RefreshAutoPositionSlot()
				
		mouseModule.mouseController.DeattachObject()
		
	def SelectActiveSkillEmptySlot(self, slotindex):
	
		if self.autostartonoff:
			return
			
		if True == mouseModule.mouseController.isAttached():
			self.AddAutoSlot(slotindex)

	def SelectActiveSkillSlot(self,slotindex):
		mouseModule.mouseController.AttachObject(self, player.SLOT_TYPE_AUTO, slotindex, self.autoslotindex[slotindex])

	def OverActiveSkillSlot(self,slotindex):
	
		if mouseModule.mouseController.isAttached():
			return	

		if slotindex < player.AUTO_SKILL_SLOT_MAX:
			Position = player.GetAutoSlotIndex(slotindex)
			if Position == 0:
				return
			skillIndex = player.GetSkillIndex(Position)
			skillType = skill.GetSkillType(skillIndex)
			if skill.SKILL_TYPE_GUILD == skillType:
				import guild
				skillGrade = 0
				skillLevel = guild.GetSkillLevel(Position)
			else:
				skillGrade = player.GetSkillGrade(Position)
				skillLevel = player.GetSkillLevel(Position)
			self.tooltipSkill.SetSkillNew(Position, skillIndex, skillGrade, skillLevel)
		else:
			Position = player.GetAutoSlotIndex(slotindex)
			if Position == 0 or Position == player.ITEM_SLOT_COUNT:
				return
			if app.ENABLE_EXTEND_INVEN_SYSTEM:	
				self.tooltipItem.SetInventoryItem(Position, player.SLOT_TYPE_INVENTORY)
				self.tooltipSkill.HideToolTip()
			else:
				self.tooltipItem.SetInventoryItem(Position)
				self.tooltipSkill.HideToolTip()
			
	def OverSkillSlotOutItem(self):
		if 0 != self.tooltipSkill:
			self.tooltipSkill.HideToolTip()

		if 0 != self.tooltipItem:
			self.tooltipItem.HideToolTip()			
	## ??? ???? ???? ##

	def OnPressEscapeKey(self):
		self.Close()
		return True		
		