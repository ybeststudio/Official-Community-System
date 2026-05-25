# Add the following to this file:
		{
			"name" : "MessengerButton",
			"type" : "button",

			"x" : SCREEN_WIDTH - 76+10,
			"y" : 3 + Y_ADD_POSITION,

			## if app.ENABLE_GAME_OPTION_RENEWAL:
			## if app.ENABLE_MESSENGER_RENEWAL:								# 메신져 -> 커뮤니티 명칭 변경 
			##"tooltip_text" : uiScriptLocale.TASKBAR_MESSENGER,
			#"tooltip_text" : uiScriptLocale.TASKBAR_COMMUNITY,				# 커뮤니티 

			"default_image" : ROOT + "TaskBar/Community_Button_01.sub",
			"over_image" : ROOT + "TaskBar/Community_Button_02.sub",
			"down_image" : ROOT + "TaskBar/Community_Button_03.sub",
		},
