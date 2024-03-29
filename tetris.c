/* vim: autoindent tabstop=8 shiftwidth=4 expandtab softtabstop=4
 */
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <time.h>

#define VERSION "1.33"

/***********************************************************************
 *
 * TETRIS - An interactive game implemented for text screen terminals
 *
 *     Rev  Date     Author          Description
 *     ---- -------- --------------- ----------------------
 *     1.00 05/19/92 Greg Ercolano   Initial Implementation
 *     1.20 11/29/17 Greg Ercolano   Unix support for arrow keys
 *     1.30 11/30/17 Greg Ercolano   Code cleanup, K+R -> C99
 *     1.31 08/01/20 Greg Ercolano   Slightly faster row flash
 *     1.32 12/05/21 Greg Ercolano   Windows 10 DOS term support
 *     1.33 12/05/21 Greg Ercolano   MacOS BSD term support
 *     x.xx xx/xx/xx xxxx            xxxx
 *
 *     Tested to compile and work on the following machines:
 *
 *         1) Linux (make tetris)
 *         2) MacOS (make tetris)
 *         3) Microsoft Windows 10 + Visual Studio command line: (make.bat tetris)
 *
 *     NOTE: OSX uses BSD style termio; would need mods for that.
 *
 ***********************************************************************/

/* PLATFORM SPECIFIC */
#ifdef _WIN32
    #include <windows.h>                /* Sleep(msec) */
    #include <conio.h>                  /* _kbhit() */
    #define USLEEP(val) Sleep(val/1000) /* microsecs -> millisec */
#else
    #include <unistd.h>                 /* usleep() */
    #define USLEEP(val) usleep(val)     /* microsecs */
#endif

/* KEYSTROKE TRANSLATIONS */
#define DOWN   1
#define LEFT   2
#define RIGHT  3
#define ROTATE 4
#define QUIT   5
#define PAUSE  6
#define TEST   7
#define REDRAW 8

/* REDRAW MODES */
#define CHANGED 0               /* redraw only whats changed */
#define WINDOW  1               /* redraw only outer window (and score/preview) */
#define GSCREEN 2               /* redraw only playing screen */
#define ALL     WINDOW|GSCREEN  /* complete redraw */

/* SCREEN PIXELS */
#define NOSHAPECOLOR    "  "
#define VTSHAPECOLOR    "\33[7m##\33[0m"
#define WYSHAPECOLOR    "\33`6\33)##\33("

/* SHAPE/SCREEN ORIENTATION */
#define GAMEHEIGHT      20
#define GAMEWIDTH       10
#define TOPOFFSET       2
#define PREVIEWYOFFSET  15
#define PREVIEWXOFFSET  47
#define LEFTOFFSET      20
#define MAXSHAPES       7
#define SHAPEMAX        4       /* width/height of shape characters */

/* GSCREEN PIXEL BIT PLANES */
#define NOSHAPE         0       /* empty space */
#define NEWSHAPE        1       /* current moving shape */
#define OLDSHAPE        2       /* last moving shape */
#define COLLIDESHAPE    3       /* new & last shape collision */
#define PETRIFIEDSHAPE  4       /* petrified into position */

#define ABS(a)          (((a)<0)?-(a):(a))
#define SGN(a)          (((a)<0)?(-1):1)         /* binary sign of a: -1, 1 */
#define ZSGN(a)         (((a)<0)?(-1):(a)>0?1:0) /* sign of a: -1, 0, 1 */

/* GLOBALS */
char *Gterm = 0;
char *Gwindow[] = {
"\t\t ::::::::::::::::::::::::              ",
"\t\t ::                    ::              ",
"\t\t ::                    ::              ",
"\t\t ::                    ::              ",
"\t\t ::                    ::              ",
"\t\t ::                    ::              ",
"\t\t ::                    ::              ",
"\t\t ::                    ::              ",
"\t\t ::                    ::              ",
"\t\t ::                    ::              ",
"\t\t ::                    ::              ",
"\t\t ::                    ::              ",
"\t\t ::                    ::    ..........",
"\t\t ::                    ::    :        :",
"\t\t ::                    ::    :        :",
"\t\t ::                    ::    :        :",
"\t\t ::                    ::    :        :",
"\t\t ::                    ::    :        :",
"\t\t ::                    ::    :        :",
"\t\t ::                    ::    :........:",
"\t\t ::                    ::              ",
"\t\t ::::::::::::::::::::::::    Rows =    ",
NULL
};

