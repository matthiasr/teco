/* TECO for Ultrix   Copyright 1986 Matt Fichtenbaum                        */
/* This program and its components belong to GenRad Inc, Concord MA 01742   */
/* They may be copied if this copyright notice is included                  */

/* this one modified for higher speed in movenchars() */

/* te_subs.c subroutines  6/2/87 */
/* version for multiple buffers  04/13/89 15.57                             */
#include "te_defs.h"

/* routines to copy a string of characters      */
/* movenchars(from, to, n)                      */
/*      from, to are the addresses of qps       */
/*    n is the number of characters to move     */
/* moveuntil(from, to, c, &n, max)              */
/*    c is the match character that ends the move */
/*    n is the returned number of chars moved   */
/* max is the maximum number of chars to move   */

movenchars(from, to, n)
    struct qp *from, *to;       /* address of buffer pointers */
    register int n;             /* number of characters */
    {
    register struct buffcell *fp, *tp;  /* local qp ".p" pointers   */
    register int fc, tc;                /* local qp ".c" subscripts */
    register int nn;                    /* local character count */

    if (n != 0)
	{
	fp = from->p;               /* copy pointers to local registers */
	fc = from->c;
	tp = to->p;
	tc = to->c;

	while (n > 0)
	    {
	    nn = ( n < (CELLSIZE - tc)) ? n : CELLSIZE - tc;        /* find shortest count */
	    if (nn > (CELLSIZE - fc)) nn = CELLSIZE - fc;
	    n -= nn;                                                    /* adjust count */
	    for (; nn > 0; nn--) tp->ch[tc++] = fp->ch[fc++];           /* move one char: inner loop is just this */

	    if (tc > CELLSIZE-1)    /* check current cell done */
		{
		if (!tp->f)     /* is there another following? */
		    {
		    tp->f = get_bcell();    /* no, add one */
		    tp->f->b = tp;
		    }
		tp = tp->f;
		tc = 0;
		}

	    if (fc > CELLSIZE-1)    /* check current cell done */
		{
		if (!fp->f)     /* oops, run out of source */
		    {
		    if (n > 1) ERROR(E_UTC);    /* error if not done */
		    }
		else {
		    fp = fp->f;     /* chain to next cell */
		    fc = 0;
		    }
		}
	    }
	from->p = fp;       /* restore arguments */
	to->p = tp;
	from->c = fc;
	to->c = tc;
	}
    }

moveuntil(from, to, c, n, max, trace)
    struct qp *from, *to;       /* address of buffer pointers   */
    register char c;            /* match char that ends move    */
    int *n;                     /* pointer to returned value    */
    int max;                    /* limit on chars to move       */
    int trace;                  /* echo characters if nonzero   */
    {
    register struct buffcell *fp, *tp;  /* local qpr ".p" pointers  */
    register int fc, tc;                /* local qpr ".c" subscripts */

    fp = from->p;               /* copy pointers to local registers */
    fc = from->c;
    tp = to->p;
    tc = to->c;

    for (*n = 0; fp->ch[fc] != c; (*n)++)   /* until terminating char... */
	{
	if (max-- <= 0) ERROR((msp <= &mstack[0]) ? E_UTC : E_UTM);
	tp->ch[tc++] = fp->ch[fc++];    /* move one char */
	if (trace) type_char(tp->ch[tc-1]);     /* type it out if trace mode */

	if (tc > CELLSIZE-1)    /* check current cell done */
	    {
	    if (!tp->f)     /* is there another following? */
		{
		tp->f = get_bcell();    /* no, add one */
		tp->f->b = tp;
		}
	    tp = tp->f;
	    tc = 0;
	    }

	if (fc > CELLSIZE-1)    /* check current cell done */
	    {
	    if (!fp->f) ERROR(E_UTC);    /* oops, run out of source */
	    else {
		fp = fp->f;     /* chain to next cell */
		fc = 0;
		}
	    }
	}

    from->p = fp;       /* restore arguments */
    to->p = tp;
    from->c = fc;
    to->c = tc;
    }

/* routine to get numeric argument */
int get_value(d)        /* get a value, default is argument */
    int d;
    {
    int v;

    v = (esp->flag1) ? esp->val1 : 
	    (esp->op == OP_SUB) ? -d : d;
    esp->flag1 = 0;     /* consume argument */
    esp->op = OP_START;
    return(v);
    }




/* routine to convert a line count */
/* returns number of chars between dot and nth line feed */

