//#include <termios.h>
#include <stdio.h>
//#include <unistd.h>
//#include <fcntl.h>
//#include <sys/time.h>
//#include <sys/signal.h>
//#include <sys/types.h>
#include <sys/time.h>
#include <curses.h>

#define READ  1
#define WRITE 2
#define _DEBUG 0

char line1_buf[300], line2_buf[300];
char *line1_devname, *line2_devname;
short int line1_cmd_len, line1_cmd_pos;
short int line2_cmd_len, line2_cmd_pos;


void oap_hex_add_to_str(char *str, char *buf, int len)
{
	int j;
	for(j=0;j<len;j++)
	{
		sprintf(str,"%s %02X", str, (unsigned char)buf[j]);
	}
}


void oap_print_msg(char *str)
{
	struct timeval tp;
	struct timezone tz;
	gettimeofday(&tp, &tz);
	long int ms = tp.tv_sec * 1000 + tp.tv_usec / 1000;

	// if we are using curses, then use printw, otherwise printf
	if(baudrate()>0)
	{
		printw("%li: %s\n", ms, str);
	}
	else
	{
		printf("%li: %s\n", ms, str);
		fflush(stdout);
	}
}

int oap_calc_checksum(char *buf, int len)
{
	int checksum=0, j;
	/* do not consider first 2 bytes and last byte for checksum*/
	for(j=2;j<len-1;j++)
	{
		checksum+=buf[j];	
	}
	checksum &= 0xff;
	checksum = 0x100 - checksum;
	return checksum;
}

void oap_print_char(int line, int rw, char byte)
{
	char str[1300];

	if(_DEBUG)
	{
		sprintf(str,"Line %d: %s: %02X", line, (rw==READ?"RCV":"WRITE"), (unsigned char)byte);
		oap_print_msg(str);
	}
}
				
int oap_receive_byte(int line, unsigned char byte)
{
	char str[1200];
	char *line_buf;
	short int *line_cmd_pos, *line_cmd_len;

	oap_print_char(line, READ, byte);
	
	if(line==1)
	{
		line_buf=line1_buf;
		line_cmd_pos=&line1_cmd_pos;
		line_cmd_len=&line1_cmd_len;

	}
	else
	{
		line_buf=line2_buf;
		line_cmd_pos=&line2_cmd_pos;
		line_cmd_len=&line2_cmd_len;

	}

	if(*line_cmd_pos == 0 && byte != 0xff)
	{
		sprintf(str, "Line %d: ERROR: first byte is not 0xFF", line);
		oap_print_msg(str);
		return -1;
	}

	if(*line_cmd_pos == 1 && byte != 0x55)
	{
		sprintf(str, "Line %d: ERROR: second byte is not 0x55", line);
		oap_print_msg(str);
		return -1;
	}

	/* at least chesum could be 0xFF so the next code should not be uncommented*/
	/*
	if(*line_cmd_pos > 0 && byte == 0xff)
	{
		sprintf(str, "Line %d: ERROR: New message arrived while still reading previous message.", line);
                oap_print_msg(str);
		*line_cmd_pos=0;
		*line_cmd_len=0;
	}
	*/

	if(*line_cmd_pos == 2)
	{
		/*
		 * additional bytes: 
                 *     2 for control bytes (0xFF 0x55)
		 *     1 for msg len
		 *     (not counted) 1 for msg mode
		 *     (not counted) 2 for command
		 *     (not counted) parameter
		 *     1 at the end for checksum
		 */
		(*line_cmd_len) = byte + 4; 

		if(_DEBUG)
		{
			sprintf(str, "Line %d: msg LEN: %d TOTAL LEN: %d", line, byte, (*line_cmd_len));
        	        oap_print_msg(str);
		}

	}
	
	if(*line_cmd_len > 252 + 4)
	{
		sprintf(str, "Line %d: ERROR: message len cannot exceed 252", line);
                oap_print_msg(str);
                return -1;
	}

	line_buf[*line_cmd_pos]=byte;

	(*line_cmd_pos)++;

	if(*line_cmd_pos == (*line_cmd_len))
	{
		int checksum;
		sprintf(str, "Line %d: MSG  IN: ", line);
		oap_hex_add_to_str(str, line_buf, *line_cmd_len);
		oap_print_msg(str);

		checksum=oap_calc_checksum(line_buf, *line_cmd_len);
		if((unsigned char)line_buf[(*line_cmd_len)-1] != (unsigned char)checksum)
		{
			sprintf(str, "Line %d: ERROR: checksum error. Received: %02X  Should be: %02X", line, (unsigned char)line_buf[*line_cmd_len-1], (unsigned char)checksum);
	                oap_print_msg(str);
		}
		else
		{
			//sprintf(str, "Line %d: ERROR: checksum OK. Received: %d  Should be: %d", line, line_buf[*line_cmd_len-1], checksum);
			if(_DEBUG)
			{
				sprintf(str, "Line %d: Checksum OK", line);
		                oap_print_msg(str);
			}
		}

		(*line_cmd_pos) = 0;
		(*line_cmd_len) = 0;
	}

	return 0;
}



