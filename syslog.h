// syslog.h
//
//
//
//
//
#ifndef __SYSLOG__
#define __SYSLOG__ 1
//
//
//
//

#define LOG_OFF     0x0000
#define LOG_EMERG   0x0001
#define LOG_ALERT   0x0002
#define LOG_CRIT    0x0004
#define LOG_ERR     0x0010
#define LOG_WARNING 0x0020
#define LOG_NOTICE  0x0040
#define LOG_INFO    0x0080
#define LOG_DEBUG   0x0100
#define LOG_ALL     0x01FF
#define LOG_PRINT   0xFFFF

//int set_syslog_prio(int x);
//int get_syslog_level(void);

void syslog(int prio, char *template, ...);
int syslog_level_get(void);
void syslog_level_set(int level);
void syslog_print_set(void (*ftn)(char*));
char* syslog_help(void);



#define SYS_LOG_ALL() set_syslog_prio(LOG_ALL)
#define SYS_LOG_OFF() set_syslog_prio(LOG_OFF)
#define SYS_LOG_EMERG() set_syslog_prio(LOG_EMERG)


#endif
