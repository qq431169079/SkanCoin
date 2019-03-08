#include "Blockchain.hpp"

using namespace std;

Block::Block (int index, string hash, string previousHash, long timestamp, vector<Transaction> data, int difficulty, int nonce) {
  #if DEBUG_FLAG == 1
  DEBUG_INFO("");
  #endif

  this->index = index;
  this->previousHash = previousHash;
  this->timestamp = timestamp;
  this->data.swap(data);
  this->hash = hash;
  this->difficulty = difficulty;
  this->nonce = nonce;
}

string Block::toString(){
  string ret = "{\"index\": " + to_string(index) + ", \"previousHash\": \"" + previousHash  + "\", \"timestamp\": " + to_string(timestamp) + ", \"hash\": \"" + hash + "\", \"difficulty\": " + to_string(difficulty)  + ", \"nonce\": " + to_string(nonce)   + ", \"data\": [";
  vector<Transaction>::iterator it;
  for(it = data.begin(); it != data.end(); ++it){
    if(it != data.begin()){
      ret = ret + ", ";
    }
    ret = ret + it->toString();
  }
  ret = ret + "]}";
  return ret;
}

bool Block::isEqual(Block other){
  #if DEBUG_FLAG == 1
  DEBUG_INFO("");
  #endif

  return this->toString() == other.toString();
}

BlockChain::BlockChain(){
  #if DEBUG_FLAG == 1
  DEBUG_INFO("");
  #endif

  Block genesisBlock = getGenesisBlock();
  blockchain = {genesisBlock};
  try{
    this->unspentTxOuts = processTransactions(blockchain.front().data, unspentTxOuts, 0);
    saveBlockchainStats();
    initStatFiles();
  }catch(const char* msg){
    cout << msg << endl << endl;
    throw "EXCEPTION (BlockChain): Creazione della blockchain fallita!";
  }
}

/*Generazione blocco di genesi (il primo blocco della blockchain)*/
Block BlockChain::getGenesisBlock(){
  #if DEBUG_FLAG == 1
  DEBUG_INFO("");
  #endif

  vector<TxIn> txInsVector = {TxIn("","",0)};
  vector<TxOut> txOutsVector = {TxOut(getPublicFromWallet(), COINBASE_AMOUNT)};
  Transaction genesisTransaction("", txInsVector, txOutsVector);
  genesisTransaction.id = getTransactionId(genesisTransaction);
  vector<Transaction> transactionsVector = {genesisTransaction};
  Block ret(0, "", "", time(nullptr), transactionsVector, 0, 0);
  ret.hash = calculateHash(ret.index, ret.previousHash, ret.timestamp , ret.data, ret.difficulty, ret.nonce);
  return ret;
}

/*Rappresentazione in stringa della blockchain*/
string BlockChain::toString(){
  string ret = "[";
  list<Block>::iterator it;
  for(it = blockchain.begin(); it != blockchain.end(); ++it){
    if(it != blockchain.begin()){
      ret = ret + ", ";
    }
    ret = ret + it->toString();
  }
  ret = ret + "]";
  return ret;
}

/*Tabella di conversione dei caratteri esadecimali in byte,
usata per verificare la correttezza (difficoltà) degli hash*/
string BlockChain::hexToBinaryLookup(char c){
  #if DEBUG_FLAG == 1
  DEBUG_INFO("");
  #endif

  string ret= "";
  switch(c){
      case '0': ret = "0000";
      break;
      case '1': ret = "0001";
      break;
      case '2': ret = "0010";
      break;
      case '3': ret = "0011";
      break;
      case '4': ret = "0100";
      break;
      case '5': ret = "0101";
      break;
      case '6': ret = "0110";
      break;
      case '7': ret = "0111";
      break;
      case '8': ret = "1000";
      break;
      case '9': ret = "1001";
      break;
      case 'a': ret = "1010";
      break;
      case 'b': ret = "1011";
      break;
      case 'c': ret = "1100";
      break;
      case 'd': ret = "1101";
      break;
      case 'e': ret = "1110";
      break;
      case 'f': ret = "1111";
      break;
      default: ret = "";
  }
  return ret;
}

