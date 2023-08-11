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

const
char *version ="$Version: $ tree v1.8.0 (c) 1996 - 2018 by Steve Baker, Thomas Moore, Francesc Rocher, Florian Sesser, Kyosuke Tokoro $",
     *hversion="\t\t tree v1.8.0 %s 1996 - 2018 by Steve Baker and Thomas Moore <br>\n"
                      "\t\t HTML output hacked and copyleft %s 1998 by Francesc Rocher <br>\n"
                      "\t\t JSON output hacked and copyleft %s 2014 by Florian Sesser <br>\n"
                      "\t\t Charsets / OS/2 support %s 2001 by Kyosuke Tokoro\n";

/* Globals */
bool dflag, lflag, pflag, sflag, Fflag, aflag, fflag, uflag, gflag;
bool qflag, Nflag, Qflag, Dflag, inodeflag, devflag, hflag, Rflag;
bool Hflag, siflag, cflag, Xflag, Jflag, duflag, pruneflag;
bool noindent, force_color, nocolor, xdev, noreport, nolinks, flimit;
bool ignorecase, matchdirs, fromfile, VCignore, showinfo;
bool reverse;
bool lsicons = false;
char *host = NULL;

int  pattern = 0, maxpattern  = 0,
    ipattern = 0, maxipattern = 0;
char **patterns  = NULL,
     **ipatterns = NULL;

const
char *title = "Directory Tree", *sp = " ", *_nl = "\n",
     *file_comment = "#", *file_pathsep = "/";
char *timefmt = NULL;
const char *charset = NULL;

struct _info **(*getfulltree)(char *d, u_long lev, dev_t dev, off_t *size, char **err) = unix_getfulltree;
off_t (*listdir)(char *, int *, int *, u_long, dev_t) = unix_listdir;
int (*basesort)(struct _info **a, struct _info **b) = alnumsort;
int (*topsort) () = NULL;

char *sLevel, *curdir, *outfilename = NULL;
FILE *outfile;
long long Level;
int *dirs, maxdirs;

int mb_cur_max;

// RWM - My globals
char *FSCOLOR = (char *) "", // file size color
     *TRCOLOR = (char *) "", // tree color set string
     *AGE_cols[32],          // file age  colors
     *LVL_cols[32],          // indent level colors
      lvl_colr[32] = {"\0"}, // current level color with escape codes
     *COL_clr = (char *) ""; //           color clear string
int   AGE_ftcnt = 0,
      LVL_cnt   = 0;
unsigned long AGE_secs[32];  // 32 date colors
time_t The_Time; // = time( NULL );
off_t dusize = 0;
const char *TF[] = { "False", "True" },
           *firstdir = ".";
bool showsym = false;

#ifdef __EMX__
const u_short ifmt[]={ FILE_ARCHIVED, FILE_DIRECTORY, FILE_SYSTEM, FILE_HIDDEN, FILE_READONLY, 0};
#else
  #ifdef S_IFPORT
  const u_int ifmt[] = {S_IFREG, S_IFDIR, S_IFLNK, S_IFCHR, S_IFBLK, S_IFSOCK, S_IFIFO, S_IFDOOR, S_IFPORT, 0};
  const char fmt[] = "-dlcbspDP?";
  const char *ftype[] = {"file", "directory", "link", "char", "block", "socket", "fifo", "door", "port", "unknown", NULL};
  #else
  const u_int ifmt[] = {S_IFREG, S_IFDIR, S_IFLNK, S_IFCHR, S_IFBLK, S_IFSOCK, S_IFIFO, 0};
  const char fmt[] = "-dlcbsp?";
  const char *ftype[] = {"file", "directory", "link", "char", "block", "socket", "fifo", "unknown", NULL};
  #endif
#endif

struct sorts {
  char *name;
  int (*cmpfunc)();
} sorts[] = {
  {"name",    alnumsort},
  {"version",   versort},
  {"size",    fsizesort},
  {"mtime",   mtimesort},
  {"ctime",   ctimesort},
  {NULL, NULL}
};

/* Externs */
/* hash.c */
extern struct xtable *gtable[256], *utable[256];
extern struct inotable *itable[256];
/* color.c */
extern bool colorize, ansilines, linktargetcolor;
extern char *leftcode, *rightcode, *endcode;
extern const struct linedraw *linedraw;

bool  RMisdir     ( const char *name ) {
  struct stat sbuf;
  bool        isdir = false;

  if (stat(name,&sbuf) == 0)
    if ( S_ISDIR( sbuf.st_mode ) ) isdir = true;

  return( isdir );
}

