#ifndef __ASCPAINT_H__
#define __ASCPAINT_H__ 1

#define TOOL_EDIT 0
#define TOOL_PAINT 1
#define TOOL_ERASE 2
#define TOOL_LINE 4
#define TOOL_SELECT 5
#define TOOL_MOVE 6

#define DLFORMAT_INVERT 16
#define DLFORMAT_IGNORE 32

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
  txtrow *dlformat_rowset; //Row array for the formatting of the display (colours etc)
  int dlrs_size; //size of display layer
  int dl_x, dl_y; //offset for display layer from screen
  int view_x, view_y; //View offsets
  int cx, cy; //cursor offsets
  int dl_cx, dl_cy; //Cursor position on display layer, if show_dl is set.
  int swidth, sheight; //screen size
  unsigned char show_ml : 1; //Show the move layer
  unsigned char show_dl : 1; //Show the display layer
} rows;


void cleanscr();
void on_die(int i);
void on_resize(int i);
int maxrowlength(txtrow *arowset, int numrows);
int minrowlength(txtrow *arowset, int numrows);
void DoError(char *errortext);
void DoSimpleError(char *errortext);
void UpdateDisplay();
txtrow *makerowset(int width, int height);
int getscreensize(rows *ars);
int inserttextinrow(txtrow *arow, char *text, int offset);
void freerow(txtrow *arow);
void freerowset(txtrow *arowset, int numrows);
void freers_subs(rows *ars);
int expandtabs(char *ostr, int omax, const char *istr);
int deltextfromrow(txtrow *arow, int tlen, int offset);
int overwritetext(char *text, int x, int y, int allowsemiinsert);  //Returns: 1=success, 0=Cannot fit text in, -1=bad params, -2=OoM & data corrupted!
int insertrow(int at_y, char *rowtext);  //Returns: 1=success, -1=bad params, -2=OoM & data corrupted!
int movecursor_internal(int by_x, int by_y);  //Move the cursor relative to it's current position, in rs.
int movecursor(int by_x, int by_y);
/* Move the cursor relative to it's current position;
 * Returns (an OR of):
 * 0  = Did not move cursor!
 * b0 = Moved Cursor somewhat by x
 * b1 = Moved Cursor fully by x
 * b2 = Moved Cursor somewhat by y
 * b3 = Moved Cursor fully by y
 * b4 = Updated Screen
 * -1 = Other Error!  */
int main(int argc, char *argv[]);

#endif