int lines(arg)
    register int arg;
    {
    register int i, c;
    register struct buffcell *p;

    for (i = pbuff->dot / CELLSIZE, p = pbuff->f; (i > 0) && (p->f); i--) p = p->f; /* find dot */
    c = pbuff->dot % CELLSIZE;
    if (arg <= 0)               /* scan backwards */
	{
	for (i = pbuff->dot; (arg < 1) && (i > 0); )        /* repeat for each line */
	    {
	    --i;                /* count characters */
	    if (--c < 0)    /* back up the pointer */
		{
		if (!(p = p->b)) break;
		c = CELLSIZE - 1;
		}
	    if ( (ez_val & EZ_NOVTFF) ? (p->ch[c] == LF) : (spec_chars[p->ch[c]] & A_L) ) ++arg;    /* if line sep found */
	    }
	if (arg > 0) ++i;               /* if terminated on a line separator, advance over the separator */
	}

    else                        /* scan forwards */
	{
	for (i = pbuff->dot; (arg > 0) && (i < pbuff->z); i++)
	    {
	    if ( (ez_val & EZ_NOVTFF) ? (p->ch[c] == LF) : (spec_chars[p->ch[c]] & A_L) ) --arg;
	    if (++c > CELLSIZE-1)
		{
		if (!(p = p->f)) break;
		c = 0;
		}
	    }           /* this will incr over the separator anyway */
	}
    return(i - pbuff->dot);
    }

/* routine to handle args for K, T, X, etc.     */
/* if two args, 'char x' to 'char y'            */
/* if just one arg, then n lines (default 1)    */
/* sets a pointer to the beginning of the specd */
/* string, and a char count value               */

int line_args(d, p)
    int d;                  /* nonzero: leave dot at start */
    struct qp *p;
    {
    int n;

    if (esp->flag1 && esp->flag2)       /* if two args */
	{
	if (esp->val1 <= esp->val2)     /* in right order */
	    {
	    if (esp->val1 < 0) esp->val1 = 0;
	    if (esp->val2 > pbuff->z) esp->val2 = pbuff->z;
	    if (d) pbuff->dot = esp->val1;      /* update dot */
	    set_pointer(esp->val1, p);  /* set the pointer */
	    esp->flag2 = esp->flag1 = 0;    /* consume arguments */
	    esp->op = OP_START;
	    return(esp->val2 - esp->val1);  /* and return the count */
	    }
	else
	    {
	    if (esp->val2 < 0) esp->val2 = 0;
	    if (esp->val1 > pbuff->z) esp->val1 = pbuff->z;
	    if (d) pbuff->dot = esp->val2;      /* update dot */
	    set_pointer(esp->val2, p);          /* args in reverse order */
	    esp->flag2 = esp->flag1 = 0;        /* consume arguments */
	    esp->op = OP_START;
	    return(esp->val1 - esp->val2);
	    }
	}
    else
	{
	n = lines(get_value(1));
	if (n < -pbuff->dot) n = -pbuff->dot;
	else if (n > pbuff->z - pbuff->dot) n = pbuff->z - pbuff->dot;
	if (n >= 0) set_pointer(pbuff->dot, p);
	else
	    {
	    n = -n;
	    set_pointer(pbuff->dot - n, p);
	    if (d) pbuff->dot -= n;
	    }
	return(n);
	}
    }

/* convert character c to a q-register spec */
int getqspec(fors, c)   /* fors ("file or search") nonzero = allow _ or * */
    int fors;
    char c;
    {
    if (isdigit(c)) return(c - '0' + 1);
    else if (isalpha(c)) return(mapch_l[c] - 'a' + 11);
    else if (fors)
	{
	if (c == '_') return (SERBUF);
	if (c == '*') return (FILBUF);
	if (c == '%') return (SYSBUF);
	if (c == '#') return (TIMBUF);
	}
    ERROR(E_IQN);
    }



/* routines to do insert operations */
/* insert1() copies current cell up to dot into a new cell */
/* leaves bb pointing to end of that text */
/* insert2() copies rest of buffer */

struct buffcell *insert_p;

insert1()
    {
    int nchars;             /* number of chars in cell */

    set_pointer(pbuff->dot, &aa);   /* convert dot to a qp */
    if (pbuff->dot < pbuff->buff_mod) pbuff->buff_mod = pbuff->dot;     /* update earliest char loc touched */
    insert_p = bb.p = get_bcell();      /* get a new cell */
    bb.c = 0;
    nchars = aa.c;          /* save char position of dot in cell */
    aa.c = 0;

/* now aa points to the beginning of the buffer cell that */
/* contains dot, bb points to the beginning of a new cell,*/
/* nchars is the number of chars before dot */

    movenchars(&aa, &bb, nchars);   /* copy cell up to dot */
    }



