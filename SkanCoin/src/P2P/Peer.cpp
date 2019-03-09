#include "Peer.hpp"

using namespace std;
using easywsclient::WebSocket;

/*Business logic per la gestione dei messaggi in arrivo dai peer*/
void Peer::handlePeerMessage(const string & data, int isServer){
  string nome;

  if(isServer == 1){
    nome = "Server Peer";
  }else if (isServer == 0){
    nome = "Client Peer";
  }else{
    cout << "ERRORE - HandlePeerMessage: Non è stato specificato correttamente se il tipo di socket è client o server!" << endl;
    return;
  }

  if(data.compare("") == 0){
    cout << "ERRORE - HandlePeerMessage: Il messaggio ricevuto è vuoto!" << endl;
  }


  //Parsing dell'oggetto JSON ricevuto dalla socket
  rapidjson::Document document;
  cout << endl << nome << ": Messaggio ricevuto: ";
  //cout << data;
  document.Parse(data.c_str());

  //Controllo che il messaggio ricevuto sia valido
  if(document["type"].IsNull() || Peer::getInstance().isValidType(document["type"].GetInt()) == false){
    cout << endl << "ERRORE (" << nome << "): il tipo di messaggio ricevuto non è valido" << endl;
    return;
  }

  //Mapping dell'oggetto in una nuova istanza di Message
  Message message = Message(static_cast<MessageType>(document["type"].GetInt()), "");

  /*Liste usate per la gestione di alcuni tipi di messaggi (non è possibile inizializzare
   nuove variabili all'interno dei singoli case, se non si usano le parentesi graffe)*/
  list<Block> receivedBlocks;
  vector<Transaction> receivedTransactions;
  vector<Transaction>::iterator it;
  ofstream myfile;
  string row;
  bool bAddedBlock = false;

  //Questo switch contiene la logica di gestione dei vari tipi di messaggi in arrivo
  switch (message.type) {
    //Un peer ha richiesto l'ultimo blocco, esso viene inserito nella risposta
    case QUERY_LATEST:
      cout << " - QUERY_LATEST" << endl;
          if(isServer == false){
            tempClientWs->send(responseLatestMsg(-1,0));
          }else{
            tempServerWs->send_text(responseLatestMsg(-1,0));
          }
          break;
          //Un peer ha richiesto la blockchain, essa viene inserita nella risposta
    case QUERY_ALL:
      cout << " - QUERY_ALL" << endl;
          if(isServer == false){
            tempClientWs->send(responseChainMsg());
          }else{
            tempServerWs->send_text(responseChainMsg());
          }
          break;
          //Un peer ha inviato la propria versione di blockchain
    case RESPONSE_BLOCKCHAIN:
      cout << " - RESPONSE_BLOCKCHAIN" << endl;
          if(document["data"].IsNull()){
            cout << "ERROR (" << nome << " - RESPONSE_BLOCKCHAIN): Nessun dato ricevuto" << endl;
            return;
          }
          try {
            receivedBlocks = parseBlockList(document["data"]);
            bAddedBlock = handleBlockchainResponse(receivedBlocks);
          } catch(const char* msg){
            cout << msg << endl;
            cout << "EXCEPTION (" << nome << " - RESPONSE_BLOCKCHAIN): Errore durante l'elaborazione' del messaggio!" << endl;
            return;
          }
          /*Aggiornamento dei dati relativi al tempo di mining del blocco (questo campo
           è non nullo solo se siamo in corrispondenza della diffuzione di un nuovo
            blocco che è stato minato ed aggiunto alla blockchain)*/
          if(bAddedBlock && !document["index"].IsNull() && !document["duration"].IsNull()){
            if(document["index"].GetInt() != -1){
              row = "{\"block\": " +  to_string(document["index"].GetInt()) + ", \"miningtime\": " + to_string(document["duration"].GetDouble()) + "}\n";
              ofstream myfile;
              myfile.open ("blocksminingtime.txt", ios::out | ios::app);
              if(myfile.is_open()) {
                myfile << row;
              } else {
                cout << "ERRORE (" << nome << " - RESPONSE_BLOCKCHAIN): non è stato possibile aprire il file per salvare il tempo di mining del blocco!";
              }
              myfile.close();
            }
          }
          break;
          //Un peer ha richiesto il transaction pool, esso viene inserito nella risposta
    case QUERY_TRANSACTION_POOL:
      cout << " - QUERY_TRANSACTION_POOL" << endl;
          if(!(TransactionPool::getInstance().getTransactionPool().size() == 0)){
            if(isServer == false){
              tempClientWs->send(responseTransactionPoolMsg());
            }else{
              tempServerWs->send_text(responseTransactionPoolMsg());
            }
          }
          break;

          //Un peer ha inviato la propria versione di transaction pool
    case RESPONSE_TRANSACTION_POOL:
      cout << " - RESPONSE_TRANSACTION_POOL" << endl;

          //Parsing del pool ricevuto
          if(document["data"].IsNull()){
            cout << "ERRORE (" << nome << " - RESPONSE_TRANSACTION_POOL) nessun dato ricevuto!" << endl;
            return;
          }
          try{
            receivedTransactions = parseTransactionVector(document["data"]);
          }catch(const char* msg){
            cout << msg << endl;
            cout << "ERRORE (" << nome << " - RESPONSE_TRANSACTION_POOL): errore durante il parsing del messaggio!" << endl;
            return;
          }
          /*Per ogni transazione questa viene elaborata e successivamente si
          effettua un broadcast del transaction pool aggiornato*/
          for(it = receivedTransactions.begin(); it != receivedTransactions.end(); ++it){
            try{
              BlockChain::getInstance().handleReceivedTransaction(*it);
              broadCastTransactionPool();
            }catch(const char* msg) {
              cout << msg << endl;
              cout << "ERRORE (" << nome << " - RESPONSE_TRANSACTION_POOL): Errore durante l'inserimento della transazione nel pool" << endl;
            }
          }
          break;
          /*Un peer ha prelevato delle transazioni dal proprio pool,, aggiornamento
          del file contenente le statistiche*/
    case TRANSACTION_POOL_STATS:
      cout << " - TRANSACTION_POOL_STATS" << endl;
          if(!document["data"].IsNull()){
            try{
              myfile.open ("transactionwaitingtime.txt", ios::out | ios::app);
              if(myfile.is_open()) {
                for (rapidjson::SizeType i = 0; i < document["data"].Size(); i++){
                  myfile << "{" << "\"transactionId\": " << document["data"][i]["transactionId"].GetString() <<  ", \"millisWaitTime\": " << to_string(document["data"][i]["millisWaitTime"].GetDouble()) << "}"  << "\n";
                }
                myfile.close();
              } else {
                string msg = "EXCEPTION (" + nome + " - TRANSACTION_POOL_STATS): non è stato possibile aprire il file per salvare le statistiche di attesa delle transazioni!";
                throw msg;
              }
            }catch(const char* msg) {
              cout << msg << endl;
              cout << "ERRORE (" << nome << " - TRANSACTION_POOL_STATS): Errore durante l'apertura del file per aggiornare le statistiche" << endl;
            }
          }
          break;

    default:
      cout << "ERRORE (" << nome << "): Il messaggio non ha un tipo valido!" << endl;
  }
}

