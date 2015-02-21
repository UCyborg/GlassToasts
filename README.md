# GlassToasts
Replaces Vista's ugly "balloon"popup with nice glassy ones

![screenshot](http://www.aveapps.com/img/glasstoast.png)

copyright (c) Andreas Verhoeven ave@aveapps.com, 2007.

#LICENSE
	You are free to use this code in whatever you like, however you like.

	Do not re-release any binary files resulting from these projects under
	your own name.



#WHAT?
	Two projects:
		- TrayHook,hooks into the tray to intercept WM_COPYDATA's from
			Shell_NotifyIcon() calls and send them to glasstoasts.exe
		- hooktester, GUI and code to display glasstoast.


#REMARKS:
- Output paths are hardcoded to c:\stuff. Change as you like.
- The code is a bunch of things hacked together.


#NEEDED:
- Vista PlatformSDK
- WTL 7.5+
- GDI+
