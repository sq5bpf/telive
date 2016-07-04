/* telive v1.8 - tetra live monitor
 * (c) 2014-2016 Jacek Lipkowski <sq5bpf@lipkowski.org>
 * Licensed under GPLv3, please read the file LICENSE, which accompanies 
 * the telive program sources 
 *
 * Changelog:
 * v1.9 - better text input, scanning support --sq5bpf
 * v1.8 - handle osmo-tetra-sq5bpf protocol changes, use call identifiers for some stuff, various fixes --sq5bpf
 * v1.7 - show country and network name --sq5bpf
 * v1.6 - various fixes, add a star to the status line to show if there is network coverage, show afc info --sq5bpf
 * v1.5 - added 'z' key to clear learned info, added play lock file --sq5bpf
 * v1.4 - added frequency window --sq5bpf
 * v1.3 - add frequency info --sq5bpf
 * v1.2 - fixed various crashes, made timeouts configurable via env variables --sq5bpf
 * v1.1 - made some buffers bigger, ignore location with INVALID_POSITION --sq5bpf
 * v1.0 - add KML export --sq5bpf
 * v0.9 - add TETRA_KEYS, add option to filter playback, fixed crash on problems playing, report when the network parameters change too quickly --sq5bpf
 * v0.8 - code cleanups, add the verbose option, display for text sds in the status window  --sq5bpf
 * v0.7 - initial public release --sq5bpf
 *
 *
 * TODO: 
 * ack! functions with no bounds checking, usage of popen, environment 
 * variables instead of getopt(), spaghetti code - basically this should 
 * be rewritten from scratch some day :)
 */


#include <fnmatch.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <ncurses.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/time.h>
#include <time.h>
#include <signal.h>
#include <sys/file.h>
#include <fcntl.h>
#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <ctype.h>

#include "telive_util.h"
#include "telive.h"
#include "telive_receiver.h"

#define TELIVE_VERSION "1.9"

#define BUFLEN 8192
#define PORT 7379

/******* definitions *******/
int rec_timeout=30; /* after how long we stop to record a usage identifier */
int ssi_timeout=60; /* after how long we forget SSIs */
int idx_timeout=8; /* after how long we disable the active flag */
int curplaying_timeout=5; /* after how long we stop playing the current usage identifier */
int vbuf=1;

char *outdir;
char def_outdir[BUFLEN]="/tetra/in";
char *logfile;
char def_logfile[BUFLEN]="telive.log";
char *freqlogfile;
char def_freqlogfile[BUFLEN]="telive_frequency.log";
char *freqreportfile;
char def_freqreportfile[BUFLEN]="telive_frequency_report.txt";

char *ssifile;
char def_ssifile[BUFLEN]="ssi_descriptions";
char ssi_filter[BUFLEN];
char def_tetraxmlfile[BUFLEN]="tetra.xml";
char *tetraxmlfile;
int use_filter=0;

char *kml_file=NULL;
char *kml_tmp_file=NULL;
int kml_interval;
int last_kml_save=0;
int kml_changed=0;
int freq_changed=0;

char *lock_file=NULL;
int lockfd=0;
int locked=0;

int log_found_freq=1; /* do we log the frequencies we find during scanning etc */

/* stuff for handling the tetra networks list in xml */
xmlDocPtr tetraxml_doc;
char *tetraxml_country=NULL;
char *tetraxml_network=NULL;
uint16_t tetraxml_last_queried_country=0;
uint16_t tetraxml_last_queried_network=0;


/* various times for scheduling in tickf() */
time_t last_1s_event=0;
time_t last_10s_event=0;
time_t last_1min_event=0;
struct timeval current_timeval; /* cached current time, so that we don't call gettimeofday() and time() all the time */
#define max(a,b) \
	({ __typeof__ (a) _a = (a); \
	 __typeof__ (b) _b = (b); \
	 _a > _b ? _a : _b; })


int verbose=0;
WINDOW *msgwin=0; /* message window */
WINDOW *statuswin=0; /* status window */
WINDOW *titlewin=0; /* title window */
WINDOW *mainwin=0; /* main window */
WINDOW *freqwin=0; /* frequency window */
WINDOW *inputwin=0; /* interactive text input window */
WINDOW *ssiwin=0; /* SSI window */
WINDOW *locwin=0; /* Location window */
WINDOW *displayedwin=0; /* the window currently displayed */
int ref; /* window refresh flag, set to refresh all windows */

/* text input that doesn't block the rest of the program */
enum interactive_input { 
	INPUT_NONE=0,
	INPUT_FILTER=1,
	INPUT_FREQ=2, 
	INPUT_BASEBAND=3, 
	INPUT_PPM=4, 
	INPUT_GAIN=5, 
};
int interactive_text_input=0;
char interactive_text_buf[200];

char prevtmsg[BUFLEN]; /* contents of previous message */

struct netinfo_s {
	uint16_t mcc;
	uint16_t mnc;
	uint8_t colour_code;
	uint32_t dl_freq;
	uint32_t ul_freq;
	uint16_t la;
	time_t last_change;
	uint32_t changes;
} netinfo;

struct netinfo_s netinfo;
struct netinfo_s netinfo_tmp;

/* this structure is used to describe all known frequencies */
/* struct freqinfo {
   uint32_t dl_freq;
   uint32_t ul_freq;
   uint16_t mcc;
   uint16_t mnc;
   uint16_t la;
   time_t last_change;
   int reason;
   int rx;
   void *next;
   void *prev;
   };
   */

struct usi {
	unsigned int ssi[3];
	time_t ssi_time[3];
	time_t ssi_time_rec;
	int encr;
	int timeout;
	int active;
	int play;
	char curfile[BUFLEN];
	char curfiletime[32];
	uint16_t cid;
	int lastrx;
};

struct opisy {
	unsigned int ssi;
	char *opis;
	void *next;
	void *prev;
};

struct locations {
	unsigned int ssi;
	float lattitude;
	float longtitude;
	char *description;
	time_t lastseen;
	void *next;
	void *prev;

};

int telive_receiver_mode=TELIVE_RX_NORMAL;
int telive_auto_tune=0;
int freq_timeout=600;
int receiver_timeout=60; 


/* scanning stuff */
#define SCANSTATE_NULL 0
#define SCANSTATE_GOTSIGNAL 1
#define SCANSTATE_GOTBURST 2
#define SCANSTATE_GOTSYSINFO 3
int scan_state=SCANSTATE_NULL;
int scan_timeout_signal=50; /* how much to wait for the receiver to settle */
int scan_timeout_burst=300; /* how much to wait for bursts */
int scan_timeout_sysinfo=2000; /* how much to wait for the first sysinfo */

int previous_freq_signal=0; /* we had something on the last frequency we scanned, this flag is used to wait more before scanning the next frequency */

#define SCAN_UP 1
#define SCAN_DOWN 0
int scan_direction=SCAN_UP;
uint32_t scan_step=12500;

struct timeval scan_last_tune;
struct timeval scan_last_burst;


uint32_t scan_low=0;
uint32_t scan_high=0;

/* this should cover most allocations, although this list should be shortened for greater scanning speed */
char def_scan_list[256]="390-395/12.5;420-430/12.5;870-876/12.5;450-470/12.5;915-921/12.5";
char *scan_list=NULL;
int scan_list_item=0;

/* end scanning stuff */

char *grxml_url=NULL;
char *grxml_rx_description;
int grxml_rx_channels=0;


#define RX_TUNE_NORMAL 0
#define RX_TUNE_FORCE_BASEBAND 1

/* reciver info */
#define RX_DEFINED 1
#define RX_ACTIVE 2
#define RX_MUTED 4


struct receiver {
	unsigned int rxid;
	int afc;
	uint32_t freq;
	time_t lastseen;
	void *next;
	void *prev;
	time_t lastburst;
	int state;
};

uint32_t receiver_baseband_freq=0; /* the baseband frequency of the receiver */
uint32_t receiver_sample_rate=0; /* the sample rate of the receiver */
float receiver_ppm=0; /* frequency correction of the receiver */
int receiver_ppm_autocorrect=1; /* can  autotune the ppm value */
int receiver_baseband_autocorrect=1; /* can change the baseband frequency so that more channels are covered */
int receiver_gain=0;

struct receiver *receivers=NULL;
struct opisy *opisssi;
struct locations *kml_locations=NULL;
struct freqinfo *frequencies=NULL;
struct freqinfo *freqdb=NULL; /* a database of all frequencies found (by scanning etc) */

int curplayingidx=0;
time_t curplayingtime=0;
int curplayingticks=0; 
FILE *playingfp=NULL;
int mutessi=0;
int alldump=0;
int ps_record=0;
int ps_mute=0;
int do_log=0;
int last_burst=0;


enum telive_screen { DISPLAY_IDX, DISPLAY_FREQ, DISPLAY_END };
int display_state=DISPLAY_IDX;

void appendfile(char *file,char *msg) {
	FILE *f;
	f=fopen(file,"ab");
	if (f) {
		fprintf(f,"%s\n",msg);
		fclose(f);
	}
}

void appendlog(char *msg) {
	appendfile(logfile,msg);
}




/* tetra.xml file handling */
void tetraxml_read() {
	tetraxml_doc=xmlParseFile(tetraxmlfile);
	if (tetraxml_doc==NULL) {
		wprintw(statuswin,"File %s was not read succesfully\n",tetraxmlfile);
		return;
	}


}

void tetraxml_query(uint16_t mcc, uint16_t mnc,xmlDocPtr xml_doc)
{
	char unn[]="Unknown network";
	char unc[]="Unknown country";
	xmlChar xpath[128];
	xmlNodeSetPtr nodeset;
	xmlChar *keyword;
	xmlXPathContextPtr xml_context;
	xmlXPathObjectPtr result;

	if ((tetraxml_last_queried_country==mcc)&&(tetraxml_last_queried_network==mnc)) return;
	if (tetraxml_country) free(tetraxml_country);
	if (tetraxml_network) free(tetraxml_network);
	tetraxml_last_queried_country=mcc;
	tetraxml_last_queried_network=mnc;

	xml_context = xmlXPathNewContext(tetraxml_doc);
	if (xml_context==NULL) {
		wprintw(statuswin,"Error in xmlXPathNewContext\n");
		return;
	}
	if ((!xml_doc)||(!xml_context)) {
		tetraxml_country=strdup((char *)&unc);
		tetraxml_network=strdup((char *)&unn);
		return;
	}
	sprintf((char *)&xpath,"//country[mcc='%i']/countryname",mcc);
	result = xmlXPathEvalExpression((xmlChar *)&xpath, xml_context);
	if(xmlXPathNodeSetIsEmpty(result->nodesetval)){
		xmlXPathFreeContext(xml_context);
		xmlXPathFreeObject(result);
		tetraxml_country=strdup((char *)&unc);
		tetraxml_network=strdup((char *)&unn);
		return;
	} else {
		nodeset = result->nodesetval;
		keyword=xmlNodeListGetString(tetraxml_doc, nodeset->nodeTab[0]->xmlChildrenNode, 1);
		tetraxml_country= strdup(keyword);
		xmlFree(keyword); //should be called for other nodeTab[xxx]

	}

	sprintf((char *)&xpath,"//country[mcc='%i']/network[mnc='%i']/name",mcc,mnc);
	result = xmlXPathEvalExpression((xmlChar *)&xpath, xml_context);
	if(xmlXPathNodeSetIsEmpty(result->nodesetval)){
		xmlXPathFreeContext(xml_context);
		xmlXPathFreeObject(result);
		tetraxml_network=strdup((char *)&unn);
		return;
	} else {
		nodeset = result->nodesetval;
		keyword=xmlNodeListGetString(tetraxml_doc, nodeset->nodeTab[0]->xmlChildrenNode, 1);
		tetraxml_network= strdup(keyword);
		xmlFree(keyword); //should be called for other nodeTab[xxx]
	}
	xmlXPathFreeContext(xml_context);
	xmlXPathFreeObject (result);
}

void clearopisy()
{
	struct opisy *ptr,*ptr2;
	ptr=opisssi;

	while(ptr)
	{
		ptr2=ptr->next;
		free((void *)ptr->opis);
		free(ptr);
		ptr=ptr2;
	}
	opisssi=0;
}

