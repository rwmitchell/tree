/* $Copyright: $
 * Copyright (c) 1996 - 2018 by Steve Baker (ice@mama.indstate.edu)
 * All Rights reserved
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include "tree.h"

/*
 * Hacked in DIR_COLORS support for linux. ------------------------------
 */
/*
 *  If someone asked me, I'd extend dircolors, to provide more generic
 * color support so that more programs could take advantage of it.  This
 * is really just hacked in support.  The dircolors program should:
 * 1) Put the valid terms in a environment var, like:
 *    COLOR_TERMS=linux:console:xterm:vt100...
 * 2) Put the COLOR and OPTIONS directives in a env var too.
 * 3) Have an option to dircolors to silently ignore directives that it
 *    doesn't understand (directives that other programs would
 *    understand).
 * 4) Perhaps even make those unknown directives environment variables.
 *
 * The environment is the place for cryptic crap no one looks at, but
 * programs.  No one is going to care if it takes 30 variables to do
 * something.
 */
enum {
  CMD_COLOR, CMD_OPTIONS, CMD_TERM, CMD_EIGHTBIT, COL_NORMAL, COL_FILE, COL_DIR,
  COL_LINK, COL_FIFO, COL_DOOR, COL_BLK, COL_CHR, COL_ORPHAN, COL_SOCK,
  COL_SETUID, COL_SETGID, COL_STICKY_OTHER_WRITABLE, COL_OTHER_WRITABLE,
  COL_STICKY, COL_EXEC, COL_MISSING, COL_LEFTCODE, COL_RIGHTCODE, COL_ENDCODE,
  DOT_EXTENSION, ERROR
};

bool colorize = false, ansilines = false, linktargetcolor = false;
char *term, termmatch = false, istty;
char *leftcode      = NULL,  *rightcode    = NULL, *endcode     = NULL,
     *forecode      = "[38;5;",   // foreground
     *backcode      = "[48;5;";   // background

char *all_flags[ERROR+1],
     *all_glyph[ERROR+1];
char *norm_flgs     = NULL,  *file_flgs    = NULL, *dir_flgs    = NULL, *link_flgs = NULL;
char *fifo_flgs     = NULL,  *door_flgs    = NULL, *block_flgs  = NULL, *char_flgs = NULL;
char *orphan_flgs   = NULL,  *sock_flgs    = NULL, *suid_flgs   = NULL, *sgid_flgs = NULL;
char *stickyow_flgs = NULL,  *otherwr_flgs = NULL, *sticky_flgs = NULL;
char *exec_flgs     = NULL,  *missing_flgs = NULL;

const
char *vgacolor[] = {
  "black", "red", "green", "yellow", "blue", "fuchsia", "aqua", "white",
  NULL, NULL,
  "transparent", "red", "green", "yellow", "blue", "fuchsia", "aqua", "black"
};

struct colortable colortable[11];
struct extensions *ext = NULL;
const struct linedraw *linedraw;

char **split(char *str, const char *delim, int *nwrds);
int cmd(char *s);

extern FILE *outfile;
extern bool Hflag, force_color, nocolor, lsicons, exfat;
extern const char *charset;

