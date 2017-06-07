/**
 * @file netlogging.h
 * @author hbuyse
 * @date 08/06/2017
 *
 * @brief  Functions that reports to the user's devices
 */


#ifndef __NETLOGGING_H__
#define __NETLOGGING_H__

#include <stdint.h>   // int8_t, int32_t

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
#define NETLOGG(l, f, ...)  netlogg_send(l, __DATE__, __TIME__, __FILE__, __LINE__, __FUNCTION__, f, __VA_ARGS__)


typedef enum Netlogging_lvl
{
    NETLOGG_CRIT = 0,
    NETLOGG_ERROR,
    NETLOGG_WARN,
    NETLOGG_INFO,
    NETLOGG_DEBUG,
    NETLOGG_LVLS
} Netlogging_lvl ;

/**
 * \brief      Initiate the logging system
 *
 * \param      progname  The program name
 * \param[in]  dft_lvl   The default level
 *
 * \return     Error code
 */
int8_t netlogg_init(char* progname, Netlogging_lvl dft_lvl);

/**
 * \brief      Send a message to all connected clients
 *
 * \param[in]  lvl        The level
 * \param      format     The format
 * \param[in]  ...        List of variable
 *
 * \return     Error code
 */
int8_t netlogg_send(Netlogging_lvl lvl, const char* date, const char* time, const char* file, const int32_t lineno, const char* function, char* format, ...);

/**
 * \brief      Change the displayed logging level
 *
 * \param[in]  new_lvl  The new logging level
 *
 * \return     Error code
 */
int8_t netlogg_change_loglevel(Netlogging_lvl new_lvl);

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