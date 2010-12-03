/* TECO for Ultrix   Copyright 1986 Matt Fichtenbaum                        */
/* This program and its components belong to GenRad Inc, Concord MA 01742   */
/* They may be copied if this copyright notice is included                  */

/* te_data.c global variables  12/31/85 */
/* version for multiple buffers  04/13/89 15.54                             */

#include "te_defs.h"

/* error message text */
char *errors[] =
    {
    "> not in iteration",
    "Can't pop Q register",
    "Can't open output file ",
    "File not found ",
    "Invalid E character",
    "Invalid F character",
    "Invalid insert arg",
    "Invalid command",
    "Invalid number",
    "Invalid P arg",
    "Invalid \" character",
    "Invalid Q-reg name",
    "Invalid radix arg",
    "Invalid search arg",
    "Invalid search string",
    "Invalid ^ character",
    "Insufficient memory available",
    "Missing )",
    "No arg before ^_",
    "No arg before ,",
    "No arg before =",
    "No arg before )",
    "No arg before \"",
    "No arg before ;",
    "No arg before U",
    "No file for input",
    "No file for output",
    "Numeric arg with Y",
    "Output file already open",
    "Pushdown list overflow",
    "Pointer off page",
    "; not in iteration",
    "Search failure ",
    "String too long",
    "Unterminated command",
    "Unterminated macro",
    "Execution interrupted",
    "Y command suppressed",
    "Invalid W arg",
    "Numeric arg with FR",
    "Internal error",
    "EOF read from std input",
    "Invalid A arg",
    "Ambiguous file specification ",
    "System fork or pipe error",
    "Invalid buffer specification",
    "File I/O switching is implicit",
    "File open or buffer not empty"
    } ;

/* declare global variables */
    struct buffcell *freebuff = NULL;       /* buffcell free-list pointer */
    struct buffcell *dly_freebuff = NULL;   /* delayed free-list pointer */
    struct qp *freedcell = NULL;            /* cell free-list pointer */

/* the text buffer headers */
    struct bh buffs[2] = {
	{ NULL, NULL, 0, 0, 0 },
	{ NULL, NULL, 0, 0, 0 }
    } ;
    struct bh *pbuff = buffs;

/* the q-register headers point to the start of the buffer and registers */
    struct qh qreg[NQREGS+5];       /* for q regs, command, search, file, sys-command, time/date */

/* the q-register stack contains temporary copies of q-register contents */
    struct qh qstack[QSTACKSIZE];   /* q-reg stack */
    struct qh *qsp;                 /* q-reg stack pointer */

/* the macro stack contains pointers to the currently active macros. */
/* the top of the stack is the command pointer */
    struct qp mstack[MSTACKSIZE];   /* macro stack */
    struct qp *msp;                 /* macro stack pointer */

/* the expression stack */
    struct exp_entry estack[ESTACKSIZE];    /* expression stack */
    struct exp_entry *esp;                  /* expression stack pointer */

/* global variables, etc. */

int char_count = 0;                 /* char count for tab typer         */
char lastc = ' ';                   /* last char read                   */
int ttyerr;                         /* error return from ioctl          */
extern int errno;                   /* system error code                */
jmp_buf xxx;                        /* preserved environment for error restart */
int err;                            /* local error code                 */
struct qp t_qp;                     /* temporary buffer pointer         */
struct qp aa, bb, cc;               /* more temporaries                 */
struct buffcell t_bcell;            /* temporary bcell                  */
int tabmask = 7;                    /* mask for typing tabs             */
int exitflag;                       /* flag for ending command str exec */
char term_char = ESC;               /* terminator for insert, search, etc.  */
char cmdc;                          /* current command character        */
char skipc;                         /* char found by "skipto()"         */
int dot, z, tdot;                   /* current, last, temp buffer position  */
int ins_count;                      /* count of chars inserted          */
int ll, mm, nn;                     /* general temps                    */
int ctrl_e = 0;                     /* form feed flag                   */
int ctrl_r = 10;                    /* numeric radix (8, 10, 16)        */
int ctrl_s = 0;                     /* string length for S, I, G        */
int ctrl_x = 0;                     /* search mode flag                 */
int ed_val = 0;                     /* ED value                         */
int es_val = 0;                     /* ES value                         */
int et_val = 518;                   /* ET value                         */
int eu_val = -1;                    /* EU value                         */
int ev_val = 0;                     /* EV value                         */
int ez_val = 0;                     /* EZ value                         */
int srch_result = 0;                /* result of last :S executed       */
int atflag = 0;                     /* flag for @ char typed            */
int colonflag = 0;                  /* flag for : char typed            */
int trace_sw = 0;                   /* nonzero if tracing command exec  */

