
minichess - another stupid dock app
===================================

**Works with [ Blackbox Slit | Window Maker Dock | Afterstep Wharf ].**

<table>
  <tr>
    <td>
      At the start of a game the troops are lined up and ready to fight.  The
      war is about to begin...
    </td>
    <td>&nbsp;&nbsp;&nbsp;</td>
    <td>
      ![gcalcli](minichess/raw/master/docs/start.jpg)
    </td>
  </tr>
  <tr>
    <td>
      The battle gets more intense as time passes.  You frequently squint with
      your face inches from the monitor in search of the enemy's Queen.  You
      know she's somewhere lurking, waiting, salivating...
    </td>
    <td>&nbsp;&nbsp;&nbsp;</td>
    <td>
      ![gcalcli](minichess/raw/master/docs/middle.jpg)
    </td>
  </tr>
  <tr>
    <td>
      But then... oh no!  The Queen appears and she pushes her sword deep
      through your King's heart.  The war is over and you suck.
    </td>
    <td>&nbsp;&nbsp;&nbsp;</td>
    <td>
      ![gcalcli](minichess/raw/master/docs/end.jpg)
    </td>
  </tr>
</table>

minichess was born out of boredom.  I warn you... if your a chess enthusiast
and play a lot then please use xboard or some other chess gui.  Even though
minichess has that kinda cool factor, it does get hard on the eyes.  I like
using minichess for games in which I'm in no hurry to finish and go on for a
couple days.  Sorry but minichess has no save/restore features yet.

Note that minichess requires the 'gnuchessx' (note the x) application which is
included within the gnuchess package.

Installing
----------

1. Edit the Makefile. (namely specify the location of gnuchessx)
2. make install

Usage
-----

minichess has a variety of options:

```
   -h         help
   -t         play text game (def = mouse game)
   -r         more random moves by the engine
   -a         hard, engine will also think on your time (def = no)
   -d n       max search depth used by the engine       (def = 29)
   -c n       max time the engine gets to make a move   (def = inf)
   -T n       size of the transposition table           (def = 150001)
   -C n       size of the cache table                   (def = 18001)
   -k #color  background color for a checkmated king    (def = #ff0000)
   -b #color  background color for the black pieces     (def = #000000)
   -f #color  foreground color for the black pieces     (def = #ffffff)
   -1 #color  color for the dark squares                (def = #c8c365)
   -2 #color  color for the light squares               (def = #77a26d)
```

If you specify a search depth (-d) to (say) 4 ply, the program will search
until all moves have been examined to a depth of 4.  If you set a maximum time
(-t) per move and also use the depth option, the search will stop at the
specified time or the specified depth, whichever comes first.

Gnuchess is a cpu hog while it's thinking.  Be aware.  When the hard option
(-a) is specified then the engine thinks all the time and pins your cpu.

Also gnuchess can be a memory hog.  Specifying the size of the transposition
and cache tables will modify the memory used.  See the memory usage excerpt
from the gnuchess FAQ below.

White piece colors are the reverse of what is specified for the black pieces.

Text Game
---------

The text game (-t) option specifies that you want to play via the keyboard
instead of the mouse.  The following is a list of commands recognized.  See the
gnuchess man page for more info on making a move.

```
   [q]uit          - quit this stupid application
   [n]ew           - start a new game
   [p]rint         - print a text version of the board
   [h]int          - get a hint from the engine and use it
   [r]efresh       - refresh the xpm window
   [?]help         - this message

   Making a move (examples):

   g1f3            - move piece from g1 to f3
   a7a8q           - move pawn from and promote to a queen
   o-o or 0-0      - castle on the king side
   o-o-o or 0-0-0  - castle on the queen side
```

There are three different prompts used by the text game.  The "&gt;" prompt is
used for you to enter in commands and move.  The "H&gt;" prompt shows your hint
move that was received from the engine.  And the "C&gt;" prompt shows the engines
move.

Mouse Game
----------

The default playing mode is to use the mouse.  A move is made by left clicking
on the piece's 'from' position and then clicking on the piece's 'to' position.
The computer will beep if the move was invalid.  The following button commands
are recognized:

```
   Button1         - make a move (click on the 'from' and 'to' positions)
   Button2         - get a hint from the engine and use it
   Button3         - start a new game
   Button3+Control - quit this stupid application
```

GNUCHESS MEMORY USAGE (from the gnuchess FAQ)
---------------------------------------------

```
 [C.7] GNU Chess runs way too slow and makes my disk seek wildly.
 This happens if you don't have enough real memory (RAM) to run GNU Chess.
 You may need 16MB or more. You can reduce GNU Chess's memory requirements
 by reconfiguring it, or just buy more memory. Some (rather out of date)
 suggestions are in the file doc/PORTING from the GNU Chess source tree.

 It is perfectly possible to run gnuchess on an 8Mb system. I would suggest
 that you don't edit the source (though the defaults are the definitions of
 ttblsz or something like that in src/ttable.h and DEFETABS in
 src/gnuchess.h), but rather use the -C and -T command-line options (which
 even work for gnuan, though not documented in the manpage). The defaults
 are `-C 18001 -T 150001' (for MS-DOS, -T 8001). On my Linux system, this
 uses just over 9Mb. From memory, `-C 6001 -T 40001' uses around 3Mb. Fiddle
 with these and see what results you get. 

 Why does GNU Chess use so much memory? The extra memory lets it keep large
 hash tables that speed up its search and make it play better, and a large
 on-line book that improves opening play. If you have lots of memory you may
 want to reconfigure GNU Chess to use *more* than the default amount.
```

