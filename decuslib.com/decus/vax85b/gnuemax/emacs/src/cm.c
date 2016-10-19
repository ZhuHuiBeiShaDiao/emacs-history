/* Cursor motion subroutines for GNU Emacs. 
   Copyright (C) 1984 Richard M. Stallman 
    based primarily on public domain code written by Chris Torek 
 
This file is part of GNU Emacs. 
 
GNU Emacs is distributed in the hope that it will be useful, 
but without any warranty.  No author or distributor 
accepts responsibility to anyone for the consequences of using it 
or for whether it serves any particular purpose or works at all, 
unless he says so in writing. 
 
Everyone is granted permission to copy, modify and redistribute 
GNU Emacs, but only under the conditions described in the 
document "GNU Emacs copying permission notice".   An exact copy 
of the document is supposed to have been given to you along with 
GNU Emacs so that you can know how you may redistribute it all. 
It should be in a file named COPYING.  Among other things, the 
copyright notice and this notice must be preserved on all copies.  */ 
 
 
#include <stdio.h> 
#include <sgtty.h> 
#include "cm.h" 
 
#define	BIG	9999		/* 9999 good on VAXen.  For 16 bit machines 
				   use about 2000.... */ 
 
char	*malloc (), *mytgoto (), *getenv (); 
 
static int cost;		/* sums up costs */ 
 
/* 
 * In order to accomodate binary cursor motion on certain terminals, the NUL 
 * character '\0' is put into the string by "character stuffing" using 4 as 
 * the magic escape code.  (0 breaks the termcap routines since that is their 
 * end-of-string marker.)  The cmevalcost and cmput routines decode these. 
 */ 
 
static 
cmevalcost (c) char c; { 
    static int esc; 
 
    if (esc) { 
	esc = 0; 
	return; 
    } 
    if (c == 4) 
	esc++; 
    cost++; 
} 
 
static 
cmput (c) char c; { 
    static int esc; 
 
    if (esc) { 
	esc = 0; 
	if (c == 9) 
	    c = 0; 
	putchar (c); 
	return; 
    } 
    if (c == 4) { 
	esc++; 
    } 
    else 
	putchar (c); 
} 
 
/* 
 * But here we don't bother with such silliness. 
 */ 
 
/* ARGSUSED */ 
static evalcost (c) char c; { 
    cost++; 
} 
 
static put (c) char c; { 
    putchar (c); 
} 
 
/* NEXT TWO ARE DONE WITH MACROS */ 
#if 0 
/* 
 * Assume the cursor is at row row, column col.  Normally used only after 
 * clearing the screen, when the cursor is at (0, 0), but what the heck, 
 * let's let the guy put it anywhere. 
 */ 
 
static 
at (row, col) { 
    curY = row; 
    curX = col; 
} 
 
/* 
 * Add n columns to the current cursor position. 
 */ 
 
static 
addcol (n) { 
    curX += n; 
 
    /* 
     * If cursor hit edge of screen, what happened? 
     * N.B.: DO NOT!! write past edge of screen.  If you do, you 
     * deserve what you get.  Furthermore, on terminals with 
     * autowrap (but not magicwrap), don't write in the last column 
     * of the last line. 
     */ 
 
    if (curX == Wcm.cm_cols) { 
	/* 
	 * Well, if magicwrap, still there, past the edge of the 
	 * screen (!).  If autowrap, on the col 0 of the next line. 
	 * Otherwise on last column. 
	 */ 
 
	if (Wcm.cm_magicwrap) 
	    ;			/* "limbo" */ 
	else if (Wcm.cm_autowrap) { 
	    curX = 0; 
	    curY++;		/* Beware end of screen! */ 
	} 
	else 
	    curX--; 
    } 
} 
#endif 
 
/* 
 * (Re)Initialize the cost factors, given the output speed of the terminal 
 * in the variable ospeed.  (Note: this holds B300, B9600, etc -- ie stuff 
 * out of <sgtty.h>.) 
 */ 
 
