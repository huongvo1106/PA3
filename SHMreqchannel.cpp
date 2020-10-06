#include "common.h"
#include "SHMreqchannel.h"
using namespace std;


/*--------------------------------------------------------------------------*/
/* CONSTRUCTOR/DESTRUCTOR FOR CLASS   R e q u e s t C h a n n e l  */
/*--------------------------------------------------------------------------*/

SHMRequestChannel::SHMRequestChannel(const string _name, const Side _side, int _len) : RequestChannel (_name, _side){
	s1 = "/SHM_" + my_name + "1";
	s2 = "/SHM_" + my_name + "2";
	len = _len;
		
	share_mem_queue_1 = new SHMQ(s1, len); //share memory queue
	share_mem_queue_2 = new SHMQ(s2, len); //share memory queue

	if (my_side == CLIENT_SIDE) {
		swap(share_mem_queue_1, share_mem_queue_2);
	}
}

SHMRequestChannel::~SHMRequestChannel(){ 
	delete share_mem_queue_1;
	delete share_mem_queue_2;
}


int SHMRequestChannel::cread(void* msgbuf, int bufcapacity){
	return share_mem_queue_1->my_shm_recv(msgbuf, bufcapacity); 
}

int SHMRequestChannel::cwrite(void* msgbuf, int len){
	return share_mem_queue_2->my_shm_send(msgbuf, len);
}

