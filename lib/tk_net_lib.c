#define TK_NET_LIB_C
//(ip.addr == 10.1.0.175) ||  (tcp.flags.syn == True) 

#include "includes.h"

#define  TK_PRINT_ERROR_MSG(x)	printf("%s: %s %s at %s.%d\r\n",__FUNCTION__,x, strerror(errno),__FILE__,__LINE__); 


#define  TK_LIB_PRINT_ERROR_MSG(x)	printf("LIB-%s: %s %s at %s.%d\r\n",__FUNCTION__,x, strerror(errno),__FILE__,__LINE__); 

/*******************************
 현재 시간 출력
 size는 buffer size.. 음 최소가 필요할 수 있나 ??
*******************************/

void tk_print_curTime(char *buffer,int size)
{
    time_t rawtime;
    struct tm *timeinfo;


    // Get current time
    time(&rawtime);
    timeinfo = localtime(&rawtime);

    // Format: Weekday, Month Day Year - HH:MM:SS
    strftime(buffer, size, "%Y:%m:%d - %H:%M:%S", timeinfo);

//    printf("Formatted time: %s\n", buffer);

    return ;		
}

void tk_print_curTime_onthefly( void)
{
    time_t rawtime;
    struct tm *timeinfo;
    char  buffer[80] ;
    int size;
    
    size=80;

    // Get current time
    time(&rawtime);
    timeinfo = localtime(&rawtime);

    // Format: Weekday, Month Day Year - HH:MM:SS
    strftime(buffer, size, "%Y:%m:%d - %H:%M:%S", timeinfo);
    printf("\r\n%s\r\n",buffer);
//    printf("Formatted time: %s\n", buffer);

    return ;		
}


// 
/**
 * 과거 시간과 현재 시간의 차이를 밀리초 단위로 반환하는 함수
 * @param past_time: 과거 시간 (struct timeval 형식)
 * @return: 시간 차이 (밀리초), 오류 시 -1 반환
 */
int  elapsed_msec(struct timeval past_time) {
    struct timeval current_time;
    
    // 현재 시간 구하기
    if (gettimeofday(&current_time, NULL) != 0) {
        return -1; // gettimeofday 실패
    }
    
    // 과거 시간이 미래 시간인지 확인
    if (current_time.tv_sec < past_time.tv_sec || 
        (current_time.tv_sec == past_time.tv_sec && current_time.tv_usec < past_time.tv_usec)) {
        return -1; // 과거 시간이 현재보다 미래임
    }
    
    // 시간 차이 계산
    long sec_diff = current_time.tv_sec - past_time.tv_sec;
    long usec_diff = current_time.tv_usec - past_time.tv_usec;
    
    // 마이크로초가 음수인 경우 처리
    if (usec_diff < 0) {
        sec_diff--;
        usec_diff += 1000000; // 1초 = 1,000,000 마이크로초
    }
    
    // 밀리초로 변환하여 반환
    return  (int) ( (sec_diff * 1000) + (usec_diff / 1000));
}
 
/* 사장님 pause 대체용 pause 는 singal 에서 사용된다. */

void  msleep( int mSec)
{	
	usleep(mSec*1000) ;
} 

/***********************************************
*    block인 경우는 1회 write로 성공한다.
*	 nonblock인 경우 확인 요 
* 
*   return  n > 0 byte to be written
*			n ==0  ( 일단 error 로 처리...n을 받았는데 timeout 시간동안 전송 못 함.)
*           n < such as -1  write eror such as PIPE BROKEN 또는 system error
*           
*
*  timeout : mSec
************************************************/