static 
costinit () 
{ 
    char *p; 
 
#define	COST(x,e)	(x ? (cost = 0, tputs (x, 1, e), cost) : BIG) 
 
    Wcm.cc_up =		COST (Wcm.cm_up, evalcost); 
    Wcm.cc_down =	COST (Wcm.cm_down, evalcost); 
    Wcm.cc_left =	COST (Wcm.cm_left, evalcost); 
    Wcm.cc_right =	COST (Wcm.cm_right, evalcost); 
    Wcm.cc_home =	COST (Wcm.cm_home, evalcost); 
    Wcm.cc_cr =		COST (Wcm.cm_cr, evalcost); 
    Wcm.cc_ll =		COST (Wcm.cm_ll, evalcost); 
    Wcm.cc_tab =	Wcm.cm_tabwidth ? COST (Wcm.cm_tab, evalcost) : BIG; 
 
    /* 
     * These last three are actually minimum costs.  When (if) they are 
     * candidates for the least-cost motion, the real cost is computed. 
     * (Note that "0" is the assumed to generate the minimum cost. 
     * While this is not necessarily true, I have yet to see a terminal 
     * for which is not; all the terminals that have variable-cost 
     * cursor motion seem to take straight numeric values.  --ACT) 
     */ 
 
    p = mytgoto (Wcm.cm_abs, 0, 0); 
    Wcm.cc_abs =  COST (p, cmevalcost); 
    p = mytgoto (Wcm.cm_habs, 0, 0); 
    Wcm.cc_habs = COST (p, cmevalcost); 
    p = mytgoto (Wcm.cm_vabs, 0, 0); 
    Wcm.cc_vabs = COST (p, cmevalcost); 
 
#undef	COST 
} 
 
/* 
 * Calculate the cost to move from (srcy, srcx) to (dsty, dstx) using 
 * up and down, and left and right, motions, and tabs.  If doit is set 
 * actually perform the motion. 
 */ 
 
static 
calccost (srcy, srcx, dsty, dstx, doit) { 
    register int    deltay, 
                    deltax, 
                    c, 
                    totalcost; 
    int     ntabs, 
            n2tabs, 
            tabx, 
            tab2x, 
            tabcost; 
    register char  *p; 
 
    totalcost = 0; 
    if ((deltay = dsty - srcy) == 0) 
	goto x; 
    if (deltay < 0) 
	p = Wcm.cm_up, c = Wcm.cc_up, deltay = -deltay; 
    else 
	p = Wcm.cm_down, c = Wcm.cc_down; 
    if (c == BIG) {		/* caint get thar from here */ 
	if (doit) 
	    printf ("OOPS"); 
	return c; 
    } 
    totalcost = c * deltay; 
    if (doit) 
	while (--deltay >= 0) 
	    tputs (p, 1, put); 
x:  
    if ((deltax = dstx - srcx) == 0) 
	goto done; 
    if (deltax < 0) { 
	p = Wcm.cm_left, c = Wcm.cc_left, deltax = -deltax; 
	goto dodelta;		/* skip all the tab junk */ 
    } 
    /* Tabs (the toughie) */ 
    if (Wcm.cc_tab >= BIG || !Wcm.cm_usetabs) 
	goto olddelta;		/* forget it! */ 
 
    /*  
     * ntabs is # tabs towards but not past dstx; n2tabs is one more 
     * (ie past dstx), but this is only valid if that is not past the 
     * right edge of the screen.  We can check that at the same time 
     * as we figure out where we would be if we use the tabs (which 
     * we will put into tabx (for ntabs) and tab2x (for n2tabs)). 
     */ 
 
    ntabs = deltax / Wcm.cm_tabwidth; 
    n2tabs = ntabs + 1; 
    tabx = (srcx / Wcm.cm_tabwidth + ntabs) * Wcm.cm_tabwidth; 
    tab2x = tabx + Wcm.cm_tabwidth; 
 
    if (tab2x >= Wcm.cm_cols)	/* too far (past edge) */ 
	n2tabs = 0; 
 
    /*  
     * Now set tabcost to the cost for using ntabs, and c to the cost 
     * for using n2tabs, then pick the minimum. 
     */ 
 
		   /* cost for ntabs     +    cost for right motion */ 
    tabcost = ntabs ? ntabs * Wcm.cc_tab + (dstx - tabx) * Wcm.cc_right 
		    : BIG; 
 
		   /* cost for n2tabs    +    cost for left motion */ 
    c = n2tabs  ?    n2tabs * Wcm.cc_tab + (tab2x - dstx) * Wcm.cc_left 
		: BIG; 
 
    if (c < tabcost)		/* then cheaper to overshoot & back up */ 
	ntabs = n2tabs, tabcost = c, tabx = tab2x; 
 
    if (tabcost >= BIG)		/* caint use tabs */ 
	goto newdelta; 
 
    /*  
     * See if tabcost is less than just moving right 
     */ 
 
    if (tabcost < (deltax * Wcm.cc_right)) { 
	totalcost += tabcost;	/* use the tabs */ 
	if (doit) 
	    while (--ntabs >= 0) 
		tputs (Wcm.cm_tab, 1, put); 
	srcx = tabx; 
    } 
 
    /*  
     * Now might as well just recompute the delta. 
     */ 
 
newdelta:  
    if ((deltax = dstx - srcx) == 0) 
	goto done; 
olddelta:  
    if (deltax > 0) 
	p = Wcm.cm_right, c = Wcm.cc_right; 
    else 
	p = Wcm.cm_left, c = Wcm.cc_left, deltax = -deltax; 
 
dodelta:  
    if (c == BIG) {		/* caint get thar from here */ 
	if (doit) 
	    printf ("OOPS"); 
	return c; 
    } 
    totalcost += c * deltax; 
    if (doit) 
	while (--deltax >= 0) 
	    tputs (p, 1, put); 
done:  
    return totalcost; 
} 
 
