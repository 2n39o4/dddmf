/*
  -- MiniForth:  The evolution of a C based forth interpreter
  first written in 1982 (RForth), and re-written in 2008 to be 
  embedded on [sic] less powerful hardware.  This (third) version 
  was designed from the outset to be variable cell sized, compiled 
  from a single source file, and with the ability to run on hosted 
  operating systems or natively on the more capable SoC's on the 
  smaller end.

  Written by Robert S. Sciuk of Control-Q Research 
  Copyright 1982 - 2013.

  LICENSE:

  This code is offered into the public domain with no usage restrictions 
  under similar terms to SQLite, but with the one additional proviso that 
  MiniForth and/or its derivatives must never be encumbered in any way 
  by a more restrictive license, and most specifically no GPL license of 
  any sort (including LGPL) may be applied.  Otherwise, and to quote 
  Mr. D.R. Hipp, author of SQLite:

        "May you do good and not evil
         May you find forgiveness for yourself and forgive others
         May you share freely, never taking more than you give."

  To use, simply download this file, and use your trusty C compiler:

        clang -o mforth MiniForth.c

  Some systems might require the -ldl option, and embedded systems
  might require a bit more effort.  No documentation is available.

notes: str_token is called from 
        +) quit()
        +) word()
        +) compile()
        +) reset()


notes: error_code gets assigned in
        +) reset() err_OK -> 0
        +) err_throw(whence,err) (throw(x) <= this is a macro )


notes: 'throw()' appears in a lot of functions. 'catch()' appears in 
        1) quit() 2) execute()

notes: initial state is 'state_Interactive. Functions that assign state are
       +) reset()
       +) does()
       +) colon()
       +) compile()
       +) pvState() (previous state)
       +) imState()
       +) doColon()
       +) semicolon()

notes: I have changed chk(x) to stkchk(x)

*/

#include <stdint.h>
#include <stdarg.h>

#define NULL            (void*)0x0000
#define sz_STACK        16               /* cells */
#define sz_INBUF        80               /* bytes */
#define sz_ColonDefs    32               /* # entries */
#define sz_FLASH        (4*256)          /* cells */
#define INPUT           0
#define OUTPUT          1
#define _HALFMASK       0xffff


typedef int32_t         Wrd_t ;
typedef uint32_t        uWrd_t ;
typedef int16_t         Hlf_t ;
typedef uint16_t        uHlf_t ;
typedef void            (*Fptr_t)() ;
typedef Wrd_t           (*Cptr_t)() ;
typedef char *          Str_t ;
typedef int8_t          Byt_t ;
typedef uint8_t         uByt_t ;
typedef void            Nul_t ;
typedef void *          Opq_t ;
typedef Wrd_t           Cell_t ;
typedef uWrd_t          uCell_t ;

//
//  -- useful macros ...
//
#define v_Off           0
#define v_On            1
#define push( x )       *(++tos) = x 
#define pop()           *(tos--)
#define nos             tos[-1]
#define rpush( x )      *(++rtos) = x
#define rpop()          *(rtos--)
#define rnos            rtos[-1]
#define StartOf(x)      (&x[0])
#define isNul( x )      (x == NULL)
#define WHITE_SPACE     " \t\r\n"
#define inEOF           "<eof>"
#define MaxStr( x, y )  ((str_length( x ) > str_length( y )) ? str_length( x ) : str_length( y ))
#define isMatch( x, y ) (str_match( x, y, MaxStr( x, y )))
#define fmt( x, ... )   str_format( (Str_t) StartOf( tmp_buffer ), (Wrd_t) sz_INBUF, x, ## __VA_ARGS__ )
#define __THIS__        ( (Str_t) __FUNCTION__ )
//#define throw( x )    err_throw( str_error( err_buffer, sz_INBUF, __THIS__, __LINE__), x )
#define throw( x )      err_throw( str_error( (Str_t)err_buffer, sz_INBUF, __THIS__, __LINE__), x )


#ifdef NOCHECK
#define chk( x )        {}
#define dbg             ' '
#else



#define chk( x ) \
  do { int xxx; \
  xxx = checkstack(x,(Str_t) __func__ ); \
  if(xxx==1) BAIL=0; \
  else BAIL=1; \
  } while(0); \
  if(BAIL) return



#define stkchk(x) \
  { int d; char* func;	    \
  func = __func__; \
  d = tos - stack; \
  if(d < x){ uC_printf("\n\rstkchk:depth=%d function=%s",d,func); return; } \
  }



#define dbg             'D'
#endif


// CAN'T DO THIS !!! chk needs to be a macro
// void chk(int x){        do { if( !checkstack( x, (Str_t) __func__ ) ) return ; } while(0)




//
//  -- specify output uart
//     The lm3s6965 has 3 uarts (0,1,2). The function init_uC() initializes the uart
//     see init-uC.c
//
//static int output = 1;
static int output = 0;
static int input = 0;
static int console = 0;
//
//  -- a stack ...
//
Cell_t stack[sz_STACK] ;
Cell_t *tos = stack;

//
//  -- and a return stack ...
//
Cell_t rstack[sz_STACK] ;
Cell_t *rtos = rstack;

//
//  -- an input and scratch buffers ...
//
char  input_buffer[sz_INBUF] ;
Str_t  inbuf = (Str_t) StartOf( input_buffer ) ;

char  scratch_buffer[sz_INBUF] ;
Str_t  scratch = (Str_t)scratch_buffer;

char  err_buffer[sz_INBUF] ;
char  tmp_buffer[sz_INBUF] ;
Str_t  tmp  = (Str_t) StartOf( tmp_buffer ) ;

Wrd_t checkstack( Wrd_t n, Str_t fun );

//
//  -- forth primitives must be pre-declared ...
//
void quit(void); 
void banner(void); 
void add(void); 
void subt(void); 
void mult(void); 
void exponent(void); 
void divide(void); 
void modulo(void); 
void dotS(void); 
void dot(void); 
void udot(void); 
void prompt(void); 
void words(void); 
void depth(void); 
void dupe(void); 
void drop(void); 
void over(void); 
void swap(void); 
void toR(void); 
void Rto(void); 
//void Eof(void); 
void cells(void); 
void cellsize(void); 
void wrd_fetch(void); 
void wrd_store(void); 
void reg_fetch(void); 
void reg_store(void); 
void crg_fetch(void); 
void crg_store(void); 
void hlf_fetch(void); 
void hlf_store(void); 
void byt_fetch(void); 
void byt_store(void); 
void lft_shift(void);
void rgt_shift(void);
void cmove(void);
void word(void);
void ascii(void);
void q_key(void);
static void key(void);
static void emit(void);
void type(void);
void cr(void);
void dp(void);
void here(void);
void freespace(void);
void comma(void);
void doLiteral(void);
void colon(void);
void compile(void);
void semicolon(void);
void call(void) ;
void execute(void) ;
void doColon(void) ;
void tick(void) ;
void nfa(void) ;
void cfa(void) ;
void pfa(void) ;
void decimal(void) ;
void hex(void) ;
//void sigvar(void) ;
void errvar(void) ;
void base(void) ;
void trace(void) ;
//void resetter(void) ;
void see(void) ;
void pushPfa(void) ;
void does(void) ;
void allot(void) ;
void create(void) ;
void constant(void) ;
void pvState(void) ;
void imState(void) ;
void normal(void) ;
void immediate(void) ;
void unresolved(void);
void fwd_mark(void);
void fwd_resolve(void);
void bkw_mark(void);
void bkw_resolve(void);
void q_branch(void);
void branch(void);
void begin(void);
void again(void);
void While(void);
void Repeat(void);
void Leave(void);
void Until(void);
void If(void);
void Else(void);
void Then(void);
void lt(void);
void gt(void);
void ge(void);
void le(void);
void eq(void);
void ne(void);
void And(void);
void and(void);
void or(void);
void xor(void);
void not(void);
void SCratch(void);
void Tmp(void);
void pad(void);
void comment(void);
void dotcomment(void);
void quote(void);
void dotquote(void);
void count(void);
void ssave(void);
void unssave(void);
//void infile(void);
//void outfile(void);
//void sndtty(void);
//void rcvtty(void);
//void opentty(void);
//void closetty(void);
void Memset(void);
//void waitrdy(void);
//#ifdef HOSTED
//void qdlopen(void);
//void qdlclose(void);
//void qdlsym(void);
//void qdlerror(void);
//#endif /* HOSTED */
//void last_will(void);
void spinner(void);
void callout(void);

void doth8(void);
void doth16(void);
void doth32(void);
void dumpi(void);
void fd(void); // flash dictionary
void rd(void); // ram dictionary

void finfo(void);
void variable(void);
void For(void);
void Next(void);

//void oneplus(void);
//
// 1+ ( n -- n+1 )
void oneplus(void){
  stkchk(1);
  (*tos)++;
}

void oneminus(void){
  stkchk(1);
  (*tos)--;
}

//void uptime(void);

void uptime(void){push(uC_get_system_seconds()); }

void sysloghelp(void){ syslog_help(); }

extern void ResetISR(void);

void reboot(void){ ResetISR(); }


//
//  -- dictionary is simply an array of struct ...
//

typedef enum {
  Normal,
  Immediate,
  Undefined
} Flg_t ;

typedef struct _dict_ {
  Fptr_t  cfa ;
  Str_t   nfa ;
  Flg_t   flg ;
  Cell_t  *pfa ;
} Dict_t ;


