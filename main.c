#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <unistd.h>
#include <signal.h>
#include <sys/ioctl.h>

#include "qdinp2.h"

#include "ascpaint.h"

int numedits = 0;
char *filename = NULL;
rows rs;
int insertmode = 0;
int toolmode = TOOL_EDIT;
char palettemode = '#';
char inversepalettemode = ' ';

void cleanscr()
{
  freers_subs(&rs);
  printf("\033[?1049h\033[2J\033[?1049l\033[?25h\033[1;1H");
}

void on_die(int i)
{
  exit(1);
}

void on_resize(int i)
{
  getscreensize(&rs);
  
  //Repaint screen
  UpdateDisplay();
}

int maxrowlength(txtrow *arowset, int numrows)
{
  int i, maxn = -1;
  for (i=0;i<numrows;i++)
  {
//    if (arowset[i].rowtext && strlen(arowset[i].rowtext) > maxn)
//      maxn = strlen(arowset[i].rowtext);
    if (arowset[i].rowtext && arowset[i].rowsize > maxn)
      maxn = arowset[i].rowsize;
  }
  return maxn;
}

int minrowlength(txtrow *arowset, int numrows)
{
  int i, minn = -1;
  for (i=0;i<numrows;i++)
  {
//    if (arowset[i].rowtext && (strlen(arowset[i].rowtext) < minn || minn < 0))
//      minn = strlen(arowset[i].rowtext);
    if (arowset[i].rowtext && (arowset[i].rowsize < minn || minn < 0))
      minn = arowset[i].rowsize;
  }
  return minn;
}

