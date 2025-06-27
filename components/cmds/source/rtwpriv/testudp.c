
static char ibmcopyr[] =
   "UPDC     - Licensed Materials - Property of IBM. "
   "This module is \"Restricted Materials of IBM\" "
   "5647-A01 (C) Copyright IBM Corp. 1992, 1996. "
   "See IBM Copyright Instructions.";
#include <stdio.h>
#include <stdlib.h>
#include <linux/sockios.h>	/* SIOCDEVPRIVATE */
#include <errno.h>
#include <string.h>		/* for memcpy() et al */
#include <unistd.h>		/* for close() */
#include <sys/socket.h>		/* for "struct sockaddr" et al  */
#include <sys/ioctl.h>		/* for ioctl() */
#include <linux/wireless.h>	/* for "struct iwreq" et al */
#include <math.h>
#include <assert.h>
#include <netinet/in.h>
#include <arpa/inet.h>


int main(argc, argv)
int argc;
char **argv;
{


   int s;
   unsigned short port;
   struct sockaddr_in server;
   char buf[128];

   /* argv[1] is internet address of server argv[2] is port of server.
    * Convert the port from ascii to integer and then from host byte
    * order to network byte order.
    */
   if(argc != 4)
   {
      printf("Usage: %s <host address> <port> \n",argv[0]);
      exit(1);
   }
   port = htons(atoi(argv[2]));


   /* Create a datagram socket in the internet domain and use the
    * default protocol (UDP).
    */
   if ((s = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
   {
       printf("socket()");
       exit(1);
   }

   /* Set up the server name */
   server.sin_family      = AF_INET;            /* Internet Domain    */
   server.sin_port        = port;               /* Server Port        */
   server.sin_addr.s_addr = inet_addr(argv[1]); /* Server's Address   */

   //strcpy(buf, "Hello");
     printf("socket() to argv[3]");
   strcpy(buf, argv[3]);

   /* Send the message in buf to the server */
   if (sendto(s, buf, (strlen(buf)+1), 0,
                 (struct sockaddr *)&server, sizeof(server)) < 0)
   {
      printf("sendto()");
       exit(2);
   }

   /* Deallocate the socket */
   close(s);
}