/*Converte una stringa esadecimale in una sequenza binaria, in modo da poter
 verificare il numero di zeri iniziali (difficoltà) durante la validazione del blocco*/
string BlockChain::hexToBinary(string s){
  #if DEBUG_FLAG == 1
  DEBUG_INFO("");
  #endif

  string ret = "";
  for(char& c : s) {
    ret = ret + hexToBinaryLookup(c);
  }
  return ret;
}

/*Ritorna la BlockChain*/
list<Block> BlockChain::getBlockchain(){
  #if DEBUG_FLAG == 1
  DEBUG_INFO("");
  #endif

  return blockchain;
}

/*Ritorna il vettore di output non spesi*/
vector<UnspentTxOut> BlockChain::getUnspentTxOuts(){
  #if DEBUG_FLAG == 1
  DEBUG_INFO("");
  #endif

  return unspentTxOuts;
}

/*Reimposta il vettore di output non spesi*/
void BlockChain::setUnspentTxOuts(vector<UnspentTxOut> newUnspentTxOuts){
  #if DEBUG_FLAG == 1
  DEBUG_INFO("");
  #endif

  vector<UnspentTxOut>::iterator it;
  for(it = newUnspentTxOuts.begin(); it != newUnspentTxOuts.end(); ++it){
    cout << it->toString();
  }
  unspentTxOuts.swap(newUnspentTxOuts);
}

/*Ritorna l'ultimo blocco della BlockChain*/
Block BlockChain::getLatestBlock(){
  #if DEBUG_FLAG == 1
  DEBUG_INFO("");
  #endif

  return blockchain.back();
}

/*Ritorna un blocco dato il suo hash*/
Block BlockChain::getBlockFromHash(string hash){
  #if DEBUG_FLAG == 1
  DEBUG_INFO("");
  #endif

  list<Block>::iterator it;
  for(it = blockchain.begin(); it != blockchain.end(); ++it){
    if(it->hash == hash){
      return *it;
    }
  }
  cout << endl;
  throw "EXCEPTION (getBlockFromHash): Blocco non trovato!";
}

/*Calcola la nuova difficoltà per i blocchi*/
int BlockChain::getAdjustedDifficulty(Block latestBlock, list<Block> aBlockchain){
  #if DEBUG_FLAG == 1
  DEBUG_INFO("");
  #endif

  list<Block>::iterator it = aBlockchain.begin();
  //Avanza l'iteratore fino al blocco in cui è cambiata l'ultima volta la difficoltà
  advance(it, (blockchain.size() - DIFFICULTY_ADJUSTMENT_INTERVAL));
  Block prevAdjustmentBlock = *it;
  //tempo atteso tra un aumento di diffoltà e l'altro
  long timeExpected = long(BLOCK_GENERATION_INTERVAL * DIFFICULTY_ADJUSTMENT_INTERVAL);
  //tempo trascorso dall'ultimo aumento di difficoltà
  long timeTaken = latestBlock.timestamp - prevAdjustmentBlock.timestamp;

  //Si cerca sempre di fare in modo che i blocchi vengano aggiunti regolarmente con un intervallo pari a timeExpected
  if (timeTaken < timeExpected / 2) {
      return prevAdjustmentBlock.difficulty + 1;
  } else if (timeTaken > timeExpected * 2) {
      return prevAdjustmentBlock.difficulty - 1;
  } else {
      return prevAdjustmentBlock.difficulty;
  }
}

/*Ritorna la difficoltà per il prossimo blocco*/
int BlockChain::getDifficulty(list<Block> aBlockchain){
  #if DEBUG_FLAG == 1
  DEBUG_INFO("");
  #endif

  Block latestBlock = aBlockchain.back();
  if ((latestBlock.index % DIFFICULTY_ADJUSTMENT_INTERVAL) == 0 && latestBlock.index != 0) {
      return getAdjustedDifficulty(latestBlock, aBlockchain);
  } else {
      return latestBlock.difficulty;
  }
}

