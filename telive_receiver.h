#include <libxml/xmlmemory.h>
#include <libxml/nanohttp.h>
#include <libxml/tree.h>
#include <libxml/parser.h>
#include <libxml/xmlerror.h>
#include <stdint.h>

#ifndef GRXMLRPC_H
#define GRXMLRPC_H
extern char *grxml_url;

enum telive_receiver_modes { 
	TELIVE_RX_NORMAL = 0, /* normal receiver mode */ 
	TELIVE_RX_SCAN_FINDFIRST = 1, /* scan using channel 1, stop when a network is found */
	TELIVE_RX_SCAN_FINDALL = 2, /* scan all frequencies, don't stop  when something is found, repeat */
};
extern int telive_receiver_mode;


enum telive_receiver_autotune {
	TELIVE_AUTOTUNE_DISABLED=0, /* don't attempt autotuning */
	TELIVE_AUTOTUNE_ENABLED=1, /* try to tune receivers using data from RX1 */
};
extern int telive_auto_tune; /* auto tune channels >1 from received frequency data */


#define GRXML_SET 1
#define GRXML_GET 2

int getvaluexml(xmlDoc *doc,xmlNode *a_node,char *vartype,char *varvalue,int varlen);
int grxml_send(char *url,int method,char *varname,char *vartype,char *varvalue,int varlen);

extern int freq_timeout; /* how long we remember frequency info */
extern int receiver_timeout; /* how long we remember receiver info */

extern char *grxml_rx_description;
extern int grxml_rx_channels;

int grxml_discover_receiver(char *url);

enum encryption_options {
	ENCOPTION_UNKNOWN = 0,
	ENCOPTION_DISABLED = 1,
	ENCOPTION_ENABLED = 2
};


/* this structure is used to describe all known frequencies */
struct freqinfo {
	uint32_t rx_freq; /* frequency this info was received on */
	uint32_t dl_freq; /* downlink frequency */
	uint32_t ul_freq; /* uplink  frequency */
	uint16_t mcc; /* MCC */
	uint16_t mnc; /* MNC */
	uint16_t cc; /*  colour code */
	uint16_t la; /* LA */
	time_t last_change; /* timestamp when this entry changed the last time */
	int reason; /* bit flags giving the reason why this entry was found */
	int encoption; /* is the encryption option enabled on in this network */
	int rx; /* which receiver we received this on */
	void *next; 
	void *prev;
};

/* this is a structure with all stuff we got during scanning etc */
struct freqdb {
	uint32_t rx_freq; /* frequency this info was received on */
	uint32_t dl_freq; /* downlink frequency */
	uint32_t ul_freq; /* uplink  frequency */
	uint16_t mcc; /* MCC */
	uint16_t mnc; /* MNC */
	uint16_t cc; /*  colour code */
	uint16_t la; /* LA */
	time_t last_change; /* timestamp when this entry changed the last time */
	int reason; /* bit flags giving the reason why this entry was found */
	int rx; /* which receiver we received this on */
	void *next; 
	void *prev;
};



#endif