/* SHAPE TABLES (Indexing: [shape] [y] [rotation] [x]) */
char *Gshapes[MAXSHAPES+1][4][4] = {    /* shape icons */
  {
    { "    ", "    ", "    ", "    " }, /* 4 rotated positions of each shape */
    { " ## ", " ## ", " ## ", " ## " },
    { " ## ", " ## ", " ## ", " ## " },
    { "    ", "    ", "    ", "    " },
  },{
    { "    ", " #  ", " #  ", " #  " },
    { "### ", " ## ", "### ", "##  " },
    { " #  ", " #  ", "    ", " #  " },
    { "    ", "    ", "    ", "    " },
  },{
    { "    ", " #  ", "    ", " #  " },
    { "####", " #  ", "####", " #  " },
    { "    ", " #  ", "    ", " #  " },
    { "    ", " #  ", "    ", " #  " },
  },{
    { "    ", " #  ", "  # ", "##  " },
    { "### ", " #  ", "### ", " #  " },
    { "#   ", " ## ", "    ", " #  " },
    { "    ", "    ", "    ", "    " },
  },{
    { "    ", " ## ", "#   ", " #  " },
    { "### ", " #  ", "### ", " #  " },
    { "  # ", " #  ", "    ", "##  " },
    { "    ", "    ", "    ", "    " },
  },{
    { " ## ", " #  ", " ## ", " #  " },
    { "##  ", " ## ", "##  ", " ## " },
    { "    ", "  # ", "    ", "  # " },
    { "    ", "    ", "    ", "    " },
  },{
    { "##  ", "  # ", "##  ", "  # " },
    { " ## ", " ## ", " ## ", " ## " },
    { "    ", " #  ", "    ", " #  " },
    { "    ", "    ", "    ", "    " },
  }
};

/* GLOBAL VARIABLES */
int Gnextshape,                         /* the next shape coming */
    Gshape,                             /* current shape (0 if none selected) */
    Gx,Gy,Grotate,                      /* current shape's orientation */
    Grows=0,                            /* completed rows (score) */
    Glastrows=0,                        /* (last displayed score) */
    Gtest=0;                            /* test mode */

char Gscreen[GAMEHEIGHT][GAMEWIDTH];    /* game screen's memory space */

/* CLEAR THE TERMINAL SCREEN */
void ClearScreen(void)
{
    if (Gterm) {
        if (Gterm[0]=='w' && Gterm[1]=='y') {   /* WYSE TERMINALS */
            printf("%c%c%c\r",0x1b,0x28,0x1a);  /* CLEAR */
            printf("%c",0x1e);                  /* HOME */
            /* printf("%c%c%c%c",0x1b,0x3d,0x20,0x20); */
            return;
        }
    }

    /* CLEAR/HOME FOR VT100/IRIS-ANSI/IBMPC-ANSI TERMINALS */
    printf("\33[2J\33[1;1H\r");
}

/* LOCATE THE CURSOR ON THE TERMINAL SCREEN */
void LocateXY(int x, int y)      /* x: 1-80, y:1-24 */
{
    if (Gterm && Gterm[0]=='w') {               /* WYSE TERMINALS */
        printf("%c%c%c%c",0x1b, 0x3d, 0x20+y-1, 0x20+x-1);
    } else {
        printf("\33[%d;%dH",y,x);               /* VT100/IBMPC/etc */
    }
}

/* DRAW A PIXEL ON THE TERMINAL AT CURRENT CURSOR POSITION
 *     on: 1=pixel is on, 0=pixel is off
 */
void DrawPixel(int on)
{
    if (on) printf("%s",(Gterm && Gterm[0]=='w') ? WYSHAPECOLOR : VTSHAPECOLOR);
    else    printf("%s",NOSHAPECOLOR);
}

/* DRAW THE SHAPE THAT IS BEING "PREVIEWED" IN THE RIGHT HAND BOX */
void DrawPreview(void)
{
    int x,y;
    for (y=0; y<SHAPEMAX; y++) {
        LocateXY(PREVIEWXOFFSET,y+PREVIEWYOFFSET+1);
        for (x=0; x<SHAPEMAX; x++)
            DrawPixel( (Gshapes[Gnextshape][y][0][x]=='#') ? 1 : 0 );
    }
}