void parse_dir_colors() {
  const
  char *s;
  char buf[1025], **arg, **c, *colors, *cc, *pt;
  int i, n, j;
  struct extensions *e;

  if (Hflag) return;

  if (getenv("TERM") == NULL) {
    colorize = false;
    return;
  }

  s = getenv("LS_ICONS");
  if ( s ) lsicons = true;
  else s = getenv("LS_COLORS");
  cc = getenv("CLICOLOR");
  if (getenv("CLICOLOR_FORCE") != NULL) force_color=true;
  if ((s == NULL || strlen(s) == 0) && (force_color || cc != NULL)) s = ":no=00:fi=00:di=01;34:ln=01;36:pi=40;33:so=01;35:bd=40;33;01:cd=40;33;01:or=40;31;01:ex=01;32:*.bat=01;32:*.BAT=01;32:*.btm=01;32:*.BTM=01;32:*.cmd=01;32:*.CMD=01;32:*.com=01;32:*.COM=01;32:*.dll=01;32:*.DLL=01;32:*.exe=01;32:*.EXE=01;32:*.arj=01;31:*.bz2=01;31:*.deb=01;31:*.gz=01;31:*.lzh=01;31:*.rpm=01;31:*.tar=01;31:*.taz=01;31:*.tb2=01;31:*.tbz2=01;31:*.tbz=01;31:*.tgz=01;31:*.tz2=01;31:*.z=01;31:*.Z=01;31:*.zip=01;31:*.ZIP=01;31:*.zoo=01;31:*.asf=01;35:*.ASF=01;35:*.avi=01;35:*.AVI=01;35:*.bmp=01;35:*.BMP=01;35:*.flac=01;35:*.FLAC=01;35:*.gif=01;35:*.GIF=01;35:*.jpg=01;35:*.JPG=01;35:*.jpeg=01;35:*.JPEG=01;35:*.m2a=01;35:*.M2a=01;35:*.m2v=01;35:*.M2V=01;35:*.mov=01;35:*.MOV=01;35:*.mp3=01;35:*.MP3=01;35:*.mpeg=01;35:*.MPEG=01;35:*.mpg=01;35:*.MPG=01;35:*.ogg=01;35:*.OGG=01;35:*.ppm=01;35:*.rm=01;35:*.RM=01;35:*.tga=01;35:*.TGA=01;35:*.tif=01;35:*.TIF=01;35:*.wav=01;35:*.WAV=01;35:*.wmv=01;35:*.WMV=01;35:*.xbm=01;35:*.xpm=01;35:";

  if (s == NULL || (!force_color && (nocolor || !isatty(1)))) {
    colorize = false;
    return;
  } else {
    colorize = true;
    /* You can uncomment the below line and tree will always try to ANSI-fy the indentation lines */
    /*    ansilines = true; */
  }

  colors = scopy(s);

  arg = split(colors,":",&n);

//int ext_cnt=0;

  for(i=0;arg[i];i++) {
    c = split(arg[i],"=",&n);

    if ( (j=cmd( c[0] )) <  ERROR ) {
      pt = all_flags[j] = scopy(c[1]);
      while ( *pt != '\0' ) { *pt = toupper( *pt ); ++pt; }   //  uppercase pattern
      if ( lsicons ) {
         pt = strrchr( all_flags[j], ';' );
        *pt = '\0';
        all_glyph[j]    = pt+1;
//      printf( "%2d: %s :: %s\n", j, all_glyph[j], c[0] );
      }
    }

//  if ( cmd( c[0] ) == COL_NORMAL )
//    fprintf( stdout, "1: >%s< : >%s< : %s : %s\n", norm_flgs, *c, c[0], c[1] );

    switch( cmd( c[0] ) ) {
      case COL_NORMAL: if (c[1]) norm_flgs = scopy("7;0;0"); break;  // Override  LS_ICONS value
      case COL_FILE:   if (c[1]) file_flgs = scopy(c[1]); break;
      case COL_DIR:    if (c[1])  dir_flgs = scopy(c[1]); break;
      case COL_LINK:
        if (c[1]) {
          if (strcasecmp("target",c[1]) == 0) {
            linktargetcolor = true;
            link_flgs = (char *) "01;36"; /* Should never actually be used */
          } else link_flgs = scopy(c[1]);
        }
        break;
      case COL_FIFO:      if (c[1]) fifo_flgs    = scopy(c[1]); break;
      case COL_DOOR:      if (c[1]) door_flgs    = scopy(c[1]); break;
      case COL_BLK:       if (c[1]) block_flgs   = scopy(c[1]); break;
      case COL_CHR:       if (c[1]) char_flgs    = scopy(c[1]); break;
      case COL_ORPHAN:    if (c[1]) orphan_flgs  = scopy(c[1]); break;
      case COL_SOCK:      if (c[1]) sock_flgs    = scopy(c[1]); break;
      case COL_SETUID:    if (c[1]) suid_flgs    = scopy(c[1]); break;
      case COL_SETGID:    if (c[1]) sgid_flgs    = scopy(c[1]); break;
      case COL_STICKY_OTHER_WRITABLE: if (c[1]) stickyow_flgs = scopy(c[1]); break;
      case COL_OTHER_WRITABLE:        if (c[1]) otherwr_flgs  = scopy(c[1]); break;
      case COL_STICKY:    if (c[1]) sticky_flgs  = scopy(c[1]); break;
      case COL_EXEC:      if (c[1]) exec_flgs    = scopy(c[1]); break;
      case COL_MISSING:   if (c[1]) missing_flgs = scopy(c[1]); break;
      case COL_LEFTCODE:  if (c[1]) leftcode     = scopy(c[1]); break;
      case COL_RIGHTCODE: if (c[1]) rightcode    = scopy(c[1]); break;
      case COL_ENDCODE:   if (c[1]) endcode      = scopy(c[1]); break;
      case DOT_EXTENSION:
        if (c[1]) {
          e           = xmalloc(sizeof(struct extensions));
          pt = e->ext = scopy(c[0]+0);
          while ( *pt != '\0' ) { *pt = toupper( *pt ); ++pt; }   //  uppercase pattern
          e->term_flg = scopy(c[1]);
          if ( lsicons ) {
             pt = strrchr( e->term_flg, ';' );
            *pt = '\0'; ++pt;
            e->glyph    = pt;
          }
          e->nxt      = ext;
          ext         = e;
//        ext_cnt++;
        }
    }
//  if ( cmd( c[0] ) == COL_NORMAL )
//    fprintf( stdout, "2: >%s< : >%s<\n", norm_flgs, *c );
    free(c);
  }
//for ( i=0; i<ERROR; ++i ) if ( all_glyph[i] ) printf( "%3d: %s\n", i, all_glyph[i] );
//printf( "%3d: extensions\n", ext_cnt );
  free(arg);

  /* make sure at least norm_flgs is defined.  We're going to assume vt100 support */
  if (!leftcode ) leftcode  = scopy("\033[");
  if (!rightcode) rightcode = scopy("m");
  if (!norm_flgs) norm_flgs = scopy("00");

  if (!endcode) {
//  fprintf( stderr, "L: >%s<\n", leftcode );
//  fprintf( stderr, "N: >%s<\n", norm_flgs );
//  fprintf( stderr, "R: >%s<\n", rightcode );
    sprintf(buf,"%s%s%s",leftcode,"0",rightcode);
//  sprintf(buf,"%s%s%s",leftcode,norm_flgs,rightcode);
    endcode = scopy(buf);
  }

  free(colors);

  /*  if (!termmatch) colorize = false; */
}
void parse_env_colors() {    // RWM
    char *env;
extern
char *FSCOLOR,
     *TRCOLOR,
     *AGE_cols[],
     *LVL_cols[],      // colorize levels when only dirs are shown
     *COL_clr;
extern
int   AGE_ftcnt,
      LVL_cnt;         // count of LVL_cols entries
extern
unsigned long AGE_secs[];

  env = getenv( "ELS_FS_COLOR" );     // color file size
  if ( env ) asprintf( &FSCOLOR, "[%sm", env );

  // do not index array for LVL_cols, using AGE_secs as dummy field
  env = getenv( "TREE_LEVELS");       // color by level depth
  if ( env )   LVL_cnt = age_env2ft( env, ':', AGE_secs, LVL_cols );

  env = getenv( "ELS_FT_COLORS");     // color by file age
  if ( env ) AGE_ftcnt = age_env2ft( env, ':', AGE_secs, AGE_cols );

  env = getenv( "TREE_COLOR"   );     // tree color
  if ( env ) asprintf( &TRCOLOR, "[%sm", env );

  asprintf( &COL_clr, "[39;0m");
}
/*
 * You must free the pointer that is allocated by split() after you
 * are done using the array.
 */
