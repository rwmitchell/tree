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

extern bool dflag, lflag, pflag, sflag, Fflag, aflag, fflag, uflag, gflag;
extern bool Dflag, inodeflag, devflag, Rflag, duflag, pruneflag;
extern bool noindent, force_color, xdev, nolinks, flimit;

extern struct _info **(*getfulltree)(char *d, u_long lev, dev_t dev, off_t *size, char **err);
extern void (*listdir)(char *, int *, int *, u_long, dev_t);
extern int (*topsort)();
extern FILE *outfile;
extern int Level, *dirs, maxdirs;

extern bool colorize, linktargetcolor,
            showsym;
extern char *endcode;
extern char *firstdir,  // RWM
            *TRCOLOR,
            *COL_clr,
             lvl_colr[];

char *get_dirname( const char *name ) {
  int rc;
  char *oname,
       *nname;

  oname = (char *) malloc( MAXNAMLEN );
  nname = (char *) malloc( MAXNAMLEN );

  if ( !oname || !nname ) { fprintf(stderr, "get_dirname(%s): failed to allocate space\n", name); exit(0); }
  getcwd( oname, MAXNAMLEN );
  rc = chdir( name );
  if ( rc < 0 ) { fprintf( stderr, "get_dirname(%s): chdir failed\n", name ); }
  getcwd( nname, MAXNAMLEN );
  chdir( oname );   // return to starting point

  return( nname );
}

