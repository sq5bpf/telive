enum tetra_mac_res_addr_type {
	ADDR_TYPE_NULL  = 0,
	ADDR_TYPE_SSI   = 1,
	ADDR_TYPE_EVENT_LABEL   = 2,
	ADDR_TYPE_USSI          = 3,
	ADDR_TYPE_SMI           = 4,
	ADDR_TYPE_SSI_EVENT     = 5,
	ADDR_TYPE_SSI_USAGE     = 6,
	ADDR_TYPE_SMI_EVENT     = 7,
};

void tune_free_receiver(char *url,int src_rxid,uint32_t dlf);

void grxml_autocorrect_ppm (char *) ;
void grxml_update_receivers(char *url);
void scan_tune(char *url);
void set_grxml_baseband(char *url,uint32_t  freq);
void set_grxml_ppm(char *url,float ppm);
 void set_grxml_gain(char *url,int gain);
