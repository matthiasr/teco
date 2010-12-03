/* TECO for Ultrix   Copyright 1986 Matt Fichtenbaum                        */
/* This program and its components belong to GenRad Inc, Concord MA 01742   */
/* They may be copied if this copyright notice is included                  */

/* te_srch.c   routines associated with search operations   2/5/86 */
/* version for multiple buffers  04/13/89 15.56                             */
#include "te_defs.h"

/* routine to read in a string with string-build characters */
/* used for search, tag, file name operations               */
/* returns 0 if empty string entered, nonzero otherwise     */

int build_string(sbuff)
    struct qh *sbuff;           /* arg is addr of q-reg header */
    {
    int count;                  /* char count */
    struct buffcell *tp;        /* pointer to temporary string */
    char c;                     /* temp character */

    term_char = (atflag) ? getcmdc(trace_sw) : ESC;     /* read terminator */
    count = atflag = 0;         /* initialize char count */
    if (!peekcmdc(term_char))   /* if string is not empty */
	{

/* create a temporary string and read chars into it until the terminator */
	for (tp = bb.p = get_bcell(), bb.c = 0; (c = getcmdc(trace_sw)) != term_char; )
	    {
	    if ((c == '^') && !(ed_val & ED_CARET))     /* read next char as CTL */
		{
		if ((c = getcmdc(trace_sw)) == term_char) ERROR(msp <= &mstack[0] ? E_UTC : E_UTM);
		c &= 0x1f;
		}
	    if ((c &= 0177) < ' ')          /* if a control char */
		{
		switch (c)
		    {
		    case CTL('Q'):           /* take next char literally */
		    case CTL('R'):
			if ((c = getcmdc(trace_sw)) == term_char) ERROR((msp <= &mstack[0]) ? E_UTC : E_UTM);
			break;              /* fetch character and go store */

		    case CTL('V'):           /* take next char as lower case */
			if (getcmdc(trace_sw) == term_char) ERROR((msp <= &mstack[0]) ? E_UTC : E_UTM);
			c = mapch_l[cmdc];
			break;

		    case CTL('W'):           /* take next char as upper case */
			if ((c = getcmdc(trace_sw)) == term_char) ERROR((msp <= &mstack[0]) ? E_UTC : E_UTM);
			if (islower(c)) c = toupper(c);
			break;

		    case CTL('E'):           /* expanded constructs */
			if (getcmdc(trace_sw) == term_char) ERROR((msp <= &mstack[0]) ? E_UTC : E_UTM);
			switch (mapch_l[cmdc])
			    {
			    case 'u':       /* use char in q-reg */
				if (getcmdc(trace_sw) == term_char) ERROR((msp <= &mstack[0]) ? E_UTC : E_UTM);
				c = qreg[getqspec(1, cmdc)].v & 0x7f;
				break;

			    case 'q':       /* use string in q-reg */
				if (getcmdc(trace_sw) == term_char) ERROR((msp <= &mstack[0]) ? E_UTC : E_UTM);
				ll = getqspec(1, cmdc);         /* read the reg spec */
				aa.p = qreg[ll].f;              /* set a pointer to it */
				aa.c = 0;
				for (mm = 0; mm < qreg[ll].z; mm++)
				    {
				    bb.p->ch[bb.c] = aa.p->ch[aa.c];        /* store char */
				    fwdcx(&bb);                 /* store next char */
				    fwdc(&aa);
				    ++count;
				    }
				continue;               /* repeat loop without storing */

			    default:
				bb.p->ch[bb.c] = CTL('E');       /* not special: store the ^E */
				fwdcx(&bb);
				++count;
				c = cmdc;                       /* and go store the following char */
				break;

			    }                           /* end ^E switch */
			}                           /* end outer switch */
		}                           /* end "if a control char */
	    bb.p->ch[bb.c] = c;         /* store character */
	    fwdcx(&bb);                 /* advance pointer */
	    ++count;                    /* count characters */
	    }                       /* end "for" loop */
	free_blist(sbuff->f);       /* return old buffer */
	sbuff->f = tp;              /* put in new one */
	sbuff->f->b = (struct buffcell *) sbuff;
	sbuff->z = count;           /* store count of chars in string */
	}                   /* end non-null string */
    else getcmdc(trace_sw);     /* empty string: consume terminator */
    return(count);          /* return char count */
    }           




/* routine to handle end of a search operation  */
/* called with pass/fail result from search     */
/* returns same pass/fail result                */