struct buffcell *insert_p;          /* pointer to temp buffer segment during insert */
int search_flag;                    /* set nonzero by search            */

/* character mapping table (direct) */
char mapch[] =
{
 0,   1,   2,   3,   4,   5,   6,   7,   8,   9,  10,  11,  12,  13,  14,  15,      /* ^@ - ^M */
16,  17,  18,  19,  20,  21,  22,  23,  24,  25,  26,  27,  28,  29,  30,  31,      /* ^N - ^_ */
' ', '!', '"', '#', '$', '%', '&', '\'','(', ')', '*', '+', ',', '-', '.', '/',     /* sp - /  */
'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', ':', ';', '<', '=', '>', '?',     /* 0 - ?   */
'@', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O',     /* @ - O   */
'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', '[', '\\',']', '^', '_',     /* P - _   */
'`', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o',     /* ` - o   */
'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z', '{', '|','}', '~', 0177      /* p - del */
} ;

/* character table (mapped to lower case) */
char mapch_l[] =
{
 0,   1,   2,   3,   4,   5,   6,   7,   8,   9,  10,  11,  12,  13,  14,  15,      /* ^@ - ^M */
16,  17,  18,  19,  20,  21,  22,  23,  24,  25,  26,  27,  28,  29,  30,  31,      /* ^N - ^_ */
' ', '!', '"', '#', '$', '%', '&', '\'','(', ')', '*', '+', ',', '-', '.', '/',     /* sp - /  */
'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', ':', ';', '<', '=', '>', '?',     /* 0 - ?   */
'@', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o',     /* @ - O   */
'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z', '[', '\\',']', '^', '_',     /* P - _   */
'`', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o',     /* ` - o   */
'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z', '{', '|','}', '~', 0177      /* p - del */
} ;

/* table of special characters for "search," "skipto()," and "lines()"  */
/* see "te_defs.h for meaning of bits */

unsigned char spec_chars[] =
{
0,          A_S,        0,          0,              /* ^@ ^A ^B ^C */
0,          A_A,        0,          0,              /* ^D ^E ^F ^G */
0,          A_T,        A_L,        A_L,            /* ^H ^I ^J ^K */
A_L,        0,          A_A,        0,              /* ^L ^M ^N ^O */
0,          A_A,        A_A,        A_A,            /* ^P ^Q ^R ^S */
0,          A_T|A_Q,    0,          0,              /* ^T ^U ^V ^W */
A_A,        0,          0,          0,              /* ^X ^Y ^Z ^[ */
0,          0,          A_S,        0,              /* ^\ ^] ^^ ^_ */
0,          A_S,        A_X,        0,              /*    !  "  #  */
0,          0,          0,          A_X,            /* $  %  &  '  */
0,          0,          0,          0,              /* (  )  *  +  */
0,          0,          0,          0,              /* ,  -  .  /  */
0,          0,          0,          0,              /* 0  1  2  3  */
0,          0,          0,          0,              /* 4  5  6  7  */
0,          0,          0,          0,              /* 8  9  :  ;  */
A_X,        0,          A_X,        0,              /* <  =  >  ?  */
A_S,        0,          A_E|A_F,    0,              /* @  A  B  C  */
0,          A_S,        A_S,        A_Q,            /* D  E  F  G  */
0,          A_T|A_E,    0,          0,              /* H  I  J  K  */
0,          A_Q,        A_T|A_F,    A_T,            /* L  M  N  O  */
0,          A_Q,        A_E,        A_T|A_F,        /* P  Q  R  S  */
0,          A_Q,        0,          A_E,            /* T  U  V  W  */
A_Q,        0,          0,          A_Q,            /* X  Y  Z  [  */
0,          A_Q,        A_S,        A_T|A_E,        /* \  ]  ^  _  */
0,          0,          A_E|A_F,    0,              /* `  a  b  c  */
0,          A_S,        A_S,        A_Q,            /* d  e  f  g  */
0,          A_T|A_E,    0,          0,              /* h  i  j  k  */
0,          A_Q,        A_T|A_F,    A_T,            /* l  m  n  o  */
0,          A_Q,        A_E,        A_T|A_F,        /* p  q  r  s  */
0,          A_Q,        0,          A_E,            /* t  u  v  w  */
A_Q,        0,          0,          0,              /* x  y  z  {  */
A_X,        0,          0,          0,              /* |  }  ~  del */
} ;