int  tk_net_writen(int sockfd, UINT8 *txBuffer, int size,int timeout)
{
    int retval ;
	int totalSize ;
	int remains;
	UINT8  *txData;

	struct timeval start ; 
		
	totalSize = 0;
	remains = size;
		
	txData = txBuffer ;
	
    do
	{
     	gettimeofday( &start, NULL) ;   		
  		//if (( retval = TKwrite(sockfd, txData, remains,timeout)) < 0)	
  		// write 의 return 이  0 이라는 말은 non-blocking 에서 buffer가 꽉 차있다는 것이다.
  		// non-blocking으로 호촐되더라도 tcp buffer가 꽉 찬다는 것이...호출함수에서  error로  
	    // 그리고 대부분은 write 1회로 처리된다.
	    if(( retval = write(sockfd, txData, remains)) <= 0)
	  	{
            //printf("Err-Lib> tk_net_writen %s\r\n",strerror(errno)); 	
            TK_LIB_PRINT_ERROR_MSG("") ;
			return TK_ERROR;	  				
	  	}
	 
	 			 	 	 	 	  		
	  	remains -= retval;	  	
	  	totalSize += retval;
	  	txData +=retval;	
	 	// hkkim 현재는 write 만 호출하기 때문에 time 처리가 안 된다.
	 	if ( remains == 0) 
	 	{	
//	 		printf("tk_net_writen : retval %d\r\n",totalSize);
	 		return totalSize ; 
		}
	    msleep(10); 
	    timeout = timeout -  elapsed_msec(start);
	  	if ( timeout <= 0)  return TK_TIMEOUT ;
	  	 			  		  		  	
    } while ( remains> 0) ;
	  
	return totalSize;
}

#if 0
int  tk_net_write(int sockfd, UINT8 *txBuffer, int size)
{
    // 이건 그냥 write 지...
    int retVal ;
    retVal = write ( sockfd, txBuffer,r size);
    return retVal ;
}
#endif  

/*
*   
FUNC :  최대 지정된 시간동안 1회 ,  size 만큼 읽기 시도
NOTE :  return 0 은 error가 아니다.  
  
  지정된 수만큼 읽어 return
  EOF return
  Timeout return
  sys error return 
  
arg : 
	 timeout is mS Unit
return  : 
 n 만큼 읽을 수도 있고
 0 연결이 끊어 질 수도 있고  ( 중간에 EOF 로 EOF 로 처리)
 TK_TIMEOUT (-2) timout
 TK_SYS_ERR (3) Eror가 발생
 tm : msec 단위
 
*/


int tk_net_read(int fd, char *buf, int size , int tm)
{
    int retval;
    fd_set  rset;      
    struct timeval timeout;

    FD_ZERO(&rset);
    FD_SET(fd,&rset);  

//    printf("%s\n\r",__FUNCTION__); 
  
    if (tm)  // tm 이 있으면 timeval 구조체로 변환
    {
        if ( tm < 1000)
        {
            timeout.tv_usec = tm*1000 ;
            timeout.tv_sec =  0 ;
        }
        else
        {
            timeout.tv_usec = (tm%1000)*1000 ; 
            timeout.tv_sec =  tm/1000 ;          
        }
        retval =select(fd+1,&rset,NULL,NULL,&timeout);  
    }
    else   /* wait for ever */
        retval =select(fd+1,&rset,NULL,NULL,NULL);

   // select는 >0 , 0 (timeout), -1 ( sys error) 
  //   printf("select retVal[%d]\n\r",retval); 
 
   if ( retval > 0) /* 여기에 EOF 도 포함 */
   {
    	if ( FD_ISSET(fd, &rset))
    	{
        	/* TPC retval=0 means End-of-Connetion */
        	retval = read(fd, buf, size);
        	return retval ; 
    	}   	
   		else return TK_SYS_ERR ;
   			
   }
   else if ( retval ==0 )
   {
   		return TK_TIMEOUT ; 
   }
   else // retval < 0
   {
    // signal을 받았다는 건...무슨 의미일까... 일단 error 로 다시 처리하자.
    //    if ( errno == EINTR) /* select 가 signal을 받았다 */
    //    	return TK_TIMEOUT ;    	  
   	  	return TK_SYS_ERR ;
   	  
   }

 
 


     
}
 

