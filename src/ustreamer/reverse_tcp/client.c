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


#include "client.h"

#include "../../libs/types.h"
#include "../../libs/tools.h"
#include "../../libs/threading.h"
#include "../../libs/logging.h"
#include "../../libs/process.h"
#include "../../libs/frame.h"
#include "../../libs/ring.h"

#define _RUN(x_next)	tcp->run->x_next

us_reversetcp_s *us_reversetcp_init(us_stream_s *stream) {
	us_reversetcp_runtime_s *run;
	US_CALLOC(run, 1);
	run->stream = stream;
    atomic_init(&run->stop, false);
    atomic_init(&run->connected, false);
    run->sockfd = -1;
    run->last_checked_fps = us_get_now_monotonic();
    run->fps_sended = 0;

	us_reversetcp_s *tcp;
	US_CALLOC(tcp, 1);
	tcp->host = "127.0.0.1";
	tcp->port = 1337;
    tcp->retry_sec = 10;
	tcp->run = run;

	return tcp;
}

void us_reversetcp_destroy(us_reversetcp_s *tcp) {
    US_LOG_INFO("Destroying tcp ...")
    close(_RUN(sockfd));
	free(tcp->run);
	free(tcp);
}

bool us_reversetcp_connect(us_reversetcp_s *tcp) {
    if (!(_RUN(sockfd) < 0)) {
        close(_RUN(sockfd));
    }
    
    _RUN(sockfd) = socket(AF_INET, SOCK_STREAM, 0);
    
    US_LOG_INFO("Connecting on TCP Server %s:%u", tcp->host, tcp->port);
    struct sockaddr_in client_addr; 
    bzero(&client_addr, sizeof(client_addr)); 

    client_addr.sin_addr.s_addr = inet_addr(tcp->host); 
    client_addr.sin_port = htons(tcp->port); 
    client_addr.sin_family = AF_INET;

    int err = connect(_RUN(sockfd), (struct sockaddr *)&client_addr, sizeof(client_addr));
    if (err < 0) 
    { 
        US_LOG_ERROR("Unable to connect to TCP Server (%i).", errno)
        return false;
    }

    atomic_store(&_RUN(connected), true);
    return true;
}

void us_reversetcp_loop(us_reversetcp_s *tcp) {
    US_LOG_DEBUG("Starting TCP Loop.")
    while (!atomic_load(&_RUN(stop))) {
        if (!atomic_load(&_RUN(connected))) {
            if (!us_reversetcp_connect(tcp)) {
                US_LOG_INFO("Retrying in %u second(s).", tcp->retry_sec)
                usleep(1000000 * tcp->retry_sec);
            }
            continue;
        }

    	const long double now = us_get_now_monotonic();

        const int ri = us_ring_consumer_acquire(ring, 0);
        if (ri >= 0) {
            const us_frame_s *const frame = ring->items[ri];

            if (send(_RUN(sockfd), (const void *)&frame->used, sizeof(size_t), 0) < 0) {
                goto socketerror;
            }

            if (send(_RUN(sockfd), (const void *)frame->data, frame->used, 0) < 0) {
                socketerror:
                US_LOG_ERROR("Unable to send message, server disconnected (%i).", errno)
                US_LOG_ERROR("Retrying in %u second(s).", tcp->retry_sec)

                us_ring_consumer_release(ring, ri);
                atomic_store(&_RUN(connected), false);
                usleep(1000000 * tcp->retry_sec);
                continue;
            } else {
                US_LOG_DEBUG("TCP Packet send (Typical size: %lu bytes).", frame->used)
                _RUN(fps_sended) = _RUN(fps_sended) + 1;
            }

            us_ring_consumer_release(ring, ri);
        }

        if ((now - _RUN(last_checked_fps)) > 1) {
            US_LOG_INFO("TCP Stream in progress (%u fps).", _RUN(fps_sended))
            _RUN(last_checked_fps) = now;
            _RUN(fps_sended) = 0;
        }
    }
}

void us_reversetcp_loop_break(us_reversetcp_s *tcp) {
    US_LOG_DEBUG("Break TCP Loop.")
    atomic_store(&_RUN(stop), true);
}
