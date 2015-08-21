#include "../lib/oap.h"
#include <termios.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/signal.h>
#include <sys/types.h>
#include <string.h>

#define BAUDRATE B57600
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1

volatile int STOP=FALSE; 

void signal_handler_IO (int status); /* definition of signal handler */
int exit_flag=FALSE;   /* TRUE while no signal received */

/********************************************************************************/

void  INThandler(int sig)
{
	exit_flag=TRUE;
}

main(int argc, char **argv)
{
	int fd, c, res, retval, j, chksum;
	struct termios oldtio,newtio;
	char buf_r[255], buf_w[300], buf_r_hex[500], tmp_buf[255], buf[255];
	char str[1300]="TEST: ";
	int buf_len, readline_done;
	fd_set fds;
	struct timeval timeout;
	timeout.tv_sec=0;
	timeout.tv_usec=50;

	WINDOW *cw;

	line1_cmd_len=0;
	line1_cmd_pos=0;

	line2_cmd_len=0;
	line2_cmd_pos=0;

	if(argc!=2)
	{
		printf("Usage: %s <dev1>\n",argv[0]);
		return -2;
	}

	line1_devname=argv[1];
	

	/* open the device to be non-blocking (read will return immediatly) */
	fd = open(line1_devname, O_RDWR | O_NOCTTY | O_NONBLOCK);
	if (fd <0) 
	{
		perror(line1_devname); 
		return -1; 
	}

	cw=initscr(); 	/* init curses */
	noecho();
	cbreak();         // don't interrupt for user input
	timeout(50);     // wait 500ms for key press

    	immedok(cw, TRUE);
    	scrollok(cw, TRUE);

	// replace CTRL-C handler
	signal(SIGINT, INThandler);

	/* allow the process to receive SIGIO */
	fcntl(fd, F_SETOWN, getpid());
	/* Make the file descriptor asynchronous (the manual page says only 
	O_APPEND and O_NONBLOCK, will work with F_SETFL...) */
	//fcntl(fd, F_SETFL, FASYNC);

	tcgetattr(fd,&oldtio); /* save current port settings */
	/* set new port settings for canonical input processing */
//	newtio.c_cflag = BAUDRATE | CRTSCTS | CS8 | CLOCAL | CREAD;
	newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
//	newtio.c_iflag = IGNPAR | ICRNL;
	newtio.c_iflag = IGNPAR ;
	newtio.c_oflag = 0;
	newtio.c_lflag &= !ICANON;
	newtio.c_cc[VMIN]=1;
	newtio.c_cc[VTIME]=0;
	tcflush(fd, TCIFLUSH);
	tcsetattr(fd,TCSANOW,&newtio);


	printw("Start\n");
	buf_len=0;
	readline_done=0;
	while(1)
	{
		int tmp;

		tmp=getch();
		if(tmp!=ERR)
		{
			printw("%c",tmp);

			if(tmp=='\n')
			{
				readline_done=1;
			}
			else
			{
				buf_r_hex[buf_len]=tmp;
				buf_len++;
			}
		}

		if(readline_done)
		{
			buf_w[2]=0;
			/* now need to convert hex string into walues */
			for(j=0;j<buf_len;j+=2)
			{
				char tmp[5];
				tmp[0]='0';
				tmp[1]='x';
				tmp[2]=buf_r_hex[j];
				tmp[3]=buf_r_hex[j+1];
				tmp[4]=0;
				buf_r[j/2]=(char)(int)strtol(tmp,NULL, 0);
				/* increase length of message to be sent */
				buf_w[2]++;
			}

			// setting control bytes
			buf_w[0]=0xff;
			buf_w[1]=0x55;
			chksum=buf_w[2];
			for(j=0;j<buf_w[2];j++)
			{
				buf_w[j+3]=buf_r[j];
				chksum+=buf_r[j];
			}
			chksum&=0xff;
			chksum=0x100-chksum;
			buf_w[j+3]=chksum;
			
			sprintf(str, "Line %d: MSG OUT: ", 1);
			oap_hex_add_to_str(str, buf_w, j+4);
			oap_print_msg(str);

			retval=write(fd, buf_w, j+4);
			if(retval<0)
			{
				oap_print_msg((char*)"ERROR: cannot write to serial terminal");
			}

			if(_DEBUG)
			{
				printw("Status: %d bytes sent\n", retval);
			}

			readline_done=0;
			buf_len=0;
		}
		

		FD_ZERO(&fds);
		FD_SET(fd, &fds);  
		//FD_SET(hdle2, &rd1);
		res = select(fd+1, &fds, NULL, NULL, &timeout);  
	 
		if ( FD_ISSET(fd, &fds) ) 
		{
			int j;
			res = read(fd,buf,255);
			buf[res]=0;
			for(j=0;j<res;j++)
			{
				oap_receive_byte(1, buf[j]);
			}

		}
		else
		{
			usleep(1000);
		}

		if(exit_flag) break;
	}
	/* restore old port settings */
	tcsetattr(fd,TCSANOW,&oldtio);
	endwin();
	return 0;
}


