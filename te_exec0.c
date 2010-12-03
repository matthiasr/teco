/* TECO for Ultrix   Copyright 1986 Matt Fichtenbaum                        */
/* This program and its components belong to GenRad Inc, Concord MA 01742   */
/* They may be copied if this copyright notice is included                  */

/* te_exec0.c   execute command string   6/10/87 */
/* version for multiple buffers  04/13/89 10.22                             */
#include "te_defs.h"
#include <time.h>
#include <sys/time.h>

exec_cmdstr()
    {
    char c;
    int digit_sw;
    struct tm *timeptr;
    char *timestring, *asctime();
    time_t now;

    exitflag = 0;                   /* set flag to "executing" */
    cmdstr.p = cbuf.f;              /* cmdstr to start of command string */
    cmdstr.z = cbuf.z;
    cmdstr.flag = cmdstr.dot = cmdstr.c = 0;    /* clear char ptr and iteration flag */
    msp = &cmdstr;                  /* initialize macro stack pointer */
    esp = &estack[0];               /* initialize expression stack pointer */
    atflag = colonflag = 0;         /* clear flags */
    esp->flag2 = esp->flag1 = 0;    /* initialize expression reader */
    esp->op = OP_START;
    trace_sw = 0;                   /* start out with trace off */
    digit_sw = 0;                   /* and no digits read */

    while (!exitflag)               /* until end of command string */
	{
	if (getcmdc0(trace_sw) == '^')      /* interpret next char as corresp. control char */
	    cmdc = getcmdc(trace_sw) & 0x1f;

	if (isdigit(cmdc))      /* process number */
	    {                   /* this works lousy for hex but so does TECO-11 */
	    if (cmdc - '0' >= ctrl_r) ERROR(E_ILN);             /* invalid digit */
	    if (!(digit_sw++)) esp->val1 = cmdc - '0';          /* first digit */
	    else esp->val1 = esp->val1 * ctrl_r + cmdc - '0';   /* other digits */
	    esp->flag1++;       /* indicate a value read in */
	    }

    /* not a digit: dispatch on character */

	else
	    {
	    digit_sw = 0;
	    switch (mapch_l[cmdc])
		{

/* characters ignored */

		case CR:
		case LF:
		case VT:
		case FF:
		case ' ':
		    break;

/* ESC: one absorbs argument, two terminate current level */

		case ESC:
		    if (peekcmdc(ESC))  /* if next char is an ESC */
			{
			if (msp <= &mstack[0]) exitflag = 1;    /* pop stack; if empty, terminate */
			else --msp;
			}
		    else
			{
			esp->flag1 = 0;     /* else consume argument */
			esp->op = OP_START;
			}
		    break;

/* skip comments */

		case '!':
		    while (getcmdc(trace_sw) != '!');
		    break;

/* modifiers */

		case '@':
		    atflag++;
		    break;

		case ':':
		    if (peekcmdc(':'))      /* is it "::" ? */
			{
			getcmdc(trace_sw);  /* yes, skip 2nd : */
			colonflag = 2;      /* and set flag to show 2 */
			}
		    else colonflag = 1;     /* otherwise just 1 colon */
		    break;

/* trace control */

		case '?':
		    trace_sw = !(trace_sw);
		    break;

/* values */

		case '.':
		    esp->val1 = pbuff->dot;
		    esp->flag1 = 1;
		    break;

		case 'z':
		    esp->val1 = pbuff->z;
		    esp->flag1 = 1;
		    break;

		case 'b':
		    esp->val1 = 0;
		    esp->flag1 = 1;
		    break;

	    case 'h':
		    esp->val1 = pbuff->z;
		    esp->val2 = 0;
		    esp->flag2 = esp->flag1 = 1;
		    esp->op = OP_START;
		    break;

		case CTL('S'):       /* -length of last insert, etc. */
		    esp->val1 = ctrl_s;
		    esp->flag1 = 1;
		    break;

		case CTL('Y'):       /* .-^S, . */
		    esp->val1 = pbuff->dot + ctrl_s;
		    esp->val2 = pbuff->dot;
		    esp->flag1 = esp->flag2 = 1;
		    esp->op = OP_START;
		    break;

		case '(':
		    if (++esp > &estack[ESTACKSIZE-1]) ERROR(E_PDO);
		    esp->flag2 = esp->flag1 = 0;
		    esp->op = OP_START;
		    break;

		case CTL('E'):               /* form feed flag */
		    esp->val1 = ctrl_e;
		    esp->flag1 = 1;
		    break;

		case CTL('N'):               /* eof flag */
		    esp->val1 = infile->eofsw;
		    esp->flag1 = 1;
		    break;

		case CTL('^'):               /* value of next char */
		    esp->val1 = getcmdc(trace_sw);
		    esp->flag1 = 1;
		    break;

/* date, time */

		case CTL('B'):
		case CTL('H'):
		    (void) time(&now);
		    timeptr = localtime(&now);
		    esp->val1 = (cmdc == CTL('B')) ?
			    timeptr->tm_year * 512   + (timeptr->tm_mon + 1) * 32  + timeptr->tm_mday :
			    timeptr->tm_hour * 1800  + timeptr->tm_min * 30  + timeptr->tm_sec/2;
		    esp->flag1 = 1;
		    make_buffer(&timbuf);       /* make a time buffer */
		    timestring = asctime(timeptr);
		    for (timbuf.z = 0; timbuf.z < 24; timbuf.z++)       /* copy character string */
			timbuf.f->ch[timbuf.z] = *(timestring + timbuf.z);
		    break;

/* number of characters to matching ( ) { } [ ] */

		case CTL('P'):
		    do_ctlp();
		    break;

/* none of the above: incorporate the last value into the expression */

		default:
		    if (esp->flag1)             /* if a value entered */
			{
			switch (esp->op)
			    {
			    case OP_START:
				break;

			    case OP_ADD:
				esp->val1 += esp->exp;
				esp->op = OP_START;
				break;

			    case OP_SUB:
				esp->val1 = esp->exp - esp->val1;
				esp->op = OP_START;
				break;

			    case OP_MULT:
				esp->val1 *= esp->exp;
				esp->op = OP_START;
				break;

			    case OP_DIV:
				esp->val1 = (esp->val1) ? esp->exp / esp->val1 : 0;
				esp->op = OP_START;
				break;

			    case OP_AND:
				esp->val1 &= esp->exp;
				esp->op = OP_START;
				break;

			    case OP_OR:
				esp->val1 |= esp->exp;
				esp->op = OP_START;
				break;
			    }
		    }               /* end of "if a new value" */

		exec_cmds1();   /* go do the command */
		}       /* end of command dispatch */
	    }       /* end of "not a digit" */
	}       /* end of "while" command loop */
    return;
    }           /* end of exec_cmdstr */
