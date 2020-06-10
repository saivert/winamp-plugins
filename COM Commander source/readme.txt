COM Commander - ReadMe

  Introduction
--------------
COM Commander is a fairly small Win32 application that monitors the
Modem Status signals on a communications port (COM) on your PC.
It turns these signals into commands that is sent to Winamp.

  How to wire it up
-------------------

Make 4 buttons connected to the pins like this:

  Button 1 ==> Pin 4 and 8
  Button 2 ==> Pin 4 and 6
  Button 3 ==> Pin 4 and 1
  Button 4 ==> Pin 4 and 9
 
Read the "Pin layout" section for more information.


  Pin layout
------------

  -------------
  \ 1 2 3 4 5 /
   \ 6 7 8 9 /
    ---------

 Fig 1: Serial port - Male - Connector side


  Pin        Description
   1          RLSD (receive line signal detect)
   2          Data (not used)
   3          Data (not used)
   4          DTR (data terminal ready)
   5          Ground (-)
   6          DSR (data set ready)
   7          RTS (request to send)
   8          CTS (clear to send)
   9          RING (ring indicator)

 Fig 2: Pin layout table


