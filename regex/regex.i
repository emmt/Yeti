/*
 * regex.i --
 *
 * POSIX regular expressions for Yorick.
 *
 *-----------------------------------------------------------------------------
 *
 * This file is part of Yeti (https://github.com/emmt/Yeti) released under the
 * MIT "Expat" license.
 *
 * Copyright (C) 1996-2020: Éric Thiébaut.
 *
 *-----------------------------------------------------------------------------
 */

/* load dynamic code */
if (is_func(plug_in)) plug_in, "yor_regex";

extern regcomp;
/* DOCUMENT regcomp(reg);
     Compile regular expression STR for further use.  REG must be a scalar
     string.

     If keyword NEWLINE  is true (non-nil and non-zero),  then newlines are
     treated specifically (i.e. as if text lines separated by newlines were
     individual strings):
       - Match-any-character operators don't match a newline.
       - A non-matching  list ([^...])  not  containing a  newline does not
         match a newline.
       - Match-beginning-of-line  operator  (^) matches  the  empty  string
         immediately after a newline,  regardless of whether keyword NOTBOL
         is true when the regular expression is used in regsub or regmatch.
       - Match-end-of-line   operator   ($)  matches    the   empty  string
         immediately before a newline, regardless of whether keyword NOTEOL
         is true when the regular expression is used.

     The default is to treat newline as any other character.

     If  keyword BASIC  is true  (non-nil and  non-zero), then  POSIX Basic
     Regular  Expression  syntax is  used;  the  default  is to  use  POSIX
     Extended Regular Expression syntax.

     If  keyword  ICASE  is   true  (non-nil  and  non-zero),  the  regular
     expression will  be case insensitive. The default  is to differentiate
     case.

     Keyword  NOSUB  can  be  set  to  a  non-zero  value  if  support  for
     subexpression addressing  of matches is  not required (see  regsub and
     regmatch).

   KEYWORDS: basic, icase, newline, nosub.

   SEE ALSO: regmatch, regsub. */

extern regmatch;
/* DOCUMENT regmatch(reg, str);
       -or- regmatch(reg, str, match0, match1, match2, match3, ...);
     Test if regular  expression REG matches string array  STR.  The result
     is an array of int's with same dimension list as STR and with elements
     set to 1/0 whether REG match or not the corresponding element in STR.

     REG can be a scalar string  which get compiled at runtime and the same
     compilation keywords  as those accepted  by regcomp can  be specified:
     BASIC,  ICASE,  NEWLINE  and/or  NOSUB.   Otherwise,  REG  must  be  a
     pre-compiled  regular  expression  (by  regcomp)  and  no  compilation
     keyword can be specified.

     Keyword START can be used to specify a starting index for the matching
     in the  input string(s).  If START is  less or equal zero,  then it is
     counted from the end of the input string(s): START=0 to start with the
     last character,  START=-1 to  start with the  2 last  characters.  The
     match  is considered  as  a  failure when  the  starting character  is
     outside  the string.   The default  is START=1  (start with  the first
     character ot every input string).

     If   keyword   NOTBOL   is    true   (non-nil   and   non-zero),   the
     match-beginning-of-line   operator   (^)   always  fails   to   match.
     Similarly, if  keyword NOTEOL is true,  the match-end-of-line operator
     ($) always fails to match.   These keywords may be used when different
     portions of a string are  matched against a regular expression and the
     beginning/end  of  the  string   should  not  be  interpreted  as  the
     beginning/end of the line (but see the keyword NEWLINE in regcomp).

     In the second  form, matching patterns get stored  in output variables
     MATCH0,  MATCH1...  MATCH0  will be  filled by  the part  of  STR that
     matched the  complete regular expression REG;  MATCHn (with n=1,2...),
     will  contain the  part of  STR  that matched  the n-th  parenthesized
     subexpression  in  REG.   If  keyword  INDICES is  true  (non-nil  and
     non-zero) the indices of matching (sub)expressions get stored into the
     MATCHn variables: MATCHn(1,..) and MATCHn(2,..) are the indices of the
     first  and last+1 characters  of the  matching (sub)expressions  -- it
     must be last+1  to allow for empty match.  If  keyword INDICE is false
     or  omitted, the  MATCHn variables  will  be string  arrays with  same
     dimension lists as STR.

   KEYWORDS: basic, icase, indices, newline, nosub, notbol, noteol.

   SEE ALSO: regcomp, , regmatch_part, regsub. */


extern regsub;
/* DOCUMENT regsub(reg, str)
       -or- regsub(reg, str, sub)
     Substitute pattern(s) in STR matching regular expression REG by string
     SUB (which is the nil string  by default). STR is an array of strings,
     and the result has the same dimension list as STR.  Each string of STR
     is matched  against regular expression REG  and, if there  is a match,
     the matching part is replaced by  SUB (which must be a scalar string).
     If SUB contains a sequence  "\n" (where n=0,...,9), then this sequence
     is replaced in  the result by the n-th  parenthesized subexpression of
     REG  ("\0"  stands for  the  whole  matching  part).  Other  backslash
     sequences "\c" (where  c is any character) in SUB  get replaced by the
     escaped character c.  Beware that Yorick's parser interprets backslash
     sequences  in literal  strings,  you  may therefore  have  to write  2
     backslashes.

     REG can be a scalar string  which get compiled at runtime and the same
     compilation keywords  as those accepted  by regcomp can  be specified:
     BASIC,  ICASE,  NEWLINE  and/or  NOSUB.   Otherwise,  REG  must  be  a
     pre-compiled  regular  expression  (by  regcomp)  and  no  compilation
     keyword can be specified.

     If keyword ALL is true  (non-nil and non-zero), then all occurences of
     REG  get substituted.   Otherwise only  the 1st  occurence of  REG get
     substituted.  Of course substitution is performed for every element of
     STR.

     If   keyword   NOTBOL   is    true   (non-nil   and   non-zero),   the
     match-beginning-of-line   operator   (^)   always  fails   to   match.
     Similarly, if  keyword NOTEOL is true,  the match-end-of-line operator
     ($) always fails to match.   These keywords may be used when different
     portions of a string are  matched against a regular expression and the
     beginning/end  of  the  string   should  not  be  interpreted  as  the
     beginning/end of the line (but see the keyword NEWLINE above).

   KEYWORDS: all, basic, icase, newline, nosub, notbol, noteol.

   SEE ALSO: regcomp, regmatch. */

func regmatch_part(s, i)
/* DOCUMENT regmatch_part(str, idx);
     Get part  of string  STR indexed by  IDX (which  should be one  of the
     outputs of  regmatch when called with keyword  INDICES=1).  The result
     is an array of strings with same dimension list as STR.  IDX must have
     one more dimension than STR, the  first dimension of IDX must be 2 and
     the other must be identical to those of STR.

   SEE ALSO regmatch. */
{
  sp = array(string, dimsof(s));
  i1 = i(1,);
  i2 = i(2,) - 1;
  i = where(i2 >= i1);
  n = numberof(i);
  for (k=1 ; k<=n ; ++k) {
    ik = i(k);
    sp(ik) = strpart(s(ik), i1(ik):i2(ik));
  }
  return sp;
}