/*Gestore per messaggi in arrivo dalle socket aperte dal thread client (easywsclient::WebSocket)
Questo metodo deve avere un solo parametro (const string & data), questo è il motivo per cui
 abbiamo bisogno di una variabile esterna di supporto (tempClientWs) che sia un riferimento alla socket corrente.
 */
void handleClientMessage(const string & data){
  #if DEBUG_FLAG == 1
  DEBUG_INFO("");
  #endif
  Peer::getInstance().handlePeerMessage(data, 0);
}

Message::Message(MessageType type, string data){
  #if DEBUG_FLAG == 1
  DEBUG_INFO("");
  #endif

  this->type = type;
  this->data = data;
  this->index = -1;
  this->duration = 0;
  if(data.compare("") == 0){
    this->data = "\"\"";
  }
}
Message::Message(MessageType type, string data, int index, double duration){
  #if DEBUG_FLAG == 1
  DEBUG_INFO("");
  #endif

  this->type = type;
  this->data = data;
  this->index = index;
  this->duration = duration;
  if(data.compare("") == 0){
    this->data = "\"\"";
  }
}

/*Rappresentazione in formato JSON dell'oggetto Message*/
string Message::toString(){
   return "{\"type\": " + to_string(type) + ", \"data\": " + data + ", \"index\": " + to_string(index) + ", \"duration\": " + to_string(duration) + "}";
}

