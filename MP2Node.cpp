
 * FILE NAME: MP2Node.cpp
 *
 * DESCRIPTION: MP2Node class definition
 **********************************/
#include "MP2Node.h"
#include "MP1Node.h"


static char s[256];
static int COunt=2;

/**
 * constructor
 */
MP2Node::MP2Node(Member *memberNode, Params *par, EmulNet * emulNet, Log * log, Address * address) {
	this->memberNode = memberNode;
	this->par = par;
	this->emulNet = emulNet;
	this->log = log;
	ht = new HashTable();
	this->memberNode->addr = *address;
}

/**
 * Destructor
 */
MP2Node::~MP2Node() {
	delete ht;
	delete memberNode;
}

/**
 * FUNCTION NAME: updateRing
 *
 * DESCRIPTION: This function does the following:
 * 				1) Gets the current membership list from the Membership Protocol (MP1Node)
 * 				   The membership list is returned as a vector of Nodes. See Node class in Node.h
 * 				2) Constructs the ring based on the membership list
 * 				3) Calls the Stabilization Protocol
 */
void MP2Node::updateRing() {
	/*
	 * Implement this. Parts of it are already implemented
	 */
	vector<Node> curMemList;
	bool change = false;
	int i;
	vector<Node>::iterator myPos;

	//if (COunt==3){ stabilizationProtocol5("ABCDE");}
	/*
	 *  Step 1. Get the current membership list from Membership Protocol / MP1
	 */
	curMemList = getMembershipList();

	/*
	 * Step 2: Construct the ring
	 */
	// Sort the list based on the hashCode
	sort(curMemList.begin(), curMemList.end());
	for (myPos=curMemList.begin(); myPos != curMemList.end();myPos++){
		sprintf(s,"updateRing:()  id(%s) ",myPos->nodeAddress.getAddress().c_str());
        	//log->LOG(&memberNode->addr, s);
	}
	hasMyReplicas.clear();
	haveReplicasOf.clear();
    // check the ring in last round
	if (ring.size() > 0) {
		populateNeighborNodes();
	}
	ring = curMemList;


	/*
	 * Step 3: Run the stabilization protocol IF REQUIRED
	 */
	// Run stabilization protocol if the hash table size is greater than zero and if there has been a changed in the ring
	if (ring.size() > 0 && hashT.size() > 0){
		stabilizationProtocol();
		COunt++;
		COunt %= 5;
	}

}

/**
 * FUNCTION NAME: getMemberhipList
 *
 * DESCRIPTION: This function goes through the membership list from the Membership protocol/MP1 and
 * 				i) generates the hash code for each member
 * 				ii) populates the ring member in MP2Node class
 * 				It returns a vector of Nodes. Each element in the vector contain the following fields:
 * 				a) Address of the node
 * 				b) Hash code obtained by consistent hashing of the Address
 */
vector<Node> MP2Node::getMembershipList() {
	unsigned int i;
	vector<Node> curMemList;
	int j = atoi(memberNode->addr.getAddress().c_str());
	for ( i = 0 ; i < this->memberNode->memberList.size(); i++ ) {
		Address addressOfThisMember;
		int id = this->memberNode->memberList.at(i).getid();
		short port = this->memberNode->memberList.at(i).getport();
		if(port != 0)continue;
		memcpy(&addressOfThisMember.addr[0], &id, sizeof(int));
		memcpy(&addressOfThisMember.addr[4], &port, sizeof(short));
		curMemList.emplace_back(Node(addressOfThisMember));
	}
	return curMemList;
}

/**
 * FUNCTION NAME: hashFunction
 *
 * DESCRIPTION: This functions hashes the key and returns the position on the ring
 * 				HASH FUNCTION USED FOR CONSISTENT HASHING
 *
 * RETURNS:
 * size_t position on the ring
 */
size_t MP2Node::hashFunction(string key) {
	std::hash<string> hashFunc;
	size_t ret = hashFunc(key);
	return ret%RING_SIZE;
}

/**
 * FUNCTION NAME: clientCreate
 *
 * DESCRIPTION: client side CREATE API
 * 				The function does the following:
 * 				1) Constructs the message
 * 				2) Finds the replicas of this key
 * 				3) Sends a message to the replica
 */
void MP2Node::clientCreate(string key, string value) {
	/*
	 * Implement this
	 */

	vector<Node>::iterator myPos;


	vector<Node> replicas = findNodes(key);
	g_transID++;
	Message msg(g_transID,memberNode->addr,CREATE,key,value,PRIMARY);
	quorum[g_transID] = vector<string>();
	outgoingMsgTimestamp[g_transID] = par->getcurrtime(); 
	outgoingMsg.emplace(g_transID,msg); // attention!!! opertor[] would result in compile-error 
       
	Message msg_sec(g_transID,memberNode->addr,CREATE,key,value,SECONDARY);
	Message msg_ter(g_transID,memberNode->addr,CREATE,key,value,TERTIARY);
	sendMsg(msg,&replicas[0].nodeAddress);
	sendMsg(msg_sec,&replicas[1].nodeAddress);
	sendMsg(msg_ter,&replicas[2].nodeAddress);


}

/**
 * FUNCTION NAME: clientRead
 *
 * DESCRIPTION: client side READ API
 * 				The function does the following:
 * 				1) Constructs the message
 * 				2) Finds the replicas of this key
 * 				3) Sends a message to the replica
 */
void MP2Node::clientRead(string key){
	/*
	 * Implement this
	 */

    g_transID++;
    vector<Node> replicas = findNodes(key);
        for ( vector<Node>::iterator myPos=replicas.begin(); myPos != replicas.end();myPos++){
                sprintf(s,"clientRead:()  id(%s) (%d)",myPos->nodeAddress.getAddress().c_str(),myPos->nodeAddress.addr[4]);
                log->LOG(&memberNode->addr, s);
        }
    Message msg(g_transID,memberNode->addr,READ,key);
    quorum[g_transID] = vector<string>();
    outgoingMsgTimestamp[g_transID] = par->getcurrtime();
    outgoingMsg.emplace(g_transID,msg); // attention!!! opertor[] would result in compile-error

    sendMsg(msg,&replicas[0].nodeAddress);
    sendMsg(msg,&replicas[1].nodeAddress);
    sendMsg(msg,&replicas[2].nodeAddress);

}


}

/**
 * FUNCTION NAME: clientUpdate
 *
 * DESCRIPTION: client side UPDATE API
 * 				The function does the following:
 * 				1) Constructs the message
 * 				2) Finds the replicas of this key
 * 				3) Sends a message to the replica
 */
