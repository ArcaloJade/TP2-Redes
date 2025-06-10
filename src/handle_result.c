#include <endian.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <arpa/inet.h>  /* htonl */
#include <string.h>     /* memcpy */
#include <ctype.h>     /* needed for isprint() */
#include <errno.h>
#include "header.h"

#ifndef HEXDUMP_COLS
#define HEXDUMP_COLS 16
#endif 

void hexdump(void *mem, unsigned int len);

#define NUM_CONN 10

#define E_MINIMUM_DATA    -1
#define E_NOT_ENOUGH_DATA -2
#define E_NO_NEW_LINE     -3
#define E_LINE_TOO_LONG   -4
#define E_INV_LINE_FORMAT -5
#define E_NUMBER_PARSE    -6



void printBwResult(BW_result bw_result);
int packResultPayload(BW_result bw_result, void *buffer, int buffer_size);
int unpackResultPayload(BW_result *bw_result, void *buffer, int buffer_size);


// int main(int argc, char *argv[])
// {
//   BW_result bw_result;

//   union {
//     uint32_t u32;
//     uint8_t arr[4];
//   } id_measurement;

//   /* Generate a random measurement ID*/
//   /*  seed initialization */
//   srandom(time(NULL));
//   id_measurement.u32 = (uint32_t) random();
//   /* Make sure MSB is not FF */
//   if (id_measurement.arr[3] == 0xff) {
//     id_measurement.arr[3] = id_measurement.arr[3] & 0x7f;
//   }
//   id_measurement.arr[0] = 0xc9;
//   id_measurement.arr[1] = 0xaf;
//   id_measurement.arr[2] = 0x3a;
//   id_measurement.arr[3] = 0x42;
    
//   /* Print id as individual byte from lower to higher addresses */
//   for (int i=0 ; i < sizeof(id_measurement.u32); i++ ) {
//     /* i=0 Lowest-address byte */
//     printf("id.arr[%d] = 0x%02x\n", i, id_measurement.arr[i]);
//   }
//   printf("id.u32 = 0x%x\n\n", id_measurement.u32);
  
//   bw_result.id_measurement = id_measurement.u32;
//   /* Populate with some example values */
//   for (int i=0; i< NUM_CONN; i++) {
//     bw_result.conn_bytes[i] = 1000000000+i*100;
//     bw_result.conn_duration[i] = 20.0 + (i*1.0)/100;
//   }
  
//   printf("Results to be packed\n");
//   printBwResult(bw_result);

//   char my_buffer[200];
//   int nbytes;
//   nbytes = packResultPayload(bw_result,  my_buffer, (int) sizeof(my_buffer));
//   printf("nbytes = %d\n", nbytes);
//   if (nbytes > 0) {

//     printf("Hexdump of packed results\n");
//     // Tests
//     // my_buffer[13] = 'c'; // uncomment to generate number parse error
//     // my_buffer[14] = 'x'; // uncomment to generate invalid format line error
//     // my_buffer[0x15] = '0'; my_buffer[0x20] = '0';  my_buffer[0x23] = '5';// uncomment to generate not enough data
//     // my_buffer[0x15] = '0'; my_buffer[0x20] = '0';  my_buffer[0x23] = '5'; my_buffer[0x27] = '4';// uncomment to generate Line too long
//      hexdump(my_buffer, nbytes);
//   }

//   /* Structure for parsed results */
//   BW_result rcv_bw_result;
//   int parsed_bytes = 0;
//   parsed_bytes = unpackResultPayload(&rcv_bw_result, my_buffer, nbytes);
//   /* Print received results */
//   if (parsed_bytes > 0 ) {
//     printf("\nUnpacked results");
//     printBwResult(rcv_bw_result);
//     printf("parsed bytes %d\n", parsed_bytes);
//   }
//   else {
//     switch (parsed_bytes) {
//       case E_MINIMUM_DATA:
//         printf("Too few data. Not even for measurement id\n");
//         break;
//       case E_NOT_ENOUGH_DATA:
//         printf("Too few data for connection measurement data\n");
//         break;
//       case E_NO_NEW_LINE:    
//         printf("No new line found before reaching buffer end\n");
//         break;
//       case E_LINE_TOO_LONG:  
//         printf("Line too long to be parsed\n");
//         break;
//       case E_INV_LINE_FORMAT:
//         printf("Invalid format line\n");
//         break;
//       case E_NUMBER_PARSE:   
//         printf("Error while parsing number\n");
//         break;
//     }
//   }

//   return 0;
// }


void printBwResult(BW_result bw_result) {
  printf("id 0x%x\n", bw_result.id_measurement);
  for (int i=0; i< NUM_CONN; i++) {
    printf("conn %2d  %20ld bytes %.3f seconds\n", i, bw_result.conn_bytes[i], bw_result.conn_duration[i]);
  }
}