// the 'const' forces the array into flash.  Without 'const' the arry is put into SRAM
const Dict_t Primitives[] = {
  { reboot,     "reboot", Normal, NULL },
  { sysloghelp, "sysloghelp", Normal, NULL },
  { uptime,     "uptime", Normal, NULL },
  { quit,       "quit", Normal, NULL },
  { banner,     "banner", Normal, NULL },
  { add,        "+", Normal, NULL },
  { oneplus,    "1+", Normal, NULL },
  { oneminus,   "1-", Normal, NULL },
  { subt,       "-", Normal, NULL },
  { mult,       "*", Normal, NULL },
  { exponent,   "^", Normal, NULL },
  { divide,     "/", Normal, NULL },
  { modulo,     "%", Normal, NULL },
  { dotS,       ".s", Normal, NULL },
  { dot,        ".", Normal, NULL },


  { doth8,       ".h8", Normal, NULL },
  { doth16,      ".h16", Normal, NULL },
  { doth32,      ".h32", Normal, NULL },
  { dumpi, "dumpi",Normal, NULL },
  { fd, "fd",Normal, NULL },
  { rd, "rd",Normal, NULL },
  { finfo, "finfo", Normal, NULL },




  { udot,       "u.", Normal, NULL },
  //  { exit,   "bye", Normal, NULL },
  { words,      "words", Normal, NULL },
  { depth,      "depth", Normal, NULL },
  { dupe,       "dup", Normal, NULL },
  { drop,       "drop", Normal, NULL },
  { over,       "over", Normal, NULL },
  { swap,       "swap", Normal, NULL },
  { toR,        ">r", Normal, NULL },
  { Rto,        "r>", Normal, NULL },
  //  { Eof,    inEOF, Normal, NULL },
  { cells,      "cells", Normal, NULL },
  { cellsize,   "cellsize", Normal, NULL },
  { wrd_fetch,  "@", Normal, NULL },
  { wrd_store,  "!", Normal, NULL },
  { reg_fetch,  "r@", Normal, NULL },
  { reg_store,  "r!", Normal, NULL },
  { crg_fetch,  "cr@", Normal, NULL },
  { crg_store,  "cr!", Normal, NULL },
  { hlf_fetch,  "h@", Normal, NULL },
  { hlf_store,  "h!", Normal, NULL },
  { byt_fetch,  "c@", Normal, NULL },
  { byt_store,  "c!", Normal, NULL },
  { lft_shift,  "<<", Normal, NULL },
  { rgt_shift,  ">>", Normal, NULL },
  { cmove,      "cmove", Normal, NULL },
  { word,       "word", Normal, NULL },
  { ascii,      "ascii", Normal, NULL },
  { q_key,      "?key", Normal, NULL },
  { key,        "key", Normal, NULL },
  { emit,       "emit", Normal, NULL },
  { type,       "type", Normal, NULL },
  { cr,         "cr", Normal, NULL },
  { dp,         "dp", Normal, NULL },
  { here,       "here", Normal, NULL },
  { freespace,  "freespace", Normal, NULL },
  { comma,      ",", Normal, NULL },
  { doLiteral,  "(literal)", Normal, NULL },
  { colon,      ":", Normal, NULL },
  { semicolon,  ";", Normal, NULL },
  { execute,    "execute", Normal, NULL },
  { call,       "call", Normal, NULL },
  { doColon,    "(colon)", Normal, NULL },
  { tick,       "'", Normal, NULL },
  { nfa,        ">name", Normal, NULL },
  { cfa,        ">code", Normal, NULL },
  { pfa,        ">body", Normal, NULL },
  { decimal,    "decimal", Normal, NULL },
  { hex,        "hex", Normal, NULL },
  { base,       "base", Normal, NULL },
  { trace,      "trace", Normal, NULL },
  //  { sigvar, "sigval", Normal, NULL },
  { errvar,     "errval", Normal, NULL },
  //  { resetter,       "reset", Normal, NULL },
  { see,        "see", Normal, NULL },

  { pushPfa,    "(variable)", Normal, NULL },

  { variable, "variable", Normal, NULL },

  { allot,      "allot", Normal, NULL },
  { create,     "create", Normal, NULL },
  { does,       "does>", Normal, NULL },
  { constant,   "constant", Normal, NULL },
  { normal,     "normal", Normal, NULL },
  { immediate,  "immediate", Normal, NULL },
  { imState,    "[", Immediate, NULL },
  { pvState,    "]", Immediate, NULL },
  { unresolved, "unresolved", Normal, NULL },
  { fwd_mark,   ">mark", Normal, NULL },
  { fwd_resolve,">resolve", Normal, NULL },
  { bkw_mark,   "<mark", Normal, NULL },
  { bkw_resolve,"<resolve", Normal, NULL },
  { q_branch,   "?branch", Normal, NULL },
  { branch,     "branch", Normal, NULL },
  { begin,      "begin", Immediate, NULL },
  { again,      "again", Immediate, NULL },
  { While,      "while", Immediate, NULL },
  { Repeat,     "repeat", Immediate, NULL },

  { For, "for", Immediate, NULL },
  { Next, "next", Immediate, NULL },

  { Until,      "until", Immediate, NULL },
  { Leave,      "leave", Normal, NULL },
  { If,         "if", Immediate, NULL },
  { Else,       "else", Immediate, NULL },
  { Then,       "then", Immediate, NULL },
  { lt,         "<", Normal, NULL },
  { gt,         ">", Normal, NULL },
  { ge,         ">=", Normal, NULL },
  { le,         "<=", Normal, NULL },
  { eq,         "==", Normal, NULL },
  { ne,         "!=", Normal, NULL },
  { And,        "&", Normal, NULL },
  { and,        "and", Normal, NULL },
  { or,         "or", Normal, NULL },
  { xor,        "xor", Normal, NULL },
  { not,        "not", Normal, NULL },
  { pad,        "pad", Normal, NULL },
  { comment,    "(", Immediate, NULL },
  { dotcomment, ".(", Immediate, NULL },
  { quote,      "\"", Immediate, NULL },
  { dotquote,   ".\"", Immediate, NULL },
  { count,      "count", Normal, NULL },
  { ssave,      "save", Normal, NULL },
  { unssave,    "unsave", Normal, NULL },
  //  { infile, "infile", Normal, NULL },
  //  { outfile,        "outfile", Normal, NULL },
  //  { opentty,        "opentty", Normal, NULL },
  //  { closetty,       "closetty", Normal, NULL },
  //  { sndtty, "sndtty", Normal, NULL },
  //  { rcvtty, "rcvtty", Normal, NULL },
  { Memset,     "memset", Normal, NULL },
  //  { waitrdy,        "waitrdy", Normal, NULL },
  //  { last_will,      "atexit", Normal, NULL },
  { spinner,    "spin", Normal, NULL },
  { callout,    "native", Normal, NULL },
  { NULL,       NULL, 0, NULL }
} ;

Dict_t Colon_Defs[sz_ColonDefs] ;
Cell_t n_ColonDefs = 0 ;
Cell_t flash[sz_FLASH] ;
Cell_t *Here = StartOf( flash ) ;
Cell_t *DictPtr = StartOf( flash ) ;
Cell_t  Base = 10 ;
Cell_t  Trace = 0 ;
char  *String_Data = (char *) (&flash[sz_FLASH] - 1) ;

typedef enum {
 state_Interactive,
 state_Compiling,
 state_Interpret,
 state_Immediate,
 state_Undefined
} State_t ;

State_t state = state_Interactive ;
State_t state_save = state_Interactive ;

//
// -- error codes and strings
//
typedef enum {
  err_OK = 0,
  err_StackOvr,
  err_StackUdr,
  err_DivZero,
  err_NoInput,
  err_BadBase,
  err_BadLiteral,
  err_BufOvr,
  err_NullPtr,
  err_NoSpace,
  err_BadState,
  err_CaughtSignal,
  err_UnResolved,
  err_Unsave,
  err_NoWord,
  err_TknSize,
  err_SysCall,
  err_BadString,
  err_Undefined
} Err_t ;

Str_t errors[] = {
  "-- Not an error.",
  "-- Stack overflow.",
  "-- Stack underflow.",
  "-- Division by zero.",
  "-- No more input.",
  "-- Radix is out of range.",
  "-- Bad literal conversion.",
  "-- Buffer overflow.",
  "-- NULL pointer.",
  "-- Dictionary space exhausted.",
  "-- Bad state.",
  "-- Caught a signal.",
  "-- Unresolved branch.",
  "-- Too late to un-save.",
  "-- No such word exists.",
  "-- Tkn too large.",
  "-- System call glitch.",
  "-- Bad String.",
  "-- Undefined error.",
  NULL,
} ;

Str_t resetfrom[] = {
  "unexpected",
  "sig_hdlr",
  "catch",
  "application",
  "checkstack",
  NULL
} ;

Wrd_t promptVal ;

Str_t promptStr[] = {
  "ok ",
  "-- ",
  NULL
} ;

Str_t error_loc = (Str_t) NULL ;
Err_t error_code = 0 ;

static int kludge = 0;



void catch(void) ;
void err_throw( Str_t w, Err_t e ) ;
Wrd_t put_str( Str_t s );
Wrd_t getstr( Wrd_t fd, Str_t buf, Wrd_t len );
Wrd_t inp( Wrd_t fd, Str_t buf, Wrd_t len );
Wrd_t outp( Wrd_t fd, Str_t buf, Wrd_t len );
Str_t str_error( Str_t buf, Wrd_t len, Str_t fn, Wrd_t lin );
Wrd_t str_match( Str_t a, Str_t b, Wrd_t len );
Wrd_t str_length( Str_t str );
Wrd_t str_literal( Str_t tkn, Wrd_t radix );
Wrd_t str_format( Str_t dst, Wrd_t dlen, Str_t fmt, ... );
void str_set( Str_t dst, char dat, Wrd_t len );
Wrd_t str_copy( Str_t dst, Str_t src, Wrd_t len );
Wrd_t str_utoa( uByt_t *dst, Wrd_t dlen, Cell_t val, Wrd_t radix );
Wrd_t str_ntoa( Str_t dst, Wrd_t dlen, Cell_t val, Wrd_t radix, Wrd_t isSigned );
Str_t str_token( Str_t buf, char len );
Str_t str_delimited( Str_t term ) ;
Str_t str_cache( Str_t tag );
Str_t str_uncache( Str_t tag );
Wrd_t ch_matches( char ch, Str_t anyOf );
char ch_tolower( char b );
Wrd_t ch_index( Str_t str, char c );
void sig_hdlr( int sig );
Wrd_t io_cbreak( int fd );


