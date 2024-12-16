/*****************************************************************************
#                                                                            #
#    uStreamer - Lightweight and fast MJPEG-HTTP streamer.                   #
#                                                                            #
#    Copyright (C) 2018-2023  Maxim Devaev <mdevaev@gmail.com>               #
#                                                                            #
#    This program is free software: you can redistribute it and/or modify    #
#    it under the terms of the GNU General Public License as published by    #
#    the Free Software Foundation, either version 3 of the License, or       #
#    (at your option) any later version.                                     #
#                                                                            #
#    This program is distributed in the hope that it will be useful,         #
#    but WITHOUT ANY WARRANTY; without even the implied warranty of          #
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           #
#    GNU General Public License for more details.                            #
#                                                                            #
#    You should have received a copy of the GNU General Public License       #
#    along with this program.  If not, see <https://www.gnu.org/licenses/>.  #
#                                                                            #
*****************************************************************************/


#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdatomic.h>
#include <string.h>
#include <inttypes.h>
#include <unistd.h>
#include <fcntl.h>
#include <assert.h>

#include <arpa/inet.h> 
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <netinet/tcp.h>
#include <netinet/in.h>
#include <netinet/ip.h>

#include "../../libs/tools.h"
#include "../../libs/threading.h"
#include "../../libs/logging.h"
#include "../../libs/process.h"
#include "../../libs/frame.h"
#include "../../libs/base64.h"
#include "../../libs/list.h"

#include "../stream.h"

typedef struct {
    int                 sockfd;
    long double			last_checked_fps;
    unsigned int        fps_sended;
	us_stream_s			*stream;
    atomic_bool         connected;
    atomic_bool         stop;
} us_reversetcp_runtime_s;

typedef struct us_reversetcp_sx {
	char		 *host;
	unsigned	 port;
    unsigned int retry_sec;

    us_reversetcp_runtime_s *run;
} us_reversetcp_s;

us_reversetcp_s *us_reversetcp_init(us_stream_s *stream);
void us_reversetcp_destroy(us_reversetcp_s *tcp);

bool us_reversetcp_connect(us_reversetcp_s *tcp);
void us_reversetcp_loop(us_reversetcp_s *tcp);
void us_reversetcp_loop_break(us_reversetcp_s *tcp);