void DoError(char *errortext)
{
  int dworks = 0, i;
  if (rs.displaylayer_rowset)
  {
    if (rs.dlrs_size >= rs.sheight)
    {
      //use this
      if (minrowlength(rs.displaylayer_rowset, rs.dlrs_size) >= rs.swidth)
        dworks = 1;
      else
      {
        freerowset(rs.displaylayer_rowset, rs.dlrs_size);
        rs.displaylayer_rowset = NULL;
      }
    }
    else
    {
      //free it and rebuild
      freerowset(rs.displaylayer_rowset, rs.dlrs_size);
      rs.displaylayer_rowset = NULL;
    }
  }
  if (!dworks)
  {
    //Create new display layer to build on
    if (rs.dlformat_rowset)
    {
      freerowset(rs.dlformat_rowset, rs.dlrs_size);
      rs.dlformat_rowset = NULL;
    }
    if ((rs.displaylayer_rowset = makerowset(rs.swidth,rs.sheight)) == NULL
       || (rs.dlformat_rowset = makerowset(rs.swidth,rs.sheight)) == NULL )
    {
      if (rs.displaylayer_rowset) freerowset(rs.displaylayer_rowset, rs.sheight);
      DoSimpleError(errortext);
      return;
    }
    rs.dlrs_size = rs.sheight;
  }
  else
  {
    //Clear the display layer
    for (i=0;i<rs.dlrs_size;i++)
    {
      rs.displaylayer_rowset[i].rowtext[0] = 0;
      rs.displaylayer_rowset[i].rowsize = 0;
      memset(rs.dlformat_rowset[i].rowtext,' ',rs.dlformat_rowset[i].rowsize);
    }
  }
  
  //Build the dialogue box on the display layer
  
  int etextw = strlen(errortext);
  if ((etextw & 1) != 0) etextw++;
  int etexth = (etextw / (rs.swidth-2));
  if (etextw % (rs.swidth - 2) != 0)
  {
    etexth++;
  }
  if (etextw > rs.swidth - 2)
  {
    etextw = rs.swidth - 2;
  }
  int boxh = etexth + 7;
  int boxw = etextw + 2;
  if (boxw < rs.swidth) boxw += 2;
  if (boxw < 10) boxw = 10;
  //	printf("boxw=%d",boxw);
  int boxx = (rs.swidth - boxw) / 2;
  int boxy = (rs.sheight - boxh) / 2;
  
  i = 0;
  strcpy(rs.displaylayer_rowset[i].rowtext,"+");
  int j;
  for (j=0;j<boxw-2;j++) strcat(rs.displaylayer_rowset[i].rowtext,"-");
  strcat(rs.displaylayer_rowset[i].rowtext,"+");
  rs.displaylayer_rowset[i].rowsize = strlen(rs.displaylayer_rowset[i].rowtext);
  i++;
  strcpy(rs.displaylayer_rowset[i].rowtext,"| ");
  for (j=0;j<(boxw/2)-5;j++) strcat(rs.displaylayer_rowset[i].rowtext," ");
  strcat(rs.displaylayer_rowset[i].rowtext,"ERROR!");
  for (j=0;j<(boxw/2)-5;j++) strcat(rs.displaylayer_rowset[i].rowtext," ");
  strcat(rs.displaylayer_rowset[i].rowtext," |");
  rs.displaylayer_rowset[i].rowsize = strlen(rs.displaylayer_rowset[i].rowtext);
  i++;
  strcpy(rs.displaylayer_rowset[i].rowtext,"+");
  for (j=0;j<boxw-2;j++) strcat(rs.displaylayer_rowset[i].rowtext,"-");
  strcat(rs.displaylayer_rowset[i].rowtext,"+");
  int k;
  char *tstr = (char *) malloc(sizeof(char)*(1+etextw));
  if (tstr) tstr[0] = 0;
  for (k=0; k<etexth; k++)
  {
    rs.displaylayer_rowset[i].rowsize = strlen(rs.displaylayer_rowset[i].rowtext);
    i++;
    strcpy(rs.displaylayer_rowset[i].rowtext,"|");
    for (j=0;j<((boxw-etextw)/2)-1;j++)
      strcat(rs.displaylayer_rowset[i].rowtext," ");
    if (!tstr)
    {
      strcat(rs.displaylayer_rowset[i].rowtext,errortext);
      if ((strlen(errortext) & 1) != 0)
        strcat(rs.displaylayer_rowset[i].rowtext," ");
      for (j=0;j<((boxw-etextw)/2)-1;j++)
        strcat(rs.displaylayer_rowset[i].rowtext," ");
      strcat(rs.displaylayer_rowset[i].rowtext,"|");
      break;
    }
    else
    {
      strncpy(tstr,errortext+(sizeof(char)*k*etextw),etextw);
      tstr[etextw] = 0;
      strcat(rs.displaylayer_rowset[i].rowtext,tstr);
      if ((strlen(tstr) & 1) != 0)
        strcat(rs.displaylayer_rowset[i].rowtext," ");
    }
    for (j=0;j<((boxw-etextw)/2)-1;j++)
      strcat(rs.displaylayer_rowset[i].rowtext," ");
    strcat(rs.displaylayer_rowset[i].rowtext,"|");
  }
  if (tstr) free(tstr);
  tstr = NULL;
  rs.displaylayer_rowset[i].rowsize = strlen(rs.displaylayer_rowset[i].rowtext);
  i++;
  strcpy(rs.displaylayer_rowset[i].rowtext,"|");
  for (j=0;j<((boxw-6)/2)-1;j++)
    strcat(rs.displaylayer_rowset[i].rowtext," ");
  strcat(rs.displaylayer_rowset[i].rowtext,"+----+");
  for (j=0;j<((boxw-6)/2)-1;j++)
    strcat(rs.displaylayer_rowset[i].rowtext," ");
  strcat(rs.displaylayer_rowset[i].rowtext,"|");
  rs.displaylayer_rowset[i].rowsize = strlen(rs.displaylayer_rowset[i].rowtext);
  i++;
  strcpy(rs.displaylayer_rowset[i].rowtext,"|");
  for (j=0;j<((boxw-6)/2)-1;j++)
    strcat(rs.displaylayer_rowset[i].rowtext," ");
  strcat(rs.displaylayer_rowset[i].rowtext,"| OK |");
  //Inversing:
  rs.dlformat_rowset[i].rowtext[1+((boxw-6)/2)] |= DLFORMAT_INVERT;
  rs.dlformat_rowset[i].rowtext[1+((boxw-6)/2)+1] |= DLFORMAT_INVERT;
  rs.dlformat_rowset[i].rowtext[1+((boxw-6)/2)+2] |= DLFORMAT_INVERT;
  rs.dlformat_rowset[i].rowtext[1+((boxw-6)/2)+3] |= DLFORMAT_INVERT;
  for (j=0;j<((boxw-6)/2)-1;j++)
    strcat(rs.displaylayer_rowset[i].rowtext," ");
  strcat(rs.displaylayer_rowset[i].rowtext,"|");
  rs.displaylayer_rowset[i].rowsize = strlen(rs.displaylayer_rowset[i].rowtext);
  i++;
  strcpy(rs.displaylayer_rowset[i].rowtext,"|");
  for (j=0;j<((boxw-6)/2)-1;j++)
    strcat(rs.displaylayer_rowset[i].rowtext," ");
  strcat(rs.displaylayer_rowset[i].rowtext,"+----+");
  for (j=0;j<((boxw-6)/2)-1;j++)
    strcat(rs.displaylayer_rowset[i].rowtext," ");
  strcat(rs.displaylayer_rowset[i].rowtext,"|");
  rs.displaylayer_rowset[i].rowsize = strlen(rs.displaylayer_rowset[i].rowtext);
  i++;
  strcpy(rs.displaylayer_rowset[i].rowtext,"+");
  for (j=0;j<boxw-2;j++) strcat(rs.displaylayer_rowset[i].rowtext,"-");
  strcat(rs.displaylayer_rowset[i].rowtext,"+");
  rs.displaylayer_rowset[i].rowsize = strlen(rs.displaylayer_rowset[i].rowtext);
  i++;
  for (j=i;j<rs.dlrs_size;j++)
  {
    rs.displaylayer_rowset[j].rowtext[0] = 0;
    rs.displaylayer_rowset[j].rowsize = 0;
  }
  
  //Draw this at (boxx,boxy)!
  rs.dl_x = boxx;
  rs.dl_y = boxy;
  rs.dl_cx = (boxw/2)-1;
  rs.dl_cy = 4+etexth;
  rs.show_dl = 1;
  UpdateDisplay();
  
  /*
  printf("\033[%d;%dH",boxy+4+etexth,boxx+(boxw/2)-1);
  */
  
  //... Finish this ...
  //int n;
  do
  {
    i = (getkeyn() & OK_NOMODMAX);
  }
  while (i!=10 && i!=13 && i!=OK_NENTER && i!=OK_ENL);
  
  rs.show_dl = 0;
  UpdateDisplay();
  
  
}