void MP2Node::clientUpdate(string key, string value){
	/*
	 * Implement this
	 */
 log->LOG(&memberNode->addr, "clientUpdate:  ");

    g_transID++;
    vector<Node> replicas = findNodes(key);
    Message msg(g_transID,memberNode->addr,UPDATE,key,value,PRIMARY);
    quorum[g_transID] = vector<string>();
    outgoingMsgTimestamp[g_transID] = par->getcurrtime();
        outgoingMsg.emplace(g_transID,msg); // attention!!! opertor[] would result in compile-error

    Message msg_sec(g_transID,memberNode->addr,UPDATE,key,value,SECONDARY);
    Message msg_ter(g_transID,memberNode->addr,UPDATE,key,value,TERTIARY);
    sendMsg(msg,&replicas[0].nodeAddress);
    sendMsg(msg_sec,&replicas[1].nodeAddress);
    sendMsg(msg_ter,&replicas[2].nodeAddress);


}

/**
 * FUNCTION NAME: clientDelete
 *
 * DESCRIPTION: client side DELETE API
 * 				The function does the following:
 * 				1) Constructs the message
 * 				2) Finds the replicas of this key
 * 				3) Sends a message to the replica
 */
void MP2Node::clientDelete(string key){
	/*
	 * Implement this
	 */

    g_transID++;
    vector<Node> replicas = findNodes(key);
    Message msg(g_transID,memberNode->addr,DELETE,key);
    quorum[g_transID] = vector<string>();
    outgoingMsgTimestamp[g_transID] = par->getcurrtime();
        outgoingMsg.emplace(g_transID,msg); // attention!!! opertor[] would result in compile-error

    sendMsg(msg,&replicas[0].nodeAddress);
    sendMsg(msg,&replicas[1].nodeAddress);
    sendMsg(msg,&replicas[2].nodeAddress);

}

/**
 * FUNCTION NAME: createKeyValue
 *
 * DESCRIPTION: Server side CREATE API
 * 			   	The function does the following:
 * 			   	1) Inserts key value into the local hash table
 * 			   	2) Return true or false based on success or failure
 */
bool MP2Node::createKeyValue(string key, string value, ReplicaType replica) {
	/*
	 * Implement this
	 */
	// Insert key, value, replicaType into the hash table
	Entry entry(value,par->getcurrtime(),replica);

	if (key.length() != 5) { return false;}
 	auto ret = hashT.emplace (key.c_str(), entry.convertToString());
	return ret.second;
}



/**
 * FUNCTION NAME: readKey
 *
 * DESCRIPTION: Server side READ API
 * 			    This function does the following:
 * 			    1) Read key from local hash table
 * 			    2) Return value
 */
string MP2Node::readKey(string key) {
	/*
	 * Implement this
	 */
	// Read key from local hash table and return value

        auto search = hashT.find(key.c_str());
	  auto srch = hashT.find(key);
        if(search == hashT.end()){
                if(srch == hashT.end()){
                }
        }

        if ( search != hashT.end() ) {
                // Value found

                return getValue(search->second);
        }
        else {
                // Value not found
                return "";
        }


}

/**
 * FUNCTION NAME: updateKeyValue
 *
 * DESCRIPTION: Server side UPDATE API
 * 				This function does the following:
 * 				1) Update the key to the new value in the local hash table
 * 				2) Return true or false based on success or failure
 */
bool MP2Node::updateKeyValue(string key, string value, ReplicaType replica) {
	/*
	 * Implement this
	 */
	// Update key in local hash table and return true or false
	 log->LOG(&memberNode->addr, "updateKeyValue:  key(%s) value(%s)  ",key.c_str(),value.c_str());

  	auto ret = readKey(key.c_str());
  	if (ret =="") {
                // Key not found
                return false;
        }
	int found = ret.find(":");
    	ret = ret.substr(0,found );

        // Key found
    	Entry entry(value,par->getcurrtime(),replica);
    	hashT.at(key.c_str()) = entry.convertToString();
      // Update successful
    	return true;

}

/**
 * FUNCTION NAME: deleteKey
 *
 * DESCRIPTION: Server side DELETE API
 * 				This function does the following:
 * 				1) Delete the key from the local hash table
 * 				2) Return true or false based on success or failure
 */
bool MP2Node::deletekey(string key) {
	/*
	 * Implement this
	 */
	// Delete the key from the local hash table

	auto ret = hashT.erase(key.c_str());
	return ret;


}

/**
 * FUNCTION NAME: checkMessages
 *
 * DESCRIPTION: This function is the message handler of this node.
 * 				This function does the following:
 * 				1) Pops messages from the queue
 * 				2) Handles the messages according to message types
 */
void MP2Node::checkMessages() {
	/*
	 * Implement this. Parts of it are already implemented
	 */
	char * data;
	int size;

	/*
	 * Declare your local variables here
	 */

	// dequeue all messages and handle them
	while ( !memberNode->mp2q.empty() ) {
		/*
		 * Pop a message from the queue
		 */
		data = (char *)memberNode->mp2q.front().elt;
		size = memberNode->mp2q.front().size;
		memberNode->mp2q.pop();

		string message(data, data + size);

		/*
		 * Handle the message types here
		 */

		 handleMsg(message);

	}

	/*
	 * This function should also ensure all READ and UPDATE operation
	 * get QUORUM replies
	 */
	int J;
	for (auto it : outgoingMsgTimestamp) {
        	if (par->getcurrtime() - it.second > 10) {
                        // get type of msg
			if(it.second == 0 || it.first==0){break;}	
			auto ogm = outgoingMsg.find(it.first);
                        if ( ogm != outgoingMsg.end() ) {
            			Message msg = outgoingMsg.at(it.first);
	                        if (static_cast<MessageType>(msg.type) == CREATE) {
	                                log->logCreateSuccess(&memberNode->addr,true,it.first,msg.key,msg.value);
	            		}else if(static_cast<MessageType>(msg.type) == READ) {
	                		log->logReadFail(&memberNode->addr,true,it.first,msg.key);
	            		}else if (static_cast<MessageType>(msg.type) == UPDATE) {
	                		log->logUpdateFail(&memberNode->addr,true,it.first,msg.key,msg.value);
	            		}else if (static_cast<MessageType>(msg.type) == DELETE) {
	                		log->logDeleteFail(&memberNode->addr,true,it.first,msg.key);
	            		}
	
			 	auto sq= quorum.find(it.first);
				if ( sq != quorum.end() ) {
		            		 quorum.erase(it.first);
				}
			 	auto og = outgoingMsg.find(it.first);
				if ( og != outgoingMsg.end() ) {
		            		 outgoingMsg.erase(it.first);
				}
			 	auto ot = outgoingMsgTimestamp.find(it.first);
				if ( ot != outgoingMsgTimestamp.end() ) {
		            		 outgoingMsgTimestamp.erase(it.first);
				}
			}
        	}
    	}

}

/**
 * FUNCTION NAME: findNodes
 *
 * DESCRIPTION: Find the replicas of the given keyfunction
 * 				This function is responsible for finding the replicas of a key
 */
