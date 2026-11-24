//==============================================================================
//
// File:	detExec.c
//
//==============================================================================

//
// Include File
//
#include    "cli.h"
#include    "external.h"
//
// External Variables
//
extern  RTC         *rtc;
extern  OPR_MSG     *opr;

extern  CMD_ENTRY	cmdTable[MAX_COMMAND];
extern  TTY_DESC    consolePort;


// hkkim
#include <readline/readline.h>
#include <readline/history.h>


//
// Global Variables
//
char	command[CMD_BUF_LEN], *cmdPtr;
char	token[TOKEN_LENGTH];

#if	0
/******************************************************************************
 *
 * 모듈명:	_divide()
 *
 ******************************************************************************/

int _divide(int dividend, int divider)
{
	if (divider == 0 || dividend == 0)
		return(0);
	return((dividend + (divider/2))/divider);
}

/******************************************************************************
 *
 * 모듈명:	_divideFull()
 *
 ******************************************************************************/
 
int _divideFull(int dividend, int divider)
{
	if (divider == 0 || dividend == 0)
		return(0);
	return(dividend/divider + ((dividend % divider) != 0 ? 1 : 0));
}

/************************************************************************
 *
 * 모듈명:	_getHexa()
 *
 ************************************************************************/
 
int _getHexa(int lowLimit, int highLimit)
{
	int	value, index;
 
	for (index = 0, value = 0; index < strlen(token); index++)
	{
		if (isxdigit(token[index]) == 0)
			return(-1);
		if ((token[index] >= '0') && (token[index] <= '9'))
			value = value * 16 + (token[index] - '0');
		else	value = value * 16 + (tolower(token[index]) - 'a' + 10);
	}
 
	if ((lowLimit >= 0) && (value < lowLimit))
		return(-1);
	if ((highLimit >= 0) && (value > highLimit))
		return(-1);
	return(value);
}

void _getAnyKey(void)
{
	printf("type <ENTER> to continue");
	fgets(command, COMMAND_BUFFER_LENGTH, stdin);
}
#endif



//
// 모듈:	GetWord()
//
WORD GetWord1(char *ptr)
{
	WORD	value;

	value = (BYTE)*ptr + (BYTE)*(ptr+1) * 256;
	return(value);
}

//
// 모듈:	PrnLine()
//
void PrnLine(char mode)
{
	char	buffer[84];

	memset(buffer, mode, 80);
	buffer[80] = 0;
	printf("%s\r\n", buffer);
}

//
// 모듈:	PrnSubTitle()
//
void PrnSubTitle(char *title)
{
	int		idx;
	char	*ptr;

	printf("%s\r\n", title);
	for (idx = 0, ptr = title; idx < strlen(title); idx++, ptr++)
		printf("%c", *ptr != ' ' ? '-' : *ptr);
	printf("\r\n");
}



//
// Module:	GetToken()
//
int GetToken(void)
{
	int		index, byteCount;

	memset(token, 0, TOKEN_LENGTH);
	for (index = 0, byteCount = 0;; index++, cmdPtr++)
	{
		switch (*cmdPtr)
		{
		case ',':
		case ' ':
		case ':':
		case ';':
		case 0x0D:
		case 0x0A:
			if (byteCount == 0)
				break;
			else
			{
				token[byteCount] = 0;
				return(byteCount);
			}
			break;
		case 0x00:
			if (byteCount == 0)
				return(0);
			else
			{
				token[byteCount] = 0;
				return(byteCount);
			}
			break;
		default:
			if (byteCount <= TOKEN_LENGTH - 1)
			{
				token[byteCount] = *cmdPtr;	
				byteCount++;
			}
			break;
		}
	}	
}

//
//
// 모듈:	CheckEOT()
//
int CheckEOT(void)
{
	if (GetToken() > 0)
	{
		printf("%%%%ERR_입력 %s 이상\n", token);
		return(-1);
	}
	return(0);
}

