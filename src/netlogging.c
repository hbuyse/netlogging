/**
 * @file netlogging.c
 * @author hbuyse
 * @date 07/06/2017
 */

#include <stdlib.h>          // NULL
#include <string.h>          // stdup
#include <sys/epoll.h>          // epoll_create, epoll_wait, eop
#include <stdio.h>          // fprintf, stderr
#include <assert.h>          // assert
#include <stdarg.h>          // va_list, va_start, va_end
#include <sys/types.h>          // socket, bind, listen
#include <sys/socket.h>          // socket, bind, listen
#include <netinet/ip.h>          // INADDR_ANY
#include <arpa/inet.h>          // inet_ntoa
#include <unistd.h>             // close
#include <netdb.h>              // getnameinfo
#include <errno.h>              // errno

#include "netlogging.h"          // Netlogging_lvl


#ifndef INET4_ADDRSTRLEN
    #define INET4_ADDRSTRLEN    16
#endif


#define BUFF_SIZE_MAX           4096
#define HOSTNAME_MAX_SIZE       256
#define SERVICE_MAX_SIZE        256

/**
 * Variable contenant le nom du programme
 */
static char     *gProgname      = NULL;


/**
 * \brief Variable permettant de stocket la valeur du loglevel global
 */
static Netlogging_lvl     gLvl  = NETLOGG_DEBUG;


/**
 * General epoll file descriptor
 */
static int     ep_fd            = -1;


/**
 * Socket where we send the data
 */
static int     netlogg_send_fd  = -1;


/**
 * \struct REC_fdContext
 * \brief Définition du contexte des événements de la boucle epoll
 */
typedef struct epoll_fd_ctx {
    void(*const handler)(struct epoll_fd_ctx *p, unsigned long events);          ///< Gestionnaire dédié à une cause de réveil de la boucle epoll du
                                                                                 // module d'enregistrement
    int fd;          ///< Descripteur de l'événement
    char *ipv4_addr;          ///< Client's IP (dynamically created by strdup, careful when freeing it)
    char hostname[HOSTNAME_MAX_SIZE];       ///< Client host name
    char service[SERVICE_MAX_SIZE];    ///< Service name
} epoll_fd_ctx;


typedef enum {
    EPOLL_FD_LISTEN = 0,
    EPOLL_FD_RECV,
    EPOLL_FD_SEND0,
    EPOLL_FD_SEND1,
    EPOLL_FD_SEND2,
    EPOLL_FD_SEND3,
    EPOLL_FD_SEND4,
    EPOLL_FD_SEND5,
    EPOLL_FD_SEND6,
    EPOLL_FD_SEND7,
    EPOLL_FD_SEND8,
    EPOLL_FD_SEND9,
    EPOLL_FD_MAX,
} epoll_evt_t;



static void netlogg_handle_new_connection(struct epoll_fd_ctx *p, unsigned long events);


static void netlogg_handle_comm(struct epoll_fd_ctx *p, unsigned long events);


static void netlogg_send_to_all_connected_clients(struct epoll_fd_ctx *p, unsigned long events);


static void netlogg_close_conn(epoll_fd_ctx *p);


static epoll_fd_ctx     netlogger_ctx[] = {
    [EPOLL_FD_LISTEN]   = {netlogg_handle_new_connection, -1, NULL},
    [EPOLL_FD_RECV]     = {netlogg_send_to_all_connected_clients, -1, NULL},
    [EPOLL_FD_SEND0]    = {netlogg_handle_comm, -1, NULL},
    [EPOLL_FD_SEND1]    = {netlogg_handle_comm, -1, NULL},
    [EPOLL_FD_SEND2]    = {netlogg_handle_comm, -1, NULL},
    [EPOLL_FD_SEND3]    = {netlogg_handle_comm, -1, NULL},
    [EPOLL_FD_SEND4]    = {netlogg_handle_comm, -1, NULL},
    [EPOLL_FD_SEND5]    = {netlogg_handle_comm, -1, NULL},
    [EPOLL_FD_SEND6]    = {netlogg_handle_comm, -1, NULL},
    [EPOLL_FD_SEND7]    = {netlogg_handle_comm, -1, NULL},
    [EPOLL_FD_SEND8]    = {netlogg_handle_comm, -1, NULL},
    [EPOLL_FD_SEND9]    = {netlogg_handle_comm, -1, NULL}
};


