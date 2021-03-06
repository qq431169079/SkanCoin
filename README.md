# SkanCoin
A simple criptocurrency written in C++


## Table of Contents
- [References](#References)
- [Screenshots](#screenshots)
- [Requirements](#Requirements)
- [Dependencies](#Dependencies)
- [Running for test](#Running-for-test)
- [Running with virtual box](#Running-with-virtual-box)
- [Future developments](#Future-developments)


## References
References available (italian only) at:
- doc/Relazione_Scavo_Messina.docx



## Screenshots
- **Web Application**
![screenshot](screenshots/1.png)

- **Diagnostic client**
![screenshot](screenshots/2.png)



## Requirements
The application has the following requirements (needed to run the project)
Note: this app was successfully tested on Ubuntu 18.04 and ArchLinux. The following links refer to Ubuntu 18.04. If using a different Operative System, it is necessary to search and download the proper software. Because of C++ App dependencies it iss not possible to run this project on Windows or MacOS.
- **C++ Application**:
  1. Cmake. Open a terminal and write:
		```
		sudo apt install cmake
		```
  2. Crow library requirements. Open a terminal and write:
		```
		sudo apt-get install build-essential libtcmalloc-minimal4 && sudo ln -s /usr/lib/libtcmalloc_minimal.so.4 /usr/lib/libtcmalloc_minimal.so
		sudo apt-get install libboost-all-dev
		cmake -DOPENSSL_ROOT_DIR=/usr/local/ssl - DOPENSSL_LIBRARIES=/usr/local/ssl/lib
		```
  3. Clang 7.0.1 instead of g++ for Crow library compilation. Open a terminal and write:
		```
		sudo apt update
		sudo apt upgrade
		sudo apt install build-essential xz-utils curl
		curl -SL http://releases.llvm.org/7.0.1/clang+llvm-7.0.1-x86_64-linux-gnu-ubuntu-18.04.tar.xz | tar -xJC .
		mv clang+llvm-7.0.1-x86_64-linux-gnu-ubuntu-18.04 clang_7.0.1
		sudo mv clang_7.0.1 /usr/local
		export PATH=/usr/local/clang_7.0.1/bin:$PATH
		export LD_LIBRARY_PATH=/usr/local/clang_7.0.1/lib:$LD_LIBRARY_PATH
		```

- **Web Application**:
  1. Nodejs and npm. Open a terminal and write:
		```
		sudo apt update
		sudo apt install nodejs
		sudo apt install npm
		```
  2. Http server. Open a terminal and write:
		```
		sudo npm install -g http-server -o
		```
- **R Application**:
  1. R language. Open a terminal and write:
  		```
		sudo apt-get install -y software-properties-common
		```
     Then follow this link: https://www.digitalocean.com/community/tutorials/how-to-install-r-on-ubuntu-18-04-quickstart
  2. R packages: shiny, ggplot2 and jsonlite.  Open a terminal and write:
		```
		sudo apt install libcurl4-openssl-dev
		sudo -i R
		install.packages("shiny")
		install.packages("ggplot2")
		install.packages("jsonlite")
		install.packages('curl')
		```


## Dependencies
The application has the following dependencies (libraries and packages)
- **C++ Application**:
  1. Crow https://github.com/ipkn/crow. Used for HTTP server and P2P server (WebSocket server).
  2. Easywsclient https://github.com/dhbaird/easywsclient. Used for P2P client (WebSocket client).
  3. Rapidjson https://github.com/Tencent/rapidjson. Used to parse json in POST HTTP request of the HTTP server and to parse json in Peer's WebSocket message exchange.
  4. easy-ecc https://github.com/esxgx/easy-ecc. Used for asymmetric encryption.
  5. PicoSHA2 https://github.com/okdshin/PicoSHA2. Used for SHA256 encryption.
- **Web Application**:
  1. AngularJS 1.6.9 https://angularjs.org/.
  2. Bootstrap 4.0.0 https://getbootstrap.com/docs/4.0/getting-started/download/.
  3. FontAwesome 5.6.1 https://fontawesome.com/.
- **R Application**:
  1. Shiny https://shiny.rstudio.com/. Used to create a web application with R.
  2. Ggplot2 https://ggplot2.tidyverse.org/index.html. Used to plot graphs.
  3. Jsonlite https://cran.r-project.org/web/packages/jsonlite/index.html. Used to make HTTP request in R.



## Running for test
After having installed all the requirements it is possible to clone this project and run it by following the next steps. It is possible to avoid installing all the requirements; if Virtual Box is already installed the next steps can be skipped, going directly to the "Running with virtual box" section.

#### 1 - Clone the project from master branch
Open a terminal and write
```
git clone https://github.com/Taletex/SkanCoin.git
```

#### 2 - Build and run the C++ project
Open a terminal in the folder where the repository is cloned and write
```
cd SkanCoin/SkanCoin
mkdir build
cd build
export CXX=/usr/local/clang_7.0.1/bin/clang++
cmake -DDEBUG_FLAG=0 ..
make
./skancoin
```
> To enable debug informations on the terminal where the C++ application is running, write "-DDEBUG_FLAG=1" instead of "-DDEBUG_FLAG=0". Every time a function is called, this feature prints on stdout the called function name, the file name and the row number where it is placed.

> The http server port and the p2p server port can be changed before running the application by writing them in the command "./skancoin" separated by a space. For example, writing "./skancoin 3002 6002" the http server will run on port 3002, while the p2p server will run on port 6002.

> Multiple instances of the C++ application can be run in the same PC by following the previous commands, but using each time different folders (e.g. build2, build3, ...) and different http server and p2p server port numbers (e.g. 3002 - 6002, 3003 - 6003, ...). It is important to use different folders because in each build folder the backend information are stocked (such as public key, stats informations, ...).

#### 3 - Run the Web Application
Open a new terminal in the folder where the repository is cloned and write
```
cd SkanCoin
cd WebApp
http-server -o
```
A new browser tab at the address where the web application is running will open.
> Note: it is necessary to know the http server's IP address and port to use the web application. The first step to use the web application is inserting those informations. This also allows the user to connect it to other active HTTP servers of the P2P network.

#### 4 - Run the Diagnostic Client
Open a new terminal and write
```
sudo -i R
library(shiny)
runApp("/home/taletex/Projects/SkanCoin/DiagnosticClient/app")
```
> Note: the path in the "runApp("")" function needs to be changed according to the path of the folder "app" located inside the "DiagnosticClient" folder. At the end of the execution of these commands the address where the Diagnostic Client web application is in running will appear. Copy and paste this address into a browser page

> Note: it is necessary to know the http server's IP address and port to use the R application. The first step to use the R application is inserting those informations. This also allows the user to connect it to other active HTTP servers of the P2P network.


## Running with virtual box
We have created a virtual box image with all requirements already installed. So, if you have virtual box installed on your PC you can follow the next steps to run the project.

#### 1 - Download ubuntu 18.04 skancoin image
Follow this link and download the image of ubuntu 18.04 with preinstalled requirements to run skancoin: 
- https://drive.google.com/open?id=1vQUMoFhjfCTkuQCu_JkmkKqw7buygNYl

#### 2 - Run ubuntu 18.04 skancoin image
1. Open Virtual Box and click on File -> Import virtual application. 
2. Select the previous downloaded image.
3. Click on Next.
4. Click on Import.
5. Run the VM.
> Note: the password of the main user is "apl20182019"

#### 4 - Update skancoin repo and run the system
Once the ubuntu 18.04 skancoin image is running, open a new terminal and write
```
cd Project/SkanCoin
git pull
```
Now follow the steps of the "Running for test" section (excluding the clone repository step) to run the system. 
> Note that the project is located in Home/Projects, so when you open a new terminal in every step of the "Running for test" section you have to write "cd Projects/SkanCoin" to move in the SkanCoin root folder.


## Future developments
- Dockerization
- Authentication
- Multiple wallet management from a single peer
- Creating a Join server mechanism to allow peers remember their open connections (alternately, saving connected peers URLs into file and get those URLs when the Peer start in order to ripristinate old connections). This is useful only in case of crash.
- Updating stats files for query2 and query3 also when a node's whole blockchain is replaced.
- Implementing full broadcasting of statistics update. At the moment, if there are 3 nodes, A, B and C where: A is connected to B and C, while B is connected only with A and C is connected only with A, if A updates statistics then both B and C will take the updates, but if B or C update statistics, then only A will take the update!)
- Persistent storing of blockchain
- Changing websocket client library (easywsclient) because of its limitations. We have encountered a lot of problems which led us to limit the amount of data exchanged between client websocket and server websocket (the client websocket crashes if it has to exchange to much data). This is why the webapp only allows the user to create blocks with a maximum of 3 destinations and mining block with only 2 transactions from the transaction pool.
