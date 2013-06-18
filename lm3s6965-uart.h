// lm3s6965-uart.h
//
//
//
//
//
//



int uC_kbhit(int uart);
int uC_getc(int uart);

void uC_print(char* s, int uart);
void uC_init_UART(int uart, int baud, int irda);
void uC_emit(char c, int uart);

void uC_set_console(int uart); 
void uC_printf(const char* s, ...);

char* uC_fgets(char *s, int size, int uart);
