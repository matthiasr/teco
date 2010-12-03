/* TECO for Ultrix   Copyright 1986 Matt Fichtenbaum                        */
/* This program and its components belong to GenRad Inc, Concord MA 01742   */
/* They may be copied if this copyright notice is included                  */

/* te_defs.h definitions file       5/18/87 */
/* version for multiple buffers 04/19/89 11.25                              */

#include <stdio.h>
#include <ctype.h>
#include <setjmp.h>

#define VERSION 36              /* this TECO is closest to v. 36        */
#define CTL(x) ((x) & 0x1f)   /* for control chars                    */
#define ERROR(e) longjmp(xxx, (e))
#define BLOCKSIZE (0x10000 - 8) /* size of memory block to allocate     */
#define CELLSIZE 256            /* number of characters per cell        */
#define CSTACKSIZE 64           /* size of command stack                */
#define MSTACKSIZE 64           /* size of macro stack                  */
#define QSTACKSIZE 64           /* size of q register stack             */
#define ESTACKSIZE 64           /* size of expression stack             */
#define TTIBUFSIZE 1024         /* size of type-ahead buffer            */
#define BUFF_LIMIT 16384        /* text buffer soft limit for ED & 4    */
#define NQREGS 36               /* number of Q registers                */
#define MAX_BUFFERS 2           /* maximum number of text buffers       */
#define CBUFF 0                 /* id for command buffer                */
#define SERBUF NQREGS+1         /* and search string buffer             */
#define FILBUF NQREGS+2         /* and file string buffer               */
#define SYSBUF NQREGS+3         /* and system command buffer            */
#define TIMBUF NQREGS+4         /* and time/date buffer                 */
#define cbuf qreg[CBUFF]        /* shorthand for command-string header  */
#define sbuf qreg[SERBUF]       /* and for search-buffer header         */
#define fbuf qreg[FILBUF]       /* and for file-buffer header           */
#define sysbuf qreg[SYSBUF]     /* and for system command header        */
#define timbuf qreg[TIMBUF]     /* and for time/date header             */
#define cptr (*msp)             /* command string is top of macro stack */
#define cmdstr mstack[0]        /* for entering command string          */
#define TAB 011                 /* define special chars                 */
#define LF 012
#define VT 013
#define FF 014
#define CR 015
#define ESC 033
#define DEL 0177

/* expression operators */
#define OP_START 1
#define OP_ADD  2
#define OP_SUB  3
#define OP_MULT 4
#define OP_DIV  5
#define OP_AND  6
#define OP_OR   7

/* macro flags */
#define F_ITER  1

/* bits in special-character table */
#define A_S 1                   /* "skipto()" special character     */
#define A_T 2                   /* command with std text argument   */
#define A_E 4                   /* E<char> takes a text argument    */
#define A_F 8                   /* F<char> takes a text argument    */
#define A_X 16                  /* char causes "skipto()" to exit   */
#define A_Q 32                  /* command with q-register argument */
#define A_A 64                  /* special char in search string    */
#define A_L 128                 /* character is a line separator    */

/* error codes */
#define E_BNI 1
#define E_CPQ 2
#define E_COF 3
#define E_FNF 4
#define E_IEC 5
#define E_IFC 6
#define E_IIA 7
#define E_ILL 8
#define E_ILN 9
#define E_IPA 10
#define E_IQC 11
#define E_IQN 12
#define E_IRA 13
#define E_ISA 14
#define E_ISS 15
#define E_IUC 16
#define E_MEM 17
#define E_MRP 18
#define E_NAB 19
#define E_NAC 20
#define E_NAE 21
#define E_NAP 22
#define E_NAQ 23
#define E_NAS 24
#define E_NAU 25
#define E_NFI 26
#define E_NFO 27
#define E_NYA 28
#define E_OFO 29
#define E_PDO 30
#define E_POP 31
#define E_SNI 32
#define E_SRH 33
#define E_STL 34
#define E_UTC 35
#define E_UTM 36
#define E_XAB 37
#define E_YCA 38
#define E_IWA 39
#define E_NFR 40
#define E_INT 41
#define E_EFI 42
#define E_IAA 43
#define E_AMB 44
#define E_SYS 45
#define E_IBS 46
#define E_IOS 47
#define E_FBC 48

/* define names for window control registers */
#define WN_type win_data[0]
#define WN_width win_data[1]
#define WN_height win_data[2]
#define WN_seeall win_data[3]
#define WN_mark win_data[4]
#define WN_hold win_data[5]
#define WN_origin win_data[6]
#define WN_scroll win_data[7]
#define WN_secondary win_data[8]
#define WN_sec_origin win_data[9]
#define WN_primary win_data[10]

