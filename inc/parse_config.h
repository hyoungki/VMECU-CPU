#ifndef  _PARSE_CONFIG_H_
#define  _PARSE_CONFIG_H_


#ifndef _PARSE_CONFIG_C_
#define MY_EXTERN  extern 
#else 
#define MY_EXTERN 
#endif 
 

#define  CONFIG_NAME_MAX  40

typedef struct config_tap
{

	char user[CONFIG_NAME_MAX] ;
	char target[CONFIG_NAME_MAX];
	char link_port [CONFIG_NAME_MAX] ;
	char scu_port [CONFIG_NAME_MAX ] ;  
	

} config_t ;

MY_EXTERN  config_t default_config ;

MY_EXTERN void read_config( int display);
MY_EXTERN void display_logo(void) ;
MY_EXTERN char * get_target_name(void);
MY_EXTERN void display_config_info(void);


#endif 