int8_t netlogg_init(const char      *progname,
                    uint16_t        port,
                    Netlogging_lvl  dft_lvl
                    )
{
    int     res         = -1;
    int     reuseAddr   = 1;
    struct epoll_event ep_ev;
    struct sockaddr_in sock_tcp_in;
    int     sv[2]       = {-1, -1};


    /* Set global variables
     */
    gProgname   = strdup(progname);
    gLvl        = dft_lvl;


    /* Create epoll file descriptor
     */
    ep_fd       = epoll_create(EPOLL_FD_MAX);

    if ( ep_fd == -1 )
    {
        NETLOGG(NETLOGG_ERROR, "%s - epoll_create: %m", __FUNCTION__);
        assert(ep_fd != -1);
    }


    /* Listening socket for the netlogger clients
     */
    netlogger_ctx[EPOLL_FD_LISTEN].fd = socket(AF_INET, SOCK_STREAM, 0);
    assert(netlogger_ctx[EPOLL_FD_LISTEN].fd != -1);


    /* Reuse addr
     */
    res = setsockopt(netlogger_ctx[EPOLL_FD_LISTEN].fd, SOL_SOCKET, SO_REUSEADDR, (int *) &reuseAddr, sizeof(reuseAddr) );

    if ( res == -1 )
    {
        NETLOGG(NETLOGG_ERROR, "%s - setsockopt: %m", __FUNCTION__);
        assert(res != -1);
    }

    /* TCP IP / port / interfaces
     */
    sock_tcp_in.sin_family      = AF_INET;
    sock_tcp_in.sin_port        = htons(port);
    sock_tcp_in.sin_addr.s_addr = htonl(INADDR_ANY);


    /* Bind to the port
     */
    res             = bind(netlogger_ctx[EPOLL_FD_LISTEN].fd, (struct sockaddr *) &sock_tcp_in, sizeof(sock_tcp_in) );

    if ( res == -1 )
    {
        NETLOGG(NETLOGG_ERROR, "%s - bind: %m", __FUNCTION__);
        assert(res != -1);
    }


    /* Listen for incoming connection
     */
    res             = listen(netlogger_ctx[EPOLL_FD_LISTEN].fd, 5);

    if ( res == -1 )
    {
        NETLOGG(NETLOGG_ERROR, "%s - listen: %m", __FUNCTION__);
        assert(res != -1);
    }

    ep_ev.events    = EPOLLIN | EPOLLHUP | EPOLLERR | EPOLLRDHUP;
    ep_ev.data.ptr  = &netlogger_ctx[EPOLL_FD_LISTEN];


    // Ajout du contexte de la socket d'écoute  dans la table des evenements epoll
    res             = epoll_ctl(ep_fd, EPOLL_CTL_ADD, netlogger_ctx[EPOLL_FD_LISTEN].fd, &ep_ev);

    if ( res == -1 )
    {
        NETLOGG(NETLOGG_ERROR, "%s - epoll_ctl: %m", __FUNCTION__);
        assert(res != -1);
    }

    /* Create socket pair
     */
    res             = socketpair(AF_UNIX, SOCK_STREAM, 0, sv);

    if ( res == -1 )
    {
        NETLOGG(NETLOGG_ERROR, "%s - socketpair: %m", __FUNCTION__);
        assert(res != -1);
    }
    else
    {
        netlogger_ctx[EPOLL_FD_RECV].fd = sv[1];
        netlogg_send_fd = sv[0];
        NETLOGG(NETLOGG_DEBUG, "socketpair");
    }

    ep_ev.events    = EPOLLIN | EPOLLHUP | EPOLLERR | EPOLLRDHUP;
    ep_ev.data.ptr  = &netlogger_ctx[EPOLL_FD_RECV];


    // Ajout du contexte de la socket d'écoute  dans la table des evenements epoll
    res             = epoll_ctl(ep_fd, EPOLL_CTL_ADD, netlogger_ctx[EPOLL_FD_RECV].fd, &ep_ev);

    if ( res == -1 )
    {
        NETLOGG(NETLOGG_ERROR, "%s - epoll_ctl: %m", __FUNCTION__);
        assert(res != -1);
    }

    return (0);
}



void netlogg_start(void)
{
    for ( ; ; )
    {
        int     timeout = -1;
        struct epoll_event ep_ev;
        int     nb      = epoll_wait(ep_fd, &ep_ev, 1, timeout);

        if ( nb > 0 )
        {
            epoll_fd_ctx     *p = ep_ev.data.ptr;

            assert(nb == 1);
            assert(p);
            assert(p->handler);

            // Traitement de l'événement
            (*p->handler)(p, ep_ev.events);
        }
        else if ( nb == 0 )
        {
            NETLOGG(NETLOGG_WARN, "st_fsm: rien à traiter");
        }
        else
        {
            if ( (nb == -1) && (errno == EINTR) )
            {
                NETLOGG(NETLOGG_WARN, "st_fsm: signal intercepté");
            }
            else
            {
                NETLOGG(NETLOGG_ERROR, "Invalide: %m");
            }
        }
    }
}