vector<Node> MP2Node::findNodes(string key) {
	size_t pos = hashFunction(key);
	vector<Node> addr_vec;
	if (ring.size() >= 3) {
		// if pos <= min || pos > max, the leader is the min
		if (pos <= ring.at(0).getHashCode() || pos > ring.at(ring.size()-1).getHashCode()) {
			addr_vec.emplace_back(ring.at(0));
			addr_vec.emplace_back(ring.at(1));
			addr_vec.emplace_back(ring.at(2));
		}
		else {
			// go through the ring until pos <= node
			for (int i=1; i<ring.size(); i++){
				Node addr = ring.at(i);
				if (pos <= addr.getHashCode()) {
					addr_vec.emplace_back(addr);
					addr_vec.emplace_back(ring.at((i+1)%ring.size()));
					addr_vec.emplace_back(ring.at((i+2)%ring.size()));
					break;
				}
			}
		}
	}
	return addr_vec;
}

/**
 * FUNCTION NAME: recvLoop
 *
 * DESCRIPTION: Receive messages from EmulNet and push into the queue (mp2q)
 */
bool MP2Node::recvLoop() {
    if ( memberNode->bFailed ) {
    	return false;
    }
    else {
    	return emulNet->ENrecv(&(memberNode->addr), this->enqueueWrapper, NULL, 1, &(memberNode->mp2q));
    }
}

/**
 * FUNCTION NAME: enqueueWrapper
 *
 * DESCRIPTION: Enqueue the message from Emulnet into the queue of MP2Node
 */
int MP2Node::enqueueWrapper(void *env, char *buff, int size) {
	Queue q;
	return q.enqueue((queue<q_elt> *)env, (void *)buff, size);
}
/**
 * FUNCTION NAME: stabilizationProtocol
 *
 * DESCRIPTION: This runs the stabilization protocol in case of Node joins and leaves
 * 				It ensures that there always 3 copies of all keys in the DHT at all times
 * 				The function does the following:
 *				1) Ensures that there are three "CORRECT" replicas of all the keys in spite of failures and joins
 *				Note:- "CORRECT" replicas implies that every key is replicated in its two neighboring nodes in the ring
 */




void MP2Node::stabilizationProtocol() {
	/*
	 * Implement this
	 */
    int curHash = hashFunction(memberNode->addr.addr);
    int i;
static char ss[256];
    for (i = 0; i < ring.size(); i++) {
        if (curHash == ring.at(i).getHashCode()) {
            break;
        }
    }

    Node pre1 = ring.at( ((i - 2 + ring.size()) % ring.size()) );
    Node pre2 = ring.at( ((i - 1 + ring.size()) % ring.size()) );
    Node post1 = ring.at((i + 1) % ring.size());
    Node post2 = ring.at((i + 2) % ring.size());

    /*
     * Topology:
     * pre1 - pre2 - currentNode - post1 - post2
     *
     */
     string a = "";
     for (int i = 0; i < ring.size(); i++) {
         a += ring[i].nodeAddress.getAddress() + " ";
     }
     const char * s = a.c_str();
    // old pre1 fails
    if (pre1.getHashCode() != haveReplicasOf[0].getHashCode() && pre2.getHashCode() == haveReplicasOf[1].getHashCode()) {
        s = ("pre 1 " + a).c_str();
        log->LOG(&memberNode->addr,s);
    }
    // old pre2 fails
    // promote secondary to primary locally
    else if (pre2.getHashCode() == haveReplicasOf[0].getHashCode() && pre2.getHashCode() != haveReplicasOf[1].getHashCode()) {
        s = ("pre 2 " + a).c_str();
        log->LOG(&memberNode->addr,s);
        // promte current secondary to primary
        // and send msg to post1 to promote tertiary to secondary
        // send msg to post2 to create tertiary
        for (auto it : hashT) {
            string value = getValue(it.second);
            int replicaType = getReplicaType(it.second);

            if (static_cast<ReplicaType>(replicaType) == SECONDARY) {
                if (updateKeyValue(it.first,value,PRIMARY)) {
                    log->logUpdateSuccess(&memberNode->addr,false,0,it.first,value);
                }else {
                    log->logUpdateFail(&memberNode->addr,false,0,it.first,value);
                }

                // send to next two availiable nodes
                Message msg_sec(0,memberNode->addr,UPDATE,it.first,value,SECONDARY);
                Message msg_ter(0,memberNode->addr,CREATE,it.first,value,TERTIARY);

                sendMsg(msg_sec,&(pre1.nodeAddress));
                sendMsg(msg_ter,&(pre2.nodeAddress));

            }
        }
    }

    // both old pre1 and old pre2 fail
    // promote both secondary and tertiary to primary locally
    else if (pre1.getHashCode() != haveReplicasOf[0].getHashCode() && pre2.getHashCode() != haveReplicasOf[1].getHashCode()) {
        s = ("pre 1 2 " + a).c_str();
        log->LOG(&memberNode->addr,s);
        for (auto it : hashT) {
            string value = getValue(it.second);
            int replicaType = getReplicaType(it.second);

            if (static_cast<ReplicaType>(replicaType) == SECONDARY) {
                if (updateKeyValue(it.first,value,PRIMARY)) {
                    log->logUpdateSuccess(&memberNode->addr,false,0,it.first,value);
                }else {
                    log->logUpdateFail(&memberNode->addr,false,0,it.first,value);
                }
                //promoteTerToSec();
                // create ter
                // send to next two availiable nodes
                Message msg_sec(0,memberNode->addr,UPDATE,it.first,value,SECONDARY);
                Message msg_ter(0,memberNode->addr,CREATE,it.first,value,TERTIARY);

                sendMsg(msg_sec,&(pre1.nodeAddress));
                sendMsg(msg_ter,&(pre2.nodeAddress));
            }
            if (static_cast<ReplicaType>(replicaType) == TERTIARY) {
                if (updateKeyValue(it.first,value,PRIMARY)) {
                   log->logUpdateSuccess(&memberNode->addr,false,0,it.first,value);
                }else {
                    log->logUpdateFail(&memberNode->addr,false,0,it.first,value);
                }

                // send to next two availiable nodes
                Message msg_sec(0,memberNode->addr,UPDATE,it.first,value,SECONDARY);
                Message msg_ter(0,memberNode->addr,UPDATE,it.first,value,TERTIARY);

                sendMsg(msg_sec,&(pre1.nodeAddress));
                sendMsg(msg_ter,&(pre2.nodeAddress));
            }
        }

    }

    // only post2 fails
    if (post1.getHashCode() == hasMyReplicas[0].getHashCode() && post2.getHashCode() != hasMyReplicas[1].getHashCode()) {
        s = ("post 2 " + a).c_str();
        log->LOG(&memberNode->addr, s);
        for (auto it : hashT) {
            string value = getValue(it.second);
            int replicaType = getReplicaType(it.second);

            if (static_cast<ReplicaType>(replicaType) == PRIMARY) {
                Message msg_ter(0,memberNode->addr,CREATE,it.first,value,TERTIARY);

                sendMsg(msg_ter,&(post2.nodeAddress));
            }
        }
    }
    // only post1 fails
    else if (post1.getHashCode() != hasMyReplicas[0].getHashCode() && post1.getHashCode() == hasMyReplicas[1].getHashCode()) {
        s = ("post 1 " + a).c_str();
        log->LOG(&memberNode->addr,s);
        for (auto it : hashT) {
            string value = getValue(it.second);
            int replicaType = getReplicaType(it.second);

            if (static_cast<ReplicaType>(replicaType) == PRIMARY) {
                Message msg_sec(0,memberNode->addr,UPDATE,it.first,value,SECONDARY);
                Message msg_ter(0,memberNode->addr,CREATE,it.first,value,TERTIARY);

                sendMsg(msg_sec,&(post1.nodeAddress));
                sendMsg(msg_ter,&(post2.nodeAddress));
            }
        }
    }
    // both post1 and post2 fail
    else if (post1.getHashCode() != hasMyReplicas[0].getHashCode() && post2.getHashCode() != hasMyReplicas[1].getHashCode()) {
        s = ("post 1 2 " + a).c_str();
        log->LOG(&memberNode->addr,s);
        for (auto it : hashT) {
            string value = getValue(it.second);
            int replicaType = getReplicaType(it.second);

            if (static_cast<ReplicaType>(replicaType) == PRIMARY) {
                Message msg_sec(0,memberNode->addr,CREATE,it.first,value,SECONDARY);
                Message msg_ter(0,memberNode->addr,CREATE,it.first,value,TERTIARY);

                sendMsg(msg_sec,&(post1.nodeAddress));
                sendMsg(msg_ter,&(post2.nodeAddress));
            }
        }
    }


}


