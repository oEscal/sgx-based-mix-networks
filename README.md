# SGX based mix-networks
Hidden anonymization with Intel SGX based mixes

## How to execute

 - The first requirement needed to run our solution is to have installed the SGX drivers and SDK:
    ```bash
    $ sudo apt install build-essential ocaml automake autoconf libtool wget python3 libssl-dev dkms
    $ wget https://download.01.org/intel-sgx/latest/linux-latest/distro/ubuntu20.04-server/sgx_linux_x64_driver_1.41.bin; sudo bash sgx_linux_x64_driver_1.41.bin
    
    $ sudo apt install libssl-dev libcurl4-openssl-dev libprotobuf-dev
    $ echo 'deb [arch=amd64] https://download.01.org/intel-sgx/sgx_repo/ubuntu focal main' | sudo tee /etc/apt/    sources.list.d/intel-sgx.list
    $ wget -qO - https://download.01.org/intel-sgx/sgx_repo/ubuntu/intel-sgx-deb.key | sudo apt-key add -
    $ sudo apt update
    $ sudo apt install libsgx-launch libsgx-urts
    $ sudo apt install libsgx-epid libsgx-urts
    
    $ wget https://download.01.org/intel-sgx/latest/linux-latest/distro/ubuntu20.04-server/sgx_linux_x64_sdk_2.13.103.1.bin; sudo bash sgx_linux_x64_sdk_2.13.103.1.bin             # answer no and choose /opt/intel
    # then, copy the contents of the file /opt/intel/sgxsdk/environment to your .bashrc
    
    $ wget https://download.01.org/intel-sgx/latest/linux-latest/as.ld.objdump.gold.r3.tar.gz
    $ tar xzvf as.ld.objdump.gold.r3.tar.gz external/toolset/ubuntu20.04
    $ sudo cp -v external/toolset/ubuntu20.04/* /usr/local/bin/
    ```

 - After that, you should change your terminal directory to the directory `mix_solution/` of this repository.

 - Then, you have also to set the Python virtual environment and install the needed requirements:
    ```bash
    $ cd consumer_producer/
    $ virtualenv venv
    $ source venv/bin/activate

    $ pip install -r requirements.txt
    ```

 - Finally, open two terminals, one in the directory `mix_solution/` and the other on the directory `mix_solution/consumer_producer/`, and run:
    ```bash
    # on the mix_solution/consumer_producer/, first enter the virtual environment (source venv/bin/activate)
    $ python3 server.py <minimum_number_of_mixes> <number_of_messages>                 # for example, "python3 server.py 100 10000" will create a consumer/producer that will wait for the public keys of 100 different mixes before starting to send the messages to the network, and will send a total of 10000 messages

    # on the mix_solution/
    $ make
    $ ./run_multiple.sh <number_of_mixes>                                                   # for example, "./run_multiple 100" will run create a network of 100 mixes
    ```

    - Note that you must run the Python server first and just after it is running is when you can run the script that will create the mix network.

    - You can check the mixes logs on the directory `logs/`.

 - You can also run each mix (node) for yourself without the need of the script `run_multiple.sh`. For that, you just have to run:
    ```bash
    $ make
    $ ./app <mix_port> <prev_mix_port> <next_mix_port> <consumer_producer_port>        # where <mix_port> is the port you want your mix to run on, the <prev_mix_port> is the port where is running the previous mix, the <next_mix_port> is the port where is running the next mix and the <consumer_producer_port> is where is running the Python consumer/producer
    ```