int8_t netlogg_send(const char              *date,
                    const char              *time,
                    const char              *file,
                    const int32_t           lineno,
                    const char              *function,
                    const Netlogging_lvl    lvl,
                    const char              *format,
                    ...
                    )
{
    ssize_t     send_bytes          = -1;
    char        buff[BUFF_SIZE_MAX] = {0};
    int         w = -1;
    va_list     ap;

    // Add the traces informations
    w = snprintf(buff, sizeof(buff), "%s %s - %s:%d - %s - ", date, time, file, lineno, function);

    // Add the level in the traces
    switch(lvl)
    {
        case NETLOGG_CRIT:
            w += snprintf(buff + w, sizeof(buff) - w, "CRIT - ");
            break;
            
        case NETLOGG_ERROR:
            w += snprintf(buff + w, sizeof(buff) - w, "ERROR - ");
            break;
            
        case NETLOGG_WARN:
            w += snprintf(buff + w, sizeof(buff) - w, "WARN - ");
            break;
            
        case NETLOGG_INFO:
            w += snprintf(buff + w, sizeof(buff) - w, "INFO - ");
            break;
            
        case NETLOGG_DEBUG:
            w += snprintf(buff + w, sizeof(buff) - w, "DEBUG - ");
            break;

        default:
            w += snprintf(buff + w, sizeof(buff) - w, "UNKNOWN_LVL - ");
    }

    // Beginning of the varibale list
    va_start(ap, format);
    w += vsnprintf(buff + w, sizeof(buff) - w, format, ap);
    va_end(ap);

    w += snprintf(buff + w, sizeof(buff) - w, "\n");


    // Check if we have to send the message
    if ( lvl <= gLvl )
    {
        if ( netlogg_send_fd != -1 )
        {
            // Send to a connected client
            send_bytes = send(netlogg_send_fd, buff, w, 0);

            if ( send_bytes == -1 )
            {
                fprintf(stderr, "%s - send: %m\n", __FUNCTION__);
            }
        }
        
        fprintf((lvl < NETLOGG_INFO) ? stdout : stderr, "%s", buff);
    }

    return (0);
}



void netlogg_change_loglevel(Netlogging_lvl new_lvl)
{
    gLvl = new_lvl;
}



int32_t netlogg_nb_connected_clients(void)
{
    uint8_t         i = 0;
    uint32_t        nb_connected_clients = 0;


    // Parse all possible communication socket
    for ( i = EPOLL_FD_SEND0; i <= EPOLL_FD_SEND9; i++ )
    {
        if ( netlogger_ctx[i].fd != -1 )
        {
            nb_connected_clients++;
        }
    }

    return (nb_connected_clients);
}



static void netlogg_handle_new_connection(struct epoll_fd_ctx   *p,
                                          unsigned long         events
                                          )
{
    uint8_t                 i       = 0;
    int                     new_fd  = -1;
    struct sockaddr_in      remote_sockaddr;
    socklen_t               r_sz    = sizeof(remote_sockaddr);
    struct epoll_event      ep_ev;


    /* Accept the new remote connection
     */
    new_fd = accept(p->fd, (struct sockaddr *) &remote_sockaddr, &r_sz);

    if ( new_fd == -1 )
    {
        NETLOGG(NETLOGG_ERROR, "accept: %m");
        assert(new_fd != -1);
    }
    else
    {
        NETLOGG(NETLOGG_DEBUG, "Accept new remote connection");
    }


    /* Check for empty socket
     */
    for ( i = EPOLL_FD_SEND0; i <= EPOLL_FD_SEND9; i++ )
    {
        if ( netlogger_ctx[i].fd == -1 )
        {
            // Update epoll context
            netlogger_ctx[i].fd         = new_fd;
            netlogger_ctx[i].ipv4_addr  = strdup(inet_ntoa(remote_sockaddr.sin_addr) );

            getnameinfo((const struct sockaddr *) &remote_sockaddr, sizeof(remote_sockaddr), netlogger_ctx[i].hostname, sizeof(netlogger_ctx[i].hostname), netlogger_ctx[i].service, sizeof(netlogger_ctx[i].service), 0);


            // Create the event struct
            ep_ev.events    = EPOLLIN | EPOLLHUP | EPOLLERR | EPOLLRDHUP;
            ep_ev.data.ptr  = &netlogger_ctx[i];

            if ( epoll_ctl(ep_fd, EPOLL_CTL_ADD, new_fd, &ep_ev) == -1 )
            {
                NETLOGG(NETLOGG_ERROR, "epoll_ctl: %m");
                netlogg_close_conn(&netlogger_ctx[i]);
            }
            else
            {
                NETLOGG(NETLOGG_DEBUG, "Client %s added in the epoll loop (%s, %s)", netlogger_ctx[i].ipv4_addr, netlogger_ctx[i].hostname, netlogger_ctx[i].service );
                NETLOGG(NETLOGG_INFO, "%d clients connected", netlogg_nb_connected_clients());
            }

            break;
        }
    }
}