// fd ( -- addr) 
void fd(void){ push((int)Primitives); }
void rd(void){ push((int)Colon_Defs); }

char* str_tokeniiii(char* inbuf);


// When reset() is called we do the following
//  +) set base to decimal
//  +) reset parameter stack
//  +) reset return stack
//  +) set error_code to 'err_OK' (zero)
//  +) set state to 'state_Interactive' (also zero, see above)
//  +) reset the token parser
//
//
//
void reset(void){
  promptVal = 0 ; 
  decimal() ;
  tos = stack; 
  rtos = rstack;
  error_code = err_OK ;
  str_token( NULL, -1 ) ;
  str_tokeniiii(NULL);
  state = state_Interactive ;

  kludge = 1;

}

//
//  -- innards of the `machine'.
//
void dddmf_main(void){
  reset() ;
  banner() ;
  kludge = 0;
  quit() ;
}


Dict_t *lookup( Str_t tkn );


void f_eval(char* s){
  char* tkn ; // Str_t is char* !!!
  Dict_t *dp ;
  str_tokeniiii(NULL);
  while(1){
    tkn = str_tokeniiii(s);
    if(tkn == NULL) return;
    dp = lookup( tkn );
    if( isNul( dp ) ){
      push( str_literal( tkn, Base ) ) ;
      catch() ;
    } else {
      push( (Cell_t) dp ) ; 
      execute() ;
    }
  } /* tkn */
}

#if 0
int main(void){
  reset();
  f_eval("words"); // this works
  //  f_eval(": bob 1 2 3 .s ;"); // !!!!!! this DOES NOT WORK !!!!!!
  dddmf_main();
  return 0;
}
#endif

// answer 1 if ch matches any character in 'anyOf'
Wrd_t ch_matches( char ch, Str_t anyOf ){
  Str_t  p ;

  p = (Str_t) StartOf( anyOf ) ;
  while( *p ){
    if( ch == *(p++) ){
      return 1 ;
    }
  }
  return 0 ; 
}

char ch_tolower( char b ){
 if( b <= 'Z' && b >= 'A' ){
  return b ^ 0x20 ;
 }
 return b & 0xFF ;
}

Wrd_t ch_index( Str_t str, char c ){
  char *p, *start ; 

  start = (char *) StartOf( str ) ;
  p = start ;
  while( *p ){
    if( *p == c ){
      return p - start ;
    }
    p++ ;
  }
  return -1;
}
//
//
//
Str_t str_token( Str_t buf, char len ){
  static int ch = -1 ;
  static int nr = -1 ; 
  int tkn ;

  inbuf[0] = (char) 0 ;
  if( isNul( buf ) || len < 0 ) { /* reset requested */
    ch = nr = -1 ;
    return (Str_t) NULL ; 
  }

  tkn = 0 ;
  do {
    if( nr < 1 ){
      prompt() ;
      nr = inp( input, (Str_t) inbuf, sz_INBUF ) ;
      if( nr == 0 )
        return inEOF ;
      ch = 0 ;
    }

    if( ch > (nr-1) ){
      ch = nr = -1 ;
      continue ;
    }

    if( !ch_matches( inbuf[ch++], WHITE_SPACE ) ){
      buf[tkn++] = inbuf[ch-1] ; 
      buf[tkn] = (char) 0 ; 
      continue ;
    }

    if( tkn > 0 ){
      return buf ;
    }

  } while( 1 ) ;
  return (Str_t) NULL ; 
}

//Str_t str_tokenii( Str_t buf, char len ){
//  static int ch = -1 ;
//  static int nr = -1 ; 
//  int tkn ;
//
//  //  inbuf[0] = (char) 0 ; 
//
////  if( isNul( buf ) || len < 0 ) { /* reset requested */
////    ch = nr = -1 ;
////    return (Str_t) NULL ; 
////  }
//
//  nr = len;
//  ch = 0;
//
//  tkn = 0 ;
//
//  do {
//
////    if( nr < 1 ){
////      prompt() ;
////      nr = inp( INPUT, (Str_t) inbuf, sz_INBUF ) ;
////      if( nr == 0 )
////        return inEOF ;
////      ch = 0 ;
////    }
//
//    if( ch > (nr-1) ){
//      ch = nr = -1 ;
//      continue ;
//    }
//
//    if( !ch_matches( inbuf[ch++], WHITE_SPACE ) ){
//      buf[tkn++] = inbuf[ch-1] ; 
//      buf[tkn] = (char) 0 ; 
//      continue ;
//    }
//
//    if( tkn > 0 ){
//      return buf ;
//    }
//
//  } while( 1 ) ;
//  return (Str_t) NULL ; 
//}

//char* str_tokeniii(char* buf, int len){
//  int i;
//  for(i=0; inbuf[i]; i++){
//    if(!ch_matches(inbuf[i], WHITE_SPACE)){
//      buf[i] = inbuf[i];
//    }
//    else{
//      buf[i] = 0;
//      return buf;
//    }
//  }
//  buf[i] = 0;
//  return buf;
//}

char* str_tokeniiii(char* inbuf){
  static int nxt; // next char
  static int new; // new char
  static char tknbuf[24];


  if(inbuf == NULL){ nxt = 0; return NULL; } // reset
  
  if(inbuf[0] == 0) return NULL;

  while(1){
    if(ch_matches(inbuf[nxt],WHITE_SPACE)) nxt++;
    else{
      new = 0;
      tknbuf[new] = 0;
      while(1){
        if(inbuf[nxt] == 0) goto done;
        else{
          if(!ch_matches(inbuf[nxt],WHITE_SPACE)){
            tknbuf[new++] = inbuf[nxt++];
            tknbuf[new] = 0;
          }
          else goto done;
        }
      }
    }
  }
 done:{ if(new == 0) return NULL; else return tknbuf; }
}





    
Str_t str_error( Str_t buf, Wrd_t len, Str_t fn, Wrd_t lin ){
  str_format( buf, len, "%s():[%d]", fn, lin ) ;
  return buf ; 
}

Wrd_t str_match( Str_t a, Str_t b, Wrd_t len ){
  int8_t i ;

  if( (str_length( a ) == len) && (str_length( b ) == len) ){
    for( i = 0 ; i < len ; i++ ){
      if( a[i] != b[i] ){
        return 0 ;
      }
    }
    return 1 ;
  }
  return 0 ;
}

Wrd_t str_length( Str_t str ){
  Str_t  p ; 
  Wrd_t ret = 0 ;

  if( isNul( str ) ){
    return 0 ;
  }

  p = str ;
  while( *p++ ) ret++ ;
  return ret ; 
}

Wrd_t str_literal( Str_t tkn, Wrd_t radix ){
  Str_t digits = "0123456789abcdefghijklmnopqrstuvwxyz" ;
  //  Wrd_t  ret, len, sign, digit, base ;
  Wrd_t  ret, sign, digit, base ;
  Str_t p ;

  if( radix > str_length( digits ) ){
    put_str( tkn ) ;
    //throw( err_BadBase ) ;
    err_throw( str_error( (Str_t)err_buffer, sz_INBUF, __THIS__, __LINE__), err_BadBase );
    return -1 ;
  }

  sign = 1 ;
  base = radix ;
  p = tkn ; 
  switch( *p++ ){
    case '-': /* negative */
     sign = -1 ;
     break ;
    case '+': /* positive */
     sign = 1 ;
     break ;
    case '$': /* hex constant */
     base = 16 ;
     break ;
    case '0': /* octal or hex constant */
     base = 8 ;
     if( *p == 'x' || *p == 'X' ){
      base = 16 ;
      p++ ;
     }
     break ;
    case 'O':
    case 'o': /* octal constant */
     base = 8 ;
     break ;
    default: /* none of the above ... start over */
     p = tkn ;
     break ;
   }

   ret = 0 ; 
   while( *p ){
     digit = ch_index( digits, ch_tolower( *p++ ) ) ;
     if( digit < 0 || digit > (base - 1) ){
       put_str( tkn ) ;
       throw( err_BadLiteral ) ;
       return -1 ;
     }
     ret *= base ; 
     ret += digit ;
   }
   ret *= sign ;
   return ret ;
}

void str_set( Str_t dst, char dat, Wrd_t len ){
  //  Wrd_t i ;
  Str_t ptr ;

  for( ptr = dst ; ptr - dst < len ; ptr++ ){
    *ptr = (char) dat & 0xff ;
  } 
}

Wrd_t str_copy( Str_t dst, Str_t src, Wrd_t len ){
  Wrd_t i ;
  Str_t from, to ; 

  to = dst ;
  from = src ;
  for( i = 0 ; i < len ; i++ ){
    *to++ = *from++ ;
  }
  return i ;
}

Wrd_t str_utoa( uByt_t *dst, Wrd_t dlen, Cell_t val, Wrd_t radix ){

  uCell_t n, i, dig ;
  uByt_t *p, *q, buf[30] ;

  i = 0 ; 
  n = val ;
  do{
    dig = (n % radix) + '0' ;
    dig = (dig > '9') ? dig - '9' + 'a' - 1 : dig ; 
    buf[i++] = dig ;
    n /= radix ;
  } while( n != 0 ) ;
  buf[i] = (char) 0 ; 

  n = 0 ; 
  p = dst ;
  q = &buf[i] ;
  do {
    if( n > dlen ){
      throw( err_BufOvr ) ;
      return -1 ;
    }
    *p++ = *q-- ; n++ ;
  } while( q >= &buf[0] ); 
  *p++ = (char) 0 ; 

  return n ;
}