time_t ostopis=0;

int newopis()
{
	struct stat st;
	if (stat(ssifile,&st)) return(0);

	if (st.st_mtime>ostopis) {
		ostopis=st.st_mtime;
		return(1);
	}
	return(0);
}

int initopis()
{
	FILE *g;
	char str[100];
	struct opisy *ptr,*prevptr;
	char *c;

	prevptr=NULL;
	clearopisy();
	g=fopen(ssifile,"r");
	if (!g) return(0);
	str[sizeof(str)-1]=0;
	while(!feof(g))
	{
		if (!fgets(str,sizeof(str)-1,g)) break;
		c=strchr(str,' ');
		if (c==NULL) continue;
		*c=0;
		c++;
		ptr=malloc(sizeof(struct opisy));
		if (prevptr) {
			prevptr->next=ptr;
			ptr->prev=prevptr;
			ptr->next=NULL;
		} else
		{
			opisssi=ptr;
		}
		ptr->ssi=atoi(str);
		ptr->opis=strdup(c);
		prevptr=ptr;
	}
	fclose(g);
	return(1);
}

const char nop[2]="-\0";
char *lookupssi(int ssi)
{
	struct opisy *ptr=opisssi;

	while(ptr)
	{
		if (ptr->ssi==ssi) return(ptr->opis);
		ptr=ptr->next;
	}
	return((char *)&nop);
}


void add_location(int ssi,float lattitude,float longtitude,char *description)
{
	struct locations *ptr=kml_locations;
	struct locations *prevptr=NULL;
	char *c;

	if (ptr) {
		/* maybe we already know this ssi? */
		while(ptr) {
			if (ptr->ssi==ssi) break;
			prevptr=ptr;
			ptr=ptr->next;
		}
	}

	if (!ptr) {
		ptr=calloc(1,sizeof(struct locations));
		if (!kml_locations) kml_locations=ptr;
		if (prevptr) { 
			ptr->prev=prevptr; 
			prevptr->next=ptr; 
		} 
		ptr->ssi=ssi;
	} else {
		free(ptr->description);
	}

	ptr->lastseen=time(0);
	ptr->lattitude=lattitude;
	ptr->longtitude=longtitude;
	ptr->description=strdup(description);
	c=ptr->description;
	/* ugly hack so that we don't get <> there, which would break the xml */
	while(*c) { if (*c=='>') *c='G';  if (*c=='<') *c='L'; c++; } 
	kml_changed=1;

}

void dump_kml_file() {
	FILE *f;
	struct locations *ptr=kml_locations;
	if (verbose>1) wprintw(statuswin,"called dump_kml_file()\n");

	if (!kml_tmp_file) return;
	f=fopen(kml_tmp_file,"w");
	if (!f) return;
	if (verbose>1) wprintw(statuswin,"dump_kml_file(%s)\n",kml_tmp_file);
	fprintf(f,"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<kml xmlns=\"http://www.opengis.net/kml/2.2\">\n<Folder>\n<name>");
	fprintf(f,"Telive MCC:%5i MNC:%5i ColourCode:%3i Down:%3.4fMHz Up:%3.4fMHz LA:%5i",netinfo.mcc,netinfo.mnc,netinfo.colour_code,netinfo.dl_freq/1000000.0,netinfo.ul_freq/1000000.0,netinfo.la);
	fprintf(f,"</name>\n<open>1</open>\n");

	while(ptr)
	{
		fprintf(f,"<Placemark> <name>%i</name> <description>%s %s</description> <Point> <coordinates>%f,%f,0</coordinates></Point></Placemark>\n",ptr->ssi,lookupssi(ptr->ssi),ptr->description,ptr->longtitude,ptr->lattitude);
		ptr=ptr->next;
	}

	fprintf(f,"</Folder></kml>\n");
	fclose(f);
	rename(kml_tmp_file,kml_file);
	last_kml_save=time(0);
	kml_changed=0;
}

/* delete the whole location info */
void clear_locations() {
	struct locations *ptr=kml_locations;
	struct locations *nextptr;
	while(ptr) {
		nextptr=ptr->next;
		free(ptr->description);
		free(ptr);
		ptr=nextptr;
	}
	kml_locations=NULL;
}

#define MAXUS 64
struct usi ssis[MAXUS];

void diep(char *s)
{
	wprintw(statuswin,"DIE: %s\n",s);
	wrefresh(statuswin);
	sleep(1);
	perror(s);
	exit(1);
}
#define RR 13
int getr(int idx) {
	return((idx%RR)*4);
}

int getcl(int idx)
{
	return ((idx/RR)*40);
}

/* update usage identifier display */
void updidx(int idx) {
	char opis[40];
	char opis2[11];
	int i;
	int row=getr(idx);
	int col=getcl(idx);
	int bold=0;

	if ((idx<0)||(idx>=MAXUS)) { wprintw(statuswin,"BUG! updidx(%i)\n",idx); wrefresh(statuswin); return; }

	opis[0]=0;
	opis2[0]=0;
	wmove(mainwin,row,col+5);
	if (ssis[idx].active) strcat(opis,"OK ");
	//	if (ssis[idx].timeout) strcat(opis,"timeout ");
	if (ssis[idx].encr) strcat(opis,"ENCR ");
	if ((ssis[idx].play)&&(ssis[idx].active)) { bold=1; strcat(opis,"*PLAY* "); }
	if (ssis[idx].cid) { sprintf(opis2,"%i [%i]",ssis[idx].cid,ssis[idx].lastrx);  }
	if (bold) wattron(mainwin,A_BOLD|COLOR_PAIR(1));
	wprintw(mainwin,"%-20s %10s",opis,opis2);
	if (bold) wattroff(mainwin,COLOR_PAIR(1));
	for (i=0;i<3;i++) {
		wmove(mainwin,row+i+1,col+5);
		if (ssis[idx].ssi[i]) {
			wprintw(mainwin,"%8i %20s",ssis[idx].ssi[i],lookupssi(ssis[idx].ssi[i]));
		}
		else 
		{
			wprintw(mainwin,"                              ");

		}
	}
	if (bold) wattroff(mainwin,A_BOLD);
	ref=1;
}

void draw_idx() {
	int idx;
	for (idx=0;idx<MAXUS;idx++)
	{
		wmove(mainwin,getr(idx),getcl(idx));
		wprintw(mainwin,"%2i:",idx);
	}
}

/* refresh the main window */
void display_mainwin() {
	int idx;
	wclear(mainwin);
	draw_idx();
	for (idx=0;idx<MAXUS;idx++) updidx(idx);
	ref=1;
}

#define STATUSLINES 7
int initcur() {
	int maxx,maxy;
	initscr();
	start_color();
	cbreak();
	init_pair(1, COLOR_RED, COLOR_BLUE);
	init_pair(2, COLOR_YELLOW, COLOR_BLUE);
	init_pair(3, COLOR_RED, COLOR_GREEN);
	init_pair(4, COLOR_WHITE, COLOR_GREEN);
	init_pair(5, COLOR_BLACK, COLOR_YELLOW);
	init_pair(6, COLOR_WHITE, COLOR_RED);
	clear();

	inputwin=newwin(3,COLS,LINES/2,0);
	scrollok(inputwin,TRUE);
	wattron(inputwin,COLOR_PAIR(6));
	wbkgdset(inputwin,COLOR_PAIR(6));
	wclear(inputwin);

	titlewin=newwin(1,COLS,0,0);
	mainwin=newwin(LINES-STATUSLINES-1,COLS,1,0);
	freqwin=newwin(LINES-STATUSLINES-1,COLS,1,0);
	locwin=newwin(LINES-STATUSLINES-1,COLS,1,0);
	ssiwin=newwin(LINES-STATUSLINES-1,COLS,1,0);
	displayedwin=mainwin;
	msgwin=newwin(STATUSLINES,COLS/2,LINES-STATUSLINES,0);
	wattron(msgwin,COLOR_PAIR(2));
	wbkgdset(msgwin,COLOR_PAIR(2));
	wclear(msgwin);
	wprintw(msgwin,"Message window\n");
	scrollok(msgwin,TRUE);

	statuswin=newwin(STATUSLINES,COLS/2,LINES-STATUSLINES,COLS/2+1);
	wattron(statuswin,COLOR_PAIR(3));
	wbkgdset(statuswin,COLOR_PAIR(3));
	wclear(statuswin);
	wprintw(statuswin,"####  Press ? for keystroke help  ####\n");
	getmaxyx(stdscr,maxy,maxx);
	if ((maxx!=203)||(maxy!=60)) {
		wprintw(statuswin,"\nWARNING: The terminal size is %ix%i, but it should be 203x60\nThe program will still work, but the display may be mangled\n\n",maxx,maxy);
	}
	scrollok(statuswin,TRUE);

	wattron(titlewin,A_BOLD);
	wprintw(titlewin,"** SQ5BPF TETRA Monitor %s **",TELIVE_VERSION);
	wattroff(titlewin,A_BOLD);

	draw_idx();
	ref=1;
	return(1);
}

void updopis()
{
	wmove(titlewin,0,32);
	wattron(titlewin,COLOR_PAIR(4)|A_BOLD);
	wprintw(titlewin,"MCC:%5i MNC:%5i ColourCode:%3i Down:%3.4fMHz Up:%3.4fMHz LA:%5i %c",netinfo.mcc,netinfo.mnc,netinfo.colour_code,netinfo.dl_freq/1000000.0,netinfo.ul_freq/1000000.0,netinfo.la,last_burst?'*':' ');
	wattroff(titlewin,COLOR_PAIR(4)|A_BOLD);
	wprintw(titlewin," mutessi:%i alldump:%i mute:%i record:%i log:%i verbose:%i lock:%i",mutessi,alldump,ps_mute,ps_record,do_log,verbose,locked);
	switch(use_filter)
	{
		case 0:	wprintw(titlewin," no filter"); break;
		case  1:	wprintw(titlewin," Filter ON"); break;
		case -1:	wprintw(titlewin," Inv. Filt"); break;
	}
	wprintw(titlewin," [%s] ",ssi_filter); 
	wclrtobot(titlewin);
	ref=1;
}

int addssi(int idx,int ssi)
{
	int i;

	if (!ssi) return(0);
	if ((idx<0)||(idx>=MAXUS)) { wprintw(statuswin,"BUG! addssi(%i,%i)\n",idx,ssi); wrefresh(statuswin); return(0); }
	for(i=0;i<3;i++) {
		if (ssis[idx].ssi[i]==ssi) {
			ssis[idx].ssi_time[i]=time(0);
			return(1);
		}
	}
	for(i=0;i<3;i++) {
		if (!ssis[idx].ssi[i]) {
			ssis[idx].ssi[i]=ssi;
			ssis[idx].ssi_time[i]=time(0);
			return(1);
		}
	}	



	/* no room to add, forget one ssi */
	ssis[idx].ssi[0]=ssis[idx].ssi[1];
	ssis[idx].ssi[1]=ssis[idx].ssi[2];
	ssis[idx].ssi[2]=ssi;
	ssis[idx].ssi_time[2]=time(0);
	ssis[idx].active=1;
	return(1);
}

int addssi2(int idx,int ssi,int i)
{
	if (!ssi) return(0);
	ssis[idx].ssi[i]=ssi;
	ssis[idx].ssi_time[i]=time(0);
	ssis[idx].active=1;
	return(0);
}

/* add an ssi to a call identified by a call identifier */
int addcallssi(int cid,int ssi) {
	int i;
	if (!cid) return(0);
	for (i=1;i<MAXUS;i++) {
		if (ssis[i].cid==cid) {
			addssi(i,ssi);
			return(i);
		}
	}
	return(0);
}

int addcallident(int idx,uint16_t cid,int rxid)
{
	if (!idx) return(0);
	if (!cid) return(0);
	if ((verbose)&&(ssis[idx].cid)&&(ssis[idx].cid!=cid)) {
		wprintw(statuswin,"Overlapping CIDs for IDX:%i. Previous: %i (RX:%i)  Current: %i (RX:%i)\n",idx,ssis[idx].cid,ssis[idx].lastrx,cid,rxid);
		ref=1;
	}
	ssis[idx].cid=cid;
	ssis[idx].lastrx=rxid;

}