char **split(char *str, const char *delim, int *nwrds) {
  int n = 128;
  char **w = xmalloc(sizeof(char *) * n);

  w[*nwrds = 0] = strtok(str,delim);

  while (w[*nwrds]) {
    if (*nwrds == (n-2)) w = xrealloc(w,sizeof(char *) * (n+=256));
    w[++(*nwrds)] = strtok(NULL,delim);
  }

  w[*nwrds] = NULL;
  return w;
}

int age_env2ft( char *env, char sep, unsigned long *age, char **col ) {  // RWM
  int  cnt = 0;
  char *p1, *p2;
  // routine is too sensitive on the env variable being exactly right
  // with an ending :-1;nn:
  // or it will segfault

  p1 = env;
  while ( *p1 ) {
    p2 = strchr( p1, sep );  // find separator
   *p2 = '\0';               // truncate string

   age[cnt] = strtoul( p1, NULL, 10 );
   p1 = strchr( p1, '=' );
   col[cnt++] = p1 + 1;
   p1 = p2 + 1;
  }
  return( cnt );
}

int cnt_printable(char *str ) {            // RWM
  int cnt   = 0,
      state = 0;
  char *pc;

  for ( pc = str; *pc; ++pc ) {
    if ( *pc == 033 ) {                    // Escape sequence YAY
        state = 1;
    } else if (state == 1) {
        if (('a' <= *pc && *pc <= 'z') || ('A' <= *pc && *pc <= 'Z'))
            state = 2;
    } else {
        state = 0;
        cnt++;
    }
  }

  return( cnt);
}
int cmd(char *s) {
  static struct {
    char *cmd;
    char cmdnum;
  } cmds[] = {
    {"no", COL_NORMAL},       {"fi", COL_FILE},
    {"di", COL_DIR},          {"ln", COL_LINK},
    {"pi", COL_FIFO},         {"do", COL_DOOR},
    {"bd", COL_BLK},          {"cd", COL_CHR},
    {"or", COL_ORPHAN},       {"so", COL_SOCK},
    {"su", COL_SETUID},       {"sg", COL_SETGID},
    {"tw", COL_STICKY_OTHER_WRITABLE},
    {"ow", COL_OTHER_WRITABLE},
    {"st", COL_STICKY},       {"ex", COL_EXEC},
    {"mi", COL_MISSING},      {"lc", COL_LEFTCODE},
    {"rc", COL_RIGHTCODE},    {"ec", COL_ENDCODE},
    {"NORMAL",    COL_NORMAL},   {"FILE", COL_FILE},
    {"DIRECTORY", COL_DIR},      {"LINK", COL_LINK},
    {"FIFO",      COL_FIFO},     {"DOOR", COL_DOOR},
    {"BLOCK",     COL_BLK},      {"CHARDEV", COL_CHR},
    {"ORPHAN",    COL_ORPHAN},   {"SOCKET", COL_SOCK},
    {"SETUID",    COL_SETUID},   {"SETGID", COL_SETGID},
    {"STOTHERWRITE", COL_STICKY_OTHER_WRITABLE},
    {"OTHERWRITE",   COL_OTHER_WRITABLE},
    {"STICKY",    COL_STICKY},   {"EXEC", COL_EXEC},
    {"MISSING",   COL_MISSING},
    {NULL, 0}
  };
  int i;

  if (s[0] == '*'
   || s[0] == '.'
   || s[0] == '^' ) return DOT_EXTENSION;

  for(i=0;cmds[i].cmdnum;i++) {
    if (!strcmp(cmds[i].cmd,s)) return cmds[i].cmdnum;
  }
  return DOT_EXTENSION;
  return ERROR;
}