int end_search(result)
    int result;
    {
    if (!result)        /* if search failed */
	{
	if (!(esp->flag2 || (ed_val & ED_SFAIL))) pbuff->dot = 0;       /* if an unbounded search failed, clear ptr */
	if (!colonflag && !peekcmdc(';')) ERROR(E_SRH);       /* if no real or implied colon, error if failure */
	}
    esp->flag1 = colonflag;             /* return a value if a :S command */
    srch_result = esp->val1 = result;   /* and leave it for next ";" */
    esp->flag2 = colonflag = atflag = 0;    /* consume arguments */
    esp->op = OP_START;
    return(result);
    }

/* routine to set up for search operation */
/* reads search arguments, returns search count */

static struct qp sm, sb;        /* match-string and buffer pointers */
static char *pmap;              /* pointer to character mapping table */
static int locb;                /* reverse search limit */
static int last_z;              /* end point for reverse search */

int setup_search()
    {
    int count;              /* string occurrence counter */

    set_pointer(pbuff->dot, &aa);       /* set a pointer to start of search */
    if (colonflag >= 2) esp->flag2 = esp->flag1 = esp->val2 = esp->val1 = 1;    /* ::S is 1,1S */
    if ((count = get_value(1)) == 0) ERROR(E_ISA);   /* read search count: default is 1 */
    else if (count > 0)             /* search forward */
	{
	if (esp->flag2)     /* if bounded search */
	    {
	    if (esp->val2 < 0) esp->val2 = -(esp->val2);    /* set limit */
	    if ((aa.z = pbuff->dot + esp->val2) > pbuff->z) aa.z = pbuff->z;        /* or z, whichever less */
	    }
	else aa.z = pbuff->z;
	}
    else
	{
	if (esp->flag2)     /* if bounded search */
	    {
	    if (esp->val2 < 0) esp->val2 = -(esp->val2);    /* set limit */
	    if ((locb = pbuff->dot - esp->val2) < 0) locb = 0;      /* or 0, whichever greater */
	    }
	else locb = 0;
	}
    return(count);
    }

/* routine to do N, _, E_ searches:  search, if search fails, then get  */
/* next page and continue                                               */

do_nsearch(arg)
    char arg;       /* arg is 'n', '_', or 'e' to define which search */
    {
    int scount;     /* search count */

    build_string(&sbuf);            /* read the search string */
    if ((scount = get_value(1)) <= 0) ERROR(E_ISA);     /* count must be >0 */
    set_pointer(pbuff->dot, &aa);   /* start search at dot */
    esp->flag2 = locb = 0;          /* make it unbounded */

    while (scount > 0)              /* search until found */
	{
	if (!do_search(1))          /* search forwards */
	    {                       /*   if search fails... */
	    if (infile->eofsw || !infile->fd) break;    /* if no input, quit */
	    if (arg == 'n')
		{
		set_pointer(0, &aa);    /* write file if 'n' */
		write_file(&aa, pbuff->z, ctrl_e);
		}

	    /* not 'n': if _, and an output file, and data to lose, error */
	    else if ((arg == '_') && (outfile->fd) && (pbuff->z) && (ed_val & ED_YPROT)) ERROR(E_YCA);

	    pbuff->buff_mod = pbuff->dot = pbuff->z = 0;                        /* clear buffer */
	    set_pointer(0, &aa);
	    read_file(&aa, &pbuff->z, (ed_val & ED_EXPMEM ? -1 : 0) );          /* read next page */
	    set_pointer(0, &aa);        /* search next page from beginning */
	    }
	else --scount;                  /* search successful: one fewer to look for */
	}
    return( end_search( (scount == 0) ? -1 : 0) );      /* use end_search to clean up */
    }


/* routine to do "FB" search - m,nFB is search from m to n, */
/* nFB is search from . to nth line                         */
/* convert arguments to args of normal m,nS command         */

int do_fb()             /* returns search result */
    {
    if (esp->flag1 && esp->flag2)   /* if two arguments */
	{
	pbuff->dot = esp->val2;                 /* start from "m" arg */
	esp->val2 = esp->val1 - esp->val2;      /* get number of chars */
	}
    else                            /* if no or one args, treat as number of lines */
	{
	esp->val2 = lines(get_value(1));        /* number of chars */
	esp->flag2 = esp->flag1 = 1;            /* conjure up two args */
	}
    esp->val1 = (esp->val2 > 0) ? 1 : -1;   /* set search direction */

    build_string(&sbuf);        /* read search string and terminator */
    return(end_search(  do_search( setup_search() )  ));    /* do search and return result */
    }