/* UPDATE SCORE IF CHANGED (or forced to update) */
void UpdateScore(int force)
{
    if (force || Grows!=Glastrows) {
        LocateXY(54,22);
        printf("%d ",Grows);
        Glastrows = Grows;
    }
}

/* COME UP WITH A NEW SHAPE */
void MakeNewShape(int update)
{
    Grotate    = 0;
    Gx         = GAMEWIDTH/2 - SHAPEMAX/2;
    Gy         = -3;
    Gshape     = Gnextshape;
    Gnextshape = (rand() % MAXSHAPES);
    if (update) DrawPreview();
    return;
}

/* REDRAW THE SCREEN
 * 'all' bit flags:
 *     0       -- draws moving shape + score
 *     WINDOW  -- clear screen, redraw game's Gwindow[] outline, score+preview.
 *     GSCREEN -- redraw the Gscreen[] buffer of pieces, score+preview.
 */
void Redraw(int all)
{
    int x,y;
    char flags;

    /* REDRAW WINDOW
     *    Outline for game + preview + "Rows ="
     */
    if (all & WINDOW) {
        ClearScreen();
        for (y=0; Gwindow[y]; y++)
            printf("%s\n",Gwindow[y]);
    }

    /* REDRAW SCREEN BUFFER
     *    Redraws all pieces
     */
    if (all & GSCREEN) {
        for (y=0; y<GAMEHEIGHT; y++) {
            LocateXY(LEFTOFFSET,y+TOPOFFSET);
            for (x=0; x<GAMEWIDTH; x++)
                DrawPixel((Gscreen[y][x]==NOSHAPE)?0:1);
        }
    }

    /* DRAW TEST SCREEN (DEBUGGING) */
    if (Gtest) {
        for (y=0; y<GAMEHEIGHT; y++) {
            LocateXY(0,y+TOPOFFSET);
            for (x=0; x<GAMEWIDTH; x++)
                printf("%d",Gscreen[y][x]);
        }
    }

    if (all) {
        UpdateScore(1);
        DrawPreview();
    } else {
        for (y=0; y<GAMEHEIGHT; y++) {
            for (x=0; x<GAMEWIDTH; x++) {
                flags = Gscreen[y][x] & (COLLIDESHAPE^0xffff);
                switch(Gscreen[y][x]&COLLIDESHAPE) {
                    /* NEWLY DRAWN */
                    case NEWSHAPE:
                        LocateXY(x*2+LEFTOFFSET,y+TOPOFFSET);
                        DrawPixel(1);
                        Gscreen[y][x] = OLDSHAPE|flags;
                        break;

                    /* LEFTOVER TO BE ERASED */
                    case OLDSHAPE:
                        LocateXY(x*2+LEFTOFFSET,y+TOPOFFSET);
                        DrawPixel(0);
                        Gscreen[y][x] = NOSHAPE|flags;
                        break;

                    /* OVERLAPPED LAST SHAPE */
                    case COLLIDESHAPE:
                        Gscreen[y][x] = OLDSHAPE|flags;
                        break;
                }
            }
        }
        UpdateScore(0);
    }
    LocateXY(1,1);
    fflush(stdout);
}

/* CLEAR THE GAME/INITIALIZE VARIABLES */
void Clear(void)
{
    time_t lt;

    Gnextshape= 0;
    Gshape    = 0;
    Gx        = 0;
    Gy        = -3;
    Grotate   = 0;
    Grows     = 0;
    Glastrows = -1;

    time(&lt);
    srand((int)lt);

    /* CLEAR THE GAME SCREEN BITMAP */
    {
        int x,y;
        for (x=0; x<GAMEWIDTH; x++)
            for (y=0; y<GAMEHEIGHT; y++)
                Gscreen[y][x] = NOSHAPE;
    }
    MakeNewShape(0);     /* new shape */
    MakeNewShape(0);     /* and another for preview */
    Redraw(ALL);
}

#ifdef _WIN32
#include "tetris-win32.c"
#endif

#ifdef __APPLE__
#include "tetris-bsd.c"
#endif

#if defined(__linux__) || defined(__unix__)
#include "tetris-sysv.c"
#endif

time_t lasttime=0;

/* HANDLE TIMER FOR SHAPE DROPPING
 * Returns '1' if shape should be moved
 */