Wrd_t str_ntoa( Str_t dst, Wrd_t dlen, Cell_t val, Wrd_t radix, Wrd_t isSigned ){
  Wrd_t i, sign, n ;
  char c, buf[30] ;
  Str_t p ;

  n = val ;
  sign = (char) 0 ;  
  if( isSigned && val < 0 ){
    sign = '-' ;
    n = -1 * n ;
  }

  i = 0 ; 
  do {
   c = '0' + (n % radix) ;
   //ddd   c = (c > '9') ? c - '9' + 'a' - 1 : c ; 
   c = (c > '9') ? c - '9' + 'A' - 1 : c ; 
   buf[i++] = c ;
   n /= radix ;
  } while( n != 0 ) ;

  buf[ i ] = ' ' ;
  if( sign ){
    buf[i] = sign ;
  } 

  if( i > dlen ){
    throw( err_BufOvr ) ;
    return -1 ;
  }

  if( !sign ){
    i-- ;
  } 
  n = i + 1 ;
  p = dst ;
  do {
    *p++ = buf[i--] ;
  } while( i > -1 ) ;
  *p++ = (char) 0 ; 
  return n ;
} 
// %% %c %s %l %d %x %o %u
Wrd_t str_format( Str_t dst, Wrd_t dlen, Str_t fmt, ... ){
  va_list ap;
  Str_t p_fmt, p_dst, str ;
  char ch ;
  Wrd_t cell; //, n ;

  p_dst = dst ;
  p_fmt = fmt ;
  va_start( ap, fmt );
   while( (ch = *(p_fmt++)) ){
     if( ch == '%' ){
       ch = *(p_fmt++) ;
       switch( ch ){
        case '%':
          *p_dst++ = ch & 0xff ;
          break ;
        case 'c':
          ch = va_arg( ap, int );
          *p_dst++ = ch & 0xff ;
          break ;
        case 's':
          str = va_arg( ap, Str_t );
          p_dst += str_copy( p_dst, str, str_length( str ) ) ;
          break ;
        case 'l':
          ch = *(p_fmt++) ;
        case 'd':
          cell = va_arg( ap, Cell_t ) ;
          p_dst += str_ntoa( p_dst, dlen - (p_dst - dst) - 1, cell, Base, 1 ) ;
          break ;
        case 'x':
          cell = va_arg( ap, Cell_t ) ;
          p_dst += str_ntoa( p_dst, dlen - (p_dst - dst) - 1, cell, 16, 0 ) ;
          break ;
        case 'o':
          cell = va_arg( ap, Cell_t ) ;
          p_dst += str_ntoa( p_dst, dlen - (p_dst - dst) - 1, cell, 8, 0 ) ;
          break ;
        case 'u':
          cell = va_arg( ap, uCell_t ) ;
          p_dst += str_utoa( (uByt_t *) p_dst, dlen - (p_dst - dst) - 1, (uCell_t) cell, Base ) ;
          break ;
        default:
          break ;
       }
    } else {
      *p_dst++ = ch ;
    }
  }
  va_end( ap ) ;
  *p_dst++ = (char) 0 ;
  return p_dst - dst ;
}

Str_t str_uncache( Str_t tag ){
  //  Str_t ret ;
  Cell_t len ;

  len = str_length( tag ) + 1 ;
  String_Data += len ;
  return (Str_t) String_Data ;
}

Str_t str_cache( Str_t tag ){
  Cell_t len ;

  if( isNul( tag ) ){
    return NULL ;
  }

  len = str_length( tag ) + 1;
  String_Data -= len ;
  str_copy( (Str_t) String_Data, tag, len ) ;
  return (Str_t) String_Data ;
}

Dict_t *lookup(Str_t tkn){
  Dict_t *p ;
  Cell_t  i ;

  if(tkn==0) return (Dict_t *)NULL ;

  // First we search the 'Primitives' table
  p = Primitives;
  while(p->nfa){
   if( isMatch(tkn, p->nfa)){
     return p ; // found a match
   }
   p++ ;
  }

  // Next we search the 'ColonDefs' table
  // Notice we search the array from highest index to ZERO
  for(i=n_ColonDefs-1; i>-1; i--){
    p = &Colon_Defs[i] ;
    if( isMatch(tkn, p->nfa)){
      return p ; // found a match
    }
  }
  return (Dict_t *)NULL ; // shit out of luck 
}

// 
// -- Forth primitives ...
//    for visibility within the interpreter, they
//    must be pre-declared, and placed in the Primitive[]
//    dictionary structure above ...
//

static char tib[80];


int read(int,char*,int);

void accept(void){
  int i;
  for(i=0; i<80; i++) tib[i] = 0; // clear tib
  uC_fgets(tib,80,console);
}

static char* newprompt = "OK> ";
static char* newprompt2 = "----";

// OKx[0]>
// OK_[0]>
// OK-[0]>

void new_prompt(void){
  int b;
  int d;

  d = tos - stack  ;

  if(Base == 10) b = '_'; else
    if(Base == 16) b = 'x'; else
      b = '?';
  uC_printf("\n\rOK%c[%d]> ",b,d);
}


//  outp(output,"\n\r",2);
//  outp(output,newprompt,4);
//}/

void new_prompt2(void){
  outp(output,"\n\r",2);
  outp(output,newprompt2,4);
}

//#include <setjmp.h>

//jmp_buf env ;

void quit(){
  Dict_t *dp ;

  Wrd_t beenhere, n ; // Wrd_t is int32 !!!
  char* token;

  // beenhere = setjmp(env); // !!! I don't know what setjmp does !!!
  // since quit is a forth word and can be called from the interpreter, the following code gets called
  // only once as long as `beenhere' is less than 0 (or is zero) !!!!!
  beenhere = 0;
  if( beenhere > 0 ){
    catch();
    n = fmt( "-- Reset by %s.\n", resetfrom[beenhere] ) ;
    outp( output, (Str_t) StartOf( tmp_buffer ), n ) ;
  }
  for(;;){

    new_prompt();
 
    accept();

    str_tokeniiii(NULL);

    while((token = str_tokeniiii(tib))){


      //      n = fmt( "\n\r[dp=0x%x token=%s] ", dp,token);

      //      outp( output, (Str_t) StartOf( tmp_buffer ), n ) ;


      dp = lookup(token);
      if( isNul( dp ) ){
        push( str_literal( token, Base ) ) ;
        catch() ;

	if(kludge){ token = NULL; kludge = 0; break; }

      } else {
        push( (Cell_t) dp ) ; 
        execute() ;
      }
    }
  }
}


#if 0      
    while( (tkn = str_token( scratch, sz_INBUF )) ){
      dp = lookup( tkn );
      if( isNul( dp ) ){
        push( str_literal( tkn, Base ) ) ;
        catch() ;
      } else {
        push( (Cell_t) dp ) ; 
        execute() ;
      }
    } /* tkn */
  } /* ever */
} /* quit */
#endif


void finfo(void){
  int n,x;
  // ------------------------------------------------
  n = fmt("-- flash=0x%x[%d] sizeof(flash)=%d\n\r",&flash, sz_FLASH, sizeof(flash) );
  outp( output, (Str_t) StartOf( tmp_buffer ), n ) ;
  // ------------------------------------------------
  n = fmt("-- Colon_Defs=0x%x[%d] sizeof(Colon_Defs)=%d\n\r",&Colon_Defs, sz_ColonDefs, sizeof(Colon_Defs));
  outp( output, (Str_t) StartOf( tmp_buffer ), n ) ;
  // ------------------------------------------------
  x = sizeof(Primitives)/sizeof(Dict_t);
  n = fmt("-- Primitives=0x%x[%d] sizeof(Primitives)=%d numberOfPrimitives=%d\n\r",&Primitives, x, sizeof(Primitives),x-1);
  outp( output, (Str_t) StartOf( tmp_buffer ), n ) ;
  // ------------------------------------------------
  n = fmt("-- parameter stack=0x%x[%d]\n\r",stack, sz_STACK);
  outp( output, (Str_t) StartOf( tmp_buffer ), n ) ;
  n = fmt("-- return stack=0x%x[%d]\n\r",rstack, sz_STACK);
  outp( output, (Str_t) StartOf( tmp_buffer ), n ) ;
  n = fmt("-- input_buffer=0x%x[%d]\n\r",input_buffer, sz_INBUF);
  outp( output, (Str_t) StartOf( tmp_buffer ), n ) ;
  n = fmt("-- scratch_buffer=0x%x[%d]\n\r",scratch_buffer, sz_INBUF);
  outp( output, (Str_t) StartOf( tmp_buffer ), n ) ;
  n = fmt("-- err_buffer=0x%x[%d]\n\r",err_buffer, sz_INBUF);
  outp( output, (Str_t) StartOf( tmp_buffer ), n ) ;
  n = fmt("-- tmp_buffer=0x%x[%d]\n\r",tmp_buffer, sz_INBUF);
  outp( output, (Str_t) StartOf( tmp_buffer ), n ) ;


  // ------------------------------------------------
  // ------------------------------------------------
  // ------------------------------------------------
  // ------------------------------------------------
}

static char* version = "-- dddmf version 1.0.0_rc1\n\r";

void banner(){
  Wrd_t n ;
  //  int x;
  n = fmt(version);
  outp( output, "\n\r", 2);
  outp( output, (Str_t) StartOf( tmp_buffer ), n ) ;
  finfo();
}

void prompt(){
  if( INPUT == 0 ){
    outp( output, (Str_t) promptStr[promptVal], 3 ) ;
  }
}


void add(){
  register Cell_t n ;

  stkchk( 2 ) ;
  n = pop() ;
  *tos += n ;
}

void subt(){
  register Cell_t n ;

  stkchk( 2 ) ;
  n = pop() ;
  *tos -=  n ; 
}

