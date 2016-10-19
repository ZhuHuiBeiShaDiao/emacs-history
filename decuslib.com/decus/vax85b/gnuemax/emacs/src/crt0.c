/* C code startup routine for Vax 4.2 BSD ONLY! 
   Copyright (C) 1985 Richard M. Stallman 
 
This program is distributed in the hope that it will be useful, 
but without any warranty.  No author or distributor 
accepts responsibility to anyone for the consequences of using it 
or for whether it serves any particular purpose or works at all, 
unless he says so in writing. 
 
   Permission is granted to anyone to distribute verbatim copies 
   of this program's source code as received, in any medium, provided that 
   the copyright notice, the nonwarraty notice above 
   and this permission notice are preserved, 
   and that the distributor grants the recipient all rights 
   for further redistribution as permitted by this notice, 
   and informs him of these rights. 
 
   Permission is granted to distribute modified versions of this 
   program's source code, or of portions of it, under the above 
   conditions, plus the conditions that all changed files carry 
   prominent notices stating who last changed them and that the 
   derived material, including anything packaged together with it and 
   conceptually functioning as a modification of it rather than an 
   application of it, is in its entirety subject to a permission 
   notice identical to this one. 
 
   Permission is granted to distribute this program (verbatim or 
   as modified) in compiled or executable form, provided verbatim 
   redistribution is permitted as stated above for source code, and 
    A.  it is accompanied by the corresponding machine-readable 
      source code, under the above conditions, or 
    B.  it is accompanied by a written offer, with no time limit, 
      to distribute the corresponding machine-readable source code, 
      under the above conditions, to any one, in return for reimbursement 
      of the cost of distribution.   Verbatim redistribution of the 
      written offer must be permitted.  Or, 
    C.  it is distributed by someone who received only the 
      compiled or executable form, and is accompanied by a copy of the 
      written offer of source code which he received along with it. 
 
   Permission is granted to distribute this program (verbatim or as modified) 
   in executable form as part of a larger system provided that the source 
   code for this program, including any modifications used, 
   is also distributed or offered as stated in the preceding paragraph. 
 
In other words, you are welcome to use, share and improve this program. 
You are forbidden to forbid anyone else to use, share and improve 
what you give them.   Help stamp out software-hoarding!  */ 
 
 
/* It is carefully set up to use only the SP register's initial contents 
  since I find that AP and FP contain mysterious data at fork start time. 
 
Also, it allocates no initialized data. 
 
Data format on startup: 
  sp ->  word containing argc 
         word pointing to first arg string 
	 [word pointing to next arg string]... 0 or more times 
	 0 
Optionally: 
	 [word pointing to environment variable]... 1 or more times 
	 ... 
	 0 
And always: 
	 first arg string 
	 [next arg string]... 0 or more times 
*/ 
 
 
char **environ; 
 
static start () 
{ 
  start1 (); 
} 
 
static start1 (argc, xargv) 
     int argc; 
     char *xargv; 
{ 
  register char **argv = &xargv; 
  environ = argv + argc + 1; 
 
  if ((char *)environ == xargv) 
    environ--; 
  exit (main (argc, argv, environ)); 
} 
