#ifndef __BLOCKCHAIN_DEFINITION__
#define __BLOCKCHAIN_DEFINITION__

#include <list>
#include <math.h>
#include <ctime>
#include "Transactions.hpp"
#include "Wallet.hpp"
#include "picosha2.h"
#include "TransactionPool.hpp"

class Block {
  public:
    int index;
    std::string hash;
    std::string previousHash;
    long timestamp;
    std::vector<Transaction> data;
    int difficulty;
    int nonce;
    Block(){}
    Block (int index, std::string hash, std::string previousHash, long timestamp, std::vector<Transaction> data, int difficulty, int nonce);
    std::string toString();
    bool isEqual(Block block);
};

class BlockChain {
  int BLOCK_GENERATION_INTERVAL= 10;
  int DIFFICULTY_ADJUSTMENT_INTERVAL = 10;

  public:
    static BlockChain& getInstance() {
        static BlockChain bc;
        return bc;
    }
    std::list<Block> blockchain;
    std::vector<UnspentTxOut> unspentTxOuts;

    std::string toString();

    //ritorna la blockchain
    std::list<Block> getBlockchain();

    //ritorna il vettore di output non spesi
    std::vector<UnspentTxOut> getUnspentTxOuts();

    //ritorna l'ultimo blocco della blockchain
    Block getLatestBlock();

    //Data la lista di transazioni da inserire nel blocco si esegue il mining
    //del blocco e si inserisce nella blockchain
    Block generateRawNextBlock(std::vector<Transaction> blockData);

    // Ritorna la lista degli output non spesi appartenenti al nodo
    std::vector<UnspentTxOut> getMyUnspentTransactionOutputs();

    //Colleziona le transazioni dal transaction pool, inizializza la coinbase
    //transaction ed avvia la procedura di mining ed inserimento del blocco
    Block generateNextBlock();

    //Genera un nuovo blocco con una sola transazione (oltre alla coinbase)
    //e lo inserisce nella blockchain
    Block generatenextBlockWithTransaction(std::string receiverAddress, float amount);

    //ritorna il totale degli output non spesi nel wallet del nodo
    float getAccountBalance();

    //Crea una nuova transazione e la inserisce nel transaction pool
    Transaction sendTransaction(std::string address, float amount);

    //Validazione della struttura del blocco (type checking)
    bool isValidBlockStructure(Block block);

    //Aggiunta di un nuovo blocco alla blockchain
    bool addBlockToChain(Block newBlock);

    //Validazione della blockchain riceevuta ed eventuale sostituzione con quella
    //attuale (si sceglie quella con la difficolta cumulativa maggiore)
    void replaceChain(std::list<Block> newBlocks);

    //Gestione della ricezione di una transazione, questa va inserita
    //nel transaction pool
    void handleReceivedTransaction(Transaction transaction);
  private:
    //Il pattern singleton viene implementato rendendo il costruttore di default privato
    // ed eliminando il costruttore dicopia e l'operazione di assegnamento
    BlockChain();
    BlockChain(const BlockChain&) = delete;
    BlockChain& operator=(const BlockChain&) = delete;

    //Tabella di conversione dei caratteri esadecimali in byte
    std::string hexToBinaryLookup(char c);

    //Genera il blocco di genesi (il primo blocco della blockchain)
    Block getGenesisBlock();

    //Converte una stringa esadecimale in una sequenza binaria
    std::string hexToBinary(std::string s);

    //Reimposta il vettore di output non spesi
    void setUnspentTxOuts(std::vector<UnspentTxOut> newUnspentTxOuts);

    //Calcola la nuova difficoltà per i blocchi
    int getAdjustedDifficulty(Block latestBlock, std::list<Block> aBlockchain);

    //Ritorna la difficoltà per il prossimo blocco
    int getDifficulty(std::list<Block> aBlockchain);

    //Calcolo dell'hash del blocco
    std::string calculateHash(int index, std::string previousHash, time_t timestamp, std::vector<Transaction> data, int difficulty, int nonce);

    //Questo metodo effettua il mining di un nuovo blocco, viengono generati
    // (e scartati) nuovi blocchi finche l'hash ottenuto non rispetta la difficoltà richiesta
    Block findBlock(int index, std::string previousHash, time_t timestamp, std::vector<Transaction> data, int difficulty);

    //Calcolo dell'hash del blocco
    std::string calculateHashForBlock(Block block);

    //Calcolo della complessità del mining del prossimo blocco per la blockchain data
    int getAccumulatedDifficulty(std::vector<Block> aBlockchain);

    //Validazione del timestamp del blocco, per evitare manipolazioni (attacchi
    // volontari sul timestamp) vedi spiegazione nell'implementazione del metodo
    bool isValidTimestamp(Block newBlock, Block previousBlock);

    //Ricalcola l'hash del blocco e lo confronta con quello proposto (per rilevare modifiche)
    bool hashMatchesBlockContent(Block block);

    //Controlla se l'hash rispetta la difficoltà minima (deve iniziare con un certo numero di zeri)
    bool hashMatchesDifficulty(std::string hash, int difficulty);

    //Controllo della validità dell'hash e del rispetto della difficoltà minima (proof of work)
    bool hasValidHash(Block block);

    //Validazione della struttura e della correttezza del blocco
    bool isValidNewBlock(Block newBlock, Block previousBlock);

    //Controllo validità della blockchain data, eventualmente ritorna la lista
    // di output non spesi aggiornata per la nuova blockchain
    std::vector<UnspentTxOut> isValidChain(std::list<Block> blockchainToValidate);

    //Salva in un file le statistiche della blockchain (numero di blocchi, di transazioni e di coin)
    void saveBlockchainStats();
};
#endif
