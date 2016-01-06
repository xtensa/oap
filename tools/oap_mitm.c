#include "../lib/oap.h"
#include <sys/signal.h>

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

int main(int argc, char **argv)
{
	int fd1, fd2, res, msg_len, fd_max;
	char buf[259]; // max total message length could be 259
	fd_set fds;
	struct timeval timeout;
	timeout.tv_sec=0;
	timeout.tv_usec=50;

	if(argc!=3)
	{
		printf("Usage: %s <tty_input_dev> <tty_output_dev>\n",argv[0]);
		return -2;
	}

	line1_devname=argv[1];
	line2_devname=argv[2];
	
	fd1=oap_open_sterm(line1_devname);
	fd2=oap_open_sterm(line2_devname);

	if(fd1<0)
	{
		printf("ERROR: cannot open tty device %s\n", line1_devname);
	}

	if(fd2<0)
	{
		printf("ERROR: cannot open tty device %s\n", line2_devname);
	}


	fd_log=open(logfile_name, O_CREAT | O_RDWR | O_APPEND, S_IRWXU);
	if(fd_log<0)
	{
		printf("ERROR: cannot open log file %s\n", logfile_name);
	}

	if(fd1<0 || fd2<0 || fd_log<0) return -1;


	// init curses screen
//	oap_initscr();

	// replace CTRL-C handler
	signal(SIGINT, INThandler);

	while(1)
	{
		int read_sth=0;
/*
		int tmp;

		tmp=getch();
		if(tmp!=ERR && strchr("0123456789abcdefABCDEF\n",tmp)!=NULL)
		{
			printw("%c",tmp);

			if(tmp=='\n')
			{
				readline_done=1;
			}
			else
			{
				if(buf_len==510)
				{
					oap_print_msg((char*)"ERROR: Cannot enter more then 510 hex characters (255 bytes)!");
				}
				else
				{
					buf_r_hex[buf_len]=tmp;
					buf_len++;
				}
			}
		}

		if(readline_done)
		{
			// setting control bytes
			buf_w[0]=0xff;
			buf_w[1]=0x55;
			// initializing length of the message
			read_len=0;
			// now need to convert hex string into walues 
			for(j=0;j<buf_len;j+=2)
			{
				char tmp[5];
				tmp[0]='0';
				tmp[1]='x';
				tmp[2]=buf_r_hex[j];
				tmp[3]=buf_r_hex[j+1];
				tmp[4]=0;
				buf_r[j/2]=(char)(int)strtol(tmp,NULL, 0);
				// increase length of message to be sent 
				read_len++;
			}
			buf_w[2]=read_len;
			
			chksum=read_len;
			for(j=0;j<read_len;j++)
			{
				buf_w[j+3]=buf_r[j];
				chksum+=buf_r[j];
			}
			chksum&=0xff;
			chksum=0x100-chksum;
			buf_w[j+3]=chksum;
			
			sprintf(str, "Line %d: RAW MSG OUT: ", 1);
			oap_hex_add_to_str(str, buf_w, j+4);
			oap_print_msg(str);
			oap_print_podmsg(1, buf_w);

			retval=write(fd1, buf_w, j+4);
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
		
*/
		FD_ZERO(&fds);
		FD_SET(fd1, &fds);  
		FD_SET(fd2, &fds);
		if(fd1>fd2) fd_max=fd1; else fd_max=fd2;
		res = select(fd_max+1, &fds, NULL, NULL, &timeout);  
	 
		if ( FD_ISSET(fd1, &fds) ) 
		{
			int j;
			res = read(fd1,buf,259);
			buf[res]=0;
			for(j=0;j<res;j++)
			{
				msg_len=oap_receive_byte(1, buf[j]);
				if(msg_len>0) // full msg received
				{
					if(write(fd2, line1_buf, msg_len)<0)
					{
						if(baudrate()>0)
						{
							int x,y;
							getyx(cw, y, x);
							if(x!=0) printw("\n");
							printw("ERROR: cannot write to line 1\n");
							x=y;		
						}
						else
						{
							printf("ERROR: cannot write to line 1\n");
							fflush(stdout);
						}
					}
				}
			}
			read_sth=1;
		}

		if ( FD_ISSET(fd2, &fds) ) 
		{
			int j;
			res = read(fd2,buf,259);
			buf[res]=0;
			for(j=0;j<res;j++)
			{
				msg_len=oap_receive_byte(2, buf[j]);
				if(msg_len>0) // full msg received
				{
					if(write(fd1, line2_buf, msg_len)<0)
					{
						if(baudrate()>0)
						{
							int x,y;
							getyx(cw, y, x);
							if(x!=0) printw("\n");
							printw("ERROR: cannot write to line 2\n");
							x=y;		
						}
						else
						{
							printf("ERROR: cannot write to line 2\n");
							fflush(stdout);
						}
					}
				}
			}
			read_sth=1;
		}

		if(!read_sth)
		{
			usleep(10);
		}

		if(exit_flag) break;
	}

	oap_close_sterm(fd1);
	oap_close_sterm(fd2);
	close(fd_log);
//	oap_endwin();
	return 0;
}


