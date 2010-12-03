/* TECO for Ultrix   Copyright 1986 Matt Fichtenbaum                        */
/* This program and its components belong to GenRad Inc, Concord MA 01742   */
/* They may be copied if this copyright notice is included                  */

/* te_main.c  main program  5/27/87 */
/* version for multiple buffers  04/13/89 10.22                             */

/*
* This is TECO for Ultrix on a Vax.  It is mostly compatible with DEC TECO
* as explained in the DEC TECO manual.  It was written from a manual for
* TECO-11 version 34, and so adheres most closely to that version.
*
* This program consists of several source files, as follows:
*
* te_main.c (this file)     Main program - initialize, read command line and
*                           startup file, handle errors, high-level read and
*                           execute command strings.
*
* te_defs.h                 Definitions file, to be #included with other files
*
* te_data.c                 Global variables
*
* te_rdcmd.c                Read in a command string
*
* te_exec0.c                First-level command execution - numbers, values,
*                           assemble expressions
*
* te_exec1.c                Most commands
*
* te_exec2.c                "E" and "F" commands, and file I/O
*
* te_srch.c                 routines associated with "search" commands
*
* te_subs.c                 higher-level subroutines
*
* te_utils.c                lower-level subroutines
*
* te_chario.c               keyboard (stdin), typeout (stdout), suspend
*
* te_window.c               display window and display special functions
*
* These routines should be compiled and linked to form the TECO executable.
*/

#include "te_defs.h"

main(argc, argv)
    int argc;           /* arg count */
    char *argv[];       /* array of string pointers */
    {
    int i;

    save_args(argc, argv, &qreg[36]);   /* copy command line to Qz */
    read_startup();                     /* read startup file */
    setup_tty(TTY_ON);      /* set tty to CBREAK, no echo, asynch mode */
    get_term_par();         /* set terminal screen-size parameters */
    window(WIN_INIT);       /* initialize screen-image buffer */
    qsp = &qstack[-1];      /* initialize q-reg stack pointer */

/* set up error restart */
    if (err = setjmp(xxx))
	{
	if (err == E_EFI) goto quit;        /* EOF from standard input - clean up and exit */
	printf("\015\012?  %s", errors[err-1]);
	if (err == E_SRH) print_string(SERBUF);         /* print unfulfilled search string */
	else if ((err == E_FNF) || (err == E_COF) || (err == E_AMB)) print_string(FILBUF);  /* or file string */
	crlf();
	eisw = 0;               /* stop indirect command execution */
	et_val &= ~(ET_CTRLC | ET_NOWAIT | ET_CTRLO | ET_NOECHO);   /* reset ^C trap, read w/o wait, ^O (unused), no echo */
	if (et_val & ET_QUIT)   /* if ET has "quit on error" set, exit (phone home) */
	    {
	    cleanup();          /* reset screen, keyboard, output files */
	    exit(1);            /* and exit */
	    }
	}

/* forever: read and execute command strings */

    for (exitflag = 1; exitflag >= 0; )     /* "exit" sets exitflag to -1; ^C to -2; "hangup" to -3 */
	{
	window(WIN_REFR);               /* display the buffer */
	free_blist(insert_p);           /* free any storage from failed insert */
	free_blist(dly_freebuff);       /* return any delayed cells */
	insert_p = dly_freebuff = NULL;
	et_val &= ~ET_QUIT;             /* clear "abort on error" */
	if (read_cmdstr()) goto quit;
	exitflag = 0;                   /* enable ^C detector */
	if (!WN_scroll) window(WIN_REDRAW);     /* if not in scroll mode, force full redraw on first ^W or nW */
	exec_cmdstr();
	}

    if (exitflag == -2) ERROR(E_XAB);       /* ^C detected during execution */
    else if (exitflag == -3) panic();       /* hangup during execution: save buffer and close files */

/* exit from program */

  quit:
    ev_val = es_val = 0;    /* no last one-line window */
    window(WIN_REFR);       /* last display */
    cleanup();              /* reset screen, terminal, output files */
    exit(0);                /* and quit */
    }

/* reset screen state, keyboard state; remove open output files */

cleanup()
    {
    window(WIN_OFF);            /* restore screen */
    setup_tty(TTY_OFF);         /* restore terminal */
    kill_output(&po_file);      /* kill any open primary output file */
    kill_output(&so_file);      /* and secondary file */
    }


/* print string for error message */
/* argument is subscript of a qreg qh, prints text from that buffer */

print_string(arg)
    int arg;
    {
    int i, c;
    struct buffcell *p;

    type_char('"');
    for (p = qreg[arg].f, c = 0, i = 0; i < qreg[arg].z; i++)
	{
	if (!p->ch[c]) break;
	type_char(p->ch[c]);
	if (++c > CELLSIZE-1)
	    {
	    p = p->f;
	    c = 0;
	    }
	}
    type_char('"');
    }

/* copy invocation command line to a text buffer */

save_args(argc, argv, q)
    int argc;
    char *argv[];
    struct qh *q;
    {
    char c;
    struct qp ptr;

    make_buffer(q);             /* attach a text buffer */
    ptr.p = q->f;               /* initialize pointer to output string */
    ptr.c = q->z = 0;           /* and output char count */
    for (; argc > 0; argv++, argc--)    /* for each arg */
	{
	while ( ((c = *((*argv)++)) != '\0') && (q->z < CELLSIZE-1) )
	    {
	    ptr.p->ch[ptr.c] = c;       /* copy char to q-reg */
	    fwdcx(&ptr);
	    ++q->z;                     /* count characters */
	    }
	if (argc > 1)                   /* if not last argument... */
	    {
	    ptr.p->ch[ptr.c] = ' ';     /* space to separate arguments */
	    fwdcx(&ptr);
	    ++q->z;
	    }
	}
    }



/* routine to read startup file */

char startup_name[] = "/.tecorc";       /* name of startup file */

read_startup()
    {
    char *hp, *getenv();
    char fn[CELLSIZE];      /* filename storage */
    int i;

/* look for ".tecorc" in current directory first */

    if (!(eisw = fopen(&startup_name[1], "r")))
	{
	if (hp = getenv("HOME"))    /* if not found, look in home directory */
	    {
	    for (i = 0; i < CELLSIZE; i++) if (!(fn[i] = *(hp++))) break;   /* copy until trailing null */
	    for (hp = &startup_name[0]; i < CELLSIZE; i++) if (!(fn[i] = *(hp++))) break;
	    eisw = fopen(fn, "r");      /* set eisw if file found, or not if not */
	    }
	}
    }

/* routine to get terminal height and width from termcap */

get_term_par()
    {
    char tbuff[1024];   /* termcap buffer */
    char *pname;        /* pointer to name of terminal */
    extern char *getenv();

    if (pname = getenv("TERM"))     /* read terminal name */
	{
	tgetent(tbuff, pname);      /* get entry */
	set_term_par(tgetnum("li"), tgetnum("co")); /* get #lines and #columns and set params */
	}
    }