// wrapper for all message types handling
void MP2Node::handleMsg(string message) {
    string originalMsg = message;
    int found = message.find("::");
    int transID = stoi(message.substr(0,found));
    message = message.substr(found + 2);

    found = message.find("::");
    string fromAddr = message.substr(0,found);
    message = message.substr(found + 2);

    found = message.find("::");
    MessageType msgType = static_cast<MessageType>(stoi(message.substr(0,found)));
    message = message.substr(found + 2);

    if (msgType == CREATE || msgType == UPDATE) {
        createUpdateMsgHandler(message,transID,fromAddr,msgType);
    }else if (msgType == DELETE) {
        deleteMsgHandler(message,transID,fromAddr);
       log->LOG(&memberNode->addr,s);
    }else if (msgType == READ) {
        readMsgHandler(message,transID,fromAddr);
       log->LOG(&memberNode->addr,s);
    }else if (msgType == REPLY) {
       log->LOG(&memberNode->addr,s);
        replyMsgHandler(originalMsg,message,transID);
    }else if (msgType == READREPLY) {
        readReplyMsgHandler(originalMsg,message,transID);
    }
}



// handles CREATE and UPDATE msg
void MP2Node::createUpdateMsgHandler(string message, int transID, string masterAddrStr, MessageType msgType) {
    string om = message;
    int found = message.find("::");
    string key = message.substr(0,found);
    message = message.substr(found + 2);

    found = message.find("::");
    string value = message.substr(0,found);

    ReplicaType replicaType = static_cast<ReplicaType>(stoi(message.substr(found + 2)));
    Address masterAddr = Address(masterAddrStr);

    // create/update the K/V pair on local hash table and send back a reply to master
    bool success;
    if (msgType == CREATE) {
        success = createKeyValue(key,value, replicaType);

        // logging
        if (success) {
            log->logCreateSuccess(&memberNode->addr,false,transID,key,value);
        }else {
            log->logCreateFail(&memberNode->addr,false,transID,key,value);
	}
       
    }else if (msgType == UPDATE) {
        success = updateKeyValue(key,value, replicaType);

        // logging
        if (success) {
            log->logUpdateSuccess(&memberNode->addr,false,transID,key,value);
        }else {
            log->logUpdateFail(&memberNode->addr,false,transID,key,value);
        }
    }

    Message reply(transID,memberNode->addr,REPLY,success);
    sendMsg(reply,&masterAddr);
}


// handles REPLY messages
void MP2Node::replyMsgHandler(string originalMsg, string leftMsg,int transID) {


    int lm = atoi(leftMsg.c_str());
    if (transID != 0) {
        quorum[transID].push_back(originalMsg);
    }

    // if all replies have been received
    if (quorum[transID].size() >= 2) {
        int vote = 0;

        for (int i = 0; i < quorum[transID].size(); i++) {
            if (lm == 1) {
                vote++;
            }
        }

        Message outgoingMessage = outgoingMsg.at(transID);
        if (vote >= 2) {
            if (outgoingMessage.type == CREATE) {
                log->logCreateSuccess(&memberNode->addr,true,transID,outgoingMessage.key,outgoingMessage.value);
            }else if (outgoingMessage.type == UPDATE) {
                log->logUpdateSuccess(&memberNode->addr,true,transID,outgoingMessage.key,outgoingMessage.value);
            }else if (outgoingMessage.type == DELETE) {
                log->logDeleteSuccess(&memberNode->addr,true,transID,outgoingMessage.key);
            }
        }else {
            if (outgoingMessage.type == CREATE) {
                log->logCreateFail(&memberNode->addr,true,transID,outgoingMessage.key,outgoingMessage.value);
            }else if (outgoingMessage.type == UPDATE) {
                log->logUpdateFail(&memberNode->addr,true,transID,outgoingMessage.key,outgoingMessage.value);
            }else if (outgoingMessage.type == DELETE) {
                log->logDeleteFail(&memberNode->addr,true,transID,outgoingMessage.key);
            }
        }

        // close this transaction
        quorum.erase(transID);
        outgoingMsg.erase(transID);
        outgoingMsgTimestamp.erase(transID);
    }
}


// handles DELETE msg
void MP2Node::deleteMsgHandler(string key, int transID, string masterAddrStr) {
    Address masterAddr = Address(masterAddrStr);
    bool success = deletekey(key);

    if (success) {
        log->logDeleteSuccess(&memberNode->addr,true,transID,key);
    }else {
        log->logDeleteFail(&memberNode->addr,false,transID,key);
    }

    Message reply(transID,memberNode->addr,REPLY,success);
    sendMsg(reply,&masterAddr);
}


// update hasMyReplicas and haveReplicasOf
void MP2Node::populateNeighborNodes() {
    int curHash = hashFunction(memberNode->addr.addr);
    int i;
    for (i = 0; i < ring.size(); i++) {
        if (curHash == ring.at(i).getHashCode()) {
            break;
        }
    }

    hasMyReplicas.push_back(ring.at((i + 1) % ring.size()));
    hasMyReplicas.push_back(ring.at((i + 2) % ring.size()));

    haveReplicasOf.push_back( ring.at((i - 2 + ring.size()) % ring.size()) );
    haveReplicasOf.push_back(ring.at((i - 1 + ring.size()) % ring.size()));
}


// wrapper for message sending
void MP2Node::sendMsg(Message msg, Address *toAddr) {
    string msgStr = msg.toString();
    char* msgChar = (char*)malloc(msgStr.size() + 1);

    strcpy(msgChar,msgStr.c_str());
    emulNet->ENsend(&memberNode->addr,toAddr,msgChar,msgStr.size() + 1);
    
    free(msgChar);
}