int releasessi(int ssi)
{
	int i,j;
	for (i=0;i<MAXUS;i++) {
		for (j=0;j<3;j++) {
			if ((ssis[i].active)&&(ssis[i].ssi[j]==ssi)) {
				ssis[i].active=0;
				ssis[i].play=0;
				ssis[i].lastrx=0;
				ssis[i].cid=0;
				updidx(i);
				ref=1;
			}
		}
	}
	return(0);
}

/* check if an SSI matches the filter expression */
int matchssi(int ssi) 
{
	int r;
	char ssistr[16];
	if (!ssi) return(0);
	sprintf(ssistr,"%i",ssi);
	if (strlen(ssi_filter)==0) return(1); 

#ifdef FNM_EXTMATCH
	r=fnmatch((char *)&ssi_filter,(char *)&ssistr,FNM_EXTMATCH);
#else
	/* FNM_EXTMATCH is a GNU libc extension, not present in other libcs, like the MacOS X one */
#warning -----------    Extended match patterns for fnmatch are not supported by your libc. You will have to live with that.    ------------
	r=fnmatch((char *)&ssi_filter,(char *)&ssistr,0); 
#endif
	return(!r);
}

/* check if any SSIs for this usage identifier match the filter expression */
int matchidx(int idx)
{
	int i;
	int j=0;
	if (!use_filter) return (1);
	for(i=0;i<3;i++) {
		if (matchssi(ssis[idx].ssi[i])) { 
			j=1; 
			break; 
		}
	}
	if (use_filter==-1) j=!j;
	return(j);
}

/* locking functions */
int trylock() {
	int i;
	if (!lockfd) return(1);
	i=flock(lockfd,LOCK_EX|LOCK_NB);
	if (!i) { locked=1; updopis(); return(1); }
	return(0);
}

void releaselock() {
	int i=locked;
	locked=0;
	if (i) updopis();
	if (!lockfd) return;
	flock(lockfd,LOCK_UN);
}

/* receiver table functions */
//void update_receivers(int rx,int afc,uint32_t freq)
struct receiver *update_receivers(int rx)
{
	struct receiver *ptr=receivers;
	struct receiver *prevptr;

	/* do we know this rx? */
	while(ptr)
	{
		if (ptr->rxid==rx) break;
		prevptr=ptr;
		ptr=ptr->next;
	}
	/* nope, new one */
	if (!ptr) {
		ptr=calloc(1,sizeof(struct receiver));
		if (!receivers) {

			receivers=ptr;
		} else {
			prevptr->next=ptr;
			ptr->prev=prevptr;
		}
		ptr->rxid=rx;
	}
	/*	ptr->afc=afc;
		if (freq)	ptr->freq=freq;
		*/
	ptr->lastseen=time(0);
	return(ptr);
}

/* time out old receivers */
void timeout_receivers() {
	struct receiver *ptr=receivers;
	struct receiver *prevptr,*nextptr;
	if (!ptr) return;

	time_t timenow=time(0);

	while(ptr) {
		if ((!(ptr->state&RX_DEFINED))&&((timenow-ptr->lastseen)>receiver_timeout))
		{
			prevptr=ptr->prev;
			nextptr=ptr->next;
			free(ptr);
			if (prevptr) {
				prevptr->next=nextptr;
				if (nextptr) nextptr->prev=prevptr;
			} else {
				receivers=nextptr;
				if (nextptr) nextptr->prev=NULL;
			}
		}
		ptr=ptr->next;
	}
}

/* clear all known receivers */
void clear_all_receivers() {
	struct receiver *ptr=receivers;
	struct receiver *ptr2;

	while(ptr) {
		ptr2=ptr;
		ptr=ptr->next;
		free(ptr2);

	}
	receivers=0;
}

/* find pointer for receiver */
struct receiver *find_receiver(int rx)
{
	struct receiver *ptr=receivers;
	while(ptr)
	{
		if (ptr->rxid==rx) return(ptr);
		ptr=ptr->next;
	}
	return(NULL);
}

/* update receiver afc */
void update_receiver_afc(int rx,int afc) {

	struct receiver *ptr;

	ptr=find_receiver(rx);
	if (ptr) ptr->afc=afc;
}

void update_receiver_freq(int rx,uint32_t freq) {

	struct receiver *ptr;
	if (!freq) return;

	ptr=find_receiver(rx);

	if (ptr) {
		ptr->freq=freq;
		/* frequency has changed, unmute  it */
		ptr->state=ptr->state&~RX_MUTED;
	}
}

void update_receiver_lastburst(int rx) {

	struct receiver *ptr;
	ptr=find_receiver(rx);

	if (ptr) ptr->lastburst=current_timeval.tv_sec;
}



/* frequency table functions */
#define REASON_NETINFO 1<<0
#define REASON_FREQINFO 1<<1
#define REASON_DLFREQ 1<<2
#define REASON_SCANNED 1<<3


void dump_freqdb()
{
	char buf[256];
	char buf2[64];
	struct freqinfo *ptr;
	struct freqinfo *ptr2;
	FILE *f;
	int lastmnc;
	uint32_t tmpmnc=0;
	uint32_t tmpmcc=0;
	int i;
	f=fopen(freqreportfile,"w+");
	if (!f) return;

	strftime(buf,sizeof(buf)-1,"Telive " TELIVE_VERSION " frequency report %Y%m%d %H:%M:%S\n\n",localtime(&current_timeval.tv_sec));
	fputs(buf,f);

	/*  sort by MNC, the list isn't that  big, so we just use the dumbest algorithm which traverses the list multiple times */
	lastmnc=0;
	while(1) {
		i=0;
		tmpmnc=0xffff;

		ptr=freqdb;
		while(ptr) {
			if (ptr->mnc>lastmnc) {
				if (tmpmnc>ptr->mnc) { tmpmcc=ptr->mcc; tmpmnc=ptr->mnc; i=1; }
			}
			ptr=ptr->next;
		}
		if (!i) break; 
		lastmnc=tmpmnc;

		tetraxml_query(tmpmcc,tmpmnc,tetraxml_doc);
		sprintf(buf,"########  MCC %i (%s)  MNC %i (%s)   ########\n",tmpmcc,tetraxml_country,tmpmnc,tetraxml_network); fputs(buf,f);

		ptr2=freqdb;
		while(ptr2) 
		{
			if (ptr2->mnc!=lastmnc) { ptr2=ptr2->next; continue; }

			if (ptr2->rx_freq==ptr2->dl_freq) { sprintf(buf2, "CONTROL CHANNEL"); } else { sprintf(buf2,"Control: %3.4fMHz",ptr2->dl_freq/1000000.0); }

			/* BUG (sort of): we should soft by  MCC + MNC, and not only by MNC, 
			 * because someone at the border might see two networks with the same 
			 * MNC but different MCCs. */
			tetraxml_query(ptr2->mcc,ptr2->mnc,tetraxml_doc);

			if (ptr2->mcc==tmpmcc) {
				sprintf(buf,"%3.4fMHz MCC %i MNC %i LA %i CC %i  %s\n",ptr2->rx_freq/1000000.0,ptr2->mcc,ptr2->mnc,ptr2->la,ptr2->cc,buf2);
			} else {
				sprintf(buf,"%3.4fMHz MCC %i (%s) MNC %i (%s) LA %i CC %i  %s\n",ptr2->rx_freq/1000000.0,ptr2->mcc,tetraxml_country,ptr2->mnc,tetraxml_network,ptr2->la,ptr2->cc,buf2);
			}
			fputs(buf,f);
			ptr2=ptr2->next;
		}
		fputs("\n\n",f);

	}
	fputs("\nEND\n",f);
	fclose(f);
}


int isknown_rxf(struct freqinfo **freqptr,uint32_t rxf) 
{
	struct freqinfo *ptr=*freqptr;
	if (!rxf) return(0);
	while(ptr) {
		if (rxf==ptr->rx_freq)  return(1); 
		ptr=ptr->next;
	}
	return(0);
}

/* insert frequency into the freq table */
int insert_freq2(struct freqinfo **freqptr,int reason,uint16_t mnc,uint16_t mcc,uint32_t rxf,int match_rxf,uint32_t ulf,uint32_t dlf,uint16_t la, uint16_t cc,int rx)
{

	struct freqinfo *ptr=*freqptr;
	struct freqinfo *prevptr=NULL;
	char *c;
	int known=0;

	/* maybe we already know the uplink or downlink frequency */
	while(ptr) {
		/* either find duplicates via uplink/downlink (for the info we got from the network),
		 * or via the receiver frequency (when scanning) */
		if (match_rxf) {
			if ((rxf)&&(rxf==ptr->rx_freq))  break; 
		} else {
			if ((ulf)&&(ulf==ptr->ul_freq))  break; 
			if ((dlf)&&(dlf==ptr->dl_freq))  break; 
		}

		prevptr=ptr;
		ptr=ptr->next;
	}

	if (!ptr) {
		ptr=calloc(1,sizeof(struct freqinfo));
		if (!(*freqptr)) *freqptr=ptr;
		if (prevptr) { 
			ptr->prev=prevptr; 
			prevptr->next=ptr; 
		}
	} else {
		known=1;
	}	
	if ((verbose)&&(reason!=REASON_NETINFO)) {
		wprintw(statuswin,"Down:%3.4fMHz Up:%3.4fMHz LA:%i MCC:%i MNC:%i reason:%i RX:%i\n",dlf/1000000.0,ulf/1000000.0,la,mcc,mnc,reason,rx);
		wrefresh(statuswin);
	}
	ptr->last_change=time(0);
	ptr->ul_freq=ulf;
	ptr->dl_freq=dlf;
	ptr->rx_freq=rxf;
	ptr->la=la;
	ptr->cc=cc;
	ptr->mcc=mcc;
	ptr->mnc=mnc;
	ptr->reason=ptr->reason|reason;
	ptr->rx=rx;
	freq_changed=1;
	return(known);
}

/* clear the freq table, delete old frequencies */
void clear_freqtable(struct freqinfo *freqptr) {
	struct freqinfo *ptr=freqptr;
	struct freqinfo *prevptr,*nextptr;
	int del;
	if (!ptr) return;

	time_t timenow=time(0);

	while(ptr) {
		del=0;
		if ((timenow-ptr->last_change)>freq_timeout) del=1; /* timed out */
		if ((!ptr->ul_freq)&&(!ptr->dl_freq)) del=1; /* no frequency info */
		if (del) {
			prevptr=ptr->prev;
			nextptr=ptr->next;
			free(ptr);
			if (prevptr) {
				prevptr->next=nextptr;
				if (nextptr) nextptr->prev=prevptr;
			} else {
				freqptr=nextptr;
				if (nextptr) nextptr->prev=NULL;
			}
			freq_changed=1;
		}
		ptr=ptr->next;
	}
}

/* delete the whole frequency table */
void clear_all_freqtable(struct freqinfo *freqptr) {
	struct freqinfo *ptr=freqptr;
	struct freqinfo *nextptr;

	while(ptr) {
		nextptr=ptr->next;
		free(ptr);
		ptr=nextptr;
	}
	freqptr=NULL;
}

