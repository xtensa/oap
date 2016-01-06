#include "../lib/oap.h"
#include <sys/signal.h>

#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1

volatile int STOP=FALSE; 

void signal_handler_IO (int status); /* definition of signal handler */
int exit_flag=FALSE;   /* TRUE while no signal received */



//char* c_test[2];
//char c_result[2][300];


// c_test array contains data for tests. Odd lines are the commands to send and 
// even lines are the results. First number in each line is command length

#define test_count 39


char c_test[test_count*2][300]={

	// 1
	{3, 	0x00, 0x01, 0x04},
	{0},

	// 2
	{3,	0x04, 0x00, 0x12},
	{5, 	0x04, 0x00, 0x13, 0x01, 0x0E },

	// 3
	{2,	0x00, 0x09},
	{5, 	0x00, 0x0A, 0x01, 0x00, 0x04},

	// 4
	{3, 	0x04, 0x00, 0x14},
	{18, 	0x04, 0x00, 0x15, 0x41, 0x6E, 0x64, 0x72, 0x6F, 0x69, 0x64, 0x20, 0x50, 0x6F, 0x64, 0x45, 0x6D, 0x75, 0x00 },

	// 5
	{3, 	0x04, 0x00, 0x04}, // set current chapter
	{6,	0x04, 0x00, 0x01, 0x04, 0x00, 0x04}, // cmd NOK because parameter is missing

	// 6
	{3, 	0x04, 0x00, 0x16}, // selection reset
	{6,	0x04, 0x00, 0x01, 0x00, 0x00, 0x16}, // cmd ok

	// 7
	{8, 	0x04, 0x00, 0x17, 0x04, 0x00, 0x00, 0x00, 0x00}, // switch to genre
	{6,	0x04, 0x00, 0x01, 0x02, 0x00, 0x17}, // cmd NOK because DB is not initialized

	//8
	{8, 	0x04, 0x00, 0x17, 0x02, 0x00, 0x00, 0x00, 0x00}, // switch to artist
	{6,	0x04, 0x00, 0x01, 0x02, 0x00, 0x17}, // cmd NOK because DB is not initialized

	// 9
	{8, 	0x04, 0x00, 0x17, 0x03, 0x00, 0x00, 0x00, 0x00}, // switch to album
	{6,	0x04, 0x00, 0x01, 0x02, 0x00, 0x17}, // cmd NOK because DB is not initialized

	// 10
	{8, 	0x04, 0x00, 0x17, 0x05, 0x00, 0x00, 0x00, 0x00}, // switch to song
	{6,	0x04, 0x00, 0x01, 0x02, 0x00, 0x17}, // cmd NOK because DB is not initialized

	// 11
	{4, 	0x04, 0x00, 0x18, 0x05}, // get count of the given type (tracks)
	{6,	0x04, 0x00, 0x19, 0x00, 0x00, 0x00, 0x03}, // last byte is not checked, because could vary

	// 12
	{12, 	0x04, 0x00, 0x1A, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
	{3,	0x04, 0x00, 0x1B}, // we just check that proper command is returned 

	// 13
	{3, 	0x04, 0x00, 0x1C}, // get status info
	{3, 	0x04, 0x00, 0x1D}, // check for proper command only

	// 14
	{3, 	0x04, 0x00, 0x1E}, // get current playlist position
	{3, 	0x04, 0x00, 0x1F}, // check for proper command only

	// 15
	{7, 	0x04, 0x00, 0x20, 0x00, 0x00, 0x00, 0x01}, // get title of the song number
	{3, 	0x04, 0x00, 0x21}, // check for proper command only

	// 16
	{7, 	0x04, 0x00, 0x22, 0x00, 0x00, 0x00, 0x01}, // get artist
	{3, 	0x04, 0x00, 0x23}, // check for proper command only

	// 17
	{7, 	0x04, 0x00, 0x24, 0x00, 0x00, 0x00, 0x01}, //get album of a song number
	{3, 	0x04, 0x00, 0x25}, // check for proper command only

	// 18
	{7, 	0x04, 0x00, 0x28, 0x00, 0x00, 0x00, 0x01}, // jump to song 01
	{6, 	0x04, 0x00, 0x01, 0x00, 0x00, 0x28}, // cmd ok

	// 19
	{4, 	0x04, 0x00, 0x29, 0x03}, // engine playback NEXT
	{6, 	0x04, 0x00, 0x01, 0x00, 0x00, 0x29}, // cmd ok

	// 20 
	{4, 	0x04, 0x00, 0x2E, 0x02}, // set shuffle mode
	{6, 	0x04, 0x00, 0x01, 0x00, 0x00, 0x2E}, // cmd ok

	// 21
	{3, 	0x04, 0x00, 0x2C}, // get shuffle mode
	{4, 	0x04, 0x00, 0x2D, 0x02 }, 

	// 22
	{4, 	0x04, 0x00, 0x31, 0x02}, // set repeat mode
	{6, 	0x04, 0x00, 0x01, 0x00, 0x00, 0x31}, // cmd ok

	// 23
	{3, 	0x04, 0x00, 0x2F}, // get repeat mode
	{4, 	0x04, 0x00, 0x30, 0x02}, 

	// 24
	{3,	0x04, 0x00, 0x16}, //selection reset
	{6, 	0x04, 0x00, 0x01, 0x00, 0x00, 0x16}, // cmd ok

	// 25
	{8, 	0x04, 0x00, 0x17, 0x01, 0x00, 0x00, 0x00, 0x00 }, // select playlist 0
	{6,	0x04, 0x00, 0x01, 0x02, 0x00, 0x17}, // expected error	

	// 26
	{9, 	0x04, 0x00, 0x38, 0x01, 0x00, 0x00, 0x00, 0x00, 0x01 }, // select playlist 0 and sort by artist
	{6,	0x04, 0x00, 0x01, 0x02, 0x00, 0x38}, // expected error	

	// 27
	{4, 	0x04, 0x00, 0x18, 0x04 }, // get count of genres
	{7,     0x04, 0x00, 0x19, 0x00, 0x00, 0x00, 0x01}, // return 1 record

	// 28
	{8, 	0x04, 0x00, 0x17, 0x04, 0x00, 0x00, 0x00, 0x00 }, // select genre 0
	{6,     0x04, 0x00, 0x01, 0x00, 0x00, 0x17}, // cmd ok
	

	// 29
	{4, 	0x04, 0x00, 0x18, 0x02 }, // get count of artists
	{7,     0x04, 0x00, 0x19, 0x00, 0x00, 0x00, 0x01}, // return 1 record

	// 30
	{8, 	0x04, 0x00, 0x17, 0x05, 0x00, 0x00, 0x00, 0x00 }, // select artist 0
	{6,     0x04, 0x00, 0x01, 0x00, 0x00, 0x17}, // cmd ok
	

	// 31
	{4, 	0x04, 0x00, 0x18, 0x03 }, // get count of albums
	{7,     0x04, 0x00, 0x19, 0x00, 0x00, 0x00, 0x01}, // return 1 record

	// 32
	{8, 	0x04, 0x00, 0x17, 0x03, 0x00, 0x00, 0x00, 0x02 }, // select album 2
	{6,     0x04, 0x00, 0x01, 0x04, 0x00, 0x17}, // cmd out of range

	// 33
	{8, 	0x04, 0x00, 0x17, 0x03, 0x00, 0x00, 0x00, 0x00 }, // select album 0
	{6,     0x04, 0x00, 0x01, 0x00, 0x00, 0x17}, // cmd ok
	

	// 34
	{4, 	0x04, 0x00, 0x18, 0x05 }, // get count of tracks
	{6,     0x04, 0x00, 0x19, 0x00, 0x00, 0x00, 0x0A}, // return some records (last byte is not checked

	// 35
	{8, 	0x04, 0x00, 0x17, 0x05, 0x00, 0x00, 0x00, 0x05 }, // select track 5
	{6,     0x04, 0x00, 0x01, 0x00, 0x00, 0x17}, // cmd ok

	// 36
	{3, 	0x04, 0x00, 0x16}, // reset db
	{6,     0x04, 0x00, 0x01, 0x00, 0x00, 0x16}, // cmd ok

	// 37
	{4, 	0x04, 0x00, 0x18, 0x02},
	{6,     0x04, 0x00, 0x19, 0x00, 0x00, 0x00}, // return some records (last byte not checked)

	// 38
	{9, 	0x04, 0x00, 0x38, 0x02, 0x00, 0x00, 0x00, 0x00, 0xFF},
	{6,     0x04, 0x00, 0x01, 0x00, 0x00, 0x38}, // cmd ok

	// 39
	{4, 	0x04, 0x00, 0x18, 0x03},
	{6,     0x04, 0x00, 0x19, 0x00, 0x00, 0x00} // return some records (last byte not checked)

};


/********************************************************************************/

void  INThandler(int sig)
{
	exit_flag=TRUE;
}

int main(int argc, char **argv)
{
	int fd, res, retval, j, chksum;
	char buf_w[261];
	// char initcmd[]={0xFF, 0x55, 0x03, 0x00, 0x01, 0x04, 0xF8};
	char buf[259]; // max total message length could be 259
	char str[3000];
	char logfile_name[]="oap_console.log";
	int read_len, ext_mode=0, test_num, test_sent, test_received, msg_len, failed_tests_count=0;
	int ctimeout=1000;
	long int curr_ms, sent_ms;
	struct timeval tp;
        struct timezone tz;
	fd_set fds;
	struct timeval timeout;
	timeout.tv_sec=0;
	timeout.tv_usec=50;
/*
	if(argc!=2)
	{
		printf("Usage: %s <dev1>\n",argv[0]);
		return -2;
	}
*/


	fd_log=open(logfile_name, O_CREAT | O_RDWR | O_APPEND, S_IRWXU);
	if(fd_log<0)
	{
		printf("ERROR: cannot open log file %s\n", logfile_name);
	}


	line1_devname=argv[1];
	
	fd=oap_open_sterm(line1_devname);

	// init curses screen
	// oap_initscr();

	// replace CTRL-C handler
	signal(SIGINT, INThandler);

/*	retval=write(fd, initcmd, 7);
	if(retval<0)
	{
		oap_print_msg((char*)"ERROR: cannot write to serial terminal");
	}
*/

	test_num=0;
	test_sent=0;
	test_received=0;
	ext_mode=0;
	gettimeofday(&tp, &tz);
	sent_ms = tp.tv_sec * 1000 + tp.tv_usec / 1000;

	while(1)
	{
	        gettimeofday(&tp, &tz);
		curr_ms = tp.tv_sec * 1000 + tp.tv_usec / 1000;

		if(curr_ms - sent_ms >= ctimeout) 
		{
			printf("ERR: timeout\n - ERR\n\n");
			test_received=0;
			test_sent=0;
			test_num++;
			sent_ms=curr_ms;
			if(test_num==test_count) break;
		}


	
		if(test_sent)
		{
			// testing received result
			if(test_received)
			{
				int result=0, j=0, pos_shift;
				if(ext_mode) pos_shift=5; else pos_shift=3;

				// ignore status updates
				if(line1_buf[4] == 0x00 && line1_buf[5] == 0x27)
				{
					test_received=0;
				}
				else
				{	

					printf("RCV: ");
					oap_print_podmsg(1, (unsigned char*)line1_buf, msg_len-4, ext_mode);

					// result is stored in line1_buf
					for(j=0;j<c_test[test_num*2+1][0];j++)
					{
						if(c_test[test_num*2+1][j+1] != line1_buf[pos_shift+j])
							result=-1;
					}
					
					if(!result)
						printf(" - OK\n");
					else
					{
						printf(" - ERR\n");
						failed_tests_count++;
					}
					if(test_num+1==test_count) exit_flag=1;

					printf("\n");
					test_num++;
					test_received=0;
					test_sent=0;
				}
			}

		}
		else
		{
			printf("TEST %3d/%d\n", test_num+1, test_count);

			str[0]=0;
			oap_hex_add_to_str(str, (char*)((c_test[test_num*2])+1), c_test[test_num*2][0]);
			printf("CMD: %s\n", str);

			str[0]=0xff;
			str[1]=0x55;
			str[2]=c_test[test_num*2+1][0];
			for(j=0;j<c_test[test_num*2+1][0];j++) str[j+3]=c_test[test_num*2+1][j+1];
			printf("EXP: ");
			oap_print_podmsg(1, (unsigned char*)str, c_test[test_num*2+1][0], ext_mode);

			// if nothing is expected then assume message received
			if(!c_test[test_num*2+1][0]) test_received=1;

			read_len=c_test[test_num*2][0];

			{ // TODO: move to separate function
				int pos_shift=3;
				// setting control bytes
				buf_w[0]=0xff;
				buf_w[1]=0x55;

				// initializing length of the message
				
				if(ext_mode)
				{
					buf_w[2]=0x00;
					buf_w[3]=(read_len/0xff);
					buf_w[4]=read_len;
					pos_shift=5;
					chksum=buf_w[3]+buf_w[4];
				}
				else
				{
					buf_w[2]=read_len;
					pos_shift=3;
					chksum=read_len;
				}
			
				for(j=0;j<read_len;j++)
				{
					buf_w[j+pos_shift]=c_test[test_num*2][j+1];
					chksum+=c_test[test_num*2][j+1];
				}
				chksum&=0xff;
				chksum=0x100-chksum;
				buf_w[j+pos_shift]=chksum;
				
				retval=write(fd, buf_w, j+1+pos_shift);
			        gettimeofday(&tp, &tz);
				sent_ms = tp.tv_sec * 1000 + tp.tv_usec / 1000;


				if(retval<0)
				{
					oap_print_msg((char*)"ERROR: cannot write to serial terminal");
				}

				if(_DEBUG>1)
				{
					printf("Status: %d bytes sent\n", retval);
				}

				ext_mode=0;
				if(retval<0)
				{
					oap_print_msg((char*)"ERROR: cannot write to serial terminal");
				}
			}

			test_sent=1;
			usleep(150);
		}

	
		FD_ZERO(&fds);
		FD_SET(fd, &fds);  
		//FD_SET(hdle2, &rd1);
		res = select(fd+1, &fds, NULL, NULL, &timeout);  
	 
		if ( FD_ISSET(fd, &fds) && !test_received) 
		{
			int j;
			res = read(fd,buf,259);
			buf[res]=0;
			for(j=0;j<res;j++)
			{
				if((msg_len=oap_receive_byte(1, buf[j]))>0) 
				{
					test_received=1;
					break;
				}
			}

		}
		else
		{
			usleep(300);
		}

		if(exit_flag) break;
	}

	oap_close_sterm(fd);
	close(fd_log);

	if(!failed_tests_count)
		printf("OK: All tests passed!\n");
	else
		printf("ERR: %d tests out of %d failed.\n", failed_tests_count, test_count);
	return 0;
}