void color_256( FILE *fout, char *code, char *glyph ) {

  if ( lsicons ) {
    int f, b, s,   // fg, bg, style
        rc;
    rc = sscanf( code, "%d;%d;%d", &f, &b, &s );
    if ( rc > 0 ) {
      fprintf( fout, "%s%d%s", forecode, f, rightcode );
      fprintf( fout,"%s  ", glyph ? glyph : "A");         // XYZZY/ - put 2 spaces after glyph ( rocket glyph needs extra space )
    }
  } else
    fprintf(outfile,"%s%s%s",leftcode,code,rightcode);
}

#ifdef GO_WILD
bool rwm_col_wild( char *fn, char y, int *b, int *f, int *s, int *i ) {
  const
  char *pls = NULL,
       *ps;
  char  pat[32],
       *pt,
       *tfn = strdup( fn );
  int   d = 0;
  Boole done = FALSE;

  // reverse search method
  //   find '*' in LSCOLOR/LSICON,
  //   get associated pattern
  //   find pattern _inside_ filename
  //   assume format of:   :*PAT=

  pt = tfn;
  while ( *pt != '\0' ) { *pt = toupper( *pt ); ++pt; }

  pls = rwm_doicons ? LSICONS : LSCOLOR;

  while ( !done ) {
    pls = strchr( pls, y );           // y='*', ''
    if ( pls ) {
      pls++;      // move past 'y' or pattern type char

//    sscanf( pls, "%31s=", pat );    // sscanf() does NOT stop at =
      ps = pls;
      pt = pat;
      int l=31;
      while ( *ps != '=' ) { *pt++ = *ps++; l--; }
      *pt = '\0';

//    printf( "Wild: |%s| [%s]\n", tfn, pat );
      if ( strstr( tfn, pat )) {      // found pat in filename
        pls = strchr( pls, '=' );
        if ( pls ) {
          if ( ! rwm_doicons )
            d=sscanf( pls, "=%d;%d;%d", b, f, s );
          else
            d=sscanf( pls, "=%d;%d;%d;%lc:", f, b, s, i );  // f / b order reversed
          done = ( d >= 2 ) ? true : false;
        }
      }
    } else done = true;
  }
  free( tfn );

  return( d > 0 );
}
#endif

