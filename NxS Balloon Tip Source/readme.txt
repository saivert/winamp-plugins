What's new?

Version 2.8 (July 12th 2005)
* This version includes support for MSN Messenger
  It will now updated your current "I'm listening to" text.
* Fixed the problems with HTTP streams once and for all.
  Finally got cracking with plug-in development again, and
  this time around I saw the problem from a different angle.

Version 2.78 (February 27th 2005)
* This version should work on Microsoft Windows 2000

Version 2.77 (January 1st 2005)
* Fixed song detection algorithm once again. Forgot about the
  "((lParam & 0x40000000) == 0x40000000)" check.

Version 2.76 (January 29th 2005)
* %nextsong% tag will now display the next song in JTFE plugins's queue if any
  songs are enqueued. Tech info: It uses the IPC Message "IPC_GET_NEXT_PLITEM".
* Note: %prevsong% tag uses the new Winamp IPC to fetch the previous song, but
  JTFE currently does not hook this, so it will just display the playlist item
  before the current one. Maybe I will use the Media Library to get this in a
  next version. Mmm.. Just maybe... should I bother with this??

Version 2.75 (January 13th 2005)
* Fixed: Bitrate, sample rate and channels now work, i.e. they are not zero anymore.
  This bug was introduced in version 2.73/2.74. Sorry!

Version 2.74 (January 12th 2005)
* You can specify that the balloon should be displayed when the title changes when
  playing a ShoutCAST stream or equivalent.

Version 2.73 2nd rel. (January 11th 2005)
* 2.73 contained a bug where one line of code caused an infinite loop.
  Sorry... the second release fixed that. Didn't wanna change version number
  just for that, since it was a silly (but fatal) bug.

Version 2.73 (January 9th 2005)
* This time you should never ever see the balloon popup all the time when
  playing a ShoutCAST stream. It took a lot of releases for this to be
  finally fixed. I don't know why I couldn't fix this earlier.
  Must have been drunk... 
* WRONG: You can specify that the balloon should popup once in a while when playing a
  ShoutCAST stream or equivalent. 
* The balloon can now be displayed for more than 10 seconds. The reason why it
  disappeared after 10 seconds when the timeout value was set to e.g. 25 is that
  this plugin uses a service in Windows 2000/XP to display the balloon and Windows
  hides it after 10 seconds. I'm now redisplaying the balloon if timeout value is set
  to more than 10 seconds. 
* Happy new year! But remember those who lost their lives in South-East Asia because
  of the terrible Tsunami. 

Complete version history is in the ReadMe document (gen_nxsballoontip.html).

Greetings

Thanks to ShaneH and DrO for being helpful on the forums.