off_t unix_listdir(char *d, int *dt, int *ft, u_long lev, dev_t dev)
{
  char *path;
  bool nlf = false, colored = false;
  long pathsize = 0;
  struct _info **dir, **sav;
  struct stat sb;
  int n, c;

  if ((Level >= 0) && (lev > (u_long) Level)) {
    fputc('\n',outfile);
    return 0;
  }

  if (xdev && lev == 0) {
    stat(d,&sb);
    dev = sb.st_dev;
  }

  sav = dir = read_dir(d, &n, false );  // inf != NULL );

//printf( "\nREAD_DIR: %s: %d : %s\n", d, n, dir ? "DATA" : "Null" );

  if (!dir && n) {
    fprintf(outfile," [Error opening dir: %d:%s]\n", n, d);
    return 0;
  }
  if ( !dir && n == 0 ) { fprintf( outfile, "\n" ); return 0; }  // no subdirs

  if ( sav && n <= 0 ) {
    fputc('\n', outfile);
    free_dir(sav);
    return 0;
  }
  if (flimit > 0 && n > flimit) {
    fprintf(outfile," [%d entries exceeds filelimit, not opening dir]\n",n);
    free_dir(sav);
    return 0;
  }

  if (topsort) qsort(dir,n,sizeof(struct _info *), topsort);
  if (lev >= (u_long) maxdirs-1) {
    dirs = (int *) xrealloc(dirs,sizeof(int) * (maxdirs += 1024));
    memset(dirs+(maxdirs-1024), 0, sizeof(int) * 1024);
  }
//printf( "DIRS: %s -> %s : %ld\n", dirs ? "DATA" : "Null", dir ? "Data" : "NULL", lev );
  dirs[lev] = 1;
  if (!*(dir+1)) dirs[lev] = 2;
  fprintf(outfile,"\n");

  path = (char *) malloc(pathsize=PATH_MAX);

  while(*dir) {

    fillinfo(path,*dir);
    if ( firstdir ) {
      int n = cnt_printable( path );
      n += n>0 ? 3 : 0;     // add 3 if n is > 0
      if (colorize) colored = color( S_IFDIR, firstdir, false, false);
      firstdir = get_dirname( firstdir );
      fprintf(outfile, "%*s%s - MARK\n", n, n ? " " : "", firstdir );
      if (colored) fprintf(outfile,"%s",endcode);
      free( firstdir );
      firstdir = NULL;
    }
    if (path[0] == ' ') {
      path[0] = '[';
      fprintf(outfile, "%s]  ",path);
    }

    if (!noindent) indent(lev);

    if (colorize) {
      if ((*dir)->lnk && linktargetcolor) colored = color((*dir)->lnkmode,(*dir)->name,(*dir)->orphan,false);
      else colored = color((*dir)->mode,(*dir)->name,(*dir)->orphan,false);
    }

    if (fflag) {
      if (sizeof(char) * (strlen(d)+strlen((*dir)->name)+2) > (u_long) pathsize)
        path = (char *) xrealloc(path,pathsize=(sizeof(char) * (strlen(d)+strlen((*dir)->name)+1024)));
      if (!strcmp(d,"/")) sprintf(path,"%s%s",d,(*dir)->name);
      else sprintf(path,"%s/%s",d,(*dir)->name);
    } else {
      if (sizeof(char) * (strlen((*dir)->name)+1) > (u_long) pathsize)
        path = (char *) xrealloc(path,pathsize=(sizeof(char) * (strlen((*dir)->name)+1024)));
      sprintf(path,"%s",(*dir)->name);
    }

    printit(path);

    if (colored) fprintf(outfile,"%s",endcode);
    if (Fflag && !(*dir)->lnk) {
      if ((c = Ftype((*dir)->mode))) fputc(c, outfile);
    }

    if ((*dir)->lnk && showsym ) {
      fprintf(outfile," -> ");
      if (colorize) colored = color((*dir)->lnkmode,(*dir)->lnk,(*dir)->orphan,true);
      printit((*dir)->lnk);
      if (colored) fprintf(outfile,"%s",endcode);
      if (Fflag) {
        if ((c = Ftype((*dir)->lnkmode))) fputc(c, outfile);
      }
    }

    if ((*dir)->isdir) {
      if ((*dir)->lnk) {
        if (lflag && !(xdev && dev != (*dir)->dev)) {
          if (findino((*dir)->inode,(*dir)->dev)) {
            fprintf(outfile,"  [recursive, not followed]");
          } else {
            saveino((*dir)->inode, (*dir)->dev);
            if (*(*dir)->lnk == '/')
              listdir((*dir)->lnk,dt,ft,lev+1,dev);
            else {
              if (strlen(d)+strlen((*dir)->lnk)+2 > (u_long) pathsize)
                path = (char *) xrealloc(path,pathsize=(strlen(d)+strlen((*dir)->name)+1024));
              if (fflag && !strcmp(d,"/")) sprintf(path,"%s%s",d,(*dir)->lnk);
              else sprintf(path,"%s/%s",d,(*dir)->lnk);
              listdir(path,dt,ft,lev+1,dev);
            }
            nlf = true;
          }
        }
      } else if (!(xdev && dev != (*dir)->dev)) {
        if (strlen(d)+strlen((*dir)->name)+2 > (u_long) pathsize)
          path = (char *) xrealloc(path,pathsize=(strlen(d)+strlen((*dir)->name)+1024));
        if (fflag && !strcmp(d,"/")) sprintf(path,"%s%s",d,(*dir)->name);
        else sprintf(path,"%s/%s",d,(*dir)->name);
        saveino((*dir)->inode, (*dir)->dev);
        listdir(path,dt,ft,lev+1,dev);
        nlf = true;
      }
      *dt += 1;
    } else *ft += 1;
    if (*(dir+1) && !*(dir+2)) dirs[lev] = 2;
    if (nlf) nlf = false;
    else fprintf(outfile,"\n");
    dir++;
  }
  dirs[lev] = 0;
  free(path);
  free_dir(sav);
  return 0;
}

off_t unix_rlistdir(char *d, int *dt, int *ft, u_long lev, dev_t dev)
{
  struct _info **dir;
  off_t size = 0;
  char *err;

  dir = getfulltree(d, lev, dev, &size, &err);

  memset(dirs, 0, sizeof(int) * maxdirs);

  r_listdir(dir, d, dt, ft, lev);

  return size;
}

