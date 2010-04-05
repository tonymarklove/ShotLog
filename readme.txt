ShotLog
----

Automatically take screen shots of your entire desktop on a given interval.

Personally I use it as an aid to time-tracking. While it doesn't completely
negate the need to track time manually, any missing periods from my time-sheet
can be checked in the shot logs to give me a reasonable idea of what I was
doing. (Often it is then possible to cross reference with my version control
commit logs for even more detail.)


Getting Started
---

Run ShotLog to start with the default options. See options below to find out
what you can change, then run with command line switches.


Options
---

-i
	Path to icon file used in notification (tray) area. Must be in double quotes.
	
-t
	Tooltip to display when mouse over tray icon. Must be in double quotes.
	
-s
	Save path. Where the log files are saved. Must be in double quotes. Defaults
	to the current working directory.
	
-d
	Delay between shots in seconds. Miniumum of 10 seconds. Usually you will want
	this to be something like 600 (for 10 minutes) between shots. Defaults to 900
	(15 minutes).

-h
	Hide notification (tray) area icon. Useful to you plan to run ShotLog
	continuously without it getting in your way.


Thanks
---

DevIL (http://openil.sourceforge.net/) - For the easy to use image saving
library, in all the formats I could possibly need.