int color(u_short mode, char *name, bool orphan, bool islink) {
  struct extensions *e;
  int l, xl;

  if (orphan) {
    if (islink) {
      if (missing_flgs) {
        color_256( outfile, missing_flgs, all_glyph[ COL_MISSING ] );
        return true;
      }
    } else {
      if (orphan_flgs) {
        color_256( outfile, orphan_flgs, all_glyph[ COL_ORPHAN ] );
        return true;
      }
    }
  }
  switch(mode & S_IFMT) {
    case S_IFIFO:
      if (!fifo_flgs) return false;
      color_256( outfile, fifo_flgs, all_glyph[ COL_FIFO ] );
      return true;
    case S_IFCHR:
      if (!char_flgs) return false;
      color_256( outfile, char_flgs, all_glyph[ COL_CHR ] );
      return true;
    case S_IFDIR:
      if (mode & S_ISVTX) {
        if ((mode & S_IWOTH) && stickyow_flgs) {
          color_256( outfile, stickyow_flgs, all_glyph[ COL_STICKY_OTHER_WRITABLE ] );
          return true;
        }
        if (!(mode & S_IWOTH) && sticky_flgs) {
          color_256( outfile, sticky_flgs, all_glyph[ COL_STICKY ] );
          return true;
        }
      }
      if ((mode & S_IWOTH) && otherwr_flgs) {
        color_256( outfile, otherwr_flgs, all_glyph[ COL_OTHER_WRITABLE ] );
        return true;
      }
      if (!dir_flgs) return false;
      color_256( outfile, dir_flgs, all_glyph[ COL_DIR ] );
      return true;
#ifndef __EMX__
    case S_IFBLK:
      if (!block_flgs) return false;
      color_256( outfile, block_flgs, all_glyph[ COL_BLK ] );
      return true;
    case S_IFLNK:
      if (!link_flgs) return false;
      color_256( outfile, link_flgs, all_glyph[ COL_LINK ] );
      return true;
  #ifdef S_IFDOOR
    case S_IFDOOR:
      if (!door_flgs) return false;
      color_256( outfile, door_flgs, all_glyph[ COL_DOOR ] );
      return true;
  #endif
#endif
    case S_IFSOCK:
      if (!sock_flgs) return false;
      color_256( outfile, sock_flgs, all_glyph[ COL_SOCK ] );
      return true;
    case S_IFREG:
      if ((mode & S_ISUID) && suid_flgs) {
        color_256( outfile, suid_flgs, all_glyph[ COL_SETUID ] );
        return true;
      }
      if ((mode & S_ISGID) && sgid_flgs) {
        color_256( outfile, sgid_flgs, all_glyph[ COL_SETGID ] );
        return true;
      }
      if (!exec_flgs) return false;      // needs 'ex' in LSICONS

      if ( ! exfat && mode & (S_IXUSR | S_IXGRP | S_IXOTH)) {
        color_256( outfile, exec_flgs, all_glyph[ COL_EXEC ] );
        return true;
      }

      /* not a directory, link, special device, etc, so check for extension match */
      char *pt,
           *tname = strdup( name );
      pt = tname;
      while ( *pt != '\0'  ) { *pt = toupper( *pt  ); ++pt;  }   //  uppercase pattern
      l = strlen(name);
      char *x;
      for(e=ext;e;e=e->nxt) {
        pt = e->ext;
        xl = strlen(e->ext);
        switch( *pt ) {
          case '.':
            if (!strcmp((l>xl)?tname+(l-xl):tname,e->ext)) {
//            fprintf( outfile, "<%s> ", e->ext );               // XYZZY
              color_256( outfile, e->term_flg, e->glyph );
              return true;
            }
            break;
          case '*': pt++;
          default:
            if ( xl > 2 ) {
              if ( l > xl ) x = strstr( tname, pt );
              else          x = strstr( pt, tname );
              if ( x ) {
//              fprintf( outfile, ">%d:%s< ", xl, e->ext );      // XYZZY
                color_256( outfile, e->term_flg, e->glyph );
                return true;
              }
            }
            break;
        }
      }
      // default if nothing above matched
//    fprintf( stderr, "n: >%s<\n", norm_flgs );
      if ( lsicons )
        color_256( outfile, norm_flgs, all_glyph[ COL_FILE ] );
      return false;
    }
    if ( lsicons ) fprintf( outfile, "Y " );
  return false;
}

