/*
 * Copyright (C) 2011-2013 Michael Tuexen
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE PROJECT AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE PROJECT OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * Usage: speedtest_server [local_encaps_port] [remote_encaps_port]
 * 
 * Example
 * Server: $ ./speedtest_server 11111 22222
 * Client: $ ./speedtest_client 127.0.0.1 7 0 22222 11111
 */

#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <sys/types.h>
#ifndef _WIN32
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif
#include <usrsctp.h>
#include <sys/time.h>
#include <sys/signal.h>

#define PORT 7
#define BUFFER_SIZE 64*1024
#define SLEEP 1

#define INTERVAL 1
#define __to_mbps(b) (b/(1024*1024))

const int use_cb = 1;
struct timeval begin, end;
int cnt;
int seconds = 0;
double size;
int done = 0;

double timeval_diff(struct timeval x , struct timeval y){ //return ms
    x.tv_usec -= y.tv_usec;
    if(x.tv_usec < 0){
        x.tv_sec--;
        x.tv_usec += 1000000;
    }
    x.tv_sec -= y.tv_sec;
    return x.tv_sec*1000.0 + x.tv_usec/1000.0;
}

void print_bw(int signo){
    double bandwidth;
    double diff;

    seconds++;
    diff = timeval_diff(end, begin); //ms
    if(diff > 0 && cnt > 1){
        bandwidth = size*8 / (diff/1000);
        printf("[%d] cnt: %d, size: %lf, timediff: %lf, bandwidth: %lfMbps\n",
                seconds, cnt, size, diff, __to_mbps(bandwidth));
    }
    alarm(INTERVAL);
}

static int
receive_cb(struct socket *sock, union sctp_sockstore addr, void *data,
           size_t datalen, struct sctp_rcvinfo rcv, int flags, void *ulp_info)
{
    //char namebuf[INET6_ADDRSTRLEN];
    //const char *name;
    //uint16_t port;

    if (data) {
        if (flags & MSG_NOTIFICATION) {
            //printf("Notification of length %d received.\n", (int)datalen);
        } else {
            gettimeofday(&end, NULL);
            if(cnt == 0){
                memcpy(&begin, &end, sizeof(struct timeval));
            }
            cnt++;
            size+=datalen;
            /*
            switch (addr.sa.sa_family) {
#ifdef INET
            case AF_INET:
                name = inet_ntop(AF_INET, &addr.sin.sin_addr, namebuf, INET_ADDRSTRLEN);
                port = ntohs(addr.sin.sin_port);
                break;
#endif
#ifdef INET6
            case AF_INET6:
                name = inet_ntop(AF_INET6, &addr.sin6.sin6_addr, namebuf, INET6_ADDRSTRLEN),
                port = ntohs(addr.sin6.sin6_port);
                break;
#endif
            case AF_CONN:
#ifdef _WIN32
                _snprintf(namebuf, INET6_ADDRSTRLEN, "%p", addr.sconn.sconn_addr);
#else
                snprintf(namebuf, INET6_ADDRSTRLEN, "%p", addr.sconn.sconn_addr);
#endif
                name = namebuf;
                port = ntohs(addr.sconn.sconn_port);
                break;
            default:
                name = NULL;
                port = 0;
                break;
            }
            printf("Msg of length %d received from %s:%u on stream %u with SSN %u and TSN %u, PPID %u, context %u.\n",
                   (int)datalen,
                   name,
                   port,
                   rcv.rcv_sid,
                   rcv.rcv_ssn,
                   rcv.rcv_tsn,
                   ntohl(rcv.rcv_ppid),
                   rcv.rcv_context);
            if (flags & MSG_EOR) {
                struct sctp_sndinfo snd_info;

                snd_info.snd_sid = rcv.rcv_sid;
                snd_info.snd_flags = 0;
                if (rcv.rcv_flags & SCTP_UNORDERED) {
                    snd_info.snd_flags |= SCTP_UNORDERED;
                }
                snd_info.snd_ppid = rcv.rcv_ppid;
                snd_info.snd_context = 0;
                snd_info.snd_assoc_id = rcv.rcv_assoc_id;
                if (usrsctp_sendv(sock, data, datalen, NULL, 0, &snd_info, sizeof(struct sctp_sndinfo), SCTP_SENDV_SNDINFO, 0) < 0) {
                    perror("sctp_sendv");
                }
            }
            */
        }
        free(data);
    }
    else{
        //printf("no data!!!\n");
        done = 1;
        //printf("usrsctp_close!!!\n");
        usrsctp_close(sock);
    }
    return (1);
}