/*Data la lista di transazioni da inserire nel blocco si esegue il mining del
blocco e si inserisce nella BlockChain*/
Block BlockChain::generateRawNextBlock(vector<Transaction> blockData){
  #if DEBUG_FLAG == 1
  DEBUG_INFO("");
  #endif

  Block previousBlock = getLatestBlock();
  int difficulty = getDifficulty(getBlockchain());
  int nextIndex = previousBlock.index + 1;
  long nextTimestamp =  time(nullptr);
  Block newBlock;
  double duration;
  try{
    //Mining del nuovo blocco
    newBlock = findBlock(nextIndex, previousBlock.hash, nextTimestamp, blockData, difficulty, &duration);
  }catch(const char* msg){
    cout << endl;
    throw "EXCEPTION (generateRawNextBlock): Errore durante il mining del nuovo blocco";
  }
  if (addBlockToChain(newBlock)) {
    string data = "";
    try{
      //Aggiornamento dell'apposito file per la raccolta di statistiche sul tempo di mining
      ifstream inFile;
      inFile.open("blocksminingtime.txt");
      string line;
      if(inFile.is_open()) {
        while (getline(inFile, line)) {
          data = line;
        }
        inFile.close();
      } else {
        throw "Errore (generateRawNextBlock): non è stato possibile aprire il file per salvare il tempo di mining del blocco!";
      }
    }catch(const char* msg){
      cout << msg << endl << endl;
      cout << "ERRORE (generateRawNextBlock): Apertura del file contenente i tempi di mining fallita!";
    }
      Peer::getInstance().connectionsMtx.lock();
      Peer::getInstance().broadcastLatest(nextIndex, duration);
      Peer::getInstance().connectionsMtx.unlock();
      return newBlock;
  } else {
      cout << endl;
      throw "EXCEPTION (generateRawNextBlock): Errore durante l'inserimento del nuovo blocco nella BlockChain!";
  }
}

/*Ritorna la lista degli output non spesi appartenenti al nodo*/
vector<UnspentTxOut> BlockChain::getMyUnspentTransactionOutputs(){
  #if DEBUG_FLAG == 1
  DEBUG_INFO("");
  #endif

  return findUnspentTxOutsOfAddress(getPublicFromWallet(), getUnspentTxOuts());
}

/*Colleziona le transazioni dal transaction pool, inizializza la coinbase
transaction ed avvia la procedura di mining ed inserimento del blocco*/
Block BlockChain::generateNextBlock(){
  #if DEBUG_FLAG == 1
  DEBUG_INFO("");
  #endif

  Transaction coinbaseTx = getCoinbaseTransaction(getPublicFromWallet(), getLatestBlock().index + 1);
  vector<Transaction> blockData = TransactionPool::getInstance().getTransactionPool();
  blockData.insert (blockData.begin() , coinbaseTx);
  try{
    Block ret = generateRawNextBlock(blockData);
    /*Aggiornamento locale e verso tutti i peer delle statistiche relative ai tempi di attesa
    delle transazioni nel pool, vengono comunicati i tempi di attesa delle transazioni appena prelevate*/
    vector<string> stats = TransactionPool::getInstance().getStatStrings();
    vector<string>::iterator it;
    Peer::getInstance().connectionsMtx.lock();
    Peer::getInstance().broadcastTxPoolStat(stats);
    Peer::getInstance().connectionsMtx.unlock();
    ofstream myfile;
    myfile.open ("transactionwaitingtime.txt", ios::out | ios::app);
    if(myfile.is_open()) {
      string msg = "";
      for(it = stats.begin(); it != stats.end(); ++it){
        msg = msg + *it + "\n";
      }
      myfile << msg;
      myfile.close();
    } else {
      throw "EXCEPTION (handleClientMessage - TRANSACTION_POOL_STATS): Blocco generato, ma non è stato possibile aprire il file per salvare le statistiche di attesa delle transazioni!";
    }
    return ret;
  }catch(const char* msg){
    cout << msg << endl << endl;
    throw "EXCEPTION (generateNextBlock): Errore durante la generazione del nuovo blocco";
  }
}

