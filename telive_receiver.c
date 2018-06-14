/* grxmlrpc.c - communicate with the gnuradio xmlrpc server
 * (c) 2016 Jacek Lipkowski <sq5bpf@lipkowski.org>
 * Licensed under GPLv3, please read the file LICENSE, which accompanies 
 * the telive program sources, or is avaliable from GNU.
 *
 */

#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdint.h>

#include "telive_receiver.h"

#define HTTPBUF_SIZE 8192

#define GRXML_SET 1
#define GRXML_GET 2

/* walk xml tree, get the first "value", and compare the type, break on "fault" */
int getvaluexml(xmlDoc *doc,xmlNode *a_node,char *vartype,char *varvalue,int varlen) {

	xmlNode *cur_node = NULL;
	for (cur_node = a_node; cur_node; cur_node = cur_node->next) {
		int ret;
		if (cur_node->type == XML_ELEMENT_NODE) {
			if (!xmlStrcasecmp(cur_node->name, BAD_CAST"fault")) return(-1);
			if ((varlen)&&(!xmlStrcasecmp(cur_node->name, BAD_CAST"value"))&&(cur_node->children)) {
/*				
//check if the type received is the one requested. commented out for now, because sometimes we might get double, and sometimes int				
				if (xmlStrcasecmp(cur_node->children->name,vartype)) return(0); */
				strncpy(varvalue,(const char *)xmlNodeListGetString(doc, cur_node->children->xmlChildrenNode, 1),varlen);
				return(1);
			}
		}
		ret=getvaluexml(doc,cur_node->children,vartype,varvalue,varlen);
		if (ret) return(ret); 
	}
	return(0);
}

/* send an xmlrpc request to gnuradio, parse the result */
int grxml_send(char *url,int method,char *varname,char *vartype,char *varvalue,int varlen)
{
	void *ctxt;
	char *content_type = "application/xml";
	char postbuf[HTTPBUF_SIZE];
	char outbuf[HTTPBUF_SIZE];
	int ret;
	xmlDocPtr doc;
	int len;
	xmlNode *root_element;

	switch(method) {
		case GRXML_SET: 
			snprintf(postbuf,HTTPBUF_SIZE-1,"<?xml version=\"1.0\"?><methodCall> <methodName>set_%s</methodName><params><param><value><%s>%s</%s></value></param></params></methodCall>\n",varname,vartype,varvalue,vartype);
			break;
		case GRXML_GET: 
			snprintf(postbuf,HTTPBUF_SIZE-1,"<?xml version=\"1.0\"?><methodCall> <methodName>get_%s</methodName></methodCall>\n",varname);
			break;
		default:
			fprintf(stderr,"ERROR: grxml_send unknown method\n"); 
			return(0);
			break;
	}
	postbuf[HTTPBUF_SIZE-1]=0;

	ctxt=xmlNanoHTTPMethod(url, "POST", postbuf, &content_type,NULL,strlen(postbuf)  ); 
	if (!ctxt) { fprintf(stderr,"blad http\n"); return(0); }

	fflush(stdout); fflush(stderr); 
	ret = xmlNanoHTTPReturnCode(ctxt);

	if (content_type) {  xmlFree(content_type); }
	if (ret!=200) {
		xmlNanoHTTPClose(ctxt);
		return(0);
	}

	len = xmlNanoHTTPRead(ctxt, outbuf, sizeof(outbuf));
	xmlNanoHTTPClose(ctxt);

	if (len<1) {
		return(0);
	}

	/* parse return doc */
	doc=xmlReadMemory(outbuf,len,NULL,NULL,0);
	if (!doc) {
		fprintf(stderr,"ERROR: xml parse error\n");
		return(0);
	}
	root_element = xmlDocGetRootElement(doc);
	switch(method) {
		case GRXML_SET: 
			ret=getvaluexml(doc,root_element,NULL,NULL,0);
			break;
		case GRXML_GET: 
			ret=getvaluexml(doc,root_element,vartype,varvalue,varlen);
			break;
		default:
			break;
	}

	xmlFreeDoc(doc);

	if ((method==GRXML_SET)&&(ret==-1))
	{
		fprintf(stderr,"ERROR: xmlrpc fault set(%s,%s,%s)!\n",varname,vartype,varvalue);
		return(0);
	}
	if ((method==GRXML_GET)&&(ret!=1))
	{
		fprintf(stderr,"ERROR: xmlrpc fault get(%s,%s,%s)!\n",varname,vartype,varvalue);
		return(0);
	}

	return(1);
}

/*

// simple test case in case someone wants to use this in their sofware
// it sets and gets a variable called freq
   
int main(int argc,char **argv) 
{
	int ret;
	char buf[64];
	LIBXML_TEST_VERSION
	
	#define URL "http://127.0.0.1:10000/"

	xmlNanoHTTPInit	();
	ret=grxml_send(URL,GRXML_SET,"freq","double",argv[1],0);
	printf("ret=%i\n",ret);
	ret=grxml_send(URL,GRXML_GET,"freq","double",buf,sizeof(buf));
	printf("ret=%i %s\n",ret,buf);

}
*/

int grxml_discover_receiver(char *url) {
	int ret;
	char buf[256];
	if (!grxml_url) return(0);
	ret=grxml_send(url,GRXML_GET,"telive_receiver_name","string",buf,sizeof(buf));
	if (!ret) return(0);
	grxml_rx_description=strdup(buf);
	ret=grxml_send(url,GRXML_GET,"telive_receiver_channels","int",buf,sizeof(buf));
	if (!ret) { free(grxml_rx_description); grxml_rx_description=NULL; return(0); }
	grxml_rx_channels=atoi(buf);

	return(1);

}