/*
 * Charsets provided by Kyosuke Tokoro (NBG01720@nifty.ne.jp)
 */
const char *getcharset(void) {
  #ifndef __EMX__
  return getenv("TREE_CHARSET");
  #else
  static char buffer[13];
  ULONG aulCpList[3],ulListSize,codepage=0;
  char*charset=getenv("TREE_CHARSET");
  if(charset)
    return charset;

  if(!getenv("WINDOWID"))
    if(!DosQueryCp(sizeof aulCpList,aulCpList,&ulListSize))
      if(ulListSize>=sizeof*aulCpList)
        codepage=*aulCpList;

      switch(codepage){
        case 437: case 775: case 850: case 851: case 852: case 855:
        case 857: case 860: case 861: case 862: case 863: case 864:
        case 865: case 866: case 868: case 869: case 891: case 903:
        case 904:
          sprintf(buffer,"IBM%03lu",codepage);
          break;
        case 367:
          return"US-ASCII";
        case 813:
          return"ISO-8859-7";
        case 819:
          return"ISO-8859-1";
        case 881: case 882: case 883: case 884: case 885:
          sprintf(buffer,"ISO-8859-%lu",codepage-880);
          break;
        case  858: case  924:
          sprintf(buffer,"IBM%05lu",codepage);
          break;
        case 874:
          return"TIS-620";
        case 897: case 932: case 942: case 943:
          return"Shift_JIS";
        case 912:
          return"ISO-8859-2";
        case 915:
          return"ISO-8859-5";
        case 916:
          return"ISO-8859-8";
        case 949: case 970:
          return"EUC-KR";
        case 950:
          return"Big5";
        case 954:
          return"EUC-JP";
        case 1051:
          return"hp-roman8";
        case 1089:
          return"ISO-8859-6";
        case 1250: case 1251: case 1253: case 1254: case 1255: case 1256:
        case 1257: case 1258:
          sprintf(buffer,"windows-%lu",codepage);
          break;
        case 1252:
          return"ISO-8859-1-Windows-3.1-Latin-1";
        default:
          return NULL;
      }
      #endif
}

