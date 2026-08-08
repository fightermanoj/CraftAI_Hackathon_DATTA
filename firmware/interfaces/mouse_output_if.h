#ifndef MOUSE_OUTPUT_IF_H
#define MOUSE_OUTPUT_IF_H

#include <stdbool.h>
#include <stdint.h>

bool mouse_output_init(void);
bool mouse_output_connected(void);
bool mouse_output_send(int8_t dx, int8_t dy);

#endif