void
debug_printf(const char *format, ...)
{
    va_list ap;

    va_start(ap, format);
    vprintf(format, ap);
    va_end(ap);
}

int
main(int argc, char *argv[])
{
    struct socket *sock;
    struct sockaddr_in6 addr;
    struct sctp_udpencaps encaps;
    struct sctp_event event;
    uint16_t event_types[] = {SCTP_ASSOC_CHANGE,
                              SCTP_PEER_ADDR_CHANGE,
                              SCTP_REMOTE_ERROR,
                              SCTP_SHUTDOWN_EVENT,
                              SCTP_ADAPTATION_INDICATION,
                              SCTP_PARTIAL_DELIVERY_EVENT};
    unsigned int i;
    struct sctp_assoc_value av;
    const int on = 1;
    ssize_t n;
    int flags;
    socklen_t from_len;
    char buffer[BUFFER_SIZE];
    //char name[INET6_ADDRSTRLEN];
    socklen_t infolen;
    struct sctp_rcvinfo rcv_info;
    unsigned int infotype;

    if (argc > 1) {
        usrsctp_init(atoi(argv[1]), NULL, debug_printf);
    } else {
        usrsctp_init(9899, NULL, debug_printf);
    }
#ifdef SCTP_DEBUG
    usrsctp_sysctl_set_sctp_debug_on(SCTP_DEBUG_NONE);
#endif
    usrsctp_sysctl_set_sctp_blackhole(2);

    if ((sock = usrsctp_socket(AF_INET6, SOCK_SEQPACKET, IPPROTO_SCTP, use_cb?receive_cb:NULL, NULL, 0, NULL)) == NULL) {
        perror("usrsctp_socket");
    }
    if (usrsctp_setsockopt(sock, IPPROTO_SCTP, SCTP_I_WANT_MAPPED_V4_ADDR, (const void*)&on, (socklen_t)sizeof(int)) < 0) {
        perror("usrsctp_setsockopt SCTP_I_WANT_MAPPED_V4_ADDR");
    }
    memset(&av, 0, sizeof(struct sctp_assoc_value));
    av.assoc_id = SCTP_ALL_ASSOC;
    av.assoc_value = 47;

    if (usrsctp_setsockopt(sock, IPPROTO_SCTP, SCTP_CONTEXT, (const void*)&av, (socklen_t)sizeof(struct sctp_assoc_value)) < 0) {
        perror("usrsctp_setsockopt SCTP_CONTEXT");
    }
    if (usrsctp_setsockopt(sock, IPPROTO_SCTP, SCTP_RECVRCVINFO, &on, sizeof(int)) < 0) {
        perror("usrsctp_setsockopt SCTP_RECVRCVINFO");
    }
    if (argc > 2) {
        memset(&encaps, 0, sizeof(struct sctp_udpencaps));
        encaps.sue_address.ss_family = AF_INET6;
        encaps.sue_port = htons(atoi(argv[2]));
        if (usrsctp_setsockopt(sock, IPPROTO_SCTP, SCTP_REMOTE_UDP_ENCAPS_PORT, (const void*)&encaps, (socklen_t)sizeof(struct sctp_udpencaps)) < 0) {
            perror("usrsctp_setsockopt SCTP_REMOTE_UDP_ENCAPS_PORT");
        }
    }
    memset(&event, 0, sizeof(event));
    event.se_assoc_id = SCTP_FUTURE_ASSOC;
    event.se_on = 1;
    for (i = 0; i < (unsigned int)(sizeof(event_types)/sizeof(uint16_t)); i++) {
        event.se_type = event_types[i];
        if (usrsctp_setsockopt(sock, IPPROTO_SCTP, SCTP_EVENT, &event, sizeof(struct sctp_event)) < 0) {
            perror("usrsctp_setsockopt SCTP_EVENT");
        }
    }
    memset((void *)&addr, 0, sizeof(struct sockaddr_in6));
#ifdef HAVE_SIN6_LEN
    addr.sin6_len = sizeof(struct sockaddr_in6);
#endif
    addr.sin6_family = AF_INET6;
    addr.sin6_port = htons(PORT);
    addr.sin6_addr = in6addr_any;
    if (usrsctp_bind(sock, (struct sockaddr *)&addr, sizeof(struct sockaddr_in6)) < 0) {
        perror("usrsctp_bind");
    }
    if (usrsctp_listen(sock, 1) < 0) {
        perror("usrsctp_listen");
    }

    signal(SIGALRM, print_bw);
    alarm(INTERVAL);
    while (!done) {
        if (use_cb) {
#ifdef _WIN32
        Sleep(SLEEP * 1000);
#else
        sleep(SLEEP);
#endif
        } else {
            from_len = (socklen_t)sizeof(struct sockaddr_in6);
            flags = 0;
            infolen = (socklen_t)sizeof(struct sctp_rcvinfo);
            n = usrsctp_recvv(sock, (void*)buffer, BUFFER_SIZE, (struct sockaddr *) &addr, &from_len, (void *)&rcv_info,
                              &infolen, &infotype, &flags);
            if (n > 0) {
                if (flags & MSG_NOTIFICATION) {
                    //printf("Notification of length %llu received.\n", (unsigned long long)n);
                } else {
                    gettimeofday(&end, NULL);
                    if(cnt == 0){
                        memcpy(&begin, &end, sizeof(struct timeval));
                    }
                    cnt++;
                    size+=n;
                    /*
                    if (infotype == SCTP_RECVV_RCVINFO) {
                        printf("Msg of length %llu received from %s:%u on stream %u with SSN %u and TSN %u, PPID %u, context %u, complete %d.\n",
                                (unsigned long long)n,
                                inet_ntop(AF_INET6, &addr.sin6_addr, name, INET6_ADDRSTRLEN), ntohs(addr.sin6_port),
                                :q
                                rcv_info.rcv_sid,
                                rcv_info.rcv_ssn,
                                rcv_info.rcv_tsn,
                                ntohl(rcv_info.rcv_ppid),
                                rcv_info.rcv_context,
                                (flags & MSG_EOR) ? 1 : 0);
                        if (flags & MSG_EOR) {
                            struct sctp_sndinfo snd_info;

                            snd_info.snd_sid = rcv_info.rcv_sid;
                            snd_info.snd_flags = 0;
                            if (rcv_info.rcv_flags & SCTP_UNORDERED) {
                                snd_info.snd_flags |= SCTP_UNORDERED;
                            }
                            snd_info.snd_ppid = rcv_info.rcv_ppid;
                            snd_info.snd_context = 0;
                            snd_info.snd_assoc_id = rcv_info.rcv_assoc_id;
                            if (usrsctp_sendv(sock, buffer, (size_t)n, NULL, 0, &snd_info, (socklen_t)sizeof(struct sctp_sndinfo), SCTP_SENDV_SNDINFO, 0) < 0) {
                                perror("sctp_sendv");
                            }
                        }
                    } else {
                        printf("Msg of length %llu received from %s:%u, complete %d.\n",
                                (unsigned long long)n,
                                inet_ntop(AF_INET6, &addr.sin6_addr, name, INET6_ADDRSTRLEN), ntohs(addr.sin6_port),
                                (flags & MSG_EOR) ? 1 : 0);
                    }
                    */
                }
            } else {
                break;
            }
        }
    }
    while (usrsctp_finish() != 0) {
#ifdef _WIN32
        Sleep(SLEEP * 1000);
#else
        sleep(SLEEP);
#endif
    }
    print_bw(0);
    return (0);
}
