/* TECO for Ultrix   Copyright 1986 Matt Fichtenbaum                        */
/* This program and its components belong to GenRad Inc, Concord MA 01742   */
/* They may be copied if this copyright notice is included                  */

/* te_utils.c utility subroutines  10/28/85 */
/* version for multiple buffers  04/13/89 11.30                             */

#include "te_defs.h"

/* routines to handle storage */
/* get a buffcell */
/* if there are no buffcells available, call malloc for more storage */

struct buffcell *get_bcell()
    {
    char *malloc();
    struct buffcell *p;
    int i;

    if (freebuff == NULL)
	{
	p = (struct buffcell *) malloc(BLOCKSIZE);
	if (!p) ERROR(E_MEM);
	else
	    {
	    freebuff =  p;                  /* take the given address as storage */
	    for (i = 0; i < (BLOCKSIZE/sizeof(struct buffcell)) - 1; i++)   /* for all cells in the new block */
		(p+i)->f = p+i+1;           /* chain the forward pointers together */
	    (p+i)->f = NULL;                /* make the last one's forward pointer NULL */
	    }
	}

    p = freebuff;               /* cut out head of "free" list */
    freebuff = freebuff->f;
    p->f = p->b = NULL;
    return(p);
    }


/* free a list of buffcells */
free_blist(p)
    struct buffcell *p;
    {
    struct buffcell *t;

    if (p != NULL)
    {
	for (t = p; t -> f != NULL; t = t -> f); /* find end of ret'd list */
	t->f = freebuff;        /* put ret'd list at head of "free" list */
	freebuff = p;
	}
    }

/* free a list of buffcells to the "delayed free" list */
dly_free_blist(p)
    struct buffcell *p;
    {
    struct buffcell *t;

    if (p != NULL)
    {
	for (t = p; t -> f != NULL; t = t -> f); /* find end of ret'd list */
	t->f = dly_freebuff;        /* put ret'd list at head of "free" list */
	dly_freebuff = p;
	}
    }



/* get a cell */
/* if there are no cells available, get a buffcell and make more */

struct qp *get_dcell()
    {
    struct qp *t;
    int i;

    if (freedcell == NULL)
	{
	t = freedcell = (struct qp *) get_bcell();  /* get a buffcell */
	for (i = 0; i < (sizeof(struct buffcell)/sizeof(struct qp)) - 1; i++)
	    {
	    (t+i)->f = t+i+1;   /* chain the fwd pointers together */
	    (t+i)->f = NULL;    /* make the last one's forward pointer NULL */
	    }
	}

    t = freedcell;              /* cut out head of "free" list */
    freedcell = (struct qp *) freedcell->f;
    t->f =  NULL;
    return(t);
    }


/* free a list of cells */
free_dlist(p)
    struct qp *p;
    {
    struct qp *t;

    if (p != NULL)
	{
	for (t = p; t->f != NULL; t = t->f); /* find end of ret'd list */
	t->f = freedcell;   /* put ret'd list at head of "free" list */
	freedcell = p;
	}
    }

/* build a buffer:  called with address of a qh */
/* if no buffer there, get a cell and link it in */
make_buffer(p)
    struct qh *p;
    {
    if (!(p->f))
	{
	p->f = get_bcell();
	p->f->b = (struct buffcell *) p;
	}
    }


/* routines to advance one character forward or backward */
/* argument is the address of a qp */
/* fwdc, backc return 1 if success, 0 if beyond extremes of buffer) */
/* fwdcx extends buffer if failure */

int fwdc(arg)
    struct qp *arg;
    {
    if ((*arg).c >= CELLSIZE-1)     /* test char count for max */
	{
	if ((*arg).p->f == NULL) return(0);     /* last cell: fail */
	else
	    {
	    (*arg).p = (*arg).p->f; /* chain through list */
	    (*arg).c = 0;           /* and reset char count */
	    }
	}
    else ++(*arg).c;                /* otherwise just incr char count */
    ++(*arg).dot;
    return(1);
    }

fwdcx(arg)
    struct qp *arg;
    {
    if ((*arg).c >= CELLSIZE-1)     /* test char count for max */
	{
	if ((*arg).p->f == NULL)    /* last cell: extend */
	    {
	    (*arg).p->f = get_bcell();
	    (*arg).p->f->b = (*arg).p;
	    }
	(*arg).p = (*arg).p->f;     /* chain through list */
	(*arg).c = 0;               /* and reset char count */
	}
    else ++(*arg).c;                /* otherwise just incr char count */
    ++(*arg).dot;
    return(1);
    }

int backc(arg)
    struct qp *arg;
    {
    if ((*arg).c <= 0)          /* test char count for min */
	{
	if ((*arg).p->b->b == NULL) return(0);      /* first cell: fail */
	else
	    {
	    (*arg).p = (*arg).p->b; /* chain through list */
	    (*arg).c = CELLSIZE-1;  /* reset char count */
	    }
	}
    else --(*arg).c;                /* otherwise just decr char count */
    --(*arg).dot;
    return(1);
    }


/* set up a pointer to a particular text buffer position */

set_pointer(pos, ptr)   /* first arg is position, 2nd is addr of pointer */
    int pos;
    struct qp *ptr;
    {
    struct buffcell *t;
    int i;

    if (!pbuff->f)
	{
	pbuff->f = get_bcell();         /* if no text buffer, make one  */
	pbuff->f->b = (struct buffcell *) &pbuff->f;
	}
    for (i = pos/CELLSIZE, t = pbuff->f; (i > 0) && (t->f != NULL); i--) t = t->f;
    ptr->p = t;
    ptr->c = pos % CELLSIZE;
    ptr->dot = pos;
    ptr->z = pbuff->z;
    }

/* routines to get next character from command buffer       */
/* getcmdc0, when reading beyond command string, pops       */
/* macro stack and continues.                               */
/* getcmdc, in similar circumstances, reports an error      */
/* if pushcmdc() has returned any chars, read them first    */
/* routines type characters as read, if argument != 0       */

char getcmdc0(trace)
    {
    while (cptr.dot >= cptr.z)      /* if at end of this level, pop macro stack */
	{
	if (--msp < &mstack[0])     /* pop stack; if top level      */
	    {
	    msp = &mstack[0];       /* restore stack pointer    */
	    cmdc = ESC;             /* return an ESC (ignored)      */
	    exitflag = 1;           /* set to terminate execution   */
	    return(cmdc);           /* exit "while" and return */
	    }
	}
    cmdc = cptr.p->ch[cptr.c++];        /* get char */
    ++cptr.dot;                         /* increment character count */
    if (trace) type_char(cmdc);         /* trace */
    if (cptr.c > CELLSIZE-1)            /* and chain if need be */
	{
	cptr.p = cptr.p->f;
	cptr.c = 0;
	}
    return(cmdc);
    }


char getcmdc(trace)
    {
    if (cptr.dot++ >= cptr.z) ERROR((msp <= &mstack[0]) ? E_UTC : E_UTM);
    else
	{
	cmdc = cptr.p->ch[cptr.c++];    /* get char */
	if (trace) type_char(cmdc);     /* trace */
	if (cptr.c > CELLSIZE-1)        /* and chain if need be */
	    {
	    cptr.p = cptr.p->f;
	    cptr.c = 0;
	    }
	}
    return(cmdc);
    }


/* peek at next char in command string, return 1 if it is equal (case independent) to argument */
int peekcmdc(arg)
    char arg;
    {
    return(((cptr.dot < cptr.z) && (mapch_l[cptr.p->ch[cptr.c]] == mapch_l[arg])) ? 1 : 0);
    }