//
// Module:	GetNumber()
//
int GetNumber(int low, int high, int *value, char *message)
{
	int		index, length;
 
	if ((length = GetToken()) <= 0)
	{
		printf("%%%%ERR_Need %s Input\n", message);
		return(E_ERR);
	}

	for (index = 0; index < length; index++)
	{
		if (token[index] == 0)
			break;
		if (isdigit(token[index]) == 0)
		{
			printf("%%%%ERR_Invalud Character %c input\n", token[index]);
			return(E_ERR);
		}
	}
 
	if ((*value = atoi(token)) < 0)
	{
		printf("%%%%ERR_Input must be in %d to %d\n", low, high);
		return(E_ERR);
	}
 
	if (*value < low || *value > high)
	{
		printf("%%%%ERR_Input must be in %d to %d\n", low, high);
		return(E_ERR);
	}
	return(E_OK);
}

//
// Module:	GetNumber()
//
int GetNumber2(int low, int high, int *value1, int *value2, char *message)
{
	int		index, length;
 
    /* ---------------------------- */
    /* INPUT#1 에 대한 입력 처리    */
    /* ---------------------------- */
	if ((length = GetToken()) <= 0)
	{
		printf("%%%%ERR_Need %s Input\n", message);
		return(E_ERR);
	}

	for (index = 0; index < length; index++)
	{
		if (token[index] == 0)
			break;
		if (isdigit(token[index]) == 0)
		{
			printf("%%%%ERR_Invalud Character %c input\n", token[index]);
			return(E_ERR);
		}
	}
 
	if ((*value1 = atoi(token)) < 0)
	{
		printf("%%%%ERR_Input must be in %d to %d\n", low, high);
		return(E_ERR);
	}
 
	if (*value1 < low || *value1 > high)
	{
		printf("%%%%ERR_Input must be in %d to %d\n", low, high);
		return(E_ERR);
	}
	
    /* ---------------------------- */
    /* INPUT#2 에 대한 입력 처리    */
    /* ---------------------------- */
	if ((length = GetToken()) <= 0)
	{
		printf("%%%%ERR_Need %s Input\n", message);
		return(E_ERR);
	}

	for (index = 0; index < length; index++)
	{
		if (token[index] == 0)
			break;
		if (isdigit(token[index]) == 0)
		{
			printf("%%%%ERR_Invalud Character %c input\n", token[index]);
			return(E_ERR);
		}
	}
 
	if ((*value2 = atoi(token)) < 0)
	{
		printf("%%%%ERR_Input must be in %d to %d\n", low, high);
		return(E_ERR);
	}
 
	if (*value2 < low || *value2 > high)
	{
		printf("%%%%ERR_Input must be in %d to %d\n", low, high);
		return(E_ERR);
	}
	
	return(E_OK);

}


//hkkim
//#define debugf(...)
#define debugf
//#define  debugf printf 


static char *
dupstr (s)
     char *s;
{
  char *r;

  r = malloc (strlen (s) + 1);
  strcpy (r, s);
  return (r);
}

/* readline completetion */
/* Generator function for command completion.  STATE lets us know whether
   to start from scratch; without any state (i.e. STATE == 0), then we
   start at the top of the list. */
char *
command_generator (text, state) // 아 입력이 3 글자이면 1, 2, 3, 호출한다.
     const char *text;
     int state;
{
  static int list_index, len;  
//  char *name;
//  CMD_ENTRY	*cp;    
//	cp = cmdTable;
  
//   debugf("---> %s\r\n",__FUNCTION__);
//   debugf("test %s\r\n",text);   
//   debugf("state  %d\r\n",state);   
  /* If this is a new word to complete, initialize now.  This includes
     saving the length of TEXT for efficiency, and initializing the index
     variable to 0. */
  if (!state) // start from ...아 문자수인가...예로 de 하면 2글자..
    {
      list_index = 0;  // 아 static 이네..
      len = strlen (text);
    }
#if 0
// 여기는 commands 위에 있는 예제
  /* Return the next name which partially matches from the command list. */
//  while (name = commands[list_index].name)  //  대입이잔항...list-index는 순서 찾는 거고...
  while (name = commands[list_index].name !)  //  대입이잔항...list-index는 순서 찾는 거고...
    {       // name 은 명령어 전체를 가르키는 pointer
        //cmdTable
      list_index++;

      if (strncmp (name, text, len) == 0)
      {  
//         debugf("<--- %s found %s\r\n",name, __FUNCTION__);        
        return (dupstr(name)); // 찾은 명령어를 복서해서 전달.
       }
    }
#else
//  while ( strcmp(cmdTable[list_index].name , "end") !=0  )  //  대입이잔항...list-index는 순서 찾는 거고...
  while ( strlen(cmdTable[list_index].name) !=0  )  //  대입이잔항...list-index는 순서 찾는 거고...
    {       // name 은 명령어 전체를 가르키는 pointer
        //cmdTable
      list_index++;

      if (strncmp (( char *)cmdTable[list_index].name, text, len) == 0)
      {  
//        debugf("<--- %s found %s\r\n",cmdTable[list_index].name, __FUNCTION__);        
        return (dupstr(   (char *)cmdTable[list_index].name    )); // 찾은 명령어를 복서해서 전달.
       }
    }


#endif  



    
    
//   debugf("<--- %s not found\r\n",__FUNCTION__);
  /* If no names matched, then return NULL. */
  return ((char *)NULL);
}


