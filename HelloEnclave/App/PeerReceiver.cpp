#include "color.h"
#include "PeerReceiver.h"
#include <iostream>
#include <string>
#include <unistd.h>
#include <mutex>

using namespace std;


std::mutex mtx;


void wrap_message(unsigned char *message_to_send, unsigned char *content, char message_type) {
   message_to_send[0] = message_type;
   std::copy(content, content + 256, message_to_send + 1);
}

char unwrap_message(unsigned char *content, unsigned char *message_received) {
   std::copy(message_received + 1, message_received + MAX_COMMAND_LEN, content);
   return message_received[0];
}


PeerReceiver::PeerReceiver(std::string ReceiverName, std::string ReceiverPort, int prev_port, int next_port, int producer_port, 
									sgx_enclave_id_t *eid){
	this->ReceiverName = ReceiverName;
	this->ReceiverPort = ReceiverPort;
	this->prev_port = prev_port;
   this->next_port = next_port;
   this->producer_port = producer_port;
	this->eid = *eid;
}

void PeerReceiver::SendRetry(string SenderName, int port, void *content, int size) {
	bool send_status = false;
	do {
		send_status = Send(SenderName, port, content, size);
	
		if (!send_status)
			sleep(1);
	} while(!send_status);
}

bool PeerReceiver::Send(string SenderName, int port, void *content, int size){
	int sockfd=0, portno=0;
	struct hostent *server;

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) {
		cout << "ERROR opening socket" << endl;
		return false;
	}

	server = gethostbyname(SenderName.c_str());

	if (server == NULL) {
		cout<< "ERROR, no such host\n";
		return false;
	}
	
	bzero((char *) &serv_addr, sizeof(serv_addr)); // Erase data
	serv_addr.sin_family = AF_INET;
	bcopy((char *) server->h_addr,(char *)&serv_addr.sin_addr.s_addr, server->h_length);
	serv_addr.sin_port = htons(port);

	if (connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
		cout<< "ERROR connecting" << endl << strerror(errno) << endl;
		return false;
	}

	struct timeval tv;
   tv.tv_sec = 20;  /* 20 Secs Timeout */
   tv.tv_usec = 0;

	if(setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, (char *)&tv, sizeof(tv)) < 0) {
        printf("Time Out\n");
        return false;
    }

	send(sockfd, content, size, 0);
	close(sockfd);
	return true;
}

float PeerReceiver::GenerateRandomValue() {
	srand( (unsigned)time( NULL ) );
	return (float) (rand()/RAND_MAX);
}


void PeerReceiver::SendMessage() {
	while (!this->flag_received_previous_module) {
		sleep(1);
	}
	while (true) {
		usleep(this->sending_rate);

		unsigned char content[256];
		int fan_out = 0;
		size_t buffer_size = 1;

		mtx.lock();
		dispatch(this->eid, content, &fan_out, &buffer_size, this->fan_all_out);
		mtx.unlock();

		cout << "Buffer size: " << buffer_size << endl;

		unsigned char message_to_send[256 + 1];

		if (fan_out) {
			wrap_message(message_to_send, content, '2');

			unsigned char message_to_send_producer[256 + 1 + this->ReceiverPort.length()];
   		std::copy(this->ReceiverPort.c_str(), this->ReceiverPort.c_str() + this->ReceiverPort.length(), message_to_send_producer);
   		std::copy(message_to_send, message_to_send + 257, message_to_send_producer + this->ReceiverPort.length());

			SendRetry("127.0.0.1", this->producer_port, message_to_send_producer, strlen((char *) message_to_send_producer));
		} else {
			wrap_message(message_to_send, content, '1');

   		// send the next message
   		SendRetry("127.0.0.1", this->next_port, message_to_send, 257);
		}

		if (this->fan_all_out && buffer_size == 0) {
			this->flag_finish = 1;
			return;
		}
	}
}

void PeerReceiver::receive_messages() {
	while(true){
		newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen); // Waiting connections
		
		if (newsockfd < 0)
			std::cout << "ERROR on accept" << strerror(errno)<<"\n";

		unsigned char cmd[MAX_COMMAND_LEN];
		recv(newsockfd, cmd, MAX_COMMAND_LEN, 0);

      // message with the public key
      unsigned char message[256];
      if (unwrap_message(message, cmd) == '0') {
         cout << "Saving the previous public key!" << endl;
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
			cout << "Received order to fan all the messages out and to stop the node!" << endl;
			this->fan_all_out = 1;
		}
		
		close(newsockfd);

		if (this->flag_finish)
			return;
	}
}

void PeerReceiver::Start(){
	
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0)
		std::cout << "ERROR opening socket" << strerror(errno)<<"\n";
	
	bzero((char *) &serv_addr, sizeof(serv_addr)); // Erase data

	portno = atoi(ReceiverPort.c_str());
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(portno);

   int opt=1;
   setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

	if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
		std::cout << "ERROR on binding" << std::endl;
	listen(sockfd, 1000);
	clilen = sizeof(cli_addr);

	// thread to receive messages
   thread ReceiveMessagesJob(&PeerReceiver::receive_messages, this);
   sleep(1);

   // create private and public key and obtain the correspondent public module
   create_keys(this->eid, this->my_public_module);

   unsigned char message_to_send[256 + 1];
   wrap_message(message_to_send, this->my_public_module, '0');

   // send the public module to the next mix
	cout << "Sending the public key to the previous mix node" << endl;
   SendRetry("127.0.0.1", this->prev_port, message_to_send, 257);

   // send the public module to the producer
	cout << "Sending the public key to the consumer/producer" << endl;
   unsigned char message_to_send_producer[256 + 1 + this->ReceiverPort.length()];
   std::copy(this->ReceiverPort.c_str(), this->ReceiverPort.c_str() + this->ReceiverPort.length(), message_to_send_producer);
   std::copy(message_to_send, message_to_send + 257, message_to_send_producer + this->ReceiverPort.length());
   SendRetry("127.0.0.1", this->producer_port, message_to_send_producer, 261);

	// thread to send messages
	thread SendMessagesJob(&PeerReceiver::SendMessage, this);
	
	ReceiveMessagesJob.join();
	SendMessagesJob.join();
	close(sockfd);

	return;
}