/* Questa funzione esegue il polling sulla lista delle connessioni aperte dal
thread client, se ci sono nuovi messaggi ricevuti viene eseguito l'handler adatto*/
void Peer::checkReceivedMessage(){
  // #if DEBUG_FLAG == 1
  // DEBUG_INFO("");
  // #endif

  /*Flag per segnalare che ci sono socket che sono state chiuse, questo viene conservato
  perchè non è possibile rimuovere la socket nello stesso loop del polling, infatti
  abbiamo bisogno di un riferimento diretto in tale loop, mentre è necessario utilizzare
  un iteratore per rimuovere l'elemento dal vettore */
  bool flag = false;
  connectionsMtx.lock();
  //Controllo dei messaggi in arrivo in ogni socket
  for(auto ws: openedConnections){
    //Controlla se la socket è stat chiusa
    if(ws->getReadyState() != WebSocket::CLOSED) {
      /*Assegno il riferimento corrente alla variabile di supporto, in modo da
        poterla utilizare all'interno dell'handler (infatti come spiegato sopra
          non è possibile passare ad esso un secondo parametro)*/
      tempClientWs = ws;

      /*Se ci sono messaggi in arrivo, questo metodo riempie il buffer che verrà
       utilizzato dall'handler con i dati ricevuti /*/
      ws->poll();
      //Esecuzione dell'handler per il messaggio ricevuto
      ws->dispatch(handleClientMessage);
    }else{

      //La socket è stat chiusa, dunque setto il relativo flag
      flag = true;
    }
  }
  //Rimozione delle socket chiuse
  if(flag == true){
    clearClosedWs();
  }

  connectionsMtx.unlock();
}

/*Questo metodo controlla il vettore di socket (client), eliminando quelle
che sono state chiuse*/
void Peer::clearClosedWs(){
  #if DEBUG_FLAG == 1
  DEBUG_INFO("");
  #endif

  vector<easywsclient::WebSocket::pointer> temp; //Creo un vettore di appoggio
  for(auto ws: openedConnections){
    //Inserisco nel nuovo vettore solo le socket aperte
    if(ws->getReadyState() != WebSocket::CLOSED) {
      temp.push_back(ws);
    }
  }
  //Svuoto il vecchio vettore e facendo una sola allocazione inserisco gli elementi presenti nel vettore di appoggio
  openedConnections.clear();
  openedConnections.reserve(openedConnections.size() + temp.size());
  openedConnections.insert(openedConnections.end(), temp.begin(), temp.end());
}

/*Dato l'url di un server P2P viene aperta una nuova connessione verso di esso dal thread client*/
void Peer::connectToPeers(std::string peer){
  #if DEBUG_FLAG == 1
  DEBUG_INFO("");
  #endif

  cout << "Client Peer: Aggiunta di { " << peer << " } alla lista dei peer..." << endl;
  WebSocket::pointer ws;
  ws = WebSocket::from_url(peer);

  if(!ws) throw "EXCEPTION (ConnectToPeers): Errore durante la connessione al peer!";

  //Lock del mutex sulle liste di connessioni per l'inserimento della nuova connessione
  connectionsMtx.lock();
  ws->send(queryChainLengthMsg());
  openedConnections.push_back(ws);
  connectionsMtx.unlock();
}