void MP2Node::getValueAndReplicaType(string str, string &value, int &replicaType) {
    int found = str.find(":");
    value = str.substr(0,found);
    str = str.substr(found + 1);

    found = str.find(":");
    replicaType = stoi(str.substr(found + 1));
}

string MP2Node::getValue(string str ) {
    int found = str.find(":");
    str = str.substr(0,found);
    return str;
}

int MP2Node::getReplicaType(string str) {
    int found = str.find(":");
    str = str.substr(found + 1);

    found = str.find(":");
    return stoi(str.substr(found + 1));
}
// handles READ messages
void MP2Node::readMsgHandler(string key,int transID,string masterAddrStr) {
    Address masterAddr = Address(masterAddrStr);
    string value = readKey(key);

    string v;
    v = (string)key.c_str() + (string)"-" + (string)getValue(value).c_str();
    Message readReply(transID,memberNode->addr,getValue(value));
    sendMsg(readReply,&masterAddr);
}
// handles READREPLY messages
void MP2Node::readReplyMsgHandler(string originalMsg, string value, int transID) {
    quorum[transID].push_back(originalMsg);
    string val;
    string ky;
    if (value.length() > 8){
        int fnd = value.find("-");
        ky = value.substr(0,fnd);
        val = value.substr(fnd+1);
        log->logReadFail(&memberNode->addr,false,transID,ky);
    }
    else if (quorum[transID].size() >= 2) {
    	Message outgoingMessage = outgoingMsg.at(transID);
	if( outgoingMessage.key.length() > 5){
        	log->logReadFail(&memberNode->addr,true,transID,outgoingMessage.key);
	} else {
        	log->logReadSuccess(&memberNode->addr,true,transID,outgoingMessage.key,value);
	}

        // close this transaction
        quorum.erase(transID);
        outgoingMsg.erase(transID);
        outgoingMsgTimestamp.erase(transID);
    } else {
    }
}





void MP2Node::stabilizationProtocol0() {
        /*
         * Implement this
         */
    int curHash = hashFunction(memberNode->addr.addr);
    int i;
static char ss[256];
    for (i = 0; i < ring.size(); i++) {
        if (curHash == ring.at(i).getHashCode()) {
            break;
        }
    }

    Node pre1 = ring.at( ((i - 2 + ring.size()) % ring.size()) );// 0
    Node pre2 = ring.at( ((i - 1 + ring.size()) % ring.size()) );// 1
    Node post1 = ring.at((i + 1) % ring.size());//1
    Node post2 = ring.at((i + 2) % ring.size());//0

    /*
     * Topology:
     * pre1 - pre2 - currentNode - post1 - post2
     *
     */
     string a;
     for (int i = 0; i < ring.size(); i++) {
         a += ring[i].nodeAddress.getAddress() + " ";
     }
sprintf(s,"stabilizationP: ringsize(%d) a(%s) hashcode(%d) COunt(%d)",ring.size(),a.c_str(),curHash,COunt);
log->LOG(&memberNode->addr,s);
    // old pre1 fails
    if (pre1.getHashCode() != haveReplicasOf[0].getHashCode() && pre2.getHashCode() == haveReplicasOf[1].getHashCode()) {
        sprintf(s,"pre 1 %s",a.c_str());
        log->LOG(&memberNode->addr,s);
    }
    // old pre2 fails
    // promote secondary to primary locally
    else if (pre2.getHashCode() == haveReplicasOf[0].getHashCode() && pre2.getHashCode() != haveReplicasOf[1].getHashCode()) {
        sprintf(s,"pre 2 %s",a.c_str());
        log->LOG(&memberNode->addr,s);
        // promte current secondary to primary
        // and send msg to post1 to promote tertiary to secondary
        // send msg to post2 to create tertiary
        for (auto it : hashT) {
            string value = getValue(it.second);
            int replicaType = getReplicaType(it.second);
            //getValueAndReplicaType(it.second,value,replicaType);
        log->LOG(&memberNode->addr,"pre 2 Iteration After getValueAndReplicaType");

            if (static_cast<ReplicaType>(replicaType) == SECONDARY) {
                if (updateKeyValue(it.first,value,PRIMARY)) {
                    log->logUpdateSuccess(&memberNode->addr,false,0,it.first,value);
                }else {
                    log->logUpdateFail(&memberNode->addr,false,0,it.first,value);
                }

                // send to next two availiable nodes

                Message msg_sec(0,memberNode->addr,UPDATE,it.first,value,SECONDARY);
                Message msg_ter(0,memberNode->addr,CREATE,it.first,value,TERTIARY);


        sprintf(ss,"pre 2 BeforeA Send msg_sec(%s) msg_ter(%s)",msg_sec.toString().c_str(),msg_ter.toString().c_str());
        log->LOG(&memberNode->addr,ss);
        sprintf(ss,"pre 2 Send  addr1(%s) addr2(%s)",pre1.nodeAddress.getAddress().c_str(),pre2.nodeAddress.getAddress().c_str());
        log->LOG(&memberNode->addr,ss);
                sendMsg(msg_sec,&(pre1.nodeAddress));
                sendMsg(msg_ter,&(pre2.nodeAddress));
        sprintf(ss,"pre 2 After Send  addr1(%s) addr2(%s)",pre1.nodeAddress.getAddress().c_str(),pre2.nodeAddress.getAddress().c_str());
        log->LOG(&memberNode->addr,ss);

            }
        log->LOG(&memberNode->addr,"pre 2 Iteration End");
        }
    log->LOG(&memberNode->addr,"pre 2 END");
    }


}



