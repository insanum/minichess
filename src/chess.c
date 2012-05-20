/*
 * minichess - another stupid dock app
 * Copyright (c) 1999 by Eric Davis, ead@pobox.com
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 * 
 * (See COPYING / GPL-2.0)
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <strings.h>
#include <signal.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/select.h>

#include <X11/xpm.h>
#include "xgen.h"

#include "chess_master.xpm"
#include "chess_mask.xbm"

#define DEBUG 0
#define VERSION "minichess v0.8"

extern char * optarg;
extern int optind;

extern Display * display;

/* input and output pipes to the chess engine */
FILE * input;
FILE * output;

/* pid of the chess engine */
int childPid;

/* should the engine make more random moves */
int engineRandom;

/* should the engine think during user's move */
int engineHard;

#define COLOR_LENGTH    7

/* player colors */
#define BLACK           1
#define WHITE           2

/* The pieces! */
#define EMPTY           0
#define B_PAWN          1
#define B_ROOK          2
#define B_KNIGHT        3
#define B_BISHOP        4
#define B_QUEEN         5
#define B_KING          6
#define W_PAWN          7
#define W_ROOK          8
#define W_KNIGHT        9
#define W_BISHOP        10
#define W_QUEEN         11
#define W_KING          12

/* index positions within the xpm for both black and white pieces */
#define B_PAWN_I     4,79
#define B_ROOK_I     4,86
#define B_KNIGHT_I  11,86
#define B_BISHOP_I  18,86
#define B_KING_I    25,86
#define B_QUEEN_I   32,86
#define W_PAWN_I     4,71
#define W_ROOK_I     4,64
#define W_KNIGHT_I  11,64
#define W_BISHOP_I  18,64
#define W_KING_I    25,64
#define W_QUEEN_I   32,64

/* red check-mated king */
#define CHECKMATE_KING_I 25,71

/* pixmap save positions used to save a move */
#define FROM_SAVE       41,65
#define TO_SAVE         51,65

/* size of a piece within the xpm */
#define PIECE_SIZE      7,7

/* the start and size of the two boards within the xpm */
#define BOARD           4,4
#define CLEAR_BOARD     4,94
#define BOARD_SIZE      56,56

/* index positions within the xpm for each of the board coordinates */
/* X AXIS */
#define XA      4
#define XB      11
#define XC      18
#define XD      25
#define XE      32
#define XF      39
#define XG      46
#define XH      53
/* Y AXIS */
#define Y1      53
#define Y2      46
#define Y3      39
#define Y4      32
#define Y5      25
#define Y6      18
#define Y7      11
#define Y8      4

/* index positions within the xpm for each of the 'clear' board coordinates */
/* CLEAR X AXIS */
#define C_XA    4
#define C_XB    11
#define C_XC    18
#define C_XD    25
#define C_XE    32
#define C_XF    39
#define C_XG    46
#define C_XH    53
/* CLEAR Y AXIS */
#define C_Y1    143
#define C_Y2    136
#define C_Y3    129
#define C_Y4    122
#define C_Y5    115
#define C_Y6    108
#define C_Y7    101
#define C_Y8    94

/*
 * The COORDS_T array is used to hold the from and to positions.  The
 * coordinate representation will be either in pixels within the xpm or
 * cartesian within the gameState array.
 *
 * The CLEAR_COORDS_T array is used to hold the from position.  The
 * coordinate representation will be in pixels within the 'clear' xpm.
 */
#define FROM_X  0
#define FROM_Y  1
#define TO_X    2
#define TO_Y    3
typedef int COORDS_T       [4];
typedef int CLEAR_COORDS_T [2];

/*
 * The POSITION_T array is used to hold a cartesian position point.
 */
#define POS_X   0
#define POS_Y   1
typedef int POSITION_T [2];


/* special control commands */

#define N               "n"             /* start a new game */
#define N_LEN           1

#define NEW             "new"           /* start a new game */
#define NEW_LEN         3

#define H               "h"             /* get a hint and make that move */
#define H_LEN           1

#define HINT            "hint"          /* get a hint and make that move */
#define HINT_LEN        4

#define P               "p"             /* print the board */
#define P_LEN           1

#define PRINT           "print"         /* print the board */
#define PRINT_LEN       5

#define BD              "bd"            /* print the board (gnuchess) */
#define BD_LEN          2

#define Q               "q"             /* quit the game and exit */
#define Q_LEN           1

#define QUIT            "quit"          /* quit the game and exit */
#define QUIT_LEN        4

#define R               "r"             /* refresh the xpm window */
#define R_LEN           1

#define REFRESH         "refresh"       /* refresh the xpm window */
#define REFRESH_LEN     7

#define HLP             "?"             /* print out help message */
#define HLP_LEN         1

#define HELP            "help"          /* print out help message */
#define HELP_LEN        4

/* used to determine if the game is over */
enum GAME_OVER { NO_MATE, BLACK_MATES, WHITE_MATES } checkMate;

/* used to determine whether the last move was a castle move */
enum CASTLE_MOVE { NO_CASTLE, BLACK_KING_SIDE, BLACK_QUEEN_SIDE,
                   WHITE_KING_SIDE, WHITE_QUEEN_SIDE } castleMove;

/* used to determine whether the last move was a pawn promotion */
enum PAWN_PROMOTE { NO_PROMOTE, BLACK_ROOK, BLACK_KNIGHT, BLACK_BISHOP,
                    BLACK_QUEEN, WHITE_ROOK, WHITE_KNIGHT, WHITE_BISHOP,
                    WHITE_QUEEN } pawnPromote;

/* used to hold the current state of the game */
#define BOARD_WIDTH  8  /* x axis */
#define BOARD_HEIGHT 8  /* y axis */
unsigned int gameState [BOARD_HEIGHT] [BOARD_WIDTH];
/* Initialized as follows:
  {
   { B_ROOK, B_KNIGHT, B_BISHOP, B_QUEEN, B_KING, B_BISHOP, B_KNIGHT, B_ROOK },
   { B_PAWN, B_PAWN  , B_PAWN  , B_PAWN , B_PAWN, B_PAWN  , B_PAWN  , B_PAWN },
   { EMPTY , EMPTY   , EMPTY   , EMPTY  , EMPTY , EMPTY   , EMPTY   , EMPTY  },
   { EMPTY , EMPTY   , EMPTY   , EMPTY  , EMPTY , EMPTY   , EMPTY   , EMPTY  },
   { EMPTY , EMPTY   , EMPTY   , EMPTY  , EMPTY , EMPTY   , EMPTY   , EMPTY  },
   { EMPTY , EMPTY   , EMPTY   , EMPTY  , EMPTY , EMPTY   , EMPTY   , EMPTY  },
   { W_PAWN, W_PAWN  , W_PAWN  , W_PAWN , W_PAWN, W_PAWN  , W_PAWN  , W_PAWN },
   { W_ROOK, W_KNIGHT, W_BISHOP, W_QUEEN, W_KING, W_BISHOP, W_KNIGHT, W_ROOK }
  }
*/

/*
 * cartesian state coordinates used for access to the gameState array
 * these makes acces to the gameState a bit more human readable
 */
/* X AXIS */
#define SA      0
#define SB      1
#define SC      2
#define SD      3
#define SE      4
#define SF      5
#define SG      6
#define SH      7
/* Y AXIS */
#define S1      7
#define S2      6
#define S3      5
#define S4      4
#define S5      3
#define S6      2
#define S7      1
#define S8      0


/***************************************************************************
 *
 * printHelp - print a help message for a text game
 *
 * RETURNS: N/A
 */
void printHelp
    (
    void
    )
    {
    printf("\n Text game commands:\n\n");
    printf("   [q]uit          - quit this stupid application\n");
    printf("   [n]ew           - start a new game\n");
    printf("   [p]rint         - print a text version of the board\n");
    printf("   [h]int          - get a hint from the engine and use it\n");
    printf("   [r]efresh       - refresh the xpm window\n");
    printf("   [?]help         - this message\n\n");
    printf("   Making a move (examples):\n\n");
    printf("   g1f3            - move piece from g1 to f3\n");
    printf("   a7a8q           - move pawn from and promote to a queen\n");
    printf("   o-o or 0-0      - castle on the king side\n");
    printf("   o-o-o or 0-0-0  - castle on the queen side\n\n");
    }


/***************************************************************************
 *
 * printUsage - print the usage and exit
 *
 * RETURNS: N/A
 */
