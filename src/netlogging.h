/**
 * @file netlogging.h
 * @author hbuyse
 * @date 07/06/2017
 */


#ifndef __NETLOGGING_H__
#define __NETLOGGING_H__

#include <stdint.h>          // int8_t, int32_t
#include <stdio.h>           // vfprintf

#ifdef __cplusplus
extern "C" {
#endif


/**
 * \brief      Send a message through
 *
 * \param      l     level
 * \param      f     { parameter_description }
 * \param      ...   { parameter_description }
 *
 * \return     { description_of_the_return_value }
 */
#define NETLOGG(...) netlogg_send(__DATE__, __TIME__, __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)


typedef enum Netlogging_lvl {
    NETLOGG_CRIT = 0,
    NETLOGG_ERROR,
    NETLOGG_WARN,
    NETLOGG_INFO,
    NETLOGG_DEBUG,
    NETLOGG_LVLS
} Netlogging_lvl;


/**
 * \brief      Initiate the logging system
 *
 * \param[in]  progname  The program name
 * \param[in]  port      The port for the TCP interface
 * \param[in]  dft_lvl   The default level
 *
 * \return     Error code
 */
int8_t netlogg_init(const char *progname, uint16_t port, Netlogging_lvl dft_lvl);

void netlogg_start(void);

/**
 * \brief      Send a message to all connected clients
 *
 * \param[in]  date       The date
 * \param[in]  time       The time
 * \param[in]  file       The file
 * \param[in]  lineno     The line number
 * \param[in]  function   The function
 * \param[in]  lvl        The logging level
 * \param      format     The format
 * \param[in]  ...        List of variable for the format
 *
 * \return     Error code
 */
int8_t netlogg_send(const char              *date,
                    const char              *time,
                    const char              *file,
                    const int32_t           lineno,
                    const char              *function,
                    const Netlogging_lvl    lvl,
                    const char              *format,
                    ...);


/**
 * \brief      Change the displayed logging level
 *
 * \param[in]  new_lvl  The new logging level
 */
void netlogg_change_loglevel(Netlogging_lvl new_lvl);


/**
 * \brief      Get the number of connected clients
 *
 * \return     Number of connected clients to the logger
 */
int32_t netlogg_nb_connected_clients(void);


#ifdef __cplusplus
}
#endif

#endif          // __NETLOGGING_H__