/*Ritorna il numero d peer (si considerano sia le connessioni aperte dal thread client che quelle ricevute dal thread server)*/
int Peer::countPeers(){
  #if DEBUG_FLAG == 1
  DEBUG_INFO("");
  #endif

  connectionsMtx.lock();
  int count = receivedConnections.size() + openedConnections.size();
  connectionsMtx.unlock();
  return count;
}

/* Business logic per un messaggio di tipo RESPONSE_BLOCKCHAIN, questa funzione
è chiamata nell'handler dei messaggi in arrivo. Ritorna true se ha aggiunto un
singolo blocco nella blockchain, altrimenti false (anche in caso di replace) */
bool Peer::handleBlockchainResponse(list<Block> receivedBlocks) {
  #if DEBUG_FLAG == 1
  DEBUG_INFO("");
  #endif

  bool ret = false;
  /*Valuto eventuali aggiornamenti locali in base ai blocchi ricevuti dagli altri peer
  NOTE: in replacechain viene controllato che la difficoltà dei nuovi blocchi
  sia superiore*/
  if (receivedBlocks.size() == 0) {
      cout << "ERRORE (handleBlockchainResponse): La blockchain ricevuta ha lunghezza 0!" << endl;
      return ret;
  }
  Block latestBlockReceived = receivedBlocks.back();
  if (BlockChain::getInstance().isValidBlockStructure(latestBlockReceived) == false) {
      cout << "ERRORE (handleBlockchainResponse): la struttura del blocco non è valida" << endl;
      return ret;
  }
  Block latestBlockHeld = BlockChain::getInstance().getLatestBlock();

  if(receivedBlocks.size() == 1) {
    if (latestBlockReceived.index == 0) {
      //Confronto fra due blocchi di genesi, verrà selezionato quello con timestamp minore
      BlockChain::getInstance().replaceChain(receivedBlocks);
    } else {
      if (latestBlockHeld.hash == latestBlockReceived.previousHash) {
        //Il nuovo blocco è l'unico da aggiungere in quanto successivo dell'ultimo della blockchian locale
        if (BlockChain::getInstance().addBlockToChain(latestBlockReceived)) {
          broadcast(responseLatestMsg(-1,0));
          //In precedenza potrei aver scartato transazioni perchè la mia blockchain non era aggiornata,
          // richiedo la transaction pool per verificarle di nuovo
          broadcastTxPoolQuery();
          ret = true;
        }
      } else {
        //Ci sono più versioni non congruenti o la blockchain locale è indietro di più di un blocco,
        //richiedo in broadcast le version di blockchain dei vari peer
        broadcast(queryAllMsg());
      }
    }
  } else {
    // E' stata ricevuta una nuova versione di blockchain (più blocchi), richiamo replacechain per confrontarla
    // con quella locale ed eventualmente sostituirla
    BlockChain::getInstance().replaceChain(receivedBlocks);
  }

  return ret;
}

/*Inizializzazione del server P2P*/
void Peer::initP2PServer(int port){
  #if DEBUG_FLAG == 1
  DEBUG_INFO("");
  #endif

  crow::SimpleApp app;
  CROW_ROUTE(app, "/").websocket()
    //Gesitione della ricezione di nuove connessioni
    .onopen([&](crow::websocket::connection& connection){
      CROW_LOG_INFO << "Server Peer: ricevuta nuova connessione";
      connectionsMtx.lock();
      receivedConnections.insert(&connection);
      connection.send_text(queryChainLengthMsg());
      connectionsMtx.unlock();

    })
    //Gestione della chiusura di una socket
    .onclose([&](crow::websocket::connection& connection, const std::string& reason){
      CROW_LOG_INFO << "Server Peer: Una websocket è stata chiusa: " << reason;
      connectionsMtx.lock();
      receivedConnections.erase(&connection);
      connectionsMtx.unlock();
    })
    //Gestione della ricezione di un messaggio
    .onmessage([&](crow::websocket::connection& connection, const std::string& data, bool){
      connectionsMtx.lock();
      tempServerWs = &connection;
      handlePeerMessage(data, 1);
      connectionsMtx.unlock();
    });

  cout << "Starting P2PServer on port " << port << "..." << endl;
  app.port(port).run();
}