void printUsage
    (
    void
    )
    {
    printf("\n %s\n", VERSION);
    printf(" Written by Eric Davis <ead@pobox.com>\n\n");
    printf(" Usage:\n\n");
    printf("   -h         help\n");
    printf("   -t         play text game (def = mouse game)\n");
    printf("   -r         more random moves by the engine\n");
    printf("   -a         hard, engine will also think on your time (def = no)\n");
    printf("   -d n       max search depth used by the engine       (def = 29)\n");
    printf("   -c n       max time the engine gets to make a move   (def = inf)\n");
    printf("   -T n       size of the transposition table           (def = 150001)\n");
    printf("   -C n       size of the cache table                   (def = 18001)\n");
    printf("   -k #color  background color for a checkmated king    (def = #ff0000)\n");
    printf("   -b #color  background color for the black pieces     (def = #000000)\n");
    printf("   -f #color  foreground color for the black pieces     (def = #ffffff)\n");
    printf("   -1 #color  color for the dark squares                (def = #c8c365)\n");
    printf("   -2 #color  color for the light squares               (def = #77a26d)\n");
    printHelp();
    printf(" Mouse game commands:\n\n");
    printf("   Button1         - make a move (click on the 'from' and 'to' positions)\n");
    printf("   Button2         - get a hint from the engine and use it\n");
    printf("   Button3         - start a new game\n");
    printf("   Button3+Control - quit this stupid application\n\n");
    exit(1);
    }

/***************************************************************************
 *
 * initBoard - initializes the game board
 *
 * This function initializes the gameState array to represent the start
 * of a chess game.
 *
 * RETURNS: N/A
 */
void initBoard
    (
    void
    )
    {
    /* initialize the gameState array */
    gameState [0] [0] = B_ROOK;
    gameState [0] [1] = B_KNIGHT;
    gameState [0] [2] = B_BISHOP;
    gameState [0] [3] = B_QUEEN;
    gameState [0] [4] = B_KING;
    gameState [0] [5] = B_BISHOP;
    gameState [0] [6] = B_KNIGHT;
    gameState [0] [7] = B_ROOK;
    gameState [1] [0] = B_PAWN;
    gameState [1] [1] = B_PAWN;
    gameState [1] [2] = B_PAWN;
    gameState [1] [3] = B_PAWN;
    gameState [1] [4] = B_PAWN;
    gameState [1] [5] = B_PAWN;
    gameState [1] [6] = B_PAWN;
    gameState [1] [7] = B_PAWN;
    gameState [2] [0] = EMPTY;
    gameState [2] [1] = EMPTY;
    gameState [2] [2] = EMPTY;
    gameState [2] [3] = EMPTY;
    gameState [2] [4] = EMPTY;
    gameState [2] [5] = EMPTY;
    gameState [2] [6] = EMPTY;
    gameState [2] [7] = EMPTY;
    gameState [3] [0] = EMPTY;
    gameState [3] [1] = EMPTY;
    gameState [3] [2] = EMPTY;
    gameState [3] [3] = EMPTY;
    gameState [3] [4] = EMPTY;
    gameState [3] [5] = EMPTY;
    gameState [3] [6] = EMPTY;
    gameState [3] [7] = EMPTY;
    gameState [4] [0] = EMPTY;
    gameState [4] [1] = EMPTY;
    gameState [4] [2] = EMPTY;
    gameState [4] [3] = EMPTY;
    gameState [4] [4] = EMPTY;
    gameState [4] [5] = EMPTY;
    gameState [4] [6] = EMPTY;
    gameState [4] [7] = EMPTY;
    gameState [5] [0] = EMPTY;
    gameState [5] [1] = EMPTY;
    gameState [5] [2] = EMPTY;
    gameState [5] [3] = EMPTY;
    gameState [5] [4] = EMPTY;
    gameState [5] [5] = EMPTY;
    gameState [5] [6] = EMPTY;
    gameState [5] [7] = EMPTY;
    gameState [6] [0] = W_PAWN;
    gameState [6] [1] = W_PAWN;
    gameState [6] [2] = W_PAWN;
    gameState [6] [3] = W_PAWN;
    gameState [6] [4] = W_PAWN;
    gameState [6] [5] = W_PAWN;
    gameState [6] [6] = W_PAWN;
    gameState [6] [7] = W_PAWN;
    gameState [7] [0] = W_ROOK;
    gameState [7] [1] = W_KNIGHT;
    gameState [7] [2] = W_BISHOP;
    gameState [7] [3] = W_QUEEN;
    gameState [7] [4] = W_KING;
    gameState [7] [5] = W_BISHOP;
    gameState [7] [6] = W_KNIGHT;
    gameState [7] [7] = W_ROOK;

    /* draw a new checkerboard */
    copyXPMArea(CLEAR_BOARD, BOARD_SIZE, BOARD);

    /* draw the black pieces */
    copyXPMArea( B_ROOK_I,   PIECE_SIZE, XA, Y8 );
    copyXPMArea( B_KNIGHT_I, PIECE_SIZE, XB, Y8 );
    copyXPMArea( B_BISHOP_I, PIECE_SIZE, XC, Y8 );
    copyXPMArea( B_QUEEN_I,  PIECE_SIZE, XD, Y8 );
    copyXPMArea( B_KING_I,   PIECE_SIZE, XE, Y8 );
    copyXPMArea( B_BISHOP_I, PIECE_SIZE, XF, Y8 );
    copyXPMArea( B_KNIGHT_I, PIECE_SIZE, XG, Y8 );
    copyXPMArea( B_ROOK_I,   PIECE_SIZE, XH, Y8 );
    copyXPMArea( B_PAWN_I,   PIECE_SIZE, XA, Y7 );
    copyXPMArea( B_PAWN_I,   PIECE_SIZE, XB, Y7 );
    copyXPMArea( B_PAWN_I,   PIECE_SIZE, XC, Y7 );
    copyXPMArea( B_PAWN_I,   PIECE_SIZE, XD, Y7 );
    copyXPMArea( B_PAWN_I,   PIECE_SIZE, XE, Y7 );
    copyXPMArea( B_PAWN_I,   PIECE_SIZE, XF, Y7 );
    copyXPMArea( B_PAWN_I,   PIECE_SIZE, XG, Y7 );
    copyXPMArea( B_PAWN_I,   PIECE_SIZE, XH, Y7 );

    /* draw the white pieces */
    copyXPMArea( W_ROOK_I,   PIECE_SIZE, XA, Y1 );
    copyXPMArea( W_KNIGHT_I, PIECE_SIZE, XB, Y1 );
    copyXPMArea( W_BISHOP_I, PIECE_SIZE, XC, Y1 );
    copyXPMArea( W_QUEEN_I,  PIECE_SIZE, XD, Y1 );
    copyXPMArea( W_KING_I,   PIECE_SIZE, XE, Y1 );
    copyXPMArea( W_BISHOP_I, PIECE_SIZE, XF, Y1 );
    copyXPMArea( W_KNIGHT_I, PIECE_SIZE, XG, Y1 );
    copyXPMArea( W_ROOK_I,   PIECE_SIZE, XH, Y1 );
    copyXPMArea( W_PAWN_I,   PIECE_SIZE, XA, Y2 );
    copyXPMArea( W_PAWN_I,   PIECE_SIZE, XB, Y2 );
    copyXPMArea( W_PAWN_I,   PIECE_SIZE, XC, Y2 );
    copyXPMArea( W_PAWN_I,   PIECE_SIZE, XD, Y2 );
    copyXPMArea( W_PAWN_I,   PIECE_SIZE, XE, Y2 );
    copyXPMArea( W_PAWN_I,   PIECE_SIZE, XF, Y2 );
    copyXPMArea( W_PAWN_I,   PIECE_SIZE, XG, Y2 );
    copyXPMArea( W_PAWN_I,   PIECE_SIZE, XH, Y2 );

    /* initialize the special move variables */
    checkMate = NO_MATE;
    castleMove = NO_CASTLE;
    pawnPromote = NO_PROMOTE;

    RedrawWindow();
    }


/***************************************************************************
 *
 * printState - prints the current state of the board to stdout
 *
 * The current state of the board is held in the gameState array.  This
 * function simply traces the two-dimensional array and prints the board
 * in a human readable format with black on top (upper case letters) and
 * white on the botton (lower case letters).
 *
 * RETURNS: N/A
 */
void printState
    (
    void
    )
    {
    int i, j;
    printf("\nCurrent board state:\n\n");
    for (i = 0; i < BOARD_HEIGHT; i++)
        {
        for (j = 0; j < BOARD_WIDTH; j++)
            {
            switch (gameState[i][j])
                {
                case EMPTY:     printf(" -"); break;
                case B_PAWN:    printf(" P"); break;
                case B_ROOK:    printf(" R"); break;
                case B_KNIGHT:  printf(" N"); break;
                case B_BISHOP:  printf(" B"); break;
                case B_QUEEN:   printf(" Q"); break;
                case B_KING:    printf(" K"); break;
                case W_PAWN:    printf(" p"); break;
                case W_ROOK:    printf(" r"); break;
                case W_KNIGHT:  printf(" n"); break;
                case W_BISHOP:  printf(" b"); break;
                case W_QUEEN:   printf(" q"); break;
                case W_KING:    printf(" k"); break;
                default: return; /* this should never happen */
                }
            }
        printf("\n");
        }
    printf("\n");
    }


void printCastleMove
    (
    void
    )
    {
    switch (castleMove)
        {
        case BLACK_KING_SIDE:
            printf("Black castled on king side.\n");
            break;
        case BLACK_QUEEN_SIDE:
            printf("Black castled on queen side.\n");
            break;
        case WHITE_KING_SIDE:
            printf("White castled on king side.\n");
            break;
        case WHITE_QUEEN_SIDE:
            printf("White castled on queen side.\n");
            break;
        case NO_CASTLE:
        default:
            ; /* should never get here */
        }
    }


