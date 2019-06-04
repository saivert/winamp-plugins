NxS BigClock v0.1 - ReadMe

    - Remember all the lost lifes in Thailand, Sri-Lanka, Eastern India,
      Indonesia and the rest of the south-east asia. -

  Overview
----------

  NxS BigClock is a plugin that gives Winamp a separate time display window
  with these "clocks":

  - Song elapsed time
  - Song remaining time
  - Playlist elapsed time
  - Playlist remaining time
  - Time of day

It also displays sound visualizations... 


  What's new?
-------------
  Version 0.3 (February 27th 2005)
  - Much better visualization.
  - User can now choose if he/she wants the configuration dialog to be skinned.
  - Configuration dialog: Asks user if he/she wants to restart winamp if
    the Enabled option was changed.
  - Other code improvements:
    * No longer calculates the playlist total time for each frame of animation.
      Recalculates the playlist total time only when song changes.
    * Visualizations are drawn the old way if "TransparentBlt" and "AlphaBlend"
      functions are not available through GDI.

  Version 0.2 (February 7th 2005)
  - Better visualization. Uses Alpha Blending. Covers the entire window.
    Just using oscilloscope at the moment. The Spectrum analyzer sucks.
  - Bug still around: The animations are very slow when the window is
    resized too big. Must find out how to optimize this. Maybe render
    it in another thread. Threads can cause a lot of trouble too.
  - Ideas: Maybe add more display modes (what would those be??)
  
  Version 0.1:
  - First release :-)
  

  Contact
---------
  Written by Saivert
  http://inthegray.com/saivert/
  saivert@email.com