/*Genera un nuovo blocco con la coinbase e una transazione avente txOuts creato a
partire da un vettore di coppie [indirizzo destinazione, amount] e lo inserisce
nella BlockChain (transazion con multipli output) */
Block BlockChain::generateNextBlockWithTransaction(vector<TxOut> txOuts){
  #if DEBUG_FLAG == 1
  DEBUG_INFO("");
  #endif

  vector<Transaction> blockData;
  vector<TxOut>::iterator it;
  blockData.push_back(getCoinbaseTransaction(getPublicFromWallet(), getLatestBlock().index + 1));

  for(it = txOuts.begin(); it != txOuts.end(); ++it) {
    if(typeid(it->amount) != typeid(float)) {
      cout << endl;
      throw "EXCEPTION (generateNextBlockWithTransaction): Importo non valido!";
    }
  }
  try{
    Transaction tx = createTransactionWithMultipleOutputs (txOuts, getPrivateFromWallet(), getUnspentTxOuts(), TransactionPool::getInstance().getTransactionPool());
    blockData.push_back(tx);
  }catch(const char* msg){
    cout << msg << endl << endl;
    throw "EXCEPTION (generateNextBlockWithTransaction): Creazione della transazione fallita";
  }
  try{
    return generateRawNextBlock(blockData);
  }catch(const char* msg){
    cout << msg << endl << endl;
    throw "EXCEPTION (generateNextBlockWithTransaction): Errore durante la generazione del nuovo blocco";
  }
}

/*Questo metodo effettua il mining di un nuovo blocco, viengono generati
(e scartati) nuovi blocchi finche l'hash ottenuto non rispetta la difficoltà richiesta*/
Block BlockChain::findBlock(int index, string previousHash, long timestamp, vector<Transaction> data, int difficulty, double *time) {
  #if DEBUG_FLAG == 1
  DEBUG_INFO("");
  #endif

  int nonce = 0;
  string hash;
  double duration;
  clock_t start = clock();

  /*Calcolo dell'hash (mining), l'operazione viene ripetuta finchè l'hash non
  rispetta la difficoltà (numero di zeri iniziali) richiesta*/
  while (true) {
    hash = calculateHash(index, previousHash, timestamp, data, difficulty, nonce);
    if(hashMatchesDifficulty(hash, difficulty)) {
      /*Il blocco generato ha un hash valido, si effetua il calcolo ed il
       salvataggio del tempo di mining del blocco in secondi (per le statistiche)*/
      duration = (std::clock() - start)/((double)CLOCKS_PER_SEC / 1000);
      *time = duration;
      ofstream myfile;
      myfile.open ("blocksminingtime.txt", ios::out | ios::app);
      if(myfile.is_open()) {
        myfile << "{\"block\": " <<  index << ", \"miningtime\": " << duration << "}\n";
      } else {
        cout << "Errore (findBlock): non è stato possibile aprire il file per salvare il tempo di mining del blocco!";
      }
      myfile.close();
      return Block(index, hash, previousHash, timestamp, data, difficulty, nonce);
    }
    nonce++;
  }
}

/*Ritorna il totale degli output non spesi nel wallet del nodo*/
float BlockChain::getAccountBalance(){
  #if DEBUG_FLAG == 1
  DEBUG_INFO("");
  #endif

  return getBalance(getPublicFromWallet(), getUnspentTxOuts());
}

