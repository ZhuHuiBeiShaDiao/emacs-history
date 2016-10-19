/* terminal control module for Concept-100's */ 
 
/*		Copyright (c) 1981,1980 James Gosling 
   Enhancements copyright (c) 1984 Fen Labalme and Chris Torek 
   Distributed by Fen Labalme, with permission from James Gosling. 
 
This file is part of GNU Emacs. 
 
GNU Emacs is distributed in the hope that it will be useful, 
but there is no warranty of any sort, and no contributor accepts 
responsibility for the consequences of using this program 
or for whether it serves any particular purpose. 
 
Everyone is granted permission to copy, modify and redistribute 
GNU Emacs, but only under the conditions described in the 
document "GNU Emacs copying permission notice".   An exact copy 
of the document is supposed to have been given to you along with 
GNU Emacs so that you can know how you may redistribute it all. 
It should be in a file named COPYING.  Among other things, the 
copyright notice and this notice must be preserved on any copy 
distributed.  */ 
 
 
#include <stdio.h> 
#include "config.h" 
#include "Trm.h" 
 
static 
int	curX, curY; 
static 
int	WindowSize, DesWindowSize; 
static 
int	ReverseVideo; 
static 
enum IDmode { m_insert = 1, m_overwrite = 0 } 
	CurMode, DesMode; 
 
static 
enum { C100, C108, perq } RealType; 
 
static 
INSmode (new) 
enum IDmode new; { 
	DesMode = new; 
}; 
 
static curHL; 
static 
HLmode (on) { 
    if (curHL == on) 
	return; 
    printf (on ? (ReverseVideo ? "\033N\110" : "\033N\170") 
	       : (ReverseVideo ? "\033N\130" : "\033N\110")); 
    curHL = on; 
} 
 
static 
setmode () { 
    if (DesMode == CurMode) 
	return; 
    putchar (033); 
    putchar (DesMode == m_insert ? 16 : 0200); 
    CurMode = DesMode; 
}; 
 
static 
inslines (n) { 
    register    line = curY; 
  /* This is commented out because it appears to work wrong  
    if (SetWindow (DesWindowSize)) 
	topos (line, 1); 
  */ 
    HLmode (0); 
    if (n > 1 && RealType == perq) 
	printf ("\033\036%c", n + 32); 
    else 
	while (--n >= 0) { 
	    printf ("\033\022"); 
	    pad (tt.t_length - curY, 750L); 
	} 
}; 
 
static 
dellines (n) { 
    register    line = curY; 
  /* This is commented out because it appears to work wrong 
    if (SetWindow (DesWindowSize)) 
	topos (line, 1); 
  */ 
    if (n > 1 && RealType == perq) 
	printf ("\033\037%c", n + 32); 
    else 
	while (--n >= 0) { 
	    printf ("\033\002"); 
	    pad (tt.t_length - curY, 750L); 
	} 
}; 
 
static 
writechars (start, end) 
register char	*start, 
		*end; { 
    register char *p; 
    register int i; 
    setmode (); 
    while (start <= end) { 
	if (*start == '_' && CurMode != m_insert) { 
	    register runlen = 0; 
	    do runlen++; while(*++start == '_' && start<=end); 
	    printf ("\033r_%c", 32+runlen); 
	    pad (runlen, 100L); 
	    curX += runlen; 
	} 
	else 
	    if(CurMode==m_insert 
	     || ((p=start+1),*start!=*p++) || *start!=*p++ 
	     || *start!=*p++ || *start!=*p++ || p>end){ 
		putchar (*start++); 
		curX++; 
	    } else {		/* we have a run of at least 5 characters */ 
		i = 5; 
		while (*start == *p++ && p<=end) i++; 
		printf ("\033r%c%c", *start, 32+i); 
		pad (i, 100L); 
		curX += i; 
		start = p-1; 
	    } 
	if (CurMode == m_insert) 
	    pad (tt.t_width - curX, 50L); 
    } 
}; 
 
static 
update_end () 
{ 
  HLmode (0); 
} 
 
static 
blanks (n) { 
    setmode (); 
    if (CurMode == m_insert || n <= 5) 
	while (--n >= 0) { 
	    putchar (' '); 
	    curX++; 
	    if (CurMode == m_insert) 
		pad (tt.t_width - curX, 50L); 
	} 
    else { 
	printf ("\033r %c", n + 32); 
	pad (n, 50L); 
	curX += n; 
    } 
}; 
 