/*Avvio del polling del client sulle socket aperte*/
void Peer::startClientPoll(){
  #if DEBUG_FLAG == 1
  DEBUG_INFO("");
  #endif

  cout << "Starting P2P client...." << endl;
  while(true){
    /*Il polling avviene con una cadenza di un secondo, non ci sono particolari
    vincoli temporali quindi non è necessario sovraccaricare la rete o il nodo effettuando un polling moto frequente*/
    checkReceivedMessage();
  }
}
/*Controllo della validità del tipo di messaggio (se appartiene all'enumeratore)*/
bool Peer::isValidType(int type){
  #if DEBUG_FLAG == 1
  DEBUG_INFO("");
  #endif

  MessageType receivedType = static_cast<MessageType>(type);
  for ( int val = QUERY_LATEST; val != TRANSACTION_POOL_STATS+1; val++ ){
    if(receivedType == val)return true;
  }
  return false;
}

/*Questo metodo effettua un broadcast di un certo messaggio, per fare ciò lo invia
 su tutte le socket aperte dal thread client e su tutte quelle aperte da altri
 client verso il thread server*/
void Peer::broadcast(string message){
  #if DEBUG_FLAG == 1
  DEBUG_INFO("");
  #endif

  //Broadcast sulle socket aperte dal thread client
  for(auto ws: openedConnections){
    if(ws->getReadyState() != WebSocket::CLOSED) {
      ws->send(message);
    }
  }
  //Broadcast sulle socket aperte da altri client verso il thread server
  for(auto ws: receivedConnections){
    ws->send_text(message);
  }
}

/*Metodi per il broadcast dei messaggi*/
void Peer::broadCastTransactionPool(){
  #if DEBUG_FLAG == 1
  DEBUG_INFO("");
  #endif

  broadcast(responseTransactionPoolMsg());
}

void Peer::broadcastTxPoolQuery(){
#if DEBUG_FLAG == 1
  DEBUG_INFO("");
#endif

  broadcast(queryTransactionPoolMsg());
}

void Peer::broadcastLatest(int index, double time){
  #if DEBUG_FLAG == 1
  DEBUG_INFO("");
  #endif

  broadcast(responseLatestMsg(index, time));
}

void Peer::broadcastTxPoolStat(vector<string> stats){
  #if DEBUG_FLAG == 1
  DEBUG_INFO("");
  #endif

  broadcast(txPoolStatsMessage(stats));
}


/*metodi per la costruzione dei messaggi da inviare sulle socket*/
string Peer::queryChainLengthMsg(){
  return Message(QUERY_LATEST, "").toString();
}
string Peer::queryAllMsg(){
  return Message(QUERY_ALL, "").toString();
}
string Peer::queryTransactionPoolMsg(){
  return Message(QUERY_TRANSACTION_POOL, "").toString();
}
string Peer::responseChainMsg(){
  return Message(RESPONSE_BLOCKCHAIN, BlockChain::getInstance().toString()).toString();
}
string Peer::responseLatestMsg(int index, double duration){
  return Message(RESPONSE_BLOCKCHAIN, "[" + BlockChain::getInstance().getLatestBlock().toString() + "]",  index, duration).toString();
}
string Peer::responseTransactionPoolMsg(){
  return Message(RESPONSE_TRANSACTION_POOL, TransactionPool::getInstance().toString()).toString();
}
string Peer::txPoolStatsMessage(vector<string> stats){
    string msg = "[";

    vector<string>::iterator it;
    for(it = stats.begin(); it != stats.end(); ++it){
        if(it != stats.begin()){
          msg = msg + ",";
        }
        msg = msg + *it;
    }
    msg = msg + "]";
    return Message(TRANSACTION_POOL_STATS, msg).toString();
}
