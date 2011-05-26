#define PLAYER_LIST	"player.lst"
#define OUTPUT_FILE	"item.lst"
#define MAX_ITEMS	12000
#define MAX_STRING_LENGTH 4096
#define KEEP_TIME 7 * 24 * 60 * 60 /* one week */
#define TRUE 1
#define FALSE 0

int     fclose(FILE *stream);
int     ungetc(int c, FILE *stream);
int     gettimeofday();
int 	fprintf();

extern  int     _filbuf (FILE *);

typedef int bool;

char    fread_letter    (FILE *fp);
int     fread_number    (FILE *fp);
void    fread_to_eol    (FILE *fp);
char *  fread_word      (FILE *fp);