/*Crea una nuova transazione e la inserisce nel transaction pool*/
Transaction BlockChain::sendTransaction(string address, float amount){
  #if DEBUG_FLAG == 1
  DEBUG_INFO("");
  #endif

  Transaction tx;
  try{
    tx = createTransaction(address, amount, getPrivateFromWallet(), getUnspentTxOuts(), TransactionPool::getInstance().getTransactionPool());
  }catch(const char* msg){
    cout << msg << endl << endl;
    throw "EXCEPTION (sendTransaction): Creazione delle transazione fallita";
  }
  try{
    //Aggiunta della nuova transazione al transaction pool
    TransactionPool::getInstance().addToTransactionPool(tx, getUnspentTxOuts());
  }catch(const char* msg){
    cout << msg << endl << endl;
    throw "EXCEPTION (sendTransaction):Inserimento della transazione nel pool fallito!";
  }
  //Broadcast a tutti gli altri peer della transactionpool aggiornata
  Peer::getInstance().connectionsMtx.lock();
  Peer::getInstance().broadCastTransactionPool();
  Peer::getInstance().connectionsMtx.unlock();
  return tx;
}

/*Ritorna una transazione della blockchain dato il suo id*/
Transaction BlockChain::getTransactionFromId(string transactionId){
  #if DEBUG_FLAG == 1
  DEBUG_INFO("");
  #endif

  list<Block>::iterator it;
  vector<Transaction>::iterator it2;
  for(it = blockchain.begin(); it != blockchain.end(); ++it){
    for(it2 = it->data.begin(); it2 != it->data.end(); ++it2){
      if(it2->id == transactionId){
        return *it2;
      }
    }
  }
  cout << endl;
  throw "EXCEPTION (getTransactionFromId): Transazione non presente nella blockchain!";
}

/*Calcolo dell'hash per un blocco*/
string BlockChain::calculateHash (int index, string previousHash, long timestamp, vector<Transaction> data, int difficulty, int nonce){
  #if DEBUG_FLAG == 1
  DEBUG_INFO("");
  #endif

  string ret = to_string(index) + previousHash + to_string(timestamp);
  vector<Transaction>::iterator it;
  for(it = data.begin(); it != data.end(); ++it){
    ret = ret + it->toString();
  }
  ret = ret + to_string(difficulty) + to_string(nonce);
  return picosha2::hash256_hex_string(ret);
}

/*Validazione della struttura (type checking) del blocco*/
bool BlockChain::isValidBlockStructure(Block block){
  #if DEBUG_FLAG == 1
  DEBUG_INFO("");
  #endif

  return (
    typeid(block.index).name() == typeid(int).name() &&
    typeid(block.hash).name() == typeid(string).name() &&
    typeid(block.previousHash).name() == typeid(string).name() &&
    typeid(block.timestamp).name() == typeid(long).name() &&
    typeid(block.data).name() == typeid(vector<Transaction>).name() );
}

/*Calcolo della complessità del mining del prossimo blocco (numero di
zeri iniziali necessari nell'hash) della blockchain data*/
int BlockChain::getAccumulatedDifficulty(vector<Block> aBlockchain){
  #if DEBUG_FLAG == 1
  DEBUG_INFO("");
  #endif

  int res = 0;
  vector<Block>::iterator it;
  for(it = aBlockchain.begin(); it != aBlockchain.end(); ++it){
    res = res + pow(2, it->difficulty);
  }
  return res;
}

/*Validazione del timestamp, per evitare che venga introdotto un timestamp falso in modo da
rendere valido un blocco con difficoltà inferiore a quella attuale vengono accettati solo i blocchi per
cui il mining è iniziato al massimo un minuto prima del mining dell'ultimo blocco ed al
massimo un minuto prima del tempo percepito dal nodo che effettua la validazione*/
bool BlockChain::isValidTimestamp(Block newBlock, Block previousBlock){
  #if DEBUG_FLAG == 1
  DEBUG_INFO("");
  #endif

  return (( previousBlock.timestamp - 60 < newBlock.timestamp ) && ( newBlock.timestamp - 60 < time(nullptr) ) );
}

