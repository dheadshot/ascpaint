#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>

#include "qdinp2.h"

#include "ascpaint.h"

int numedits = 0;
char *filename = NULL;
rows rs;
int insertmode = 0;
int toolmode = TOOL_EDIT;
char palettemode = '#';
char inversepalettemode = ' ';

void DoError(char *errortext)
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
  	printf("boxw=%d",boxw);
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

int getscreensize(rows *ars)
{
  int ox, oy, x, y;
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
}

int freerow(txtrow *arow)
{
  if (arow->rowtext) free(arow->rowtext);
  free(arow);
}

int expandtabs(char *ostr, int omax, const char *istr)
{
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
}

int main(int argc, char *argv[])
{
  int iwidth = 0, iheight = 0;
  rs.rowset = NULL;
  rs.rs_size = 0;
  rs.movelayer_rowset = NULL;
  rs.mlrs_size = 0;
  rs.ml_x = 0;
  rs.ml_y = 0;
  rs.displaylayer_rowset = NULL;
  rs.dlrs_size = 0;
  
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
    if (ret < 0) return 1;
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
    if (ret < 0) return 1;
    sscanf(rline,"%d",&iheight);
    if (rline[strlen(rline)-1] != 10 && rline[strlen(rline)-1] != 13) printf("\n");
    if (iwidth < 1 || iheight < 1 )
    {
      printf("Error: Width and height must be positive numbers!\n");
      return 1;
    }
    printf("Creating image as %dx%d.\n", iwidth, iheight);
  }
  int x, y;
  if (getansicursorpos(&y, &x)) printf("gxy");
  printf("at %d, %d: \n \n", x, y);
  //printf("\033[9999;9999H*");
  getscreensize(&rs);
  //getkeyn();
  printf("Screen Size = %d x %d\n", rs.swidth, rs.sheight);
  DoError("No Program yet!");
  return 0;
}
