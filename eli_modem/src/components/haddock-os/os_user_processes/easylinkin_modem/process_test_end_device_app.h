/**
 * process_test_end_device_app.h
 *
 * \date   
 * \author jtuki@foxmail.com
 * \author 
 */
#ifndef PROCESS_TEST_USER_APPLICATION_H_
#define PROCESS_TEST_USER_APPLICATION_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include "lpwan_utils.h"

#define SIGNAL_USER_APP_UPLINK_MSG_PERIODICALLY BV(0)

#define PROC_PRIORITY_TEST_USER_APP 2

void proc_test_end_device_app_init(void);

#ifdef __cplusplus
}
#endif

#endif /* PROCESS_TEST_USER_APPLICATION_H_ */
