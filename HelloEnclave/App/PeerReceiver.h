/*
 * Peer.h
 *
 *  Created on: 24-Apr-2018
 *      Author: pinaki
 */

#ifndef PEERRECEIVER_H_
#define PEERRECEIVER_H_
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <regex.h>
#include <thread>
#include <iostream>
#include <vector>
#include <thread>
#include <unistd.h>
#include <memory>
#include <exception>
#include <algorithm>

#include "Enclave_u.h"

class PeerReceiver{

private:
	std::string ReceiverName ="";
	std::string ReceiverPort ="";
	std::vector<std::string> buffer;
	sgx_enclave_id_t eid;
	int sending_rate = 1;
	float fan_out_probability = 0.01;
	int watermark = 10;
	int sockfd=0, newsockfd=0, portno=0;
	socklen_t clilen;
	struct sockaddr_in serv_addr, cli_addr;
	void Send(std::string, int, std::string);
	void SendMessage();
	float GenerateRandomValue();

public:
	PeerReceiver(std::string ,std::string, sgx_enclave_id_t*);
	void Start();
};


#endif /* PEERRECEIVER_H_ */