int HandleTimer(int *yforce)
{
    time_t lt;
    time(&lt);
    if (lt!=lasttime) { lasttime = lt; if (yforce) *yforce = 1; return(1); }
    else              { if (yforce) *yforce = 0; return(0); }
}

/* EXIT PROGRAM WITH SCORE SHOWN */
void Texit(char *msg, int v)
{
    //ClearScreen();
    printf("\033[24H\r");
    if ( msg ) printf("%s\n", msg);
    printf("Total rows: %d\n",Grows);
    fflush(stdout);
    EndTerminal();
    exit(v);
}

/* HANDLE READING BUTTON EVENTS AND UPDATING POSITION VARIS */
int HandleButtons(int *x, int *y, int *rotate, int *yforce)
{
    int c, events=0;
    /* READ ALL BUFFERED KEYSTROKES */
    while ((c=ReadKey())) {
        switch(c) {
            case   LEFT: *x      -= 1; ++events; break;
            case  RIGHT: *x      += 1; ++events; break;
            case   DOWN: *y      += 1; ++events; break;
            case ROTATE: *rotate += 1; ++events; break;
            case   QUIT: Texit("Quit", 1);       break;
            case  PAUSE: while (!ReadKey()) { }  break;
            case   TEST: Gtest ^= 1;             break;  /* testing mode */
            case REDRAW: Redraw(ALL);            break;
        }
    }
    return(events);
}

#define SIDES(x,y) ((x)<0||(x)>=GAMEWIDTH)
#define BOTT(x,y) ((y)>=GAMEHEIGHT)
#define TOP(x,y)  ((y)<0)
#define CLIP(x,y) (TOP(x,y)||BOTT(x,y)||SIDES(x,y))

/* CHECK IF SHAPE OVERLAPS OTHERS
 * Returns
 *      1 - hit left or right edges
 *      2 - hit bottom or petrified boxes
 */
int CollisionCheck(int x, int y, int rotate)
{
    int t, r;
    int err=0;

    for (t=0; t<SHAPEMAX; t++) {
        for (r=0; r<SHAPEMAX; r++) {
            if (Gshapes[Gshape][t][rotate][r]=='#') {
                if (BOTT(x+r,y+t)) {
                    return(2);         /* hit bottom */
                }
                if (SIDES(x+r,y+t)) {  /* hit edge, but continue looking for */
                    err = 1;           /* petrified shape or bottom */
                } else {
                    if (Gscreen[y+t][x+r]==PETRIFIEDSHAPE) {
                        return(2);     /* hit petrified shape */
                    }
                }
            }
        }
    }
    return(err);
}

/* DRAWS THE SHAPE INTO THE SCREEN ARRAY
 * Returns
 *      1 - hit left or right edges
 *      2 - hit bottom or petrified boxes
 */
int DrawShape(int x, int y, int rotate)
{
    int t, r;

    /* DRAW THE SHAPE INTO THE SCREEN ARRAY */
    for (t=0; t<SHAPEMAX; t++) {
        for (r=0; r<SHAPEMAX; r++) {
            if (CLIP(x+r, y+t)) {
                continue;
            } else {
                Gscreen[y+t][x+r] |=
                    (Gshapes[Gshape][t][rotate][r]=='#') ? NEWSHAPE : NOSHAPE;
            }
        }
    }
    return(0);
}

 /* PETRIFY ALL SHAPES ON THE SCREEN */
void PetrifyScreen(void)
{
    int x, y;

    /* THIS PETRIFIES ANY OLD/OVERLAP DATA INTO PLACE
     * (ie. the last drawn shape)
     */
    for (y=0; y<GAMEHEIGHT; y++)
        for (x=0; x<GAMEWIDTH; x++)
            Gscreen[y][x] = (Gscreen[y][x]) ? PETRIFIEDSHAPE : NOSHAPE;
    MakeNewShape(1);
}

/* FLASH THE ROWS */
void FlashCompletedRows(int *rows, int trows)
{
    int x,t,r;
    for (t=1; t<3; t++) {   /* off-on-off */
        for (r=0; r<trows; r++)
            for (x=0; x<GAMEWIDTH; x++)
                Gscreen[rows[r]][x] = (t&1) ? OLDSHAPE : NEWSHAPE;
        Redraw(CHANGED);
        USLEEP(300000);     /* approx 1/3 sec delay */
    }
}

