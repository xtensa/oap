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
	int fd, res, retval, j, chksum;
	char buf_r[255], buf_w[259], buf_r_hex[510];
	char buf[259]; // max total message length could be 259
	char str[3000];
	int buf_len, read_len, readline_done;
	fd_set fds;
	struct timeval timeout;
	timeout.tv_sec=0;
	timeout.tv_usec=50;

	if(argc!=2)
	{
		printf("Usage: %s <dev1>\n",argv[0]);
		return -2;
	}

	line1_devname=argv[1];
	
	fd=oap_open_sterm(line1_devname);

	// init curses screen
	oap_initscr();

	// replace CTRL-C handler
	signal(SIGINT, INThandler);

	printw("Start\n");
	buf_len=0;
	readline_done=0;
	while(1)
	{

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
			oap_print_podmsg(1, (unsigned char *)buf_w);

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
			res = read(fd,buf,259);
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

	oap_close_sterm(fd);
	oap_endwin();
	return 0;
}