void DoSimpleError(char *errortext)
{
  /*
   * Text:
   * +-----------------------+
   * |         ERROR         |
   * +-----------------------+
   * |      Error Text!      |
   * |       +----+          |
   * |       | OK |          |
   * |       +----+          |
   * +-----------------------+
   *
   * Inverse = \033[7m OK \033[0m
   */
  int etextw = strlen(errortext);
  if ((etextw & 1) != 0) etextw++;
  int etexth = (etextw / (rs.swidth-2));
  if (etextw % (rs.swidth - 2) != 0)
  {
    etexth++;
  }
  if (etextw > rs.swidth - 2)
  {
    etextw = rs.swidth - 2;
  }
  //etc...
  int boxh = etexth + 7;
  int boxw = etextw + 2;
  if (boxw < rs.swidth) boxw += 2;
  if (boxw < 10) boxw = 10;
  //	printf("boxw=%d",boxw);
  int boxx = (rs.swidth - boxw) / 2;
  int boxy = (rs.sheight - boxh) / 2;
  
  int ox, oy;
  getansicursorpos(&oy, &ox);
  
  printf("\033[%d;%dH",boxy,boxx);
  int i;
  printf("+");
  for (i=0;i<boxw-2;i++) printf("-");
  printf("+");
  printf("\033[%d;%dH",boxy+1,boxx);
  printf("| ");
  for (i=0;i<(boxw/2)-5;i++) printf(" ");
  printf("ERROR!");
  for (i=0;i<(boxw/2)-5;i++) printf(" ");
  printf(" |");
  printf("\033[%d;%dH",boxy+2,boxx);
  printf("+");
  for (i=0;i<boxw-2;i++) printf("-");
  printf("+");
  int j;
  char *tstr = (char *) malloc(sizeof(char)*(1+etextw));
  if (tstr) tstr[0] = 0;
  for (j=0; j<etexth; j++)
  {
    printf("\033[%d;%dH",boxy+3+j,boxx);
    printf("|");
    for (i=0;i<((boxw-etextw)/2)-1;i++) printf(" ");
    if (!tstr)
    {
      printf("%s", errortext);
      if ((strlen(errortext) & 1) != 0) printf(" ");
      for (i=0;i<((boxw-etextw)/2)-1;i++) printf(" ");
      printf("|");
      break;
    }
    else
    {
      strncpy(tstr,errortext+(sizeof(char)*j*etextw),etextw);
      tstr[etextw] = 0;
      printf("%s", tstr);
      if ((strlen(tstr) & 1) != 0) printf(" ");
    }
    for (i=0;i<((boxw-etextw)/2)-1;i++) printf(" ");
    printf("|");
  }
  if (tstr) free(tstr);
  tstr = NULL;
  printf("\033[%d;%dH",boxy+3+etexth,boxx);
  printf("|");
  for (i=0;i<((boxw-6)/2)-1;i++) printf(" ");
  printf("+----+");
  for (i=0;i<((boxw-6)/2)-1;i++) printf(" ");
  printf("|");
  printf("\033[%d;%dH",boxy+4+etexth,boxx);
  printf("|");
  for (i=0;i<((boxw-6)/2)-1;i++) printf(" ");
  printf("|\033[7m OK \033[0m|");
  for (i=0;i<((boxw-6)/2)-1;i++) printf(" ");
  printf("|");
  printf("\033[%d;%dH",boxy+5+etexth,boxx);
  printf("|");
  for (i=0;i<((boxw-6)/2)-1;i++) printf(" ");
  printf("+----+");
  for (i=0;i<((boxw-6)/2)-1;i++) printf(" ");
  printf("|");
  printf("\033[%d;%dH",boxy+6+etexth,boxx);
  printf("+");
  for (i=0;i<boxw-2;i++) printf("-");
  printf("+");
  
  printf("\033[%d;%dH",boxy+4+etexth,boxx+(boxw/2)-1);
  
  int n;
  do
  {
    n = (getkeyn() & OK_NOMODMAX);
  }
  while (n!=10 && n!=13 && n!=OK_NENTER && n!=OK_ENL);
  
  printf("\033[%d;%dH",oy,ox);
}

