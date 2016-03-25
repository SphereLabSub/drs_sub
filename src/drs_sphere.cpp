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

int main()
{
    
    struct config_param par;
    
    bool chanToPlot[128];
    
    int currentChannel = 0;
    int ch;
    int i;
    int j;
    int nBoards;
    int nmeasure = 0;
    
    
    struct myEvent ev;
    struct interface interfaccia = {0};
    
    char branchDef[130];
    char rootFileName[130];
    char command[10];
    
    DRS *drs;
    DRSBoard *b;
    
    
    if((fp = fopen("config.txt","r+")) == NULL)
    {
        perror("ERROR: Cannot open file \"config.txt\"");
        return 1;
    }
    /*Trigger edge true it's negative edge, false it's positive*/
    fscanf(fp,"%d %s %d %f %B %d %d",&(par.nEvents), par.filename, &(par.delayNs), &(par.treshMV), &(par.triggerEdge), &(par.channel),&(par.trigger_source));
    
    fclose(fp);
    


    for (i=0; i<MAXCH; i++)
    {
        chanToPlot[i]=false;
    
    }

    chanToPlot[par.channel] = true;

   while(!(interface.quit))
   {
       cout << "Put a command between start, quit" << endl;
       scanf("%s",command);
       switch(command)
       {
           case start:
               interface.start = 1;
               interface.quit  = 0;
               break;
           
           case quit :
               interface.quit = 1;
               interface.start = 0;
               break;
   }
   if((interface.start) == 1)
   {
       nmeasure ++;
       
       TTree *tree = new TTree("t1", "title");
       
       
       TBranch * b_trigId = tree->Branch("trigId", &ev.trigId, "trigId/I");
       TBranch * b_channels   = tree->Branch("channels", &ev.channels, "channels/I");
       
       sprintf(branchDef,"id[%d]/I",MAXCH);
       TBranch * b_id   = tree->Branch("id", ev.id, branchDef);
       
       sprintf(branchDef,"time_array[%d][1024]/F",MAXCH);
       TBranch * b_time_array   = tree->Branch("time_array", &ev.time_array[0][0], branchDef);
       
       sprintf(branchDef,"wave_array[%d][1024]/F",MAXCH);
       TBranch * b_wave_array   = tree->Branch("wave_array", &ev.wave_array[0][0], branchDef);
       
       
       sprintf(rootFileName,"%s_%d.root",nmeasure,par.filename);
       TFile f1(rootFileName,"RECREATE");
       Int_t comp   = 0;
       f1.SetCompressionLevel(comp);
       
       
       
    /* do initial scan */
   drs = new DRS();

   /* show any found board(s) */
   for (i=0 ; i<drs->GetNumberOfBoards() ; i++)
   {
       b = drs->GetBoard(i);
       cout << "Found DRS4 evaluation board, serial #%d, firmware revision" << endl;
       b->GetBoardSerialNumber(), b->GetFirmwareVersion());
   }

   /* exit if no board found */
   nBoards = drs->GetNumberOfBoards();
   if (nBoards == 0)
   {
       cout << "No DRS4 evaluation board found " << endl;
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
   if (b->GetBoardType() >= 8)
   {        // Evaluaiton Board V4&5
      b->EnableTrigger(1, 0);           // enable hardware trigger
      b->SetTriggerSource(1<<triggerSource);        // set input trigger source
   }
   else if (b->GetBoardType() == 7)
   { // Evaluation Board V3
      b->EnableTrigger(0, 1);           // lemo off, analog trigger on
      b->SetTriggerSource(0);           // use CH1 as source
   }
   b->SetTriggerLevel(par.threshMV/1000.);            // threshold is in V
   b->SetTriggerPolarity(par.triggerEdge);        // positive edge
   
   /* use following lines to set individual trigger elvels */
   //b->SetIndividualTriggerLevel(1, 0.1);
   //b->SetIndividualTriggerLevel(2, 0.2);
   //b->SetIndividualTriggerLevel(3, 0.3);
   //b->SetIndividualTriggerLevel(4, 0.4);
   //b->SetTriggerSource(15);
   
   b->SetTriggerDelayNs(par.delayNs);             // ns trigger delay
   
   /* use following lines to enable the external trigger */
   //if (b->GetBoardType() == 8) {     // Evaluaiton Board V4
   //   b->EnableTrigger(1, 0);           // enable hardware trigger
   //   b->SetTriggerSource(1<<4);        // set external trigger as source
   //} else {                          // Evaluation Board V3
   //   b->EnableTrigger(1, 0);           // lemo on, analog trigger off
   // }

   /* open file to save waveforms */

   if ((f=fopen("data.txt", "w+")) == NULL)
   {
       perror("ERROR: Cannot open file \"data.txt\"");
       return 1;
   }
   
   /* repeat nEvents times */
   for (j=0 ; j<(par.nEvents) ; j++)
   {
      /* start board (activate domino wave) */
      b->StartDomino();

      /* wait for trigger */
       if(j==0)
       {
           cout << "Waiting for trigger..." << endl;
       }
      //fflush(stdout);
      while (b->IsBusy());

      if(j==0)
      {
          cout << "Trigger found, reading data..." << endl;
      }
      ev.trigId = j;

      /* read all waveforms */
      b->TransferWaves(0, 8);

 
      for (ch=0; ch<MAXCH; ch++)
      {
         if (chanToPlot[ch]==true)
         {
             
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
      
      if((j%(par.nEvents/10))==0)
      {
          cout << "Event #" << j << " read successfully" << endl;
      }
      
       tree->Fill();
    }

   tree->Write();
   
    
   /* delete DRS object -> close USB connection */
   delete drs;
   fclose(f);
       
    }
   }
    cout << "Closing interface" << endl;
  }
}