/******************************************************
* 지정된 timeout 동안 Loop를 수행하며 최대 요청받은 수 만큼 읽기시도
* return  n > 0 read byte
*         n = 0 close socket  TK_SOCKET_EOF
*         n = -1 timeout     or   TK_TIMEOUT
*         n = -2 system error  except EINTR  TK_SYS_ERR
* to-do 사실은 TKread 마다 timeout을 제외해야 된다.

 note : tkread return o if EOF 
 이 함수는 return 0 은 없다.

이 함수의 return 값에 0 은 없다
요청받은 것을 return 못하는 이유

*******************************************************/


int tk_net_readn (int fd, char *buf, int size , int timeout)
{
  int totalSize;
  char *ptr = buf;
  int retval ; 
  struct timeval start ;  
  int  readSize;  // 읽기 요청 size 
  
  
  totalSize = 0 ;
  readSize= size ; 
    
//  printf("IN %s \r\n",__FUNCTION__);   
  while ( totalSize < size)
  {
     gettimeofday( &start, NULL) ;      
     retval = tk_net_read(fd, ptr,readSize,timeout);
	 
//	 printf("tk_net_read return %d\r\n",retval);
	 if ( retval <=0 ) 
	 {	    // == 0 만약 앞에 읽은 것이 있으면...?? 	 		
	 		if ( retval ==0 )  return TK_SOCKET_EOF ; 	// 여기서 TK_SOCET_EOF 가 0 이 아닌이유는 ??
	 												    // 2025-10-10 오후 1:59:21 TCP에서 return 0 은 EOF 이다.
	 													// 2025-10-10 오후 2:03:36  TK_SOCEKT_EOF를 0 으로 하고 ..검토
	 			 		 	 		                    // 이 부분 나도 고민스벏네..return total ?
	 		if ( retval == TK_SYS_ERR )    return TK_SYS_ERR  ; 
	 		// timeout 
	 		timeout -= elapsed_msec(start);
	 		
	 		if ( timeout <=0 )  return TK_TIMEOUT ;
	 		else continue ; 
	 }	 	
       
     totalSize +=retval;
     readSize -= retval ; 
     ptr +=retval ;
     
     // totalSize > size는 발생하면 안 됨....
     if ( totalSize >= size) 
     {
//     		printf("%s return %d\r\n",__FUNCTION__, totalSize);
     		return totalSize ;  
     }
     
	 // 2025-10-10 오후 2:00:53 남은 시간 다시 계산.
	 
	 timeout -= elapsed_msec(start);    
	 if ( timeout <=0 ) return totalSize ;  
        
  }
  // 여기 올 수 없다. 
	TK_LIB_PRINT_ERROR_MSG("can't be here");      
  // printf("%s system err %d\r\n",__FUNCTION__, totalSize);  
  return TK_SYS_ERR;
}
 


int tk_connect_once (int domain, int type, int protocol,
                              const struct sockaddr *addr, socklen_t addrlen,
                              int timeout_msec,  int *sockfd_out) 
{
		return tk_connect( domain, type, protocol, addr, addrlen, timeout_msec, 1, 500, sockfd_out) ;		
}

 
 /*********************************************
 
 
 성공하면 0,
 실패하면 -1 
 
 ************************************************/
 
/*
예
	if (( retVal = tk_connect (AF_INET, SOCK_STREAM, IPPROTO_TCP ,
					(const struct sockaddr *) &dnp->serv_addr, sizeof(dnp->serv_addr),
					2000, 2, 500, &dnp->SocketFd )) < 0 )


 * Custom connect function with retry and timeout support
 * Creates new socket for each retry attempt as per POSIX standards
 * 
 * @param domain: socket domain (e.g., AF_INET)
 * @param type: socket type (e.g., SOCK_STREAM)
 * @param protocol: socket protocol (usually 0)
 * @param addr: pointer to socket address structure
 * @param addrlen: size of the address structure
 * @param timeout_sec: timeout in seconds for each connection attempt
 * @param max_retries: maximum number of retry attempts
 * @param retry_delay_sec: delay between retries in seconds
 * @param sockfd_out: pointer to store the successful socket fd
 * 
 * @return: 0 on success with sockfd_out set, -1 on failure
 */