void MP2Node::stabilizationProtocol1() {
        /*
         * Implement this
         */
    int curHash = hashFunction(memberNode->addr.addr);
    int i;
static char ss[256];
    for (i = 0; i < ring.size(); i++) {
        if (curHash == ring.at(i).getHashCode()) {
            break;
        }
    }

    Node pre1 = ring.at( ((i - 2 + ring.size()) % ring.size()) );// 0
    Node pre2 = ring.at( ((i - 1 + ring.size()) % ring.size()) );// 1
    Node post1 = ring.at((i + 1) % ring.size());//1
    Node post2 = ring.at((i + 2) % ring.size());//0

sprintf(s,"STABILIZATION-1-P: i(%d) pre1.add(%s) hashcode(%d) replicaOf[0].hc(%d)",i,pre1.nodeAddress.getAddress().c_str(),pre1.getHashCode(), haveReplicasOf[0].getHashCode());
log->LOG(&memberNode->addr,s);
sprintf(s,"STABILIZATION-1-P: i(%d) pre2add(%s) hashcode(%d) replicaOf[1].hc(%d)",i,pre2.nodeAddress.getAddress().c_str(),pre2.getHashCode(), haveReplicasOf[1].getHashCode());
log->LOG(&memberNode->addr,s);
sprintf(s,"STABILIZATION-1-P: i(%d) post1.add(%s) hashcode(%d) myreplica[0].hc(%d)",i,post1.nodeAddress.getAddress().c_str(),post1.getHashCode(), hasMyReplicas[0].getHashCode());
log->LOG(&memberNode->addr,s);
sprintf(s,"STABILIZATION-1-P: i(%d) post2.add(%s) hashcode(%d) myreplica[1].hc(%d)",i,post2.nodeAddress.getAddress().c_str(),post2.getHashCode(), hasMyReplicas[1].getHashCode());
log->LOG(&memberNode->addr,s);
    /*
     * Topology:
     * pre1 - pre2 - currentNode - post1 - post2
     *
     */
     string a;
     for (int i = 0; i < ring.size(); i++) {
         a += ring[i].nodeAddress.getAddress() + " ";
     }
sprintf(s,"stabilizationP: ringsize(%d) a(%s) hashcode(%d) COunt(%d)",ring.size(),a.c_str(),curHash,COunt);
log->LOG(&memberNode->addr,s);
    // both old pre1 and old pre2 fail
    // promote both secondary and tertiary to primary locally
    if (pre1.getHashCode() != haveReplicasOf[0].getHashCode() && pre2.getHashCode() != haveReplicasOf[1].getHashCode()) {
        sprintf(s,"pre 1 2 %s 1(%d) 2(%d) 3(%d) 4(%d)",a.c_str(), pre1.getHashCode(),haveReplicasOf[0].getHashCode(),pre2.getHashCode(),haveReplicasOf[1].getHashCode());
        log->LOG(&memberNode->addr,s);
//        sprintf(s,"pre 1 2 %s 1(%s) 2(%s) 3(%s) 4(%s)",a.c_str(), pre1.getAddress()->c_str(),haveReplicasOf[0].getAddress()->c_str(),pre2.getAddress()->c_str(),haveReplicasOf[1].getAddress()->c_str());
 //       log->LOG(&memberNode->addr,s);
        for (auto it : hashT) {
            string value = getValue(it.second);
            int replicaType = getReplicaType(it.second);
            //getValueAndReplicaType(it.second,value,replicaType);
        log->LOG(&memberNode->addr,"pre 1 2 Iteration after Call");

            if (static_cast<ReplicaType>(replicaType) == SECONDARY) {
                if (updateKeyValue(it.first,value,PRIMARY)) {
                    log->logUpdateSuccess(&memberNode->addr,false,0,it.first,value);
                }else {
                    log->logUpdateFail(&memberNode->addr,false,0,it.first,value);
                }
                //promoteTerToSec();
                // create ter
                // send to next two availiable nodes
                Message msg_sec(0,memberNode->addr,UPDATE,it.first,value,SECONDARY);
                Message msg_ter(0,memberNode->addr,UPDATE,it.first,value,TERTIARY);
        sprintf(ss,"pre 1 2 BeforeA Send msg_sec(%s) msg_ter(%s)",msg_sec.toString().c_str(),msg_ter.toString().c_str());
        log->LOG(&memberNode->addr,ss);

                sendMsg(msg_sec,&(pre1.nodeAddress));
        log->LOG(&memberNode->addr,"pre 1 2 after SendA msg_sec");
                sendMsg(msg_ter,&(pre2.nodeAddress));
        sprintf(ss,"pre 1 2 AfterA Send addr1(%s) addr2(%s)",pre1.nodeAddress.getAddress().c_str(),pre2.nodeAddress.getAddress().c_str());
        log->LOG(&memberNode->addr,ss);
            }
            if (static_cast<ReplicaType>(replicaType) == TERTIARY) {
                if (updateKeyValue(it.first,value,PRIMARY)) {
                    log->logUpdateSuccess(&memberNode->addr,false,0,it.first,value);
                }else {
                    log->logUpdateFail(&memberNode->addr,false,0,it.first,value);
                }

                // send to next two availiable nodes
                Message msg_sec(0,memberNode->addr,UPDATE,it.first,value,SECONDARY);
                Message msg_ter(0,memberNode->addr,UPDATE,it.first,value,TERTIARY);

        log->LOG(&memberNode->addr,"pre 1 2 BeforeB Send");
        sprintf(ss,"pre 1 2 BeforeB Send msg_sec(%s) msg_ter(%s)",msg_sec.toString().c_str(),msg_ter.toString().c_str());
        log->LOG(&memberNode->addr,ss);
        sprintf(ss,"pre 1 2 Send  addr1(%s) addr2(%s)",pre1.nodeAddress.getAddress().c_str(),pre2.nodeAddress.getAddress().c_str());
        log->LOG(&memberNode->addr,ss);
                sendMsg(msg_sec,&(pre1.nodeAddress));
                sendMsg(msg_ter,&(pre2.nodeAddress));
        log->LOG(&memberNode->addr,"pre 1 2 AfterB Send");
        sprintf(ss,"pre 1 2 After B Send  addr1(%s) addr2(%s)",pre1.nodeAddress.getAddress().c_str(),pre2.nodeAddress.getAddress().c_str());
        log->LOG(&memberNode->addr,ss);
            }
        log->LOG(&memberNode->addr,"pre 1 2 Iteration End");
        }
    log->LOG(&memberNode->addr,"pre 1 2 END");

    }
}




void MP2Node::stabilizationProtocol2() {
        /*
         * Implement this
         */
    int curHash = hashFunction(memberNode->addr.addr);
    int i;
static char ss[256];
    for (i = 0; i < ring.size(); i++) {
        if (curHash == ring.at(i).getHashCode()) {
            break;
        }
    }

    Node pre1 = ring.at( ((i - 2 + ring.size()) % ring.size()) );// 0
    Node pre2 = ring.at( ((i - 1 + ring.size()) % ring.size()) );// 1
    Node post1 = ring.at((i + 1) % ring.size());//1
    Node post2 = ring.at((i + 2) % ring.size());//0

sprintf(s,"STABILIZATION-2-P: i(%d) pre1.add(%s) hashcode(%d) replicaOf[0].hc(%d)",i,pre1.nodeAddress.getAddress().c_str(),pre1.getHashCode(), haveReplicasOf[0].getHashCode());
log->LOG(&memberNode->addr,s);
sprintf(s,"STABILIZATION-2-P: i(%d) pre2add(%s) hashcode(%d) replicaOf[1].hc(%d)",i,pre2.nodeAddress.getAddress().c_str(),pre2.getHashCode(), haveReplicasOf[1].getHashCode());
log->LOG(&memberNode->addr,s);
sprintf(s,"STABILIZATION-2-P: i(%d) post1.add(%s) hashcode(%d) myreplica[0].hc(%d)",i,post1.nodeAddress.getAddress().c_str(),post1.getHashCode(), hasMyReplicas[0].getHashCode());
log->LOG(&memberNode->addr,s);
sprintf(s,"STABILIZATION-2-P: i(%d) post2.add(%s) hashcode(%d) myreplica[1].hc(%d)",i,post2.nodeAddress.getAddress().c_str(),post2.getHashCode(), hasMyReplicas[1].getHashCode());
log->LOG(&memberNode->addr,s);
    /*
     * Topology:
     * pre1 - pre2 - currentNode - post1 - post2
     *
     */
     string a;
     for (int i = 0; i < ring.size(); i++) {
         a += ring[i].nodeAddress.getAddress() + " ";
     }
sprintf(s,"stabilizationP: ringsize(%d) a(%s) hashcode(%d) COunt(%d)",ring.size(),a.c_str(),curHash,COunt);
log->LOG(&memberNode->addr,s);
  // only post2 fails
    if (post1.getHashCode() == hasMyReplicas[0].getHashCode() && post2.getHashCode() != hasMyReplicas[1].getHashCode()) {
        sprintf(s,"post 2 %s",a.c_str());
        log->LOG(&memberNode->addr, s);
        for (auto it : hashT) {
            string value = getValue(it.second);
            int replicaType = getReplicaType(it.second);
            //getValueAndReplicaType(it.second,value,replicaType);
        log->LOG(&memberNode->addr,"post 2 Iteration After calling getValueAndReplicaType");

            if (static_cast<ReplicaType>(replicaType) == PRIMARY) {
                Message msg_ter(0,memberNode->addr,CREATE,it.first,value,TERTIARY);

        log->LOG(&memberNode->addr,"post 2 Before Send");
                sendMsg(msg_ter,&(post2.nodeAddress));
        log->LOG(&memberNode->addr,"post 2 After Send");
            }
        }
    }


}