int main(int argc, char **argv)
{
  char **dirname = NULL;
  int i,j=0,k,n,optf,p,q,dtotal,ftotal,colored = false;
  struct stat st;
  char sizebuf[64], *stmp;
  off_t size = 0;
  mode_t mt;
  bool needfulltree;

  q = p = dtotal = ftotal = 0;
  aflag = dflag = fflag = lflag = pflag = sflag = Fflag = uflag = gflag = false;
  Dflag = qflag = Nflag = Qflag = Rflag = hflag = Hflag = siflag = cflag = false;
  noindent   = force_color = nocolor = xdev = noreport = nolinks = reverse = false;
  ignorecase = matchdirs = inodeflag = devflag = Xflag = Jflag = false;
  duflag     = true;
  pruneflag  = false;
  VCignore   = false;
  showinfo   = false;
  flimit = 0;
  dirs = (int *) xmalloc(sizeof(int) * (maxdirs=PATH_MAX));
  memset(dirs, 0, sizeof(int) * maxdirs);
  dirs[0] = 0;
  Level = -1;

  setlocale(LC_CTYPE, "");
  setlocale(LC_COLLATE, "");

  charset = getcharset();
  if (charset == NULL
    && ( strcmp( nl_langinfo(CODESET), "UTF-8" ) == 0
      || strcmp( nl_langinfo(CODESET), "utf8"  ) == 0 ) ) {
    charset = "UTF-8";
  }

// Still a hack, but assume if the macro is defined, we use it
#ifdef MB_CUR_MAX
  mb_cur_max = (int)MB_CUR_MAX;
#else
  mb_cur_max = 1;
#endif

  memset(utable,0,sizeof(utable));
  memset(gtable,0,sizeof(gtable));
  memset(itable,0,sizeof(itable));

  optf = true;
  for(n=i=1;i<argc;i=n) {
    n++;
    if (optf && argv[i][0] == '-' && argv[i][1]) {
      for(j=1;argv[i][j];j++) {
        switch(argv[i][j]) {
        case 'N': Nflag = true; break;
        case 'q': qflag = true; break;
        case 'Q': Qflag = true; break;
        case 'd': dflag = true; break;
        case 'l': lflag = true; break;
        case 's': sflag = true; break;
        case 'h': hflag = true;
                  sflag = true; break;  /* Assume they also want -s */
        case 'u': uflag = true; break;
        case 'g': gflag = true; break;
        case 'f': fflag = true; break;
        case 'F': Fflag = true; break;
        case 'a': aflag = true; break;
        case 'p': pflag = true; break;
        case 'i': noindent    = true;
                  _nl         = "";   break;
        case 'C': force_color = true; break;
        case 'n': nocolor     = true; break;
        case 'x': xdev        = true; break;
        case 'P': if (argv[n] == NULL) {
                    fprintf(stderr,"tree: missing argument to -P option.\n");
                    exit(1);
                  }
                  if ( pattern >= maxpattern-1 )
                    patterns = xrealloc( patterns, sizeof( char *) * (maxpattern += 10 ) );
                  patterns[ pattern++ ] = argv[n++];
                  patterns[ pattern   ] = NULL;
                  break;
        case 'I': if (argv[n] == NULL) {
                    fprintf(stderr,"tree: missing argument to -I option.\n");
                    exit(1);
                  }
                  if ( ipattern >= maxipattern-1 )
                    ipatterns = xrealloc( ipatterns, sizeof( char *) * (maxipattern += 10 ) );
                  ipatterns[ ipattern++ ] = argv[n++];
                  ipatterns[ ipattern   ] = NULL;
                  break;
        case 'A': ansilines = true;      break;
        case 'S': charset   = "IBM437";  break;
        case 'D': Dflag     = true;      break;
        case 't': basesort  = mtimesort; break;
        case 'c': basesort  = ctimesort;
                  cflag     = true;      break;
        case 'r': reverse   = true;      break;
        case 'v': basesort  = versort;   break;
        case 'U': basesort  = NULL;      break;
        case 'X': Hflag     = false;
                  Xflag     = true;      break;
        case 'J': Jflag     = true;      break;
        case 'H': Hflag     = true;
                  Xflag     = false;
                  if (argv[n] == NULL) {
                    fprintf(stderr,"tree: missing argument to -H option.\n");
                    exit(1);
                  }
                  host = argv[n++];
                  sp = "&nbsp;";         break;
        case 'T': if (argv[n] == NULL) {
                    fprintf(stderr,"tree: missing argument to -T option.\n");
                    exit(1);
                  }
                  title     = argv[n++]; break;
        case 'R': Rflag     = true;      break;
        case 'L': if ((sLevel = argv[n++]) == NULL) {
                    fprintf(stderr,"tree: Missing argument to -L option.\n");
                    exit(1);
                  }
                  Level = strtoul(sLevel,NULL,0)-1;
                  if (Level < 0) {
                    fprintf(stderr,"tree: Invalid level, must be greater than 0.\n");
                    exit(1);
                  }
                  break;
        case 'o': if (argv[n] == NULL) {
                    fprintf(stderr,"tree: missing argument to -o option.\n");
                    exit(1);
                  }
                  outfilename = argv[n++];
                  break;
        case '-':
          if (j == 1) {
            if (!strcmp("--", argv[i])) {
              optf = false;
              break;
            }
            if (!strcmp("--help",argv[i])) {
              usage(2);
              exit(0);
            }
            if (!strcmp("--version",argv[i])) {
              char *v = (char *) version+12;
              printf("%.*s\n",(int)strlen(v)-1,v);
              exit(0);
            }
            if (!strcmp("--inodes",argv[i])) {
              j = strlen(argv[i])-1;
              inodeflag=true;
              break;
            }
            if (!strcmp("--device",argv[i])) {
              j = strlen(argv[i])-1;
              devflag=true;
              break;
            }
            if (!strcmp("--noreport",argv[i])) {
              j = strlen(argv[i])-1;
              noreport = true;
              break;
            }
            if (!strcmp("--nolinks",argv[i])) {
              j = strlen(argv[i])-1;
              nolinks = true;
              break;
            }
            if (!strcmp("--dirsfirst",argv[i])) {
              j = strlen(argv[i])-1;
              topsort = dirsfirst;
              break;
            }
            if (!strcmp("--filesfirst", argv[i])) {
              j = strlen(argv[i])-1;
              topsort = filesfirst;
              break;

            }
            if (!strncmp("--filelimit",argv[i],11)) {
              j = 11;
              if (*(argv[i]+11) == '=') {
                if (*(argv[i]+12)) {
                  flimit=atoi(argv[i]+12);
                  j = strlen(argv[i])-1;
                  break;
                } else {
                  fprintf(stderr,"tree: missing argument to --filelimit=\n");
                  exit(1);
                }
              }
              if (argv[n] != NULL) {
                flimit = atoi(argv[n++]);
                j = strlen(argv[i])-1;
              } else {
                fprintf(stderr,"tree: missing argument to --filelimit\n");
                exit(1);
              }
              break;
            }
            if (!strncmp("--charset",argv[i],9)){
              j = 9;
              if (*(argv[i]+j) == '=') {
                if (*(charset = (argv[i]+10))) {
                  j = strlen(argv[i])-1;
                  break;
                } else {
                  fprintf(stderr,"tree: missing argument to --charset=\n");
                  exit(1);
                }
              }
              if (argv[n] != NULL) {
                charset = argv[n++];
                j = strlen(argv[i])-1;
              } else {
                initlinedraw(1);
                exit(1);
              }
              break;
            }
            if (!strncmp("--si", argv[i], 4)) {
              j = strlen(argv[i])-1;
              sflag = true;
              hflag = true;
              siflag = true;
              break;
            }
            if (!strncmp("--du",argv[i],4)) {
              j = strlen(argv[i])-1;
//     sflag = true;
              duflag = false;
              break;
            }
            if (!strncmp("--prune",argv[i],7)) {
              j = strlen(argv[i])-1;
              pruneflag = true;
              break;
            }
            if (!strncmp("--sym", argv[i],5)) {
              j = strlen(argv[i])-1;
              showsym = true;
              break;
            }
            if (!strncmp("--timefmt",argv[i],9)) {
              j = 9;
              if (*(argv[i]+j) == '=') {
                if (*(argv[i]+ (++j))) {
                  timefmt=scopy(argv[i]+j);
                  j = strlen(argv[i])-1;
                  break;
                }else {
                  fprintf(stderr,"tree: missing argument to --timefmt=\n");
                  exit(1);
                }
              } else if (argv[n] != NULL) {
                timefmt = scopy(argv[n]);
                n++;
                j = strlen(argv[i])-1;
              } else {
                fprintf(stderr,"tree: missing argument to --timefmt\n");
                exit(1);
              }
              Dflag = true;
              break;
            }
            if (!strncmp("--ignore-case",argv[i],13)) {
              j = strlen(argv[i])-1;
              ignorecase = true;
              break;
            }
            if (!strncmp("--matchdirs",argv[i],11)) {
              j = strlen(argv[i])-1;
              matchdirs = true;
              break;
            }
            if (!strncmp("--sort",argv[i],6)) {
              j = 6;
              if (*(argv[i]+j) == '=') {
                if (*(argv[i]+(++j))) {
                  stmp = argv[i]+j;
                  j = strlen(argv[i])-1;
                } else {
                  fprintf(stderr,"tree: missing argument to --sort=\n");
                  exit(1);
                }
              } else if (argv[n] != NULL) {
                stmp = argv[n++];
              } else {
                fprintf(stderr,"tree: missing argument to --sort\n");
                exit(1);
              }
              basesort = NULL;
              for(k=0;sorts[k].name;k++) {
                if (strcasecmp(sorts[k].name,stmp) == 0) {
                  basesort = sorts[k].cmpfunc;
                  break;
                }
              }
              if (basesort == NULL) {
                fprintf(stderr,"tree: sort type '%s' not valid, should be one of: ", stmp);
                for(k=0; sorts[k].name; k++)
                  printf("%s%c", sorts[k].name, sorts[k+1].name? ',': '\n');
                exit(1);
              }
              break;
            }
            if (!strncmp("--fromfile",argv[i],10)) {
              j = strlen(argv[i])-1;
              fromfile=true;
              break;
            }

            if ( !strncmp( "--vcignore", argv[i], 10 ) ) {
              j = strlen( argv[i] ) - 1;
              VCignore = true;
              break;
            }

            if ( !strncmp( "--info",     argv[i],  6 ) ) {
              j = strlen( argv[i] ) - 1;
              showinfo = true;
              break;
            }

            fprintf(stderr,"tree: Invalid argument `%s'.\n",argv[i]);
            usage(1);
            exit(1);
          }
        default:
          fprintf(stderr,"tree: Invalid argument -`%c'.\n",argv[i][j]);
          usage(1);
          exit(1);
          break;
        }
      }
    } else {
      if ( RMisdir ( argv[i] )) {
        if (!dirname) dirname = (char **)xmalloc(sizeof(char *) * (q=MINIT));
        else if (p == (q-2)) dirname = (char **)xrealloc(dirname,sizeof(char *) * (q+=MINC));
        dirname[p++] = scopy(argv[i]);
      }
    }
  }
  if (p) dirname[p] = NULL;

  if ( !nocolor && !Hflag && !Jflag && ! Xflag ) parse_env_colors();
  The_Time = time( NULL );

  if (outfilename == NULL) {
#ifdef __EMX__
    _fsetmode(outfile=stdout,Hflag?"b":"t");
#else
    outfile = stdout;
#endif
  } else {
#ifdef __EMX__
    outfile = fopen(outfilename,Hflag?"wb":"wt");
#else
    outfile = fopen(outfilename,"w");
#endif
    if (outfile == NULL) {
      fprintf(stderr,"tree: invalid filename '%s'\n",outfilename);
      exit(1);
    }
  }


  if (timefmt)
    setlocale(LC_TIME,"");

  parse_dir_colors();
  initlinedraw(0);

  // Insure sensible defaults and sanity check options:
  if ( dirname == NULL ) {
    dirname = xmalloc( sizeof( char * ) * 2 );
    dirname[0] = scopy( "." );
    dirname[1] = NULL;
  }
  if ( topsort == NULL ) topsort = basesort;
  if ( timefmt ) setlocale( LC_TIME, "" );
  if ( dflag )   pruneflag = false;       // else You'll just get nothing
  if ( Rflag && ( Level == -1 ) ) Rflag = false;

  // Not going to implement git configs so no core.excludesFile support.
  if ( VCignore &&  ( stmp = getenv( "GIT_DIR" ) ) ) {
    char *path = xmalloc( PATH_MAX );
    snprintf( path, PATH_MAX, "%s/info/exclude", stmp );
//  XYZZY - this is dead until you update filter.c
//  push_filterstack( new_ignorefile( path ) );
    free( path );
  }
  if ( showinfo ) push_infostack( new_infofile( INFO_PATH ) );

  needfulltree = duflag || pruneflag || matchdirs || fromfile;

  if (fromfile) {
    getfulltree = file_getfulltree;
  }

  /* Set our listdir function and sanity check options. */
  if (Hflag) {
    listdir = needfulltree ? html_rlistdir : html_listdir;
    Xflag = false;
  } else if (Xflag) {
    listdir = needfulltree ? xml_rlistdir : xml_listdir;
    colorize = false;
    colored = false; /* Do people want colored XML output? */
  } else if (Jflag) {
    listdir = needfulltree ? json_rlistdir : json_listdir;
    colorize = false;
    colored = false; /* Do people want colored JSON output? */
  } else {
    listdir = needfulltree ? unix_rlistdir : unix_listdir;
  }
  if (dflag) pruneflag = false; /* You'll just get nothing otherwise. */

  if (Rflag && (Level == -1))
    Rflag = false;

  if (Hflag) {
    emit_html_header(charset, (char *) title, (char *) version);

    fflag = false;
    if (nolinks) {
      if (force_color) fprintf(outfile, "<b class=\"NORM\">%s</b>",host);
      else fprintf(outfile,"%s",host);
    } else {
      if (force_color) fprintf(outfile,"<a class=\"NORM\" href=\"%s\">%s</a>",host,host);
      else fprintf(outfile,"<a href=\"%s\">%s</a>",host,host);
    }
    curdir = gnu_getcwd();
  } else if (Xflag) {
    fprintf(outfile,"<?xml version=\"1.0\"");
    if (charset) fprintf(outfile," encoding=\"%s\"",charset);
    fprintf(outfile,"?>%s<tree>%s",_nl,_nl);
  } else if (Jflag)
    fputc('[',outfile);

//fprintf( outfile, "Tree: " );         // RWM: only output for plain text

  // This block outputs the working directory at the top - RWM
  firstdir = "."; // curdir;   // dirname[0];
  firstdir = get_dirname( firstdir );
  if (colorize) colored = fprintf( outfile, "%s", TRCOLOR );
  fprintf(outfile, "%s\n",  firstdir );
  if (colored) fprintf(outfile,"%s",COL_clr);
  // --- End Block

  if (dirname) {
    firstdir = dirname[0];
    for(colored=i=0;dirname[i];i++,colored=0) {
      if (fflag) {
        do {
          j=strlen(dirname[i]);
          if (j > 1 && dirname[i][j-1] == '/') dirname[i][--j] = 0;
        } while (j > 1 && dirname[i][j-1] == '/');
      }
      if ((n = lstat(dirname[i],&st)) >= 0) {
        saveino(st.st_ino, st.st_dev);
        if (colorize) colored = color(st.st_mode,dirname[i],n<0,false);
        size += st.st_size;
      }
      if (Xflag || Jflag) {
        mt = st.st_mode & S_IFMT;
        for(j=0;ifmt[j];j++)
          if (ifmt[j] == mt) break;
        if (Xflag)
          fprintf(outfile,"%s<%s name=\"%s\">", noindent?"":"  ", ftype[j], dirname[i]);
        else if (Jflag) {
          if (i) fprintf(outfile, ",");
          fprintf(outfile,"%s{\"type\":\"%s\",\"name\":\"%s\",\"contents\":[", noindent?"":"\n  ", ftype[j], dirname[i]);
        }
      } else if (!Hflag) printit(dirname[i]);             // XYZZY - output starting directory
      if (colored) fprintf(outfile,"%s",endcode);
//    printf( "\tListDir: %s\n", dirname[i] );            // WALDO
      if (!Hflag) size += listdir(dirname[i],&dtotal,&ftotal,0,0);
      else {
        if (chdir(dirname[i])) {
          fprintf(outfile,"%s [error opening dir]\n",dirname[i]);
          exit(1);
        } else {
//        printf( "\tLISTDir: %s\n", "." );               // WALDO
          size += listdir(".",&dtotal,&ftotal,0,0);
//        printf( "Size: %lld (.)\n", size );             // WALDO
          chdir(curdir);
        }
      }
      if (Xflag) fprintf(outfile,"%s</%s>\n",noindent?"":"  ", ftype[j]);
      if (Jflag) fprintf(outfile,"%s]}",noindent?"":"  ");
    }
  } else {
    if ((n = lstat(".",&st)) >= 0) {
      saveino(st.st_ino, st.st_dev);
      if (colorize) colored = color(st.st_mode,".",n<0,false);
      size = st.st_size;
    }
    if (Xflag) fprintf(outfile,"%s<directory name=\".\">",noindent?"":"  ");
    else if (Jflag) fprintf(outfile, "{\"type\":\"directory\",\"name\": \".\",\"contents\":[");
    else if (!Hflag) fprintf(outfile,".");
    if (colored) fprintf(outfile,"%s",endcode);
//  printf( "\tLISTdIR: %s\n", "." );                     // WALDO
    size += listdir(".",&dtotal,&ftotal,0,0);
//  printf( "SIZE: %lld (.)\n", size );                   // WALDO
    if (Xflag) fprintf(outfile,"%s</directory>%s",noindent?"":"  ", _nl);
    if (Jflag) fprintf(outfile,"%s]}",noindent?"":"  ");
  }

  if (Hflag)
    fprintf(outfile,"\t<br><br>\n\t</p>\n\t<p>\n");

  if (!noreport) {
    if (Xflag) {
      fprintf(outfile,"%s<report>%s",noindent?"":"  ", _nl);
      if (duflag) fprintf(outfile,"%s<size>%lld</size>%s", noindent?"":"    ", (long long int)size, _nl);
      fprintf(outfile,"%s<directories>%d</directories>%s", noindent?"":"    ", dtotal, _nl);
      if (!dflag) fprintf(outfile,"%s<files>%d</files>%s", noindent?"":"    ", ftotal, _nl);
      fprintf(outfile,"%s</report>%s",noindent?"":"  ", _nl);
    } else if (Jflag) {
      fprintf(outfile, ",%s{\"type\":\"report\"",noindent?"":"\n  ");
      if (duflag) fprintf(outfile,",\"size\":%lld", (long long int)size);
      fprintf(outfile,",\"directories\":%d", dtotal);
      if (!dflag) fprintf(outfile,",\"files\":%d", ftotal);
      fprintf(outfile, "}");
    } else {
      if (duflag) {
        psize(sizebuf, size);
        fprintf(outfile,"\n%s%s used in ", sizebuf, hflag || siflag? "" : " bytes");
      } else fputc('\n', outfile);
      if (dflag)
        fprintf(outfile,"%s%d%s director%s\n",FSCOLOR, dtotal,COL_clr, (dtotal==1? "y":"ies"));
      else
        fprintf(outfile,"%s%d%s director%s, %s%d%s file%s\n",
          FSCOLOR, dtotal,COL_clr, (dtotal==1? "y":"ies"),FSCOLOR,ftotal,COL_clr,(ftotal==1? "":"s"));
    }
  }

  if (Hflag) {
    fprintf(outfile,"\t<br><br>\n\t</p>\n");
    fprintf(outfile,"\t<hr>\n");
    fprintf(outfile,"\t<p class=\"VERSION\">\n");
    fprintf(outfile,hversion,linedraw->copy, linedraw->copy, linedraw->copy, linedraw->copy);
    fprintf(outfile,"\t</p>\n");
    fprintf(outfile,"</body>\n");
    fprintf(outfile,"</html>\n");
  } else if (Xflag) {
    fprintf(outfile,"</tree>\n");
  } else if (Jflag) {
      fprintf(outfile, "%s]\n",_nl);
  }

  if (outfilename != NULL) fclose(outfile);

  return 0;
}