insert2(count)              /* count is the number of chars added */
    int count;
    {
    aa.p->b->f = insert_p;      /* put the new cell where the old one was */
    insert_p->b = aa.p->b;
    insert_p = NULL;

    bb.p->f = aa.p;         /* splice rest of buffer to end */
    aa.p->b = bb.p;
    movenchars(&aa, &bb, pbuff->z - pbuff->dot);    /* squeeze buffer */
    free_blist(bb.p->f);    /* return unused cells */
    bb.p->f = NULL;         /* and end the buffer */
    pbuff->z += count;              /* add # of chars inserted */
    pbuff->dot += count;
    ctrl_s = -count;        /* save string length */
    }

/* subroutine to delete n characters starting at dot    */
/* argument is number of characters                     */

delete1(nchars)
    int nchars;
    {
    if (!nchars) return;        /* 0 chars is a nop */
    if (nchars < 0)     /* delete negative number of characters? */
	{
	nchars = -nchars;           /* make ll positive */
	if (nchars > pbuff->dot) ERROR(E_POP);      /* don't delete beyond beg of buffer */
	pbuff->dot -= nchars;               /* put pointer before deleted text */
	}
    else if (pbuff->dot + nchars > pbuff->z) ERROR(E_POP);  /* don't delete beyond end of buffer */

    set_pointer(pbuff->dot, &aa);           /* pointer to beginning of area to delete */
    set_pointer(pbuff->dot+nchars, &bb);    /* and to end */
    if (pbuff->dot < pbuff->buff_mod) pbuff->buff_mod = pbuff->dot;     /* update earliest char loc touched */
    movenchars(&bb, &aa, pbuff->z - (pbuff->dot+ nchars));  /* move text unless delete ends at z */
    free_blist(aa.p->f);            /* return any cells after end */
    aa.p->f = NULL;                 /* end the buffer */
    pbuff->z -= nchars;             /* adjust z */
    }

/* routine to process "O" command */

struct qh obuff;        /* tag string buffer */

do_o()
    {
    int i, j;           /* i used as start of tag, j as end */
    int p, level;       /* p is pointer to tag string, level is iteration lvl */
    int epfound;        /* flag for "second ! found"        */

    if (!build_string(&obuff)) return;      /* no tag spec'd: continue */
    if (obuff.z > CELLSIZE) ERROR(E_STL);   /* string too long */
    esp->op = OP_START;                     /* consume any argument */
    if (esp->flag1)                         /* is there one? */
	{
	esp->flag1 = 0;                     /* consume it */
	if (esp->val1 < 0) return;          /* computed goto out of range - */
	for (i = 0; (i < obuff.z) && (esp->val1 > 0); i++)      /* scan to find right tag */
	    if (obuff.f->ch[i] == ',') esp->val1--;             /* count commas */
	if (esp->val1 > 0) return;          /* computed goto out of range + */

/* now i is either at 0 or after the nth comma */

	for (j = i; j < obuff.z; j++)   /* find end of tag */
	    if (obuff.f->ch[j] == ',') break;       /* stop at next comma */
	if (j == i) return;             /* two adjacent commas: zero length tag */
	}

    else
	{
	i = 0;              /* not a computed goto: use whole tag buffer */
	j = obuff.z;
	}

/* start from beginning of iteration or macro, and look for tag */

    if (cptr.flag & F_ITER)         /* if in iteration */
	{
	cptr.p = cptr.il->p;        /* restore */
	cptr.c = cptr.il->c;
	cptr.dot = cptr.il->dot;
	}
    else for (cptr.dot = cptr.c = 0; cptr.p->b->b != NULL; cptr.p = cptr.p->b); /* find macro start */

/* search for tag */

    for (level = 0; ;)          /* look through rest of command string */
	{
	switch (skipto(1))      /* look for interesting things, including ! */
	    {
	    case '<':           /* start of iteration */
		++level;
		break;

	    case '>':           /* end of iteration */
		if ((level == 0) && (cptr.flag & F_ITER)) pop_iteration(1);
		else --level;
		break;

	    case '!':               /* start of tag */
		for (epfound = 0; ; epfound = 0)        /* keep looking for tag */
		    {
		    for (p = i; p < j; p++)
			{
			if (getcmdc(0) == '!') epfound = 1;     /* mark "trailing ! found */
			if (mapch_l[cmdc] != mapch_l[obuff.f->ch[p]]) break;    /* compare */
			}
		    if (p >= j)         /* if all comparison chars matched */
			{
			if (getcmdc(0) == '!') return;  /* and tag ends with !, found it */
			}
		    else if (!epfound) while (getcmdc(0) != '!');       /* else look for next ! and continue */
		    }
		break;
	    }           /* end of switch */
	}           /* end of scan loop */
    }           /* end of subroutine */