void MP2Node::stabilizationProtocol3() {
        /*
         * Implement this
         */
    int curHash = hashFunction(memberNode->addr.addr);
    int i;
static char ss[256];
    for (i = 0; i < ring.size(); i++) {
        if (curHash == ring.at(i).getHashCode()) {
            break;
        }
    }

    Node pre1 = ring.at( ((i - 2 + ring.size()) % ring.size()) );// 0
    Node pre2 = ring.at( ((i - 1 + ring.size()) % ring.size()) );// 1
    Node post1 = ring.at((i + 1) % ring.size());//1
    Node post2 = ring.at((i + 2) % ring.size());//0

sprintf(s,"STABILIZATION-3-P: i(%d) pre1.add(%s) hashcode(%d) replicaOf[0].hc(%d)",i,pre1.nodeAddress.getAddress().c_str(),pre1.getHashCode(), haveReplicasOf[0].getHashCode());
log->LOG(&memberNode->addr,s);
sprintf(s,"STABILIZATION-3-P: i(%d) pre2add(%s) hashcode(%d) replicaOf[1].hc(%d)",i,pre2.nodeAddress.getAddress().c_str(),pre2.getHashCode(), haveReplicasOf[1].getHashCode());
log->LOG(&memberNode->addr,s);
sprintf(s,"STABILIZATION-3-P: i(%d) post1.add(%s) hashcode(%d) myreplica[0].hc(%d)",i,post1.nodeAddress.getAddress().c_str(),post1.getHashCode(), hasMyReplicas[0].getHashCode());
log->LOG(&memberNode->addr,s);
sprintf(s,"STABILIZATION-3-P: i(%d) post2.add(%s) hashcode(%d) myreplica[1].hc(%d)",i,post2.nodeAddress.getAddress().c_str(),post2.getHashCode(), hasMyReplicas[1].getHashCode());
log->LOG(&memberNode->addr,s);
    /*
     * Topology:
     * pre1 - pre2 - currentNode - post1 - post2
     *
     */
     string a;
     for (int i = 0; i < ring.size(); i++) {
         a += ring[i].nodeAddress.getAddress() + " ";
     }
sprintf(s,"stabilizationP: ringsize(%d) a(%s) hashcode(%d) COunt(%d)",ring.size(),a.c_str(),curHash,COunt);
log->LOG(&memberNode->addr,s);
    if (post1.getHashCode() != hasMyReplicas[0].getHashCode() && post1.getHashCode() == hasMyReplicas[1].getHashCode()) {
        sprintf(s,"post 1  %s",a.c_str());
        log->LOG(&memberNode->addr,s);
        for (auto it : hashT) {
            string value = getValue(it.second);
            int replicaType = getReplicaType(it.second);
            //getValueAndReplicaType(it.second,value,replicaType);

            if (static_cast<ReplicaType>(replicaType) == PRIMARY) {
                Message msg_sec(0,memberNode->addr,UPDATE,it.first,value,SECONDARY);
                Message msg_ter(0,memberNode->addr,CREATE,it.first,value,TERTIARY);

                sendMsg(msg_sec,&(post1.nodeAddress));
                sendMsg(msg_ter,&(post2.nodeAddress));
            }
        }
    }
}




void MP2Node::stabilizationProtocol4() {
        /*
         * Implement this
         */
    int curHash = hashFunction(memberNode->addr.addr);
    int i;
static char ss[256];
    for (i = 0; i < ring.size(); i++) {
        if (curHash == ring.at(i).getHashCode()) {
            break;
        }
    }

    Node pre1 = ring.at( ((i - 2 + ring.size()) % ring.size()) );// 0
    Node pre2 = ring.at( ((i - 1 + ring.size()) % ring.size()) );// 1
    Node post1 = ring.at((i + 1) % ring.size());//1
    Node post2 = ring.at((i + 2) % ring.size());//0

sprintf(s,"STABILIZATIONP: i(%d) pre1.add(%s) hashcode(%d) replicaOf[0].hc(%d)",i,pre1.nodeAddress.getAddress().c_str(),pre1.getHashCode(), haveReplicasOf[0].getHashCode());
log->LOG(&memberNode->addr,s);
sprintf(s,"STABILIZATION-4-P: i(%d) pre2add(%s) hashcode(%d) replicaOf[1].hc(%d)",i,pre2.nodeAddress.getAddress().c_str(),pre2.getHashCode(), haveReplicasOf[1].getHashCode());
log->LOG(&memberNode->addr,s);
sprintf(s,"STABILIZATION-4-P: i(%d) post1.add(%s) hashcode(%d) myreplica[0].hc(%d)",i,post1.nodeAddress.getAddress().c_str(),post1.getHashCode(), hasMyReplicas[0].getHashCode());
log->LOG(&memberNode->addr,s);
sprintf(s,"STABILIZATION-4-P: i(%d) post2.add(%s) hashcode(%d) myreplica[1].hc(%d)",i,post2.nodeAddress.getAddress().c_str(),post2.getHashCode(), hasMyReplicas[1].getHashCode());
log->LOG(&memberNode->addr,s);
    /*
     * Topology:
     * pre1 - pre2 - currentNode - post1 - post2
     *
     */
     string a;
     for (int i = 0; i < ring.size(); i++) {
         a += ring[i].nodeAddress.getAddress() + " ";
     }
sprintf(s,"stabilizationP: ringsize(%d) a(%s) hashcode(%d) COunt(%d)",ring.size(),a.c_str(),curHash,COunt);
log->LOG(&memberNode->addr,s);
    // both post1 and post2 fail
    if (post1.getHashCode() != hasMyReplicas[0].getHashCode() && post2.getHashCode() != hasMyReplicas[1].getHashCode()) {
        sprintf(s,"post 1 2  %s",a.c_str());
        log->LOG(&memberNode->addr,s);
        for (auto it : hashT) {
            string value = getValue(it.second);
            int replicaType = getReplicaType(it.second);
            //getValueAndReplicaType(it.second,value,replicaType);

            if (static_cast<ReplicaType>(replicaType) == PRIMARY) {
                Message msg_sec(0,memberNode->addr,CREATE,it.first,value,SECONDARY);
                Message msg_ter(0,memberNode->addr,CREATE,it.first,value,TERTIARY);

                sendMsg(msg_sec,&(post1.nodeAddress));
                sendMsg(msg_ter,&(post2.nodeAddress));
            }
        }
    }
log->LOG(&memberNode->addr,"LEAVING: stabilizationP:");

}