/* routine to do search operation: called with search count as argument */
/* returns -1 (pass) or 0 (fail)                                        */

int do_search(count)
    int count;
    {
    pmap = (ctrl_x) ? &mapch[0] : &mapch_l[0];      /* set approp. mapping table */
    sm.z = sbuf.z;                  /* copy # of chars in search buffer */

    if (count > 0)
	{
	for (sm.dot = 0; count > 0; count--)     /* loop to count occurrences */
	    {
	    for (; aa.dot < aa.z; aa.dot++)  /* loop to advance search pointer */
		{
		for (sb.p = aa.p, sb.c = aa.c, sb.dot = aa.dot, sm.p = sbuf.f, sm.dot = sm.c = 0;
					    (sb.dot < pbuff->z) && (sm.dot < sm.z); sm.dot++, sb.dot++)
		    {                               /* for each char in search string */
		    if (spec_chars[ sm.p->ch[sm.c] ] & A_A)     /* if search string char is "special" */
			{
			if (!srch_cmp()) break;         /* then use expanded comparison routine */
			}
		    else if (*(pmap + sb.p->ch[sb.c]) != *(pmap + sm.p->ch[sm.c])) break;       /* else just compare */
		    if (++sm.c > CELLSIZE-1)        /* advance search-string ptr */
			{
			sm.p = sm.p->f;
			sm.c = 0;
			}
		    if (++sb.c > CELLSIZE-1)        /* advance buffer ptr */
			{
			sb.p = sb.p->f;
			sb.c = 0;
			}
		    }                   /* end comparison loop */

		if (sm.dot >= sm.z) break;          /* exit if found */
		if (++aa.c > CELLSIZE-1)            /* else not found: advance buffer pointer */
		    {
		    aa.p = aa.p->f;
		    aa.c = 0;
		    }
		}                       /* end search loop */

	    if (sm.dot < sm.z) break;               /* if one search failed, don't do more */
	    else
		{
		ctrl_s = aa.dot - sb.dot;           /* otherwise save -length of string found */
		if ((ed_val & ED_SMULT) && (count > 1))     /* if funny "advance by 1" mode */
		    {
		    ++aa.dot;                       /* advance buffer pointer by one only */
		    if (++aa.c > CELLSIZE-1)
			{
			aa.p = aa.p->f;
			aa.c = 0;
			}
		    }

		else
		    {
		    aa.dot = sb.dot;                /* advance search pointer past string */
		    aa.p = sb.p;
		    aa.c = sb.c;
		    }
		}
	    }                   /* end "search n times" */
	}               /* end "search forwards" */

    else                /* search backwards */
	{
	for (last_z = pbuff->z, sm.dot = 0; count < 0; count++)  /* loop to count occurrences */
	    {
	    for (; aa.dot >= locb; aa.dot--)     /* loop to advance (backwards) search pointer */
		{
		for (sb.p = aa.p, sb.c = aa.c, sb.dot = aa.dot, sm.p = sbuf.f, sm.dot = sm.c = 0;
						(sb.dot < last_z) && (sm.dot < sm.z); sm.dot++, sb.dot++)
		    {                               /* loop to compare string */
		    if (spec_chars[ sm.p->ch[sm.c] ] & A_A)     /* if search string char is "special" */
			{
			if (!srch_cmp()) break;     /* then use expanded comparison routine */
			}
		    else if (*(pmap + sb.p->ch[sb.c]) != *(pmap + sm.p->ch[sm.c])) break;       /* else just compare */
		    if (++sm.c > CELLSIZE-1)        /* advance search-string ptr */
			{
			sm.p = sm.p->f;
			sm.c = 0;
			}
		    if (++sb.c > CELLSIZE-1)        /* advance buffer ptr */
			{
			sb.p = sb.p->f;
			sb.c = 0;
			}
		    }                   /* end comparison loop */
		if (sm.dot >= sm.z)                 /* search matches: */
		    {
		    if (!(ed_val & ED_SMULT)) last_z = aa.dot;  /* set last_z to point where this string was found */
		    break;
		    }
		if (sb.dot >= last_z)               /* or if string is beyond end of buffer */
		    {
		    sm.dot = sm.z;                      /* make search appear to have succeeded */
		    --count;                            /* so as to back up pointer, and force one more look */
		    break;
		    }
		if (--aa.c < 0)             /* else advance buffer pointer (backwards) */
		    {
		    aa.p = aa.p->b;
		    aa.c = CELLSIZE-1;
		    }
		}                       /* end search loop */
	    if (sm.dot < sm.z) break;               /* if one search failed, don't do more */
	    else
		{
		if (count < -1) backc(&aa);         /* if this is not last search, back pointer up one */
		else                    
		    {
		    ctrl_s = aa.dot - sb.dot;       /* otherwise save -length of string found */
		    aa.dot = sb.dot;                /* advance pointer past string */
		    aa.p = sb.p;
		    aa.c = sb.c;
		    }
		}
	    }                   /* end "search n times" */
	}               /* end "search backwards" */
    if (sm.dot >= sm.z) pbuff->dot = aa.dot;        /* if search succeeded, update pointer  */
    search_flag = 1;                                /* set "search occurred" (for ES)       */
    return((sm.dot >= sm.z) ? -1 : 0);              /* and return -1 (pass) or 0 (fail)     */
    }               /* end "do_search" */