void UpdateDisplay()
{
  int i,j, x, y, k, preformatted = 0, sw;
  
  sw = rs.swidth; //In case of interrupts
  char *tstr = (char *) malloc(sizeof(char)*(1+sw));
  if (tstr && rs.rowset != NULL)
  {
    memset(tstr,0,sizeof(char)*(1+sw));
    
    //Do Layer
    y = rs.view_y;
    if (y+rs.sheight>rs.rs_size && rs.rs_size>rs.sheight)
      y = rs.rs_size-rs.sheight;
    if (rs.rs_size < rs.sheight)
    {
      //y+a+(rs_size/2)=sheight .: y+a=sheight-(rs_size/2)
      //(2*a)+(rs_size)=sheight .: a=(sheight-rs_size)/2
      //.: y=sheight - (rs_size/2) - (sheight/2) - (rs_size/2)
      //    =(sheight/2) - rs_size.  QED
      y = (rs.sheight/2) - rs.rs_size;
    }
    for (i = y; i<rs.sheight; i++)
    {
      if (i+y>=0 && i+y<rs.rs_size)
      {
        x = rs.view_x;
        if (x+sw>rs.rowset[i+y].rowsize && 
            rs.rowset[i+y].rowsize>sw)
        {
          x = rs.rowset[i+y].rowsize - sw;
        }
        if (rs.rowset[i+y].rowsize < sw)
        {
          x = (sw/2) - rs.rowset[i+y].rowsize;
          j=0;
          k=rs.rowset[i+y].rowsize;
        }
        //if (x<0) 
        //{ a=0 ; b=sw
        //} else {a=x; b=x+sw}
        ////if (a==x) b=a+sw else b=sw
        //if (b+a>rowsize) b=rowsize - a
        if (x<0)
        {
          j=0;
          k=sw;
        }
        else if (x<=rs.rowset[i+y].rowsize)
        {
          j=x;
          k=x+sw;
          //k=sw-x;
        }
        //TODO: Something is wrong around here - if the screen is wider 
        // than the image it segfaults! - Think I've fixed it!
        if (j+k>rs.rowset[i+y].rowsize) k = rs.rowset[i+y].rowsize - j;
        // Where am I going with this? //if (j>rs.rowset[i+y].rowsize) 
        strncpy(tstr,rs.rowset[i+y].rowtext+(sizeof(char)*j),k);
        if (j==0 && x>0) j=x;
        else if (x<0) j=-x;
        else j=0;
        printf("\033[%d;%dH\033[0m%s",i+1,j,tstr);
        
      }
    } //End For
    
  }
  if (tstr) free(tstr);
  
  preformatted = 0;
  if (rs.show_ml && rs.movelayer_rowset != NULL)
  {
    x = rs.view_x - rs.ml_x;
    y = rs.view_y - rs.ml_y;
    for (i=0; i<rs.sheight; i++)
    {
      if (i+y>=0 && i+y<rs.mlrs_size)
      {
        printf("\033[%d;%dH\033[0m",i+1,1);
        for (j=0; j<rs.swidth; j++)
        {
          if (j+x>=0 && j+y<rs.movelayer_rowset[y+i].rowsize)
          {
            if (preformatted != 71)
            {
              printf("\033[7;1m");
              preformatted = 71;
            }
            printf("%c",rs.movelayer_rowset[y+i].rowtext[x+j]);
          }
          else
          {
            if (preformatted)
            {
              printf("\033[0m");
              preformatted = 0;
            }
          }
        } //End For
      }
      else if (preformatted)
      {
        printf("\033[0m");
        preformatted = 0;
      }
    } //End For
  } //End ML Draw
  
  //Move Cursor
  i = rs.cx - rs.view_x;
  if (i<0) i=0;
  else if (i>=sw) i = sw-1;
  j = rs.cy - rs.view_y;
  if (j<0) j=0;
  else if (j>=rs.sheight) j=rs.sheight - 1;
  printf("\033[%d;%dH\033[0m",j+1,i+1);
  
  preformatted = 0;
  if (rs.show_dl && rs.displaylayer_rowset != NULL && rs.dlformat_rowset != NULL)
  {
    //Draw the display layer
    x = rs.dl_x +1;
    y = rs.dl_y +1;
    for (i=0;i<rs.dlrs_size;i++)
    {
      if ((y+i)>=rs.sheight) break;
      printf("\033[%d;%dH\033[0m",y+i,x);
      preformatted = 0;
      for (j=0;j<rs.displaylayer_rowset[i].rowsize;j++)
      {
        
        if (j<rs.dlformat_rowset[i].rowsize && (j+x)<=rs.swidth)
        {
          if (preformatted != (rs.dlformat_rowset[i].rowtext[j] & ~(DLFORMAT_IGNORE)))
          {
            if (preformatted)
            {
              printf("\033[0m");
            }
            if ((rs.dlformat_rowset[i].rowtext[j] & DLFORMAT_INVERT) != 0)
            {
              printf("\033[7m");
            }
            if ((rs.dlformat_rowset[i].rowtext[j] & 15) != 0)
            {
              if ((rs.dlformat_rowset[i].rowtext[j] & 8) != 0)
              {
                printf("\033[9%dm", (rs.dlformat_rowset[i].rowtext[j] & 7));
              }
              else
              {
                printf("\033[3%dm", (rs.dlformat_rowset[i].rowtext[j] & 7));
              }
            }
            
            
          }
          preformatted = (rs.dlformat_rowset[i].rowtext[j] & ~(DLFORMAT_IGNORE));
        }
        else if (preformatted)
        {
          printf("\033[0m");
          preformatted = 0;
        }
        
        printf("%c",rs.displaylayer_rowset[i].rowtext[j]);
        
      }
    }
    printf("\033[%d;%dH",y+rs.dl_cy,x+rs.dl_cx);
    
  }
}

