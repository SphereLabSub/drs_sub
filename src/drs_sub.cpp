/********************************************************************\

  Name:         drs_sub.cpp
  Created by:   Stefan Ritt
  Modified by:  Stefano veneziano

  Contents:     Simple example application to read out a DRS4 
                evaluation board and 
                write a ROOT output file
                

  $Id: drs_sub.cpp $

\********************************************************************/


#ifdef _MSC_VER

#include <windows.h>

#elif defined(OS_LINUX)

#define O_BINARY 0

#include <unistd.h>
#include <ctype.h>
#include <sys/ioctl.h>
#include <errno.h>

#define DIR_SEPARATOR '/'

#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <iostream>     // std::cout
#include <fstream>      // std::ifstream
#include <sstream>      // std::ifstream

#include <math.h>
#include "TCanvas.h"
#include "TROOT.h"
#include "TGraphErrors.h"
#include "TF1.h"
#include "TH1F.h"
#include "TLegend.h"
#include "TArrow.h"
#include "TSystem.h"
#include "TBranch.h"
#include "TTree.h"
#include "TFile.h"
#include "TRint.h"
#include <TApplication.h>

#include "strlcpy.h"
#include "DRS.h"

using namespace std;

/*------------------------------------------------------------------*/

int main(int argc, char* argv[]){
    //cout << "no. of arguments: " << argc << endl;
    if (argc !=8) {
        cout << argv[0] << ": Usage"<< endl;
        cout << argv[0] << " filename nchans" << endl;
        cout << "          nEvents  : Number of events Read Out " << endl;
        cout << "          [chlist] : Optional, list of Channels to be plotted. Example 0,1,3" << endl;
        cout << "          delay    : Trigger delay in ns" << endl;
        cout << "          thresh   : Threshold in mV" << endl;
        cout << "          edge     : Threshold edge [pos=rising edge, neg=falling edge]" << endl;
        cout << "          source   : trigger source [0=ch1, 1=ch2, 2=ch3, 3=ch4, 4=ext]" << endl;
        cout << " Example: drs_sub pippo 1000 0 100 25.0 pos 0" << endl;
        return 0;
    }
    
  const int MAXCH = 4;

  struct myEvent {
    int trigId;
    int channels;
    int id[MAXCH];
    float time_array[MAXCH][1024];
    float wave_array[MAXCH][1024];
  };
    //readBin(argv[1],atoi(argv[2]));
    char *fileName = argv[1];
    int nEvents = atoi(argv[2]);
    int delayNs = atoi(argv[4]);
    float threshMV = atof(argv[5]);
    bool triggerEdge;
    if (strncmp(argv[6],"neg",3)==0) triggerEdge = true;
    else if (strncmp(argv[6],"pos",3)==0) triggerEdge = false;
    int triggerSource = atoi(argv[7]);

    bool chanToPlot[128];
    //TRint* rootapp = new TRint("example",&argc, argv);    //readBin(argv[1]);
    //cout << "number of arguments is " << argc << endl;
    if (argc==8) {
        
      for (int i=0; i<MAXCH; i++)
	{
	  chanToPlot[i]=false;
        }
	std::string input = argv[3];
        std::istringstream myss(input);
        std::string token;
        
        while(std::getline(myss, token, ',')) {
            //std::cout << token << '\n';
            int thechan = atoi(token.c_str());
            chanToPlot[thechan] = true;
            cout << "channel " << thechan << " will be recorded" << endl;
        }

    }

    //Scrittura file ROOT
  struct myEvent ev;

  TTree *tree = new TTree("t1", "title");
  
  char branchDef[130];
  TBranch * b_trigId = tree->Branch("trigId", &ev.trigId, "trigId/I");
  TBranch * b_channels   = tree->Branch("channels", &ev.channels, "channels/I");
  
  sprintf(branchDef,"id[%d]/I",MAXCH);
  TBranch * b_id   = tree->Branch("id", ev.id, branchDef);

  sprintf(branchDef,"time_array[%d][1024]/F",MAXCH);
  TBranch * b_time_array   = tree->Branch("time_array", &ev.time_array[0][0], branchDef);

  sprintf(branchDef,"wave_array[%d][1024]/F",MAXCH);
  TBranch * b_wave_array   = tree->Branch("wave_array", &ev.wave_array[0][0], branchDef);

  char rootFileName[130];
  sprintf(rootFileName,"%s.root",fileName);
  TFile f1(rootFileName,"RECREATE");
  Int_t comp   = 0;
  f1.SetCompressionLevel(comp);
 
   int i, j, nBoards;
   DRS *drs;
   DRSBoard *b;
   //float time_array[8][1024];
   //float wave_array[8][1024];
   FILE  *f;

   /* do initial scan */
   drs = new DRS();

   /* show any found board(s) */
   for (i=0 ; i<drs->GetNumberOfBoards() ; i++) {
      b = drs->GetBoard(i);
      printf("Found DRS4 evaluation board, serial #%d, firmware revision %d\n", 
         b->GetBoardSerialNumber(), b->GetFirmwareVersion());
   }

   /* exit if no board found */
   nBoards = drs->GetNumberOfBoards();
   if (nBoards == 0) {
      printf("No DRS4 evaluation board found\n");
      return 0;
   }

   /* continue working with first board only */
   b = drs->GetBoard(0);

   /* initialize board */
   b->Init();

   /* set sampling frequency */
   b->SetFrequency(1, true);

   /* enable transparent mode needed for analog trigger */
   b->SetTranspMode(1);

   /* set input range to -0.5V ... +0.5V */
   b->SetInputRange(0);

   /* use following line to set range to 0..1V */
   //b->SetInputRange(0.5);
   
   /* use following line to turn on the internal 100 MHz clock connected to all channels  */
   //b->EnableTcal(1);

   /* use following lines to enable hardware trigger on CH1 at 50 mV positive edge */
   if (b->GetBoardType() >= 8) {        // Evaluaiton Board V4&5
      b->EnableTrigger(1, 0);           // enable hardware trigger
      b->SetTriggerSource(1<<triggerSource);        // set input trigger source
   } else if (b->GetBoardType() == 7) { // Evaluation Board V3
      b->EnableTrigger(0, 1);           // lemo off, analog trigger on
      b->SetTriggerSource(0);           // use CH1 as source
   }
   b->SetTriggerLevel(threshMV/1000.);            // threshold is in V
   b->SetTriggerPolarity(triggerEdge);        // positive edge
   
   /* use following lines to set individual trigger elvels */
   //b->SetIndividualTriggerLevel(1, 0.1);
   //b->SetIndividualTriggerLevel(2, 0.2);
   //b->SetIndividualTriggerLevel(3, 0.3);
   //b->SetIndividualTriggerLevel(4, 0.4);
   //b->SetTriggerSource(15);
   
   b->SetTriggerDelayNs(delayNs);             // ns trigger delay
   
   /* use following lines to enable the external trigger */
   //if (b->GetBoardType() == 8) {     // Evaluaiton Board V4
   //   b->EnableTrigger(1, 0);           // enable hardware trigger
   //   b->SetTriggerSource(1<<4);        // set external trigger as source
   //} else {                          // Evaluation Board V3
   //   b->EnableTrigger(1, 0);           // lemo on, analog trigger off
   // }

   /* open file to save waveforms */
   f = fopen("data.txt", "w");
   if (f == NULL) {
      perror("ERROR: Cannot open file \"data.txt\"");
      return 1;
   }
   
   /* repeat ten times */
   for (j=0 ; j<nEvents ; j++) {

      /* start board (activate domino wave) */
      b->StartDomino();

      /* wait for trigger */
      if(j==0) cout << "Waiting for trigger..." << endl;
      
      //fflush(stdout);
      while (b->IsBusy());

      if(j==0) cout << "Trigger found, reading data..." << endl;

      ev.trigId = j;

      /* read all waveforms */
      b->TransferWaves(0, 8);

      int currentChannel = 0;
      for (int ch=0; ch<MAXCH; ch++) {
         if (chanToPlot[ch]==true) {
            ev.id[currentChannel++] = ch;

            /* read time (X) array of first channel in ns */
            b->GetTime(0, 2*ch, b->GetTriggerCell(0), ev.time_array[ch]);

            /* decode waveform (Y) array of first channel in mV */
            b->GetWave(0, 2*ch, ev.wave_array[ch]);

            /* read time (X) array of second channel in ns
             Note: On the evaluation board input #1 is connected to channel 0 and 1 of
             the DRS chip, input #2 is connected to channel 2 and 3 and so on. So to
             get the input #2 we have to read DRS channel #2, not #1. */

            /* decode waveform (Y) array of second channel in mV */
          }
      }
      ev.channels = currentChannel;
      /* print some progress indication */
      if((j%(nEvents/10))==0) cout << "Event #" << j << " read successfully" << endl;
      tree->Fill();
   }

   tree->Write();
   
   /* delete DRS object -> close USB connection */
   delete drs;
}