int packResultPayload(BW_result bw_result, void *buffer, int buffer_size) {

  int bytes_temp_buffer = 0;

  // bytes   [20 chars] 1.8e+19
  // in-line separator[1] ","
  // duration[6 chars]  20.001 
  // line separator[1] "\n"
  // size: NUM_CONN*(20+1+6+1) + 4  
  //    if NUM_CONN = 10:   284
  // Large enough temporary buffer
  char my_buffer[500];
  memset(my_buffer, 0, sizeof(my_buffer));
  
  // Pack id
  uint32_t netorder_id = htonl(bw_result.id_measurement);
  memcpy(my_buffer, &netorder_id, sizeof(netorder_id));
  bytes_temp_buffer += sizeof(netorder_id);

  // Pack each connection measurement
  for (int i=0; i<NUM_CONN; i++) {
    char aux[20];
    int  n_aux;
    memset(aux, 0 , sizeof(aux));
    n_aux = sprintf(aux, "%lu,%.3f\n", bw_result.conn_bytes[i], bw_result.conn_duration[i]);
    memcpy(my_buffer+bytes_temp_buffer, aux, n_aux);
    bytes_temp_buffer += n_aux;
  }
  
  if (buffer_size >= bytes_temp_buffer) {
    /* Copy results to buffer given */
    memcpy(buffer, my_buffer, bytes_temp_buffer);
  }
  else {
    /* Not enough buffer space */
    return -1;
  }

  return bytes_temp_buffer;
}

int unpackResultPayload(BW_result *bw_result, void *buffer, int buffer_size) {
  if (buffer_size < sizeof(uint32_t)) {
    /* Buffer too small for minimum data */
    return E_MINIMUM_DATA; 
  }

  char *buf = (char *)buffer;
  int offset = 0;
    
  // Unpack id_measurement
  uint32_t netorder_id;
  memcpy(&netorder_id, buf + offset, sizeof(netorder_id));
  bw_result->id_measurement = ntohl(netorder_id);
  offset += sizeof(netorder_id);

  // Unpack each connection measurement
  for (int i = 0; i < NUM_CONN; i++) {
    if (offset >= buffer_size) {
      /* Not enough data */
      return E_NOT_ENOUGH_DATA; 
    }
    
    /* Find the end of current line (newline character) */
    int line_start = offset;
    int line_end = line_start;
    while (line_end < buffer_size && buf[line_end] != '\n') {
        line_end++;
    }
    if (line_end >= buffer_size) {
      /* No newline found or buffer overflow */
      return E_NO_NEW_LINE; 
    }
    
    // Extract the line (without newline)
    int line_length = line_end - line_start;
    char line[40]; /*  Should be enough for the format  20 + 1+ 6  */
    if (line_length >= sizeof(line)) {
      /* Line too long */
      return E_LINE_TOO_LONG; 
    }
    memcpy(line, buf + line_start, line_length);
    line[line_length] = '\0'; /* Null terminate C-string */
    //printf("Line [%d,%d] %s\n", line_start, line_end, line);
    
    // Parse the line: "conn_bytes,conn_duration"
    char *comma_pos = strchr(line, ',');
    if (comma_pos == NULL) {
      /* Invalid format - no comma found */
      return E_INV_LINE_FORMAT; 
    }
    
    // Split at comma
    *comma_pos = '\0';
    char *bytes_str = line;
    char *duration_str = comma_pos + 1;
    
    /* Parse bytes (uint64_t) */
    char *endptr;
    errno = 0;
    unsigned long long parsed_bytes = strtoull(bytes_str, &endptr, 10);
    if (errno != 0 || *endptr != '\0') {
        return E_NUMBER_PARSE;
    }
    bw_result->conn_bytes[i] = (uint64_t)parsed_bytes;
    
    /* Parse duration (double) */
    errno = 0;
    double parsed_duration = strtod(duration_str, &endptr);
    if (errno != 0 || *endptr != '\0') {
        return E_NUMBER_PARSE;
    }
    bw_result->conn_duration[i] = parsed_duration;
    
    /* Move to next line (skip the newline) */
    offset = line_end + 1;
  }
  
  return offset; // Return total bytes consumed
}



// http://grapsus.net/blog/post/Hexadecimal-dump-in-C
void hexdump(void *mem, unsigned int len)
{
  unsigned int i, j;

  for(i = 0; i < len + ((len % HEXDUMP_COLS) ? (HEXDUMP_COLS - len % HEXDUMP_COLS) : 0); i++)
  {
    /* print offset */
    if(i % HEXDUMP_COLS == 0)
    {
      printf("0x%06x: ", i);
    }

    /* print hex data */
    if(i < len)
    {
      printf("%02x ", 0xFF & ((char*)mem)[i]);
    }
    else /* end of block, just aligning for ASCII dump */
    {
      printf("   ");
    }

    /* print ASCII dump */
    if(i % HEXDUMP_COLS == (HEXDUMP_COLS - 1))
    {
      for(j = i - (HEXDUMP_COLS - 1); j <= i; j++)
      {
        if(j >= len) /* end of block, not really printing */
        {
          putchar(' ');
        }
        else if(isprint(((char*)mem)[j])) /* printable char */
        {
          putchar(0xFF & ((char*)mem)[j]);
        }
        else /* other char */
        {
          putchar('.');
        }
      }
      putchar('\n');
    }
  }
}