void display_freq() {
	char tmpstr2[64];
	char tmpstr[256];
	struct freqinfo *ptr=frequencies;
	struct receiver *rptr=receivers;
	time_t time_now=time(0);

	wclear(freqwin);
	tetraxml_query(netinfo.mcc,netinfo.mnc,tetraxml_doc);
	wprintw(freqwin,"\nCountry: %s [%i]\tNetwork: %s [%i]\n\n",tetraxml_country,netinfo.mcc,tetraxml_network,netinfo.mnc);


	wprintw(freqwin,"\n***  Known frequencies:  ***\n");
	while(ptr) {
		tmpstr[0]=0;
		if (ptr->dl_freq) {
			sprintf(tmpstr2,"Downlink:%3.4fMHz\t",ptr->dl_freq/1000000.0);
			strcat(tmpstr,tmpstr2);
		}  else {
			strcat(tmpstr,"                    \t");
		}

		if (ptr->ul_freq) {
			sprintf(tmpstr2,"Uplink:%3.4fMHz\t",ptr->ul_freq/1000000.0);
			strcat(tmpstr,tmpstr2);
		} else {
			strcat(tmpstr,"                  \t");
		}

		if (ptr->mcc) {
			sprintf(tmpstr2,"MCC:%5i\t",ptr->mcc);
			strcat(tmpstr,tmpstr2);
		} else {
			strcat(tmpstr,"         \t");
		}


		if (ptr->mnc) {
			sprintf(tmpstr2,"MNC:%5i\t",ptr->mnc);
			strcat(tmpstr,tmpstr2);
		} else {
			strcat(tmpstr,"         \t");
		}


		if (ptr->la) {
			sprintf(tmpstr2,"LA:%5i\t",ptr->la);
			strcat(tmpstr,tmpstr2);
		} else {
			strcat(tmpstr,"        \t");
		}

		strcat(tmpstr,"reason:[");
		if (ptr->reason&REASON_NETINFO) strcat(tmpstr,"S");
		if (ptr->reason&REASON_FREQINFO) strcat(tmpstr,"N");
		if (ptr->reason&REASON_DLFREQ) strcat(tmpstr,"A");
		strcat(tmpstr,"]\t");
		sprintf(tmpstr2,"RX:%i",ptr->rx);
		strcat(tmpstr,tmpstr2);
		wprintw(freqwin,"%s\n",tmpstr);
		ptr=ptr->next;
	}

	wprintw(freqwin,"\n\n***    Receiver info:    ***\n\n");

	if (grxml_rx_description) {
		wprintw(freqwin,"Receiver name: %s. ",grxml_rx_description); 
		wprintw(freqwin,"Mode: ");

		switch(telive_receiver_mode) {
			case TELIVE_RX_NORMAL:
				wprintw(freqwin,"not scanning");
				break;

			case TELIVE_RX_SCAN_FINDFIRST:
				wprintw(freqwin,"SCANNING until a network is found.");
				break;

			case TELIVE_RX_SCAN_FINDALL:
				wprintw(freqwin,"SCANNING all avaliable networks.");
				break;
		}
		wprintw(freqwin," Scanning direction: %s\n",(scan_direction==SCAN_UP)?"UP":"DOWN");
		wprintw(freqwin,"Scanning range: %s , current range %3.4f -  %3.4f MHz step %2.1fkHz\n",scan_list,scan_low/1000000.0,scan_high/1000000.0,scan_step/1000.0);
	}

	wprintw(freqwin,"Baseband Frequency: %3.4fMHz (baseband autocorrection:%s)  PPM: %3.1f (PPM autocorrection:%s)  Frontend gain: %i   ",receiver_baseband_freq/1000000.0,(receiver_baseband_autocorrect)?"on":"off",receiver_ppm,(receiver_ppm_autocorrect)?"on":"off",receiver_gain);

	wprintw(freqwin,"Bandwidth:%4ikHz.  Automatically tune receiver: %s\n",receiver_sample_rate/1000,(telive_auto_tune)?"on":"off");
	wprintw(freqwin,"\n");

	wprintw(freqwin,"RX:\tAFC:\t\t\t\tFREQUENCY\n");

	while(rptr) {

		char buf[24]="[..........:..........]";
		int i;
		int valid_rx;
		i=(11+rptr->afc/10);
		if (i<2) { buf[2]='<'; } 
		else  if (i>21) { buf[21]='>'; } else { buf[i]='|';  }

		if ((time_now-rptr->lastburst)<3) { valid_rx=1; } else { valid_rx=0; }

		sprintf(tmpstr,"%i%c\t%+3.3i %s\t",rptr->rxid,valid_rx?'*':' ',rptr->afc,buf);
		if (rptr->freq) {
			sprintf(tmpstr2,"%3.4fMHz",rptr->freq/1000000.0);
			strcat(tmpstr,tmpstr2);
		} else {
			strcat(tmpstr,"\t");
		}	

		if (rptr->state&RX_MUTED)  {
			strcat(tmpstr,"\tMUTED");
		} else
		{
			strcat(tmpstr,"\t");
		}
		wprintw(freqwin,"%s\n",tmpstr);
		rptr=rptr->next;
	}


	wattron(freqwin,A_BOLD);
	wmove(freqwin,1,170);
	wprintw(freqwin,"reasons: N:D-NWRK-BROADCAST");
	wmove(freqwin,2,170);
	wprintw(freqwin,"S:SYSINFO A:ChanAlloc");
	wattroff(freqwin,A_BOLD);

	if (grxml_rx_description) {
		wattron(freqwin,A_BOLD);
		int line=30;
		int col=140;
		wmove(freqwin,line++,col);
		wprintw(freqwin,"Receiver control keystroke help:");
		wmove(freqwin,line++,col);
		wprintw(freqwin,"x - change channel frequency");
		wmove(freqwin,line++,col);
		wprintw(freqwin,"X - change receiver baseband frequency");
		wmove(freqwin,line++,col);
		wprintw(freqwin,"G - change  receiver frontend gain");
		wmove(freqwin,line++,col);
		wprintw(freqwin,"P - change receiver PPM");
		wmove(freqwin,line++,col);
		wprintw(freqwin,"p - toggle PPM autocorrection");
		wmove(freqwin,line++,col);
		wprintw(freqwin,"B - toggle automagic tuning of baseband frequency");
		wmove(freqwin,line++,col);
		wprintw(freqwin,"b - automatically tune the receiver from  received info");
		wmove(freqwin,line++,col);
		wprintw(freqwin,"e - end scanning, go back to normal receiver mode");
		wmove(freqwin,line++,col);
		wprintw(freqwin,"q - scan until first network is found");
		wmove(freqwin,line++,col);
		wprintw(freqwin,"Q - scan all frequencies without stopping");
		wmove(freqwin,line++,col);
		wprintw(freqwin,"- / + - scan DOWN / UP");
		wmove(freqwin,line++,col);
		wprintw(freqwin,"d - write frequency report file");
		wattroff(freqwin,A_BOLD);

	}
	ref=1;
	freq_changed=0;
}

/* find an usage identifier with live audio so that we can play it */
int findtoplay(int first)
{
	int i;
	if (curplayingidx) return(0);
	releaselock();

	for (i=first;i<MAXUS;i++) {
		if ((ssis[i].active)&&(!ssis[i].encr)&&(matchidx(i)&&(trylock()))) {
			curplayingidx=i;
			if (verbose>0) wprintw(statuswin,"NOW PLAYING %i\n",i);
			ref=1;
			return(1);
		}
	}
	for (i=0;i<first;i++) {
		if ((ssis[i].active)&&(!ssis[i].encr)&&(matchidx(i)&&(trylock()))) {
			curplayingidx=i;
			if (verbose>0) wprintw(statuswin,"NOW PLAYING %i\n",i);
			ref=1;
			return(1);
		}
	}
	curplayingidx=0;
	return(0); 
}

void timeout_ssis(time_t t)
{
	int i,j,k;
	for (i=0;i<MAXUS;i++) {

		if(!ssis[i].active) { /* don't timeout ssis in calls with the active flag */	
			for (j=0;j<3;j++) {
				if ((ssis[i].ssi[j])&&(ssis[i].ssi_time[j]+ssi_timeout<t)) {
					ssis[i].ssi[j]=0;
					ssis[i].ssi_time[j]=0;
					updidx(i);
					ref=1;
				}
			}
			k=1;
			for (j=0;j<3;j++) if (ssis[i].ssi[j]) k=0;
			if (k) {
				/* no SSIs known and call is not active, so forget the callid and rx */
				ssis[i].cid=0;
				ssis[i].lastrx=0;
				updidx(i);
				ref=1;
			}
		}
		/* move the array elements so that the indexes start at 0 */
		for (j=0;j<2;j++) {
			if (!ssis[i].ssi[j]) {
				ssis[i].ssi[j]=ssis[i].ssi[j+1];
				ssis[i].ssi_time[j]=ssis[i].ssi_time[j+1];
				ssis[i].ssi[j+1]=0;
				ssis[i].ssi_time[j+1]=0;
			}
		}
	}
}

void timeout_idx(time_t t)
{
	int i;
	for (i=0;i<MAXUS;i++) {
		if ((ssis[i].active)&&(ssis[i].timeout+idx_timeout<t)) {
			ssis[i].active=0;
			ssis[i].play=0;
			ssis[i].cid=0;
			ssis[i].lastrx=0;
			updidx(i);
			ref=1;
		}
	}

}

void timeout_curplaying(time_t t)
{
	if ((curplayingidx)&&(curplayingtime+curplaying_timeout<t)) {
		//wprintw(statuswin,"STOP PLAYING %i\n",curplayingidx);
		ssis[curplayingidx].active=0;
		ssis[curplayingidx].play=0;
		updidx(curplayingidx);
		ref=1;
		/* find another to play */
		curplayingidx=0;
		findtoplay(curplayingidx);
	}
}

/* timing out the recording */
void timeout_rec(time_t t)
{
	char tmpfile[256];
	int i;
	for (i=0;i<MAXUS;i++) {
		if ((strlen(ssis[i].curfile))&&(ssis[i].ssi_time_rec+rec_timeout<t)) {
			snprintf(tmpfile,sizeof(tmpfile),"%s/traffic_%s_idx%i_callid%i_%i_%i_%i.out",outdir,ssis[i].curfiletime,i,ssis[i].cid,ssis[i].ssi[0],ssis[i].ssi[1],ssis[i].ssi[2]);
			rename(ssis[i].curfile,tmpfile);
			ssis[i].curfile[0]=0;
			ssis[i].active=0;
			ssis[i].cid=0;
			ssis[i].lastrx=0;
			updidx(i);
			if(verbose>1) wprintw(statuswin,"timeout rec %s\n",tmpfile);
			ref=1;
		}
	}
}

void refresh_scr()
{
	ref=0;
	wnoutrefresh(stdscr);
	wnoutrefresh(titlewin);
	wnoutrefresh(displayedwin);
	wnoutrefresh(statuswin);
	wnoutrefresh(msgwin);
	if (interactive_text_input) {
		wnoutrefresh(inputwin);		
		redrawwin(inputwin);
	}
	doupdate();
}

/* reopen a pipe to tplay */
void do_popen() {
	if (playingfp) pclose(playingfp);
	playingfp=popen("tplay >/dev/null 2>&1","w");
	if (!playingfp) {
		wprintw(statuswin,"PLAYBACK PROBLEM!! (fix tplay)\n");
		playingfp=NULL;
		curplayingidx=0;
	}
	if (vbuf) setvbuf(playingfp,NULL,_IONBF,1);
}



void do_scanning_stuff() {
	long timediff;
	struct receiver *ptr;
	long delaysig=scan_timeout_signal;
	char buf[256];

	timediff=timeval_subtract_ms(&current_timeval,&scan_last_tune);

	if (previous_freq_signal) {
		delaysig=scan_timeout_signal+1000; /* wait an extra 1s in case the previously scaned channel had a signal */
	}

	//	sprintf(buf,"wait timediff=%i delaysig=%i previous=%i",timediff,delaysig,previous_freq_signal);
	//	appendfile(freqlogfile,buf);

	if ((scan_state==SCANSTATE_NULL)&&(delaysig<timediff)) { 
		scan_state=SCANSTATE_GOTSIGNAL;

		memset(&netinfo,0,sizeof(struct netinfo_s));
		memset(&netinfo_tmp,0,sizeof(struct netinfo_s));
		ref=1;
		freq_changed=1;
		gettimeofday(&scan_last_tune,NULL);
		previous_freq_signal=0;
		//		appendfile(freqlogfile,"timeout scanstate_null");
		return;
	}
	if ((scan_state==SCANSTATE_GOTSIGNAL)&&(scan_timeout_burst<timediff)) { 
		previous_freq_signal=0;
		scan_tune(grxml_url);
		//		appendfile(freqlogfile,"timeout scanstate_gotsignal");
		return; 
	}

	if ((scan_state==SCANSTATE_GOTBURST)&&(scan_timeout_sysinfo<timediff)) { 
		previous_freq_signal=1;
		scan_tune(grxml_url);
		//		appendfile(freqlogfile,"timeout scanstate_gotburst");
		return; 
	}

}