void r_listdir(struct _info **dir, char *d, int *dt, int *ft, u_long lev)
{
  char *path;
  long pathsize = 0;
  struct _info **sav = dir;
  bool nlf = false, colored = false;
  int c;

  if (dir == NULL) return;

  dirs[lev] = 1;
  if (!*(dir+1)) dirs[lev] = 2;
  fprintf(outfile,"\n");

  path = (char *) malloc(pathsize=PATH_MAX);

  while(*dir) {

    fillinfo(path,*dir);                 // rwm - XYZZY mark
    if ( firstdir ) {
      int n = cnt_printable( path );
      n += n>0 ? 3 : 0;     // add 3 if n is > 0
#ifdef  USE_PROPER_COLOR
      if (colorize) colored = color( S_IFDIR, firstdir, false, false);
#else
      if (colorize) colored = fprintf( outfile, "%s", TRCOLOR );
#endif
      firstdir = get_dirname( firstdir );
//    fprintf(outfile, "%*s%s\n", n, n ? " " : "", firstdir );     // rwm XYZZY - prints full path near top
#ifdef  USE_PROPER_COLOR
      if (colored) fprintf(outfile,"%s",endcode);
#else
      if (colored) fprintf(outfile,"%s",COL_clr);
#endif
      free( firstdir );
      firstdir = NULL;
    }
    if (path[0] == ' ') {
      path[0] = '[';
      fprintf(outfile, "%s]  ",path);
    }

    if (!noindent) indent(lev);

    if (colorize) {
      if ((*dir)->lnk && linktargetcolor) colored = color((*dir)->lnkmode,(*dir)->name,(*dir)->orphan,false);
      else {
        // use LSCOLOR if showing files or dir is a symlink
        if ( !dflag || (*dir)->lnk )      colored = color((*dir)->mode,   (*dir)->name,(*dir)->orphan,false);
        else {
          color( (*dir)->mode, (*dir)->name, (*dir)->orphan, false );
          colored = fprintf(outfile, "%s ", lvl_colr);
        }
      }
    }

    if (fflag) {
      if (sizeof(char) * (strlen(d)+strlen((*dir)->name)+2) > (u_long) pathsize)
        path = (char *) xrealloc(path,pathsize=(sizeof(char) * (strlen(d)+strlen((*dir)->name)+1024)));
      if (!strcmp(d,"/")) sprintf(path,"%s%s",d,(*dir)->name);
      else sprintf(path,"%s/%s",d,(*dir)->name);
    } else {
      if (sizeof(char) * (strlen((*dir)->name)+1) > (u_long) pathsize)
        path = (char *) xrealloc(path,pathsize=(sizeof(char) * (strlen((*dir)->name)+1024)));
      sprintf(path,"%s",             (*dir)->name);
    }

    printit(path);

    if (colored) fprintf(outfile,"%s",endcode);
    if (Fflag && !(*dir)->lnk) {
      if ((c = Ftype((*dir)->mode))) fputc(c, outfile);
    }

    if ((*dir)->lnk && showsym ) {
      fprintf(outfile," -> ");
      if (colorize) colored = color((*dir)->lnkmode,(*dir)->lnk,(*dir)->orphan,true);
      printit((*dir)->lnk);
      if (colored) fprintf(outfile,"%s",endcode);
      if (Fflag) {
        if ((c = Ftype((*dir)->lnkmode))) fputc(c, outfile);
      }
    }

    if ((*dir)->err) {
      fprintf(outfile," [%s]", (*dir)->err);
      free((*dir)->err);
      (*dir)->err = NULL;
    }
    if ((*dir)->child) {
      if (fflag) {
        if (strlen(d)+strlen((*dir)->name)+2 > (u_long) pathsize)
          path = (char *) xrealloc(path,pathsize=(strlen(d)+strlen((*dir)->name)+1024));
        if (!strcmp(d,"/")) sprintf(path,"%s%s",d,(*dir)->name);
        else sprintf(path,"%s/%s",d,(*dir)->name);
      }
      r_listdir((*dir)->child, fflag? path : NULL, dt, ft, lev+1);
      nlf = true;
      *dt += 1;
    } else {
      if ((*dir)->isdir) *dt += 1;
      else *ft += 1;
    }

    if (*(dir+1) && !*(dir+2)) dirs[lev] = 2;
    if (nlf) nlf = false;
    else fprintf(outfile,"\n");
    dir++;
  }
  dirs[lev] = 0;
  free(path);
  free_dir(sav);
}