/* expanded search comparison */
/* returns 1 if match, 0 if not */

int srch_cmp()
    {
    int tq;                     /* q-reg name for ^EGq */
    struct qp tqp;              /* pointer to read q reg */

    switch (mapch_l[sm.p->ch[sm.c]])        /* what is search character */
	{
	case CTL('N'):               /* match anything but following construct */
	    if (sm.dot >= sm.z) ERROR(E_ISS);   /* don't read past end of string */
	    fwdc(&sm);              /* skip the ^N */
	    return(!srch_cmp());

	case CTL('X'):               /* match any character */
	    return(1);

	case CTL('Q'):               /* take next char literally */
	case CTL('R'):
	    if (sm.dot >= sm.z) ERROR(E_ISS);   /* don't read past end of string */
	    fwdc(&sm);              /* skip the ^Q */
	    return(*(pmap + sb.p->ch[sb.c]) == *(pmap + sm.p->ch[sm.c]));

	case CTL('S'):               /* match any nonalphanumeric */
	    return(!isalnum(sb.p->ch[sb.c]));

	case CTL('E'):
	    if (sm.dot >= sm.z) ERROR(E_ISS);   /* don't read past end of string */
	    fwdc(&sm);              /* skip the ^E */
	    switch (mapch_l[sm.p->ch[sm.c]])
		{
		case 'a':           /* match any alpha */
		    return(isalpha(sb.p->ch[sb.c]));

		case 'b':           /* match any nonalpha */
		    return(!isalnum(sb.p->ch[sb.c]));

		case 'c':           /* rad50 symbol constituent */
		    return(!isalnum(sb.p->ch[sb.c]) || (sb.p->ch[sb.c] == '$') || (sb.p->ch[sb.c] == '.'));

		case 'd':           /* digit */
		    return(isdigit(sb.p->ch[sb.c]));

		case 'l':           /* line terminator LF, VT, FF */
		    return((sb.p->ch[sb.c] == LF) || (sb.p->ch[sb.c] == FF) || (sb.p->ch[sb.c] == VT));

		case 'r':           /* alphanumeric */
		    return(isalnum(sb.p->ch[sb.c]));

		case 'v':           /* lower case */
		    return(islower(sb.p->ch[sb.c]));

		case 'w':           /* upper case */
		    return(isupper(sb.p->ch[sb.c]));

		case 's':           /* any non-null string of spaces or tabs */
		    if (((sb.p->ch[sb.c]&0177) != ' ') && ((sb.p->ch[sb.c]&0177) != TAB)) return(0);    /* failure */
		    /* skip remaining spaces or tabs */
		    for ( fwdc(&sb); ((sb.p->ch[sb.c]&0177) == ' ') || ((sb.p->ch[sb.c]&0177) == TAB); fwdc(&sb) );
		    backc(&sb);     /* back up one char (calling routine will skip it) */
		    return(1);      /* success */

		case 'g':           /* any char in specified q register */
		    if (sm.dot >= sm.z) ERROR(E_ISS);   /* don't read past end of string */
		    fwdc(&sm);      /* get to the next char */
		    tq = getqspec(1, sm.p->ch[sm.c]);       /* read q-reg spec */                   
		    for (tqp.dot = tqp.c = 0, tqp.p = qreg[tq].f; tqp.dot < qreg[tq].z; fwdc(&tqp))
			if (*(pmap + tqp.p->ch[tqp.c]) == *(pmap + sb.p->ch[sb.c])) return(1);  /* match */
		    return(0);      /* fail */

		default:
		    ERROR(E_ISS);
		}                           /* end ^E constructions */

	default:
	    return(*(pmap + sb.p->ch[sb.c]) == *(pmap + sm.p->ch[sm.c]));
	}                                   /* end other constructions */
    }