txtrow *makerowset(int width, int height)
{
  txtrow *thers = NULL;
  thers = (txtrow *) malloc(sizeof(txtrow)*height);
  if (!thers) return NULL;
  memset(thers,0,sizeof(txtrow)*height);
  int i;
  for (i=0;i<height;i++)
  {
    thers[i].rownum = i;
    thers[i].rowtext = (char *) malloc(sizeof(char)*(1+width));
    if (thers[i].rowtext == NULL)
    {
      int j;
      for (j=0;j<i;j++) free(thers[j].rowtext);
      free(thers);
      return NULL;
    }
    thers[i].allocsize = width+1;
    memset(thers[i].rowtext,' ',width);
    thers[i].rowsize = width;
  }
  return thers;
}

int getscreensize(rows *ars)
{
  int ox, oy, x, y;
  struct winsize ws;
  memset(&ws,0,sizeof(struct winsize));
  if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) != -1 && ws.ws_col != 0 && ws.ws_row != 0)
  {
    x = ws.ws_col;
    y = ws.ws_row;
    ars->swidth = x;
    ars->sheight = y;
    return 1;
  }
  if (!getansicursorpos(&oy, &ox)) return 0;
  printf("\033[9999;9999H");
  getansicursorpos(&y, &x); //For some reason, this is necessary...
  if (!getansicursorpos(&y, &x))
  {
    printf("\033[%d;%dH",oy,ox);
    return 0;
  }
  ars->swidth = x;
  ars->sheight = y;
  printf("\033[%d;%dH",oy,ox);
  //  printf("w=%d, h=%d\n",ars->swidth, ars->sheight);
  return 1;
}

