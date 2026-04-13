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

  uint64_t ack_sequence_number = 0;
  uint64_t next_expected_data_sequence = 0;

  /* Loop and acknowledge every incoming datagram back to its source */
  while (true) {
    const UDPSocket::received_datagram recd = socket.recv();
    ContestMessage message = recd.payload;

    const uint64_t received_sequence = message.header.sequence_number;
    if (received_sequence == next_expected_data_sequence) {
      next_expected_data_sequence++;
    } else if (next_expected_data_sequence > 0) {
      message.header.sequence_number = next_expected_data_sequence - 1;
    }

    message.transform_into_ack(ack_sequence_number++, recd.timestamp);
    message.set_send_timestamp();
    socket.sendto(recd.source_address, message.to_string());
  }

  return EXIT_SUCCESS;
}
