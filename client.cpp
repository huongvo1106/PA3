/*
    Tanzir Ahmed
    Department of Computer Science & Engineering
    Texas A&M University
    Date  : 2/8/19
 */
#include "common.h"
#include <sys/wait.h>
#include "FIFOreqchannel.h"
#include "MQreqchannel.h"
#include "SHMreqchannel.h"

using namespace std;


int main(int argc, char *argv[]){
    
    int c;
    int buffercap = MAX_MESSAGE;
    int p = 0, ecg = 1;
    double t = -1.0;
    bool isnewchan = true;
    bool isfiletransfer = false;
    string filename;
    string ipcmethod = "f";
    int nchannels = 1;


    while ((c = getopt (argc, argv, "p:t:e:m:f:c:i:")) != -1){
        switch (c){
            case 'p':
                p = atoi (optarg);
                break;
            case 't':
                t = atof (optarg);
                break;
            case 'e':
                ecg = atoi (optarg);
                break;
            case 'm':
                buffercap = atoi (optarg);
                break;
            case 'i':
                ipcmethod = optarg;
                break;
            case 'c':
                isnewchan = true;
                nchannels = atoi(optarg);
                break;
            case 'f':
                isfiletransfer = true;
                filename = optarg;
                break;
            
        }
    }
    
    // fork part
    
    
    if (fork()==0){ // child 
    
        char* args [] = {"./server", "-m", (char *) to_string(buffercap).c_str(), "-i", (char *) ipcmethod.c_str(), NULL};
        if (execvp (args [0], args) < 0){
            perror ("exec filed");
            exit (0);
        }
    }
    RequestChannel* control_chan = NULL;    
    
    if (ipcmethod == "f") 
        control_chan = new FIFORequestChannel ("control", RequestChannel::CLIENT_SIDE);
    else if (ipcmethod == "q") 
        control_chan = new MQRequestChannel ("control", RequestChannel::CLIENT_SIDE);
    else if (ipcmethod == "m") 
        control_chan = new SHMRequestChannel ("control", RequestChannel::CLIENT_SIDE, buffercap);
    
    
    
    vector <RequestChannel*> store_child(0);
    if (isnewchan){
        cout << "Using the new channel everything following" << endl;
        MESSAGE_TYPE m = NEWCHANNEL_MSG;
        control_chan->cwrite (&m, sizeof (m));
        char newchanname [100];
        control_chan->cread (newchanname, sizeof (newchanname));
        for(int i = 0; i < nchannels; ++i){
            RequestChannel* chan = control_chan;
            if (ipcmethod == "f") 
                chan = new FIFORequestChannel (newchanname, RequestChannel::CLIENT_SIDE);
            else if (ipcmethod == "q") 
                chan = new MQRequestChannel (newchanname, RequestChannel::CLIENT_SIDE);
            else if (ipcmethod == "m") 
                chan = new SHMRequestChannel (newchanname, RequestChannel::CLIENT_SIDE, buffercap);
            //chan[200];
            store_child.push_back(chan);

            //chan = new FIFORequestChannel (newchanname, FIFORequestChannel::CLIENT_SIDE);
            cout << "New channel by the name " << newchanname << " is created" << endl;
            cout << "All further communication will happen through it instead of the main channel" << endl;
        }
    }
    if (!isfiletransfer){   // requesting data msgs
        
        if (t >= 0){    // 1 data point
            datamsg d (p, t, ecg);
            store_child[0]->cwrite (&d, sizeof (d));
            double ecgvalue;
            store_child[0]->cread (&ecgvalue, sizeof (double));
            cout << "Ecg " << ecg << " value for patient "<< p << " at time " << t << " is: " << ecgvalue << endl;
        }else{          // bulk (i.e., 1K) data requests 
            double ts = 0;  
            datamsg d (p, ts, ecg);
            double ecgvalue;
            for(int i = 0; i < nchannels; ++i) {
                int j = 0;
                cout << "Channel " << i + 1 << endl;
                while(j < 1000){
                    store_child[i]->cwrite (&d, sizeof (d));
                    store_child[i]->cread (&ecgvalue, sizeof (double));
                    d.seconds += 0.004; //increment the timestamp by 4ms
                    cout << ecgvalue << endl;
                    j++;
                }
            }
        }
        
    }
    else if (isfiletransfer){
        // part 2 requesting a file
        filemsg f (0,0);  // special first message to get file size
        int to_alloc = sizeof (filemsg) + filename.size() + 1; // extra byte for NULL
        char* buf = new char [to_alloc];
        
        memcpy (buf, &f, sizeof(filemsg));
        strcpy (buf + sizeof (filemsg), filename.c_str());
        
        store_child[0]->cwrite (buf, to_alloc);
        __int64_t filesize;
        store_child[0]->cread (&filesize, sizeof (__int64_t));
        cout << "File size: " << filesize << endl;

        //int transfers = ceil (1.0 * filesize / MAX_MESSAGE);
        filemsg* fm = (filemsg*) buf;
        __int64_t rem = filesize;
        string outfilepath = string("received/") + filename;
        FILE* outfile = fopen (outfilepath.c_str(), "wb");  
        fm->offset = 0;

        char* recv_buffer = new char [MAX_MESSAGE];
        while (rem>0){
            fm->length = (int) min (rem, (__int64_t) MAX_MESSAGE);    
            store_child[0]->cwrite (buf, to_alloc);
            store_child[0]->cread (recv_buffer, MAX_MESSAGE);
            

            fwrite (recv_buffer, 1, fm->length, outfile);
            
            rem -= fm->length;
            fm->offset += fm->length;
            // cout << fm->offset << endl;
        }
        fclose (outfile);
        delete recv_buffer;
        delete buf;
        cout << "File transfer completed" << endl;
    }

    
    MESSAGE_TYPE q = QUIT_MSG;

    // if (chan != control_chan){ // this means that the user requested a new channel, so the control_channel must be destroyed as well 
    
    
    for(int i = 0; i < store_child.size(); ++i) {
        store_child[i]->cwrite (&q, sizeof (MESSAGE_TYPE));
        delete store_child[i];       
    }
    control_chan->cwrite (&q, sizeof (MESSAGE_TYPE));
    delete control_chan;
    
    // }
    wait(0);
        
    
    
	// wait for the child process running server
    // this will allow the server to properly do clean up
    // if wait is not used, the server may sometimes crash
    
}