void tickf ()
{

	gettimeofday(&current_timeval,NULL);
	if (curplayingidx) {
		/* crude hack to hack around buffering, i know this should be done better */
		if (curplayingticks==2) do_popen();

		curplayingticks++;
	}

	if ((current_timeval.tv_sec-last_1s_event)>0) {
		/* this gets executed every second */
		clear_freqtable(frequencies);
		timeout_ssis(current_timeval.tv_sec);
		timeout_idx(current_timeval.tv_sec);
		last_1s_event=current_timeval.tv_sec;
		if (displayedwin==freqwin) display_freq();
	}
	if ((current_timeval.tv_sec-last_10s_event)>9) {
		/* this gets executed every 10 seconds */
		timeout_receivers(grxml_url);
		if (receiver_ppm_autocorrect) grxml_autocorrect_ppm(grxml_url); 
		//if ((freq_changed)&&(displayedwin==freqwin)) display_freq();
		last_10s_event=current_timeval.tv_sec;
	}
	if ((current_timeval.tv_sec-last_1min_event)>59) {
		/* this gets executed every minute */
		ref=1;
		last_1min_event=current_timeval.tv_sec;
	}

	timeout_curplaying(current_timeval.tv_sec);
	timeout_rec(current_timeval.tv_sec);
	if (ref) refresh_scr();
	if (last_burst) { 
		if (last_burst==1) {
			last_burst--; 
			wprintw(statuswin,"Signal lost\n");
			updopis(); 
		} else {
			last_burst--; 
		}
	}
	if ((kml_changed)&&(kml_interval)&&((current_timeval.tv_sec-last_kml_save)>kml_interval)) dump_kml_file();


	if ((telive_receiver_mode==TELIVE_RX_SCAN_FINDFIRST)||(telive_receiver_mode==TELIVE_RX_SCAN_FINDALL)) do_scanning_stuff();

}

void start_text_input(int type)
{ 
	int i,j;
	char *msg=NULL;
	switch(type) {
		case INPUT_FILTER:
			msg="Enter filter expression";
			strncpy(interactive_text_buf,ssi_filter,sizeof(interactive_text_buf)-1);
			break;
		case INPUT_FREQ:
			msg="Enter receiver number <space> frequency in MHz (for example: \"1 435.2125\")";
			break;
		case INPUT_BASEBAND:
			msg="Enter receiver baseband frequency in MHz (for example: \"435.2125\")";
			snprintf(interactive_text_buf,sizeof(interactive_text_buf)-1,"%3.4f",receiver_baseband_freq/1000000.0);
			break;
		case INPUT_PPM:
			msg="Enter receiver frequency correction in PPM (for example: 56)";
			snprintf(interactive_text_buf,sizeof(interactive_text_buf)-1,"%2.1f",receiver_ppm);
			break;
		case INPUT_GAIN:
			msg="Enter receiver gain (valid 0-50)";
			snprintf(interactive_text_buf,sizeof(interactive_text_buf)-1,"%i",receiver_gain);
			break;
	}
	wmove(inputwin,0,0);
	j=((COLS-strlen(msg)-4)/2);

	for(i=0;i<j;i++) waddch(inputwin,'-');
	wprintw(inputwin,"  %s  ",msg);
	for(i=0;i<j;i++) waddch(inputwin,'-');
	wmove(inputwin,1,0);
	wprintw(inputwin,"%s",interactive_text_buf);
	interactive_text_input=type;
	wrefresh(inputwin);
	ref=1;
}

void do_text_input(unsigned char c)
{
	int i;
	int end=0;
	uint32_t f;
	int known;
	uint32_t  tmpu;
	int tmpint;
	double tmpdouble;
	char tmpstr[256];

	i=strlen(interactive_text_buf);
	switch(c) {
		case 0x08:
		case 0x7f:
			/* backspace */
			if (i) {
				interactive_text_buf[i-1]=0;
			} 
			break;
		case 0x0d:
			/* enter */
			end=1;
			break;
		default:
			if ((isprint(c))&&(i<sizeof(interactive_text_buf)-1)) {
				interactive_text_buf[i]=c;
				interactive_text_buf[i+1]=0;
			}
			break;

	}

	if (!end)
	{	wmove(inputwin,1,0);
		wclrtobot(inputwin);
		wprintw(inputwin,"%s",interactive_text_buf);
		wrefresh(inputwin);
		ref=1;
		return;
	}
	switch(interactive_text_input) {
		case INPUT_FILTER:
			strncpy(ssi_filter,interactive_text_buf,sizeof(ssi_filter)-1);
			updopis();
			break;
		case INPUT_FREQ:
			i=sscanf(interactive_text_buf,"%i %lf",&tmpint,&tmpdouble);
			tmpu=tmpdouble*1000000.0;
			tune_grxml_receiver(grxml_url,tmpint,tmpu,RX_TUNE_FORCE_BASEBAND);
			grxml_update_receivers(grxml_url);
			ref=1;
			break;
		case INPUT_BASEBAND:
			tmpdouble=atof(interactive_text_buf);
			set_grxml_baseband(grxml_url,tmpdouble*1000000.0);
			grxml_update_receivers(grxml_url);

			ref=1;
			break;
		case INPUT_PPM:
			tmpdouble=atof(interactive_text_buf);
			set_grxml_ppm(grxml_url,tmpdouble);
			ref=1;
			break;

		case INPUT_GAIN:
			tmpint=atoi(interactive_text_buf);
			set_grxml_gain(grxml_url,tmpint);
			ref=1;
			break;
	}

	interactive_text_input=INPUT_NONE;
	wclear(inputwin);
	redrawwin(displayedwin);
	wrefresh(displayedwin);
}


void keyf(unsigned char r)
{
	time_t tp;
	char tmpstr[40];
	char tmpstr2[80];
	if (interactive_text_input) {
		/* handle interactive text input */ 
		do_text_input(r);
	} 
	else 
	{
		/* handle keystroke commands */
		switch (r) {
			case 'l':
				do_log=!do_log;
				updopis();

				tp=time(0);
				strftime(tmpstr,sizeof(tmpstr),"%Y%m%d %H:%M:%S",localtime(&tp));
				if (do_log)
				{
					sprintf(tmpstr2,"%s **** log start ****",tmpstr);
				} else {

					sprintf(tmpstr2,"%s **** log end ****",tmpstr);
				}
				appendlog(tmpstr2);
				break;
			case 'M':
				ps_mute=!ps_mute;
				updopis();
				break;
			case 'R':
				ps_record=!ps_record;
				updopis();
				break;
			case 'm':
				mutessi=!mutessi;
				updopis();
				break;
			case 'a':
				alldump=!alldump;
				updopis();
				break;
			case 'v':
				if (verbose) verbose--;
				updopis();
				break;
			case 'V':
				if (verbose<4) verbose++;
				updopis();
				break;
			case 'r': /* refresh screen */
				redrawwin(stdscr);
				redrawwin(titlewin);
				redrawwin(statuswin);
				redrawwin(msgwin);
				redrawwin(displayedwin);
				break;
			case 's': /* stop current playing, find another one */
				if (curplayingidx)
				{
					ssis[curplayingidx].active=0;
					ssis[curplayingidx].play=0;
					if (verbose>0) wprintw(statuswin,"STOP PLAYING %i\n",curplayingidx);
					updidx(curplayingidx);
					curplayingidx=0;
					findtoplay(curplayingidx+1);
					ref=1;
				}
				break;
			case 'F':
				start_text_input(INPUT_FILTER);
				break;
			case 'x':
				start_text_input(INPUT_FREQ);
				break;
			case 'X':
				start_text_input(INPUT_BASEBAND);
				break;
			case 'G':
				start_text_input(INPUT_GAIN);
				break;
			case 'P':
				start_text_input(INPUT_PPM);
				break;
			case 'p':
				receiver_ppm_autocorrect=!receiver_ppm_autocorrect;
				ref=1;
				break;
			case 'B':
				receiver_baseband_autocorrect=!receiver_baseband_autocorrect;
				ref=1;
				break;
			case 'b':
				telive_auto_tune=!telive_auto_tune;
				ref=1;
				break;
			case 'e': 
				telive_receiver_mode=TELIVE_RX_NORMAL;
				ref=1;
				break;

			case 'q': 
				telive_receiver_mode=TELIVE_RX_SCAN_FINDFIRST;
				get_scan_range(scan_list,scan_list_item);
				ref=1;
				break;

			case 'Q': 
				telive_receiver_mode=TELIVE_RX_SCAN_FINDALL;
				get_scan_range(scan_list,scan_list_item);
				ref=1;
				break;

			case '-': 
			case '_': 
				scan_direction=SCAN_DOWN;
				ref=1;
				break;

			case '+': 
			case '=': 
				scan_direction=SCAN_UP;
				ref=1;
				break;
			case 'd':
				dump_freqdb();
				break;

			case 'f':
				switch(use_filter)
				{
					case 0: use_filter=1; break;
					case 1: use_filter=-1; break;
					case -1: use_filter=0; break;
				}
				updopis();
				break;
			case 't':
				display_state++;
				if (display_state==DISPLAY_END) display_state=DISPLAY_IDX;
				switch(display_state) {
					case DISPLAY_IDX:
						displayedwin=mainwin;
						display_mainwin();
						break;
					case DISPLAY_FREQ:
						displayedwin=freqwin;
						display_freq();
						break;
					default:
						break;
				}
				ref=1;

				break;
			case 'z':
				clear_all_freqtable(frequencies);
				clear_all_receivers();
				clear_locations();
				grxml_update_receivers(grxml_url);
				if (displayedwin==freqwin) display_freq();
				wprintw(statuswin,"cleared frequency and location info\n");
				ref=1;
				break;
			case '!':
				vbuf=!vbuf;
				if (vbuf) {
					wprintw(statuswin,"unbuffered audio\n");
				} else {
					wprintw(statuswin,"buffered audio\n");
				}
				ref=1;
				break;

			case '?':
				wprintw(statuswin,"HELP: ");
				wprintw(statuswin,"m-mutessi  M-mute   R-record   a-alldump  ");
				wprintw(statuswin,"r-refresh  s-stop play  l-log v/V-less/more verbose\n");
				wprintw(statuswin,"f-enable/disable/invert filter F-enter filter t-toggle windows\n");
				wprintw(statuswin,"z-forget learned info !-toggle audio buffering\n");
				if (grxml_rx_description) {
					wprintw(statuswin,"For additional receiver keystroke help look in the frequency window\n"); 
				}
				ref=1;
				break;
			default: 
				wprintw(statuswin,"unknown key [%c] 0x%2.2x\n",r,r);
				ref=1;
		}
	}
}

/*************** message parsing ****************/

char *getptr(char *s,char *id)
{
	char *c;
	c=strstr(s,id);
	if (c) return(c+strlen(id));
	return(0);
}

int getptrint(char *s,char *id,int base)
{
	char *c;
	c=getptr(s,id);
	if (!c) return(0); /* albo moze -1 byloby lepiej? */
	return(strtol(c, (char **) NULL, base));
}

int cmpfunc(char *c,char *func)
{
	if (!c) return(0);
	if (strncmp(c,func,strlen(func))==0) return(1);
	return(0);
}

void foundfreq(int rxid) {
	char buf[512];
	char tmpstr[128];
	struct receiver *ptr;
	int known;
	ptr=find_receiver(rxid);
	if (!ptr) return; /* should never happen :) */
	known=insert_freq2(&freqdb,REASON_SCANNED,netinfo.mnc,netinfo.mcc,ptr->freq,1,netinfo.ul_freq,netinfo.dl_freq,netinfo.la,netinfo.colour_code,rxid);

	strftime(buf,40,"%Y%m%d %H:%M:%S ",localtime(&current_timeval.tv_sec));
	//	tetraxml_query(netinfo.mcc,netinfo.mnc,tetraxml_doc); 
	sprintf(tmpstr,"Found %3.4fMHz MCC:%s [%i] MNC: %s [%i] CC:%i LA:%i ",ptr->freq/1000000.0,tetraxml_country,netinfo.mcc,tetraxml_network,netinfo.mnc,netinfo.colour_code,netinfo.la);
	strcat(buf,tmpstr);
	if (ptr->freq==netinfo.dl_freq) {
		sprintf(tmpstr," CONTROL CHANNEL");
	} else {
		sprintf(tmpstr," Control:%3.4fMHz",netinfo.dl_freq/1000000.0);
	}
	strcat(buf,tmpstr);
	if (!known) {
		wprintw(statuswin,"NEW: %s\n",buf);
		if (log_found_freq) appendfile(freqlogfile,buf);
		system("aplay /usr/share/kismet/wav/packet.wav");
	}
}

