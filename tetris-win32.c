/*** MICROSOFT WINDOWS 10 and up                                                ***
 ***     Only in Windows 10 did Microsoft Windows start supporting ANSI/VT100   ***
 ***     escape sequences (like it used to in old DOS days w/ANSI.SYS)          ***/

static DWORD old_cmode = 0;

/* READ A SINGLE KEY
 *     Returns a function number
 */
int ReadKey(void)
{
    if ( !_kbhit() ) return 0;
    switch(getch()) {
        case  'q':
        case 0x1b: return(QUIT);
        case  'j': return(DOWN);
        case  'h': return(LEFT);
        case  'l': return(RIGHT);
        case  'p': return(PAUSE);
        case  'z': return(TEST);
        case  ' ': return(ROTATE);
        case  'r': return(REDRAW);
        case 0:     /* old DOS: extended key */
        case 224:   /* Win10: extended key (up/dn/lt/rt) */
            switch(getch()) {
                case 0x48: return(ROTATE);
                case 0x50: return(DOWN);
                case 0x4b: return(LEFT);
                case 0x4d: return(RIGHT);
                default: break;
            }
            break;
    }
    return(0);
}

// WINDOWS 10: Enable VT100 positioning codes
void InitTerminal(void)
{
    DWORD cmode = ENABLE_VIRTUAL_TERMINAL_PROCESSING    /* vt100 */
                | ENABLE_PROCESSED_OUTPUT;              /* crlf, backspace, etc */
    HANDLE hStdout;
    hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
    GetConsoleMode(hStdout, &old_cmode);                /* save old modes for exit */
    SetConsoleMode(hStdout, cmode);                     /* assert new mode */
}

void EndTerminal(void)
{
    HANDLE hStdout;
    hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleMode(hStdout, old_cmode);
}