/* Attempt to complete on the contents of TEXT.  START and END bound the
   region of rl_line_buffer that contains the word to complete.  TEXT is
   the word to complete.  We can use the entire contents of rl_line_buffer
   in case we want to do some simple parsing.  Return the array of matches,
   or NULL if there aren't any. */
char **
fileman_completion (text, start, end)
     const char *text;
     int start, end;
{
  char **matches;
//int i;
//   debugf("---> %s\r\n",__FUNCTION__);
//   debugf("test %s\r\n",text); //  tab 누르면 기존에 입력받을 것을 text 로 전달한다. 없을 수도 있다.

 //  debugf("start %d\r\n",start);
 //  debugf("end %d\r\n",end); // 문자수인데ㅣ.


  matches = (char **)NULL;

  /* If this word is at the start of the line, then it is a command
     to complete.  Otherwise it is the name of a file in the current
     directory. */
  if (start == 0)  //rt_com...에서 dupstr로 malloc 해서 char *를 return 
    matches = rl_completion_matches (text, command_generator);

#if 0 // just for fun
    i=0;
    if ( matches != (char **) NULL)
    {    
        
        while( matches[i] != NULL )
        {
               printf("match %s\r\n",matches[i]);
               i++;
        }
    }
#endif 
//   debugf("<--- %s\r\n",__FUNCTION__);

  return (matches);
}


//
// Module:	InputProcessing()
//

/* Tell the GNU Readline library how to complete.  We want to try to complete
   on command names if this is the first word in the line, or on filenames
   if not. */
void initialize_readline (void)
{
  /* Allow conditional parsing of the ~/.inputrc file. */
  rl_readline_name = "FileMan";

  /* Tell the completer that we want a crack first. */
  rl_attempted_completion_function = fileman_completion;
}


//
// Module:	InputProcessing()
//
void InputProcessing(void)
{
//    int     ch;
	int		index, fIndex, tokenLen, length;
	CMD_ENTRY	*cp;

// hkkim
	 char *line ; 



    bzero8248((byte *)command, CMD_BUF_LEN);
    
    if(opr->runMode == LOCAL_MASTER)    printf("[M-%02d:%02d:%02d] ", rtc->hour, rtc->min, rtc->sec); 
    else                                printf("[S-%02d:%02d:%02d] ", rtc->hour, rtc->min, rtc->sec);            
	
	//fgets(command, CMD_BUF_LEN, stdin);
	//gets(command, CMD_BUF_LEN, consolePort.fId);
	//gets(command);
	
// hkkim
#if 0 
	while(1)
	{
	    ch = getchar();
	    command[index++] = ch;
	    if(ch == 0x0a)  break;
	}
#else 
      line = readline (": ");      
      strncpy(command,line,CMD_BUF_LEN);
      add_history (line); 
	 	  free (line);

#endif 
	
	cmdPtr = command;
	if ((tokenLen = GetToken()) <= 0)   return;

    printf("(1)cmd: %s\n", command);
    
	cp = cmdTable;
	for (index = 0, fIndex = -1; index < MAX_COMMAND; index++, cp++)
	{
		if (strlen(cp->name) == 0)
			break;
		length = strlen(cp->name) > tokenLen ? strlen(cp->name) : tokenLen;
		if (strncmp(cp->name, token, length) == 0)
		{
			fIndex = index;
			break;
		}
	}
	if (fIndex == -1)
	{
		printf("%%%%ERR_Invalid Command %s Input\n", token);
		return;
	}
	
	(*cp->function)();
}

//
// End of cliDrv.c
//
