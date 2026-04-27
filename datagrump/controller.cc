#include <algorithm>
#include <cmath>
#include <iostream>

#include "controller.hh"
#include "timestamp.hh"

using namespace std;

namespace {
constexpr double ALPHA = 0.125;
constexpr double BETA = 0.25;
constexpr unsigned int MIN_RTO_MS = 100;
constexpr unsigned int MAX_RTO_MS = 5000;
constexpr unsigned int INITIAL_RTO_MS = 1000;
constexpr double INITIAL_SSTHRESH = 16.0;
constexpr unsigned int DUP_ACK_THRESHOLD = 3;

unsigned int clamp_rto(const double rto_ms) {
  return min(MAX_RTO_MS,
             max(MIN_RTO_MS, static_cast<unsigned int>(ceil(rto_ms))));
}
} // namespace

/* Default constructor */
Controller::Controller(const bool debug)
    : debug_(debug), cwnd_(1.0), ssthresh_(INITIAL_SSTHRESH), last_ack_(0),
      has_last_ack_(false), duplicate_ack_count_(0), has_rtt_sample_(false),
      estimated_rtt_ms_(0.0), dev_rtt_ms_(0.0), rto_ms_(INITIAL_RTO_MS) {}

/* Get current window size, in datagrams */
unsigned int Controller::window_size() {
  const unsigned int current_window =
      max(1u, static_cast<unsigned int>(cwnd_));

  if (debug_) {
    cerr << "At time " << timestamp_ms() << " window size is "
         << current_window << " (cwnd = " << cwnd_
         << ", ssthresh = " << ssthresh_ << ", rto = " << rto_ms_ << " ms)"
         << endl;
  }

  return current_window;
}

/* A datagram was sent */
void Controller::datagram_was_sent(
    const uint64_t sequence_number,
    /* of the sent datagram */
    const uint64_t send_timestamp,
    /* in milliseconds */
    const bool after_timeout
    /* datagram was sent because of a timeout */) {
  if (after_timeout) {
    cout << "timeout" << endl;
    ssthresh_ = max(cwnd_ / 2.0, 1.0);
    cwnd_ = 1.0;
    duplicate_ack_count_ = 0;
    rto_ms_ = min(MAX_RTO_MS, max(MIN_RTO_MS, rto_ms_ * 2));
  }

  if (debug_) {
    cerr << "At time " << send_timestamp << " sent datagram " << sequence_number
         << " (timeout = " << after_timeout << ", cwnd = " << cwnd_
         << ", ssthresh = " << ssthresh_ << ", rto = " << rto_ms_ << " ms)\n";
  }
}

/* An ack was received */
void Controller::ack_received(
    const uint64_t sequence_number_acked,
    /* what sequence number was acknowledged */
    const uint64_t send_timestamp_acked,
    /* when the acknowledged datagram was sent (sender's clock) */
    const uint64_t recv_timestamp_acked,
    /* when the acknowledged datagram was received (receiver's clock)*/
    const uint64_t timestamp_ack_received)
/* when the ack was received (by sender) */
{
  cout << "num_acked:" << sequence_number_acked << endl;

  const bool is_duplicate_ack = has_last_ack_ && sequence_number_acked == last_ack_;
  const bool is_new_ack = !has_last_ack_ || sequence_number_acked > last_ack_;
  const double sample_rtt_ms =
      static_cast<double>(timestamp_ack_received - send_timestamp_acked);

  if (is_new_ack) {
    if (!has_rtt_sample_) {
      estimated_rtt_ms_ = sample_rtt_ms;
      dev_rtt_ms_ = sample_rtt_ms / 2.0;
      has_rtt_sample_ = true;
    } else {
      estimated_rtt_ms_ =
          (1.0 - ALPHA) * estimated_rtt_ms_ + ALPHA * sample_rtt_ms;
      dev_rtt_ms_ =
          (1.0 - BETA) * dev_rtt_ms_ +
          BETA * fabs(sample_rtt_ms - estimated_rtt_ms_);
    }
    rto_ms_ = clamp_rto(estimated_rtt_ms_ + 4.0 * dev_rtt_ms_);
  }

  if (is_duplicate_ack) {
    cout << "got same ack " << sequence_number_acked << endl;
    duplicate_ack_count_++;

    if (duplicate_ack_count_ >= DUP_ACK_THRESHOLD) {
      ssthresh_ = max(cwnd_ / 2.0, 1.0);
      cwnd_ = 1.0;
      duplicate_ack_count_ = 0;
    }
  } else if (is_new_ack) {
    duplicate_ack_count_ = 0;

    if (cwnd_ < ssthresh_) {
      cwnd_ += 1.0;
    } else {
      cwnd_ += 1.0 / cwnd_;
    }
  }

  if (is_new_ack || is_duplicate_ack) {
    last_ack_ = sequence_number_acked;
    has_last_ack_ = true;
  }

  if (debug_) {
    cerr << "At time " << timestamp_ack_received
         << " received ack for datagram " << sequence_number_acked
         << " (send @ time " << send_timestamp_acked << ", received @ time "
         << recv_timestamp_acked << " by receiver's clock, sample RTT = "
         << sample_rtt_ms << " ms, estimated RTT = " << estimated_rtt_ms_
         << " ms, dev RTT = " << dev_rtt_ms_ << " ms, RTO = " << rto_ms_
         << " ms, cwnd = " << cwnd_ << ", ssthresh = " << ssthresh_ << ")"
         << endl;
  }
}

/* How long to wait (in milliseconds) if there are no acks
   before sending one more datagram */
unsigned int Controller::timeout_ms() {
  return rto_ms_;
}