/*Ricalcola l'hash del blocco e lo confronta con quello proposto (per rilevare modifiche)*/
bool BlockChain::hashMatchesBlockContent(Block block){
  #if DEBUG_FLAG == 1
  DEBUG_INFO("");
  #endif

  string blockHash = calculateHash(block.index, block.previousHash, block.timestamp, block.data, block.difficulty, block.nonce);
  return blockHash == block.hash;
}

/*Controlla se l'hash rispetta la difficoltà minima (deve iniziare con un certo numero di zeri)*/
bool BlockChain::hashMatchesDifficulty(string hash, int difficulty){
  #if DEBUG_FLAG == 1
  DEBUG_INFO("");
  #endif

  string hashInBinary = hexToBinary(hash);
  int n = hashInBinary.length();
  char charArray[n+1];
  strcpy(charArray, hashInBinary.c_str());

  /*Controllo che i primi byte dell'hash siano zeri, fino a raggiungere
  un numero pari alla difficoltà attuale*/
  for(int i = 0; i < difficulty; i++){
    if(charArray[i] != '0'){
      return false;
    }
  }
  return true;
}

/*Controllo della validità dell'hash e del rispetto della difficoltà minima (proof of work)*/
bool BlockChain::hasValidHash(Block block){
  #if DEBUG_FLAG == 1
  DEBUG_INFO("");
  #endif

  if (!hashMatchesBlockContent(block)) {
      cout << "ERRORE (hasValidHash): Hash non valido:" << endl << block.hash << endl;
      return false;
  }
  if (!hashMatchesDifficulty(block.hash, block.difficulty)) {
      cout << "ERRORE (hasValidHash): il blocco non soddisfa la difficoltà attesa(" << block.difficulty << "): " << block.hash << endl;
  }
  return true;
}

/*Validazione della struttura e della correttezza logica del blocco*/
bool BlockChain::isValidNewBlock(Block newBlock, Block previousBlock){
  #if DEBUG_FLAG == 1
  DEBUG_INFO("");
  #endif

  if (!isValidBlockStructure(newBlock)) { //Validazione struttura
    cout << "isValidNewBlock - invalid block structure: " <<  newBlock.toString();
    return false;
  }
  //Correttezza indice del blocco precedente
  if (previousBlock.index + 1 != newBlock.index) {
    cout << "isValidNewBlock - invalid index" << endl;
    return false;
  } else if (previousBlock.hash != newBlock.previousHash) {
    //Correttezza hash del blocco precedente
    cout << "isValidNewBlock - invalid previoushash";
    return false;
  } else if (!isValidTimestamp(newBlock, previousBlock)) {
    //Validità timestamp
    cout << "isValidNewBlock - invalid timestamp";
    return false;
  } else if (!hasValidHash(newBlock)) {
    //Validità hash
    return false;
  }
  return true;
}

/*Verifica la validità della blockchain ricevutaa, ritorna la lista aggiornata degli
output non spesi se questa è valida*/
vector<UnspentTxOut> BlockChain::isValidChain(list<Block> blockchainToValidate) {
  #if DEBUG_FLAG == 1
  DEBUG_INFO("");
  #endif

  vector<UnspentTxOut> aUnspentTxOuts;
  list<Block>::iterator it1;
  list<Block>::iterator it2;

  //Validità del blocco di genesi
  if(blockchainToValidate.front().isEqual(getGenesisBlock())) {
    cout << endl;
    throw "EXCEPTION (isValidChain): il blocco di genesi non è valido!";
  }

  //Controllo validità di ogni blocco della blockchain (struttura e transazioni contenute)
  for(it1 = blockchainToValidate.begin(); it1 != blockchainToValidate.end(); ++it1) {

    if(it1 != blockchainToValidate.begin()) {
      it2 = it1;
      it2--;
      if(!isValidNewBlock(*it1, *it2)){
        cout << endl;
        throw "EXCEPTION (isValidChain): la blockchain ricevuta contiene blocchi non validi!";
      }
    }


    try{
      //Check e aggiornamento lista output non spesi in base alle
      //transazioni presenti nel blocco
      aUnspentTxOuts = processTransactions(it1->data, aUnspentTxOuts, it1->index);
    }catch(const char* msg){
      cout << msg << endl << endl;
      throw "EXCEPTION (isValidChain): la blockchain ricevuta contiene delle transazioni non valide!";
    }
  }
  return aUnspentTxOuts;
}

