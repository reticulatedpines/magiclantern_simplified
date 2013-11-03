/**
 * @file ml-cbr
 * @brief ML CBRs backend
 */
#ifndef ML_CBR_H
#define	ML_CBR_H

typedef enum {
   ML_CBR_STOP = 0,
   ML_CBR_CONTINUE = 1
} ml_cbr_action;

/**
 * @brief CBR signature
 */
typedef ml_cbr_action (* cbr_func) (const char *, void *);

/**
 * @brief Register a new CBRs to an event
 * @param event Event to register the CBR to. 
 * @param cbr Callback function to be invoked when the event happens
 * @param prio CBR priority. Higer priorities CBRs will be called before lower
 * priority ones.
 * @return 0 if registration successful
 */
int ml_register_cbr(const char * event, cbr_func cbr, unsigned int prio);

/**
 * @brief Unregisters a CBR to an event
 * @param event Event to which the CBR was associated
 * @param cbr The CBR to un-register
 * @return 0 if unregistration successful
 */
int ml_unregister_cbr(const char * event, cbr_func cbr);

/**
 * @brief Notify all the CBRs of an event
 * Notify all the CBRs associated to an event passing additional data to them.
 * <br>
 * CBRs are walked in priority order, from highest to lower. Higher priority
 * CBRs can block the flow by returning ML_CBR_STOP
 * @param event Triggering event (must not be NULL)
 * @param data  Data to be shared with the CBRs (can be NULL)
 */
void ml_notify_cbr(const char* event, void* data);

/**
 * @brief Prints out all the CBRs associated to a particular event
 * @param event The event to debug (must not be NULL)
 */
void debug_cbr_tree(const char* event);

/**
 * @brief Initializes the CBR backend (called from boot-hack.c)
 */
void _ml_cbr_init();

#endif	/* ML_CBR_H */

