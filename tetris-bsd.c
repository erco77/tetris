/***
 *** BSD STYLE TERMIOS
 ***     Uses the termios(4) interface.
 ***/
#include <stdio.h>
#include <termios.h>

static struct termios G_tio,                       /* game settings */
                      G_tiosave;                   /* saved UNIX settings */

/* RESTORE USERS'S ORIGINAL TERMINAL SETTINGS */
void EndTerminal(void)
{
    if (tcsetattr(fileno(stdin), TCSAFLUSH, &G_tiosave) < 0) { /* return to previous tty settings */
        fprintf(stderr, "can't restore tty settings\n");
        exit(1);
    }
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

/* put terminal in raw mode - see termio(7I) for modes */
void InitTerminal(void)
{
    if (!isatty(fileno(stdin))) {
        fprintf(stderr, "not on a tty\n");
        exit(1);
    }
    if (tcgetattr(fileno(stdin), &G_tiosave) < 0) {
        fprintf(stderr, "can't get current tty settings\n");
        exit(1);
    }
    G_tio = G_tiosave;                  /* copy original and modify.. */
    /*
    G_tio.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    G_tio.c_oflag &= ~(OPOST);
    G_tio.c_cflag |= (CS8);
    */
    G_tio.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);   /* echo off, no cooking or signals */
    G_tio.c_cc[VMIN] = 0; G_tio.c_cc[VTIME] = 0;         /* no waiting */
    if (tcsetattr(fileno(stdin), TCSAFLUSH, &G_tio) < 0) {
        fprintf(stderr, "can't set tty settings\n");
        exit(1);
    }
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