void printPawnPromotion
    (
    void
    )
    {
    switch (pawnPromote)
        {
        case BLACK_ROOK:
            printf("Black promoted pawn to a rook.\n");
            break;
        case BLACK_KNIGHT:
            printf("Black promoted pawn to a knight.\n");
            break;
        case BLACK_BISHOP:
            printf("Black promoted pawn to a bishop.\n");
            break;
        case BLACK_QUEEN:
            printf("Black promoted pawn to a queen.\n");
            break;
        case WHITE_ROOK:
            printf("White promoted pawn to a rook.\n");
            break;
        case WHITE_KNIGHT:
            printf("White promoted pawn to a knight.\n");
            break;
        case WHITE_BISHOP:
            printf("White promoted pawn to a bishop.\n");
            break;
        case WHITE_QUEEN:
            printf("White promoted pawn to a queen.\n");
            break;
        case NO_PROMOTE:
        default:
            ; /* should never get here */
        }
    }


/***************************************************************************
 *
 * findPiece - find the position of the specified piece
 *
 * This function finds the specified piece and returns the coordinates
 * of the position for the xpm, clear xpm, and gameState array.
 *
 * RETURNS: N/A
 */
void findPiece
    (
    int piece,
    COORDS_T xCoords,
    CLEAR_COORDS_T cxCoords,
    COORDS_T stateCoords
    )
    {
    int i, j;
    for (i = 0; i < BOARD_HEIGHT; i++)
        {
        for (j = 0; j < BOARD_WIDTH; j++)
            {
            if (gameState[i][j] == piece)
                {

                stateCoords[FROM_X] = j;
                stateCoords[FROM_Y] = i;

                switch (j)
                    {
                    case SA:
                        xCoords[FROM_X] = XA; cxCoords[FROM_X] = C_XA;
                        break;
                    case SB:
                        xCoords[FROM_X] = XB; cxCoords[FROM_X] = C_XB;
                        break;
                    case SC:
                        xCoords[FROM_X] = XC; cxCoords[FROM_X] = C_XC;
                        break;
                    case SD:
                        xCoords[FROM_X] = XD; cxCoords[FROM_X] = C_XD;
                        break;
                    case SE:
                        xCoords[FROM_X] = XE; cxCoords[FROM_X] = C_XE;
                        break;
                    case SF:
                        xCoords[FROM_X] = XF; cxCoords[FROM_X] = C_XF;
                        break;
                    case SG:
                        xCoords[FROM_X] = XG; cxCoords[FROM_X] = C_XG;
                        break;
                    case SH:
                        xCoords[FROM_X] = XH; cxCoords[FROM_X] = C_XH;
                        break;
                    }

                switch (i)
                    {
                    case S1:
                        xCoords[FROM_Y] = Y1; cxCoords[FROM_Y] = C_Y1;
                        break;
                    case S2:
                        xCoords[FROM_Y] = Y2; cxCoords[FROM_Y] = C_Y2;
                        break;
                    case S3:
                        xCoords[FROM_Y] = Y3; cxCoords[FROM_Y] = C_Y3;
                        break;
                    case S4:
                        xCoords[FROM_Y] = Y4; cxCoords[FROM_Y] = C_Y4;
                        break;
                    case S5:
                        xCoords[FROM_Y] = Y5; cxCoords[FROM_Y] = C_Y5;
                        break;
                    case S6:
                        xCoords[FROM_Y] = Y6; cxCoords[FROM_Y] = C_Y6;
                        break;
                    case S7:
                        xCoords[FROM_Y] = Y7; cxCoords[FROM_Y] = C_Y7;
                        break;
                    case S8:
                        xCoords[FROM_Y] = Y8; cxCoords[FROM_Y] = C_Y8;
                        break;
                    }

                }
            }
        }
    }


/***************************************************************************
 *
 * getPieceFrom - returns the current piece located at the from position
 *
 * This function takes a COORDS_T and returns the piece that is currently
 * located in the from position.  Note that the current state is represented
 * in the gameState array.
 *
 * RETURNS: B_ROOK, B_KNIGHT, B_BISHOP, B_QUEEN, B_KING, B_PAWN,
 *          W_ROOK, W_KNIGHT, W_BISHOP, B_QUEEN, B_KING, W_PAWN, or EMPTY.
 */
int getPieceFrom
    (
    COORDS_T    stateCoords
    )
    {
    return gameState [stateCoords[FROM_Y]] [stateCoords[FROM_X]];
    }


/***************************************************************************
 *
 * getPieceTo - returns the current piece located at the to position
 *
 * This function takes a COORDS_T and returns the piece that is currently
 * located in the to position.  Note that the current state is represented
 * in the gameState array.
 *
 * RETURNS: B_ROOK, B_KNIGHT, B_BISHOP, B_QUEEN, B_KING, B_PAWN,
 *          W_ROOK, W_KNIGHT, W_BISHOP, B_QUEEN, B_KING, W_PAWN, or EMPTY.
 */
int getPieceTo
    (
    COORDS_T    stateCoords
    )
    {
    return gameState [stateCoords[TO_Y]] [stateCoords[TO_X]];
    }


/***************************************************************************
 *
 * updateMove - update the current game state with the given move
 *
 * The function takes a COORDS_T and move the piece at the from position
 * to the to position.  The from position is then filled with EMPTY.  Note
 * that the current state is represented in the gameState array.
 *
 * RETURNS: N/A
 */
void updateMove
    (
    COORDS_T    stateCoords
    )
    {
    gameState [stateCoords[TO_Y]] [stateCoords[TO_X]] =
        gameState [stateCoords[FROM_Y]] [stateCoords[FROM_X]];
    gameState [stateCoords[FROM_Y]] [stateCoords[FROM_X]] = EMPTY;
    }


/***************************************************************************
 *
 * updateCastleMove - move the necessary rook based on the castle move
 *
 * This function looks at the castleMove variable and moves the rook that
 * was involved with the specific castle move.  For instance, if a castle
 * move is made by black on the queen side, then the rook on a8 is moved
 * to d8.  The rook from position is also cleared.  Note that the current
 * state is represented in the gameState array.
 *
 * RETURNS: N/A
 */
void updateCastleMove
    (
    void
    )
    {
    switch (castleMove)
        {
        case BLACK_KING_SIDE:
            gameState [S8] [SF] = gameState [S8] [SH];
            gameState [S8] [SH] = EMPTY;
            break;
        case BLACK_QUEEN_SIDE:
            gameState [S8] [SD] = gameState [S8] [SA];
            gameState [S8] [SA] = EMPTY;
            break;
        case WHITE_KING_SIDE:
            gameState [S1] [SF] = gameState [S1] [SH];
            gameState [S1] [SH] = EMPTY;
            break;
        case WHITE_QUEEN_SIDE:
            gameState [S1] [SD] = gameState [S1] [SA];
            gameState [S1] [SA] = EMPTY;
            break;
        case NO_CASTLE:
        default:
            ; /* should never get here */
        }
    }


/***************************************************************************
 *
 * updatePawnPromote - promote the necessary pawn based on the promotion
 *
 * This function looks at the pawnPromote variable and upgrades the pawn
 * specified in the to field of the given COORDS_T to the specified piece.
 * Note that the current state is represented in the gameState array.
 *
 * RETURNS: N/A
 */
void updatePawnPromote
    (
    COORDS_T    stateCoords
    )
    {
    switch (pawnPromote)
        {
        case BLACK_ROOK:
            gameState [stateCoords[TO_Y]] [stateCoords[TO_X]] = B_ROOK;
            break;
        case BLACK_KNIGHT:
            gameState [stateCoords[TO_Y]] [stateCoords[TO_X]] = B_KNIGHT;
            break;
        case BLACK_BISHOP:
            gameState [stateCoords[TO_Y]] [stateCoords[TO_X]] = B_BISHOP;
            break;
        case BLACK_QUEEN:
            gameState [stateCoords[TO_Y]] [stateCoords[TO_X]] = B_QUEEN;
            break;
        case WHITE_ROOK:
            gameState [stateCoords[TO_Y]] [stateCoords[TO_X]] = W_ROOK;
            break;
        case WHITE_KNIGHT:
            gameState [stateCoords[TO_Y]] [stateCoords[TO_X]] = W_KNIGHT;
            break;
        case WHITE_BISHOP:
            gameState [stateCoords[TO_Y]] [stateCoords[TO_X]] = W_BISHOP;
            break;
        case WHITE_QUEEN:
            gameState [stateCoords[TO_Y]] [stateCoords[TO_X]] = W_QUEEN;
            break;
        case NO_PROMOTE:
        default:
            ; /* should never get here */
        }
    }


/***************************************************************************
 *
 * castleMoveRook - update the xpm with the new rook position in a castle move
 *
 * This function looks at the castleMove variable and updates the xpm by
 * moving the rook that was involved with the specific castle move.  For
 * instance, if a castle move is made by black on the queen side, then the
 * rook on a8 is moved to d8.  The rook from position is also cleared.
 *
 * RETURNS: N/A
 */