/* define display operations */
#define WIN_OFF 0           /* disable window   */
#define WIN_SUSP 1          /* suspend window   */
#define WIN_INIT 2          /* turn on window   */
#define WIN_RESUME 3        /* re-enable window */
#define WIN_REFR 4          /* refresh window   */
#define WIN_LINE 5          /* display one line */
#define WIN_REDRAW 6        /* force window absolute redraw on next refresh */
#define WIN_DISP 7          /* refresh window even if not enabled */

/* define scope special functions */
#define VT_CLEAR 0          /* clear screen */
#define VT_EEOL 1           /* erase to eol */
#define VT_SETSPEC1 2       /* set special (reverse) video */
#define VT_SETSPEC2 3       /* alternative special video */
#define VT_CLRSPEC 4        /* clear it */
#define VT_BS1 5            /* backspace and erase 1 */
#define VT_BS2 6            /* backspace and erase 2 */
#define VT_LINEUP 7         /* up one line */
#define VT_EBOL 8           /* erase from bol */

/* define keyboard modes */
#define TTY_OFF 0           /* final "off" */
#define TTY_SUSP 1          /* temporary suspend */
#define TTY_ON 2            /* initial "on" */
#define TTY_RESUME 3        /* resume after suspend */

/* define values for ED, ET, EZ flags */
#define ED_CARET 1
#define ED_YPROT 2
#define ED_EXPMEM 4
#define ED_SFAIL 16
#define ED_SMULT 64

#define ET_IMAGE 1
#define ET_NOECHO 8
#define ET_CTRLO 16
#define ET_NOWAIT 32
#define ET_QUIT 128
#define ET_TRUNC 256
#define ET_CTRLC 32768

#define EZ_CRLF 1
#define EZ_READFF 2
#define EZ_TAB4 4
#define EZ_NOTMPFIL 8
#define EZ_NOTABI 16
#define EZ_NOVTFF 32
#define EZ_MULT 64
#define EZ_AUTOIO 128

/* define buffer cell */
/* a buffer cell is a forward pointer, a backward pointer, */
/* and CELLSIZE characters */

struct buffcell
    {
    struct buffcell *f;     /* forward pointer  */
    struct buffcell *b;     /* backward pointer */
    char ch[CELLSIZE];      /* char storage     */
    };

/* define structures for buffer header, q-register header,          */
/* q-register pointer, macro stack entry, and macro iteration list  */
/* these are really alternative ways of looking at the same cell    */

struct qh           /* q-register header */
    {
    struct buffcell *f; /* forward pointer */
    struct buffcell *b; /* backward pointer */
    int z;              /* number of characters */
    int v;              /* q-register numeric value */
    } ;

struct qp           /* q-register pointer/macro stack entry */
    {
    struct qp *f;       /* forward pointer */
    struct buffcell *p; /* pointer to a buffer cell */
    int c;              /* character offset */
    int z;              /* number of characters in object pointed to */
    int dot;            /* current character position */
    int flag;           /* flags for "iteration in process," "ei macro," etc. */
    struct is *il;      /* iteration list pointer */
    int *condsp;        /* saved conditional stack pointer */
    } ;

struct is           /* macro iteration list entry */
    {
    struct is *f;       /* forward pointer */
    struct is *b;       /* backward pointer */
    struct buffcell *p; /* cell with start of iteration */
    int c;              /* char offset where iteration started */
    int dot;            /* char position where iteration started */
    int count;          /* iteration count */
    int dflag;          /* definite iteration flag */
    } ;

struct ms           /* macro stack entry */ /* not used at present */
    {
    struct ms *f;       /* forward pointer */
    struct ms *b;       /* backward pointer */
    struct buffcell *p; /* pointer to a buffer cell */
    int c;              /* character offset */
    struct is *il;      /* pointer to iteration list */
    } ;

struct bh           /* buffer header list entry */
    {
    struct buffcell *f; /* pointer to buffer */
    struct buffcell *b; /* null pointer */
    int z;              /* number of characters */
    int dot;            /* current pointer position */
    int buff_mod;       /* buffer - modified position for display */
    } ;

/* define expression stack entry */
struct exp_entry
    {
    int val1;       /* first value                          */
    int flag1;      /* nonzero if there is a first value    */
    int val2;       /* second value (set by 'comma')        */
    int flag2;      /* nonzero if there is one              */
    int exp;        /* expression in process                */
    int op;         /* operation to be applied              */
    } ;

/* define file data structures */
struct infiledata               /* structure of input file info */
    {
    FILE *fd;                       /* file pointer */
    int eofsw;                      /* end-of-file switch */
    } ;