int tk_connect (int domain, int type, int protocol,
                const struct sockaddr *addr, socklen_t addrlen,
                int timeout_msec, int max_retries, int retry_delay_msec,
                int *sockfd_out) {
    int sockfd, result;
    fd_set write_fds, error_fds;
    struct timeval tv;
    int so_error;
    socklen_t so_error_len = sizeof(so_error);
    
    int attempt ;
 	int flags   ;
    
    sockfd = -1 ;
    
    // 입력 조건 확인...
    if (addr == NULL || timeout_msec <= 0 || max_retries < 0 || sockfd_out == NULL) {
        errno = EINVAL;
       // printf("%s :  got invalid args\r\n",__FUNCTION__);
    	sockfd = -1 ;
        *sockfd_out = sockfd; 
        return -1;
    }
    
//

    
    for (attempt = 0; attempt < max_retries; attempt++) {
               	
        if (attempt > 0) {
            //printf("Connection attempt %d/%d failed, retrying in %d mseconds...\n", 
              //     attempt+1, max_retries, retry_delay_msec);
            msleep(retry_delay_msec);
        }
 		//else 
 		  //   printf("Connection attempt %d/%d failed, retrying in %d mseconds...\n", 
            //       attempt+1, max_retries, retry_delay_msec);
                          
        
        // Create new socket for each attempt (POSIX requirement)
        sockfd = socket(domain, type, protocol);
        
        if (sockfd == -1) {
            printf("%s : socket creation failed [%s]", __FUNCTION__,strerror(errno));          
            *sockfd_out = sockfd;            
            return -1;
        }
        
        // Set socket to non-blocking mode for timeout control
        flags = fcntl(sockfd, F_GETFL, 0);
        
        if (flags == -1) {
            close(sockfd);
            sockfd=-1; 
            *sockfd_out = sockfd;  
            printf("fcntl(F_GETFL) failed on attempt %d: %s\n", attempt + 1, strerror(errno));
            // fcntl failure might be temporary system issue, so retry
 			return -1;
        }
        
        if (fcntl(sockfd, F_SETFL, flags | O_NONBLOCK) == -1) {
            close(sockfd);
            sockfd=-1;             
            *sockfd_out = sockfd;              
            printf("fcntl(F_SETFL) failed on attempt %d: %s\n", attempt + 1, strerror(errno));   
            return -1 ; 
        }
        
        // Attempt to connect
        result = connect(sockfd, addr, addrlen);
        
        if (result == 0) {
            // Connection succeeded immediately
            // Restore blocking mode
            fcntl(sockfd, F_SETFL, flags);
         //   printf("%s : Connection successful immediately on attempt %d\n",__FUNCTION__, attempt + 1);
            *sockfd_out = sockfd;
            return 0;
        }
        
        if (errno == EINTR) {
            // Connection was interrupted by signal, retry with same socket
            close(sockfd);
			sockfd=-1;
		//	printf("%s : connect EINTR error\r\n",__FUNCTION__) ;
            continue; // Try again with new socket
        } else if (errno != EINPROGRESS) {
            // Connection failed immediately with an error other than EINPROGRESS
            close(sockfd);
            sockfd=-1;
		//	printf("%s : connect error[%s]\r\n",__FUNCTION__,strerror(errno)) ;
            continue; // Try again with new socket
        }
        
        // Connection is in progress, wait for completion with timeout
        FD_ZERO(&write_fds);
        FD_ZERO(&error_fds);
        FD_SET(sockfd, &write_fds);
        FD_SET(sockfd, &error_fds);
        
            // Convert milliseconds to timeval
            tv.tv_sec = timeout_msec / 1000;
            tv.tv_usec = (timeout_msec % 1000) * 1000;
        
        
        result = select(sockfd + 1, NULL, &write_fds, &error_fds, &tv);
        
        if (result == 0) {
            // Timeout occurred
            close(sockfd);
            sockfd = -1 ;
            errno = ETIMEDOUT;
            sockfd=-1;
            continue; // Try again with new socket
        }
        
        if (result == -1) {
        	       if (errno == EINTR) {
                 	   printf("%s: select interrupted by signal, retrying...\r\n", __FUNCTION__);
                  //  	continue; // Retry select without consuming an attempt
	                close(sockfd);
	                sockfd = -1 ;
					continue ;
                }
        	
            // select() error
            close(sockfd);
             sockfd = -1 ;           
            *sockfd_out = sockfd; 
		//	printf("%s : select error[%s]\r\n",__FUNCTION__,strerror(errno)) ;            
            return -1;
        }
        
        // Check if socket is ready for writing (connection successful) or has error
        if (FD_ISSET(sockfd, &write_fds) || FD_ISSET(sockfd, &error_fds)) {
            // Get socket error status
            if (getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &so_error, &so_error_len) == -1) {
                close(sockfd);
    			sockfd = -1 ;           
    			*sockfd_out = sockfd;                   
			//	printf("%s : getsockopt error[%s]\r\n",__FUNCTION__,strerror(errno)) ;    
			    TK_PRINT_ERROR_MSG("getsockopt") ; 
                return -1;
            }
            
            if (so_error == 0) {
                // Connection successful
                // Restore blocking mode
                fcntl(sockfd, F_SETFL, flags);
                //printf("Connection successful on attempt %d\n", attempt + 1);
                *sockfd_out = sockfd;
                return 0;
            } else {
            	// device가 바로 RST를 보내는 경우..
                // Connection failed
                close(sockfd);
				sockfd = -1 ; 
                errno = so_error;
			//    printf("%s : getsockopt error[%s]\r\n",__FUNCTION__,strerror(errno)) ;  
			//    TK_PRINT_ERROR_MSG("getsockopt_so_error") ; 
                continue; // Try again with new socket
            }
        }
    }
 
