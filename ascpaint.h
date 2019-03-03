#ifndef __ASCPAINT_H__
#define __ASCPAINT_H__ 1

#define TOOL_EDIT 0
#define TOOL_PAINT 1
#define TOOL_ERASE 2
#define TOOL_LINE 4
#define TOOL_SELECT 5
#define TOOL_MOVE 6

typedef struct rowstruct
{
  int allocsize; //Allocated size
  int rowsize;  //Actual size
  char *rowtext;
  /* int tasize; //Tab expanded size
  char *tatext; //Tab Expanded text */
  int rownum; //Row number (Y)
} txtrow;

typedef struct rows_struct
{
  txtrow *rowset; //row array
  int rs_size;
  txtrow *movelayer_rowset; //Row array for moving
  int mlrs_size;
  int ml_x, ml_y; //Offset for movelayer with respect to rowset.
  txtrow *displaylayer_rowset; //Row array for display (dialogs)
  int dlrs_size; //size of display layer
  int dl_x, dl_y; //offset for display layer from screen
  int view_x, view_y; //View offsets
  int cx, cy; //cursor offsets
  int swidth, sheight; //screen size
} rows;


void cleanscr();
void on_die(int i);
void on_resize(int i);
int maxrowlength(txtrow *arowset, int numrows);
int minrowlength(txtrow *arowset, int numrows);
void DoError(char *errortext);
void DoSimpleError(char *errortext);
txtrow *makerowset(int width, int height);
int getscreensize(rows *ars);
int inserttextinrow(txtrow *arow, char *text, int offset);
int freerow(txtrow *arow);
int freerowset(txtrow *arowset, int numrows);
int expandtabs(char *ostr, int omax, const char *istr);
int deltextfromrow(txtrow *arow, int tlen, int offset);
int main(int argc, char *argv[]);

#endif
