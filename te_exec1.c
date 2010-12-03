/* TECO for Ultrix   Copyright 1986 Matt Fichtenbaum                        */
/* This program and its components belong to GenRad Inc, Concord MA 01742   */
/* They may be copied if this copyright notice is included                  */

/* te_exec1.c   continue executing commands   9/29/87 */
/* version for multiple buffers  04/13/89 15.55                             */
#include "te_defs.h"

exec_cmds1()
    {
    char command;                   /* command character */
    int cond;                       /* conditional in progress */
    int tempz;                      /* temp copy of Z */

    switch (command = mapch_l[cmdc])
	{
/* operators */

	case '+':
	    esp->exp = (esp->flag1) ? esp->val1 : 0;
	    esp->flag1 = 0;
	    esp->op = OP_ADD;
	    break;

	case '-':
	    esp->exp = (esp->flag1) ? esp->val1 : 0;
	    esp->flag1 = 0;
	    esp->op = OP_SUB;
	    break;

	case '*':
	    esp->exp = (esp->flag1) ? esp->val1 : 0;
	    esp->flag1 = 0;
	    esp->op = OP_MULT;
	    break;

	case '/':
	    esp->exp = (esp->flag1) ? esp->val1 : 0;
	    esp->flag1 = 0;
	    esp->op = OP_DIV;
	    break;

	case '&':
	    esp->exp = (esp->flag1) ? esp->val1 : 0;
	    esp->flag1 = 0;
	    esp->op = OP_AND;
	    break;

	case '#':
	    esp->exp = (esp->flag1) ? esp->val1 : 0;
	    esp->flag1 = 0;
	    esp->op = OP_OR;
	    break;

	case ')':
	    if ((!esp->flag1) || (esp <= &estack[0])) ERROR(E_NAP);
	    --esp;
	    esp->val1 = (esp+1)->val1;  /* carry value from inside () */
	    esp->flag1 = 1;
	    break;

	case ',':
	    if (!esp->flag1) ERROR(E_NAC);
	    else esp->val2 = esp->val1;
	    esp->flag2 = 1;
	    esp->flag1 = 0;
	    break;

	case CTL('_'):
	    if (!esp->flag1) ERROR(E_NAB);
	    else esp->val1 = ~esp->val1;
	    break;

/* radix control */

	case CTL('D'):
	    ctrl_r = 10;
	    esp->flag1 = 0;
	    esp->op = OP_START;
	    break;

	case CTL('O'):
	    ctrl_r = 8;
	    esp->flag1 = 0;
	    esp->op = OP_START;
	    break;

	case CTL('R'):
	    if (!esp->flag1)    /* fetch it */
		{
		esp->val1 = ctrl_r;
		esp->flag1 = 1;
		}
	    else
		{
		if ((esp->val1 != 8) && (esp->val1 != 10) && (esp->val1 != 16)) ERROR(E_IRA);
		ctrl_r = esp->val1;
		esp->flag1 = 0;
		esp->op = OP_START;
		}
	    break;

/* other commands */

	case CTL('C'):           /* 1 ^C stops macro execution, 2 exit */
	    if (peekcmdc(CTL('C'))) exitflag = -1;       /* 2 ^C: stop execution and exit */
	    else if (msp <= &mstack[0]) exitflag = 1;   /* 1 ^C: in command string: stop execution */
	    else --msp;                                 /*       in a macro - pop it */
	    break;

	case CTL('X'):           /* search mode flag */
	    set_var(&ctrl_x);
	    break;

	case 'e':
	    do_e();
	    break;

	case 'f':
	    do_f();
	    break;

/* macro call, iteration, conditional */

	case 'm':               /* macro call */
	    mm = getqspec(0, getcmdc(trace_sw));    /* read the macro name */
	    if (msp > &mstack[MSTACKSIZE-1]) ERROR(E_PDO);  /* check room for another level */
	    ++msp;                  /* push stack */
	    cptr.p = qreg[mm].f;    /* to stack entry, put q-reg text start */
	    cptr.flag = cptr.c = cptr.dot = 0;  /* initial char position, iteration flag */
	    cptr.z = qreg[mm].z;    /* number of chars in macro */
	    break;

	case '<':               /* begin iteration */
	    if ((esp->flag1) && (esp->val1 <= 0))       /* if this is not to be executed */
		find_enditer();                         /* just skip the intervening stuff */
	    else
		{
		if (!cptr.il)       /* does this macro have an iteration list? */
		    {
		    cptr.il = (struct is *) get_dcell();    /* no, make one for it */
		    cptr.il->b = NULL;                      /* with NULL reverse pointer */
		    }
		else if (cptr.flag & F_ITER)        /* is there an iteration in process? */
		    {
		    if (!cptr.il->f)                /* yes, if it has no forward pointer */
			{
			cptr.il->f = (struct is *) get_dcell();     /* append a cell to the iteration list */
			cptr.il->f->b = cptr.il;                    /* and link it in */
			}
		    cptr.il = cptr.il->f;                       /* and advance the iteration list pointer to it */
		    }
		cptr.flag |= F_ITER;                /* set iteration flag */
		cptr.il->p = cptr.p;                /* save start of iteration */
		cptr.il->c = cptr.c;
		cptr.il->dot = cptr.dot;
		if (cptr.il->dflag = esp->flag1)    /* if there is an argument, set the "def iter" flag */
		    cptr.il->count = esp->val1;     /* save the count */
		}
	    esp->flag1 = 0;                         /* consume the argument, if any */
	    break;

	case '>':               /* end iteration */
	    if (!(cptr.flag & F_ITER)) ERROR(E_BNI);        /* error if > not in iteration */
	    pop_iteration(0);       /* decrement count and pop conditionally */
	    esp->flag1 = esp->flag2 = 0;    /* consume arguments */
	    esp->op = OP_START;
	    break;

	case ';':               /* semicolon iteration exit */
	    if (!(cptr.flag &F_ITER)) ERROR(E_SNI);     /* error if ; not in iteration */
	    if ( ( ((esp->flag1) ? esp->val1 : srch_result) >= 0) ? (!colonflag) : colonflag)   /* if exit */
		{
		find_enditer();         /* get to end of iteration */
		pop_iteration(1);       /* and pop unconditionally */
		}
	    esp->flag1 = colonflag = 0;     /* consume arg and colon */
	    esp->op = OP_START;
	    break;

/* conditionals */

	case '"':
	    if (!esp->flag1) ERROR(E_NAQ);      /* must be an argument */
	    esp->flag1 = 0;                 /* consume argument */
	    esp->op = OP_START;
	    switch (mapch_l[getcmdc(trace_sw)])
		{
		case 'a':
		    cond = isalpha(esp->val1);
		    break;

		case 'c':
		    cond = isalpha(esp->val1) | (esp->val1 == '$') | (esp->val1 == '.');
		    break;

		case 'd':
		    cond = isdigit(esp->val1);
		    break;

		case 'e':
		case 'f':
		case 'u':
		case '=':
		    cond = !(esp->val1);
		    break;

		case 'g':
		case '>':
		    cond = (esp->val1 > 0);
		    break;

		case 'l':
		case 's':
		case 't':
		case '<':
		    cond = (esp->val1 < 0);
		    break;

		case 'n':
		    cond = esp->val1;
		    break;

		case 'r':
		    cond = isalnum(esp->val1);
		    break;

		case 'v':
		    cond = islower(esp->val1);
		    break;

		case 'w':
		    cond = isupper(esp->val1);
		    break;

		default:
		    ERROR(E_IQC);
		}

	    if (!cond)          /* if this conditional isn't satisfied */
		{
		for (ll = 1; ll > 0;)       /* read to matching | or ' */
		    {
		    while ((skipto(0) != '"') && (skipc != '|') && (skipc != '\''));    /* skip chars */
		    if (skipc == '"')      ++ll;    /* start another level */
		    else if (skipc == '\'') --ll;   /* end a level */
		    else if (ll == 1) break;        /* "else" (|): if on this level, start executing */
		    }
		}
	    break;

	case '\'':              /* end of conditional */
	    break;              /* ignore it if executing */

	case '|':               /* "else" clause */
	    for (ll = 1; ll > 0;)           /* skip to matching ' */
		{
		while ((skipto(0) != '"') && (skipc != '\''));  /* skip chars */
		if (skipc == '"') ++ll;         /* start another level */
		else             --ll;          /* end a level */
		}
	    break;

/* q-register numeric operations */

	case 'u':
	    if (!esp->flag1) ERROR(E_NAU);  /* error if no arg */
	    else qreg[getqspec(0, getcmdc(trace_sw))].v = esp->val1;
	    esp->flag1 = esp->flag2;        /* command's "value" is 2nd arg */
	    esp->val1 = esp->val2;
	    esp->flag2 = 0;                 /* clear 2nd arg */
	    esp->op = OP_START;
	    break;

	case 'q':       /* Qn is numeric val, :Qn is # of chars, mQn is mth char */
	    mm = getqspec((colonflag || esp->flag1), getcmdc(trace_sw));        /* read register name */
	    if (!(esp->flag1))
		{
		esp->val1 = (colonflag) ? qreg[mm].z : qreg[mm].v;
		esp->flag1 = 1;
		}
	    else        /* esp->flag1 is already set */
		{
		if ((esp->val1 >= 0) && (esp->val1 < qreg[mm].z))   /* char subscript within range? */
		    {
		    for (ll = 0, aa.p = qreg[mm].f; ll < (esp->val1 / CELLSIZE); ll++) aa.p = aa.p->f;
		    esp->val1 = (int) aa.p->ch[esp->val1 % CELLSIZE];
		    }
		else esp->val1 = -1;    /* char position out of range */
		esp->op = OP_START;     /* consume argument */
		}
	    colonflag = 0;
	    break;

	case '%':
	    esp->val1 = (qreg[getqspec(0, getcmdc(trace_sw))].v += get_value(1));   /* add to q reg */
	    esp->flag1 = 1;
	    break;

/* move pointer */

	case 'c':
	    if (((tdot = pbuff->dot + get_value(1)) < 0) || (tdot > pbuff->z))
		ERROR(E_POP);   /* add arg to dot, default 1 */
	    else pbuff->dot = tdot;
	    esp->flag2 = 0;
	    break;

	case 'r':
	    if (((tdot = pbuff->dot - get_value(1)) < 0) || (tdot > pbuff->z))
		ERROR(E_POP);   /* add arg to dot, default 1 */
	    else pbuff->dot = tdot;
	    esp->flag2 = 0;
	    break;

	case 'j':
	    if (((tdot = get_value(0)) < 0) || (tdot > pbuff->z))
		ERROR(E_POP);   /* add arg to dot, default 1 */
	    else pbuff->dot = tdot;
	    esp->flag2 = 0;
	    break;

	case 'l':
	    pbuff->dot += lines(get_value(1));
	    break;

/* number of chars until nth line feed */

	case CTL('Q'):
	    esp->val1 = lines(get_value(1));
	    esp->flag1 = 1;
	    break;

/* print numeric value */

	case '=':
	    if (!esp->flag1) ERROR(E_NAE);  /* error if no arg */
	    else
		{
		if (peekcmdc('='))      /* at least one more '=' */
		    {
		    getcmdc(trace_sw);      /* read past it */
		    if (peekcmdc('='))      /* another? */
			{
			getcmdc(trace_sw);          /* yes, read it too */
			printf("%x", esp->val1);    /* print in hex */
			}
		    else printf("%o", esp->val1);   /* print in octal */
		    }
		else printf("%d", esp->val1);
		if (!colonflag) crlf();
		esp->flag1 = esp->flag2 =  colonflag = 0;
		esp->op = OP_START;
		if (!WN_scroll) window(WIN_REDRAW);         /* if not in scroll mode, force full redraw on next refresh */
		}
	    break;

/* insert text */

	case TAB:                   /* insert tab, then text */
	    if (ez_val & EZ_NOTABI) break;      /* tab disabled */
	    if (esp->flag1) ERROR(E_IIA);       /* can't have arg */

	case 'i':                   /* insert text at pointer */
	    term_char = (atflag) ? getcmdc(trace_sw) : ESC;     /* set terminator */
	    if (esp->flag1)     /* if a nI$ command */
		{
		if (!peekcmdc(term_char)) ERROR(E_IIA); /* next char must be term */
		insert1();          /* first part of insert */
		bb.p->ch[bb.c] = esp->val1 & 0177;      /* insert character */
		fwdcx(&bb);         /* advance pointer and extend buffer if necessary */
		ins_count = 1;      /* save string length */
		esp->op = OP_START; /* consume argument */
		}
	    else                    /* not a nI command: insert text */
		{
		insert1();          /* initial insert operations */

		if (command == TAB)         /* TAB insert puts in a tab first */
		    {
		    bb.p->ch[bb.c] = TAB;   /* insert a tab */
		    fwdcx(&bb);             /* advance pointer and maybe extend buffer */
		    }
		moveuntil(&cptr, &bb, term_char, &ins_count, cptr.z - cptr.dot, trace_sw);  /* copy cmd str -> buffer */
		if (command == TAB) ++ins_count;    /* add 1 if a tab inserted */
		cptr.dot += ins_count;  /* indicate advance over that many chars */
		}
	    insert2(ins_count);     /* finish insert */
	    getcmdc(trace_sw);      /* flush terminating char */
	    colonflag = atflag = esp->flag1 = esp->flag2 = 0;   /* clear args */
	    break;

/* type text from text buffer */
    
	case 't':
	    for (ll = line_args(0, &aa); ll > 0; ll--)  /* while there are chars to type */
		{
		type_char(aa.p->ch[aa.c]);
		fwdc(&aa);
		}
	    if (!WN_scroll) window(WIN_REDRAW);         /* if not in scroll mode, force full redraw on next refresh */
	    break;

	case 'v':
	    if ((ll = get_value(1)) > 0)        /* arg must be positive */
		{
		mm = lines(1 - ll);                 /* find start */
		nn = lines(ll) - mm;                /* and number of chars */
		set_pointer(pbuff->dot + mm, &aa);  /* pointer to start of text */
		for (; nn > 0; nn--)                /* as above */
		    {
		    type_char(aa.p->ch[aa.c]);
		    fwdc(&aa);
		    }
		}
	    if (!WN_scroll) window(WIN_REDRAW);         /* if not in scroll mode, force full redraw on next refresh */
	    break;

/* type text from command string */

	case CTL('A'):
	    term_char = (atflag) ? getcmdc(trace_sw) : CTL('A');  /* set terminator */
	    while (getcmdc(0) != term_char) type_char(cmdc);    /* output chars */
	    atflag = colonflag = esp->flag2 = esp->flag1 = 0;
	    esp->op = OP_START;
	    if (!WN_scroll) window(WIN_REDRAW);         /* if not in scroll mode, force full redraw on next refresh */
	    break;

    /* delete text */

	case 'd':
	    if (!esp->flag2)                /* if only one argument */
		{
		delete1(get_value(1));          /* delete chars (default is 1) */
		break;
		}               /* if two args, fall through to treat m,nD as m,nK */

	case 'k':                   /* delete lines or chars */
	    ll = line_args(1, &aa); /* aa points to start, ll chars, leave dot at beginning */
	    delete1(ll);            /* delete ll chars */
	    break;

/* q-register text loading commands */

	case CTL('U'):
	    mm = getqspec(0, getcmdc(trace_sw));
	    if (!colonflag)         /* X, ^U commands destroy previous contents */
		{
		dly_free_blist(qreg[mm].f);
		qreg[mm].f = NULL;
		qreg[mm].z = 0;
		}
	    term_char = (atflag) ? getcmdc(trace_sw) : ESC;     /* set terminator */
	    atflag = 0;         /* clear flag */

	    if ((esp->flag1) || (!peekcmdc(term_char)))     /* if an arg or a nonzero insert, find register */
		{
		make_buffer(&qreg[mm]);         /* attach a text buffer to the q register */
		for (bb.p = qreg[mm].f; bb.p->f != NULL; bb.p = bb.p->f);   /* find end of reg */
		bb.c = (colonflag) ? qreg[mm].z % CELLSIZE : 0;
		}
	    if (!(esp->flag1))
		{
		moveuntil(&cptr, &bb, term_char, &ll, cptr.z - cptr.dot, trace_sw);
		cptr.dot += ll;         /* indicate advance over that many chars */
		qreg[mm].z += ll;       /* update q-reg char count */
		getcmdc(trace_sw);      /* skip terminator */
		}
	    else
		{
		if (getcmdc(trace_sw) != term_char) ERROR(E_IIA);   /* must be zero length string */
		bb.p->ch[bb.c] = esp->val1;             /* store char */
		fwdcx(&bb);             /* extend the register */
		++qreg[mm].z;
		esp->flag1 = 0;         /* consume argument */
		}
	    colonflag = 0;
	    break;

	case 'x':
	    mm = getqspec(0, getcmdc(trace_sw));
	    if (!colonflag)         /* X, ^U commands destroy previous contents */
		{
		dly_free_blist(qreg[mm].f);     /* return, but delayed (in case executing now) */
		qreg[mm].f = NULL;
		qreg[mm].z = 0;
		}

	    if (ll = line_args(0, &aa))     /* read args and move chars, if any */
		{
		make_buffer(&qreg[mm]);         /* attach a text buffer to the q register */
		for (bb.p = qreg[mm].f; bb.p->f != NULL; bb.p = bb.p->f);   /* find end of reg */
		bb.c = (colonflag) ? qreg[mm].z % CELLSIZE : 0;

		movenchars(&aa, &bb, ll);
		qreg[mm].z += ll;       /* update q-reg char count */
		}
	    colonflag = 0;
	    break;

	case 'g':               /* get q register */
	    if (qreg[mm = getqspec(1, getcmdc(trace_sw))].z)    /* if any chars in it */
		{
		cc.p = qreg[mm].f;      /* point cc to start of reg */
		cc.c = 0;
		if (colonflag)      /* :Gx types q-reg */
		    {
		    for (ll = qreg[mm].z; ll > 0; ll--)
			{
			type_char(cc.p->ch[cc.c]);  /* type char */
			fwdc(&cc);
			}
		    }
		else
		    {
		    insert1();              /* set up for insert */
		    movenchars(&cc, &bb, qreg[mm].z);   /* copy q reg text */
		    insert2(qreg[mm].z);    /* finish insert */
		    }
		}
	    colonflag = 0;
	    break;

/* q-register push and pop */

	case '[':
	    if (qsp > &qstack[QSTACKSIZE-1]) ERROR(E_PDO);      /* stack full */
	    else
		{
		make_buffer(++qsp);     /* increment stack ptr and put a text buffer there */
		mm = getqspec(1, getcmdc(trace_sw));        /* get the q reg name */

		aa.p = qreg[mm].f;          /* point to the q register */
		aa.c = 0;
		bb.p = qsp->f;              /* point to the new list */
		bb.c = 0;
		movenchars(&aa, &bb, qreg[mm].z);   /* copy the text */
		qsp->z = qreg[mm].z;        /* and the length */
		qsp->v = qreg[mm].v;        /* and the value */
		}
	    break;

	case ']':
	    mm = getqspec(1, getcmdc(trace_sw));        /* get reg name */
	    if (qsp < &qstack[0])               /* if stack empty */
		{
		if (colonflag)                  /* :] returns 0 */
		    {
		    esp->flag1 = 1;
		    esp->val1 = 0;
		    colonflag = 0;
		    }
		else ERROR(E_CPQ);              /* ] makes an error */
		}
	    else                                    /* stack not empty */
		{
		free_blist(qreg[mm].f);         /* return orig contents of reg */
		qreg[mm].f = qsp->f;            /* substitute stack entry */
		qsp->f->b = (struct buffcell *) &qreg[mm];
		qsp->f = NULL;                  /* null out stack entry */
		qreg[mm].z = qsp->z;
		qreg[mm].v = qsp->v;
		if (colonflag)
		    {
		    esp->flag1 = 1;             /* :] returns -1 */
		    esp->val1 = -1;
		    colonflag = 0;
		    }
		--qsp;
		}
	    break;

	case '\\':
	    if (!(esp->flag1))      /* no argument; read number */
		{
		ll = esp->val1 = 0;         /* sign flag and initial value */
		for (ctrl_s = 0; pbuff->dot <= pbuff->z; pbuff->dot++, ctrl_s--)    /* count digits; don't read beyond buffer */
		    {
		    set_pointer(pbuff->dot, &aa);   /* point to dot */
		    if ((aa.p->ch[aa.c] == '+') || (aa.p->ch[aa.c] == '-'))
			{
			if (ll) break;      /* second sign: quit */
			else ll = aa.p->ch[aa.c];   /* first sign: save it */
			}
		    else
			{
			if (ctrl_r != 16)   /* octal or decimal */
			    {                   /* stop if not a valid digit */
			    if ((!isdigit(aa.p->ch[aa.c])) || (aa.p->ch[aa.c] - '0' >= ctrl_r)) break;
			    esp->val1 = esp->val1 * ctrl_r + (aa.p->ch[aa.c] - '0');
			    }
			else
			    {
			    if (!isxdigit(aa.p->ch[aa.c])) break;
			    esp->val1 = esp->val1 * 16 + ( (isdigit(aa.p->ch[aa.c])) ?
						aa.p->ch[aa.c] - '0' : mapch_l[aa.p->ch[aa.c]] - 'a' + 10);
			    }       /* end of hex */
			}       /* end of digit processing */
		    }       /* end of "for each char" */
		if (ll == '-') esp->val1 = -(esp->val1);    /* if minus sign */
		esp->flag1 = 1;             /* always returns a value */
		}

	    else                    /* argument: insert it as a digit string */
		{
		if (ctrl_r == 8)        sprintf(t_bcell.ch, "%o", esp->val1);   /* print as digits */
		else if (ctrl_r == 10)  sprintf(t_bcell.ch, "%d", esp->val1);
		else                    sprintf(t_bcell.ch, "%x", esp->val1);
		insert1();          /* start insert */
		cc.p = &t_bcell;    /* point cc to the temp cell */
		cc.c = 0;
		moveuntil(&cc, &bb, '\0', &ins_count, CELLSIZE-1, 0);   /* copy the char string */
		insert2(ins_count); /* finish the insert */
		esp->flag1 = 0;     /* consume argument */
		esp->op = OP_START;
		}
	    break;

	case CTL('T'):           /* type or input character */
	    if (esp->flag1)     /* type */
		{
		type_char(esp->val1);
		esp->flag1 = 0;
		if (!WN_scroll) window(WIN_REDRAW);         /* if not in scroll mode, force full redraw on next refresh */
		}
	    else
		{
		esp->val1 = (et_val & ET_NOWAIT) ? gettty_nowait() : gettty();
		if (!(et_val & ET_NOECHO) && (esp->val1 > 0) && !inp_noterm) type_char(esp->val1);      /* echo */
		esp->flag1 = 1;
		}
	    break;

/* search commands */

	case 's':                   /* search within buffer */
	    build_string(&sbuf);    /* read the search string */
	    end_search (  do_search( setup_search() )  );       /* search */
	    break;

	case 'n':                   /* search through rest of file */
	case '_':
	    do_nsearch(command);    /* call routine for N, _, E_ */
	    break;

	case 'o':                   /* branch to tag */
	    do_o();
	    break;

/* file I/O commands */

	case 'p':                   /* write a page, get next (ignore args for now) */
	    if (esp->flag1 && esp->flag2)   /* if two args */
		write_file(&aa, line_args(0, &aa), 0);      /* write spec'd buffer with no FF */
	    else                            /* one arg */
		{
		for (ll = get_value(1); ll > 0; ll--)   /* get count and loop */
		    {
		    set_pointer(0, &aa);
		    if (peekcmdc('w')) write_file(&aa, pbuff->z, 1);    /* PW writes buffer, then FF */
		    else
			{
			write_file(&aa, pbuff->z, ctrl_e);      /* P writes buffer, FF if read in, then gets next page */
			pbuff->dot = pbuff->z = 0;      /* empty the buffer */
			set_pointer(0, &aa);            /* set a pointer to the beginning of the buffer */
			pbuff->buff_mod = 0;            /* mark where new buffer starts */
			esp->val1 = read_file(&aa, &pbuff->z, (ed_val & ED_EXPMEM ? -1 : 0) );  /* read a page */
			esp->flag1 = colonflag;
			}
		    }
		}
	    if (peekcmdc('w')) getcmdc(trace_sw);       /* if a PW command, consume the W */
	    colonflag = 0;
	    break;

	case 'y':                   /* get a page into buffer */
	    if (esp->flag1) ERROR(E_NYA);
	    if ((pbuff->z) && (!(ed_val & ED_YPROT))) ERROR(E_YCA); /* don't lose text */
	    pbuff->dot = pbuff->z = 0;          /* clear buffer */
	    set_pointer(0, &aa);    /* set a pointer to the beginning of the buffer */
	    pbuff->buff_mod = 0;    /* mark where new buffer starts */
	    read_file(&aa, &pbuff->z, (ed_val & ED_EXPMEM ? -1 : 0) );      /* read a page */
	    esp->flag1 = colonflag;
	    esp->op = OP_START;
	    colonflag = 0;
	    break;

	case 'a':                           /* append, or ascii value */
	    if (esp->flag1 && !colonflag)       /* ascii value */
		{
		ll = pbuff->dot + esp->val1;    /* set a pointer before addr'd char */
		if ((ll >= 0) && (ll < pbuff->z))       /* if character lies within buffer */
		    {
		    set_pointer(ll, &aa);
		    esp->val1 = (int) aa.p->ch[aa.c];   /* get char (flag already set) */
		    }
		else esp->val1 = -1;        /* otherwise return -1 */
		}

	    else                            /* append */
		{
		set_pointer(tempz = pbuff->z, &aa);         /* set pointer to end of buffer, save current z */
		if (pbuff->z < pbuff->buff_mod) pbuff->buff_mod = pbuff->z;     /* mark where new buffer starts */
		if (esp->flag1 && (esp->val1 <= 0)) ERROR(E_IAA);               /* neg or 0 arg to :A */
		read_file(&aa, &pbuff->z, (esp->flag1 ? esp->val1 : 0) );       /* read a page */

		/* if line count spec'd, fails if eof; else fails if no chars read */
		esp->val1 = ( (esp->flag1) ? infile->eofsw : (pbuff->z == tempz) ) ? 0 : -1;
		esp->flag1 = colonflag;
		colonflag = 0;
		}
	    esp->op = OP_START;
	    break;

/* window commands */

	case 'w':
	    do_window(0);                   /* this stuff is with the window driver */
	    break;

	case CTL('W'):
	    do_window(1);                   /* this is, too */
	    break;

	default:
	    ERROR(E_ILL);                   /* invalid command */

	}       /* end of "switch" */
    return;     /* normal exit */
    }           /* end of exec_cmds1 */