void castleMoveRook
    (
    void
    )
    {
    switch (castleMove)
        {
        case BLACK_KING_SIDE:
            copyXPMArea(XH, Y8, PIECE_SIZE, XF, Y8);
            copyXPMArea(C_XH, C_Y8, PIECE_SIZE, XH, Y8);
            break;
        case BLACK_QUEEN_SIDE:
            copyXPMArea(XA, Y8, PIECE_SIZE, XD, Y8);
            copyXPMArea(C_XA, C_Y8, PIECE_SIZE, XA, Y8);
            break;
        case WHITE_KING_SIDE:
            copyXPMArea(XH, Y1, PIECE_SIZE, XF, Y1);
            copyXPMArea(C_XH, C_Y1, PIECE_SIZE, XH, Y1);
            break;
        case WHITE_QUEEN_SIDE:
            copyXPMArea(XA, Y1, PIECE_SIZE, XD, Y1);
            copyXPMArea(C_XA, C_Y1, PIECE_SIZE, XA, Y1);
            break;
        case NO_CASTLE:
        default:
            ; /* should never get here */
        }
    }


/***************************************************************************
 *
 * castleMoveRookUndo - update the xpm with the old rook position in a
 *                      castle move
 *
 * This function looks at the castleMove variable and updates the xpm by
 * moving back the rook that was involved with the specific castle move.  For
 * instance, if a castle move is made by black on the queen side, then the
 * rook on d8 is moved back to a8.  The rook from positin is also cleared.
 *
 * RETURNS: N/A
 */
void castleMoveRookUndo
    (
    void
    )
    {
    switch (castleMove)
        {
        case BLACK_KING_SIDE:
            copyXPMArea(XF, Y8, PIECE_SIZE, XH, Y8);
            copyXPMArea(C_XF, C_Y8, PIECE_SIZE, XF, Y8);
            break;
        case BLACK_QUEEN_SIDE:
            copyXPMArea(XD, Y8, PIECE_SIZE, XA, Y8);
            copyXPMArea(C_XD, C_Y8, PIECE_SIZE, XD, Y8);
            break;
        case WHITE_KING_SIDE:
            copyXPMArea(XF, Y1, PIECE_SIZE, XH, Y1);
            copyXPMArea(C_XF, C_Y1, PIECE_SIZE, XF, Y1);
            break;
        case WHITE_QUEEN_SIDE:
            copyXPMArea(XD, Y1, PIECE_SIZE, XA, Y1);
            copyXPMArea(C_XD, C_Y1, PIECE_SIZE, XD, Y1);
            break;
        case NO_CASTLE:
        default:
            ; /* should never get here */
        }
    }


/***************************************************************************
 *
 * pawnPromoteTo - updates the xpm with piece a pawn was promoted to
 *
 * The function updates the xpm by replacing the promoted pawn with the
 * piece found in the pawnPromote variable.  The pawn position is found
 * in the to position within the given COORD_T.
 *
 * RETURNS: N/A
 */
void pawnPromoteTo
    (
    COORDS_T    xCoords
    )
    {
    switch (pawnPromote)
        {
        case BLACK_ROOK:
            copyXPMArea(B_ROOK_I, PIECE_SIZE, xCoords[TO_X], xCoords[TO_Y]);
            break;
        case BLACK_KNIGHT:
            copyXPMArea(B_KNIGHT_I, PIECE_SIZE, xCoords[TO_X], xCoords[TO_Y]);
            break;
        case BLACK_BISHOP:
            copyXPMArea(B_BISHOP_I, PIECE_SIZE, xCoords[TO_X], xCoords[TO_Y]);
            break;
        case BLACK_QUEEN:
            copyXPMArea(B_QUEEN_I, PIECE_SIZE, xCoords[TO_X], xCoords[TO_Y]);
            break;
        case WHITE_ROOK:
            copyXPMArea(W_ROOK_I, PIECE_SIZE, xCoords[TO_X], xCoords[TO_Y]);
            break;
        case WHITE_KNIGHT:
            copyXPMArea(W_KNIGHT_I, PIECE_SIZE, xCoords[TO_X], xCoords[TO_Y]);
            break;
        case WHITE_BISHOP:
            copyXPMArea(W_BISHOP_I, PIECE_SIZE, xCoords[TO_X], xCoords[TO_Y]);
            break;
        case WHITE_QUEEN:
            copyXPMArea(W_QUEEN_I, PIECE_SIZE, xCoords[TO_X], xCoords[TO_Y]);
            break;
        case NO_PROMOTE:
        default:
            ; /* should never get here */
        }
    }


/***************************************************************************
 *
 * whichPosition - converts and X mouse click into a board coordinate
 *
 * This function converts an X mouse click (represented in pixels) and
 * converts is to the cartesian represention used within the gameState
 * array (i.e. [a-b][1-8]).
 *
 * RETURNS: places the converted coordinate in the given POSITION_T
 */
void whichPosition
    (
    int         x,
    int         y,
    POSITION_T  pos
    )
    {
    /*
     * Why start at 3 and end at 60?
     * Because the user might click the shaded pixel, and that's tolerable.
     */
    if      ((x >= 3)  && (x <= 10)) pos[POS_X] = SA;
    else if ((x >= 11) && (x <= 17)) pos[POS_X] = SB;
    else if ((x >= 18) && (x <= 24)) pos[POS_X] = SC;
    else if ((x >= 25) && (x <= 31)) pos[POS_X] = SD;
    else if ((x >= 32) && (x <= 38)) pos[POS_X] = SE;
    else if ((x >= 39) && (x <= 45)) pos[POS_X] = SF;
    else if ((x >= 46) && (x <= 52)) pos[POS_X] = SG;
    else if ((x >= 53) && (x <= 60)) pos[POS_X] = SH;
    else    pos[POS_X] = -1;
    if      ((y >= 3)  && (y <= 10)) pos[POS_Y] = S8;
    else if ((y >= 11) && (y <= 17)) pos[POS_Y] = S7;
    else if ((y >= 18) && (y <= 24)) pos[POS_Y] = S6;
    else if ((y >= 25) && (y <= 31)) pos[POS_Y] = S5;
    else if ((y >= 32) && (y <= 38)) pos[POS_Y] = S4;
    else if ((y >= 39) && (y <= 45)) pos[POS_Y] = S3;
    else if ((y >= 46) && (y <= 52)) pos[POS_Y] = S2;
    else if ((y >= 53) && (y <= 60)) pos[POS_Y] = S1;
    else    pos[POS_Y] = -1;
    }


/***************************************************************************
 *
 * convertMoveFromText - converts a textual move to something useful
 *
 * Using gnuchessx a move can be one of the following:
 *
 *    - g1f3    means move from g1 to f3
 *    - a7a8q   means move pawn from a7 to a8 and promote to queen
 *              promotion char can be r, n, b, or q
 *    - o-o or 0-0      castle king side
 *    - o-o-o or 0-0-0  castle queen side
 *
 * Note: All moves are terminated by '\n' (e.g. "g1f3\n")
 *
 * This function takes a textual move and figures out the following:
 *   . the coordinates for the to and from positions in the xpm
 *   . the coordinates for the clear space in the xpm
 *   . the coordinates (cartesian) for updating the gameState array
 *
 * If the move is a castle move then the castleMove variable is set
 * to the proper instance.
 *
 * If a pawn is moved and a promotion character is found, then the
 * pawnPromote variable is set to the correct promotion piece.
 *
 * RETURNS: 1 for valid move, 0 if error.
 */