void usage(int n)
{
  /*     123456789!123456789!123456789!123456789!123456789!123456789!123456789!123456789! */
  /*     \t9!123456789!123456789!123456789!123456789!123456789!123456789!123456789! */
  fprintf(n < 2? stderr: stdout,
        "usage: tree [-acdfghilnpqrstuvxACDFJQNSUX] [-H baseHREF] [-T title ]\n"
        "\t[-L level [-R]] [-P pattern] [-I pattern] [-o filename] [--version]\n"
        "\t[--help] [--inodes]\n"
        "\t[--device] [--noreport] [--nolinks] [--dirsfirst] [--filesfirst]\n"
        "\t[--charset charset] [--filelimit[=]#] [--si] [--timefmt[=]<f>]\n"
        "\t[--sort[=]<name>] [--matchdirs] [--ignore-case] [--fromfile] [--]\n"
        "\t[<directory list>]\n");
  if (n < 2) return;
  fprintf(stdout,
        "  ------- Listing options -------\n"
        "  -a            All files are listed.\n"
        "  -d            List directories only.\n"
        "  -l            Follow symbolic links like directories.\n"
        "  -f            Print the full path prefix for each file.\n"
        "  -x            Stay on current filesystem only.\n"
        "  -L level      Descend only level directories deep.\n"
        "  -R            Rerun tree when max dir level reached.\n"
        "  -P pattern    List only those files that match the pattern given.\n"
        "  -I pattern    Do not list files that match the given pattern.\n"
        "  --ignore-case Ignore case when pattern matching.\n"
        "  --matchdirs   Include directory names in -P pattern matching.\n"
        "  --noreport    Turn off file/directory count at end of tree listing.\n"
        "  --charset X   Use charset X for terminal/HTML and indentation line output.\n"
        "  --filelimit # Do not descend dirs with more than # files in them.\n"
        "  --timefmt <f> Print and format time according to the format <f>.\n"
        "  -o filename   Output to file instead of stdout.\n"
        "  ------- File options -------\n"
        "  -q            Print non-printable characters as '?'.\n"
        "  -N            Print non-printable characters as is.\n"
        "  -Q            Quote filenames with double quotes.\n"
        "  -p            Print the protections for each file.\n"
        "  -u            Displays file owner or UID number.\n"
        "  -g            Displays file group owner or GID number.\n"
        "  -s            Print the size in bytes of each file.\n"
        "  -h            Print the size in a more human readable way.\n"
        "  --si          Like -h, but use in SI units (powers of 1000).\n"
        "  -D            Print the date of last modification or (-c) status change.\n"
        "  -F            Appends '/', '=', '*', '@', '|' or '>' as per ls -F.\n"
        "  --inodes      Print inode number of each file.\n"
        "  --device      Print device ID number to which each file belongs.\n"
        "  ------- Sorting options -------\n"
        "  -v            Sort files alphanumerically by version.\n"
        "  -t            Sort files by last modification time.\n"
        "  -c            Sort files by last status change time.\n"
        "  -U            Leave files unsorted.\n"
        "  -r            Reverse the order of the sort.\n"
        "  --dirsfirst   List directories before files (-U disables).\n"
        "  --filesfirst  List files before directories (-U disables).\n"
        "  --sort X      Select sort: name,version,size,mtime,ctime.\n"
        "  ------- Graphics options -------\n"
        "  -i            Don't print indentation lines.\n"
        "  -A            Print ANSI lines graphic indentation lines.\n"
        "  -S            Print with CP437 (console) graphics indentation lines.\n"
        "  -n            Turn colorization off always (-C overrides).\n"
        "  -C            Turn colorization on always.\n"
        "  ------- XML/HTML/JSON options -------\n"
        "  -X            Prints out an XML representation of the tree.\n"
        "  -J            Prints out an JSON representation of the tree.\n"
        "  -H baseHREF   Prints out HTML format with baseHREF as top directory.\n"
        "  -T string     Replace the default HTML title and H1 header with string.\n"
        "  --nolinks     Turn off hyperlinks in HTML output.\n"
        "  ------- Input options -------\n"
        "  --fromfile    Reads paths from files (.=stdin)\n"
        "  ------- Miscellaneous options -------\n"
        "  --du          Disable 'du' sizes for directories\n"
        "  --sym         Show symlink destination\n"
        "  --version     Print version and exit.\n"
        "  --help        Print usage and this help message and exit.\n"
        "  --            Options processing terminator.\n");
  exit(0);
}