void mult(){
  register Cell_t n ;

  stkchk( 2 ) ;
  n = pop() ;
  *tos *= n ;
}

void exponent(){
  register Cell_t n, exp ;

  stkchk( 2 ) ;
  n = pop() ;
  exp = *tos; 
  *tos = 1 ;
  for( ; n > 0 ; n-- ){
    *tos *= exp ;
  }
}

void divide(){
  register Cell_t n ; 
 
  stkchk( 2 ) ;
  n = pop(); 
  if( n == 0 ){
    throw( err_DivZero ) ;
    return ;
  }
  *tos /= n ; 
}

void modulo(){
  register Cell_t n ; 
 
  stkchk( 2 ) ;
  n = pop(); 
  if( n == 0 ){
    throw( err_DivZero ) ;
    return ;
  }
  *tos %= n ; 
}

void dotS(){
  Wrd_t d ;
  Cell_t *ptr ;

  // not necessary !!!!  stkchk( 0 ) ; 

  depth() ; dupe() ; dot() ; d = pop() ; 
  push( (Cell_t) " : " ) ; type() ;
  for( ptr = StartOf( stack )+1 ; tos >= ptr ; ptr++ ){
    push( *ptr ) ; dot() ;
  }
}

char buf[80];

void dot(){
  Wrd_t n ;
  int x;


  //  chk( 1 ) ;
  stkchk(1);

  //  n = fmt( "%d ", pop() ) - 1 ;
  x = pop();
  n = str_format(buf,80,"%d ",x)-1;
  outp( output, buf, n ) ;
  //  outp( output, (Str_t) tmp_buffer, n ) ;
}

void emit_hex_digits(int x, int digit){
  int n;
  int mask;
  int i;
  int X;

  switch(digit){
  case 1: mask = 0x0000000F; break;
  case 2: mask = 0x000000F0; break; 
  case 3: mask = 0x00000F00; break; 
  case 4: mask = 0x0000F000; break; 
  case 5: mask = 0x000F0000; break; 
  case 6: mask = 0x00F00000; break; 
  case 7: mask = 0x0F000000; break; 
  case 8: mask = 0xF0000000; break; // trouble !! shifting this produces 0xFF00.0000 !!! 
  }

  for(i=0; i<digit; i++){
    X = (x & mask) >> ((digit-1-i)*4);
    X &= 0x0000000f;
    n = str_format(buf,80,"%x",X)-1;
    outp( output, buf, n ) ;
    mask >>= 4;
    mask &= 0x0FFFFFFF;
  }
  outp(output," ",1);
}

void doth(void){
  int n;
  int x;
  stkchk(1);
  x = pop();
  n = str_format(buf,80,"%x",x & 0x0000000F)-1;
  outp( output, buf, n ) ;
}

void doth8(void){ emit_hex_digits(pop(),2); }
void doth16(void){ emit_hex_digits(pop(),4); }
void doth32(void){ emit_hex_digits(pop(),8); }


void udot(){
  Wrd_t n ;

  stkchk( 1 ) ;
  n = fmt( "%u ", (uCell_t) pop() ) - 1 ;
  outp( output, (Str_t) tmp_buffer, n ) ;
}

void dumpc(int addr){
  int i;
  char* p;
  p = (char*)addr;
  for(i=0; i<16; i++,p++){
    if(*p < ' ' || *p > 126) outp(output,".",1);
    else                     outp(output,p,1);
  }
}

//void dumpc(int addr){
//  int i;
//  char* p;
//  p = (char*)addr;
//  for(i=0; i<16; i++){
//    if(p[i] < ' ' || p[i] > 126) outp(output,".",1);
//    else                         outp(output,p+i,1);
//  }
//}
//    //    else                         outp(output,&p[i],1);






void dump(int addr){
  int* p;
  int i;

  p = (int*)addr;
  push((int)p);
  doth32();
  outp(output,"\b: ",3);
  for(i=0; i<4; i++){
    push(*p);
    doth32();
    p++;
  }
  dumpc(addr);
  outp(output,"\n\r",2);
}

// dumpi ( addr lines -- )

void dumpi(void){
  int lines;
  int addr;
  int i;

  stkchk(2);
  lines = pop();
  addr = pop();
  for(i=0; i<lines; i++){
    dump(addr);
    addr += 16;
    // this DOES NOT WORK =>  if(uC_kbhit(console)){ uC_getc(console); return; }
    // IT SHOULD WORK !!!!
  }
}


void words(){
  Dict_t *p ;
  Cell_t i ;
  int x;
  int n;
  int cnt = 0;
  char* name;

  p = StartOf( Colon_Defs ) ;
  for( i = n_ColonDefs - 1 ; i > -1 ; i-- ){
    p = &Colon_Defs[i] ;
    name = p->nfa;
    cnt += str_length(name);
    put_str(name) ;
  }

  p = StartOf( Primitives ) ;
  x = 0;
  while( p ->nfa ){
    name = p->nfa;
    cnt += str_length(name);
    put_str(name);
    if(cnt > 64) { put_str("\n\r"); cnt = 0; }
    p++ ;
    x++; // count primitive words
  }
  n = fmt("\nx=%d\n",x);
  outp( output, (Str_t) StartOf( tmp_buffer ), n ) ;
}

Wrd_t checkstack( Wrd_t n, Str_t fun ){
  Wrd_t x, d ;

  if( n > 0 ) {
    depth(); d = pop() ;
    if( d < n ){
      x = fmt( "-- Found %d of %d args expected in '%s'.\n", d, n, fun ) ; 
      outp( output, (Str_t) StartOf( tmp_buffer ), x ) ;
      throw( err_StackUdr ) ;
      //      longjmp( env, 4 ) ; // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!1
      // without setjmp/longjmp the catch/throw does not work properly
      return 9999;
    }
    return 1 ;
  }

  if( tos < (Cell_t *) StartOf( stack ) ){
    put_str( fun ) ; 
    throw( err_StackUdr ) ;
    return 0 ;
  }

  if( tos > &stack[sz_STACK] ){
    put_str( fun ) ; 
    throw( err_StackOvr ) ;
    return 0 ;
  }
  return 1 ;
}

void unresolved(){
  throw( err_UnResolved ) ;
}

void fwd_mark(){
  push( (Cell_t) Here ) ;
  push( (Cell_t) lookup( "unresolved" ) ) ;
  comma() ;
}

void fwd_resolve(){
  Cell_t *p ;

  p = (Cell_t *) pop() ;
  *p = (Cell_t) Here ;
}

void bkw_mark(){
  push( (Cell_t) Here ) ;
}

void bkw_resolve(){
  comma() ;
}

void  begin() {
  bkw_mark();
}         
  
void  again() {
  push( (Cell_t) lookup( "branch" ) ) ;
  comma() ;
  bkw_resolve();
}

void  While() {
  push( (Cell_t) lookup( "?branch" ) );
  comma() ;
  fwd_mark();
  swap();
}

void  Repeat() {
  push( (Cell_t) lookup( "branch" ) );
  comma() ;
  bkw_resolve();
  fwd_resolve();
}

void  Leave(){
  *rtos = 0 ; 
/*
  rpop() ;
  rpush( 0 ) ; 
*/
}

// until ( x -- )
void  Until(){
  push( (Cell_t) lookup( "?branch" ) );
  comma() ;
  bkw_resolve() ;
}

// for 
static int forcnt;

void For(void){
  forcnt = pop();
  bkw_mark();
}
// NOT SURE HOW TO DO THIS !!!!!!!
//
// &forcnt @ 1 - 0 == 
//
void Next(void){
  push((Cell_t)&forcnt); comma();
  push( (Cell_t) lookup( "@" ) ); comma();
  push(1); comma();
  push( (Cell_t) lookup( "-" ) ); comma();
  push(0); comma();
  push( (Cell_t) lookup( "==" ) ); comma();
  push( (Cell_t) lookup( "?branch" ) ); comma() ;
  bkw_resolve() ;
}

void  If(){
  push( (Cell_t) lookup( "?branch" ) ) ;
  comma() ;
  fwd_mark() ;
}

void Else(){
  push( (Cell_t) lookup( "branch" ) ) ;
  comma();
  fwd_mark() ;
  swap() ;
  fwd_resolve() ;
}

void Then(){
  fwd_resolve() ;
}


// TRUE and FALSE
// TRUE => 1
// FALSE => 0
//
// traditional FORTH uses -1 for TRUE
//
//

void  lt() {
  register Cell_t n ;

  stkchk( 2 ) ; 
  n = pop() ;
  *tos = (*tos < n) ? 1 : 0 ;
}

void  gt() {
  register Cell_t n ;

  stkchk( 2 ) ; 
  n = pop() ;
  *tos = (*tos > n) ? 1 : 0 ;
}

void  ge() {
  register Cell_t n ;
  
  stkchk( 2 ) ; 
  n = pop() ;
  *tos = (*tos >= n) ? 1 : 0 ;
}

void  ne() {
  register Cell_t n ;
  
  stkchk( 2 ) ; 
  n = pop() ;
  *tos = (*tos != n) ? 1 : 0 ;
}

void  eq() {
  register Cell_t n ;

  stkchk( 2 ) ; 
  n = pop() ;
  *tos = (*tos == n) ? 1 : 0 ;
}

// 0= ( x -- f ) T if x == 0
void  zeq() {
  register Cell_t n ;

  stkchk( 2 ) ; 
  n = pop() ;
  *tos = (*tos == 0) ? 1 : 0 ;
}

void  le() {
  register Cell_t n ;

  stkchk( 2 ) ; 
  n = pop() ;
  *tos = (*tos <= n) ? 1 : 0 ;
}

// this looks like a bitwise and
void And(){
  register Cell_t n ;

  stkchk( 2 ) ; 
  n = pop() ;
  *tos &= n ;
}
//
// and ( x y -- z )
void and(){
  register Cell_t n ;

  stkchk( 2 ) ; 
  n = pop() ;
  *tos = *tos && n ;
}

