/* simple UDP receiver that acknowledges every datagram */

#include <cstdlib>
#include <iostream>

#include "contest_message.hh"
#include "socket.hh"

using namespace std;

int main(int argc, char *argv[]) {
  /* check the command-line arguments */
  if (argc < 1) { /* for sticklers */
    abort();
  }

  if (argc != 2) {
    cerr << "Usage: " << argv[0] << " PORT" << endl;
    return EXIT_FAILURE;
  }

  /* create UDP socket for incoming datagrams */
  UDPSocket socket;

  /* turn on timestamps on receipt */
  socket.set_timestamps();

  /* "bind" the socket to the user-specified local port number */
  socket.bind(Address("::0", argv[1]));

  cerr << "Listening on " << socket.local_address().to_string() << endl;

  uint64_t sequence_number = 0;
  uint64_t packet_count = 0;
  const uint64_t ACK_INTERVAL = 2;
  uint64_t last_ack_seq = 0;
  uint64_t last_ack_timestamp = 0;
  Address last_dest_addr;
  string last_message_str;

  /* Loop and acknowledge every incoming datagram back to its source */
  while (true) {
    const UDPSocket::received_datagram recd = socket.recv();
    ContestMessage message = recd.payload;

    packet_count++;
    last_ack_seq = sequence_number++;
    last_ack_timestamp = recd.timestamp;
    last_dest_addr = recd.source_address;

    message.transform_into_ack(last_ack_seq, last_ack_timestamp);
    message.set_send_timestamp();
    last_message_str = message.to_string();

    if (packet_count >= ACK_INTERVAL) {
      socket.sendto(last_dest_addr, last_message_str);
      packet_count = 0;
    }
  }

  return EXIT_SUCCESS;
}