losecursor () 
{ 
  curY = -1; 
} 
 
#define	USEREL	0 
#define	USEHOME	1 
#define	USELL	2 
#define	USECR	3 
 
static 
xgoto (row, col) 
{ 
    int     homecost, 
            crcost, 
            llcost, 
            relcost, 
            directcost; 
    int     use; 
    char   *p, 
           *dcm; 
 
  /* First the degenerate case */ 
  if (row == curY && col == curX)/* already there */ 
    return; 
 
  if (curY >= 0 && curX >= 0) 
    { 
      /*  
       * Pick least-cost motions 
       */ 
 
      relcost = calccost (curY, curX, row, col, 0); 
      use = USEREL; 
      if ((homecost = Wcm.cc_home) < BIG) 
	  homecost += calccost (0, 0, row, col, 0); 
      if (homecost < relcost) 
	  relcost = homecost, use = USEHOME; 
      if ((llcost = Wcm.cc_ll) < BIG) 
	  llcost += calccost (Wcm.cm_rows - 1, 0, row, col, 0); 
      if (llcost < relcost) 
	  relcost = llcost, use = USELL; 
      if ((crcost = Wcm.cc_cr) < BIG) { 
	  if (Wcm.cm_autolf) 
	      if (curY + 1 >= Wcm.cm_rows) 
		  crcost = BIG; 
	      else 
		  crcost += calccost (curY + 1, 0, row, col, 0); 
	  else 
	      crcost += calccost (curY, 0, row, col, 0); 
      } 
      if (crcost < relcost) 
	  relcost = crcost, use = USECR; 
      directcost = Wcm.cc_abs, dcm = Wcm.cm_abs; 
      if (row == curY && Wcm.cc_habs < BIG) 
	  directcost = Wcm.cc_habs, dcm = Wcm.cm_habs; 
      else if (col == curX && Wcm.cc_vabs < BIG) 
	  directcost = Wcm.cc_vabs, dcm = Wcm.cm_vabs; 
    } 
  else 
    { 
      directcost = 0, relcost = 100000; 
    } 
 
  /*  
   * In the following comparison, the = in <= is because when the costs 
   * are the same, it looks nicer (I think) to move directly there. 
   */ 
  if (directcost <= relcost) 
    { 
      /* compute REAL direct cost */ 
      cost = 0; 
      p = dcm == Wcm.cm_habs ? mytgoto (dcm, row, col) : 
			       mytgoto (dcm, col, row); 
      tputs (p, 1, cmevalcost); 
      if (cost <= relcost) 
	{	/* really is cheaper */ 
	  tputs (p, 1, cmput); 
	  curY = row, curX = col; 
	  return; 
	} 
    } 
 
  switch (use) 
    { 
    case USEHOME:  
      tputs (Wcm.cm_home, 1, put); 
      curY = 0, curX = 0; 
      break; 
 
    case USELL:  
      tputs (Wcm.cm_ll, 1, put); 
      curY = Wcm.cm_rows - 1, curX = 0; 
      break; 
 
    case USECR:  
      tputs (Wcm.cm_cr, 1, put); 
      if (Wcm.cm_autolf) 
	curY++; 
      curX = 0; 
      break; 
    } 
 
  (void) calccost (curY, curX, row, col, 1); 
  curY = row, curX = col; 
} 
 
