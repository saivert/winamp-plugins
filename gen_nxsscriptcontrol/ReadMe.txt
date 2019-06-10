NxS Script Control - ReadMe
  Version 0.2
  Under development!

------------
] Overview [
------------

NxS Script Control is a General Purpose plug-in for Winamp that
introduces scripting capabilities to Winamp.

-------------------
] Script language [
-------------------
This plugin uses the Active scripting engines installed on you computer to parse the scripts.
Engines for VBScript and JScript are preinstalled with Windows.

-------------------
] Script elements [
-------------------

Properties:
  Volume  - Winamp's volume setting
  Panning - Winamp's balance setting

Instructions:
  Methods
    The available methods are:
      Play   - Makes Winamp start playing.
      Stop   - Stops playback
      Pause  - Pauses playback if playing and resumes playback if paused.
               Does nothing if playback is stopped.
      Stop   - Stops playback.
      Next   - Skips to next song.
      Prev   - Skips to previous song.
      
      GetPlaylistLength - Returns the number of playlist items.
      
      AddToLog - Adds text to the log window. Requires one string as parameter.

Callback functions:
  Winamp_OnSongChange
    Called when Winamp starts playing a new song.


-----------
] Contact [
-----------
E-Mail> saivert@email.com
Homepage> http://members.tripod.com/files_saivert/