int parsestat(char *c)
{
	char *func;
	char *t,*lonptr,*latptr;
	int idtype=0;
	int ssi=0;
	int ssi2=0;
	int usage=0;
	int encr=0;
	int writeflag=1;
	int sameinfo=0;

	char tmpstr[BUFLEN*2];
	char tmpstr2[BUFLEN*2];
	time_t tp;
	uint16_t tmpmcc;
	uint16_t tmpmnc;
	uint16_t tmpla;
	uint8_t tmpcolour_code;
	uint32_t tmpdlf,tmpulf;
	int rxid;
	int callingssi,calledssi;
	char *sdsbegin;
	time_t tmptime;
	float longtitude,lattitude;
	uint16_t callidentifier;
	struct receiver *ptr;
	int i;

	func=getptr(c,"FUNC:");
	idtype=getptrint(c,"IDT:",10);
	ssi=getptrint(c,"SSI:",10);
	ssi2=getptrint(c,"SSI2:",10);
	usage=getptrint(c,"IDX:",10);
	encr=getptrint(c,"ENCR:",10);
	rxid=getptrint(c,"RX:",10);
	callidentifier=getptrint(c,"CID:",10);


	if (cmpfunc(func,"BURST")) {
		update_receivers(rxid);
		update_receiver_lastburst(rxid);

		if (!last_burst) {
			last_burst=10;
			updopis(); 
			wprintw(statuswin,"Signal found\n");
		} else {
			last_burst=10;
		}

		if ((telive_receiver_mode==TELIVE_RX_SCAN_FINDFIRST)||(telive_receiver_mode==TELIVE_RX_SCAN_FINDALL)) {
			if (rxid==1) previous_freq_signal=1;
			if (scan_state==SCANSTATE_GOTSIGNAL) scan_state=SCANSTATE_GOTBURST;
		}
		/* never log bursts */
		return(0);
	}

	if (cmpfunc(func,"AFCVAL")) {
		update_receivers(rxid);
		update_receiver_afc(rxid,getptrint(c,"AFC:",10));
		/* never log afc values */
		return(0);
	}

	/* scanning so  far uses only receiver 1, this should be fixed later for better speed */
	if ((telive_receiver_mode!=TELIVE_RX_NORMAL)&&(rxid!=1)) return(0);

	/* don't process data from muted receivers */
	ptr=find_receiver(rxid);
	if ((ptr)&&(ptr->state&RX_MUTED)) return(0);


	if (cmpfunc(func,"NETINFO")) {
		writeflag=0;
		tmpmnc=getptrint(c,"MNC:",16);	
		tmpmcc=getptrint(c,"MCC:",16);	
		tmpcolour_code=getptrint(c,"CCODE:",16);	
		tmpdlf=getptrint(c,"DLF:",10);
		tmpulf=getptrint(c,"ULF:",10);
		tmpla=getptrint(c,"LA:",10);
		insert_freq2(&frequencies,REASON_NETINFO,tmpmnc,tmpmcc,0,0,tmpulf,tmpdlf,tmpla,tmpcolour_code,rxid);


		/* when there is a lot of interference sometimes we get bogus info, so we wait until there are 2 consecutive frames with the same info */
		//if ((netinfo_tmp.mnc==tmpmnc)&& (netinfo_tmp.mcc==tmpmcc)&& (netinfo_tmp.colour_code==tmpcolour_code)&& (netinfo_tmp.dl_freq==tmpdlf)&& (netinfo_tmp.ul_freq==tmpulf)&& (netinfo_tmp.la==tmpla)) sameinfo=1;
		if ((netinfo_tmp.mnc==tmpmnc)&&(netinfo_tmp.mcc==tmpmcc)) sameinfo=1;


		netinfo_tmp.mnc=tmpmnc;
		netinfo_tmp.mcc=tmpmcc;
		netinfo_tmp.colour_code=tmpcolour_code;
		netinfo_tmp.dl_freq=tmpdlf;
		netinfo_tmp.ul_freq=tmpulf;
		netinfo_tmp.la=tmpla;


		if (sameinfo) {
			if ((tmpmnc!=netinfo.mnc)||(tmpmcc!=netinfo.mcc)||(tmpcolour_code!=netinfo.colour_code)||(tmpdlf!=netinfo.dl_freq)||(tmpulf!=netinfo.ul_freq)||(tmpla!=netinfo.la))
			{

				netinfo.mnc=tmpmnc;
				netinfo.mcc=tmpmcc;
				netinfo.colour_code=tmpcolour_code;
				netinfo.dl_freq=tmpdlf;
				netinfo.ul_freq=tmpulf;
				netinfo.la=tmpla;
				updopis();
				tmptime=time(0);

				if (netinfo.last_change==tmptime) { netinfo.changes++; } else { netinfo.changes=0; }
				if (netinfo.changes>10) {
					/*  this fires also when there is a lot interference on the channel unfortunately */
					wprintw(statuswin,"Too much changes. Are you monitoring only one cell? (enable alldump to see)\n");
					ref=1;
				} else {
					tetraxml_query(netinfo.mcc,netinfo.mnc,tetraxml_doc);
					wprintw(statuswin,"Found Country: %s [%i]\tNetwork: %s [%i] CC:%i LA:%i Control:%3.4fMHz RX:%i\n",tetraxml_country,netinfo.mcc,tetraxml_network,netinfo.mnc,netinfo.colour_code,netinfo.la,tmpdlf/1000000.0,rxid);
					if ((telive_receiver_mode==TELIVE_RX_NORMAL)&&(telive_auto_tune)) tune_free_receiver(grxml_url,rxid,netinfo.dl_freq);
				}
				netinfo.last_change=tmptime;

				if (telive_receiver_mode!=TELIVE_RX_NORMAL) {  
					clear_all_freqtable(frequencies);
					foundfreq(rxid); 
					previous_freq_signal=1;

				}
			}

			if (telive_receiver_mode==TELIVE_RX_SCAN_FINDFIRST)  telive_receiver_mode=TELIVE_RX_NORMAL; /* stop scanning at first found net */
			if (telive_receiver_mode==TELIVE_RX_SCAN_FINDALL) scan_tune(grxml_url);
		}

	}
	if (cmpfunc(func,"FREQINFO1")) {
		writeflag=0;
		tmpmnc=getptrint(c,"MNC:",16);	
		tmpmcc=getptrint(c,"MCC:",16);	
		tmpdlf=getptrint(c,"DLF:",10);
		tmpulf=getptrint(c,"ULF:",10);
		tmpla=getptrint(c,"LA:",10);
		/* TODO: get CC */
		insert_freq2(&frequencies,REASON_FREQINFO,tmpmnc,tmpmcc,0,0,tmpulf,tmpdlf,tmpla,0,rxid);

	}
	if (cmpfunc(func,"FREQINFO2")) {
		writeflag=0;
		tmpmnc=getptrint(c,"MNC:",16);	
		tmpmcc=getptrint(c,"MCC:",16);	
		tmpdlf=getptrint(c,"DLF:",10);
		tmpulf=getptrint(c,"ULF:",10);
		tmpla=getptrint(c,"LA:",10);
		/* TODO: get CC */
		insert_freq2(&frequencies,REASON_DLFREQ,tmpmnc,tmpmcc,0,0,tmpulf,tmpdlf,tmpla,0,rxid);
		if (telive_auto_tune) tune_free_receiver(grxml_url,rxid,tmpdlf);
	}
	if (cmpfunc(func,"DSETUPDEC"))
	{
		if (usage) {
			addssi(usage,ssi);
			addssi(usage,ssi2);
			updidx(usage);
		} 
		if (callidentifier) {
			i=addcallssi(callidentifier,ssi);
			if (i) updidx(i);
			if (ssi2) {
				i=addcallssi(callidentifier,ssi2);
				if (i) updidx(i);
			}

		}

	}
	if (cmpfunc(func,"DCONNECTDEC"))
	{
		addssi(usage,ssi);
		addssi(usage,ssi2);
		addcallident(usage,callidentifier,rxid);
		updidx(usage);

	}

	if (cmpfunc(func,"DTXGRANTDEC"))
	{
		i=getptrint(c,"TXGRANT:",10);
		/*  0 - transmission granted, 3 - transmission granted to  another user (not sure if this should be included) */
		if ((i==0)||(i==3)) { 
			i=addcallssi(callidentifier,ssi);
			if (i) updidx(i);
			if (ssi2) { 
				i=addcallssi(callidentifier,ssi2);
				if (i) updidx(i);
			}
		}
	}

	if (cmpfunc(func,"SDSDEC")) {
		callingssi=getptrint(c,"CallingSSI:",10);
		calledssi=getptrint(c,"CalledSSI:",10);
		sdsbegin=strstr(c,"DATA:");
		latptr=getptr(c," lat:");
		lonptr=getptr(c," lon:");
		if ((strstr(c,"Text")))
		{ 
			wprintw(statuswin,"SDS %i->%i %s\n",callingssi,calledssi,sdsbegin);
			ref=1;


		}
		/* handle location */
		if ((kml_tmp_file)&&(strstr(c,"INVALID_POSITION")==0)&&(latptr)&&(lonptr))
		{
			lattitude=atof(latptr);
			longtitude=atof(lonptr);
			t=latptr;
			while ((*t)&&(*t!=' ')) { 
				if (*t=='S') { lattitude=-lattitude; break; }
				t++;
			}
			t=lonptr;
			while ((*t)&&(*t!=' ')) { 
				if (*t=='W') { longtitude=-longtitude; break; }
				t++;
			}
			add_location(callingssi,lattitude,longtitude,c);

		}
	}
	if (idtype==ADDR_TYPE_SSI_USAGE) {
		if (cmpfunc(func,"D-SETUP"))
		{
			addssi(usage,ssi);
			updidx(usage);
		}
		if (cmpfunc(func,"D-CONNECT"))
		{
			addssi(usage,ssi);
			updidx(usage);
		}

	}
	if (cmpfunc(func,"D-RELEASE"))
	{
		/* don't use releasessi for now, as we can have the same ssi 
		 * on different usage identifiers. one day this should be 
		 * done properly with notif. ids */
		//		releasessi(ssi);
	}


	if (alldump) writeflag=1;
	if ((writeflag)&&(strcmp(c,prevtmsg)))
	{
		tp=time(0);
		strftime(tmpstr,40,"%Y%m%d %H:%M:%S",localtime(&tp));

		snprintf(tmpstr2,sizeof(tmpstr2)-1,"%s %s",tmpstr,c);

		wprintw(msgwin,"%s\n",tmpstr2);
		strncpy(prevtmsg,c,sizeof(prevtmsg)-1);
		prevtmsg[sizeof(prevtmsg)-1]=0;
		if (do_log) appendlog(tmpstr2);
	}

	return(0);

}