// or ( x y -- z )
void or(){
  register Cell_t n ;

  stkchk( 2 ) ; 
  n = pop() ;
  *tos |= n ;
}

// xor ( x y -- z )
void xor(){
  register Cell_t n ;

  stkchk( 2 ) ; 
  n = pop() ;
  *tos ^= n ;
}

// not ( 0x00000001 -- 0xFFFFFFFE ) bitwise invert
void not(){
  stkchk( 1 ) ; 
  *tos = ~(*tos) ;
}

void q_branch(){
  Cell_t *ptr ;

  ptr = (Cell_t *) rpop() ;
  if( pop() ){
    rpush( (Cell_t) ++ptr ) ;
    return ;
  }
  rpush( *ptr ) ;
}

void branch(){
  Cell_t *x ;

  x = (Cell_t *) rpop() ;
  rpush( *x ) ;
}

void depth(){
  Wrd_t d ; 

  d = tos - stack;
  push( d ) ; 
}

void dupe(){
  register Cell_t n ;

  stkchk( 1 ) ; 
  n = *tos;
  push( n ) ;
}

void drop(){
  Cell_t sav ;

  stkchk( 1 ) ; 
  sav = pop() ;
}

void over(){
  register Cell_t n ;

  stkchk( 2 ) ; 
  n = nos ;
  push( n ) ;
}

void Rto(){
  push( rpop() ) ;
}

void toR(){

  stkchk( 1 ) ;
  rpush( pop() ) ;
}

void swap(){
  register Cell_t t ;

  stkchk( 2 ) ;
  t = *tos ;
  *tos = nos ;
  nos = t ;
}

//void Eof(){
//  if( in_This > 0 ){
//    put_str( "<eof>\n" ) ;
//    push( 0 ) ; 
//    infile() ;
//    return ;
//  }
//  throw( err_NoInput ) ;
//  catch() ;
//  exit( 0 ) ;
//}

void cells(){
  stkchk( 1 ) ;
  *tos *= sizeof( Cell_t ) ;
}

void cellsize(){
  push( sizeof( Cell_t ) ) ;
}

void err_throw( Str_t whence, Err_t err ){
  error_loc = whence ;
  error_code = err ;
}

void catch(void){
  Wrd_t sz ;
  //  Fptr_t ok ;

  switch(error_code){
    case err_OK: return ;

    case err_NoInput:
    default:
      // whats the point !!!!      chk( 0 ) ;
      sz = fmt( "-- Error: code is %d.\n\r", error_code ) ;
      outp( output, (Str_t) tmp_buffer, sz ) ;
      sz = fmt( "-- Thrown by %s.\n\r", error_loc ) ;
      outp( output, (Str_t) tmp_buffer, sz ) ;
      if( error_code <= err_Undefined ){
        sz = fmt( "%s (%d).\n\r", errors[error_code], error_code ) ;
        outp( output, (Str_t) tmp_buffer, sz ) ;
      }
      if( error_code == err_NoInput ){
        goto die ;
      }
  }

  reset() ; // HERE IS MY PROBLEM !!!! reset() resets error_code
  //  longjmp( env, 2 );

 die:
  sz = fmt( "-- Terminated.\n\r" ) ;
  outp( output, (Str_t) tmp_buffer, sz ) ;
  //  exit( 1 ) ;
}

void wrd_fetch(){
  register Cell_t *p ;

  stkchk( 1 ) ; 
  p = (Cell_t *) pop() ;
  if( isNul( p ) ){
    throw( err_NullPtr ) ;
    return ;
  }
  push( *p ) ;
}

void wrd_store(){
  register Cell_t *p, n ;

  stkchk( 2 ) ; 
  p = (Cell_t *) pop() ;
  n = pop() ;
  if( isNul( p ) ){
    throw( err_NullPtr ) ;
    return ;
  }
  *p = n ;
}

void reg_fetch(){
  volatile register uWrd_t *p ;

  stkchk( 2 ) ; 
  p = (uWrd_t *) pop() ;
  if( isNul( p ) ){
    throw( err_NullPtr ) ;
    return ;
  }
  push( *p ) ;
}

void reg_store(){
  volatile register uWrd_t *p ; 
  register Cell_t n ;

  stkchk( 2 ) ; 
  p = (uWrd_t *) pop() ;
  n = pop() ;
  if( isNul( p ) ){
    throw( err_NullPtr ) ;
    return ;
  }
  *p = n ;
}

void crg_fetch(){
  volatile register char *p ;

  stkchk( 1 ) ; 
  p = (char *) pop() ;
  if( isNul( p ) ){
    throw( err_NullPtr ) ;
    return ;
  }
  push( *p & 0xff ) ;
}

void crg_store(){
  volatile register char *p ; 
  register Cell_t n ;

  stkchk( 2 ) ; 
  p = (char *) pop() ;
  n = pop() ;
  if( isNul( p ) ){
    throw( err_NullPtr ) ;
    return ;
  }
  *p = n & 0xff ;
}

void hlf_fetch(){
  register Hlf_t *p ;

  stkchk( 1 ) ; 
  p = (Hlf_t *) pop() ;
  if( isNul( p ) ){
    throw( err_NullPtr ) ;
    return ;
  }
  push( *p & _HALFMASK ) ;
}

void hlf_store(){
  volatile register Hlf_t *p ; 
  register Cell_t n ;

  stkchk( 2 ) ; 
  p = (Hlf_t *) pop() ;
  n = pop() ;
  if( isNul( p ) ){
    throw( err_NullPtr ) ;
    return ;
  }
  *p = n & _HALFMASK ;
}

void byt_fetch(){
  register char *p ;

  stkchk( 1 ) ; 
  p = (char *) pop();
  if( isNul( p ) ){
    throw( err_NullPtr ) ;
    return ;
  }
  push( *p & 0xff) ;
}

void byt_store(){
  volatile register char *p ; 
  register Cell_t n ;

  stkchk( 2 ) ; 
  p = (char *) pop() ;
  n = pop() ;
  if( isNul( p ) ){
    throw( err_NullPtr ) ;
    return ;
  }
  *p = n & 0xff ;
}
 
void lft_shift(){
  register Cell_t n ;

  stkchk( 2 ) ;
  n = pop() ;
  *tos <<= n ; 
}

// >> ( x n -- X )
void rgt_shift(){
  register Cell_t n ;

  stkchk( 2 ) ;
  n = pop() ;
  *tos >>= n ; 
}

void quote(){

  push( (Cell_t) str_delimited( "\"" ) ) ;
  if( state == state_Compiling ){
    ssave() ;
    push( (Cell_t) lookup( "(literal)" ) ) ;
    comma() ; 
    comma() ;
  }

}

// ." ( <string> -- ) type string
void dotquote(){

  quote() ;
  if( state == state_Compiling ){
    push( (Cell_t) lookup( "type" ) ) ;
    comma() ; 
    return ;
  }

  type() ;
}

void comment(){
  Str_t ptr ;
  ptr = str_delimited( ")" ) ;
}

void dotcomment(){
  push( (Cell_t) str_delimited( ")" ) ) ;
  type() ;
}

Str_t str_delimited( Str_t terminator ){
  Str_t tkn, ptr, ret ; 
  Wrd_t len ;

  ++promptVal ;
  pad() ; ptr = ret = (Str_t) pop() ;
  do {
    word() ;
    tkn = (Str_t) pop() ;
    len = str_length( tkn ) ;
    if( tkn[len-1] == *terminator ){
      str_copy( ptr, tkn, len-1 ) ;
      ptr[len-1] = (char) 0 ; 
      break ;
    }
    str_copy( ptr, tkn, len );
    ptr += len ;
    *ptr++ = ' ' ;
  } while( 1 ) ;
  --promptVal ;
  return( ret ) ;
}

// count ( addr -- addr n )
void count(){
  Wrd_t len ;

  stkchk( 1 ) ; 
  len = str_length( (Str_t) *tos ) ;
  push( (Cell_t) len ) ;
}

void ssave(){
  Str_t str ; 

  stkchk( 1 ) ;
  str = (Str_t) pop() ;
  push( (Cell_t) str_cache( str ) ) ;
}

void unssave(){
  char *tag; //, *buf_ptr ;
  //  Wrd_t d ;

  stkchk( 1 ) ;
  tag = (char *) pop();
  if( isMatch( tag, String_Data+2 ) ){
    str_uncache( tag ) ; 
    return ;
  }
  throw( err_Unsave ) ;

}

void SCratch(){
  push( (Cell_t) scratch ) ;
}

void Tmp(){
  push( (Cell_t) tmp ) ;
}

void pad(){
  register Cell_t n ; 
  //  Str_t  pad ;

  here() ;
  push( 20 ) ; 
  cells() ; 
  add() ; 
  n = pop() ; 
  push( n ) ;
}

void cmove(){ 
  Cell_t len ; 
  Str_t src, dst ;

  len = pop() ;
  dst = (Str_t) pop() ;
  src = (Str_t) pop() ;
  str_copy( dst, src, len ) ;
}

//void word(){ 
//  Str_t tkn ;
// 
//  do{
//    tkn = str_token( scratch, sz_INBUF );
//  } while(isNul( tkn ) ) ;
//  push( (Cell_t) tkn ) ;
//}

void word(void){
  char* token;
  token = str_tokeniiii(tib);
  if(token == NULL){ throw(err_NoInput);  return; }
  push((Cell_t)token);
}

void ascii(){ 
  Str_t p ;

  word() ;
  p = (Str_t) pop() ;
  push( (Cell_t) *p ) ;
}

void q_key(){
  push(uC_kbhit(console));
}



