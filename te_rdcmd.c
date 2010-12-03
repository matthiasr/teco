/* TECO for Ultrix   Copyright 1986 Matt Fichtenbaum                        */
/* This program and its components belong to GenRad Inc, Concord MA 01742   */
/* They may be copied if this copyright notice is included                  */

/* te_rdcmd.c  read in the command string  6/4/87 */
/* version for multiple buffers  04/13/89 10.22                             */
#include "te_defs.h"

int ccount;             /* count of chars read in */

int read_cmdstr()
    {
    char c;             /* temporary character */
    int i;              /* temporary */

    goto prompt;

  restart:                  /* prompt again: new line */
    if (!eisw && !inp_noterm) crlf();       /* if input not from a file */
  prompt:                   /* issue prompt */
    if (!eisw && !inp_noterm)
	{
#ifndef _POSIX_SOURCE
	reset_ctlo();       /* reset any ^O */
#endif /* _POSIX_SOURCE */
	type_char('*');
	}
    ccount = 0;
    lastc = ' ';

reline:                 /* continue reading */
    for (;;)            /* loop to read command string chars */
	{
	if (!eisw && !inp_noterm)       /* if terminal input */
	    {
	    if ((c = gettty()) == DEL)      /* process rubout */
		{
		if (!ccount) goto restart;      /* if at beginning, ignore */
		--ccount;                       /* decrement char count */
		backc(&cmdstr);                 /* back up the command-string pointer */

/* look at the character just deleted */
		if (((c = cmdstr.p->ch[cmdstr.c]) < ' ') && (c != ESC))     /* control chars: set c to char erased */
		    {
		    if (c == LF) vt(VT_LINEUP);     /* line up */

		    else if ((c == CR) || (c == TAB))
			{
			i = find_lasteol();         /* back up to previous line */
			type_char(CR);              /* erase current line */
			vt(VT_EEOL);
			if (i == ccount) type_char('*');        /* if this was line with prompt, retype the prompt */
			for (; (t_qp.p != cmdstr.p) || (t_qp.c != cmdstr.c); fwdc(&t_qp))
			    type_char(t_qp.p->ch[t_qp.c]);      /* retype line: stop before deleted position */
			}

		    else
			{
			vt(VT_BS2);             /* erase ordinary ctrl chars */
			char_count -= 2;
			}
		    }

		else
		    {
		    vt(VT_BS1);             /* erase printing chars */
		    char_count--;
		    }
		lastc = ' ';        /* disable dangerous last chars */
		continue;
		}                   /* end 'rubout' processing */

	    else if (c == CTL('U'))         /* process "erase current line" */
		{
		type_char(CR);              /* erase line */
		vt(VT_EEOL);
		if ((ccount -= find_lasteol()) <= 0) goto prompt;   /* back up to last eol: if beginning, restart */
		cmdstr.p = t_qp.p;          /* put command pointer back to this point */
		cmdstr.c = t_qp.c;
		lastc = ' ';
		continue;               /* and read it again */
		}

	    else            /* not a rubout or ^U */
		{
		if (!ccount)        /* if at beginning of line */
		    {
		    if (c == '*')       /* save old command string */
			{
			type_char('*');     /* echo character */
			type_char(c = gettty());    /* read reg spec and echo */
			i = getqspec(0, c);
			free_blist(qreg[i].f);      /* return its previous contents */
			qreg[i].f = cbuf.f;         /* put the old command string in its place */
			if (qreg[i].f) qreg[i].f->b = (struct buffcell *) &qreg[i];
			qreg[i].z = cbuf.z;
			cbuf.f = (struct buffcell *) (cbuf.z = 0);      /* no old command string */
			err = 0;                    /* no previous error */
			goto restart;
			}
		    else if ((c == '?') && (err))   /* echo previous command string up to error */
			{
			type_char('?');         /* echo ? */
			for (aa.p = cptr.p; aa.p->b->b != NULL; aa.p = aa.p->b);    /* find beginning */
			for (aa.c = 0; (aa.p != cptr.p) || (aa.c < cptr.c); fwdc(&aa)) type_char(aa.p->ch[aa.c]);
			type_char('?');             /* a final ? */
			err = 0;                /* reset error switch */
			goto restart;
			}
		    else if ((c == LF) || (c == CTL('H')))   /* line feed, backspace */
			{
			pbuff->dot += lines( (c == LF) ? 1 : -1);   /* pointer up or down one line */
			window(WIN_LINE);           /* display one line */
			goto restart;
			}
		    else if ((c == CTL('W')) && (WN_scroll != 0)) /* immediate window redisplay */
			{
			window(WIN_REDRAW);     /* redraw full window */
			window(WIN_REFR);
			goto restart;
			}

		    else                    /* first real command on a line */
			{
			make_buffer(&cbuf); /* start a command string if need be */
			cmdstr.p = cbuf.f;  /* set cmdstr to point to start of command string */
			cmdstr.c = 0;
			cbuf.z = 0;         /* no chars in command string now */
			err = 0;            /* clear last error flag */
			}
		    }   /* end of "if first char on line" */                


/* check ^G-something */

		if (lastc == CTL('G'))
		    {
		    if (c == CTL('G'))
			{
			cbuf.z = ccount;    /* save count for possible "save in q-reg" */
			goto restart;
			}
		    if ((c == '*') || (c == ' '))
			{
			backc(&cmdstr);     /* remove the previous ^G from buffer */
			--ccount;
			crlf();
			retype_cmdstr(c);   /* retype appropriate part of command string */
			lastc = ' ';
			continue;
			}           /* end of ^G* and ^G<sp> processing */
		    }           /* end of "last char was ^G" */
		}           /* end of "not rubout or ^U */
	    }           /* end of "if !eisw" */

	else            /* if input from indirect file or redirected std input */
	    {
	    if (!ccount)    /* first command? */
		{
		if (!cbuf.f)    /* start a command string if need be */
		    {
		    cbuf.f = get_bcell();
		    cbuf.f->b = (struct buffcell *) &cbuf;
		    }
		cmdstr.p = cbuf.f;      /* point cmdstr to start of command string */
		cbuf.z = cmdstr.c = 0;
		}

	    c = (eisw) ? getc(eisw) : gettty() ;    /* get char */
	    if (eisw && (c == EOF))         /* if this is end of the indirect command file */
		{
		fclose(eisw);               /* close the input file */
		eisw = 0;                   /* reset the switch */
		lastc = ' ';
		continue;                   /* and go read more chars */
		}
	    else
		{
		if ((c == LF) && (lastc != CR) && !(ez_val & EZ_CRLF))  /* LF: store implied CR first */
		    {
		    cmdstr.p->ch[cmdstr.c] = CR;
		    ++ccount;
		    fwdcx(&cmdstr);
		    }
		}
	    }           /* end of "if redirected std in or eisw" */


/* store character in command string */

	    cmdstr.p->ch[cmdstr.c] = c;     /* store the character */
	    ++ccount;                       /* keep count of chars */
	    if (!eisw && !inp_noterm) type_char(c);     /* echo the character */
	    fwdcx(&cmdstr);                 /* next char pos'n; extend command string if nec */

	    if ((c == ESC) && (lastc == ESC)) break;    /* stop on 2nd ESC */
	    if ((c == CTL('C')) && (lastc == CTL('C'))) return(-1);   /* immediate exit */
	    lastc = c;                              /* keep track of last char */
	    }                   /* end of read-char loop */

    cbuf.z = ccount;        /* indicate number of chars in command string */
    if (!eisw && !inp_noterm) crlf();       /* final new-line */
    return(0);
    }                   /* end of read_cmdstr() */