int convertMoveFromText
    (
    int                 player,         /* either BLACK or WHITE */
    char *              move,           /* textual move to convert */
    COORDS_T            xCoords,        /* storage for xpm coords */
    CLEAR_COORDS_T      cxCoords,       /* storage for xpm 'clear' coords */
    COORDS_T            stateCoords     /* storage for the cartesian coords */
    )
    {
    int length, i;

    /* initialize the storage arrays */
    bzero(xCoords, sizeof(COORDS_T));
    bzero(cxCoords, sizeof(CLEAR_COORDS_T));
    bzero(stateCoords, sizeof(COORDS_T));

    /* convert the castle notation to algebraic */
    if ((strncmp(move, "o-o-o", 5) == 0) ||
             (strncmp(move, "0-0-0", 5) == 0))
        {
        if (player == BLACK) strcpy(move, "e8c8\n");
        else strcpy(move, "e1c1\n");
        }
    else if ((strncmp(move, "o-o", 3) == 0) ||
        (strncmp(move, "0-0", 3) == 0))
        {
        if (player == BLACK) strcpy(move, "e8g8\n");
        else strcpy(move, "e1g1\n");
        }

    /* convert the move string to lower case */
    length = strlen(move);
    for (i = 0; i < length; i++) move[i] = (char)tolower((int)move[i]);

    switch (move[FROM_X]) /* get the from column [a-h] */
        {
        case 'a': xCoords[FROM_X] = XA; cxCoords[FROM_X] = C_XA;
                  stateCoords[FROM_X] = SA; break;

        case 'b': xCoords[FROM_X] = XB; cxCoords[FROM_X] = C_XB;
                  stateCoords[FROM_X] = SB; break;

        case 'c': xCoords[FROM_X] = XC; cxCoords[FROM_X] = C_XC;
                  stateCoords[FROM_X] = SC; break;

        case 'd': xCoords[FROM_X] = XD; cxCoords[FROM_X] = C_XD;
                  stateCoords[FROM_X] = SD; break;

        case 'e': xCoords[FROM_X] = XE; cxCoords[FROM_X] = C_XE;
                  stateCoords[FROM_X] = SE; break;

        case 'f': xCoords[FROM_X] = XF; cxCoords[FROM_X] = C_XF;
                  stateCoords[FROM_X] = SF; break;

        case 'g': xCoords[FROM_X] = XG; cxCoords[FROM_X] = C_XG;
                  stateCoords[FROM_X] = SG; break;

        case 'h': xCoords[FROM_X] = XH; cxCoords[FROM_X] = C_XH;
                  stateCoords[FROM_X] = SH; break;

        default: return 0;
        }

    switch (move[FROM_Y]) /* get the from row [1-8] */
        {
        case '1': xCoords[FROM_Y] = Y1; cxCoords[FROM_Y] = C_Y1;
                  stateCoords[FROM_Y] = S1; break;

        case '2': xCoords[FROM_Y] = Y2; cxCoords[FROM_Y] = C_Y2;
                  stateCoords[FROM_Y] = S2; break;

        case '3': xCoords[FROM_Y] = Y3; cxCoords[FROM_Y] = C_Y3;
                  stateCoords[FROM_Y] = S3; break;

        case '4': xCoords[FROM_Y] = Y4; cxCoords[FROM_Y] = C_Y4;
                  stateCoords[FROM_Y] = S4; break;

        case '5': xCoords[FROM_Y] = Y5; cxCoords[FROM_Y] = C_Y5;
                  stateCoords[FROM_Y] = S5; break;

        case '6': xCoords[FROM_Y] = Y6; cxCoords[FROM_Y] = C_Y6;
                  stateCoords[FROM_Y] = S6; break;

        case '7': xCoords[FROM_Y] = Y7; cxCoords[FROM_Y] = C_Y7;
                  stateCoords[FROM_Y] = S7; break;

        case '8': xCoords[FROM_Y] = Y8; cxCoords[FROM_Y] = C_Y8;
                  stateCoords[FROM_Y] = S8; break;

        default: return 0;
        }

    switch (move[TO_X]) /* get the to column [a-h] */
        {
        case 'a': xCoords[TO_X] = XA; stateCoords[TO_X] = SA; break;
        case 'b': xCoords[TO_X] = XB; stateCoords[TO_X] = SB; break;
        case 'c': xCoords[TO_X] = XC; stateCoords[TO_X] = SC; break;
        case 'd': xCoords[TO_X] = XD; stateCoords[TO_X] = SD; break;
        case 'e': xCoords[TO_X] = XE; stateCoords[TO_X] = SE; break;
        case 'f': xCoords[TO_X] = XF; stateCoords[TO_X] = SF; break;
        case 'g': xCoords[TO_X] = XG; stateCoords[TO_X] = SG; break;
        case 'h': xCoords[TO_X] = XH; stateCoords[TO_X] = SH; break;
        default: return 0;
        }

    switch (move[TO_Y]) /* get the to row [1-8] */
        {
        case '1': xCoords[TO_Y] = Y1; stateCoords[TO_Y] = S1; break;
        case '2': xCoords[TO_Y] = Y2; stateCoords[TO_Y] = S2; break;
        case '3': xCoords[TO_Y] = Y3; stateCoords[TO_Y] = S3; break;
        case '4': xCoords[TO_Y] = Y4; stateCoords[TO_Y] = S4; break;
        case '5': xCoords[TO_Y] = Y5; stateCoords[TO_Y] = S5; break;
        case '6': xCoords[TO_Y] = Y6; stateCoords[TO_Y] = S6; break;
        case '7': xCoords[TO_Y] = Y7; stateCoords[TO_Y] = S7; break;
        case '8': xCoords[TO_Y] = Y8; stateCoords[TO_Y] = S8; break;
        default: return 0;
        }

    /* Check for a pawn promotion */
    switch (move[4])
        {
        case 'r':
            {
            if ((stateCoords[TO_Y] == S8) &&
                (getPieceFrom(stateCoords) == W_PAWN))
                pawnPromote = WHITE_ROOK;
            else if ((stateCoords[TO_Y] == S1) &&
                     (getPieceFrom(stateCoords) == B_PAWN))
                pawnPromote = BLACK_ROOK;
            /* else error - should never happen as the board is correct */
            }
            break;
        case 'n':
            {
            if ((stateCoords[TO_Y] == S8) &&
                (getPieceFrom(stateCoords) == W_PAWN))
                pawnPromote = WHITE_KNIGHT;
            else if ((stateCoords[TO_Y] == S1) &&
                     (getPieceFrom(stateCoords) == B_PAWN))
                pawnPromote = BLACK_KNIGHT;
            /* else error - should never happen as the board is correct */
            }
            break;
        case 'b':
            {
            if ((stateCoords[TO_Y] == S8) &&
                (getPieceFrom(stateCoords) == W_PAWN))
                pawnPromote = WHITE_BISHOP;
            else if ((stateCoords[TO_Y] == S1) &&
                     (getPieceFrom(stateCoords) == B_PAWN))
                pawnPromote = BLACK_BISHOP;
            /* else error - should never happen as the board is correct */
            }
            break;
        case 'q':
            {
            if ((stateCoords[TO_Y] == S8) &&
                (getPieceFrom(stateCoords) == W_PAWN))
                pawnPromote = WHITE_QUEEN;
            else if ((stateCoords[TO_Y] == S1) &&
                     (getPieceFrom(stateCoords) == B_PAWN))
                pawnPromote = BLACK_QUEEN;
            /* else error - should never happen as the board is correct */
            }
            break;
        default:
            pawnPromote = NO_PROMOTE;
            if (move[4] != '\n')
                {
                printf("Unknown characters in the move\n");
                return 0;
                }
        }

    /* Check for a castle move */
    if ((strncmp(move, "e8g8", 4) == 0) &&
        (getPieceFrom(stateCoords) == B_KING))
        castleMove = BLACK_KING_SIDE;
    else if ((strncmp(move, "e8c8", 4) == 0) &&
             (getPieceFrom(stateCoords) == B_KING))
        castleMove = BLACK_QUEEN_SIDE;
    else if ((strncmp(move, "e1g1", 4) == 0) &&
             (getPieceFrom(stateCoords) == W_KING))
        castleMove = WHITE_KING_SIDE;
    else if ((strncmp(move, "e1c1", 4) == 0) &&
             (getPieceFrom(stateCoords) == W_KING))
        castleMove = WHITE_QUEEN_SIDE;
    else castleMove = NO_CASTLE;

    return 1;
    }


/***************************************************************************
 *
 * convertMoveFromMouse - converts a cartesian coordinates move to something
 *                        useful
 *
 * See convertMoveFromText for textual move representations.
 *
 * This function takes a cartesian coordinates move and figures out the
 * following:
 *   . the coordinates for the to and from positions in the xpm
 *   . the coordinates for the clear space in the xpm
 *   . the coordinates (cartesian) for updating the gameState array
 *   . the textual move representaion
 *
 * If the move is a castle move then the castleMove variable is set
 * to the proper instance.
 *
 * If a pawn is moved and a promotion character is found, then the
 * pawnPromote variable is set to the correct promotion piece.  Note
 * that all promotions are to a queen.  This could be fixed by popping
 * up some sort of dialog box to ask the player what piece they want.
 *
 * RETURNS: 1 for valid move or 0 if error.
 */