int parsetraffic(unsigned char *buf)
{
	unsigned char *c;
	int usage;
	int len=1380;
	time_t tt=time(0);
	FILE *f;
	int rxid;
	struct receiver *ptr;

	usage=getptrint((char *)buf,"TRA:",16);
	rxid=getptrint((char *)buf,"RX:",16);

	/* don't process data from muted receivers */
	ptr=find_receiver(rxid);
	if ((ptr)&&(ptr->state&RX_MUTED)) return(0);

	if ((usage<1)||(usage>63)) return(0);
	c=buf+13;

	if (!ssis[usage].active) {
		ssis[usage].active=1;
		updidx(usage);
	}
	ssis[usage].timeout=tt;

	if ((strncmp((char *)buf,"TRA:",4)==0)&&(!ssis[usage].encr)) {
		if ((mutessi)&&(!ssis[usage].ssi[0])&&(!ssis[usage].ssi[1])&&(!ssis[usage].ssi[2])) return(0); /* ignore it if we don't know any ssi for this usage identifier */

		findtoplay(0);
		if ((curplayingidx)&&(curplayingidx==usage)) {
			if (!matchidx(usage)) {
				ssis[curplayingidx].active=0;
				ssis[curplayingidx].play=0;
				if (verbose>0) wprintw(statuswin,"STOP PLAYING %i\n",curplayingidx);
				updidx(curplayingidx);
				curplayingidx=0;
				findtoplay(curplayingidx+1);
				ref=1;
				return(0);
			}
			ssis[usage].play=1;
			if (!ps_mute)	{
				if (!ferror(playingfp)) {
					fwrite(c,1,len,playingfp);
					fflush(playingfp);
				} else {
					wprintw(statuswin,"PLAYBACK PROBLEM!! (fix tplay)\n");
					playingfp=NULL;
					curplayingidx=0;
				}
			}
			curplayingtime=time(0);
			curplayingticks=0;
			updidx(usage);
			ref=1;
		}
		if ((strlen(ssis[usage].curfile)==0)||(ssis[usage].ssi_time_rec+rec_timeout<tt)) {
			/* either it has no name, or there was a timeout, 
			 * change the file name */
			strftime(ssis[usage].curfiletime,32,"%Y%m%d_%H%M%S",localtime(&tt));
			sprintf(ssis[usage].curfile,"%s/traffic_%i.tmp",outdir,usage);
			if (verbose>1) wprintw(statuswin,"newfile %s\n",ssis[usage].curfile);
			ref=1;
		}
		if (strlen(ssis[usage].curfile))
		{
			if (ps_record) {	
				f=fopen(ssis[usage].curfile,"ab");
				if (f) {
					fwrite(c, 1, len, f);
					fclose(f);
				}
			}
			ssis[usage].ssi_time_rec=tt;
		}


	}

	return(0);
}

void grxml_autocorrect_ppm(char *url)
{
	int i;
	struct receiver *ptr;
	int l=0;
	int n=0;
	int corr=0;
	char buf1[128];
	int ret;
	float ppm;
	float step;

	for (i=1;i<=grxml_rx_channels; i++) {
		ptr=find_receiver(i);
		if ((current_timeval.tv_sec-ptr->lastburst)>3) continue;
		if (ptr->state&RX_MUTED) continue;
		l++;
		n=n+ptr->afc;
	}
	if (!l)  return;
	corr=n/l;
	if  (abs(corr)<10) return;
	if  (abs(corr)>20) { step=1; } else { step=0.1; };

	if (corr<0) { 
		ppm=receiver_ppm+step; 
	} else {
		ppm=receiver_ppm-step; 
	}
	sprintf(buf1,"%.1f",ppm);
	ret=grxml_send(url,GRXML_SET,"ppm_corr","double",buf1,sizeof(buf1));
	if (!ret)  return; /* TODO: handle this error somehow */
	receiver_ppm=ppm;
}

/* receiver sanity  check  */
void rx_sanity_check() {
	struct receiver *ptr,*ptr2;
	int i,j;

	for (i=1;i<=grxml_rx_channels; i++) {

		ptr2=find_receiver(i);
		if ((ptr2)&&(ptr2->state&RX_MUTED)) continue;

		for (j=i+1;j<=grxml_rx_channels;j++) {
			ptr=find_receiver(j);
			if ((ptr)&&(ptr->state&RX_MUTED)) continue;



			if (ptr->freq==ptr2->freq) {
				//				wprintw(statuswin,"disabling rx %i (same as %i)\n",j,i);
				if (ptr) ptr->state=ptr->state|RX_MUTED;


			}
		}

	}
}

/* set the baseband frequency */
void set_grxml_baseband(char *url,uint32_t  freq) {

	struct receiver *ptr;
	char buf1[64];
	uint32_t old_baseband;
	int ret;
	int i;

	old_baseband=receiver_baseband_freq;
	receiver_baseband_freq=freq;
	sprintf(buf1,"%i",receiver_baseband_freq);
	ret=grxml_send(url,GRXML_SET,"freq","double",buf1,sizeof(buf1));
	if (!ret)  return; /* TODO: handle this error somehow */

	for (i=1;i<=grxml_rx_channels; i++) {
		ptr=find_receiver(i);
		if (!ptr) continue;
		ptr->state=ptr->state|RX_MUTED;
	}
}

void set_grxml_ppm(char *url,float ppm) {
	char buf1[64];
	int ret;

	receiver_ppm=ppm;
	sprintf(buf1,"%f",receiver_ppm);
	ret=grxml_send(url,GRXML_SET,"ppm_corr","double",buf1,sizeof(buf1));
}

void set_grxml_gain(char *url,int gain) {
	char buf1[64];
	int ret;

	receiver_gain=gain;
	sprintf(buf1,"%i",receiver_gain);
	ret=grxml_send(url,GRXML_SET,"sdr_gain","double",buf1,sizeof(buf1));
}



/* try to tune a receiver rxid, if the frequency is out of range, then  try to change the baseband so that it fits */
int tune_grxml_receiver(char *url,int rxid,uint32_t freq,int force) {
	uint32_t minfreq=0;
	uint32_t maxfreq=0;
	int32_t f;
	int i;
	struct receiver *ptr;
	char buf1[128];
	char buf2[128];
	int ret;
	int over_baseband;
	uint32_t old_baseband;
	int baseband_touched=0;

	if (!receiver_baseband_freq) return(0); /* sanity check */

	f=freq-receiver_baseband_freq;
	minfreq=freq;
	maxfreq=freq;
	/* wprintw(statuswin,"tune rx %i to %i force:%i\n",rxid, freq,force); */

	/* TODO: maybe these should be made configurable for very low-bandwidth sdrs like the funcube. i'll fix this if the users complain enough :) */
#define BASEBAND_EDGE_OFFSET 20000 //10kHz
#define BASEBAND_OFFSET 100000 //100kHz

	over_baseband=(abs(f)>(receiver_sample_rate/2-BASEBAND_EDGE_OFFSET));
	old_baseband=receiver_baseband_freq;

	if ((force==RX_TUNE_FORCE_BASEBAND)&&(over_baseband)) {
		/* if it doesn't fit, then just change the baseband frequency by brutal force and mute all other channels */
		receiver_baseband_freq=freq-BASEBAND_OFFSET;
		sprintf(buf1,"%i",receiver_baseband_freq);
		ret=grxml_send(url,GRXML_SET,"freq","double",buf1,sizeof(buf1));
		if (!ret)  return(0); /* TODO: handle this error somehow */
		baseband_touched=1;
		for (i=1;i<=grxml_rx_channels; i++) {
			if (i==rxid) continue;
			ptr=find_receiver(i);
			if (!ptr) continue;
			ptr->freq=receiver_baseband_freq+ptr->freq-old_baseband;
			ptr->state=ptr->state|RX_MUTED;
		}
	}

	if ((receiver_baseband_autocorrect)&&(over_baseband)) {
		/* try to figure out a good baseband frequency */

		for (i=1;i<=grxml_rx_channels; i++) {
			ptr=find_receiver(i);
			if (!ptr) continue;
			if (i==rxid) continue;
			if (ptr->state&RX_MUTED) continue;
			if ((current_timeval.tv_sec-ptr->lastseen)<receiver_timeout) {
				if (ptr->freq<minfreq) minfreq=ptr->freq;
				if (ptr->freq>maxfreq) maxfreq=ptr->freq;
			}
		}

		if ((maxfreq-minfreq)>receiver_sample_rate) {
			wprintw(statuswin,"autocorrect problem\n");
			return(0); /* sorry, can't autocorrect this */
		}
		receiver_baseband_freq=(maxfreq+minfreq)/2;

		update_receiver_freq(rxid,freq);

		/* set new baseband and retune everything */
		sprintf(buf1,"%i",receiver_baseband_freq);
		ret=grxml_send(url,GRXML_SET,"freq","double",buf1,sizeof(buf1));
		if (!ret)  return(0); /* TODO: handle this error somehow */
		baseband_touched=1;

		for (i=1;i<=grxml_rx_channels; i++) {
			ptr=find_receiver(i);
			if (!ptr) continue;
			f=ptr->freq-receiver_baseband_freq;
			if (abs(f)>(receiver_sample_rate/2)) continue; /* probably an unused channel or something */

			sprintf(buf1,"%i",f);
			sprintf(buf2,"xlate_offset%i",i);
			ret=grxml_send(url,GRXML_SET,buf2,"int",buf1,sizeof(buf1));
			if (!ret)  return(0); /* TODO: handle this error somehow */

		}

	} else {
		f=freq-receiver_baseband_freq;
		sprintf(buf1,"%i",f);
		sprintf(buf2,"xlate_offset%i",rxid);
		ret=grxml_send(url,GRXML_SET,buf2,"double",buf1,sizeof(buf1));
		if (!ret)  return(0); /* TODO: handle this error somehow */
		update_receiver_freq(rxid,freq);
	}

	/* this is a horrible hack to read the receiver state after tuning, because the tuning code sometimes looses track,  hope to remove this in the near future */
	if (baseband_touched) grxml_update_receivers(url); 

	rx_sanity_check();
}

/* find a free receiver and use it */
void tune_free_receiver(char *url,int src_rxid,uint32_t dlf) {
	int i;
	struct receiver *ptr;
	uint64_t f;
	int ret;
	int is_free;

	for (i=1;i<=grxml_rx_channels; i++) {
		ptr=find_receiver(i);
		if (!ptr) continue;
		if (ptr->state&RX_MUTED) continue;
		if ((abs(ptr->freq-dlf))<5000) 
		{
			/* we already know a frequency within 5kHz */
			return; 
		}
	}
	wprintw(statuswin,"tune %i src %i\n",dlf,src_rxid);

	for (i=1;i<=grxml_rx_channels; i++) {
		ptr=find_receiver(i);
		if (!ptr) continue;
		is_free=0;

		if ((current_timeval.tv_sec-ptr->lastburst)>3) is_free=1;
		if (ptr->state&RX_MUTED) is_free=1;
		wprintw(statuswin,"tune  %i rx %i free:%i\n",dlf,i,is_free);
		if (is_free) {
			ret=tune_grxml_receiver(url,i,dlf,RX_TUNE_NORMAL);
			//	wprintw(statuswin,"tune from rx %i to %i  ret=%i\n",src_rxid, dlf,ret);
			return;
		}
		else
		{
			wprintw(statuswin,"no free rx tune  %i rx %i %i\n",dlf,i,current_timeval.tv_sec-ptr->lastburst);

		}
	}

}

void tune_receivers(char *f)
{
	char *c;
	char *d;
	char *e;
	uint32_t tmpu;
	double tmpdouble;
	int rx=1;

	e=strdup(f);
	c=e;
	d=e;
	while (*c)
	{
		if (*c==';') {
			*c=0;
			tmpu=1000000.0*atof(d);
			tune_grxml_receiver(grxml_url,rx,tmpu,RX_TUNE_NORMAL);
			c++;
			if (*c) d=c;
			rx++;
		} else {
			c++;
		}


	}
	free(e);
}

/* get the previous/next scan range */
int inc_scan_range(char *list,int item,int dir)
{
	int nel=0;

	while (*list) {
		if (*list==';') nel++;
		list++;
	}
	if (dir==SCAN_UP)
	{
		item++;
		if (item>nel) item=0;
	} else {
		item--;
		if  (item<0) item=nel;
	}
	return(item);
}

/*  pull scaning range and  step from the config strings */
int get_scan_range(char *list,int item)
{
	char *lsl=strdup(list);
	char *d;
	char *c;
	int i;
	double lo,hi,step;

	c=lsl;
	i=item;
	while ((*c)&&(i))
	{
		if (*c==';') i--;
		c++;
	}
	if (c) {
		d=c+1;
		while (*d) {
			if (*d==';') { *d=0; break; }
			d++;
		}

	}
	i=sscanf(c,"%lf-%lf/%lf",&lo,&hi,&step);

	scan_low=(lo*1000000.0);
	scan_high=(hi*1000000.0);
	scan_step=(step*1000.0);

	free(lsl);
	return(i);
}