int inserttextinrow(txtrow *arow, char *text, int offset)
{
  int n, paddingsize = 0;
  if (offset > arow->rowsize) paddingsize = offset - arow->rowsize;
  n = (arow->rowsize + 1 + strlen(text) + paddingsize);
  if (arow->allocsize < n)
  {
    arow->rowtext = realloc(arow->rowtext, sizeof(char)*n);
    if (!(arow->rowtext))
    {
      DoError("Out of memory!"); //Implement this!
      return 0;
    }
    arow->allocsize = n;
  }
  if (paddingsize)
  {
    //Put in the padding
    memset(arow->rowtext + (sizeof(char)*(arow->rowsize)), ' ', paddingsize);
    arow->rowsize += paddingsize + strlen(text);
    arow->rowtext[arow->rowsize] = 0;
  }
  else
  {
    //Move the text at offset by strlen(text)
    memmove(arow->rowtext + (sizeof(char)*(offset + strlen(text))),
            arow->rowtext + (sizeof(char)*offset),
            sizeof(char) * (arow->rowsize - offset));
    arow->rowsize += strlen(text);
  }
  memcpy(arow->rowtext + (sizeof(char)*offset),
         text,
         strlen(text)*sizeof(char));
  numedits++;
  return 1;
}

void freerow(txtrow *arow)
{
  if (arow->rowtext) free(arow->rowtext);
  free(arow);
}

void freerowset(txtrow *arowset, int numrows)
{
  int i;
  for (i=0;i<numrows;i++)
  {
    if (arowset[i].rowtext) free(arowset[i].rowtext);
  }
  free(arowset);
}

void freers_subs(rows *ars)
{
  if (ars->rowset) freerowset(ars->rowset,ars->rs_size);
  if (ars->movelayer_rowset) freerowset(ars->movelayer_rowset,ars->mlrs_size);
  if (ars->displaylayer_rowset) freerowset(ars->displaylayer_rowset,ars->dlrs_size);
  if (ars->dlformat_rowset) freerowset(ars->dlformat_rowset,ars->dlrs_size);
  
}

int expandtabs(char *ostr, int omax, const char *istr)
{
  //Do this when loading from a file - ASCII art should NOT contain tabs!
  int i = 0, j = 0;
  for (i=0; istr[i] != 0; i++)
  {
    if (j >= omax) break;
    if (istr[i] != '\t' && istr[i] != 11)
    {
      //Not a tab
      ostr[j] = istr[i];
      j++;
    }
    else
    {
      //Tab
      ostr[j] = ' ';
      j++;
      while (j % 8 != 0 && j < omax)
      {
        ostr[j] = ' ';
        j++;
      }
    }
  }
  ostr[j] = 0;
  if (i<strlen(istr)) return 0;
  return 1;
}

int deltextfromrow(txtrow *arow, int tlen, int offset)
{
  memmove(arow->rowtext + (sizeof(char)*offset),
          arow->rowtext + (sizeof(char)*(offset+tlen)),
          sizeof(char) * (arow->rowsize + tlen - offset)); //-1 +1
  numedits++;
  return 1;
}

int overwritetext(char *text, int x, int y, int allowsemiinsert)
{
  //Returns: 1=success, 0=Cannot fit -1=bad params, -2=OoM & data corrupted!
  if (!text) return -1;
  if (y<0 || y>=rs.rs_size) return -1;
  if (x<0 || (allowsemiinsert && x>=rs.rowset[y].rowsize)) return -1;
  if (((!allowsemiinsert) && x+strlen(text)>rs.rowset[y].rowsize)) return 0;
  if (allowsemiinsert && x+strlen(text)+1>rs.rowset[y].allocsize)
  {
    rs.rowset[y].rowtext = realloc(rs.rowset[y].rowtext, x+strlen(text)+1);
    if (!rs.rowset[y].rowtext) return -2; //OoM!
  }
  strcpy(rs.rowset[y].rowtext + (sizeof(char)*x), text);
  numedits++;
  return 1;
}