int convertMoveFromMouse
    (
    POSITION_T          posFrom,        /* X mouse position from */
    POSITION_T          posTo,          /* X mouse position to */
    char *              move,           /* storage for the textual move */
    COORDS_T            xCoords,        /* storage for xpm coords */
    CLEAR_COORDS_T      cxCoords,       /* storage for xpm 'clear' coords */
    COORDS_T            stateCoords     /* storage for the cartesian coords */
    )
    {
    /* initialize the storage arrays */
    bzero(xCoords, sizeof(COORDS_T));
    bzero(cxCoords, sizeof(CLEAR_COORDS_T));
    bzero(stateCoords, sizeof(COORDS_T));

    /*
     * the given move is already in cartesian form so no translation
     * needed for the gameState array indexes
     */
    stateCoords[FROM_X] = posFrom[POS_X];
    stateCoords[FROM_Y] = posFrom[POS_Y];
    stateCoords[TO_X] = posTo[POS_X];
    stateCoords[TO_Y] = posTo[POS_Y];

    switch (posFrom[POS_X]) /* get the from column [a-h] */
        {
        case SA: xCoords[FROM_X] = XA; cxCoords[FROM_X] = C_XA;
                  move[FROM_X] = 'a'; break;

        case SB: xCoords[FROM_X] = XB; cxCoords[FROM_X] = C_XB;
                  move[FROM_X] = 'b'; break;

        case SC: xCoords[FROM_X] = XC; cxCoords[FROM_X] = C_XC;
                  move[FROM_X] = 'c'; break;

        case SD: xCoords[FROM_X] = XD; cxCoords[FROM_X] = C_XD;
                  move[FROM_X] = 'd'; break;

        case SE: xCoords[FROM_X] = XE; cxCoords[FROM_X] = C_XE;
                  move[FROM_X] = 'e'; break;

        case SF: xCoords[FROM_X] = XF; cxCoords[FROM_X] = C_XF;
                  move[FROM_X] = 'f'; break;

        case SG: xCoords[FROM_X] = XG; cxCoords[FROM_X] = C_XG;
                  move[FROM_X] = 'g'; break;

        case SH: xCoords[FROM_X] = XH; cxCoords[FROM_X] = C_XH;
                  move[FROM_X] = 'h'; break;

        default: return 0;
        }

    switch (posFrom[POS_Y]) /* get the from row [1-8] */
        {
        case S1: xCoords[FROM_Y] = Y1; cxCoords[FROM_Y] = C_Y1;
                  move[FROM_Y] = '1'; break;

        case S2: xCoords[FROM_Y] = Y2; cxCoords[FROM_Y] = C_Y2;
                  move[FROM_Y] = '2'; break;

        case S3: xCoords[FROM_Y] = Y3; cxCoords[FROM_Y] = C_Y3;
                  move[FROM_Y] = '3'; break;

        case S4: xCoords[FROM_Y] = Y4; cxCoords[FROM_Y] = C_Y4;
                  move[FROM_Y] = '4'; break;

        case S5: xCoords[FROM_Y] = Y5; cxCoords[FROM_Y] = C_Y5;
                  move[FROM_Y] = '5'; break;

        case S6: xCoords[FROM_Y] = Y6; cxCoords[FROM_Y] = C_Y6;
                  move[FROM_Y] = '6'; break;

        case S7: xCoords[FROM_Y] = Y7; cxCoords[FROM_Y] = C_Y7;
                  move[FROM_Y] = '7'; break;

        case S8: xCoords[FROM_Y] = Y8; cxCoords[FROM_Y] = C_Y8;
                  move[FROM_Y] = '8'; break;

        default: return 0;
        }

    switch (posTo[POS_X]) /* get the to column [a-h] */
        {
        case SA: xCoords[TO_X] = XA; move[TO_X] = 'a'; break;
        case SB: xCoords[TO_X] = XB; move[TO_X] = 'b'; break;
        case SC: xCoords[TO_X] = XC; move[TO_X] = 'c'; break;
        case SD: xCoords[TO_X] = XD; move[TO_X] = 'd'; break;
        case SE: xCoords[TO_X] = XE; move[TO_X] = 'e'; break;
        case SF: xCoords[TO_X] = XF; move[TO_X] = 'f'; break;
        case SG: xCoords[TO_X] = XG; move[TO_X] = 'g'; break;
        case SH: xCoords[TO_X] = XH; move[TO_X] = 'h'; break;
        default: return 0;
        }

    switch (posTo[POS_Y]) /* get the to row [1-8] */
        {
        case S1: xCoords[TO_Y] = Y1; move[TO_Y] = '1'; break;
        case S2: xCoords[TO_Y] = Y2; move[TO_Y] = '2'; break;
        case S3: xCoords[TO_Y] = Y3; move[TO_Y] = '3'; break;
        case S4: xCoords[TO_Y] = Y4; move[TO_Y] = '4'; break;
        case S5: xCoords[TO_Y] = Y5; move[TO_Y] = '5'; break;
        case S6: xCoords[TO_Y] = Y6; move[TO_Y] = '6'; break;
        case S7: xCoords[TO_Y] = Y7; move[TO_Y] = '7'; break;
        case S8: xCoords[TO_Y] = Y8; move[TO_Y] = '8'; break;
        default: return 0;
        }

    move[4] = '\n';
    move[5] = '\0';

    /* Check for a pawn promotion - as of right know always a queen */
    if ((stateCoords[TO_Y] == S8) &&
        (getPieceFrom(stateCoords) == W_PAWN))
        pawnPromote = WHITE_QUEEN;
    else pawnPromote = NO_PROMOTE;

    /* Check for a castle move */
    if ((strncmp(move, "e8g8", 4) == 0) &&
        (getPieceFrom(stateCoords) == B_KING))
        castleMove = BLACK_KING_SIDE;
    else if ((strncmp(move, "e8c8", 4) == 0) &&
             (getPieceFrom(stateCoords) == B_KING))
        castleMove = BLACK_QUEEN_SIDE;
    else if ((strncmp(move, "e1g1", 4) == 0) &&
             (getPieceFrom(stateCoords) == W_KING))
        castleMove = WHITE_KING_SIDE;
    else if ((strncmp(move, "e1c1", 4) == 0) &&
             (getPieceFrom(stateCoords) == W_KING))
        castleMove = WHITE_QUEEN_SIDE;
    else castleMove = NO_CASTLE;

    return 1;
    }


/***************************************************************************
 *
 * forceHard - force the engine to think during the player's time
 *
 * RETURNS: N/A
 */
void forceHard
    (
    void
    )
    {
    fprintf(output, "easy\n");  /* force hard mode (toggling easy) */
    fflush(output);
    }


/***************************************************************************
 *
 * forceRandom - force the engine to make somewhat random moves
 *
 * RETURNS: N/A
 */
void forceRandom
    (
    void
    )
    {
    fprintf(output, "random\n");   /* force random moves */
    fflush(output);
    }


/***************************************************************************
 *
 * newGame - start a new game
 *
 * RETURNS: N/A
 */
void newGame
    (
    void
    )
    {
    initBoard();                /* re-init the gameState */
    fprintf(output, "new\n");   /* tell the engine to start over */
    fflush(output);
    if (engineRandom) forceRandom();
    if (engineHard) forceHard();
    }


/***************************************************************************
 *
 * setDepth - set the thinking depth for the engine
 *
 * RETURNS: N/A
 */
void setDepth
    (
    int depth   /* the depth to set to */
    )
    {
    /* ask the engine to set the depth */
    fprintf(output, "depth\n"); fflush(output);

    /* send the new depth */
    fprintf(output, "%d\n", depth); fflush(output);
    }


/***************************************************************************
 *
 * getHint - get a hint from the engine
 *
 * The hint is placed in the given storage denoted by the move variable.
 *
 * RETURNS: N/A
 */
void getHint
    (
    char * move         /* storage for the move */
    )
    {
    char junk [50];
    int length;

    /* ask the engine for a hint */
    fprintf(output, "hint\n"); fflush(output);

    /* get the hint */
    fscanf(input, "Hint: %s", move);

    /* grab the remaining input */
    fgets(junk, 50, input);

    /* terminate the engine's hint with a newline */
    length = strlen(move);
    move[length] = '\n';
    move[(length + 1)] = '\0';
    }


#ifdef DEBUG
/* print out the help from gnuchess for checking clock/settings */
void gnuchessHelp
    (
    void
    )
    {
    int i;
    char junk [80];
    fprintf(output, "help\n"); fflush(output);
    for (i = 0; i < 22; i++)
        {
        fgets(junk, 80, input);
        printf("%s", junk);
        }
    }
#endif


/***************************************************************************
 *
 * sendMove - send a move to the gnuchess engine and get it's move
 *
 * This function takes a move, in textual format, and sends it to the
 * gnuchess engine.  The chess engine then makes the move or returns
 * an error indicating and illegal move.  If the move is not illegal, the
 * engine returns it's move.  This move is stored in compMove.
 *
 * RETURNS: 0 for an illegal move, 1 if success.
 */
int sendMove
    (
    char *      move,           /* the textual move to send */
    char *      compMove        /* storage for the engine's move */
    )
    {
    char buf [50];
    int length;
    int moveNum;


    /* send the move to the engine */
    fprintf(output, "%s", move); fflush(output);

    /* get the next line which shows either the move made or error */
    fgets(buf, 50, input);

    /* if the move was illegal then return */
    if (strstr(buf, "Illegal") != NULL)
        return 0;

    /* the next line shows the engine's move */
    fgets(buf, 50, input);
    sscanf(buf, "%d. ... %s", &moveNum, compMove);

    /* the next line once again shows the engine's move */
    fgets(buf, 50, input);

    /* terminate the engine's move with a newline */
    length = strlen(compMove);
    compMove[length] = '\n';
    compMove[(length + 1)] = '\0';

    /* if a checkmate occured then set the checkMate variable */
    if (strstr(buf, "Black mates") != NULL) checkMate = BLACK_MATES;
    else if (strstr(buf, "White mates") != NULL) checkMate = WHITE_MATES;
    else checkMate = NO_MATE;

    return 1;
    }


/***************************************************************************
 *
 * doMove - sends a move to the chess engine and updates the xpm
 *
 * RETURNS: N/A
 */
