#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <curses.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>

//#define BAUDRATE B115200
#define BAUDRATE B57600
#define READ  1
#define WRITE 2

// DEBUG:
// 	0 - turned off
// 	1 - info
// 	2 - debug
// 	3 - more debug
//#define _DEBUG 0 // will be defined in Makefile

char line1_buf[65026], line2_buf[65026],ext_image_buf[5];
char *line1_devname, *line2_devname;
int line1_cmd_len, line1_cmd_pos=0;
int line2_cmd_len, line2_cmd_pos=0;

int line1_is_extended_image=0, line2_is_extended_image=0;
int line1_ext_pos=2, line2_ext_pos=2;

int fd_log=0;
int use_ncurses=0;
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
	use_ncurses=1;
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
	char str2[3];
	//printw("LEN:(%d)",len);
	for(j=0;j<len;j++)
	{
		sprintf(str2,"%02X",(unsigned char)buf[j]);
		str[strlen(str)+1]=0;
		str[strlen(str)]=str2[0];
		str[strlen(str)+1]=0;
		str[strlen(str)]=str2[1];
		str[strlen(str)+1]=0;
		str[strlen(str)]=str2[2];
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
	if(use_ncurses && baudrate()>0)
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
			if(use_ncurses && baudrate()>0)
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

int oap_print_podmsg(int line, unsigned char *msg, int len, int is_ext)
{
	int j, pos_shift=0;
	char tmp1[5000]={0}, tmp2[5000]={0};
	struct timeval tp;
	struct timezone tz;
	gettimeofday(&tp, &tz);
	long int ms = tp.tv_sec * 1000 + tp.tv_usec / 1000;

	for(j=0;j<5000;j++)
	{
		tmp1[j]=0;
		tmp2[j]=0;
	}

	 
	// length is received as parameter
	//len=msg[2];
	
	// if we are dealing with extended message then we need to count 2 additional bytes for length
	if(is_ext) pos_shift+=2;

	sprintf(tmp1, "%li  ", ms);
	sprintf(tmp2, "%s|  L%d  ", tmp1, line);
	if(len>0) sprintf(tmp1, "%s|  %02X  ", tmp2, msg[3+pos_shift]); else sprintf(tmp1, "%s|      ", tmp2);
	if(len>1) sprintf(tmp2, "%s|  %02X ", tmp1, msg[4+pos_shift]); else sprintf(tmp2, "%s|      ", tmp1);
	if(len>2) sprintf(tmp1, "%s%02X  ", tmp2, msg[5+pos_shift]); else sprintf(tmp1, "%s    ", tmp2);
	sprintf(tmp2, "%s|  ", tmp1);
	// 3 bytes are for mode and command length
	for(j=6;j<len+6-3;j++)
	{
		memcpy(tmp1,tmp2,5000);
		sprintf(tmp2, "%s%02X ", tmp1, msg[j+pos_shift]);
	}
	sprintf(tmp1, "%s ", tmp2);

	// printing msg in ASCII
	if(len>3)
	{
		tmp1[strlen(tmp1)+1]=0;
		tmp1[strlen(tmp1)]='[';
		for(j=6;j<len+6-3;j++) if(msg[j+pos_shift]!='\n' && msg[j+pos_shift]!='\r' && msg[j+pos_shift]!=0)
		{
			tmp1[strlen(tmp1)+1]=0;
			tmp1[strlen(tmp1)]=msg[j+pos_shift];
		} 
		tmp1[strlen(tmp1)+1]=0;
		tmp1[strlen(tmp1)]=']';
	}

	if(use_ncurses && baudrate()>0)
	{
	
		printw("%s\n", tmp1);
	}
	else
	{
		printf("%s\n", tmp1);
	}
	
	// writing to log
	if(logfile_name && fd_log)
	{
		sprintf(tmp2, "%s\n", tmp1);
		if(write(fd_log, tmp2, strlen(tmp2))<0)
		{
			if(use_ncurses && baudrate()>0)
			{
				int x,y;
				getyx(cw, y, x);
				if(x!=0) printw("\n");
				printw("ERROR: cannot write to log file\n");
				x=y;		
			}
			else
			{
				printf("ERROR: cannot write to log file\n");
				fflush(stdout);
			}

		}
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

	if(_DEBUG>1)
	{
		sprintf(tmp,"Line %d: %s: %02X", line, (rw==READ?"RCV":"WRITE"), (unsigned char)byte);
		oap_print_msg(tmp);
	}

	sprintf(tmp, "DEBUG LINE %d: %02X\n\r", line, (unsigned char)byte);		
//	write(fd_log, tmp, strlen(tmp));

}
				

/*
 * Return values:
 *     0 - byte received but msg in progress
 *    >0 - full message received
 *    -1 - error occured
 */
int oap_receive_byte(int line, unsigned char byte)
{
	char str[1200];
	char *line_buf;
	int *line_cmd_pos, *line_cmd_len, *line_ext_pos, *is_extended_image;

	if(_DEBUG)
		oap_print_char(line, READ, byte);
	
	if(line==1)
	{
		line_buf=line1_buf;
		line_cmd_pos=&line1_cmd_pos;
		line_cmd_len=&line1_cmd_len;
		line_ext_pos=&line1_ext_pos;
		is_extended_image=&line1_is_extended_image;
	}
	else
	{
		line_buf=line2_buf;
		line_cmd_pos=&line2_cmd_pos;
		line_cmd_len=&line2_cmd_len;
		line_ext_pos=&line2_ext_pos;
		is_extended_image=&line2_is_extended_image;
	}

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

		if(_DEBUG>2)
		{
			sprintf(str, "Line %d: msg LEN: %d TOTAL LEN: %d", line, byte, (*line_cmd_len));
        	        oap_print_msg(str);
		}

		if(byte == 0x00)
		{
			// this is potential extended image candidate 
			// extended image message has 2 or 3 bytes allocated to message length instead of normal 1 byte
			// byte 2 is treated as indicator of potential ext image msg
			// length bytes are assumed to be 3 and 4
			*is_extended_image=1;
			(*line_ext_pos) = 2;
		}

	}


	if(*is_extended_image)
	{
		if(_DEBUG>2)
		{
			char t[100];
			sprintf(t,"EXT MSG check - pos:%d, byte: 0x%02X",(*line_ext_pos),byte);
			oap_print_msg(t);
		}

		if (*line_ext_pos == 3 || *line_ext_pos==4)
		{
			// remember bytes 3 and 4 in case it turn out to be ext image
			ext_image_buf[*line_ext_pos]=byte;
		}
		// if byte 5 is not 0x04 then it is not ext image
		else if( *line_ext_pos == 5 && byte != 0x04 ) *is_extended_image=0;
		// if byte 6 is not 0x00 then it is not ext image
		else if( *line_ext_pos == 6 && byte != 0x00 ) *is_extended_image=0;
		// if byte 7 is not 0x32 then it is not ext image
		else if( *line_ext_pos == 7 && byte != 0x32 ) *is_extended_image=0;
		// extended image message detected
		else if( *line_ext_pos == 8)
		{
			// assumptions: length is in bytes 3 and 4 which gives maximum length of 65025
			(*line_cmd_len) = (int)((((unsigned int)(ext_image_buf[3]))<<8) | ((unsigned char)ext_image_buf[4])) + 6;
			(*line_cmd_pos) = 8;
			// now putting the bytes in their "correct" place
			line_buf[0] = 0xff;
			line_buf[1] = 0x55;
			//line_buf[2] = ext_image_buf[3] + ext_image_buf[4]; // required only for checksum calculation
			line_buf[2] = 0x00;
			line_buf[3] = ext_image_buf[3];
			line_buf[4] = ext_image_buf[4];
			line_buf[5] = 0x04;
			line_buf[6] = 0x00;
			line_buf[7] = 0x32;

			if(_DEBUG>1)
			{
				sprintf(str, "Line %d: Extended image message detected!!!", line);
				oap_print_msg(str);
			}
			// from now and on message will be treated normally
		}
	
		(*line_ext_pos)++;
	}
	


	if(*line_cmd_pos == 0)
	{
		if(byte != 0xff)
		{
			sprintf(str, "Line %d: ERROR: first byte is not 0xFF. Received 0x%02X ", line, byte);
			oap_print_msg(str);
			return -1;
		}
	}

	if(*line_cmd_pos == 1 && byte != 0x55 )
	{
		sprintf(str, "Line %d: ERROR: second byte is not 0x55. Received 0x%02X", line, byte);
		sprintf(str, " %02X", byte);
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

	
	if(*line_cmd_len > 259 && !(*is_extended_image))
	{
		sprintf(str, "Line %d: ERROR: message len cannot exceed 259 bytes", line);
                oap_print_msg(str);
                return -1;
	}

	if(*line_cmd_len > 65025+6 && (*is_extended_image))
	{
		sprintf(str, "Line %d: ERROR: message len cannot exceed 65031 bytes", line);
                oap_print_msg(str);
                return -1;
	}

	if(_DEBUG>2)
	{
		sprintf(str,"Line %d: pos %d, ext_pos %d, len %d",line, (*line_cmd_pos), (*line_ext_pos),(*line_cmd_len));
		oap_print_msg(str);
	}


	line_buf[*line_cmd_pos]=byte;

	(*line_cmd_pos)++;

	if(*line_cmd_pos == (*line_cmd_len))
	{
		int checksum, msg_len;
		if(_DEBUG)
		{
			sprintf(str, "Line %d: RAW MSG  IN: ", line);
			oap_hex_add_to_str(str, line_buf, *line_cmd_len);
			oap_print_msg(str);
		
			oap_print_podmsg(line, (unsigned char *)line_buf, (*line_cmd_len)-4-((*is_extended_image)?2:0), *is_extended_image);
		}


		checksum=oap_calc_checksum(line_buf, *line_cmd_len);
		if((unsigned char)line_buf[(*line_cmd_len)-1] != (unsigned char)checksum)
		{
			if(_DEBUG>1)
			{
				sprintf(str, "Line %d: ERROR: checksum error. Received: %02X  Should be: %02X", line, (unsigned char)line_buf[*line_cmd_len-1], (unsigned char)checksum);
	                	oap_print_msg(str);
			}
		}
		else
		{
			if(_DEBUG>1)
			{
				//sprintf(str, "Line %d: ERROR: checksum OK. Received: %d  Should be: %d", line, line_buf[*line_cmd_len-1], checksum);
				sprintf(str, "Line %d: Checksum OK", line);
		                oap_print_msg(str);
			}
		}

		msg_len=(*line_cmd_len);
		(*line_cmd_pos) = 0;
		(*line_cmd_len) = 0;

		if((*line_ext_pos)>7) *is_extended_image=0;

		return msg_len;
	}

	return 0;
}