static void netlogg_handle_comm(struct epoll_fd_ctx *p,
                                unsigned long       events
                                )
{
    char        buff[BUFF_SIZE_MAX] = {0};
    ssize_t     r __attribute__((unused)) = -1;

    if ( events & EPOLLIN )
    {
        r = recv(p->fd, buff, sizeof(buff), 0);

        if ( r == -1 )
        {
            NETLOGG(NETLOGG_ERROR, "recv: %m");
        }

        if ( r == 0 )
        {
            NETLOGG(NETLOGG_DEBUG, "No bytes received from %s. We are going to close it.", p->ipv4_addr);
        }
        else if ( r > 0 )
        {
            NETLOGG(NETLOGG_DEBUG, "Receive %zd bytes from %s (fd: %d) - unused", r, p->ipv4_addr, p->fd);
        }
        else if ( p->fd == -1 )
        {
            NETLOGG(NETLOGG_ERROR, "p->fd == -1");

            return;
        }
    }

    if ( events & EPOLLRDHUP )
    {
        NETLOGG(NETLOGG_INFO, "Closing connection (EPOLLRDHUP)");
        netlogg_close_conn(p);
    }

    if ( events & EPOLLERR )
    {
        NETLOGG(NETLOGG_INFO, "Closing connection (EPOLLERR)");
        netlogg_close_conn(p);
    }

    if ( events & EPOLLHUP )
    {
        NETLOGG(NETLOGG_INFO, "Closing connection (EPOLLHUP)");
        netlogg_close_conn(p);
    }

    assert(! (events & EPOLLOUT) );
}



static void netlogg_send_to_all_connected_clients(struct epoll_fd_ctx   *p,
                                                  unsigned long         events
                                                  )
{
    uint8_t     i           = 0;
    ssize_t     recv_size   = -1;
    ssize_t     send_size   = -1;


    if ( events & EPOLLERR )
    {
        assert(! "EPOLLERR");
    }

    if ( events & EPOLLHUP )
    {
        assert(! "EPOLLHUP");
    }

    if ( events & EPOLLIN )
    {
        char                    buff[BUFF_SIZE_MAX] = {0};

        // Lecture des événements notifiés
        recv_size = recv(p->fd, buff, sizeof(buff), 0);

        if ( recv_size == -1 )
        {
            NETLOGG(NETLOGG_ERROR, "%s - recvfrom: %m\n", __FUNCTION__);
        }
        else if ( recv_size == 0 )
        {}
        else
        {
            // Parse all possible communication socket
            for ( i = EPOLL_FD_SEND0; i <= EPOLL_FD_SEND9; i++ )
            {
                if ( netlogger_ctx[i].fd != -1 )
                {
                    // Send to a connected client
                    send_size = send(netlogger_ctx[i].fd, buff, recv_size, 0);

                    if ( send_size == -1 )
                    {
                        NETLOGG(NETLOGG_ERROR, "%s - send: %m\n", __FUNCTION__);
                    }
                    else if ( send_size != recv_size )
                    {
                        NETLOGG(NETLOGG_ERROR, "%s - send: send_size (%zd) != recv_size (%zd)\n", __FUNCTION__, send_size, recv_size);
                    }
                }
            }
        }
    }

    if ( events & EPOLLOUT )
    {
        assert(! "EPOLLOUT");
    }
}



static void netlogg_close_conn(epoll_fd_ctx *p)
{
    if ( p->fd != -1 )
    {
        // Suppression de la socket de la boucle epoll
        if ( epoll_ctl(ep_fd, EPOLL_CTL_DEL, p->fd, NULL) == -1 )
        {
            NETLOGG(NETLOGG_ERROR, "epoll_ctl: %m");
        }


        // Closing the connection
        NETLOGG(NETLOGG_INFO, "Closing connection from %s", p->ipv4_addr);
        close(p->fd);


        // Update epoll context
        p->fd = -1;

        if ( p->ipv4_addr != NULL )
        {
            free(p->ipv4_addr);
            p->ipv4_addr = NULL;
        }
    }
    else
    {
        NETLOGG(NETLOGG_INFO, "p->fd already -1");
    }
}