void doMove
    (
    int                 verbose,
    char *              move,           /* the move to send */
    COORDS_T            xCoords,        /* xpm coords */
    CLEAR_COORDS_T      cxCoords,       /* xpm 'clear' coords */
    COORDS_T            stateCoords     /* cartesian coords */
    )
    {
    char compMove [10];

    /*
     * Save the previous squares because if sendMove() called before
     * drawing the payler's move, the chess engine can take a long time
     * and we won't see our move until the engine has decided.
     */
    copyXPMArea(xCoords[FROM_X], xCoords[FROM_Y], PIECE_SIZE,
                FROM_SAVE);
    copyXPMArea(xCoords[TO_X], xCoords[TO_Y], PIECE_SIZE,
                TO_SAVE);

    /* draw the move */
    copyXPMArea(xCoords[FROM_X], xCoords[FROM_Y], PIECE_SIZE,
                xCoords[TO_X], xCoords[TO_Y]);
    copyXPMArea(cxCoords[FROM_X], cxCoords[FROM_Y], PIECE_SIZE,
                xCoords[FROM_X], xCoords[FROM_Y]);

    /* do the castle move */
    if (castleMove != NO_CASTLE)
        {
        if (verbose) printCastleMove();
        castleMoveRook();
        }

    /* do the pawn promotion */
    if (pawnPromote != NO_PROMOTE)
        {
        if (verbose) printPawnPromotion();
        pawnPromoteTo(xCoords);
        }

    RedrawWindow();

    /* send the move to the chess engine and get it's move */
    if (!sendMove(move, compMove))
        {
        if (verbose) printf("Illegal move: %s", move);
        printf("\a"); fflush(stdout);
        /* restore the move */
        copyXPMArea(FROM_SAVE, PIECE_SIZE,
                    xCoords[FROM_X], xCoords[FROM_Y]);
        copyXPMArea(TO_SAVE, PIECE_SIZE,
                    xCoords[TO_X], xCoords[TO_Y]);
        if (castleMove != NO_CASTLE) castleMoveRookUndo();
        RedrawWindow();
        return;
        }

    /* move ok so update the state for the user's move */
    updateMove(stateCoords);
    if (castleMove != NO_CASTLE) updateCastleMove();
    if (pawnPromote != NO_PROMOTE) updatePawnPromote(stateCoords);

    if (verbose) printf("C> %s", compMove);

    /* no need to check return code for the engine move */
    convertMoveFromText(BLACK, compMove, xCoords, cxCoords, stateCoords);

    /* draw the engine's move */
    copyXPMArea(xCoords[FROM_X], xCoords[FROM_Y], PIECE_SIZE,
                xCoords[TO_X], xCoords[TO_Y]);
    copyXPMArea(cxCoords[FROM_X], cxCoords[FROM_Y], PIECE_SIZE,
                xCoords[FROM_X], xCoords[FROM_Y]);

    /* update the state for the engine's move */
    updateMove(stateCoords);
    if (castleMove != NO_CASTLE)
        {
        if (verbose) printCastleMove();
        castleMoveRook();
        updateCastleMove();
        }
    if (pawnPromote != NO_PROMOTE)
        {
        if (verbose) printPawnPromotion();
        pawnPromoteTo(xCoords);
        updatePawnPromote(stateCoords);
        }

    RedrawWindow();

    /* check if the game is over */
    if (checkMate == BLACK_MATES)
        {
        printf("\a\nBlack mates!\n"); fflush(stdout);
        printState();
        findPiece(W_KING, xCoords, cxCoords, stateCoords);
        copyXPMArea(CHECKMATE_KING_I, PIECE_SIZE,
                    xCoords[FROM_X], xCoords[FROM_Y]);
        RedrawWindow();
        }
    else if (checkMate == WHITE_MATES)
        {
        printf("\a\nWhite mates!\n"); fflush(stdout);
        printState();
        findPiece(B_KING, xCoords, cxCoords, stateCoords);
        copyXPMArea(CHECKMATE_KING_I, PIECE_SIZE,
                    xCoords[FROM_X], xCoords[FROM_Y]);
        RedrawWindow();
        }
    }


/***************************************************************************
 *
 * mouseGame - play a game using the mouse
 *
 * RETURNS: N/A
 */
void mouseGame
    (
    void
    )
    {
    XEvent xEvent;
    int Xfd = -1;
    fd_set fdset;
    char move [10];
    COORDS_T stateCoords;
    COORDS_T xCoords;
    CLEAR_COORDS_T cxCoords;
    int getFromState = 1;

    POSITION_T posFrom, posTo, posTemp;
    bzero((char *)&posFrom, sizeof(POSITION_T));
    bzero((char *)&posTo, sizeof(POSITION_T));
    bzero((char *)&posTemp, sizeof(POSITION_T));

    Xfd = XConnectionNumber(display);

    while (1)
        {
        /* Redraw Window */
        RedrawWindow();

        /* select on the X descriptor */
        FD_ZERO(&fdset);
        FD_SET(Xfd, &fdset);
        if (select((Xfd + 1), &fdset, NULL, NULL, NULL) == -1)
            {
            perror("select");
            kill(childPid, SIGTERM);
            XCloseDisplay(display);
            exit(1);
            }
        if (!FD_ISSET(Xfd, &fdset))
            continue;

        /* get all the pending X events */
        while (XPending(display))
            {
            XNextEvent(display, &xEvent);
            switch (xEvent.type)
                {

                /*
                 * There are two stats used by the mouse game.  One is
                 * getting the from position and the other is getting the
                 * to position.  For a position to be valid the button
                 * must by released on the same square it is pressed on.
                 */
                case ButtonPress:
                    {
                    if ((xEvent.xbutton.button == Button1) &&
                        (checkMate == NO_MATE))
                        {
                        if (getFromState)
                            whichPosition(xEvent.xbutton.x,
                                          xEvent.xbutton.y,
                                          posFrom);
                        else /* getToState */
                            whichPosition(xEvent.xbutton.x,
                                          xEvent.xbutton.y,
                                          posTo);
                        }
                    break;
                    } /* case ButtonPress */

                case ButtonRelease:
                    {
                    if ((xEvent.xbutton.button == Button3) &&
                        (xEvent.xbutton.state & ControlMask))
                        {
                        kill(childPid, SIGTERM);
                        XCloseDisplay(display);
                        exit(0);
                        }
                    else if (xEvent.xbutton.button == Button3)
                        {
                        newGame();
                        }
                    else if ((xEvent.xbutton.button == Button1) &&
                             (xEvent.xbutton.state & ShiftMask))
                        {
                        printState();
                        }
                    else if (checkMate != NO_MATE)
                        {
                        /* nothing to do until new game or exit */
                        printf("\a"); fflush(stdout);
                        }
                    else if (xEvent.xbutton.button == Button1)
                        {
                        if (getFromState)
                            {
                            whichPosition(xEvent.xbutton.x,
                                          xEvent.xbutton.y,
                                          posTemp);
                            /*
                             * if the button press/release is the same
                             * then go to the next state
                             */
                            if ((posFrom[POS_X] == posTemp[POS_X]) &&
                                (posFrom[POS_Y] == posTemp[POS_Y]))
                                getFromState = 0; /* now to getToState */
                            }
                        else /* getToState */
                            {
                            whichPosition(xEvent.xbutton.x,
                                          xEvent.xbutton.y,
                                          posTemp);
                            /*
                             * if the button press/release is the same
                             * then do the move
                             */
                            if ((posTo[POS_X] == posTemp[POS_X]) &&
                                (posTo[POS_Y] == posTemp[POS_Y]))
                                {
                                if (convertMoveFromMouse(posFrom, posTo,
                                                         move, xCoords,
                                                         cxCoords,
                                                         stateCoords))
                                    {
                                    /* printf(" > %s", move); */
                                    doMove(0, move, xCoords, cxCoords,
                                           stateCoords);
                                    }
                                else printf("\a"); fflush(stdout);
                                }
                            /* go to initial state to get another move */
                            getFromState = 1;
                            }
                        }
                    else if (xEvent.xbutton.button == Button2)
                        {
                        getHint(move);
                        /* convert the textual move to something useful */
                        if (convertMoveFromText(WHITE, move, xCoords,
                                                cxCoords, stateCoords))
                            doMove(0, move, xCoords, cxCoords, stateCoords);
                        else printf("\a"); fflush(stdout);
                        /* go to initial state to get another move */
                        getFromState = 1;
                        }
                    break;
                    } /* case ButtonRelease */

                case Expose:
                    {
                    RedrawWindow();
                    break;
                    } /* case Expose */

                case DestroyNotify:
                    {
                    kill(childPid, SIGTERM);
                    XCloseDisplay(display);
                    exit(0);
                    } /* case DestroyNotify */
                } /* switch */
            } /* while XPending */
        } /* while 1 */
    }


/***************************************************************************
 *
 * textGame - play a game using the keyboard
 *
 * RETURNS: N/A
 */