#if 0 // 2025-10-23 오후 4:35:52 
        // Check retry count 	
       if (attempt == max_retries) {
    		  sockfd = -1 ;           
    		  *sockfd_out = sockfd; 
              //printf("%s : over retryCount[%d]\r\n", __FUNCTION__ , attempt);      		  
              return -1;
        } 
#endif         
    
    // All attempts failed
    //errno = ETIMEDOUT;
    //?
    //fcntl(sockfd, F_SETFL, flags);    
    sockfd = -1 ;           
    *sockfd_out = sockfd;     
    return -1;
}






/*********************************/
/*
*   
FUNC :  최대 지정된 시간동안 1회 ,  size 만큼 읽기 시도
NOTE :  return 0 은 error가 아니다.  
  
  지정된 수만큼 읽어 return
  EOF return
  Timeout return
  sys error return 
  
arg : 
	 timeout is mS Unit
return  : 
 n 만큼 읽을 수도 있고
 0 연결이 끊어 질 수도 있고  ( 중간에 EOF 로 EOF 로 처리)
 TK_TIMEOUT (-2) timout
 TK_SYS_ERR (3) Eror가 발생
 tm : msec 단위
 
*/


int tk_read(int fd, char *buf, int size , int tm)
{
    int retval;
    fd_set  rset;      
    struct timeval timeout;

    FD_ZERO(&rset);
    FD_SET(fd,&rset);  

//    printf("%s\n\r",__FUNCTION__); 
  
    if (tm)  // tm 이 있으면 timeval 구조체로 변환
    {
        if ( tm < 1000)
        {
            timeout.tv_usec = tm*1000 ;
            timeout.tv_sec =  0 ;
        }
        else
        {
            timeout.tv_usec = (tm%1000)*1000 ; 
            timeout.tv_sec =  tm/1000 ;          
        }
        retval =select(fd+1,&rset,NULL,NULL,&timeout);  
    }
    else   /* wait for ever */
        retval =select(fd+1,&rset,NULL,NULL,NULL);

   // select는 >0 , 0 (timeout), -1 ( sys error) 
   //   printf("select retVal[%d]\n\r",retval); 
 
   if ( retval > 0) /* 여기에 EOF 도 포함 */
   {
    	if ( FD_ISSET(fd, &rset))
    	{
        	/* 이 것이 0 이 나올 수 있나 ???   나올 수는 있다는데 매우 드물다..무시하자.*/
        	retval = read(fd, buf, size);
        	return retval ; 
    	}   	
   		else return TK_SYS_ERR ; // -3
   			
   }
   else if ( retval ==0 )
   {
   		return TK_TIMEOUT ; // -2
   }
   else // retval < 0
   {
    // signal을 받았다는 건...무슨 의미일까... 일단 error 로 다시 처리하자.
    //    if ( errno == EINTR) /* select 가 signal을 받았다 */
    //    	return TK_TIMEOUT ;    	  
   	  	return TK_SYS_ERR ;
   	  
   }

 
 


     
}
 

