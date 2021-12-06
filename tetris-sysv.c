/*** UNIX                                                    ***
 ***     Linux, and other non-BSD unix compatible terminals. ***
 ***                                                         ***/
#include <stdio.h>
#include <termio.h>

static struct termio G_tio,                       /* game settings */
                     G_tiosave;                   /* saved UNIX settings */

/* RESTORE USERS'S ORIGINAL TERMINAL SETTINGS */
void EndTerminal(void)
{
    ioctl(fileno(stdin), TCSETA, &G_tiosave);     /* assert old settings */
}

/* WHEN USER HITS ^C -- WE MUST RESET TERMINAL BACK TO NORMAL */
void SIGINTTrap()
{
    signal(SIGINT, SIGINTTrap);
    EndTerminal();
    ClearScreen();
    fflush(stdout);
    fprintf(stderr,"SIGINT: terminating\n");
    exit(1);
}

/* FORCE UNIX TO READ TERMINAL KEYS NON-BUFFERED/NO ECHO */
void InitTerminal(void)
{
    ioctl(fileno(stdin), TCGETA, &G_tiosave);     /* save current settings */
    ioctl(fileno(stdin), TCGETA, &G_tio);         /* get ready to modify */
    G_tio.c_lflag     = ISIG;                     /* No cooking (sigs ok) */
    G_tio.c_cc[VMIN]  = 0;                        /* No waiting */
    G_tio.c_cc[VTIME] = 0;                        /* No kidding */
    ioctl(fileno(stdin), TCSETA, &G_tio);         /* assert new settings */
    signal(SIGINT, SIGINTTrap);
}

/* READ A SINGLE KEY (under UNIX)
 *     Returns a function number.
 */
int ReadKey(void)
{
    static int esc = 0;
    char c;

    if (read(fileno(stdin), &c, 1) != 1)       /* read character */
        { USLEEP(500); return(0); }            /* none ready? return 0. usleep keeps cpu idle */

    /* Handle arrow keys -- these are 3 character sequences:
     *    up=ESC[A, down=ESC[B, right=ESC[C, left=ESC[D
     */
    switch ( esc ) {
        case 0: if ( c == 0x1b ) { esc = 1; return 0; }
                break;     /* fall thru to normal char handling */
        case 1: if ( c == '[' ) { esc = 2; return 0; }
                esc = 0;   /* reset */
                break;     /* fall thru to normal char handling */
        case 2: {
            int ret = 0;
            /* LocateXY(1,1); printf("GOT '%c'\n", c); */
            switch (c) {
                case 'A': ret = ROTATE; break;  /*    UP KEY */
                case 'B': ret = DOWN;   break;  /*  DOWN KEY */
                case 'C': ret = RIGHT;  break;  /* RIGHT KEY */
                case 'D': ret = LEFT;   break;  /*  LEFT KEY */
            }
            esc = 0;     /* last char in sequence, reset */
            return ret;
       }
    }

    esc = 0;
    switch(c) {
        case  'q': return(QUIT);
        case  'j': return(DOWN);
        case  'h': return(LEFT);
        case  'l': return(RIGHT);
        case  'p': return(PAUSE);
        case  'z': return(TEST);
        case  ' ': return(ROTATE);
        case  'r': return(REDRAW);
    }
    return(0);
}
