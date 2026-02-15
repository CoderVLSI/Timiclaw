#ifndef TRANSPORT_TELEGRAM_H
#define TRANSPORT_TELEGRAM_H

typedef void (*incoming_cb_t)(const char *msg);

void transport_telegram_init(void);
void transport_telegram_poll(incoming_cb_t cb);
void transport_telegram_send(const char *msg);

#endif