int patignore( char *name ) {
  for ( int i=0; i < ipattern; i++ )
    if ( patmatch( name, ipatterns[i] ) ) return 1;
  return 0;
}
int patinclude( char *name ) {
  for ( int i=0; i < pattern; i++ )
    if ( patmatch( name, patterns[i] ) ) return 1;
  return 0;
}
struct _info *getinfo( char *name, char *path ) {
  static char *lbuf = NULL;
  static int   lbufsize = 0;
  struct _info *ent;
  struct stat st, lst;
  int    len, rs;

  if ( lbuf == NULL ) lbuf = xmalloc( lbufsize = PATH_MAX );

//if ( lstat( path, &lst ) < 0 ) printf( "UNEXPECTED NULL\n" ); // WALDO
  if ( lstat( path, &lst ) < 0 ) return NULL;

  if ( ( lst.st_mode & S_IFMT ) == S_IFLNK ) {
    if ( ( rs = stat( path, &st ) ) < 0 ) memset( &st, 0, sizeof( st ) );
  } else {
    rs = 0;
    st.st_mode = lst.st_mode;
    st.st_dev  = lst.st_dev;
    st.st_ino  = lst.st_ino;
  }

#ifndef __EMX__
//if ( VCignore && filtercheck( path, name ) ) printf( "Unexpected return\n" ); // WALDO
  if ( VCignore && filtercheck( path, name ) ) return NULL;