void scan_tune(char *url) {
	struct receiver *ptr;
	uint32_t newfreq;
	int i;
	double k;

	if ((!url)||(!grxml_rx_channels)) { 
		wprintw(statuswin,"Scanning is supported only with the gnuradio xmlrpc receiver interface\n");
		telive_receiver_mode=TELIVE_RX_NORMAL;
		return;
	}
	scan_state=SCANSTATE_NULL;
	gettimeofday(&scan_last_tune,NULL);

	ptr=find_receiver(1);

	newfreq=ptr->freq;
	while(1) {
		if (scan_direction==SCAN_UP) { 
			/* newfreq=newfreq+scan_step; */
			k=(newfreq-scan_low)/scan_step;
			newfreq=scan_low+(abs(k)+1)*scan_step;

			if (newfreq>scan_high) {
				scan_list_item=inc_scan_range(scan_list,scan_list_item,1);
				get_scan_range(scan_list,scan_list_item);
				newfreq=scan_low;
			}
		} else {
			/* newfreq=newfreq-scan_step; */
			k=(newfreq-scan_low)/scan_step;
			newfreq=scan_low-(abs(k)-1)*scan_step;

			if (newfreq<scan_low){
				scan_list_item=inc_scan_range(scan_list,scan_list_item,0);
				get_scan_range(scan_list,scan_list_item);
				newfreq=scan_high;
			}
		}
		if (freqdb) {
			if  (!isknown_rxf(&freqdb,newfreq)) break;
		} else { 
			break;
		}
	}
	tune_grxml_receiver(url,1,newfreq,RX_TUNE_FORCE_BASEBAND);
	ptr->lastburst=0;
	ptr->lastseen=0;
	ptr->state=RX_DEFINED;
	ptr->afc=0;
	ptr->lastseen=0;

	/* mute all other channels */
	for (i=2;i<=grxml_rx_channels;i++) {
		ptr=find_receiver(i);
		if (ptr) ptr->state=ptr->state|RX_MUTED;
	}
	ref=1;
	freq_changed=1;
	memset(&netinfo,0,sizeof(struct netinfo_s));
	memset(&netinfo_tmp,0,sizeof(struct netinfo_s));
	refresh_scr();
}

void grxml_update_receivers(char *url) {
	int i;
	int ret;
	char buf[128];
	char buf2[128];
	struct receiver *ptr;

	ret=grxml_send(url,GRXML_GET,"freq","double",buf,sizeof(buf));
	if (!ret)  return; /* TODO: handle this error somehow */
	receiver_baseband_freq=atoi(buf);

	ret=grxml_send(url,GRXML_GET,"samp_rate","double",buf,sizeof(buf));
	if (!ret)  return; /* TODO: handle this error somehow */
	receiver_sample_rate=atoi(buf);

	ret=grxml_send(url,GRXML_GET,"ppm_corr","double",buf,sizeof(buf));
	if (!ret)  return; /* TODO: handle this error somehow */
	receiver_ppm=atof(buf);

	ret=grxml_send(url,GRXML_GET,"sdr_gain","double",buf,sizeof(buf));
	if (!ret)  return; /* TODO: handle this error somehow */
	receiver_gain=atoi(buf);

	for(i=1;i<=grxml_rx_channels; i++) {
		ptr=update_receivers(i);
		sprintf(buf2,"xlate_offset%i",i);
		ptr->state=RX_DEFINED;
		ret=grxml_send(url,GRXML_GET,buf2,"int",buf,sizeof(buf));
		if (!ret)  continue; /* TODO: handle this error somehow */


		update_receiver_freq(i,receiver_baseband_freq+atoi(buf));
	}
	rx_sanity_check();
	ref=1;
}



/* get config from env variables, maybe i should switch it to getopt() one day */
void get_cfgenv() {
	int i;
	if (getenv("TETRA_OUTDIR"))
	{
		outdir=getenv("TETRA_OUTDIR");
	} else {
		outdir=def_outdir;
	}


	if (getenv("TETRA_LOGFILE"))
	{
		logfile=getenv("TETRA_LOGFILE");
	} else {
		logfile=def_logfile;
	}
	if (getenv("TETRA_FREQLOGFILE"))
	{
		freqlogfile=getenv("TETRA_FREQLOGFILE");
	} else {
		freqlogfile=def_freqlogfile;
	}
	if (getenv("TETRA_SSI_FILTER"))
	{
		strncpy((char *)&ssi_filter,getenv("TETRA_SSI_FILTER"),sizeof(ssi_filter)-1);
	} else {
		ssi_filter[0]=0;
	}

	if (getenv("TETRA_SSI_DESCRIPTIONS"))
	{
		ssifile=getenv("TETRA_SSI_DESCRIPTIONS");
	} else {
		ssifile=def_ssifile;	
	}

	if (getenv("TETRA_KML_FILE"))
	{
		kml_file=getenv("TETRA_KML_FILE");
		kml_tmp_file=malloc(strlen(kml_file)+6);
		sprintf(kml_tmp_file,"%s.tmp",kml_file);
		if (getenv("TETRA_KML_INTERVAL")) {
			kml_interval=atoi(getenv("TETRA_KML_INTERVAL"));
		} else {
			kml_interval=30;
		}
	} else {
		kml_interval=0;
	}

	if (getenv("TETRA_XMLFILE"))
	{
		tetraxmlfile=getenv("TETRA_XMLFILE");
	} else {
		tetraxmlfile=def_tetraxmlfile;
	}

	if (getenv("TETRA_LOCK_FILE"))
	{
		lock_file=getenv("TETRA_LOCK_FILE");
		lockfd=open(lock_file,O_RDWR|O_CREAT,0600);
		if (lockfd<1) lockfd=0;
	}

	/* internal tuning parameters */
	if (getenv("TETRA_REC_TIMEOUT")) rec_timeout=atoi(getenv("TETRA_REC_TIMEOUT"));
	if (getenv("TETRA_SSI_TIMEOUT")) ssi_timeout=atoi(getenv("TETRA_SSI_TIMEOUT"));
	if (getenv("TETRA_IDX_TIMEOUT")) idx_timeout=atoi(getenv("TETRA_IDX_TIMEOUT"));
	if (getenv("TETRA_CURPLAYING_TIMEOUT")) curplaying_timeout=atoi(getenv("TETRA_CURPLAYING_TIMEOUT"));
	if (getenv("TETRA_FREQ_TIMEOUT")) freq_timeout=atoi(getenv("TETRA_FREQ_TIMEOUT"));
	if (getenv("TETRA_SCAN_TIMEOUT_SIGNAL")) scan_timeout_signal=atoi(getenv("TETRA_SCAN_TIMEOUT_SIGNAL"));
	if (getenv("TETRA_SCAN_TIMEOUT_BURST")) scan_timeout_burst=atoi(getenv("TETRA_SCAN_TIMEOUT_BURST"));
	if (getenv("TETRA_SCAN_TIMEOUT_SYSINFO")) scan_timeout_sysinfo=atoi(getenv("TETRA_SCAN_TIMEOUT_SYSINFO"));



	/* grc xmlrpc receiver stuff */
	grxml_url=getenv("TETRA_GR_XMLRPC_URL");
	if (grxml_url) {

		if (getenv("TETRA_RX_GAIN")) {
			receiver_gain=atoi(getenv("TETRA_RX_GAIN"));
			set_grxml_gain(grxml_url,receiver_gain);
		}
		if (getenv("TETRA_AUTO_TUNE")) {
			telive_auto_tune=atoi(getenv("TETRA_AUTO_TUNE"));
		}

		if (getenv("TETRA_RX_PPM")) {
			receiver_ppm=atof(getenv("TETRA_RX_PPM"));
			set_grxml_ppm(grxml_url,receiver_ppm);
		}

		if (getenv("TETRA_RX_PPM_AUTOCORRECT")) {
			receiver_ppm_autocorrect=atoi(getenv("TETRA_RX_PPM_AUTOCORRECT"));
		}
		if (getenv("TETRA_RX_BASEBAND")) {
			receiver_baseband_freq=atof(getenv("TETRA_RX_BASEBAND"))*1000000.0;
			set_grxml_baseband(grxml_url,receiver_baseband_freq);
		}

		if (getenv("TETRA_RX_BASEBAND_AUTOCORRECT")) {
			receiver_baseband_autocorrect=atoi(getenv("TETRA_RX_BASEBAND_AUTOCORRECT"));
		}
		if (getenv("TETRA_RX_TUNE")) {
			tune_receivers(getenv("TETRA_RX_TUNE"));
		}

	}

	if (getenv("TETRA_SCAN_LIST"))
	{
		scan_list=strdup(getenv("TETRA_SCAN_LIST"));
	} else {
		scan_list=strdup((char *)&def_scan_list);
	}

	if (getenv("TETRA_FREQUENCY_REPORT_FILE"))
	{
		freqreportfile=getenv("TETRA_FREQUENCY_REPORT_FILE");
	} else {
		freqreportfile=(char *)&def_freqreportfile;
	}

}


int main(void)
{
	struct sockaddr_in si_me, si_other;
	int s;
	socklen_t slen=sizeof(si_other);
	unsigned char buf[BUFLEN];
	char *c,*d;
	int len;
	int tport;
	//system("resize -s 60 203"); /* this blocks on some xterms, no idea why */
	if ((s=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP))==-1)
		diep("socket");

	get_cfgenv(); 
	if (getenv("TETRA_PORT"))
	{
		tport=atoi(getenv("TETRA_PORT"));
	} else {
		tport=PORT;
	}

	memset((char *) &si_me, 0, sizeof(si_me));
	si_me.sin_family = AF_INET;
	si_me.sin_port = htons(tport);
	si_me.sin_addr.s_addr = htonl(INADDR_ANY);
	if (bind(s, (struct sockaddr *)&si_me, sizeof(si_me))==-1)
		diep("bind");


	initcur();
	updopis();

	if (grxml_url) {
		xmlNanoHTTPInit();		
		grxml_discover_receiver(grxml_url);
	}
	if (grxml_rx_description) {
		wprintw(statuswin,"Found receiver: %s\n",grxml_rx_description);
		grxml_update_receivers(grxml_url);
		get_scan_range(scan_list,scan_list_item);
	}

	tetraxml_read();
	do_popen();

	ref=0;

	if (getenv("TETRA_KEYS")) {
		c=getenv("TETRA_KEYS");
		while(*c) {
			keyf(*c);
			c++;
		}
	}

	signal(SIGPIPE,SIG_IGN);

	fd_set rfds;
	int nfds;
	int r;
	struct timeval timeout;

	while (1) {
		FD_ZERO(&rfds);
		FD_SET(s,&rfds);
		FD_SET(0,&rfds);
		nfds=max(0,s+1);
		timeout.tv_sec=0;
		timeout.tv_usec=50*1000; //50ms
		r=select(nfds,&rfds,0,0,&timeout);

		//timeout
		if (r==0) tickf();

		if (r==-1) {
			wprintw(statuswin,"select ret -1\n");
			wrefresh (statuswin);
		}

		if ((r>0)&&(FD_ISSET(0,&rfds)))
		{
			len=read(0,buf,1);

			if (len==1) keyf(buf[0]);
		}

		if ((r>0)&&(FD_ISSET(s,&rfds)))
		{
			bzero(buf,BUFLEN);
			len=recvfrom(s, buf, BUFLEN, 0, (struct sockaddr *)&si_other, &slen);

			if (len==-1)
				diep("recvfrom()");


			if ((telive_receiver_mode==TELIVE_RX_NORMAL)||((telive_receiver_mode!=TELIVE_RX_NORMAL)&&(scan_state!=SCANSTATE_NULL))) {
				c=strstr((char *)buf,"TETMON_begin");
				if (c)
				{
					c=c+13;
					d=strstr((char *)buf,"TETMON_end");
					if (d) {
						*d=0;
						parsestat(c);
						ref=1;
					} else
					{
						wprintw(statuswin,"bad line [%80s]\n",buf);
						ref=1;
					}


				} else
				{
					if (len==1393) 
					{ 
						if (newopis()) initopis();
						parsetraffic(buf);		
					} else
					{

						wprintw(statuswin,"### SMALL FRAME: write %i\n",len);
						ref=1; }

				}
			}
		} 
		if (ref) refresh_scr();

	}
	close(s);
	return 0;
}