/* DELETE THE COMPLETED ROWS */
void DeleteCompletedRows(int *rows, int trows)
{
    int x,y,t;
    for (t=0; t<trows; t++)
        for(y=rows[t]; y>0; y--)
            for (x=0; x<GAMEWIDTH; x++)
                Gscreen[y][x] = Gscreen[y-1][x];    /* line above */
}

/* FIND COMPLETED ROWS, AND DELETE ACCORDINGLY */
void HandleCompletedRows(void)
{
    int x,y,total,trows=0, rows[GAMEHEIGHT];

    /* Find total completed rows (trows) */
    for (y=0; y<GAMEHEIGHT; y++) {
        for (x=0,total=GAMEWIDTH; x<GAMEWIDTH; x++) {
            if (Gscreen[y][x]==NOSHAPE) break;
            else                        --total;
        }
        if (total==0) { rows[trows++] = y; }
    }

    /* Found completed rows? Handle.. */
    if (trows) {
        FlashCompletedRows(rows, trows);    /* briefly flashes completed rows on+off */
        DeleteCompletedRows(rows, trows);   /* removes completed rows */
        Redraw(GSCREEN);
        Grows += trows;
    }

    /* CLEAR THE KEYBOARD BUFFER */
    while (ReadKey()) { }
}

/* HANDLE SHAPE DRAWING/COLLISIONS/CLIPPING */
void HandleShape(int *x, int *y, int *rotate, int *yforce)
{
    /* CHECK IF SHAPE TOUCHES OTHERS OR SCREEN BOTTOM IN REQUESTED ORIENTATION
     */
    while (1) {
        switch(CollisionCheck(Gx + *x,
                              Gy + *y + *yforce,
                              (Grotate + *rotate) % 4)) {
            case 1: /* LEFT/RIGHT EDGE COLLISION */
                /* Undo button events until no collision */
                if (*rotate!=0) { *rotate -= ZSGN(*rotate); continue; }
                if (*x!=0)      { *x -= ZSGN(*x); continue; }
                if (*y!=0)      { *y -= ZSGN(*y); continue; }
                break;

            case 2: /* BOTTOM OR PETRIFIED COLLISION */
                /* Undo button events to avoid collision */
                if (ABS(*rotate)!=0) { *rotate -= ZSGN(*rotate); continue; }
                if (ABS(*x)!=0)      { *x      -= ZSGN(*x);      continue; }
                if (ABS(*y)!=0)      { *y      -= ZSGN(*y);      continue; }

                if (Gy<1) Texit("YOU DIED.", 1);

                /* Draw shape in old position and petrify accordingly */
                DrawShape(Gx, Gy, (Grotate % 4));
                PetrifyScreen();
                HandleCompletedRows();
                break;
        }
        break;
    }

    /* APPLY THE REVISED MOVEMENTS TO THE MOVING SHAPE */
    Gx      += *x;
    Gy      += (*y + *yforce);
    Grotate += *rotate;
    DrawShape(Gx, Gy, (Grotate % 4));
}

int main()
{
    char s[5];
    int x,y,rotate,yforce;

    fprintf(stderr,
        "\nTetris for terminals - V %s - 1992,2017 Greg Ercolano\n"
        "\n"
        "  You can either use arrow keys or VI keys to play:\n"
        "\n"
        "    Arrow Keys                 VI keys\n"
        "    ========================   =======================\n"
        "    DN/LT/RT -- move piece     jhl     -- move piece down/left/right\n"
        "    UP       -- rotate piece   <space> -- rotate piece\n"
        "\n"
        "    'p' to pause game, 'q' to quit.\n"
        "\n\n"
        "Hit Enter to start: ", VERSION);
    fgets(s, sizeof(s), stdin);

    InitTerminal();                     /* init termios */
    Gterm = getenv("TERM");

    Clear();
    while (1) {
        x = y = rotate = yforce = 0;
        HandleTimer(&yforce);           /* forces piece downward by clock time */
        HandleButtons(&x, &y, &rotate, &yforce);
        if ( x || y || rotate || yforce) {
            HandleShape(&x, &y, &rotate, &yforce);
            Redraw(CHANGED);            /* redraw only if something changed */
        }
    }
    Texit("WHILE LOOP", 1);
    return 0;
}