int insertrow(int at_y, char *rowtext)
{
  //Returns: 1=success, -1=bad params, -2=OoM & data corrupted!
  int i;
  if (at_y > rs.rs_size || at_y < 0) return -1;
  if (!rowtext) return -1;
  rs.rowset = realloc(rs.rowset, (rs.rs_size + 1)*sizeof(txtrow));
  if (!rs.rowset) return -2; //OoM
  if (at_y != rs.rs_size)
  {
    memmove(rs.rowset+((at_y+1)*sizeof(txtrow)), rs.rowset+(at_y*sizeof(txtrow)),sizeof(txtrow)*(rs.rs_size - at_y));
    for (i = at_y+1; i<=rs.rs_size; i++) rs.rowset[i].rownum++;
  }
  rs.rs_size++;
  memset(&(rs.rowset[at_y]), 0, sizeof(txtrow));
  rs.rowset[at_y].rowtext = (char *) malloc(sizeof(char)*(1+strlen(rowtext)));
  if (!rs.rowset[at_y].rowtext) return -2; //OoM!
  rs.rowset[at_y].allocsize = strlen(rowtext)+1;
  strcpy(rs.rowset[at_y].rowtext, rowtext);
  rs.rowset[at_y].rowsize = strlen(rowtext);
  rs.rowset[at_y].rownum = at_y;
  return 1;
}

int delrow(int at_y)
{
  //Returns: 1=success, -1=bad params, -2=OoM & data corrupted!
  if (at_y>=rs.rs_size || at_y < 0) return -1;
  int i;
  free(rs.rowset[at_y].rowtext);
  rs.rowset[at_y].rowtext = NULL;
  rs.rowset[at_y].allocsize = 0;
  rs.rowset[at_y].rowsize = 0;
  if (at_y <rs.rs_size - 1)
  {
    for (i=at_y;i<rs.rs_size - 1;i++)
    {
      memmove(rs.rowset+(i*sizeof(txtrow)), rs.rowset+((i+1)*sizeof(txtrow)), sizeof(txtrow));
      rs.rowset[i].rownum--;
    }
  }
  rs.rowset = realloc(rs.rowset, (rs.rs_size - 1)*sizeof(txtrow));
  if (!rs.rowset) return -2; //OoM and data corrupted!
  rs.rs_size--;
  return 1;
}

int breakrowtonl(int at_x, int at_y) //Insert new line
{
  //Returns: 1=success, -1=bad params, -2=OoM & data corrupted!
  if (at_y>=rs.rs_size || at_y < 0) return -1;
  if (at_x>rs.rowset[at_y].rowsize || at_x < 0) return -1;
  char *thetext = rs.rowset[at_y].rowtext;
  if (at_x==rs.rowset[at_y].rowsize)
  {
    if (insertrow(at_y+1,"\x00")!=1) return -2;
  }
  else
  {
    if (insertrow(at_y+1,thetext+(sizeof(char)*at_x))!=1) return -2;
    thetext[at_x] = 0;
    rs.rowset[at_y].rowsize = strlen(thetext);
  }
}

int joinrowatnl(int at_y) //Deletes a new line at the end of at_y
{
  //Returns: 1=success, -1=bad params, -2=OoM, -3=OoM & data corrupted!
  if (at_y>=rs.rs_size || at_y < 0) return -1;
  if (at_y == rs.rs_size) return 1; //Doesn't need a change
  txtrow *arow = &(rs.rowset[at_y]);
  if (!inserttextinrow(arow, rs.rowset[at_y+1].rowtext, arow->rowsize)) return -2;
  if (delrow(at_y+1)!=1) return -3;
  return 1;
}

int movecursor_internal(int by_x, int by_y)
//Move the cursor relative to it's current position, in rs.
{
  /*
   * Returns:
   * 0  = Did not move cursor!
   * 1  = Moved Cursor somewhat by x
   * 3  = Moved Cursor fully by x
   * 4  = Moved Cursor somewhat by y
   * 5  = Moved Cursor somewhat by x and y
   * 7  = Moved Cursor fully by x and somewhat by y
   * 12 = Moved Cursor fully by y
   * 13 = Moved Cursor fully by y and somewhat by x
   * 15 = Moved Cursor fully by x and y
   * -1 = Other Error!
   */
  int ocx, ocy, ncx, ncy;
  ocx = rs.cx;
  ocy = rs.cy;
  ncy = ocy + by_y;
  if (ncy>rs.rs_size) ncy = rs.rs_size;
  else if (ncy<0) ncy = 0;
  ncx = ocx + by_x;
  if (ncx<0) ncx = 0;
  else if (ncx>rs.rowset[ncy].rowsize) ncx = rs.rowset[ncy].rowsize;
  
  int reta, retb;
  if (ncx == ocx+by_x) reta = 3;
  else if (ncx == ocx) reta=0;
  else reta = 1;
  if (ncy == ocy+by_y) retb = 12;
  else if (ncy == ocy) retb=0;
  else retb = 4;
  return (reta | retb);
}

