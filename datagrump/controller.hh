#ifndef CONTROLLER_HH
#define CONTROLLER_HH

#include <cstdint>

/* Congestion controller interface */

class Controller {
private:
  bool debug_; /* Enables debugging output */

  double cwnd_;
  double ssthresh_;

  uint64_t last_ack_;
  bool has_last_ack_;
  unsigned int duplicate_ack_count_;

  bool has_rtt_sample_;
  double estimated_rtt_ms_;
  double dev_rtt_ms_;
  unsigned int rto_ms_;

public:
  /* Public interface for the congestion controller */
  /* You can change these if you prefer, but will need to change
     the call site as well (in sender.cc) */

  /* Default constructor */
  Controller(const bool debug);

  /* Get current window size, in datagrams */
  unsigned int window_size();

  /* A datagram was sent */
  void datagram_was_sent(const uint64_t sequence_number,
                         const uint64_t send_timestamp,
                         const bool after_timeout);

  /* An ack was received */
  void ack_received(const uint64_t sequence_number_acked,
                    const uint64_t send_timestamp_acked,
                    const uint64_t recv_timestamp_acked,
                    const uint64_t timestamp_ack_received);

  /* How long to wait (in milliseconds) if there are no acks
     before sending one more datagram */
  unsigned int timeout_ms();
};

#endif
