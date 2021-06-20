/***
 * Sockets related code adapted from the one found at:
 * https://github.com/mitrapinaki/PeerToPeer
 ***/


#include "Mix_Worker.h"
#include <iostream>
#include <string>
#include <unistd.h>
#include <mutex>

std::mutex mtx;
std::mutex send_message_mtx;

const int KEY_SIZE = 256;
const int MAX_MESSAGE_LEN = KEY_SIZE + 1;
const char *PREVIOUS_MIX_NAME = "127.0.0.1";
const char *NEXT_MIX_NAME = "127.0.0.1";
const char *CONSUMER_PRODUCER_NAME = "127.0.0.1";


void wrap_message(unsigned char *message_to_send, unsigned char *content, char message_type) {
	message_to_send[0] = message_type;
	std::copy(content, content + KEY_SIZE, message_to_send + 1);
}

char unwrap_message(unsigned char *content, unsigned char *message_received) {
	std::copy(message_received + 1, message_received + MAX_MESSAGE_LEN, content);
	return message_received[0];
}


Mix_Worker::Mix_Worker(std::string ReceiverPort, int prev_port, int next_port,
                       int producer_port, sgx_enclave_id_t *eid) {
	this->ReceiverPort = ReceiverPort;
	this->prev_port = prev_port;
	this->next_port = next_port;
	this->producer_port = producer_port;
	this->eid = *eid;
}

void Mix_Worker::send_with_retry(string sender_name, int port, void *content, int size) {
	bool send_status = false;
	do {
		send_status = send_message_to_socket(sender_name, port, content, size);

		if (!send_status)
			sleep(1);
	} while (!send_status);
}

bool Mix_Worker::send_message_to_socket(std::string sender_name, int port, void *content, int size) {
	int sockfd = 0, portno = 0;
	struct hostent *server;

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) {
		cout << "ERROR opening socket" << endl;
		return false;
	}

	server = gethostbyname(sender_name.c_str());

	if (server == NULL) {
		cout << "ERROR, no such host\n";
		return false;
	}

	bzero((char *) &serv_addr, sizeof(serv_addr)); // Erase data
	serv_addr.sin_family = AF_INET;
	bcopy((char *) server->h_addr, (char *) &serv_addr.sin_addr.s_addr, server->h_length);
	serv_addr.sin_port = htons(port);

	if (connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
		cout << "ERROR connecting" << endl << strerror(errno) << endl;
		return false;
	}

	struct timeval tv;
	tv.tv_sec = 20;  /* 20 Secs Timeout */
	tv.tv_usec = 0;

	if (setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, (char *) &tv, sizeof(tv)) < 0) {
		printf("Time Out\n");
		return false;
	}

	send(sockfd, content, size, 0);
	close(sockfd);
	return true;
}

void Mix_Worker::send_message_worker() {
	while (!this->flag_received_previous_module) {
		sleep(1);
	}
	while (true) {
		usleep(this->sending_rate);

		unsigned char content[KEY_SIZE];
		int fan_out = 0;
		size_t buffer_size = 1;
		int dispatch_response = 0;

		send_message_mtx.lock();

		mtx.lock();
		cout << "Locking" << endl;
		dispatch(this->eid, &dispatch_response, content, &fan_out, &buffer_size, this->fan_all_out);
		cout << "Unlocking" << endl;
		mtx.unlock();

		unsigned char message_to_send[MAX_MESSAGE_LEN];

		if (dispatch_response >= 0) {
			if (fan_out) {
				wrap_message(message_to_send, content, '2');

				unsigned char message_to_send_producer[MAX_MESSAGE_LEN + this->ReceiverPort.length()];
				std::copy(this->ReceiverPort.c_str(), this->ReceiverPort.c_str() + this->ReceiverPort.length(),
				          message_to_send_producer);
				std::copy(message_to_send, message_to_send + MAX_MESSAGE_LEN,
				          message_to_send_producer + this->ReceiverPort.length());

				send_with_retry(CONSUMER_PRODUCER_NAME, this->producer_port, message_to_send_producer,
				                strlen((char *) message_to_send_producer));
			} else {
				wrap_message(message_to_send, content, '1');

				// send the next message
				send_with_retry(NEXT_MIX_NAME, this->next_port, message_to_send, MAX_MESSAGE_LEN);
			}
		}

		send_message_mtx.unlock();

		cout << "Buffer size: " << buffer_size << endl;
		cout << "Fan all out: " << this->fan_all_out << endl;
		cout << "Previous started fan out: " << this->flag_previous_started_fan_out << endl;

		if (this->fan_all_out && this->flag_previous_started_fan_out && buffer_size == 0) {
			return;
		}
	}
}