void MP2Node::stabilizationProtocol5(string key) {
        /*
         * Implement this
         */
    int curHash = hashFunction(memberNode->addr.addr);
    int i;
static char ss[256];
    for (i = 0; i < ring.size(); i++) {
        if (curHash == ring.at(i).getHashCode()) {
            break;
        }
    }
  vector<Node> replicas = findNodes(key);
        for ( vector<Node>::iterator myPos=replicas.begin(); myPos != replicas.end();myPos++){
                sprintf(s,"replica(%s) key(%s)",myPos->nodeAddress.getAddress().c_str(),key.c_str());
                //log->LOG(&memberNode->addr, s);
        }


    Node pre1 = ring.at( ((i - 2 + ring.size()) % ring.size()) );// 0
    Node pre2 = ring.at( ((i - 1 + ring.size()) % ring.size()) );// 1
    Node post1 = ring.at((i + 1) % ring.size());//1
    Node post2 = ring.at((i + 2) % ring.size());//0


string PRE1 = pre1.nodeAddress.getAddress();
string REP0 = haveReplicasOf[0].nodeAddress.getAddress();
string PRE2 = pre2.nodeAddress.getAddress();
string REP1 = haveReplicasOf[1].nodeAddress.getAddress();
string PST1 = post1.nodeAddress.getAddress();
string HMR0= hasMyReplicas[0].nodeAddress.getAddress();
string PST2 = post2.nodeAddress.getAddress();
string HMR1 = hasMyReplicas[1].nodeAddress.getAddress();
sprintf(s,"is terciary of  PRE1(%s)  REPO(%s)  ",PRE1.c_str(),REP0.c_str());
log->LOG(&memberNode->addr,s);
sprintf(s,"is secondary of PRE2(%s)  REP1(%s) ",PRE2.c_str(),REP1.c_str());
log->LOG(&memberNode->addr,s);
sprintf(s,"has secondary PST1(%s)  HMR0(%s) ",PST1.c_str(),HMR0.c_str());
log->LOG(&memberNode->addr,s);
sprintf(s,"has terciary  PST2(%s)  HMR1(%s)", PST2.c_str(),HMR1.c_str());
log->LOG(&memberNode->addr,s);

sprintf(s,"Ring-ReplicaP-5-: i(%d) pre1.add(%s) repAddr(%s) replicaOf[0].hc(%d)",i,pre1.nodeAddress.getAddress().c_str(),haveReplicasOf[0].nodeAddress.getAddress().c_str(), haveReplicasOf[0].getHashCode());
//log->LOG(&memberNode->addr,s);
sprintf(s,"Ring-Replica-5-P: i(%d) pre2.add(%s) repAddr(%s) replicaOf[1].hc(%d)",i,pre2.nodeAddress.getAddress().c_str(),haveReplicasOf[1].nodeAddress.getAddress().c_str(), haveReplicasOf[1].getHashCode());
//log->LOG(&memberNode->addr,s);
sprintf(s,"Ring-Replica-5-P: i(%d) post1.add(%s) MyRepAddr(%s) myreplica[0].hc(%d)",i,post1.nodeAddress.getAddress().c_str(), hasMyReplicas[0].nodeAddress.getAddress().c_str(),hasMyReplicas[0].getHashCode());
//log->LOG(&memberNode->addr,s);
sprintf(s,"Ring-Replica-5-P: i(%d) post2.add(%s) myRepAddr(%s) myreplica[1].hc(%d)",i,post2.nodeAddress.getAddress().c_str(),hasMyReplicas[1].nodeAddress.getAddress().c_str(), hasMyReplicas[1].getHashCode());
//log->LOG(&memberNode->addr,s);
    /*
     * Topology:
     * pre1 - pre2 - currentNode - post1 - post2
     *
     */
     string a;
     for (int i = 0; i < ring.size(); i++) {
         a += ring[i].nodeAddress.getAddress() + " ";
     }
int k=i;
sprintf(s,"stabilizationP: i(%d) ringsize(%d) a(%s) hashcode(%d) COunt(%d)",k,ring.size(),a.c_str(),curHash,COunt);
log->LOG(&memberNode->addr,s);

 if (PRE1.compare(REP0) != 0){
	sprintf(s,"CHANGE-1 PRE1 was (%s) now is (%s)",REP0.c_str(),PRE1.c_str());
	log->LOG(&memberNode->addr,s);
	 for (auto it : hashT) {
		sprintf(s,"hashT (%s)  %s",it.first.c_str(),it.second.c_str());
		//log->LOG(&memberNode->addr,s);
	}

 }
 if (PRE2.compare(REP1) != 0){
	sprintf(s,"CHANGE-2 PRE2 was (%s) now is (%s)",REP1.c_str(),PRE2.c_str());
	log->LOG(&memberNode->addr,s);
 }
 if (PST1.compare(HMR0) != 0){
	sprintf(s,"CHANGE-3(secondary) PST1 was (%s) now is (%s)",HMR0.c_str(),PST1.c_str());
	log->LOG(&memberNode->addr,s);
	 for (auto it : hashT) {
		if( getReplicaType(it.second) == SECONDARY){
                	sprintf(s,"hashT (%s)  %s",it.first.c_str(),it.second.c_str());
                	log->LOG(&memberNode->addr,s);
		}
	}

 }
 if (PST2.compare(HMR1) != 0){
	sprintf(s,"CHANGE-4(terciary) PST1 was (%s) now is (%s)",HMR1.c_str(),PST2.c_str());
	log->LOG(&memberNode->addr,s);
	 for (auto it : hashT) {
		if( getReplicaType(it.second) == TERTIARY){
			string val = getValue(it.second);
                	sprintf(s,"hashT (%s)  %s (%s)",it.first.c_str(),it.second.c_str(),val.c_str());
                	log->LOG(&memberNode->addr,s);
		}
	}

 }
}