void initlinedraw(int flag) {
  static const char*latin1_3[]={
    "ISO-8859-1", "ISO-8859-1:1987", "ISO_8859-1", "latin1", "l1", "IBM819",
    "CP819", "csISOLatin1", "ISO-8859-3", "ISO_8859-3:1988", "ISO_8859-3",
    "latin3", "ls", "csISOLatin3", NULL
  };
  static const char*iso8859_789[]={
    "ISO-8859-7", "ISO_8859-7:1987", "ISO_8859-7", "ELOT_928", "ECMA-118",
    "greek", "greek8", "csISOLatinGreek", "ISO-8859-8", "ISO_8859-8:1988",
    "iso-ir-138", "ISO_8859-8", "hebrew", "csISOLatinHebrew", "ISO-8859-9",
    "ISO_8859-9:1989", "iso-ir-148", "ISO_8859-9", "latin5", "l5",
    "csISOLatin5", NULL
  };
  static const char*shift_jis[]={
    "Shift_JIS", "MS_Kanji", "csShiftJIS", NULL
  };
  static const char*euc_jp[]={
    "EUC-JP", "Extended_UNIX_Code_Packed_Format_for_Japanese",
    "csEUCPkdFmtJapanese", NULL
  };
  static const char*euc_kr[]={
    "EUC-KR", "csEUCKR", NULL
  };
  static const char*iso2022jp[]={
    "ISO-2022-JP", "csISO2022JP", "ISO-2022-JP-2", "csISO2022JP2", NULL
  };
  static const char*ibm_pc[]={
    "IBM437", "cp437", "437", "csPC8CodePage437", "IBM852", "cp852", "852",
    "csPCp852", "IBM863", "cp863", "863", "csIBM863", "IBM855", "cp855",
    "855", "csIBM855", "IBM865", "cp865", "865", "csIBM865", "IBM866",
    "cp866", "866", "csIBM866", NULL
  };
  static const char*ibm_ps2[]={
    "IBM850", "cp850", "850", "csPC850Multilingual", "IBM00858", "CCSID00858",
    "CP00858", "PC-Multilingual-850+euro", NULL
  };
  static const char*ibm_gr[]={
    "IBM869", "cp869", "869", "cp-gr", "csIBM869", NULL
  };
  static const char*gb[]={
    "GB2312", "csGB2312", NULL
  };
  static const char*utf8[]={
    "UTF-8", "utf8", NULL
  };
  static const char*big5[]={
    "Big5", "csBig5", NULL
  };
  static const char*viscii[]={
    "VISCII", "csVISCII", NULL
  };
  static const char*koi8ru[]={
    "KOI8-R", "csKOI8R", "KOI8-U", NULL
  };
  static const char*windows[]={
    "ISO-8859-1-Windows-3.1-Latin-1", "csWindows31Latin1",
    "ISO-8859-2-Windows-Latin-2", "csWindows31Latin2", "windows-1250",
    "windows-1251", "windows-1253", "windows-1254", "windows-1255",
    "windows-1256", "windows-1256", "windows-1257", NULL
  };
  static const struct linedraw cstable[]={
    { latin1_3,    "|  ",              "|--",            "&middot;--",     "&copy;", NULL, NULL, NULL, NULL, NULL,   },
    { iso8859_789, "|  ",              "|--",            "&middot;--",     "(c)",    NULL, NULL, NULL, NULL, NULL,   },
    { shift_jis,   "\204\240 ",        "\204\245",       "\204\244",       "(c)",    NULL, NULL, NULL, NULL, NULL,   },
    { euc_jp,      "\250\242 ",        "\250\247",       "\250\246",       "(c)",    NULL, NULL, NULL, NULL, NULL,   },
    { euc_kr,      "\246\242 ",        "\246\247",       "\246\246",       "(c)",    NULL, NULL, NULL, NULL, NULL,   },
    { iso2022jp,   "\033$B(\"\033(B ", "\033$B('\033(B", "\033$B(&\033(B", "(c)",    NULL, NULL, NULL, NULL, NULL,   },
    { ibm_pc,      "\263  ",           "\303\304\304",   "\300\304\304",   "(c)",    NULL, NULL, NULL, NULL, NULL,   },
    { ibm_ps2,     "\263  ",           "\303\304\304",   "\300\304\304",   "\227",   NULL, NULL, NULL, NULL, NULL,   },
    { ibm_gr,      "\263  ",           "\303\304\304",   "\300\304\304",   "\270",   NULL, NULL, NULL, NULL, NULL,   },
    { gb,          "\251\246 ",        "\251\300",       "\251\270",       "(c)",    NULL, NULL, NULL, NULL, NULL,   },
    { utf8,        "\342\224\202\302\240\302\240",
    "\342\224\234\342\224\200\342\224\200", "\342\224\224\342\224\200\342\224\200", "\302\251",    NULL, NULL, NULL, NULL, NULL, },
    { big5,        "\242x ",           "\242u",          "\242|",          "(c)",    NULL, NULL, NULL, NULL, NULL,   },
    { viscii,      "|  ",              "|--",            "`--",            "\371",   NULL, NULL, NULL, NULL, NULL,   },
    { koi8ru,      "\201  ",           "\206\200\200",   "\204\200\200",   "\277",   NULL, NULL, NULL, NULL, NULL,   },
    { windows,     "|  ",              "|--",            "`--",            "\251",   NULL, NULL, NULL, NULL, NULL,   },
    { NULL,        "|  ",              "|--",            "`--",            "(c)",    NULL, NULL, NULL, NULL, NULL,   }
  };
  const char**s;

  if (flag) {
    fprintf(stderr,"tree: missing argument to --charset, valid charsets include:\n");
    for(linedraw=cstable;linedraw->name;++linedraw) {
      for(s=linedraw->name;*s;++s) {
        fprintf(stderr,"  %s\n",*s);
      }
    }
    return;
  }
  if (charset) {
    for(linedraw=cstable;linedraw->name;++linedraw)
      for(s=linedraw->name;*s;++s)
        if(!strcasecmp(charset,*s)) return;
  }
  linedraw=cstable+sizeof cstable/sizeof*cstable-1;
}
