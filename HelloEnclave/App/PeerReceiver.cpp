#include "color.h"
#include "PeerReceiver.h"
#include <iostream>
#include <string>
#include <unistd.h>

using namespace std;


void create_message_str(unsigned char *message, int message_type) {
   cout << sizeof message << endl;
   for(int a = 0; a < 256; ++a)
{
int p = *(message + a);// or int p = data[a];
cout << p;
}
cout << "\n";
}


PeerReceiver::PeerReceiver(std::string ReceiverName,std::string ReceiverPort, sgx_enclave_id_t *eid){
	this->ReceiverName = ReceiverName;
	this->ReceiverPort = ReceiverPort;
	this->eid = *eid;
}

void PeerReceiver::Send(string SenderName, int SenderPort, void *content){
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
	serv_addr.sin_port = htons(SenderPort);

	if (connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
		cerr<< "ERROR connecting" << strerror(errno) << "\n";

	
	send(sockfd, content, MAX_COMMAND_LEN, 0);
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

void PeerReceiver::Start(){
	
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0)
		std::cerr << "ERROR opening socket" << strerror(errno)<<"\n";
	
	bzero((char *) &serv_addr, sizeof(serv_addr)); // Erase data

	portno = atoi(ReceiverPort.c_str());
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(portno);

	if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
		std::cerr << "ERROR on binding" << std::endl;
	listen(sockfd, 10);
	clilen = sizeof(cli_addr);

   // create private and public key and obtain the correspondent public module
   create_keys(this->eid, this->my_public_module);

   set_public_key(this->eid, this->my_public_module);

   // send the public module to the next mix
   Send("localhost", 4321, this->my_public_module);
   create_message_str(this->my_public_module, 0);

	// thread SendMessageJob(&PeerReceiver::SendMessage, this);

	while(true){
		newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen); // Waiting connections
		
		if (newsockfd < 0)
			std::cerr << "ERROR on accept" << strerror(errno)<<"\n";

		unsigned char cmd[MAX_COMMAND_LEN];
		recv(newsockfd, cmd, MAX_COMMAND_LEN, 0);

		// cout << cmd << endl;
      create_message_str(cmd, 0);
      cout << "Received!" << endl;
      import_message(this->eid, cmd);
		
		close(newsockfd);
	}
	// SendMessageJob.join();
	close(sockfd);
}
