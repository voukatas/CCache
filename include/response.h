#ifndef RESPONSE_H
#define RESPONSE_H

// Functions
void write_response_str(char *response, char *response_value);
void write_response_int(char *response, int response_value);

// This function pointer helps with the unit testing
// typedef void (*response_writer_t)(char *response,  char *response_value);
// void set_response_writer(response_writer_t writer);
// extern response_writer_t response_writer;

#endif // RESPONSE_H