  if ( ( lst.st_mode & S_IFMT ) != S_IFDIR && !( lflag && ( ( st.st_mode & S_IFMT ) == S_IFDIR ) ) ) {
//  if ( pattern && !patinclude( name ) ) printf( "Unexpected Return\n" ); // Waldo
    if ( pattern && !patinclude( name ) ) return NULL;
  }
//if ( ipattern && patignore( name ) ) printf( "UNEXPECTED return\n" ); // Waldo
  if ( ipattern && patignore( name ) ) return NULL;
#endif

//if ( dflag && ( ( st.st_mode & S_IFMT ) != S_IFDIR ) ) printf( "NotADir\n" ); // Waldo
  if ( dflag && ( ( st.st_mode & S_IFMT ) != S_IFDIR ) ) return NULL;

  ent = ( struct _info * ) xmalloc( sizeof( struct _info ) );
  memset( ent, 0, sizeof( struct _info ) );

  // Should incorporate struct stat _info and eliminate this unnecessary copying.
  // Made sense long ago when there were fewer options and didn't need half of stat.

  ent->name   = scopy( name );
  ent->mode   = lst.st_mode;
  ent->uid    = lst.st_uid;
  ent->gid    = lst.st_gid;
  ent->size   = lst.st_size;
  ent->dev    =  st.st_dev;
  ent->inode  =  st.st_ino;
  ent->ldev   = lst.st_dev;
  ent->linode = lst.st_ino;
  ent->lnk    = NULL;
  ent->orphan = false;
  ent->err    = NULL;
  ent->child  = NULL;

  ent->atime  = lst.st_atime;
  ent->ctime  = lst.st_ctime;
  ent->mtime  = lst.st_mtime;

#ifdef __EMX__
  ent->attr   = lst.st_attr;
#else
  // These should be eliminated as they're barely used
  ent->isdir  = ( ( st.st_mode &   S_IFMT ) == S_IFDIR  );
  ent->issok  = ( ( st.st_mode &   S_IFMT ) == S_IFSOCK );
  ent->isfifo = ( ( st.st_mode &   S_IFMT ) == S_IFIFO  );
  ent->isexe  = (   st.st_mode & ( S_IFMT | S_IXGRP | S_IXOTH ) ) ? 1 : 0;

  if ( ( lst.st_mode & S_IFMT  ) == S_IFLNK ) {
    if ( lst.st_size+1 > lbufsize ) lbuf = xrealloc( lbuf, lbufsize = ( lst.st_size + 8192 ) );
    if ( ( len = readlink( path, lbuf, lbufsize - 1 ) ) < 0 ) {
      ent->lnk     = scopy( "[Error reading symbolic link information]" );
      ent->isdir   = false;
    } else {
      lbuf[ len ] = 0;
      ent->lnk = scopy( lbuf );
      if ( rs < 0 ) ent->orphan = true;
    }
    ent->lnkmode = st.st_mode;

  }
#endif

  ent->comment = NULL;

  return ent;
}

struct _info **read_dir(char *dir, int *n, int infotop ) {
  struct comment *com;
  static char *path = NULL;
  static u_long pathsize;
  struct _info **dl, *info;
  struct dirent *ent;
  DIR *d;
  int ne, p = 0, i,
      es = ( dir[ strlen( dir ) -1 ] == '/' );

  if (path == NULL) {
    path = xmalloc( pathsize = strlen( dir ) + PATH_MAX );
  }

  *n = -1;
//printf( "DIR: %s\n", dir );                                                      // WALDO
  if ((d=opendir(dir)) == NULL) return NULL;

  dl = (struct _info **)xmalloc(sizeof(struct _info *) * (ne = MINIT));

