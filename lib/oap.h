#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <curses.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>

#define BAUDRATE B57600
#define READ  1
#define WRITE 2
#define _DEBUG 0

char line1_buf[300], line2_buf[300];
char *line1_devname, *line2_devname;
int line1_cmd_len, line1_cmd_pos;
int line2_cmd_len, line2_cmd_pos;
int fd_log=0;
WINDOW *cw;
struct termios oldtio,newtio;
char *logfile_name="mitm_out.log";

//#define POS_TIME 0
//#define POS_LINE (POS_TIME+15)
//#define POS_MODE (POS_LINE+6)
//#define POS_CMD  (POS_MODE+7)

void oap_initscr()
{
	cw=initscr();   /* init curses */
	noecho();
	cbreak();         // don't interrupt for user input
	timeout(50);     // wait 500ms for key press
	immedok(cw, TRUE);
	scrollok(cw, TRUE);
}

void oap_endwin()
{
	endwin();
}

int oap_open_sterm(char *dev_name)
{
	int fd;
	/* open the device to be non-blocking (read will return immediatly) */
	fd = open(dev_name, O_RDWR | O_NOCTTY | O_NONBLOCK);
	if (fd <0) 
	{
		perror(line1_devname); 
		return -1; 
	}

	tcgetattr(fd,&oldtio); /* save current port settings */
	/* set new port settings for canonical input processing */
	newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
	newtio.c_iflag = IGNPAR ;
	newtio.c_oflag = 0;
	newtio.c_lflag &= !ICANON;
	newtio.c_cc[VMIN]=1;
	newtio.c_cc[VTIME]=0;
	tcflush(fd, TCIFLUSH);
	tcsetattr(fd,TCSANOW,&newtio);

	return fd;
}


void oap_close_sterm(int fd)
{
	/* restore old port settings */
	tcsetattr(fd,TCSANOW,&oldtio);
}


void oap_hex_add_to_str(char *str, char *buf, int len)
{
	int j;
	//printw("LEN:(%d)",len);
	for(j=0;j<len;j++)
	{
		sprintf(str,"%s %02X", str, (unsigned char)buf[j]);
	}
}


void oap_print_msg(char *str)
{
	int y, x;
	char tmp[5000];
	struct timeval tp;
	struct timezone tz;
	gettimeofday(&tp, &tz);
	long int ms = tp.tv_sec * 1000 + tp.tv_usec / 1000;

	// if we are using curses, then use printw, otherwise printf
	if(baudrate()>0)
	{
		getyx(cw, y, x);
		if(x!=0) printw("\n");
		printw("%li: %s\n", ms, str);
		
	}
	else
	{
		printf("%li: %s\n", ms, str);
		fflush(stdout);
	}

	// writing to log
	if(logfile_name && fd_log)
	{
		sprintf(tmp, "%li: %s\n", ms, str);		
		if(write(fd_log, tmp, strlen(tmp))<0)
		{
			if(baudrate()>0)
			{
				getyx(cw, y, x);
				if(x!=0) printw("\n");
				printw("ERROR: cannot write to log file\n");
				
			}
			else
			{
				printf("ERROR: cannot write to log file\n");
				fflush(stdout);
			}

		}
	}
	// just for compiler not to throw a worning
	x=y;
}

int oap_print_podmsg(int line, unsigned char *msg)
{
	int j, len=msg[2];
	char tmp[5000];
	struct timeval tp;
	struct timezone tz;
	gettimeofday(&tp, &tz);
	long int ms = tp.tv_sec * 1000 + tp.tv_usec / 1000;

	sprintf(tmp, "%li  ", ms);
	sprintf(tmp, "%s|  L%d  ", tmp, line);
	if(len>0) sprintf(tmp, "%s|  %02X  ", tmp, msg[3]); else sprintf(tmp, "%s|      ", tmp);
	if(len>1) sprintf(tmp, "%s|  %02X ", tmp, msg[4]); else sprintf(tmp, "%s|      ", tmp);
	if(len>2) sprintf(tmp, "%s%02X  ", tmp, msg[5]); else sprintf(tmp, "%s    ", tmp);
	sprintf(tmp, "%s|  ", tmp);
	// 3 bytes are for mode and command length
	for(j=6;j<len+6-3;j++)
	{
		sprintf(tmp, "%s%02X ", tmp, msg[j]);
	}
	sprintf(tmp, "%s ", tmp);
	if(len>3)
	{
		tmp[strlen(tmp)+1]=0;
		tmp[strlen(tmp)]='[';
		for(j=6;j<len+6-3;j++)
		{
			tmp[strlen(tmp)+1]=0;
			tmp[strlen(tmp)]=msg[j];
		}
		tmp[strlen(tmp)+1]=0;
		tmp[strlen(tmp)]=']';
	}

	// if we are using curses, then use printw, otherwise printf
	if(baudrate()>0)
	{
	
		printw("%s\n", tmp);
	}
	else
	{
		printf("%s\n", tmp);
	}
	
	// writing to log
	if(logfile_name && fd_log)
	{
		sprintf(tmp, "%s\n", tmp);
		write(fd_log, tmp, strlen(tmp));
	}

	return 0;
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
	char tmp[1300]={0};

	if(_DEBUG)
	{
		sprintf(tmp,"Line %d: %s: %02X", line, (rw==READ?"RCV":"WRITE"), (unsigned char)byte);
		oap_print_msg(tmp);
	}

	sprintf(tmp, "DEBUG LINE %d: %02X\n", line, (unsigned char)byte);		
	write(fd_log, tmp, strlen(tmp));

}
				

/*
 * Return values:
 *     0 - byte received but msg in progress
 *     1 - full message received
 *    -1 - error occured
 */
int oap_receive_byte(int line, unsigned char byte)
{
	char str[1200];
	char *line_buf;
	int *line_cmd_pos, *line_cmd_len;

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
		sprintf(str, "Line %d: ERROR: first byte is not 0xFF. Received 0x%02X", line, byte);
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
	
	if(*line_cmd_len > 259)
	{
		sprintf(str, "Line %d: ERROR: message len cannot exceed 259 bytes", line);
                oap_print_msg(str);
                return -1;
	}

	line_buf[*line_cmd_pos]=byte;

	(*line_cmd_pos)++;

	if(*line_cmd_pos == (*line_cmd_len))
	{
		int checksum, msg_len;
		sprintf(str, "Line %d: RAW MSG  IN: ", line);
		oap_hex_add_to_str(str, line_buf, *line_cmd_len);
		oap_print_msg(str);

		oap_print_podmsg(line, (unsigned char *)line_buf);

		checksum=oap_calc_checksum(line_buf, *line_cmd_len);
		if((unsigned char)line_buf[(*line_cmd_len)-1] != (unsigned char)checksum)
		{
			sprintf(str, "Line %d: ERROR: checksum error. Received: %02X  Should be: %02X", line, (unsigned char)line_buf[*line_cmd_len-1], (unsigned char)checksum);
	                oap_print_msg(str);
		}
		else
		{
			if(_DEBUG)
			{
				//sprintf(str, "Line %d: ERROR: checksum OK. Received: %d  Should be: %d", line, line_buf[*line_cmd_len-1], checksum);
				sprintf(str, "Line %d: Checksum OK", line);
		                oap_print_msg(str);
			}
		}

		msg_len=(*line_cmd_len);
		(*line_cmd_pos) = 0;
		(*line_cmd_len) = 0;

		return msg_len;
	}

	return 0;
}



