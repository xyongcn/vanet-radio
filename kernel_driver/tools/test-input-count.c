/*
 * Linux IEEE 802.15.4 userspace tools
 *
 * Copyright (C) 2008, 2009 Siemens AG
 *
 * Written-by: Dmitry Eremin-Solenikov
 * Written-by: Sergey Lapin
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; version 2 of the License.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <linux/sockios.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <time.h> 
#include <stdlib.h>
#include "ieee802154.h"

int main(int argc, char **argv) {
	int ret, count, i;
	count = atoi(argv[1]);

	char *iface = argv[2] ?: "wpan0";

//	char buf[13] = {0x40, 0x00, 0x56,0x40, 0x00,0x40, 0x00,0x40, 0x00,0x40, 0x00,0x40, 0x00};
	char buf[250];
	for(i =0; i<250; i++)
		buf[i] = i;

	int sd = socket(PF_IEEE802154, SOCK_RAW, 0);
	if (sd < 0) {
		perror("socket");
		return 1;
	}

	ret = setsockopt(sd, SOL_SOCKET, SO_BINDTODEVICE, iface, strlen(iface) + 1);
	if (ret < 0)
		perror("setsockopt: BINDTODEVICE");

	time_t start,end; 
	clock_t start_c,end_c;
	i = count;
	printf("i is %d\n", i);
	start =time(NULL);
	start_c = clock();  
	while(i--) {
		ret = send(sd, buf, 127/*sizeof(buf)*/, 0);
		if (ret < 0)
			perror("send");
	}
	end =time(NULL); 
	end_c = clock(); 
	printf("time=%f\n",difftime(end,start)); 
	printf("time=%f\n",(double)(end_c-start_c)/CLOCKS_PER_SEC);  
	ret = close(sd);
	if (ret < 0)
		perror("close");

	return 0;

}
