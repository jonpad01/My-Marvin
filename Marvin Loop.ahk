#NoEnv  ; Recommended for performance and compatibility with future AutoHotkey releases.
;#Warn  ; Enable warnings to assist with detecting common errors.
SendMode Input  ; Recommended for new scripts due to its superior speed and reliability.
SetWorkingDir %A_ScriptDir%  ; Ensures a consistent starting directory.



SetTitleMatchMode, slow
SetTitleMatchMode, 3



;Once the below loop exits, this resets the index count for another cycle
Loop {
	
	
	if (HockeyTime()) {  
		KillContinuum()
		Sleep, 2000
	} else { 

	;This loop cycles through for each bot then exits
		Loop, 28 {
		    CloseBadWindows()
			Sleep, 1000
			;A_Index is a variable built into the loop and it increases by 1 for each loop cycle
			BotNumber := A_Index + 8
			;This is for the log file paths
			BotName = bot%BotNumber%
			;This is for selecting a player profile
			DownKey := A_Index - 1
			;For the ship to select after entering the game
			ShipNumber := ShipSelector(BotNumber)
		
	
			;Check log file for Disconect message
			FileRead, Read, C:\Program Files (x86)\Continuum\Logs\%BotName%.log
		
			;Start the bot if it isnt running, or restart it if disconected from server
			if (RegExMatch(Read, "m)^  WARNING:") or !WinExist(BotNumber)) {			
			
			
				;Ignored if the bot isnt running, but important when restarting the bot
				while (WinExist(BotNumber)){
					CloseBadWindows()
					
					WinClose, %BotNumber%
					Sleep, 200
					WinWait, Continuum 0.40,, 20
					Sleep, 200
					WinClose, Continuum 0.40
					
					;The game wont exit if player is at low energy, so it needs to go in and disable the bot if the window wont close
					if (A_Index > 30) {
						WinActivate, %BotNumber%				
						Sleep, 8000
						SendInput, {F10 5}
						Sleep, 10000
						Winclose, Continuum (disabled)
						Sleep, 200
						WinWait, Continuum 0.40,, 20
						Sleep, 200
						WinClose, Continuum 0.40
					}
				}
		
				
				;Keep the log file small, and another thing
				FileDelete, C:\Program Files (x86)\Continuum\logs\%BotName%.log			
	
				;Starts continnum and checks if the window exists
				while (!WinExist("Continuum 0.40")) {
					CloseBadWindows()
					Run, C:\Users\Jon\Desktop\Marvin Run Folder\multicont
					WinWait, Continuum 0.40,, 5
					;sometimes the continuum process remains, and gives the dreaded fatal error message
					if (WinExist("Error", "Fatal error 000004bc")) {
						KillContinuum()
					}
				}
				
				
				Menu := WinExist("Continuum 0.40")
					
				
				;Open the profile menu, it loops to make sure the menu opened, this loop is definetly needed
				while (!WinExist("Select/Edit Profile")) {
					WinActivate, ahk_id %Menu%
					ControlSend,, ^p, ahk_id %Menu%
					WinWait, Select/Edit Profile,, 1
				}
				Profile := WinExist("Select/Edit Profile")
					
				;Send keys to select a profile, loops to make sure the profile menu closed, probably not needed
				WinActivate, ahk_id %Profile%
				ControlSend,, {PGUP 5}{DOWN %DownKey%}{ENTER}, ahk_id %Profile%									
				Sleep, 300
				while (WinExist("Select/Edit Profile")) {
					WinActivate, ahk_id %Profile%
					ControlSend,, {ENTER}, ahk_id %Profile%
				}
			
				CloseBadWindows()
				
				;Login
				WinActivate, ahk_id %menu%
				ControlSend,, {ENTER}, ahk_id %Menu%
				;Wait 35 Seconds, if there is no connection, the client will timeout
				WinWait, Continuum,, 35
				Sleep, 2000
				
				
				Loop {
					Sleep, 1000
					CloseBadWindows()
					;If the client times out it throws this window, try again
					if (WinExist("Information")) {
						WinClose, Information
						Sleep, 1000
						ControlSend,, {ENTER}, ahk_id %Menu%
						WinWait, Continuum,, 35
					}
					;if it connects to the game, the log file previously deleted will be created
					if (FileExist("C:\Program Files (x86)\Continuum\Logs\" BotName ".log")) {
						Game := WinActive("Continuum")
						break
					}
					;sometimes the game will get stuck at the entering arena screen, this should activate after 20 seconds
					if (A_Index > 20) {
						Sleep, 2000
						Send, {ESC}
						Sleep, 2000
						if (!WinExist("Continuum 0.40")) {
							Run, C:\Users\Jon\Desktop\Marvin Run Folder\multicont
						}
						WinWait, Continuum 0.40
						Menu := WinExist("Continuum 0.40")
						ControlSend,, {ENTER}, ahk_id %Menu%
						WinWait, Continuum,, 35
					}
				}
	
				;Press ESC too early and it will exit back to the menu
				Sleep, 3000
				;Marvin will default to Warbird if the client is not in a ship
				ControlSend,, {ESC}, ahk_id %Game%
				Sleep, 1000
				ControlSend,, %ShipNumber%, ahk_id %Game%												
				;Without this sleep timer the game will minimize before the player enters the ship
				Sleep, 1000
				WinMinimize, ahk_id %Game%
				Sleep, 2000
						
				
				;This selects\copys the injector terminal text and stores it in a variable
				Loop {
					CloseBadWindows()
					;When the cpu is loaded the injector text will load slowly, so check if the first chars are "1: " and last char is a ">"
					Text := 0
					while (!RegExMatch(Text, "s)^0: .*>$")) {
						CloseBadWindows()
						;check if injector is open on each iteration of this loop, so it doesnt get stuck
						;run function sometimes fails
						if (!WinExist("Injector") and !WinExist("Select Injector")) {
							Run, C:\Users\Jon\Desktop\Marvin Run Folder\Injector,, UseErrorLevel
							WinWait, Injector,, 10
							while (A_LastError != 0) {
								Run, C:\Users\Jon\Desktop\Marvin Run Folder\Injector,, UseErrorLevel
							    WinWait, Injector,, 10
							}
						}
						Sleep, 1000
						if (WinExist("Injector") {
						Injector := WinExist("Injector")
						}
						else {
						Injector := WinExist("Select Injector")
						}						
						
						ControlSend,, ^a, ahk_id %Injector%
						Sleep, 500
						ControlSend,, {ENTER}, ahk_id %Injector%
						Sleep, 500
						Text := Clipboard
					}
					Sleep, 2000
					
					;Finds the line with ")" at the end, then stores the number/s at the beginning preceding the ":" into the output, thanks Arry
					Found := RegExMatch(Text, "m)^(\d+):.+?\)$", Match)
					;this repeats the loop until the regex doesnt match to anything
					if (%Match1% >= 50) {
							KillContinuum()
							break
					}
					if (%Found% != 0) {
						ControlSend,, %Match1%, ahk_id %Injector%
						Sleep, 300
						ControlSend,, {ENTER}, ahk_id %Injector%
						Sleep, 500
					} else {
						WinWait, Continuum (enabled),, 10
						WinClose, ahk_id %Injector%
						break
					}
				}
				WinSetTitle, ahk_id %Game%,, %BotNumber%
				
				;Clean up
				while (WinExist("Continuum 0.40")) {
					CloseBadWindows()
					WinClose, Continuum 0.40
				}
				
				if (WinExist("Continuum (enabled)")) {
					WinClose, Continuum (enabled)
					WinWait, Continuum 0.40,, 10
					if (!WinExist("Continuum 0.40")) {
						WinActivate, Continuum (enabled)				
						Sleep, 8000
						SendInput, {F10 5}
						Sleep, 10000
						Winclose, Continuum (disabled)
						Sleep, 200
						WinWait, Continuum 0.40,, 20
						Sleep, 200
						WinClose, Continuum 0.40
					}
				}
					
				
				if (WinExist("Continuum")) {
					WinClose, Continuum
					WinWait, Continuum 0.40,, 10 
					if (!WinExist("Continuum 0.40")) {
						WinActivate, Continuum (enabled)				
						Sleep, 8000
						SendInput, {F10 5}
						Sleep, 10000
						Winclose, Continuum (disabled)
						Sleep, 200
						WinWait, Continuum 0.40,, 20
						Sleep, 200
						WinClose, Continuum 0.40
					}
				}
				WinClose, Injector
				while (WinExist("Continuum 0.40")) {
					CloseBadWindows()
					WinClose, Continuum 0.40
				}
				
				CloseBadWindows()
				
			}
		}
	}	
}

KillContinuum() {
	Process, Exist, Continuum.exe
	PID := ErrorLevel
					;if that happens this loop will kill all continuum processes until none remain
	while (%PID% != 0){
		RunWait, taskkill /PID %PID% /T /F
		Sleep, 200
		Process, Exist, Continuum.exe
		PID := ErrorLevel
		Sleep, 500
		if (A_Index = 100){
			MsgBox,,1, taskkill failed
		}
	}
	return
}

HockeyTime() {
	Time = %A_Hour%%A_Min%
	Day = %A_WDay%
	If (((Time > 1800 and Time < 1900) and (Day != 7 and Day != 1)) or ((Time > 1300 and Time < 1400) and (Day = 1 or Day = 7))) {  
		return True
	} else {
		return False
    }
}

	
CloseBadWindows() {
	SetTitleMatchMode, 2
		WinClose, : Windows - Application Error
		WinClose, Visual Studio Just-In-Time Debugger
	SetTitleMatchMode, 3
	Winclose, Error, Error getting Continuum.exe process handle.
	WinClose, Error
	
	if (WinExist("Microsoft Visual C++ Runtime Library")){
		WinActivate, Microsoft Visual C++ Runtime Library
		SendInput, !a
	}
	
	WinClose, Injector.exe - Application Error
	WinClose, A, bad allocation
	return
}

ShipSelector(BotNumber) {
	Number = 1
	if (BotNumber = 11){
		Number = 2
	}
	if (BotNumber = 12){
		Number = 8
	}
	if (BotNumber = 14){
		Number = 5
	}
	if (BotNumber = 15){
		Number = 3
	}
	if (BotNumber = 16){
		Number = 4
	}
	if (BotNumber = 17){
		Number = 6
	}
	if (BotNumber = 18){
		Number = 7
	}
	return Number
}
	
	
