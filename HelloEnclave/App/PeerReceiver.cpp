#include "color.h"
#include "PeerReceiver.h"
#include <iostream>
#include <string>
#include <unistd.h>

using namespace std;


void wrap_message(unsigned char *message_to_send, unsigned char *content, char message_type) {
   message_to_send[0] = message_type;
   std::copy(content, content + 256, message_to_send + 1);
}

char unwrap_message(unsigned char *content, unsigned char *message_received) {
   std::copy(message_received + 1, message_received + MAX_COMMAND_LEN, content);
   return message_received[0];
}


PeerReceiver::PeerReceiver(std::string ReceiverName, std::string ReceiverPort, int next_port, int producer_port, sgx_enclave_id_t *eid){
	this->ReceiverName = ReceiverName;
	this->ReceiverPort = ReceiverPort;
   this->next_port = next_port;
   this->producer_port = producer_port;
	this->eid = *eid;
}

void PeerReceiver::Send(string SenderName, int port, void *content, int size){
	int sockfd=0,portno=0;
	struct hostent *server;

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0)
		cerr<< "ERROR opening socket\n";

	server = gethostbyname(SenderName.c_str());
	if (server == NULL) {
		cerr<< "ERROR, no such host\n";
		exit(0);
	}

	bzero((char *) &serv_addr, sizeof(serv_addr)); // Erase data
	serv_addr.sin_family = AF_INET;
	bcopy((char *) server->h_addr,(char *)&serv_addr.sin_addr.s_addr, server->h_length);
	serv_addr.sin_port = htons(port);

	if (connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
		cerr<< "ERROR connecting" << strerror(errno) << "\n";

	
	send(sockfd, content, size, 0);
}

float PeerReceiver::GenerateRandomValue() {
	srand( (unsigned)time( NULL ) );
	return (float) (rand()/RAND_MAX);
}


void PeerReceiver::SendMessage() {
	while (true) {
		sleep(this->sending_rate);
		double H = this->watermark;
		double N = this->buffer.size();

		if (H > N){
			float blank_probability = (H - N) / H;

			if(GenerateRandomValue() < blank_probability) {
				cout << "FALSE" << endl;
				// Send("localhost", 4321, "FALSE");
				continue;
			}
		}
		if(GenerateRandomValue() < this->fan_out_probability) { // Should fan-out
			// send Message
		}
		// Send("localhost", 4321, remove_at(this->buffer, N));
		cout <<  "TRUE" << endl;
	}
}

void PeerReceiver::receive_messages() {
	while(true){
		newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen); // Waiting connections
		
		if (newsockfd < 0)
			std::cerr << "ERROR on accept" << strerror(errno)<<"\n";

		unsigned char cmd[MAX_COMMAND_LEN];
		recv(newsockfd, cmd, MAX_COMMAND_LEN, 0);

      cout << "Received!" << endl;

      // message with the public key
      unsigned char message[256];
      if (unwrap_message(message, cmd) == '0') {
         cout << "Saving the previous public key!" << endl;
         set_public_key(this->eid, message);
      } 

      if (unwrap_message(message, cmd) == '1') {
         cout << "Saving the message to enclave!" << endl;
         import_message(this->eid, message);
      }
		
		close(newsockfd);
	}
}

void PeerReceiver::Start(){
	
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0)
		std::cerr << "ERROR opening socket" << strerror(errno)<<"\n";
	
	bzero((char *) &serv_addr, sizeof(serv_addr)); // Erase data

	portno = atoi(ReceiverPort.c_str());
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(portno);

   int opt=1;
   setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

	if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
		std::cerr << "ERROR on binding" << std::endl;
	listen(sockfd, 10);
	clilen = sizeof(cli_addr);

   thread ReceiveMessagesJob(&PeerReceiver::receive_messages, this);
   sleep(5);

   // create private and public key and obtain the correspondent public module
   create_keys(this->eid, this->my_public_module);

   unsigned char message_to_send[256 + 1];
   wrap_message(message_to_send, this->my_public_module, '0');

   // send the public module to the next mix
   Send("localhost", this->next_port, message_to_send, 257);

   // send the public module to the producer
   unsigned char message_to_send_producer[256 + 1 + this->ReceiverPort.length()];
   std::copy(this->ReceiverPort.c_str(), this->ReceiverPort.c_str() + this->ReceiverPort.length(), message_to_send_producer);
   std::copy(message_to_send, message_to_send + 257, message_to_send_producer + this->ReceiverPort.length());
   Send("localhost", this->producer_port, message_to_send_producer, 261);

	ReceiveMessagesJob.join();
	close(sockfd);
}
