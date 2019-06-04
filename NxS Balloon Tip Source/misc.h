/* Misc. functions */

typedef char * (*handlevar_t)(char*, void*);
typedef void (*freevar_t)(char*, void*);

int parse_escapes(char *in, char *out);
BOOL Tooltips_CreateDlg(HINSTANCE hinst,HWND hdlg);
VOID Tooltips_OnWMNotify(LPARAM lParam);
int ExecuteURL(char *url);
int GetOSString(char *out, int outsize);
int myatoi(char *s); /*Yeah, my very own. Hah...*/

char* myTagFunc(char *tag, void *p);
void myTagFreeFunc(char *tag, void *p);

char *GetFormattedTitleFromWinamp(char *fmtspec, HWND hwnd_winamp);