/*Aggiunta di un blocco alla blockchain*/
bool BlockChain::addBlockToChain(Block newBlock) {
  #if DEBUG_FLAG == 1
  DEBUG_INFO("");
  #endif

  //Controllo validità del blocco
  if(isValidNewBlock(newBlock, getLatestBlock())) {
    vector<UnspentTxOut> ret;
    try{
      /*Check e aggiornamento lista output non spesi in base alle
      transazioni presenti nel blocco*/
      ret = processTransactions(newBlock.data, getUnspentTxOuts(), newBlock.index);
    }catch(const char* msg){
      cout << msg << endl;
      return false;
    }
    BlockChain::blockchain.push_back(newBlock);
    setUnspentTxOuts(ret);
    /*Check e aggiornamento del transactionPool (rimozione delle
    transazioni minate nel blocco ricevuto)*/
    TransactionPool::getInstance().updateTransactionPool(getUnspentTxOuts());
    try{
        saveBlockchainStats();
    }catch(const char* msg){
      cout << msg << endl << endl;
    }
    return true;
  }else{
    return false;
  }
}

/*Sostituzione blockchain con i blocchi ricevuti (se questa è
valida ed ha una difficoltà complessiva maggiore) si ricorda infatti che non
è valida la chain più lunga ma quella con la difficoltà cumulativa maggiore*/
void BlockChain::replaceChain(list<Block> newBlocks) {
  #if DEBUG_FLAG == 1
  DEBUG_INFO("");
  #endif
  int difficultyApproved = 0;


  if(getBlockchain().size() == 1 && newBlocks.size() == 1 && getBlockchain().back().timestamp > newBlocks.back().timestamp){
      //Si sta facendo un confronto tra due blocchi di genesi, si seleziona quello con timestamp minore
      difficultyApproved = 1;
  }else{
    /*Confronto della difficoltà cumulativa della blockchain attuale con quella ricevuta, a parità di difficoltà si
     * seleziona la blockchain per cui l'ultimo blocco ha timestamp mionre. Viene effettuata
     * una conversione da liste a vettori per migliorare le prestazioni del calcolo della difficoltà*/
    int newDiff = getAccumulatedDifficulty({ begin(newBlocks), end(newBlocks) });
    int localDiff = getAccumulatedDifficulty({begin(blockchain), end(blockchain)});
    if(newDiff > localDiff || (newDiff == localDiff && getBlockchain().back().timestamp > newBlocks.back().timestamp)) {
      difficultyApproved = 1;
    }
  }

  if(difficultyApproved == 0){
    cout << "INFO (replaceChain): La blockchain ricevuta è indietro rispetto a quella locale e verrà scartata...";
    return;
  }
  vector<UnspentTxOut> aUnspentTxOuts;
  try{
    aUnspentTxOuts = isValidChain(newBlocks);
    //Verifica della validità dei blocchi ricevuti
  }catch(const char* msg){
    cout << msg << endl << endl;
    throw "EXCEPTION (replaceChain): La blockchain ricevuta non è valida!";
  }


  cout << "La blockchain ricevuta è corretta! Verrà effettuata la sostituzione della blockchain." << endl;
  BlockChain::blockchain = newBlocks; //Sostituzione blockchain
  setUnspentTxOuts(aUnspentTxOuts); //Aggiornamento output non spesi
  //Aggiornamento transactionPool in base agli output non spesi aggiornati
  TransactionPool::getInstance().updateTransactionPool(getUnspentTxOuts());
  /*Broadcast della nuova blockchain a tutti i nodi (non indichiamo alcuna
   statistica perchè non si tratta di un nuovo blocco per cui si vuole il tempo di mining)
   in questo caso non effettuiamo il lock del mutex perchè questo metodo viene usato solo
   dai thread peer durante l'handling dei messaggi, dunque il lock è già acquisito
  */
  Peer::getInstance().broadcastLatest(-1,0);

  //In precedenza potrei aver scartato transazioni perchè la mia blockchain non era aggiornata,
  // richiedo la transaction pool per verificarle di nuovo
  Peer::getInstance().broadcastTxPoolQuery();
  cout << "Blockchain sostituita! Nuova BlockChain:" << endl << BlockChain::getInstance().toString();
  try{
    //Salvataggio dei nuovi dati relativi alla blockchain nell'apposito file
    saveBlockchainStats();
  }catch(const char* msg){
    cout << msg << endl << endl;
  }
}

