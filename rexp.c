/********************************************
rexp.c
copyright 2008-2012,2013, Thomas E. Dickey
copyright 1991-1993,1996, Michael D. Brennan

This is a source file for mawk, an implementation of
the AWK programming language.

Mawk is distributed without warranty under the terms of
the GNU General Public License, version 2, 1991.
********************************************/

/*
 * $MawkId: rexp.c,v 1.18 2013/02/19 10:51:21 tom Exp $
 * @Log: rexp.c,v @
 * Revision 1.3  1996/09/02 18:47:36  mike
 * Make ^* and ^+ syntax errors.
 *
 * Revision 1.2  1993/07/23 13:21:32  mike
 * cleanup rexp code
 *
 * Revision 1.1.1.1  1993/07/03     18:58:26  mike
 * move source to cvs
 *
 * Revision 3.4     1991/08/13  09:09:59  brennan
 * VERSION .9994
 *
 * Revision 3.3     91/08/04  15:45:03  brennan
 * no longer attempt to recover mem on failed REcompile
 * Its not worth the effort
 *
 * Revision 3.2     91/08/03  07:24:06  brennan
 * check for empty machine stack (missing operand) wasn't quite right
 *
 * Revision 3.1     91/06/07  10:33:16  brennan
 * VERSION 0.995
 *
 * Revision 1.7     91/06/05  08:58:47  brennan
 * change RE_free to free
 *
 * Revision 1.6     91/06/03  07:07:17  brennan
 * moved parser stacks inside REcompile
 * removed unnecessary copying
 *
 */

/*  op precedence  parser for regular expressions  */

#include "rexp.h"
#include "regexp.h"

/*  DATA   */
int REerrno;
const char *REerrlist[] =
{(char *) 0,
 /* 1  */ "missing '('",
 /* 2  */ "missing ')'",
 /* 3  */ "bad class -- [], [^] or [",
 /* 4  */ "missing operand",
 /* 5  */ "resource exhaustion -- regular expression too large",
 /* 6  */ "syntax error ^* or ^+",
 /* 7  */ "bad interval expression",
 /* 8  */ ""
};
/* E5 is very unlikely to occur */

/* This table drives the operator precedence parser */
/* *INDENT-OFF* */
static  short  table[10][10]  =  {
/*        0   |   CAT   *   +   ?   (   )    {    }  */
/* 0 */   {0,  L,  L,    L,  L,  L,  L,  E1, E7, L},
/* | */   {G,  G,  L,    L,  L,  L,  L,  G,  G,  G},
/* CAT*/  {G,  G,  G,    L,  L,  L,  L,  G,  L,  G},
/* * */   {G,  G,  G,    G,  G,  G,  E7, G,  G,  G},
/* + */   {G,  G,  G,    G,  G,  G,  E7, G,  G,  G},
/* ? */   {G,  G,  G,    G,  G,  G,  E7, G,  G,  G},
/* ( */   {E2, L,  L,    L,  L,  L,  L,  EQ, G,  G},
/* ) */   {G , G,  G,    G,  G,  G,  E7, G,  E7, G},
/* { */   {G,  G,  G,    G,  G,  G,  E7, G,  G,  EQ},
/* } */   {G , G,  G,    G,  G,  G,  E7, G,  E7, G}   };
/* *INDENT-ON* */

#define     STACKSZ   64

static jmp_buf err_buf;        /*  used to trap on error */

#if OPT_TRACE > 0
static const char *
token_name(int token)
{
    const char *result;
#define CASE(name) case name: result = #name; break
    switch (token) {
    CASE(T_NONE);
    CASE(T_OR);
    CASE(T_CAT);
    CASE(T_STAR);
    CASE(T_PLUS);
    CASE(T_Q);
    CASE(T_LP);
    CASE(T_RP);
    CASE(T_START);
    CASE(T_END);
    CASE(T_ANY);
    CASE(T_CLASS);
    CASE(T_SLASH);
    CASE(T_CHAR);
    CASE(T_STR);
    CASE(T_LB);
    CASE(T_RB);
    CASE(T_U);
    default:
    result = "?";
    break;
    }
#undef CASE
    return result;
}
#endif

void
RE_error_trap(int x)
{
    REerrno = x;
    longjmp(err_buf, 1);
}

