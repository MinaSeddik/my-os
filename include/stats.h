
#ifndef STATS_H
#define STATS_H

#include <types.h>

typedef struct stats_nic 
{
  u32_t rx_packets;             // Total packets received
  u32_t tx_packets;             // Total packets transmitted
  u32_t rx_bytes;               // Total bytes received
  u32_t tx_bytes;               // Total bytes transmitted
  u32_t rx_errors;              // Bad packets received
  u32_t tx_errors;              // Packet transmit problems
  u32_t rx_dropped;             // Received packets dropped
  u32_t tx_dropped;             // Transmitted packets dropped
  u32_t multicast;              // Multicast packets received
  u32_t collisions;

  // Detailed rx errors
   u32_t rx_length_errors;
   u32_t rx_over_errors;         // Receiver ring buff overflow
   u32_t rx_crc_errors;          // Recved pkt with crc error
   u32_t rx_frame_errors;        // Recv'd frame alignment error
   u32_t rx_fifo_errors;         // Recv'r fifo overrun
   u32_t rx_missed_errors;       // Receiver missed packet

  // Detailed tx errors
  u32_t tx_aborted_errors;
  u32_t tx_carrier_errors;
  u32_t tx_fifo_errors;
  u32_t tx_heartbeat_errors;
  u32_t tx_window_errors;

  // Compression
  u32_t rx_compressed;
  u32_t tx_compressed;

}stats_nic;

typedef struct stats_proto 
{
  u32_t xmit;    // Transmitted packets
  u32_t rexmit;  // Retransmitted packets
  u32_t recv;    // Received packets
  u32_t fw;      // Forwarded packets
  u32_t drop;    // Dropped packets
  u32_t chkerr;  // Checksum error
  u32_t lenerr;  // Invalid length error
  u32_t memerr;  // Out of memory error
  u32_t rterr;   // Routing error
  u32_t proterr; // Protocol error
  u32_t opterr;  // Error in options
  u32_t err;     // Misc error
}stats_proto;

typedef struct stats_pbuf 
{
  u32_t avail;
  u32_t used;
  u32_t max;  
  u32_t err;
  u32_t reclaimed;

  u32_t alloc_locked;
  u32_t refresh_locked;

  u32_t rwbufs;
}stats_pbuf;

typedef struct netstats 
{
  stats_proto link;
  stats_proto ip;
  stats_proto icmp;
  stats_proto udp;
  stats_proto tcp;
  stats_proto raw;
  stats_pbuf pbuf;
}netstats;

extern netstats stats;

extern netstats* get_netstats();

void stats_init();

#endif