int movecursor(int by_x, int by_y)
//Move the cursor relative to it's current position;
{
  /*
   * Returns (an OR of):
   * 0  = Did not move cursor!
   * b0 = Moved Cursor somewhat by x
   * b1 = Moved Cursor fully by x
   * b2 = Moved Cursor somewhat by y
   * b3 = Moved Cursor fully by y
   * b4 = Updated Screen
   * -1 = Other Error!
   */
  int ocx = rs.cx, ocy = rs.cy;
  int a = movecursor_internal(by_x, by_y), b=0;
  if (a>0)
  {
    if (rs.cx>=(rs.view_x+rs.swidth) || rs.cx<rs.view_x)
    {
      rs.view_x = rs.cx - rs.swidth;
      b++;
    }
    if (rs.cy>=(rs.view_y+rs.sheight) || rs.cy<rs.view_y)
    {
      rs.view_y = rs.cy - rs.sheight;
      b++;
    }
    if (b) UpdateDisplay();
    else printf("\033[%d;%dH", rs.cy - rs.view_y, rs.cx - rs.view_x);
  }
  return (a | (b*16));
}

int main(int argc, char *argv[])
{
  int iwidth = 0, iheight = 0;
  memset(&rs,0,sizeof(rows));
  rs.rowset = NULL;
  rs.rs_size = 0;
  rs.movelayer_rowset = NULL;
  rs.mlrs_size = 0;
  rs.ml_x = 0;
  rs.ml_y = 0;
  rs.displaylayer_rowset = NULL;
  rs.dlrs_size = 0;
  getscreensize(&rs);
  
  atexit(cleanscr);
  signal(SIGTERM, on_die);
  signal(SIGWINCH, on_resize);
  printf("\033[?1049h");
  
  if (argc == 1)
  {
    //New file
    printf("New File.\n");
    char rline[255];
    int ret = 1;
    rline[0] = 0;
    do
    {
      printf("Image Width: ");
      ret = NEWreadqdline(rline,"80",255,0);
      //printf("%d\n",ret);
    }
    while (ret > 0 || !rline[0] || rline[0]==10 || rline[0]==13 || rline[0]==3);
    if (ret == -3) printf("Out of memory!\n");
    else if (ret == -1) printf("Signal Error!\n");
    if (ret < 0) exit(1);
    sscanf(rline,"%d",&iwidth);
    if (rline[strlen(rline)-1] != 10 && rline[strlen(rline)-1] != 13) printf("\n");
    //printf("Image Height: ");
    ret = 1;
    rline[0] = 0;
    do
    {
      printf("Image Height: ");
      ret = NEWreadqdline(rline,"24",255,0);
//      printf("'%s'%d %d\n",rline,rline[0], ret);
    }
    while (ret > 0 || !rline[0] || rline[0]==10 || rline[0]==13 || rline[0]==3);
    if (ret == -3) printf("Out of memory!\n");
    else if (ret == -1) printf("Signal Error!\n");
    if (ret < 0) exit(1);
    sscanf(rline,"%d",&iheight);
    if (rline[strlen(rline)-1] != 10 && rline[strlen(rline)-1] != 13) printf("\n");
    if (iwidth < 1 || iheight < 1 )
    {
      printf("Error: Width and height must be positive numbers!\n");
      exit(1);
    }
    printf("Creating image as %dx%d.\n", iwidth, iheight);
    rs.rowset = makerowset(iwidth, iheight);
    if (!rs.rowset)
    {
      printf("Out of Memory!\n");
      exit(2);
    }
    rs.rs_size = iheight;
  }
  int x, y;
  //if (getansicursorpos(&y, &x)) printf("gxy");
  //printf("at %d, %d: \n \n", x, y);
  //printf("\033[9999;9999H*");
  //getscreensize(&rs);
  //getkeyn();
  printf("Screen Size = %d x %d\n", rs.swidth, rs.sheight);
  //for (x=0;x<100;x++) printf("qwertyuiopasdfghjklzxcvbnm");
  DoError("No Program yet!");
  exit(0);
}