PTR
REcompile(char *re, size_t len)
{
    MACHINE m_stack[STACKSZ];
    struct op {
    int token;
    int prec;
    } op_stack[STACKSZ];
    register MACHINE *m_ptr;
    register struct op *op_ptr;
    register int t;
    register int i;

    /* do this first because it also checks if we have a
       run time stack */
    RE_lex_init(re, len);

    if (len == 0) {
        STATE *p = (STATE *) RE_malloc(sizeof(STATE));
        p->s_type = M_ACCEPT;
        return (PTR) p;
    }

    if (setjmp(err_buf))
        return (PTR) 0;
    /* we used to try to recover memory left on machine stack ;
       but now m_ptr is in a register so it won't be right unless
       we force it out of a register which isn't worth the trouble */

    /* initialize the stacks  */
    m_ptr = m_stack - 1;
    op_ptr = op_stack;
    op_ptr->token = 0;

    t = RE_lex(m_stack);

    while (1) {
    TRACE(("RE_lex token %s\n", token_name(t)));
    switch (t) {
    case T_STR:
    case T_ANY:
    case T_U:
    case T_START:
    case T_END:
    case T_CLASS:
        m_ptr++;
        break;

    case T_RB:  
        /* interval expression {n,m}
         * eg, 
         *   convert m{3} to mmm
         *   convert m{3,} to mmm* (with a limit of MAX_INT)
         *   convert m{3,10} to mm* with a limit of 10
         */
        if (intrvalmin == 0) {     /* zero or more */ 
            switch (intrvalmax) {
            case 0:  
                /* user stupidity: m{0} or m{0,0} 
                * 
                * don't add this re token
                */
                if (op_ptr != op_stack) {
                    /* no previous re */
                    RE_free(m_ptr->start);
                    m_ptr--;
                    op_ptr--;
                } else if (*lp == '\0') {
                    /* this was the only re expr
                       so leave one M_ACCEPT as the machine */
                    m_ptr->start->s_type = M_ACCEPT;
                } else {
                    RE_free(m_ptr->start);
                    m_ptr--;
                }
                TRACE(("RE_lex token %s\n", "of zero interval is ignored!"));
                break;
            case 1:  
                RE_01(m_ptr);        /* m{0,1} which is m? */
                TRACE(("RE_lex token %s\n", token_name(T_Q)));
                break;
            default:
                RE_close_limit(m_ptr, intrvalmax);
                TRACE(("RE_lex token %s\n", token_name(T_Q)));
            }
        }
        else if (intrvalmin == 1) {   /* one or more */ 
            RE_poscl_limit(m_ptr, intrvalmax);
            TRACE(("RE_lex token %s\n", token_name(T_PLUS)));
        } 
        else {                /* n or more */
            /* copy 2 copies of m_ptr, use 2nd copy to replace
               the first copy that gets swallowed by concat */
            MACHINE * result_mp = m_ptr;
            MACHINE * concat_mp = (MACHINE *) (m_ptr + 1);
            MACHINE * new_mp = (MACHINE *) (m_ptr + 2);
            duplicate_m(concat_mp, result_mp);
            duplicate_m(new_mp, result_mp);
            for (i = 2; i <= intrvalmin; i++) {
                RE_cat(result_mp, concat_mp);
                duplicate_m(concat_mp, new_mp);
            }
            /* don't need 2nd copy in new_mp */
            RE_free(new_mp->start);
        }
        break;
        case 0:        /*  end of reg expr   */
        if (op_ptr->token == 0) {
        /*  done   */
        if (m_ptr == m_stack)
            return (PTR) m_ptr->start;
        else {
            /* machines still on the stack  */
            RE_panic2("values still on machine stack for ", re);
        }
        }

        /*  otherwise  fall  thru to default
           which is operator case  */

    default:

        if ((op_ptr->prec = table[op_ptr->token][t]) == G) {
        do {        /* op_pop   */

            if (op_ptr->token <= T_CAT) {    /*binary op */
            if (m_ptr == m_stack
                && op_ptr->token == T_CAT) {
                TRACE(("...ignoring empty T_CAT\n"));
                op_ptr--;
                continue;
            }
            m_ptr--;
            }
            /* if not enough values on machine stack
               then we have a missing operand */
            if (m_ptr < m_stack)
            RE_error_trap(-E4);

            switch (op_ptr->token) {
            case T_CAT:
            RE_cat(m_ptr, m_ptr + 1);
            break;

            case T_OR:
            RE_or(m_ptr, m_ptr + 1);
            break;

            case T_STAR:
            RE_close(m_ptr);
            break;

            case T_PLUS:
            RE_poscl(m_ptr);
            break;

            case T_Q:
            RE_01(m_ptr);
            break;

            default:
            /*nothing on ( or ) */
            break;
            }

            op_ptr--;
        }
        while (op_ptr->prec != L);

        continue;    /* back thru switch at top */
        }

        if (op_ptr->prec < 0) {
        if (op_ptr->prec == E7)
            RE_panic("parser returns E7");
        else
            RE_error_trap(-op_ptr->prec);
        }

        if (++op_ptr == op_stack + STACKSZ) {
        /* stack overflow */
        RE_error_trap(-E5);
        }

        op_ptr->token = t;
    }            /* end of switch */

    if (m_ptr == m_stack + (STACKSZ - 1)) {
        /*overflow */
        RE_error_trap(-E5);
    }

    t = RE_lex(m_ptr + 1);
    }
}

void
REdestroy(PTR ptr)
{
    int done = 0;
    int n = 0;
    STATE *q = (STATE *) ptr;

    TRACE(("REdestroy %p\n", ptr));
    while (!done) {
    TRACE(("...%d type %d\n", n, q->s_type));
    switch (q->s_type) {
    case M_ACCEPT:
        done = 1;
        break;
    case M_STR:
        RE_free(q->s_data.str);
        break;
    default:
        if (q->s_type < 0 || q->s_type > END_ON)
        done = -1;
        break;
    }
    ++q;
    ++n;
    }
    RE_free(ptr);
}

/* getting here means a logic flaw or unforeseen case */
void
RE_panic(const char *s)
{
    fprintf(stderr, "REcompile() - panic:  %s\n", s);
    mawk_exit(100);
}

void
RE_panic2(const char *s1, char *s2)
{
    fprintf(stderr, "REcompile() - panic:  %s %s\n", s1, s2);
    mawk_exit(100);
}

/* getting regexp error message */
const char *
REerror(void)
{
    return REerrlist[REerrno];
}

/* duplicate a machine, oldmp into newmp */
void 
duplicate_m(MACHINE *newmp, MACHINE *oldmp) {
    register STATE *p;
    p = (STATE *) RE_malloc(2 * STATESZ);
    memcpy(p, oldmp->start, 2 * STATESZ);
    newmp->start = (STATE *) p;
    newmp->stop = (STATE *) (p + 1);
}