//void q_key(){
//#ifdef HOSTED
//#if !defined( __WIN32__ )
//  Wrd_t  rv ;
//
//  push( (Cell_t) INPUT ) ;
//  push( (Cell_t) 0 ) ;
//  push( (Cell_t) 0 ) ;
//  io_cbreak( INPUT ) ;
//  waitrdy() ;
//  io_cbreak( INPUT ) ;
//#endif
//#endif
//}
//
void key(){
  char ch ;
  Wrd_t x ;
  while(uC_kbhit(console) == 0){}
  push(uC_getc(console));
}
//
//  x = io_cbreak( INPUT ) ;
//  inp( input, (Str_t) &ch, 1 ) ;
//  x = io_cbreak( INPUT ) ;
//  push( ch & 0xff ) ;
//
//}

void emit(){
  stkchk(1);
  uC_emit(pop(),console);
  //  outp( output, (Str_t) tmp_buffer, str_format( (Str_t) tmp_buffer, (Wrd_t) sz_INBUF, "%c", pop() ) ) ;
}
// type (addr -- )
void type(){
  Str_t str ;

  stkchk( 1 ) ; 
  str = (Str_t) pop() ;
  outp( output, (Str_t) str, str_length( str ) ) ;
}

void cr(){
  uC_print("\n\r");
  //  outp( output, (Str_t) "\n\r", 1 ) ;
}

void dp(){
  push( (Cell_t) DictPtr ) ;
}

void here(){
  push( (Cell_t) Here ) ;
}

void freespace(){
  push( (Cell_t) ((Str_t) String_Data - (Str_t) Here) ) ;
}

void comma(){
  Cell_t space ;

  stkchk( 1 ) ; 
  freespace();
  space = pop() ;
  if( space > sizeof( Cell_t ) ){
    push( (Cell_t) Here++ ) ;
    wrd_store() ;
  } else {
    throw( err_NoSpace ) ;
  }
}

void doLiteral(){
  Cell_t *p ; 
  p = (Cell_t *) rpop() ;
  push( *(p++) ) ;
  rpush( (Cell_t) p ) ;
}


void pushPfa(){
  push( rpop() ) ;
}

void does(){
  Dict_t *dp ; 
  Cell_t **p ;

  dp = &Colon_Defs[n_ColonDefs-1] ;
  push( (Cell_t) dp ->pfa ) ;
  dp ->pfa = Here ;
  push( (Cell_t) lookup( "(literal)" ) ) ;
  comma() ; /* push the original pfa  */
  comma() ;

  switch( state ){
    case state_Interactive:
      state = state_Compiling ;

    case state_Compiling:
      compile() ;
      return ;

    case state_Interpret: /* copy the does> behaviour into the new word */
      dp ->cfa = doColon ;
      while( (p = (Cell_t **) rpop()) ) {
        dp = (Dict_t *) *p ; ;
        if( isNul( dp ) ){
          rpush( 0 ) ; /* end of current word interpretation */
          push( 0 ) ; /* end of defined word (compile next) */
          comma() ;
          break ;
        }
        rpush( (Cell_t) ++p ) ;
        push( (Cell_t) dp ) ;
        comma() ;
      }
      break ;

    default:
      throw( err_BadState ) ;
  }
}

void allot(){
  Cell_t n ;

  stkchk( 1 ) ;
  n = pop() ;
  Here += n ; 
}

// create 
void create(){
  //  Cell_t *save ;
  Str_t   tag ;
  Dict_t *dp ;

  dp = &Colon_Defs[n_ColonDefs++] ;

  word();
  tag = (Str_t) pop() ;
  dp ->nfa = str_cache( tag ) ; /* cache tag */
  dp ->cfa = pushPfa ;
  dp ->flg = Normal ;
  dp ->pfa = Here ;
}

void variable(void){
  create();
  push(1);
  allot();
}

void doConstant(){
  push( rpop() ) ;
  wrd_fetch() ;
}

// constant ( x -- )
void constant(){
  Dict_t *dp ;

  create() ;
  comma() ;
  dp = &Colon_Defs[n_ColonDefs-1] ;
  dp ->cfa = doConstant ;
}

void colon(){
  state = state_Compiling ;
  create();
  compile() ; 
}

void compile(){
  Dict_t *dp ;
  Str_t   tkn ; 
  Cell_t *save, value ;

  save = Here ;
  dp = &Colon_Defs[n_ColonDefs-1] ;
  dp ->cfa = doColon ;

  ++promptVal ;


  //  while( (tkn = str_token( scratch, sz_INBUF )) ){


  while(1){
    tkn = str_tokeniiii(tib);

    if(tkn == NULL){
      new_prompt2();
      accept();
      str_tokeniiii(NULL);
      continue;
    }

    if( isMatch( tkn, ";" ) ){
      semicolon() ;
      return; // break ;
    }

    dp = (Dict_t *) lookup( tkn ) ;

    if( !isNul( dp ) ){
      push( (Cell_t) dp ) ;
      if( state == state_Immediate || dp ->flg == Immediate ){
        execute() ; /* execute */
      }
      else {
        comma() ;   /* compile */
      }
    }
    else {
      value = (Cell_t) str_literal( tkn, Base ) ;
      if( error_code != err_OK ){
        str_uncache( (Str_t) String_Data ) ; 
        Here = save ;
        state = state_Interpret ;
        throw( err_BadString ) ;
        return ; /* like it never happened */
      }

      push( value ) ;
      if( state != state_Immediate ){
        push( (Cell_t) lookup( "(literal)" ) ) ;
        comma() ;
        comma() ;
      }
    }
  } /* while */
}

void pvState(){
  state = state_save ;
}

void imState(){
  state_save = state ;
  state = state_Immediate ;
}

// normal ( -- )
void normal(){
  Dict_t *dp ;

  dp = &Colon_Defs[n_ColonDefs-1] ;
  dp ->flg = Normal ;
}

// immediate ( -- )
void immediate(){
  Dict_t *dp ;

  dp = &Colon_Defs[n_ColonDefs-1] ;
  dp ->flg = Immediate ;
}

void call(){
  Cptr_t fun ;
 
  stkchk( 1 ) ; 
  fun = (Cptr_t) pop() ;
  push( (*fun)() ) ;
}

void tracing( Dict_t *dp ){
  if( !Trace ){
    return ;
  }
  dotS() ;
  put_str( "\t\t\t" ) ;
  if( isNul( dp ) )
    put_str( "next" ) ;
  else 
    put_str( dp ->nfa ) ;
  cr() ;
}

void execute(){
  Dict_t *dp ; 

  stkchk( 1 ) ; 
  dp = (Dict_t *) pop() ;
  if( dp ->pfa ){
    rpush( (Cell_t) dp->pfa ) ;
  }
  tracing( dp ) ;
  (*dp ->cfa)() ;
  catch() ;
}

void doColon(){
  Dict_t *dp ;
  Cell_t **p ;
  State_t save ;

  save = state ;
  state = state_Interpret ;
  while( (p = (Cell_t **) rpop()) ) {
    dp = (Dict_t *) *p ; ;
    if( isNul( dp ) ){
      break ;
    }
    rpush( (Cell_t) ++p ) ;
    push( (Cell_t) dp ) ;
    execute();
  }
  state = save ;
}

void semicolon(){
  //  Dict_t *ptr ; 

  if( state != state_Compiling ){
    throw( err_BadState );
    return ;
  }
  push( 0 ) ; /* next is NULL */
  comma() ;
  --promptVal ;
  state = state_Interactive ;
}

void tick(){
  Str_t tkn ; 

  stkchk( 0 ) ; 
  word() ;
  tkn = (Str_t) pop() ;
  push( (Cell_t) lookup( tkn ) ) ;
  if( *tos == 0 ){
    put_str( tkn ) ;
    throw( err_NoWord ) ;
  }
}

void nfa(){
  Dict_t *dp ;

  stkchk( 1 ) ; 
  dp = (Dict_t *) pop();
  push( (Cell_t) dp ->nfa ) ;
}

void cfa(){
  Dict_t *dp ;

  stkchk( 1 ) ; 
  dp = (Dict_t *) pop();
  push( (Cell_t) dp ->cfa ) ;
}

void pfa(){
  Dict_t *dp ;

  stkchk( 1 ) ; 
  dp = (Dict_t *) pop() ;
  push( (Cell_t) dp ->pfa ) ;
}

void decimal(){
  Base = 10 ; 
}

void hex(){
  Base = 16 ; 
}

//void sigvar(){
//#ifdef HOSTED
//  push( (Cell_t) &sigval ); 
//#endif
//}

void errvar(){
  push( (Cell_t) &error_code ); 
}

void trace(){
  push( (Cell_t) &Trace ); 
}

void base(){
  push( (Cell_t) &Base ); 
}

//void resetter(){
//  //  longjmp( env, 3 ) ;
//}

void see(){
  register Dict_t *p, *r ; 
  Cell_t *ptr, n ; 

  stkchk( 1 ) ; 

  p = (Dict_t *) pop() ;
  if( isNul( p ->pfa ) ){
    n = fmt( "-- %s (%x) flg: %d is coded in C (%x).\n\r", p ->nfa, p, p->flg, p ->cfa ) ;
    outp( output, (Str_t) StartOf( tmp_buffer ), n ) ;
    return ;
  } else {
    if( p ->cfa == (Fptr_t) doConstant ){
      n = fmt( "-- %s constant value (0x%x).\n\r", p ->nfa, *p->pfa ) ;
      outp( output, (Str_t) StartOf( tmp_buffer ), n ) ;
      return ;
    }
    if( p ->cfa == (Fptr_t) pushPfa ){
      n = fmt( "-- %s variable value (0x%x).\n\r", p ->nfa, *p->pfa ) ;
      outp( output, (Str_t) StartOf( tmp_buffer ), n ) ;
      return ;
    }
    n = fmt( "-- %s (%x) word flg: %d.\n\r", p ->nfa, p, p->flg ) ;
    outp( output, (Str_t) StartOf( tmp_buffer ), n ) ;
  }
  ptr = p ->pfa ; 
  while( !isNul( ptr ) ){
    r = (Dict_t *) *ptr ;
    if( isNul( r ) ){                   /* next == NULL */
      n = fmt( "%x  next\n\r", ptr ) ;
      outp( output, (Str_t) StartOf( tmp_buffer ), n ) ;
      break ;
    }
    if( r ->cfa  == (Fptr_t) branch ){
      n = fmt( "%x  %s -> %x\n\r", ptr, r ->nfa, *(ptr+1) ) ;
      ptr++ ;
    } else  if( r ->cfa  == (Fptr_t) q_branch ){
      n = fmt( "%x  %s -> %x\n\r", ptr, r ->nfa, *(ptr+1) ) ;
      ptr++ ;
    } else if( r ->cfa  == (Fptr_t) doLiteral ){
      n = fmt( "%x  %s = %d\n\r", ptr, r ->nfa, *(ptr+1) ) ;
      ptr++ ;
    } else {
      n = fmt( "%x  %s\n\r", ptr, r ->nfa ) ;
    }
    outp( output, (Str_t) StartOf( tmp_buffer ), n ) ;
    ptr++ ;
  }
}