  while((ent = (struct dirent *)readdir(d))) {
    if (!strcmp("..",ent->d_name) || !strcmp(".",ent->d_name)) continue;
    if (Hflag && !strcmp(ent->d_name,"00Tree.html")) continue;
    if (!aflag && ent->d_name[0] == '.') continue;

    if (strlen(dir)+strlen(ent->d_name)+2 > pathsize) path = xrealloc(path,pathsize=(strlen(dir)+strlen(ent->d_name)+PATH_MAX));
    if ( es ) sprintf( path, "%s%s",  dir, ent->d_name );
    else      sprintf( path, "%s/%s", dir, ent->d_name );

    info = getinfo( ent->d_name, path );
    if ( info ) {
//    printf( "%s -> %s -> %s\n", ent->d_name, path, info ? "DATA" : "NULL" );     // WALDO
      if ( showinfo && ( com = infocheck( path, ent->d_name, infotop )) ) {
        for ( i = 0; com->desc[ i ] != NULL; i++ );
        info->comment = xmalloc( sizeof( char * ) * ( i+1) );
        for ( i = 0; com->desc[ i ] != NULL; i++ ) info->comment[i] = scopy( com->desc[i] );
        info->comment[i] = NULL;
      }
      if ( p == ( ne-1 ) ) dl = ( struct _info ** ) xrealloc( dl, sizeof( struct _info * ) * ( ne += MINC ));
      dl [ p++ ] = info;
    }
  }

  closedir( d );


  if ( ( *n = p ) == 0 ) {
    free( dl );
    return NULL;
  }

  dl[ p ] = NULL;

  return dl;

}
void push_files( char *dir, struct ignorefile **ig, struct infofile **inf ) {
  if ( VCignore ) {
    *ig = new_ignorefile( dir );
    if ( *ig != NULL ) push_filterstack( *ig );
  }
  if ( showinfo ) {
    *inf = new_infofile( dir );
    if ( *inf != NULL ) push_infostack( *inf );
  }
}

/* This is for all the impossible things people wanted the old tree to do.
 * This can and will use a large amount of memory for large directory trees
 * and also take some time.
 */
struct _info **unix_getfulltree(char *d, u_long lev, dev_t dev, off_t *size, char **err)
{
  char *path;
  u_long pathsize = 0;
  struct ignorefile *ig  = NULL;
  struct infofile   *inf = NULL;
  struct _info **dir, **sav, **p, *sp;
  struct stat sb;
  u_long lev_tmp;
  int n,
      tmp_pattern = 0;
  char *start_rel_path;

  *err = NULL;
  if (Level >= 0 && lev > (u_long) Level) return NULL;
  if (xdev && lev == 0) {
    stat(d,&sb);
    dev = sb.st_dev;
  }
  // if the directory name matches, turn off pattern matching for contents
  if (matchdirs && pattern) {
    lev_tmp = lev;
    start_rel_path = d + strlen(d);
    for (start_rel_path = d + strlen(d); start_rel_path != d; --start_rel_path) {
      if (*start_rel_path == '/')
        --lev_tmp;
      if (lev_tmp <= 0) {
        if (*start_rel_path)
          ++start_rel_path;
        break;
      }
    }
    if (*start_rel_path && patinclude(start_rel_path) ) {
      tmp_pattern = pattern;
      pattern = 0;
    }
  }

  push_files( d, &ig, &inf );

  sav = dir = read_dir(d, &n, inf != NULL );
//printf( "read_DIR %d: %s\n", n, dir == NULL ? "NULL" : d );  // "data" );    // WALDO
  if (tmp_pattern) {
    pattern = tmp_pattern;
    tmp_pattern = 0;
  }
  if (dir == NULL && n < 0 ) {
    *err = scopy("error opening dir");                          // RODDENBERRY
//  errors++;
    return NULL;
  }
  if (n == 0) {
    if ( sav != NULL ) free_dir(sav);
    return NULL;
  }
  path = malloc(pathsize = PATH_MAX);

  if (flimit > 0 && n > flimit) {
    sprintf(path,"%d entries exceeds filelimit, not opening dir",n);
    *err = scopy(path);
    free_dir(sav);
    free(path);
    return NULL;
  }

//if (cmpfunc) qsort(dir,n,sizeof(struct _info *),cmpfunc);

  if (lev >= (u_long) maxdirs-1) {
    dirs = xrealloc(dirs,sizeof(int) * (maxdirs += 1024));
  }

  off_t loc_dusize = dusize;

//printf( "Starting: %10lld [ %10lld %10lld ] (%s)\n", (*dir)->size, *size, dusize, (*dir)->name );

  dusize = 0;                    // RWM - reset before reading dir

  while (*dir) {
    if ((*dir)->isdir && !(xdev && dev != (*dir)->dev)) {
      if ((*dir)->lnk) {
        if (lflag) {
          if (findino((*dir)->inode,(*dir)->dev)) {
            (*dir)->err = scopy("recursive, not followed");
          } else {
            saveino((*dir)->inode, (*dir)->dev);
            if (*(*dir)->lnk == '/')
              (*dir)->child = unix_getfulltree((*dir)->lnk,lev+1,dev,&((*dir)->size),&((*dir)->err));
            else {
              if (strlen(d)+strlen((*dir)->lnk)+2 > pathsize) path=xrealloc(path,pathsize=(strlen(d)+strlen((*dir)->name)+1024));
              if (fflag && !strcmp(d,"/")) sprintf(path,"%s%s",d,(*dir)->lnk);
              else sprintf(path,"%s/%s",d,(*dir)->lnk);
              (*dir)->child = unix_getfulltree(path,lev+1,dev,&((*dir)->size),&((*dir)->err));
            }
          }
        }
      } else {
        if (strlen(d)+strlen((*dir)->name)+2 > pathsize) path=xrealloc(path,pathsize=(strlen(d)+strlen((*dir)->name)+1024));
        if (fflag && !strcmp(d,"/")) sprintf(path,"%s%s",d,(*dir)->name);
        else sprintf(path,"%s/%s",d,(*dir)->name);
        saveino((*dir)->inode, (*dir)->dev);
        (*dir)->child = unix_getfulltree(path,lev+1,dev,&((*dir)->size),&((*dir)->err));
      }
      // prune empty folders, unless they match the requested pattern
      if (pruneflag && (*dir)->child == NULL &&
          !(matchdirs && pattern && patinclude((*dir)->name) )) {
        sp = *dir;
        for(p=dir;*p;p++) *p = *(p+1);
        n--;
        free(sp->name);
        if (sp->lnk) free(sp->lnk);
        free(sp);
        continue;
      }
    }
//  if (duflag) *size += (*dir)->size
    if ( duflag) {
//    printf( "Updating: %10lld %10lld [ %10lld %10lld : %10lld ] (%s)\n",
//      (*dir)->size, *size, dusize, loc_dusize, (*dir)->size, (*dir)->name );
      if ( dflag ) dusize += (*dir)->size;
      (*dir)->size += dusize;
      *size        += (*dir)->size;
//    dusize        = 0;
    }
    dir++;
  }
  dusize = loc_dusize;


  // sorting needs to be deferred for --du
//if ( basesort ) qsort( sav, n, sizeof( struct _info * ), basesort );
  if ( topsort  ) qsort( sav, n, sizeof( struct _info * ), topsort );