/* Clear out all terminal info. 
   Used before copying into it the info on the actual terminal. 
 */ 
 
Wcm_clear () 
{ 
  bzero (&Wcm, sizeof Wcm); 
} 
 
/* 
 * Initialized stuff 
 * Return 0 if can do CM. 
 */ 
 
Wcm_init () 
{ 
    Wcm.cx_put = put; 
    Wcm.cx_costinit = costinit; 
    Wcm.cx_goto = xgoto; 
 
    /* Check that we know the size of the screen.... */ 
    if (Wcm.cm_rows <= 0 || Wcm.cm_cols <= 0) 
	return - 1; 
    if (Wcm.cm_abs && !Wcm.cm_ds) 
	return 0; 
    /* Require up and left, and, if no absolute, down and right */ 
    if (!Wcm.cm_up || !Wcm.cm_left) 
	return - 1; 
    if (!Wcm.cm_abs && (!Wcm.cm_down || !Wcm.cm_right)) 
	return - 1; 
    return 0; 
} 
 
/* 
 * This is a lot like the standard tgoto routine except that it knows: 
 *	%m - for NIH 7000 terminals.  (We had someone using one.) 
 *	Magic cm stuff for faking a NUL (and ^D...) 
 */ 
 
static char * 
mytgoto (CM, col, line) 
char	*CM; 
int	col, line; 
{ 
    static char cmbuf[100], 
                add[100]; 
    register char  *cp, 
                   *op; 
    register int    c; 
    int     val, 
            toggle; 
 
    if ((cp = CM) == 0) 
	return 0; 
    op = cmbuf; 
    val = line; 
    toggle = 0; 
    *add = 0; 
    *op = 0; 
    while (c = *cp++) { 
	if (c != '%') { 
	    *op++ = c; 
	    continue; 
	} 
	switch (c = *cp++) { 
	    case 0:  
		cp--; 
		break; 
	    case 'm':  
		col ^= 0177; 
		line ^= 0177; 
		goto setval; 
	    case 'n':  
		col ^= 0140; 
		line ^= 0140; 
		goto setval; 
	    case 'd':  
		if (val < 10) 
		    goto onedigit; 
		if (val < 100) 
		    goto twodigit; 
	    case '3':  
		*op++ = (val / 100) + '0'; 
		val %= 100; 
	    case '2':  
twodigit:  
		*op++ = (val / 10) + '0'; 
onedigit:  
		*op++ = (val % 10) + '0'; 
swap:  
		toggle = 1 - toggle; 
setval:  
		val = toggle ? col : line; 
		continue; 
	    case '>':  
		if (val > *cp++) 
		    val += *cp++; 
		else 
		    cp++; 
		continue; 
	    case '+':  
		val += *cp++; 
	    case '.':  
	use:  
		if (Wcm.cm_ds) { 
		    while (val == 0 || index (Wcm.cm_ds, val)) { 
			strcat (add, toggle ? Wcm.cm_left 
				: Wcm.cm_up); 
			++val; 
		    } 
		} 
		if (val == 0) { 
		    *op++ = 4; 
		    *op++ = 9;	/* 4 and 9 are "magic" */ 
		} 
		else if (val == 4) { 
		    *op++ = 4; 
		    *op++ = 4;	/* another magic combo */ 
		} 
		else 
		    *op++ = val; 
		goto swap; 
	    case 'r':  
		toggle = 1; 
		goto setval; 
	    case 'i':  
		++col, ++line, ++val; 
		continue; 
	    case '%':  
		*op++ = c; 
		continue; 
	    case 'B':  
		val = ((val / 10) << 4) + val % 10; 
		continue; 
	    case 'D':  
		val = val - 2 * (val % 16); 
		continue; 
	    default:  
		val += c; 
		goto use; 
	} 
    } 
    cp = add; 
    while (*cp) 
	*op++ = *cp++; 
    *op = 0; 
    return cmbuf; 
} 
