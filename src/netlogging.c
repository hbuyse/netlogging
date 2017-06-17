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

#define NBELEMS(e)              (sizeof(e) / sizeof(e[0]) )


#define NETLOGG_BACK(...)        netlogg_send(__DATE__, __TIME__, __FILE__, __LINE__, __VA_ARGS__)


/**
 * \brief      Handle the new connections
 *
 * \param      p       The epoll context
 * \param[in]  events  The events
 */
static void netlogg_handle_new_connection(struct epoll_fd_ctx *p, unsigned long events);




/**
 * \brief      Handler of the messages entering
 *
 * \param      p       The epoll context
 * \param[in]  events  The events
 */
static void netlogg_handle_comm(struct epoll_fd_ctx *p, unsigned long events);



/**
 * \brief      Send a message to all connected clients
 *
 * \param      p       The epoll context
 * \param[in]  events  The events
 */
static void netlogg_send_to_all_connected_clients(struct epoll_fd_ctx *p, unsigned long events);




/**
 * \brief      Close the specified connection (p->fd)
 *
 * \param      p       The epoll context
 * \param[in]  events  The events
 */
static void netlogg_close_conn(epoll_fd_ctx *p);


/**
 * \brief      Handler of the exit commands
 *
 * \param      p          The epoll context
 * \param      buff       The buffer
 * \param[in]  recv_size  The receive size
 */
static void handle_exit(struct epoll_fd_ctx *p, char *buff, ssize_t recv_size);


/**
 * \brief      Handler of the help command
 *
 * \param      p          The epoll context
 * \param      buff       The buffer
 * \param[in]  recv_size  The receive size
 */
static void handle_help(struct epoll_fd_ctx *p, char *buff, ssize_t recv_size);


/**
 * \brief      Function that change the global loglevel to CRIT
 *
 * \param      p          The epoll context
 * \param      buff       The buffer
 * \param[in]  recv_size  The received size
 */
static void handle_loglevel_crit(struct epoll_fd_ctx *p, char *buff, ssize_t recv_size);


/**
 * \brief      Function that change the global loglevel to ERROR
 *
 * \param      p          The epoll context
 * \param      buff       The buffer
 * \param[in]  recv_size  The received size
 */
static void handle_loglevel_error(struct epoll_fd_ctx *p, char *buff, ssize_t recv_size);


/**
 * \brief      Function that change the global loglevel to WARN
 *
 * \param      p          The epoll context
 * \param      buff       The buffer
 * \param[in]  recv_size  The received size
 */
static void handle_loglevel_info(struct epoll_fd_ctx *p, char *buff, ssize_t recv_size);


/**
 * \brief      Function that change the global loglevel to INFO
 *
 * \param      p          The epoll context
 * \param      buff       The buffer
 * \param[in]  recv_size  The received size
 */
static void handle_loglevel_warn(struct epoll_fd_ctx *p, char *buff, ssize_t recv_size);


/**
 * \brief      Function that change the global loglevel to DEBUG
 *
 * \param      p          The epoll context
 * \param      buff       The buffer
 * \param[in]  recv_size  The received size
 */
static void handle_loglevel_debug(struct epoll_fd_ctx *p, char *buff, ssize_t recv_size);


/**
 * \brief      Function that change the command that ask for the connected clients
 *
 * \param      p          The epoll context
 * \param      buff       The buffer
 * \param[in]  recv_size  The received size
 */
static void handle_client_list(struct epoll_fd_ctx *p, char *buff, ssize_t recv_size);


/**
 * Variable contenant le nom du programme
 */
static char     *gProgname          = NULL;


/**
 * \brief Variable permettant de stocket la valeur du loglevel global
 */
static Netlogging_lvl     gLvl      = NETLOGG_DEBUG;


/**
 * \brief General epoll file descriptor
 */
static int     ep_fd                = -1;


/**
 * \brief Socket where we send the data
 */
static int     netlogg_send_fd      = -1;


/**
 * \brief Command that can be send to the program and their handlers
 */