  free(path);
  if (n == 0) {
    free_dir(sav);
    return NULL;
  }

  if ( ig  != NULL ) pop_filterstack();
  if ( inf != NULL ) pop_infostack  ();
  return sav;
}

/* Sorting functions */
// filesfirst and dirsfirst are now top-level meta-sorts.
int filesfirst( struct _info **a, struct _info **b ) {
  if ( ( *a )->isdir != ( *b )->isdir ) {
    return ( *a )->isdir ? 1 : -1;
  }
  return( basesort( a, b ) );
}
int dirsfirst( struct _info **a, struct _info **b ) {
  if ( ( *a )->isdir != ( *b )->isdir ) {
    return ( *a )->isdir ? -1 : 1;
  }
  return( basesort( a, b ) );
}
int alnumsort(struct _info **a, struct _info **b)
{
  int v = strcoll((*a)->name,(*b)->name);
  return reverse? -v : v;
}

int versort(struct _info **a, struct _info **b)
{
  int v = strverscmp((*a)->name,(*b)->name);
  return reverse? -v : v;
}

int mtimesort(struct _info **a, struct _info **b)
{
  int v;

  if ((*a)->mtime == (*b)->mtime) {
    v = strcoll((*a)->name,(*b)->name);
    return reverse? -v : v;
  }
  v =  (*a)->mtime == (*b)->mtime? 0 : ((*a)->mtime < (*b)->mtime ? -1 : 1);
  return reverse? -v : v;
}

int ctimesort(struct _info **a, struct _info **b)
{
  int v;

  if ((*a)->ctime == (*b)->ctime) {
    v = strcoll((*a)->name,(*b)->name);
    return reverse? -v : v;
  }
  v = (*a)->ctime == (*b)->ctime? 0 : ((*a)->ctime < (*b)->ctime? -1 : 1);
  return reverse? -v : v;
}

int sizecmp(off_t a, off_t b)
{
  return (a == b)? 0 : ((a < b)? 1 : -1);
}

int fsizesort(struct _info **a, struct _info **b)
{
  int v = sizecmp((*a)->size, (*b)->size);
  if (v == 0) v = strcoll((*a)->name,(*b)->name);
  return reverse? -v : v;
}


void *xmalloc (size_t size)
{
  register void *value = malloc (size);
  if (value == 0) {
    fprintf(stderr,"tree: virtual memory exhausted.\n");
    exit(1);
  }
  return value;
}

void *xrealloc (void *ptr, size_t size)
{
  register void *value = realloc (ptr,size);
  if (value == 0) {
    fprintf(stderr,"tree: virtual memory exhausted.\n");
    exit(1);
  }
  return value;
}

void free_dir(struct _info **d)
{
  int i;

  for(i=0;d[i];i++) {
    free(d[i]->name);
    if (d[i]->lnk) free(d[i]->lnk);
    free(d[i]);
  }
  free(d);
}

char *gnu_getcwd()
{
  int size = 100;
  char *buffer = (char *) xmalloc (size);

  while (1)
    {
      char *value = getcwd (buffer, size);
      if (value != 0)
        return buffer;
      size *= 2;
      free (buffer);
      buffer = (char *) xmalloc (size);
    }
}

static inline char cond_lower(char c)
{
  return ignorecase ? tolower(c) : c;
}

/*
 * Patmatch() code courtesy of Thomas Moore (dark@mama.indstate.edu)
 * '|' support added by David MacMahon (davidm@astron.Berkeley.EDU)
 * Case insensitive support added by Jason A. Donenfeld (Jason@zx2c4.com)
 * returns:
 *    1 on a match
 *    0 on a mismatch
 *   -1 on a syntax error in the pattern
 */
int patmatch(char *buf, char *pat)
{
  int match = 1,m,n;
  char *bar = strchr(pat, '|');

  /* If a bar is found, call patmatch recursively on the two sub-patterns */

  if (bar) {
    /* If the bar is the first or last character, it's a syntax error */
    if (bar == pat || !bar[1]) {
      return -1;
    }
    /* Break pattern into two sub-patterns */
    *bar = '\0';
    match = patmatch(buf, pat);
    if (!match) {
      match = patmatch(buf,bar+1);
    }
    /* Join sub-patterns back into one pattern */
    *bar = '|';
    return match;
  }

  while(*pat && match) {
    switch(*pat) {
    case '[':
      pat++;
      if(*pat != '^') {
        n = 1;
        match = 0;
      } else {
        pat++;
        n = 0;
      }
      while(*pat != ']'){
        if(*pat == '\\') pat++;
        if(!*pat /* || *pat == '/' */ ) return -1;
        if(pat[1] == '-'){
          m = *pat;
          pat += 2;
          if(*pat == '\\' && *pat)
            pat++;
          if(cond_lower(*buf) >= cond_lower(m) && cond_lower(*buf) <= cond_lower(*pat))
            match = n;
          if(!*pat)
            pat--;
        } else if(cond_lower(*buf) == cond_lower(*pat)) match = n;
        pat++;
      }
      buf++;
      break;
    case '*':
      pat++;
      if(!*pat) return 1;
      while(*buf && !(match = patmatch(buf++,pat)));
      return match;
    case '?':
      if(!*buf) return 0;
      buf++;
      break;
    case '\\':
      if(*pat)
        pat++;
    default:
      match = (cond_lower(*buf++) == cond_lower(*pat));
      break;
    }
    pat++;
    if(match<1) return match;
  }
  if(!*buf) return match;
  return 0;
}


/**
 * They cried out for ANSI-lines (not really), but here they are, as an option
 * for the xterm and console capable among you, as a run-time option.
 */
void indent(int maxlevel)
{
  int i;
  char *col = TRCOLOR;

  if ( LVL_cnt ) {
    sprintf( lvl_colr, "[%sm", LVL_cols[ maxlevel < LVL_cnt ? maxlevel : LVL_cnt-1 ] );
    col = lvl_colr;
  }

  if (ansilines) {
    if (dirs[0]) fprintf(outfile,"%s\033(0%s", col, COL_clr );
    for(i=0; (i <= maxlevel) && dirs[i]; i++) {
      if (dirs[i+1]) {
        if (dirs[i] == 1) fprintf(outfile,"%s\170%s   ", col, COL_clr );
        else printf("    ");
      } else {
        if (dirs[i] == 1) fprintf(outfile,"%s\164\161\161%s ", col, COL_clr );
        else fprintf(outfile,"%s\155\161\161%s ", col, COL_clr );
      }
    }
    if (dirs[0]) fprintf(outfile,"%s\033(B%s", col, COL_clr );
  } else {
    if (Hflag) fprintf(outfile,"\t");
    for(i=0; (i <= maxlevel) && dirs[i]; i++) {
      if ( LVL_cnt )
        sprintf( lvl_colr, "[%sm", LVL_cols[ i < LVL_cnt ? i : LVL_cnt-1 ] );

      fprintf(outfile,"%s%s%s ",
              col,
              dirs[i+1] ? (dirs[i]==1 ? linedraw->vert     : (Hflag? "&nbsp;&nbsp;&nbsp;" : "   ") )
                        : (dirs[i]==1 ? linedraw->vert_left:linedraw->corner),
              COL_clr );
    }
  }
}


