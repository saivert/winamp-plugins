NxS Thinger v0.515 - ReadMe

- Remember all the lost lifes in Thailand, Sri-Lanka, Eastern India,
Indonesia and the rest of the south-east asia. -

  Overview
----------

  NxS Thinger is an attempt to port the Thinger component in
  Winamp3 to a Winamp 2.x/5.x plugin.
  It supports these "components":
    - Playlist Editor
    - Media Library
    - Video window
    - Visualization

  It also exposes an API that other plugins can use to add their own icons
  to the Thinger window (ala Global Hotkeys plugin).

  What's new?
-------------
  Version 0.53 (February 27th 2005):
  - Improved configuration dialog.
  Version 0.52 (February 6th 2005):
  - Added: Option to make icons dimmed when user presses scroll buttons
    Tech. note: This uses the "GdiAlphaBlend" function.
  - Changed: NxS Thinger is now using the "GdiTransparentBlt" GDI function instead of it's
    internal "True Mask method" to draw the icons with transparency.
  - Skipped version numbers up to 0.52
  Version 0.517 (January 30th 2005):
  - Fixed: You can now resize the thinger without seing weird stuff painted on the screen.
    I finally came to senses about the thing as the next thing shows:
  - The Thinger window now has the correct size even under Modern skins.
  - Thinger API: If icon/bitmap added is invalid, then a default one will be used.
    Previously a blank area was displayed if given an invalid icon/bitmap handle.
  Version 0.516 (January 13th 2005):
  -  Fixed: Duplicate menu item issue with Modern Skins now fixed.
  Version 0.515 (December 24th 2004 - 3rd January 2005):
  - Compiled with Visual C++ 2005 Express Edition Beta
  - Source code more compatible with IA64 (not tested on 64bit processor yet).
  - Option to select which of the built in icons to display.
  - Context-menu now only appears when you right-click the icon area.
  Version 0.514 (December 9th 2004):
  - Removed "Test: Add an icon" and "Test: Delete an icon" from the Thinger menu.
    This was misleading users who thought they could right-click icons and delete them.
  - The built in Media Library icon is now not added if the Media Library plugin (gen_ml.dll)
    is not installed.
  - Bitmaps are now drawn transparently. All areas of the bitmaps that has the color
    RGB(255,0,255) or Fuchsia will be transparent.
  Version 0.513 (December 6th 2004):
  - Bundled an example plugin project that demonstrates how to use the Thinger API.
    This plugin will be copied to "Winamp\Plugins\NxS Thinger vx.x Source\Example plugin".
	I hope this example plugin gets rid of all the misunderstandings around the API.
  - Added a new menu item to Thingers right-click menu: "Debug: Show icon info".
    Use this to display a message box with information about the icon you right-clicked.
  Version 0.512 (December 2nd to 5th 2004):
  - The NTIS_MODIFY flag now *really* works. Sorry ppl! Again...
  - The bugs when not showing statusbar is fixed.
  - The source code for this plugin now contains more comments for the interested.
  - NxSThingerAPI.h is now more documented. Read the file "NxSThingerAPI.h" for more.
  Version 0.51 (December 1st 2004):
  - A couple of bug fixes. Thanks Safai!
  - Fix: No more infinite scroll when clicking right scroll button.
  Version 0.5:
  - HTML Readme! :-)
  - The NTIS_MODIFY flag now works. Sorry ppl! :->
  - New icon for Winamp's preferences added.
  - "NxS Thinger" menu item added to Winamp's main menu.
  - "About.." menu item for Thinger's right-click menu is replaced with a "Config..." menu item.
  Version 0.4:
  - Thinger API Completed. Now you can add your own icons to the Thinger.
  - Configuration dialog fixed. It should now be of the correct size.
  Version 0.3:
  - Better looking icons. Highlights when mouse hovers over them.
  - It now behaves better when modern skins is in use. It doesn't try to enforce a
    smaller height than 116 when Modern skins is in use. It will not look as good though.
  - Tooltip for the Lightning bolt icon is now "Toggle Thinger".
  - Better looking configuration dialog.
  Version 0.2:
  - Fixed size of Thinger window.
  Version 0.1:
  - Everything...
  

  Contact
---------
  Written by Saivert
  http://inthegray.com/saivert/
  saivert@email.com