static recv_cmd_t       recv_cmds[] =
{
    {.cmd = "\004", .desc = NULL, .handler = handle_exit},
    {.cmd = "help", .desc = "Show the help", .handler = handle_help},
    {.cmd = "exit", .desc = "Close the connection", .handler = handle_exit},
    {.cmd = "quit", .desc = "Close the connection", .handler = handle_exit},
    {.cmd = "loglevel crit", .desc = "Change the client loglevel to CRIT", .handler = handle_loglevel_crit},
    {.cmd = "loglevel error", .desc = "Change the client loglevel to ERROR", .handler = handle_loglevel_error},
    {.cmd = "loglevel info", .desc = "Change the client loglevel to INFO", .handler = handle_loglevel_info},
    {.cmd = "loglevel warn", .desc = "Change the client loglevel to WARN", .handler = handle_loglevel_warn},
    {.cmd = "loglevel debug", .desc = "Change the client loglevel to DEBUG", .handler = handle_loglevel_debug},
    {.cmd = "client list", .desc = "Show the list of clients", .handler = handle_client_list}
};


static epoll_fd_ctx     netlogger_ctx[] =
{
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


    // Set global variables
    gProgname   = strdup(progname);
    gLvl        = dft_lvl;


    // Create epoll file descriptor
    ep_fd       = epoll_create(EPOLL_FD_MAX);

    if ( ep_fd == -1 )
    {
        NETLOGG(NETLOGG_ERROR, "%s - epoll_create: %m", __FUNCTION__);
        assert(ep_fd != -1);
    }

    // Listening socket for the netlogger clients
    netlogger_ctx[EPOLL_FD_LISTEN].fd = socket(AF_INET, SOCK_STREAM, 0);
    assert(netlogger_ctx[EPOLL_FD_LISTEN].fd != -1);


    // Reuse addr
    res = setsockopt(netlogger_ctx[EPOLL_FD_LISTEN].fd, SOL_SOCKET, SO_REUSEADDR, (int *) &reuseAddr, sizeof(reuseAddr) );

    if ( res == -1 )
    {
        NETLOGG(NETLOGG_ERROR, "%s - setsockopt: %m", __FUNCTION__);
        assert(res != -1);
    }

    // TCP IP / port / interfaces
    sock_tcp_in.sin_family      = AF_INET;
    sock_tcp_in.sin_port        = htons(port);
    sock_tcp_in.sin_addr.s_addr = htonl(INADDR_ANY);


    // Bind to the port
    res = bind(netlogger_ctx[EPOLL_FD_LISTEN].fd, (struct sockaddr *) &sock_tcp_in, sizeof(sock_tcp_in) );

    if ( res == -1 )
    {
        NETLOGG(NETLOGG_ERROR, "%s - bind: %m", __FUNCTION__);
        assert(res != -1);
    }

    // Listen for incoming connection
    res = listen(netlogger_ctx[EPOLL_FD_LISTEN].fd, 5);

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

    // Create socket pair
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
    res = epoll_ctl(ep_fd, EPOLL_CTL_ADD, netlogger_ctx[EPOLL_FD_RECV].fd, &ep_ev);

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
                    const int               fd,
                    const Netlogging_lvl    lvl,
                    const char              *format,
                    ...
                    )
{
    ssize_t     send_bytes          = -1;
    int         w = -1;
    va_list     ap;

    // Initiate the internal struct
    internal_buff internal_msg = {
        .fd = fd,
        .lvl = lvl,
        .buff = {0}
    };


    // Add the traces informations
    w = snprintf(internal_msg.buff, sizeof(internal_msg.buff), "%s %s - %s:%d - ", date, time, file, lineno);

    // Add the level in the traces
    switch ( lvl )
    {
        case NETLOGG_CRIT:
            w   += snprintf(internal_msg.buff + w, sizeof(internal_msg.buff) - w, "\033[31mCRIT\033[0m - ");
            break;

        case NETLOGG_ERROR:
            w   += snprintf(internal_msg.buff + w, sizeof(internal_msg.buff) - w, "\033[31mERROR\033[0m - ");
            break;

        case NETLOGG_WARN:
            w   += snprintf(internal_msg.buff + w, sizeof(internal_msg.buff) - w, "\033[33mWARN\033[0m - ");
            break;

        case NETLOGG_INFO:
            w   += snprintf(internal_msg.buff + w, sizeof(internal_msg.buff) - w, "\033[32mINFO\033[0m - ");
            break;

        case NETLOGG_DEBUG:
            w   += snprintf(internal_msg.buff + w, sizeof(internal_msg.buff) - w, "DEBUG - ");
            break;

        default:
            w   += snprintf(internal_msg.buff + w, sizeof(internal_msg.buff) - w, "\033[31mUNKNOWN_LVL\033[0m - ");
    }

    // Beginning of the varibale list
    va_start(ap, format);
    w   += vsnprintf(internal_msg.buff + w, sizeof(internal_msg.buff) - w, format, ap);
    va_end(ap);

    w   += snprintf(internal_msg.buff + w, sizeof(internal_msg.buff) - w, "\n");

    // Send to a connected client
    send_bytes = send(netlogg_send_fd, &internal_msg, sizeof(internal_msg), 0);

    if ( send_bytes == -1 )
    {
        fprintf(stderr, "%s - send: %m\n", __FUNCTION__);
    }

    if ( lvl <= gLvl )
    {
        fprintf( (lvl < NETLOGG_INFO) ? stderr : stdout, "%s", internal_msg.buff);
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
    uint8_t     i   = 0;
    int     new_fd  = -1;
    struct sockaddr_in      remote_sockaddr;
    socklen_t               r_sz = sizeof(remote_sockaddr);
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
            netlogger_ctx[i].lvl        = NETLOGG_DEBUG;
            netlogger_ctx[i].ipv4_addr  = strdup(inet_ntoa(remote_sockaddr.sin_addr) );

            getnameinfo( (const struct sockaddr *) &remote_sockaddr, sizeof(remote_sockaddr), netlogger_ctx[i].hostname,
                         sizeof(netlogger_ctx[i].hostname), netlogger_ctx[i].service, sizeof(netlogger_ctx[i].service), 0);


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
                NETLOGG(NETLOGG_DEBUG,
                        "New client %s added in the epoll loop (%s, %s)",
                        netlogger_ctx[i].ipv4_addr,
                        netlogger_ctx[i].hostname,
                        netlogger_ctx[i].service);
                NETLOGG(NETLOGG_INFO, "%d clients connected", netlogg_nb_connected_clients() );

                // Prin the help
                handle_help(&netlogger_ctx[i], NULL, 0);
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
    ssize_t     r = -1;


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
            uint32_t     i = 0;


            // Remove carriage return and newline feed if found
            buff[strcspn(buff, "\r\n")] = 0;

            if ( strlen(buff) != 0 )
            {
                // Parse all the commands, if found we use the associated function
                for ( i = 0; i < NBELEMS(recv_cmds); ++i )
                {
                    if ( strncmp(recv_cmds[i].cmd, buff, strlen(recv_cmds[i].cmd) ) == 0 )
                    {
                        if ( recv_cmds[i].handler != NULL )
                        {
                            (*recv_cmds[i].handler)(p, buff, r);
                        }

                        break;
                    }
                }

                // If the command is not known, we trace it as a warning
                if ( i == NBELEMS(recv_cmds) )
                {
                    NETLOGG(NETLOGG_WARN, "Unknown command from %s: %s", p->ipv4_addr, buff);
                    NETLOGG(NETLOGG_DEBUG, "Receive %zd bytes from %s (fd: %d) - unused", strlen(buff), p->ipv4_addr, p->fd);
                }
            }
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
        internal_buff internal_msg;


        // Lecture des événements notifiés
        recv_size = recv(p->fd, &internal_msg, sizeof(internal_msg), 0);

        if ( recv_size == -1 )
        {
            NETLOGG(NETLOGG_ERROR, "%s - recv: %m\n", __FUNCTION__);
        }
        else if ( recv_size == 0 )
        {
            assert(recv_size == 0);
        }
        else
        {
            if ( internal_msg.fd == -1 )
            {
                // Parse all possible communication socket
                for ( i = EPOLL_FD_SEND0; i <= EPOLL_FD_SEND9; i++ )
                {
                    if ( (netlogger_ctx[i].fd != -1) && (internal_msg.lvl <= netlogger_ctx[i].lvl))
                    {
                        // Send to a connected client
                        send_size = send(netlogger_ctx[i].fd, internal_msg.buff, strlen(internal_msg.buff), 0);

                        if ( send_size == -1 )
                        {
                            NETLOGG(NETLOGG_ERROR, "%s - send: %m\n", __FUNCTION__);
                        }
                        else if ( (size_t) send_size != strlen(internal_msg.buff) )
                        {
                            NETLOGG(NETLOGG_ERROR, "%s - send: send_size (%zd) != recv_size (%zd)\n", __FUNCTION__, send_size, recv_size);
                        }
                    }
                }
            }
            else
            {
                // Parse all possible communication socket
                for ( i = EPOLL_FD_SEND0; i <= EPOLL_FD_SEND9; i++ )
                {
                    if ( (netlogger_ctx[i].fd == internal_msg.fd) && (netlogger_ctx[i].fd != -1) && (internal_msg.lvl <= netlogger_ctx[i].lvl))
                    {
                        // Send to a specific connected client
                        send_size = send(netlogger_ctx[i].fd, internal_msg.buff, strlen(internal_msg.buff), 0);

                        if ( send_size == -1 )
                        {
                            NETLOGG(NETLOGG_ERROR, "%s - send: %m\n", __FUNCTION__);
                        }
                        else if ( (size_t) send_size != strlen(internal_msg.buff) )
                        {
                            NETLOGG(NETLOGG_ERROR, "%s - send: send_size (%zd) != recv_size (%zd)\n", __FUNCTION__, send_size, recv_size);
                        }
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



static void handle_exit(struct epoll_fd_ctx *p,
                        char                *buff,
                        ssize_t             recv_size
                        )
{
    NETLOGG(NETLOGG_INFO, "Closing connection (user demand)");

    netlogg_close_conn(p);
}



static void handle_help(struct epoll_fd_ctx *p,
                        char                *buff,
                        ssize_t             recv_size
                        )
{
    uint32_t     i = 0;

    NETLOGG_BACK(p->fd, NETLOGG_INFO, "The available commands are:");

    for ( i = 2; i < NBELEMS(recv_cmds); ++i )
    {
        if ( recv_cmds[i].handler == NULL )
        {
            continue;
        }

        NETLOGG_BACK(p->fd, NETLOGG_INFO, "%s : %s", recv_cmds[i].cmd, recv_cmds[i].desc);
    }
}



static void handle_loglevel_crit(struct epoll_fd_ctx    *p,
                                 char                   *buff,
                                 ssize_t                recv_size
                                 )
{
    NETLOGG_BACK(p->fd, NETLOGG_INFO, "Changing loglevel to \033[1mCRIT\033[0m (from %s)", p->ipv4_addr);

    p->lvl = NETLOGG_CRIT;
}



static void handle_loglevel_error(struct epoll_fd_ctx   *p,
                                  char                  *buff,
                                  ssize_t               recv_size
                                  )
{
    NETLOGG_BACK(p->fd, NETLOGG_INFO, "Changing loglevel to \033[1mERROR\033[0m (from %s)", p->ipv4_addr);

    p->lvl = NETLOGG_ERROR;
}



static void handle_loglevel_info(struct epoll_fd_ctx    *p,
                                 char                   *buff,
                                 ssize_t                recv_size
                                 )
{
    NETLOGG_BACK(p->fd, NETLOGG_INFO, "Changing loglevel to \033[1mINFO\033[0m (from %s)", p->ipv4_addr);

    p->lvl = NETLOGG_INFO;
}



static void handle_loglevel_warn(struct epoll_fd_ctx    *p,
                                 char                   *buff,
                                 ssize_t                recv_size
                                 )
{
    NETLOGG_BACK(p->fd, NETLOGG_INFO, "Changing loglevel to \033[1mWARN\033[0m (from %s)", p->ipv4_addr);

    p->lvl = NETLOGG_WARN;
}



static void handle_loglevel_debug(struct epoll_fd_ctx   *p,
                                  char                  *buff,
                                  ssize_t               recv_size
                                  )
{
    NETLOGG_BACK(p->fd, NETLOGG_INFO, "Changing loglevel to \033[1mDEBUG\033[0m (from %s)", p->ipv4_addr);

    p->lvl = NETLOGG_DEBUG;
}



static void handle_client_list(struct epoll_fd_ctx  *p,
                               char                 *buff,
                               ssize_t              recv_size
                               )
{
    epoll_evt_t     i = EPOLL_FD_SEND0;


    NETLOGG_BACK(p->fd, NETLOGG_INFO, "Clients list asked by %s: %d clients connected", p->ipv4_addr, netlogg_nb_connected_clients() );

    for ( i = EPOLL_FD_SEND0; i <= EPOLL_FD_SEND9; i++ )
    {
        if ( netlogger_ctx[i].fd != -1 )
        {
            NETLOGG_BACK(p->fd, NETLOGG_INFO, "Client %d: %s (%s:%s)", i + 1 - EPOLL_FD_SEND0, netlogger_ctx[i].hostname, netlogger_ctx[i].ipv4_addr, netlogger_ctx[i].service);
        }
    }

    netlogg_change_loglevel(NETLOGG_DEBUG);
}