struct outfiledata              /* structure of output file info */
    {
    FILE *fd;                       /* file pointer */
    char f_name[CELLSIZE+5];        /* real name of output */
    char t_name[CELLSIZE+5];        /* temporary output name */
    int name_size;                  /* number of chars in name */
    int bak;                        /* backup flag */
    } ;

extern struct infiledata *infile;       /* pointer to currently active intput file structure */
extern struct outfiledata *outfile;     /* pointer to currently active output file structure */
extern struct outfiledata po_file, so_file;     /* output file descriptors */

/* define global variables, etc. */

extern int char_count;                  /* char count for tab typer         */
extern char lastc;                      /* last char read                   */
extern int ttyerr;                      /* error return from ioctl          */
extern int errno;                       /* system error code                */
extern struct sgttyb ttybuf;            /* local copy of tty control data   */
extern int inp_noterm;                  /* nonzero if standard in is not a terminal */
extern int out_noterm;                  /* nonzero if standard out is not a term.   */
extern jmp_buf xxx;                     /* preserved environment for error restart  */
extern int err;                         /* local error code                 */
extern struct qp t_qp;                  /* temporary buffer pointer         */
extern struct qp aa, bb, cc;            /* more temporaries                 */
extern struct buffcell t_bcell;         /* temporary bcell                  */
extern int tabmask;                     /* mask for selecting 4/8 char tabs */
extern int exitflag;                    /* flag for ending command str exec */
extern char term_char;                  /* terminator for insert, search, etc.  */
extern char cmdc;                       /* current command character        */
extern char skipc;                      /* char found by "skipto()"         */
extern int dot, z, tdot;                /* current, last, temp buffer position  */
extern int ll, mm, nn;                  /* general temps                    */
extern int ins_count;                   /* count of chars inserted          */
extern int ctrl_e;                      /* form feed flag                   */
extern int ctrl_r;                      /* current number radix (8, 10, 16) */
extern int ctrl_s;                      /* string length for S, I, G        */
extern int ctrl_x;                      /* search case flag                 */
extern int ed_val;                      /* ED value                         */
extern int es_val;                      /* ES value                         */
extern int et_val;                      /* ET value                         */
extern int eu_val;                      /* EU value                         */
extern int ev_val;                      /* EV value                         */
extern int ez_val;                      /* EZ value                         */
extern int srch_result;                 /* result of last :S executed       */
extern int atflag;                      /* flag for @ char typed            */
extern int colonflag;                   /* flag for : char typed            */
extern int trace_sw;                    /* nonzero if tracing command exec  */

extern int win_data[];                  /* window control parameters        */
extern struct buffcell *insert_p;       /* pointer to temp text buffer during insert */
extern int buff_mod;                    /* set to earliest buffer change    */
extern int search_flag;                 /* set nonzero by search            */

extern char *errors[];                  /* error text                       */
extern char mapch[], mapch_l[];         /* char mapping tables              */
extern unsigned char spec_chars[];      /* special character table          */

extern char skipto(), find_endcond(), getcmdc(), getcmdc0();    /* routines that return chars */

extern FILE *eisw;                      /* indirect command file pointer    */
extern FILE *fopen();

extern struct buffcell *freebuff;       /* buffcell free-list pointer   */
extern struct buffcell *dly_freebuff;   /* delayed free-list pointer    */
extern struct qp *freedcell;            /* cell free-list pointer       */
extern struct buffcell *get_bcell();    /* get buffcell routine         */
extern struct qp *get_dcell();          /* get data cell routine        */

/* the text buffer header */
extern struct qh buff;
extern struct bh buffs[], *pbuff;

/* the q-register headers point to the start of the buffer and registers */
extern struct qh qreg[];            /* for q regs, command, search, file */

/* the q-register stack contains temporary copies of q-register contents */
extern struct qh qstack[];          /* q-reg stack */
extern struct qh *qsp;              /* q-reg stack pointer */

/* the macro stack contains pointers to the currently active macros. */
/* the top of the stack is the command pointer */
extern struct qp mstack[];          /* macro stack */
extern struct qp *msp;              /* macro stack pointer */

/* the expression stack */
extern struct exp_entry estack[];   /* expression stack */
extern struct exp_entry *esp;       /* expression stack pointer */

extern int ttyspeed;		    /* tty output speed */

/* POSIX-compatibility hacks */
#ifndef _POSIX_SOURCE
#define LO(s)		((int)((s)&0377))
#define HI(s)		((int)(((s)>>8)&0377))
#define WIFEXITED(s)	(LO(s)==0)
#define WEXITSTATUS(s)	HI(s)
#define O_NONBLOCK	FNDELAY
#endif /* _POSIX_SOURCE */