/* routine to skip to next ", ', |, <, or >         */
/* skips over these chars embedded in text strings  */
/* stops in ! if argument is nonzero                */
/* returns character found, and leaves it in skipc  */

char skipto(arg)
    int arg;
    {
    int atsw;               /* "at" prefix */
    char ta, term;          /* temp attributes, terminator */

    for (atsw = 0; ;)       /* forever      */
	{
	while (!(ta = spec_chars[skipc = getcmdc(0)] & (A_X | A_S | A_T | A_Q)));   /* read until something interesting found */

    again:
	if (ta & A_Q) getcmdc(0);       /* if command takes a Q spec, skip the spec */
	if (ta & A_X)                   /* sought char found: quit */
	    {
	    if (skipc == '"') getcmdc(0);   /* quote must skip next char */
	    return(skipc);
	    }
	if (ta & A_S)                               /* other special char */
	    {
	    switch (skipc)
		{
		case '^':                           /* treat next char as CTL */
		    if (ta = spec_chars[skipc = getcmdc(0) & 0x1f]) goto again;
		    break;

		case '@':                           /* use alternative text terminator */
		    atsw = 1;
		    break;

		case CTL('^'):                       /* ^^ is value of next char: skip that char */
		    getcmdc(0);
		    break;

		case CTL('A'):                       /* type text */
		    term = (atsw) ? getcmdc(0) : CTL('A');
		    atsw = 0;
		    while (getcmdc(0) != term);     /* skip text */
		    break;

		case '!':                           /* tag */
		    if (arg) return(skipc);
		    while (getcmdc(0) != '!');      /* skip until next ! */
		    break;

		case 'e':                           /* first char of two-letter E or F command */
		case 'f':
		    if (spec_chars[getcmdc(0)] & ((skipc == 'e') ? A_E : A_F))      /* if one with a text arg */
			{
			term = (atsw) ? getcmdc(0) : ESC;
			atsw = 0;
			while (getcmdc(0) != term);     /* read past terminator */
			}
		    break;
		}               /* end "switch" */
	    }               /* end "if (ta & A_S)" */

	else if (ta & A_T)                      /* command with a text argument */
	    {
	    term = (atsw) ? getcmdc(0) : ESC;
	    atsw = 0;
	    while (getcmdc(0) != term);         /* skip text */
	    }
	}               /* end "forever" */
    }               /* end "skipto()" */

/* find number of characters to next matching (, [, or {  (like '%' in vi) */

do_ctlp()
    {
    int i, l;
    char c, c1;

    set_pointer(pbuff->dot, &aa);           /* point to text buffer */
    switch(c1 = aa.p->ch[aa.c])
	{
	case '(':
	    c = ')';            /* match char is ) */
	    i = 1;              /* direction is positive */
	    break;

	case ')':
	    c = '(';            /* match char is ( */
	    i = -1;             /* direction is negative */
	    break;

	case '[':
	    c = ']';
	    i = 1;
	    break;

	case ']':
	    c = '[';
	    i = -1;
	    break;

	case '{':
	    c = '}';
	    i = 1;
	    break;

	case '}':
	    c = '{';
	    i = -1;
	    break;

	case '<':
	    c = '>';
	    i = 1;
	    break;

	case '>':
	    c = '<';
	    i = -1;
	    break;

	case '"':
	    c = '\'';
	    i = 1;
	    break;

	case '|':
	    c1 = '\'';          /* treat | as ' */
	case '\'':
	    c = '"';
	    i = -1;
	    break;

	default:
	    esp->val1 = i = 0;      /* not on a matchable char, return 0 */
	}

    l = 1;          /* start with one unmatched char */
    if (i > 0)      /* if searching forward */
	{
	for (i = pbuff->dot, fwdc(&aa); (i < pbuff->z) && (l); fwdc(&aa) )
	    {
	    ++i;
	    if (aa.p->ch[aa.c] == c) --l;
	    else if (aa.p->ch[aa.c] == c1) ++l;
	    }
	esp->val1 = (i < pbuff->z) ? i - pbuff->dot : 0;
	}
    else if (i < 0)
	{
	for (i = pbuff->dot, backc(&aa); (i >= 0) && (l); backc(&aa) )
	    {
	    --i;
	    if (aa.p->ch[aa.c] == c) --l;
	    else if (aa.p->ch[aa.c] == c1) ++l;
	    }
	esp->val1 = (i >= 0) ? i - pbuff->dot : 0;
	}
    esp->flag1 = 1;
    }