static 
topos (row, column) register row, column; { 
    if(row > WindowSize) 
	(void) SetWindow(0); 
    if (curY == row) { 
	if (curX == column) 
	    return; 
	if (curX == column + 1) { 
	    putchar (010); 
	    goto done; 
	} 
    } 
    if (curY + 1 == row && (column == 1 || column==curX)) { 
	if(column!=curX) putchar (015); 
	putchar (012); 
	goto done; 
    } 
    if (row == 1 && column == 1) { 
	putchar (033); 
	putchar ('?'); 
	goto done; 
    } 
    putchar (033); 
    putchar ('a'); 
    putchar (row + 31); 
    if (column<96) putchar (column + 31); 
    else { putchar (1); putchar (column-65); } 
done: 
    curX = column; 
    curY = row; 
}; 
 
static 
init (BaudRate) { 
    double BaudFactor; 
    if (RealType == perq || RealType == C108 && BaudRate <= 1200) 
	BaudFactor = 0.0; 
    else 
	BaudFactor = BaudRate / (1- (.45 + .3 * BaudRate / 9600.)) / 10000.; 
    tt.t_ILpf = RealType == perq ? 0.0 : BaudFactor * 0.75; 
    tt.t_ILov = 2; 
    tt.t_DLpf = tt.t_ILpf; 
    tt.t_DLov = tt.t_ILov; 
    tt.t_ICmf = (RealType == perq ? 0.0 : BaudFactor * .05 * 60) + 1.0; 
    tt.t_ICov = 4; 
    tt.t_DCmf = BaudFactor * .05 * 60 + 2.0; 
    tt.t_DCov = 0; 
}; 
 
static 
reset () { 
    curX = -1; 
    curY = -1; 
    curHL = -1; 
    HLmode (0); 
    printf ("\014\0337\0338\033s\033U\033%c", ReverseVideo ? 'k' : 'K'); 
    pad (tt.t_length, 500L); 
    if (tt.t_width > 100) printf("\033\""); 
    CurMode = m_insert; 
    DesMode = m_overwrite; 
}; 
 
static 
cleanup () { 
    HLmode (0); 
    putchar (033); 
    putchar (0); 
}; 
 
static 
wipeline () { 
    putchar (033); 
    putchar (21); 
    pad (tt.t_width - curX, 100L); 
}; 
 
static 
wipescreen () { 
    putchar (014); 
    curX = curY = -1; 
}; 
 
static 
delchars (n) { 
    while (--n >= 0) { 
	putchar (033); 
	putchar (1); 
	pad (tt.t_width - curX, 100L); 
    } 
}; 
 
/* Record that higher levels say that an n-line window 
   is sufficient for all the scrolling to be done just now */ 
static 
window (n) { 
    DesWindowSize = n<=0 ? tt.t_length : n; 
} 
 
static 
flash () { 
    register char c = ReverseVideo ? 'K' : 'k'; 
    printf("\033%c", c); 
    pad (1, 100000L); 
    printf("\033%c", c ^ 040); 
} 
 
static 
SetWindow (n) { 
    if (n <= 0 || n > tt.t_length) 
	n = tt.t_length; 
    if (n != WindowSize) { 
	if (tt.t_width < 100) 
	    printf ("\033v  %c\160", n + 32); 
	else 
	    printf ("\033v  %c\001D", n + 32); 
	curX = 1; 
	curY = 1; 
	WindowSize = n; 
	return 1; 
    } 
    return 0; 
} 
 
TrmC100 (tname) 
char   *tname; { 
    tt.t_width = 79; 
    ReverseVideo = 0; 
    RealType = C100; 
    while (*tname) 
	switch (*tname++) { 
	    case '8':  
		RealType = C108; 
		break; 
	    case 'q': 
		RealType = perq; 
		break; 
	    case 'w':  
		tt.t_width = 131; 
		break; 
	    case 'v':  
		ReverseVideo++; 
		break; 
	} 
    tt.t_needspaces = 1; 
    tt.t_INSmode = INSmode; 
    tt.t_HLmode = HLmode; 
    tt.t_inslines = inslines; 
    tt.t_dellines = dellines; 
    tt.t_blanks = blanks; 
    tt.t_init = init; 
    tt.t_cleanup = cleanup; 
    tt.t_wipeline = wipeline; 
    tt.t_wipescreen = wipescreen; 
    tt.t_topos = topos; 
    tt.t_reset = reset; 
    tt.t_delchars = delchars; 
    tt.t_writechars = writechars; 
    tt.t_UpdateEnd = update_end; 
    tt.t_window = window; 
    tt.t_flash = flash; 
    tt.t_length = 24; 
    if (RealType == perq) { 
	tt.t_flash = 0; 
	tt.t_width = 79; 
	tt.t_length = 60; 
	MetaFlag++; 
    } 
}; 