/*Gestione per la ricezione di una nuova transazione, questa deve essere
aggiunta al transaction pool*/
void BlockChain::handleReceivedTransaction(Transaction transaction) {
  #if DEBUG_FLAG == 1
  DEBUG_INFO("");
  #endif

  try{
    TransactionPool::getInstance().addToTransactionPool(transaction, getUnspentTxOuts());
  }catch(const char* msg){
    cout << msg << endl << endl;
    throw "EXCEPTION (handleReceivedTransaction): La transazione ricevuta non è valida!";
    }
}

/*Salva in un file le statistiche della blockchain (numero di blocchi,
di transazioni e di coin)*/
void BlockChain::saveBlockchainStats() {
  #if DEBUG_FLAG == 1
  DEBUG_INFO("");
  #endif

  list<Block>::iterator it;
  int transactionNumber = 0;
  vector<UnspentTxOut>::iterator it2;
  float coins = 0;
  //Conteggio numero di transazioni effettuate
  for(it = blockchain.begin(); it != blockchain.end(); ++it) {
    transactionNumber += it->data.size();
  }

  //Conteggio dei coin in circolazione
  for(it2 = unspentTxOuts.begin(); it2 != unspentTxOuts.end(); ++it2){
    coins += it2->amount;
  }

  //Prendo il tempo corrente per abbinarlo ai dati prelevati
  time_t now = time(0);
  tm *ltm = localtime(&now);
  string time = to_string(1 + ltm->tm_hour) + ":" + to_string(1 + ltm->tm_min) + ":" + to_string(1 + ltm->tm_sec) + " " + to_string(ltm->tm_mday) + "/" + to_string(1 + ltm->tm_mon) + "/" + to_string(1900 + ltm->tm_year);

  // salvataggio su file
  ofstream myfile;
  myfile.open ("blockchainstats.txt", ios::out | ios::app);
  if(myfile.is_open()) {
    //Scrittura della nuova entry su file
    myfile << "{\"time\": \"" << time << "\", \"timestamp\": " << (now - blockchain.front().timestamp) << ", \"blocks\": " << blockchain.size() << ", \"transactions\": " << transactionNumber << ", \"coins\": " << coins << "}\n";
  } else {
    cout << "ERRORE (saveBlockchainStats): non è stato possibile aprire il file per salvare il tempo di mining del blocco!";
  }
  myfile.close();
}

/*Elimina i file delle statistiche se esistono */
void BlockChain::initStatFiles() {
  #if DEBUG_FLAG == 1
  DEBUG_INFO("");
  #endif

  if (FILE* file = fopen("blockchainstats.txt", "r")) {
    fclose(file);
    remove("blockchainstats.txt");
  }
  if (FILE* file = fopen("blocksminingtime.txt", "r")) {
    fclose(file);
    remove("blocksminingtime.txt");
  }
  if (FILE* file = fopen("transactionwaitingtime.txt", "r")) {
    fclose(file);
    remove("transactionwaitingtime.txt");
  }
}
