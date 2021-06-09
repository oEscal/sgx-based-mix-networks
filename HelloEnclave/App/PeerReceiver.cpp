#include "color.h"
#include "PeerReceiver.h"
#include <iostream>
#include <string>
#include <unistd.h>

using namespace std;


string remove_at(vector<string>&v, int n)
{
    random_shuffle( v.begin(), v.end() );
    string ans = v.back();
	v.pop_back();
	return ans;
}


PeerReceiver::PeerReceiver(std::string ReceiverName,std::string ReceiverPort, sgx_enclave_id_t *eid){
	this->ReceiverName = ReceiverName;
	this->ReceiverPort = ReceiverPort;
	this->eid = *eid;
}

void PeerReceiver::Send(string SenderName, int SenderPort, string content){
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

	
	send(sockfd, content.c_str(), MAX_COMMAND_LEN, 0);
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
				Send("localhost", 4321, "FALSE");
				continue;
			}
		}
		if(GenerateRandomValue() < this->fan_out_probability) { // Should fan-out
			// send Message
		}
		Send("localhost", 4321, remove_at(this->buffer, N));
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
   unsigned char public_module[256];
   cout <<  this->eid << endl;
   encrypt(this->eid, public_module);

	thread SendMessageJob(&PeerReceiver::SendMessage, this);

	while(true){
		newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen); // Waiting connections
		
		if (newsockfd < 0)
			std::cerr << "ERROR on accept" << strerror(errno)<<"\n";

		char cmd[MAX_COMMAND_LEN];
		recv(newsockfd, cmd, MAX_COMMAND_LEN, 0);

		this->buffer.push_back(cmd);
		cout << cmd << endl;
		
		close(newsockfd);
	}
	SendMessageJob.join();
	close(sockfd);
}
