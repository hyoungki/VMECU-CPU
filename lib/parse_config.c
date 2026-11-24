#define _PARSE_CONFIG_C_ 


#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>

#include "configini.h"

#include "parse_config.h"

#define CONFIGREADFILE "/mnt/config.cfg"
#define CONFIGREADFILE_LOCAL  "./config.cfg"




#if 0
int print_config_info(void)
{
#if 0
	printf("S1 : %d\r\n",default_config.s1);
	printf("S2 : %d\r\n",default_config.s2);
	printf("S3 : %d\r\n",default_config.s3);
	printf("S4 : %d\r\n",default_config.s4);
	printf("S5 : %d\r\n",default_config.s5);
	printf("S6 : %d\r\n",default_config.s6);
	printf("S7 : %d\r\n",default_config.s7);
	printf("S8 : %d\r\n",default_config.s8);
	printf("led : %d\r\n",default_config.led);
	printf("wdt : %d\r\n",default_config.wdt);
#endif 								
        
	return 0;

}
#endif 


#define __stringify_1(x...)	#x
#define __stringify(x...)	__stringify_1(x)


void display_config_info(void)
{
	Config *cfg = NULL;
	if (ConfigReadFile(CONFIGREADFILE, &cfg) != CONFIG_OK) {
		printf("ConfigOpenFile failed for %s", CONFIGREADFILE);
		
		if (ConfigReadFile(CONFIGREADFILE_LOCAL, &cfg) != CONFIG_OK)
		{
		printf("ConfigOpenFile failed for %s", CONFIGREADFILE_LOCAL);		
		        return;
	
	        }
	
  }
		 ConfigPrintSettings(cfg, stdout);
  	 ConfigPrint(cfg, stdout);	
	
	
	printf("%s : %s\r\n",__stringify(default_config.user), default_config.user);
	printf("%s : %s\r\n",__stringify(default_config.target), default_config.target);
	printf("%s : %s\r\n",__stringify(default_config.link_port), default_config.link_port);
	printf("%s : %s\r\n",__stringify(default_config.scu_port), default_config.scu_port);	
	
} 

void  read_config(int display)
{
	Config *cfg = NULL;
	if (ConfigReadFile(CONFIGREADFILE, &cfg) != CONFIG_OK) {
		printf("ConfigOpenFile failed for %s", CONFIGREADFILE);
		
		if (ConfigReadFile(CONFIGREADFILE_LOCAL, &cfg) != CONFIG_OK)
		{
		printf("ConfigOpenFile failed for %s", CONFIGREADFILE_LOCAL);		
		        return;
	
	        }
	}
     
//		 ConfigPrintSettings(cfg, stdout); --> CLI¿¡ Ãß°¡.
//		ConfigPrint(cfg, stdout);
#if 0	
    ConfigReadInt( cfg, "SYSTEM" , "USER", &default_config.led, 0);
    ConfigReadInt( cfg, "DEVICE" , "WDT", &default_config.wdt, 0);
    ConfigReadInt( cfg, "DEVICE" , "RTC", &default_config.rtc, 0);
    ConfigReadInt( cfg, "DEVICE" , "IO", &default_config.io, 0);
    ConfigReadInt( cfg, "DEVICE" , "TTYS0", &default_config.serial[0],0);    
    ConfigReadInt( cfg, "DEVICE" , "TTYS1", &default_config.serial[1],0);  
    ConfigReadInt( cfg, "DEVICE" , "TTYS2", &default_config.serial[2],0);
    ConfigReadInt( cfg, "DEVICE" , "TTYS3", &default_config.serial[3],0);
    ConfigReadInt( cfg, "DEVICE" , "TTYS4", &default_config.serial[4],0);
    ConfigReadInt( cfg, "DEVICE" , "TTYS5", &default_config.serial[5],0);
    ConfigReadInt( cfg, "DEVICE" , "TTYS6", &default_config.serial[6],0);                   		
    ConfigReadInt( cfg, "DEVICE" , "TTYS7", &default_config.serial[7],0);
    ConfigReadInt( cfg, "DEVICE" , "TTYS8", &default_config.serial[8],0);   
    ConfigReadInt( cfg, "DEVICE" , "TTYS9", &default_config.serial[9],0);  
    ConfigReadInt( cfg, "DEVICE" , "TTYS10", &default_config.serial[10],0);  
    
//	ConfigReadString(cfg, CONFIG_SECTION_FLAT, "Mehmet Akif Ersoy", s, sizeof(s), "Poet");
    strcpy( default_config.net_param[0].name, "eth0");
    strcpy( default_config.net_param[1].name, "eth1");
#endif 
	  ConfigReadString(cfg, "SYSTEM", "USER", ( char *)default_config.user, CONFIG_NAME_MAX , "ACE");
	  ConfigReadString(cfg, "SYSTEM", "TARGET", ( char *)default_config.target, CONFIG_NAME_MAX , "VMECU-NET-V11");
	

	  ConfigReadString(cfg, "DEVICE", "LINK_PORT", ( char *)default_config.link_port, CONFIG_NAME_MAX , "/dev/ttyCPM1");
	  ConfigReadString(cfg, "DEVICE", "SCU_PORT", ( char *)default_config.scu_port, CONFIG_NAME_MAX , "/dev/ttyCPM2");	  

    /*
    
    ConfigRet ConfigReadInt(const Config *cfg, const char *section, const char *key,
		int *value, int dfl_value)
    */
//    if ( display) 
//    	print_config_info();
	  ConfigFree(cfg);
	
}


#define msleep(x)   usleep(x*1000)
// #define TWO_TAP     printf("\t\t")
#define TWO_TAP    
#include "common.h"



void display_logo(void)
{
	
	msleep(300);
	printf("\n\n");
	TWO_TAP;printf("////////////////////////////////////////////////////////\n");  msleep(100);
	TWO_TAP;printf("        %s CPU Module   \n", default_config.target); msleep(100);
	TWO_TAP;printf("        [*] Designed by %s \n", default_config.user );  msleep(100);
	TWO_TAP;printf("        [*] Version   %s    \n", VERSION_STRING);msleep(100);
	TWO_TAP;printf("        [*] %s    \n", SIMULATOR_NAME);  pause(100);
	TWO_TAP;printf("        [*] Update Date ...%s    \n", DATE_STRING);msleep(100);
	TWO_TAP;printf("////////////////////////////////////////////////////////\n\n");  msleep(100);	
	
	
}

char * get_target_name(void)
{
	return default_config.target ; 
		
}