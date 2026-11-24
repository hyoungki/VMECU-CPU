#include    "localLib.h"
#include    "eventmsg.h"


#define	VMECU_LOG_HOME	        "/mnt"
#define	VMECU_LOG_CPU_A_NAME	"vmecu-A.log"
#define	VMECU_LOG_CPU_B_NAME	"vmecu-B.log"
#define	VMECU_LOG_SIZE	(400 * 1024)	// 40 kbytes
// logging file이 filesize 보다 크면, 백업을 만든다.


int  LogFile_MPU (OPR_MSG *opr, int code, char *outstr, int osize)
{
	
#ifdef  LOG_FILE_CREATE	

    int     pos;
    char    buffer[256];
	char    fname[64], fnameOld[64], *fopen_type;
	struct stat stat_buf;
	FILE    *fp;

    struct timeval  logT;
    struct tm cTime;
    int     milisec;
    
    /* SOE 이벤트 시간 추출 */
    gettimeofday(&logT, NULL);
    //cTime = localtime(&logT.tv_sec);
    localtime_r (&logT.tv_sec, &cTime);
    
    milisec = logT.tv_usec / 1000;
    
    sprintf(buffer, "%4d-%02d-%2d %02d:%02d:%02d.%03d  ", 
        cTime.tm_year + 1900, cTime.tm_mon +1, cTime.tm_mday, cTime.tm_hour, cTime.tm_min, cTime.tm_sec, milisec);
  
    pos = strlen(buffer);
    sprintf(&buffer[pos], "%s  %s \n", errmsg[code], outstr);


    /* ---------------------------------------- */
    /*  LOG File 이름 지정 ...                  */
    /* ---------------------------------------- */
    if(opr->cpuMode == LOCAL_MASTER)        sprintf (fname, "%s/%s", VMECU_LOG_HOME, VMECU_LOG_CPU_A_NAME);
    else if(opr->cpuMode == LOCAL_SLAVE)    sprintf (fname, "%s/%s", VMECU_LOG_HOME, VMECU_LOG_CPU_B_NAME);    
    else
    {
        printf("log> *Invalid CPU Mode ... %d\n", opr->cpuMode);
        return (0);
    }               
	
	/* check filesize */
	if (stat (fname, &stat_buf) >= 0)
    {
		if (stat_buf.st_size > VMECU_LOG_SIZE)
		{
			sprintf (fnameOld, "%s.old", fname);
			if (rename (fname, fnameOld) < 0)
			{
				printf ("file : rename (%s --> %s) error, %d : %s\n",
						fname, fnameOld, errno, strerror(errno));
			}
			fopen_type = "w";
		}
		else
			fopen_type = "a+";
	}
	else
	{
        //printf ("file : stat (%s) error, %d : %s\n",
        //        fname, errno, strerror(errno));
		fopen_type = "w";
    }

	if ((fp = fopen (fname, fopen_type)) == NULL)
	{
		printf ("file : file open (%s,%s) error,  %d : %s\n",
				fname, fopen_type, errno, strerror(errno));
		return (-1);
	}
	
	fprintf (fp, "%s", buffer);
	fclose(fp);
	
	//printf("LOG> %s", buffer);
	
#else

    //printf("LOG> %s", buffer);

#endif
    
	return (1);
}