void textGame
    (
    void
    )
    {
    XEvent xEvent;
    char move [10];
    COORDS_T stateCoords;
    COORDS_T xCoords;
    CLEAR_COORDS_T cxCoords;

    while (1)
        {

        /* handle the pending XEvents */
        while (XPending(display))
            {
            XNextEvent(display, &xEvent);
            switch (xEvent.type)
                {
                case Expose:
                    RedrawWindow();
                    break;

                case DestroyNotify:
                    kill(childPid, SIGTERM);
                    XCloseDisplay(display);
                    exit(0);
                }
            }

        /* get the move from stdin */
        bzero(move, 10);
        printf(" > ");
        fgets(move, 10, stdin);

#ifdef DEBUG
        if (strncasecmp(move, "gch", 3) == 0)
            {
            gnuchessHelp();
            continue;
            }
#endif

        /* if the move is a help command then print out help message */
        if ((strncasecmp(move, HLP, HLP_LEN) == 0) ||
            (strncasecmp(move, HELP, HELP_LEN) == 0))
            {
            printHelp();
            continue;
            }

        /* if the move is a quit command the exit */
        if ((strncasecmp(move, Q, Q_LEN) == 0) ||
            (strncasecmp(move, QUIT, QUIT_LEN) == 0))
            {
            printf("Goodbye!\n");
            kill(childPid, SIGTERM);
            XCloseDisplay(display);
            exit(1);
            }

        /* if the move is a refresh command then refresh the window */
        if ((strncasecmp(move, R, R_LEN) == 0) ||
            (strncasecmp(move, REFRESH, REFRESH_LEN) == 0))
            {
            RedrawWindow();
            continue;
            }

        /* if the move is a print command the print the board */
        if ((strncasecmp(move, P, P_LEN) == 0) ||
            (strncasecmp(move, PRINT, PRINT_LEN) == 0) ||
            (strncasecmp(move, BD, BD_LEN) == 0))
            {
            printState();
            continue;
            }

        /* if the move is a new command the start over */
        if ((strncasecmp(move, N, N_LEN) == 0) ||
            (strncasecmp(move, NEW, NEW_LEN) == 0))
            {
            printf("Starting a new game...\n");
            newGame();
            continue;
            }

        /* if checkmate then can't do anything until quit or new game */
        if (checkMate != NO_MATE)
            {
            printf("Game is over.\n");
            continue;
            }

        /* if the move is a hint command then get a hint and make the move */
        if ((strncasecmp(move, H, H_LEN) == 0) ||
            (strncasecmp(move, HINT, HINT_LEN) == 0))
            {
            getHint(move);
            printf("H> %s", move);
            }

        /* convert the textual move to something useful */
        if (convertMoveFromText(WHITE, move, xCoords, cxCoords,
                                stateCoords))
            {
            doMove(1, move, xCoords, cxCoords, stateCoords);
            }
        else printf("\aInvalid move: %s", move);

        } /* while 1 */
    }


/***************************************************************************
 *
 * checkColor
 *
 */
int checkColor
    (
    char * color
    )
    {
    int i;
    if (strlen(color) != COLOR_LENGTH) return 0;
    if (*color != '#') return 0;
    for (i = 0, color++; i < 6; i++)
        {
        if (!isxdigit(*color++)) return 0;
        }
    return 1; 
    }


/***************************************************************************
 *
 * main - you should now what this is
 *
 * RETURNS: N/A
 */
void main(int argc, char* argv[])
    {
    int toChild[2];
    int fromChild[2];
    char junk [50];
    int t_flag = 0;
    int d_flag = 0;
    int c_flag = 0;
    int depth = 0;
    char trans_size[15] = "150001";    /* this is the default in gnuchess */
    char cache_size[15] = "18001";     /* this is the default in gnuchess */
    char * timer;
    char ** valid;
    char * pTemp;
    char ch;
    char * color;

    /* set up the default colors in the xpm */
    chess_xpm[DEAD_KING] = (char *)strdup("r c #ff0000");
    chess_xpm[BLACK_PIECE_BACKGROUND] = (char *)strdup("b c #000000");
    chess_xpm[BLACK_PIECE_FOREGROUND] = (char *)strdup("w c #ffffff");
    chess_xpm[DARK_SQUARE] = (char *)strdup("+ c #c8c365");
    chess_xpm[LIGHT_SQUARE] = (char *)strdup("- c #77a26d");

    while ((ch = getopt(argc, argv, "rahtd:c:T:C:k:b:f:1:2:")) != -1)
        {
        switch (ch)
            {
            case 'k': /* background color of a checkmated king */
                if (!checkColor(optarg))
                    {
                    printf("Invalid dead king color!\n");
                    printUsage();
                    }
                else bcopy(optarg, (chess_xpm[DEAD_KING] + 4),
                           COLOR_LENGTH);
                break;
            case 'b': /* background color for the black piece */
                if (!checkColor(optarg))
                    {
                    printf("Invalid black piece background color!\n");
                    printUsage();
                    }
                else bcopy(optarg, (chess_xpm[BLACK_PIECE_BACKGROUND] + 4),
                           COLOR_LENGTH);
                break;
            case 'f': /* foreground color for the black piece */
                if (!checkColor(optarg))
                    {
                    printf("Invalid black piece foreground color!\n");
                    printUsage();
                    }
                else bcopy(optarg, (chess_xpm[BLACK_PIECE_FOREGROUND] + 4),
                           COLOR_LENGTH);
                break;
            case '1': /* dark square color */
                if (!checkColor(optarg))
                    {
                    printf("Invalid dark square color!\n");
                    printUsage();
                    }
                else bcopy(optarg, (chess_xpm[DARK_SQUARE] + 4),
                           COLOR_LENGTH);
                break;
            case '2': /* light square color */
                if (!checkColor(optarg))
                    {
                    printf("Invalid light square color!\n");
                    printUsage();
                    }
                else bcopy(optarg, (chess_xpm[LIGHT_SQUARE] + 4),
                           COLOR_LENGTH);
                break;
            case 'r': /* engine randomness */
                engineRandom = 1;
                break;
            case 'a': /* engine is hard */
                engineHard = 1;
                break;
            case 't': /* text based game */
                t_flag = 1;
                break;
            case 'd': /* engine search depth */
                d_flag = 1;
                valid = &optarg;
                depth = strtol(optarg, valid, 10);
                if (**valid != '\0')
                    {
                    printf("Invalid depth!\n");
                    printUsage();
                    }
                break;
            case 'c': /* engine response time */
                c_flag = 1;
                timer = (char *)strdup(optarg);
                pTemp = timer;
                /* this still allows something like 0::01 - oops ;-) */
                while (isdigit(*pTemp) || (*pTemp == ':')) pTemp++;
                if (*pTemp != '\0')
                    {
                    printf("Invalid timer!\n");
                    printUsage();
                    }
                break;
            case 'T': /* tranposition table size */
                strcpy(trans_size, optarg);
                pTemp = trans_size;
                while (isdigit(*pTemp)) pTemp++;
                if (*pTemp != '\0')
                    {
                    printf("Invalid transition table size!\n");
                    printUsage();
                    }
                break;
            case 'C': /* move cache table size */
                strcpy(cache_size, optarg);
                pTemp = cache_size;
                while (isdigit(*pTemp)) pTemp++;
                if (*pTemp != '\0')
                    {
                    printf("Invalid cache table size!\n");
                    printUsage();
                    }
                break;
            case 'h': /* usage */
            default:
                printUsage();
                break;
            }
        } /* while (getopt) */

    /* create a pipe to and from the child */
    if ((pipe(toChild) == -1) || (pipe(fromChild) == -1))
        {
        perror("pipe");
        exit(1);
        }

    /* fork to exec the chess engine */
    switch (childPid = fork())
        {
        case 0: /* child */
            {
            /* dup stdin and stdout to the pipes and exec */
            close(toChild[1]);
            close(fromChild[0]);
            if ((dup2(toChild[0], 0) == -1) || (dup2(fromChild[1], 1) == -1))
                {
                perror("dup2");
                exit(1);
                }
            if (c_flag)
                {
                if (execl(ENGINE, "gnuchessx", "-T", trans_size,
                          "-C", cache_size, timer, NULL) == -1)
                    {
                    perror("exec");
                    exit(1);
                    }
                }
            else
                {
                if (execl(ENGINE, "gnuchessx", "-T", trans_size,
                          "-C", cache_size, NULL) == -1)
                    {
                    perror("exec");
                    exit(1);
                    }
                }
            }
        case -1:
            perror("fork");
            exit(1);
        }

    /* close unused fds and create FILE pointers */
    close(toChild[0]);
    close(fromChild[1]);
    if (((input = (FILE *)fdopen(fromChild[0], "r")) == NULL) ||
        ((output = (FILE *)fdopen(toChild[1], "w")) == NULL))
        {
        perror("fdopen");
        kill(childPid, SIGTERM);
        exit(1);
        }

    /* create the X window to draw in */
    openXwindow(argc, argv, chess_xpm, chess_mask_bits,
                CHESS_MASK_WIDTH, CHESS_MASK_HEIGHT);

    /* initial the board */
    initBoard();

    /* draw the window twice with a sleep inbetween to make sure it's seen */
    RedrawWindow(); sleep(1); RedrawWindow();

    /* get the first line of junk from the chess engine */
    fgets(junk, 50, input);

    if (engineRandom) forceRandom();
    if (engineHard) forceHard();
    if (d_flag) setDepth(depth);

    if (t_flag) textGame();
    else mouseGame();
    }

