/**
 * @file devices.h
 * @author hbuyse
 * @date 08/05/2016
 *
 * @brief  Functions that reports to the user's devices
 */

#include "netlogging.h"


/**
 * \brief      Initiate the logging system
 *
 * \param      progname  The program name
 * \param[in]  dft_lvl   The default level
 *
 * \return     Error code
 */
int8_t netlogg_init(char            *progname,
                    Netlogging_lvl  dft_lvl
                    )
{
    return (0);
}



/**
 * \brief      Send a message to all connected clients
 *
 * \param[in]  lvl        The level
 * \param      format     The format
 * \param[in]  ...        List of variable
 *
 * \return     Error code
 */
int8_t netlogg_send(Netlogging_lvl  lvl,
                    const char      *date,
                    const char      *time,
                    const char      *file,
                    const int32_t   lineno,
                    const char      *function,
                    char            *format,
                    ...
                    )
{
    return (0);
}



/**
 * \brief      Change the displayed logging level
 *
 * \param[in]  new_lvl  The new logging level
 *
 * \return     Error code
 */
int8_t netlogg_change_loglevel(Netlogging_lvl new_lvl)
{
    return (0);
}



/**
 * \brief      Get the number of connected clients
 *
 * \return     Number of connected clients to the logger
 */
int32_t netlogg_nb_connected_clients(void)
{
    return (0);
}