void Mix_Worker::receive_messages() {
	while (true) {
		newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen); // Waiting connections

		if (newsockfd < 0)
			cout << "ERROR on accept" << strerror(errno) << "\n";

		unsigned char cmd[MAX_MESSAGE_LEN];
		ssize_t recv_bytes = recv(newsockfd, cmd, MAX_MESSAGE_LEN, 0);

		// message with the public key
		unsigned char message[KEY_SIZE];
		if (unwrap_message(message, cmd) == '0') {
			cout << "Saving the next mix public key!" << endl;
			set_public_key(this->eid, message);
			this->flag_received_previous_module = true;
		}

		// message to save in the enclave's buffer
		if (unwrap_message(message, cmd) == '1') {
			cout << "Saving the message to enclave!" << endl;

			mtx.lock();
			import_message(this->eid, message);
			mtx.unlock();
		}

		if (unwrap_message(message, cmd) == '3') {
			cout << "Received order to fan all the messages out and to stop the node from the producer!" << endl;
			this->fan_all_out = 1;

			send_message_mtx.lock();
			// send order to fan out to the previous node
			unsigned char *message_to_send = (unsigned char *) "4\0";
			send_with_retry(PREVIOUS_MIX_NAME, this->next_port, message_to_send, 2);
			send_message_mtx.unlock();
		}

		if (unwrap_message(message, cmd) == '4') {
			cout << "Received end of work from the previous node!" << endl;
			this->flag_previous_started_fan_out = true;
		}

		close(newsockfd);
	}
}

void Mix_Worker::start() {
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0)
		std::cout << "ERROR opening socket" << strerror(errno) << "\n";

	bzero((char *) &serv_addr, sizeof(serv_addr)); // Erase data

	portno = atoi(ReceiverPort.c_str());
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(portno);

	int opt = 1;
	setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

	if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
		std::cout << "ERROR on binding" << std::endl;
	listen(sockfd, 4096);
	clilen = sizeof(cli_addr);

	// thread to receive messages
	thread Receive_Messages_Job(&Mix_Worker::receive_messages, this);
	sleep(1);

	// create private and public key and obtain the correspondent public module
	create_keys(this->eid, this->my_public_module);

	// send my public module to the previous mix
	unsigned char message_to_send[MAX_MESSAGE_LEN];
	wrap_message(message_to_send, this->my_public_module, '0');

	cout << "Sending the public key to the previous mix node" << endl;
	send_with_retry(PREVIOUS_MIX_NAME, this->prev_port, message_to_send, MAX_MESSAGE_LEN);

	// send the public module to the producer
	cout << "Sending the public key to the consumer/producer" << endl;
	unsigned char message_to_send_producer[MAX_MESSAGE_LEN + this->ReceiverPort.length()];
	std::copy(this->ReceiverPort.c_str(), this->ReceiverPort.c_str() + this->ReceiverPort.length(),
	          message_to_send_producer);
	std::copy(message_to_send, message_to_send + MAX_MESSAGE_LEN,
	          message_to_send_producer + this->ReceiverPort.length());
	send_with_retry(CONSUMER_PRODUCER_NAME, this->producer_port, message_to_send_producer, MAX_MESSAGE_LEN + 4);

	// thread to send messages
	thread Send_Messages_Job(&Mix_Worker::send_message_worker, this);

	Send_Messages_Job.join();
	Receive_Messages_Job.detach();
	close(sockfd);

	cout << "Exiting" << endl;

	return;
}