#ifdef __EMX__
// char *prot(long m) {     // XYZZY - 2019-12-26 commented only because having tis break 'splitfunc'
#else
char *prot(mode_t m) {
#endif
#ifdef __EMX__
  const u_short *p;
  static char buf[6];
  char*cp;

  for(p=ifmt,cp=strcpy(buf,"adshr");*cp;++p,++cp)
    if(!(m&*p))
      *cp='-';
#else
  static char buf[11], perms[] = "rwxrwxrwx";
  int i, b;

  for(i=0;ifmt[i] && (m&S_IFMT) != ifmt[i];i++);
  buf[0] = fmt[i];

  /**
   * Nice, but maybe not so portable, it is should be no less portable than the
   * old code.
   */
  for(b=S_IRUSR,i=0; i<9; b>>=1,i++)
    buf[i+1] = (m & (b)) ? perms[i] : '-';
  if (m & S_ISUID) buf[3] = (buf[3]=='-')? 'S' : 's';
  if (m & S_ISGID) buf[6] = (buf[6]=='-')? 'S' : 's';
  if (m & S_ISVTX) buf[9] = (buf[9]=='-')? 'T' : 't';

  buf[10] = 0;
#endif
  return buf;
}

#define SIXMONTHS (6*31*24*60*60)

char *do_date(time_t t)
{
  static char buf[256],
              tmp[256];
  struct tm *tm;
  int i=0;
  unsigned long f_age = (unsigned long) The_Time - (unsigned long) t;

  while ( i<AGE_ftcnt && AGE_secs[i] < f_age ) i++;

  tm = localtime(&t);

  if (timefmt) {
    strftime(tmp,255,timefmt,tm);
    buf[255] = 0;
  } else {
    time_t c = time(0);
    /* Use strftime() so that locale is respected: */
    if (t > c || (t+SIXMONTHS) < c)
      strftime(tmp,255,"%b %e  %Y", tm);
    else
      strftime(tmp,255,"%b %e %R", tm);
  }

  if ( i < AGE_ftcnt ) snprintf(buf,255,"[%sm%s%s",  AGE_cols[i], tmp, COL_clr );
  else strcpy( buf, tmp );

  return buf;
}

/**
 * Must fix this someday
 */
void printit(char *s)
{
  int c;

  if (Nflag) {
    if (Qflag) fprintf(outfile, "\"%s\"",s);
    else fprintf(outfile,"%s",s);
    return;
  }
  if (mb_cur_max > 1) {
    wchar_t *ws, *tp;
    ws = xmalloc(sizeof(wchar_t)* (c=(strlen(s)+1)));
    if (mbstowcs(ws,s,c) != (size_t)-1) {
      if (Qflag) putc('"',outfile);
      for(tp=ws;*tp && c > 1;tp++, c--) {
        if (iswprint(*tp)) fprintf(outfile,"%lc",(wint_t)*tp);
        else {
          if (qflag) putc('?',outfile);
          else fprintf(outfile,"\\%03o",(unsigned int)*tp);
        }
      }
      if (Qflag) putc('"',outfile);
      free(ws);
      return;
    }
    free(ws);
  }
  if (Qflag) putc('"',outfile);
  for(;*s;s++) {
    c = (unsigned char)*s;
#ifdef __EMX__
    if(_nls_is_dbcs_lead(*(unsigned char*)s)){
      putc(*s,outfile);
      putc(*++s,outfile);
      continue;
    }
#endif
    if((c >= 7 && c <= 13) || c == '\\' || (c == '"' && Qflag) || (c == ' ' && !Qflag)) {
      putc('\\',outfile);
      if (c > 13) putc(c, outfile);
      else putc("abtnvfr"[c-7], outfile);
    } else if (isprint(c)) putc(c,outfile);
    else {
      if (qflag) {
        if (mb_cur_max > 1 && c > 127) putc(c,outfile);
        else putc('?',outfile);
      } else fprintf(outfile,"\\%03o",c);
    }
  }
  if (Qflag) putc('"',outfile);
}

int psize(char *buf, off_t size)
{
  static char *iec_unit="BKMGTPEZY", *si_unit = "dkMGTPEZY";
  char *unit = siflag ? si_unit : iec_unit;
  int idx, usize = siflag ? 1000 : 1024;

  if (hflag || siflag) {
    for (idx=size<usize?0:1; size >= (usize*usize); idx++,size/=usize);
//  if (!idx) return sprintf(buf, " %4d", (int)size);
//  else return sprintf(buf, ((size/usize) >= 10)? " %3.0f%c" : " %3.1f%c" , (float)size/(float)usize,unit[idx]);
    if (!idx) return sprintf(buf, " %s%4d %s", FSCOLOR, (int)size, COL_clr );
    else return sprintf(buf, ((size/usize) >= 10)? " %s%4.0f%c%s" : " %s%4.1f%c%s" ,
        FSCOLOR, (float)size/(float)usize,unit[idx], COL_clr );
  } else return sprintf(buf, sizeof(off_t) == sizeof(long long)? " %s%11lld%s" : " %s%9lld%s",
     FSCOLOR, (long long int)size, COL_clr );
}

char Ftype(mode_t mode)
{
  int m = mode & S_IFMT;
  if (!dflag && m == S_IFDIR) return '/';
  else if (m == S_IFSOCK) return '=';
  else if (m == S_IFIFO) return '|';
  else if (m == S_IFLNK) return '@'; /* Here, but never actually used though. */
#ifdef S_IFDOOR
  else if (m == S_IFDOOR) return '>';
#endif
  else if ((m == S_IFREG) && (mode & (S_IXUSR | S_IXGRP | S_IXOTH))) return '*';
  return 0;
}

char *fillinfo(char *buf, struct _info *ent)
{
  int n;
  buf[n=0] = 0;
  #ifdef __USE_FILE_OFFSET64
  if (inodeflag) n += sprintf(buf," %7lld",(long long)ent->linode);
  #else
  if (inodeflag) n += sprintf(buf," %7ld",(long int)ent->linode);
  #endif
  if (devflag) n += sprintf(buf+n, " %3d", (int)ent->ldev);
  #ifdef __EMX__
  if (pflag) n += sprintf(buf+n, " %s",prot(ent->attr));
  #else
  if (pflag) n += sprintf(buf+n, " %s", prot(ent->mode));
  #endif
  if (uflag) n += sprintf(buf+n, " %-8.32s", uidtoname(ent->uid));
  if (gflag) n += sprintf(buf+n, " %-8.32s", gidtoname(ent->gid));
  if (sflag) n += psize(buf+n,ent->size);
  if (Dflag) n += sprintf(buf+n, " %s", do_date(cflag? ent->ctime : ent->mtime));

  return buf;
}