/******************************************************
* 지정된 timeout 동안 Loop를 수행하며 최대 요청받은 수 만큼 읽기시도
* return  n > 0 read byte
*         n = 0 close socket  TK_SOCKET_EOF
*         n = -1 timeout     or   TK_TIMEOUT
*         n = -2 system error  except EINTR  TK_SYS_ERR
* to-do 사실은 TKread 마다 timeout을 제외해야 된다.

 note : tkread return o if EOF 
 이 함수는 return 0 은 없다.

이 함수의 return 값에 0 은 없다
요청받은 것을 return 못하는 이유

*******************************************************/


int tk_readn (int fd, char *buf, int size , int timeout)
{
  int totalSize;
  char *ptr = buf;
  int retval ; 
  struct timeval start ;  
  int  readSize;  // 읽기 요청 size 
  
  
  totalSize = 0 ;
  readSize= size ; 
    
//  printf("IN %s \r\n",__FUNCTION__);   
  while ( totalSize < size)
  {
     gettimeofday( &start, NULL) ;      
     retval = tk_read(fd, ptr,readSize,timeout);
	 
//	 printf("tk_net_read return %d\r\n",retval);
	 if ( retval < 0 ) //  retval 0은 제외 
	 {	    // == 0 만약 앞에 읽은 것이 있으면...?? 	 		
	 		//if ( retval ==0 )  return totalSize ; /* 지금까지 받은 것 */ 	
	 		if ( retval == TK_SYS_ERR )    return TK_SYS_ERR  ; 
	 		// timeout 
	 		timeout -= elapsed_msec(start);
	 		
	 		if ( timeout <=0 )  return TK_TIMEOUT ;
	 		else continue ; 
	 }	 	
       
     totalSize +=retval;
     readSize -= retval ; 
     ptr +=retval ;
     
     // totalSize > size는 발생하면 안 됨....
     if ( totalSize >= size) 
     {
//     		printf("%s return %d\r\n",__FUNCTION__, totalSize);
     		return totalSize ;  
     }
     
	 // 2025-10-10 오후 2:00:53 남은 시간 다시 계산.
	 
	 timeout -= elapsed_msec(start);    
	 if ( timeout <=0 ) return totalSize ;  
        
  }
  // 여기 올 수 없다. 
	TK_LIB_PRINT_ERROR_MSG("can't be here");      
  // printf("%s system err %d\r\n",__FUNCTION__, totalSize);  
  return TK_SYS_ERR;
}



int open_serial(TTY_DESC *tp)
{
	//PortClose(tp);
    close(tp->id);
    tp->id = -1 ; 
    
	if ((tp->id = open(tp->name,  O_RDWR | O_NOCTTY | O_NDELAY)) < 0)
	{
	    printf("portOpen[%s] > err#1 %s\n", tp->name, strerror(errno));
		return(-1);
    }
    else
        printf("open_serial : %s open fd[%d]\r\n",tp->name, tp->id) ;
        

    
    return 0;
}