/* back up to find most recent CR or LF in entered command string */
/* return number of chars backed up */

int find_lasteol()
    {
    int i;

    for (i = 0, t_qp.p = cmdstr.p, t_qp.c = cmdstr.c; (backc(&t_qp)) ; i++) /* look for beg. of line */
	{
	if ((t_qp.p->ch[t_qp.c] == CR) || (t_qp.p->ch[t_qp.c] == LF))
	    {
	    fwdc(&t_qp);    /* stop short of previous eol */
	    break;
	    }
	}
    char_count = 0;             /* reset tab count */
    return(i);
    }



/* retype command string: entirely (arg = '*') or most recent line (arg = ' ') */

retype_cmdstr(c)
    char c;
    {
    int i;

    if (!inp_noterm)    /* if input is really from terminal */
	{
	if (c == ' ')       /* look for beginning of this line */
	    i = ccount - find_lasteol();    /* to last eol, and count char's backed up */
	else
	    {
	    t_qp.p = cbuf.f;    /* retype whole command string */
	    i = t_qp.c = 0;
	    }
	if (!i) type_char('*'); /* if from beginning, retype prompt */
	for (; i < ccount; i++)     /* type command string from starting point */
	    {
	    type_char(t_qp.p->ch[t_qp.c]);
	    fwdc(&t_qp);
	    }
	}
    }