//
//  -- I/O routines ...
//

Wrd_t put_str( Str_t s ){
  register Cell_t n ;
  if( !isNul( s ) ){
    n = str_length( s ) ; 
    outp( output, s, n ) ;
    outp( output, " ", 1 ) ;
  }
  return n ;
}

Wrd_t io_cbreak( int fd ){
#if defined( HOSTED ) 
#if !defined(__WIN32__)
  static int inCbreak = v_Off ;
  static struct termios tty_state, *tty_orig = (struct termios *) NULL ; 
  int rv ;

  if( isNul( tty_orig ) ){
    rv = tcgetattr( fd, &tty_normal_state );
    tty_orig = &tty_normal_state ;
  } 

  switch( inCbreak ){
    case v_Off:
      rv = tcgetattr( fd, &tty_state ) ;
      cfmakeraw( &tty_state ) ;
      rv = tcsetattr( fd, TCSANOW, &tty_state ) ; 
      inCbreak = v_On ;
      break ;
    case v_On:
      rv = tcsetattr( fd, TCSANOW, tty_orig ) ; 
      inCbreak = v_Off ;
  }
  return inCbreak ;
#endif
#endif
}


void Memset(){  /* ( ptr val len -- ) */
  char byt ;
  Str_t ptr ;
  Wrd_t len ;

  stkchk( 3 ) ; 
  len = (Wrd_t) pop() ;
  byt = (char) pop() & 0xff ;
  ptr = (Str_t) pop() ;
  str_set( ptr, byt, len ) ;
}


int errno;
#define EAGAIN 999

Wrd_t getstr( Wrd_t fd, Str_t buf, Wrd_t len ){
  char ch ;
  Wrd_t i, n, crlf = 0 ; 

  i = 0 ; 
  do {
    n = inp( input, (Str_t) &ch, 1 ) ;
    if( n < 1 ){
      if( errno != EAGAIN ){
       throw( err_SysCall ) ;
      }
      return i ; 
    }
    if( i > (len - 1) ){
      return i ;
    }
    if( ch_matches( ch, "\r\n" ) ){ 
       crlf++ ;
    }
    buf[i++] = ch ;
  } while( crlf < 2 ) ;
  buf[i-1] = (char) 0 ; 
  return i ;
}

//void rcvtty(){        /* ( fd n -- buf n ) */
//  Str_t buf ;
//  Wrd_t n, nx, nr, fd ;
//
//  chk( 2 ) ; 
//
//  n = pop() ;
//  fd = pop() ;
//  pad() ; buf = (Str_t) pop() ;
//  nr = getstr( fd, buf, n ) ;
//  push( (Cell_t) buf ) ;
//  if( nr > 0 ){
//    push( (Cell_t) nr ) ;
//    return ;
//  } 
//  push( 0 ) ; 
//  return ;
//}

//void opentty(){
//#ifdef HOSTED
//#if !defined(__WIN32__)
//  Str_t fn ;
//  Wrd_t rv, fd ;
//  struct termios tty_state ;
//
//  chk( 1 ) ; 
//
//  fn = (Str_t) pop() ;
//  if( !isNul( fn ) ){
//    fd = open( fn, O_RDWR | O_NDELAY | O_NONBLOCK | O_NOCTTY ) ;
//    if( fd < 0 ){
//      throw( err_SysCall ) ;
//      return ;
//    }
//    rv = tcgetattr( fd, &tty_state ) ;
//    cfsetspeed( &tty_state, B115200 ) ; 
//    tty_state.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG ) ;
//    cfmakeraw( &tty_state ) ; 
//    rv = tcsetattr( fd, TCSANOW, &tty_state ) ;
//    if( rv < 0 ){
//      throw( err_SysCall ) ;
//      return ;
//    }
//  } 
//  push( fd ) ; 
//#endif
//#endif
//}

//void closetty(){
//  chk( 1 ) ; 
//  close( (Wrd_t) pop() ) ;
//}

//void infile(){
//  Str_t fn ;
//
//  chk( 1 ) ; 
//
//  fn = (Str_t) pop() ;
//#ifdef HOSTED
//  if( !isNul( fn ) ){
//    in_files[++in_This] = open( fn, O_RDONLY ) ;
//    return ;
//  } 
//  close( INPUT ) ;
//  in_This-- ;
//#endif
//}
//
//void outfile(){
//  Str_t fn ;
//
//  fn = (Str_t) pop() ;
//#ifdef HOSTED
//  if( !isNul( fn ) ){
//    out_files[++out_This] = open( fn, O_CREAT | O_TRUNC | O_RDWR | O_APPEND
//#if !defined( __WIN32__ )
//      , S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH ) ;
//#else
//      ) ;
//#endif
//    return ;
//  } 
//  close( output ) ;
//  out_This-- ;
//#endif
//}





Wrd_t outp( Wrd_t fd, Str_t buf, Wrd_t len ){
  return uC_write( fd, buf, len ) ;
  //  return write0( buf, len ) ;
}


Wrd_t inp( Wrd_t fd, Str_t buf, Wrd_t len ){
  uC_fgets(buf,len,fd);
  //  return read( fd, buf, len ) ;
}


//#ifdef HOSTED 
//void qdlopen(){
//  Str_t lib ;
//  Opq_t opaque ;
//
//  chk( 1 ) ; 
//  lib = (Str_t) pop() ;
//  opaque = dlopen( lib, RTLD_NOW | RTLD_GLOBAL ) ;
//  push( (Cell_t) opaque ) ;
//}
//
//void qdlclose(){
//  Opq_t opaque ;
//
//  chk( 1 ) ; 
//  opaque = (Opq_t) pop() ;
//  push( (Cell_t) dlclose( opaque ) ) ;
//}
//
//void qdlsym(){
//  Str_t symbol ;
//  Opq_t handle ;
//
//  chk( 2 ) ; 
//  symbol = (Str_t) pop() ;
//  handle = (Opq_t) pop() ;
//  push( (Cell_t) dlsym( handle, symbol ) ) ;
//}
//
//void qdlerror(){
//  chk( 0 ) ; 
//  push( (Cell_t) dlerror() ) ;
//}
//
//#endif /* HOSTED */

//void last_will(){
//  Opq_t cmd ;
//
//  chk( 1 ) ; 
//  cmd = (Opq_t) pop() ;
//  //  atexit( cmd ) ;
//}

void spinner(){
  static char x = 0 ;
  char n, f[4] = { '-', '\\', '|', '/' } ;

  n = fmt( " %c\r", f[ x++ % sizeof( f ) ] ) - 1 ;
  outp( output, (Str_t) tmp_buffer, n ) ;
}

// callout ( w x y z n func -- )
//
// this fucker works !!!!!!!!!!
void callout(){
  Cptr_t fun ;
  Cell_t i, n ;
  Cell_t args[10] ;

  fun = (Cptr_t) pop(); // gdb will give me the address of a function BUT the address must
                        // be odd because of the thumb thingy !!!!!

  n = pop() ;

  stkchk( n ) ; /* really need n+2 items ... */

  for( i = n-1 ; i >= 0 ; i-- ){
    args[i] = pop() ;
  }

  switch( n ){
    case 0:
      push( (*fun)() ) ;
      break ;
    case 1:
      push( (*fun)( args[0] ) ) ;
      break ;
    case 2:
      push( (*fun)( args[0], args[1] ) ) ;
      break ;
    case 3:
      push( (*fun)( args[0], args[1], args[2] ) ) ;
      break ;
    case 4:
      push( (*fun)( args[0], args[1], args[2], args[3] ) ) ;
      break ;
    case 5:
      push( (*fun)( args[0], args[1], args[2], args[3], args[4] ) ) ;
      break ;
    case 6:
      push( (*fun)( args[0], args[1], args[2], args[3], args[4], args[5] ) ) ;
      break ;
    case 7:
      push( (*fun)( args[0], args[1], args[2], args[3], args[4], args[5], args[6] ) ) ;
      break ;
    case 8:
      push( (*fun)( args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7] ) ) ;
      break ;
    case 9:
      push( (*fun)( args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8] ) ) ;
      break ;
  }
  return ;
}




#if 0
this is the JUNK YARD
: VARIABLE  ( <name> -- )
    CREATE 0 , \ IMMEDIATE
\       DOES> [compile] aliteral  \ %Q This could be optimised

#define fmt( x, ... )   str_format( (Str_t) StartOf( tmp_buffer ), (Wrd_t) sz_INBUF, x, ## __VA_ARGS__ )
  str_format(dst,len,fmt,...)
#define StartOf(x)      (&x[0]) <= the address of the first element of array x

Wrd_t str_format( Str_t dst, Wrd_t dlen, Str_t fmt, ... ){

#endif

