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

typedef enum Netlogging_lvl {
    NETLOGG_CRIT    = 0,
    NETLOGG_ERROR,
    NETLOGG_WARN,
    NETLOGG_INFO,
    NETLOGG_DEBUG,
    NETLOGG_LVLS
} Netlogging_lvl;


typedef struct {
    const char      *progname;
    uint16_t        port;
    Netlogging_lvl  dft_lvl;
} Netlogging_args;


/**
 * \brief      Send a message through
 *
 * \param      l     level
 * \param      f     { parameter_description }
 * \param      ...   { parameter_description }
 *
 * \return     { description_of_the_return_value }
 */
#define NETLOGG(...)        netlogg_send(__FILE__, __LINE__, -1, __VA_ARGS__)


/**
 * \brief      Initiate the logging system and start it
 *
 * \param[in]  args The thread arguments
 *
 * \return     Error code
 */
void* netlogg_init(void* args);


void netlogg_start(void);


/**
 * \brief      Send a message to all connected clients
 *
 * \param[in]  file       The file
 * \param[in]  lineno     The line number
 * \param[in]  lvl        The logging level
 * \param      format     The format
 * \param[in]  ...        List of variable for the format
 *
 * \return     Error code
 */
int8_t netlogg_send(const char              *file,
                    const int32_t           lineno,
                    const int               fd,
                    const Netlogging_lvl    lvl,
                    const char              *format,
                    ...);


#ifdef __cplusplus
}
#endif

#endif          // __NETLOGGING_H__