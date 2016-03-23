/*
   Name:           read_binary.cpp
   Created by:     Stefan Ritt <stefan.ritt@psi.ch>
   Date:           July 30th, 2014

   Purpose:        Example file to read binary data saved by DRSOsc.
 
   Compile and run it with:
 
      gcc -o read_binary read_binary.cpp
 
      ./read_binary <filename>

   This program assumes that a pulse from a signal generator is split
   and fed into channels #1 and #2. It then calculates the time difference
   between these two pulses to show the performance of the DRS board
   for time measurements.

   $Id: read_binary.cpp 21834 2015-10-12 11:19:38Z ritt $
*/

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <math.h>

typedef struct {
   char           time_header[4];
   char           bn[2];
   unsigned short board_serial_number;
} THEADER;

typedef struct {
   char           event_header[4];
   unsigned int   event_serial_number;
   unsigned short year;
   unsigned short month;
   unsigned short day;
   unsigned short hour;
   unsigned short minute;
   unsigned short second;
   unsigned short millisecond;
   unsigned short range;
   char           bs[2];
   unsigned short board_serial_number;
   char           tc[2];
   unsigned short trigger_cell;
} EHEADER;

/*-----------------------------------------------------------------------------*/

int main(int argc, const char * argv[])
{
   THEADER th;
   EHEADER eh;
   char hdr[4];
   unsigned short voltage[1024];
   double waveform[4][1024], time[4][1024];
   float bin_width[4][1024];
   int i, j, ch, n, chn_index;
   double t1, t2, dt;
   char filename[256];

   int ndt;
   double threshold, sumdt, sumdt2;
   
   if (argc > 1)
      strcpy(filename, argv[1]);
   else {
      printf("Usage: read_binary <filename>\n");
      return 0;
   }
   
   // open the binary waveform file
   FILE *f = fopen(filename, "r");
   if (f == NULL) {
      printf("Cannot find file \'%s\'\n", filename);
      return 0;
   }

   // read time header
   fread(&th, sizeof(th), 1, f);
   printf("Found data for board #%d\n", th.board_serial_number);

   // read time bin widths
   memset(bin_width, sizeof(bin_width), 0);
   for (ch=0 ; ch<5 ; ch++) {
      fread(hdr, sizeof(hdr), 1, f);
      if (hdr[0] != 'C') {
         // event header found
         fseek(f, -4, SEEK_CUR);
         break;
      }
      i = hdr[3] - '0' - 1;
      printf("Found timing calibration for channel #%d\n", i+1);
      fread(&bin_width[i][0], sizeof(float), 1024, f);
      // fix for 2048 bin mode: double channel
      if (bin_width[i][1023] > 10 || bin_width[i][1023] < 0.01) {
         for (j=0 ; j<512 ; j++)
            bin_width[i][j+512] = bin_width[i][j];
      }
   }
   
   // initialize statistics
   ndt = 0;
   sumdt = sumdt2 = 0;
   
   // loop over all events in the data file
   for (n= 0 ; ; n++) {
      // read event header
      i = (int)fread(&eh, sizeof(eh), 1, f);
      if (i < 1)
         break;
      
      printf("Found event #%d\n", eh.event_serial_number);
      
      // reach channel data
      for (ch=0 ; ch<5 ; ch++) {
         i = (int)fread(hdr, sizeof(hdr), 1, f);
         if (i < 1)
            break;
         if (hdr[0] != 'C') {
            // event header found
            fseek(f, -4, SEEK_CUR);
            break;
         }
         chn_index = hdr[3] - '0' - 1;
         fread(voltage, sizeof(short), 1024, f);
         
         for (i=0 ; i<1024 ; i++) {
            // convert data to volts
            waveform[chn_index][i] = (voltage[i] / 65536. + eh.range/1000.0 - 0.5);
            
            // calculate time for this cell
            for (j=0,time[chn_index][i]=0 ; j<i ; j++)
               time[chn_index][i] += bin_width[chn_index][(j+eh.trigger_cell) % 1024];
         }
      }
      
      
      // align cell #0 of all channels
      t1 = time[0][(1024-eh.trigger_cell) % 1024];
      for (ch=1 ; ch<4 ; ch++) {
         t2 = time[ch][(1024-eh.trigger_cell) % 1024];
         dt = t1 - t2;
         for (i=0 ; i<1024 ; i++)
            time[ch][i] += dt;
      }
      
      t1 = t2 = 0;
      threshold = 0.3;
      
      // find peak in channel 1 above threshold
      for (i=0 ; i<1022 ; i++)
         if (waveform[0][i] < threshold && waveform[0][i+1] >= threshold) {
            t1 = (threshold-waveform[0][i])/(waveform[0][i+1]-waveform[0][i])*(time[0][i+1]-time[0][i])+time[0][i];
            break;
         }
      
      // find peak in channel 2 above threshold
      for (i=0 ; i<1022 ; i++)
         if (waveform[1][i] < threshold && waveform[1][i+1] >= threshold) {
            t2 = (threshold-waveform[1][i])/(waveform[1][i+1]-waveform[1][i])*(time[1][i+1]-time[1][i])+time[1][i];
            break;
         }
      
      // calculate distance of peaks with statistics
      if (t1 > 0 && t2 > 0) {
         ndt++;
         dt = t2 - t1;
         sumdt += dt;
         sumdt2 += dt*dt;
      }
   }
   
   // print statistics
   printf("dT = %1.3lfns +- %1.1lfps\n", sumdt/ndt, 1000*sqrt(1.0/(ndt-1)*(sumdt2-1.0/ndt*sumdt*sumdt)));
   
   return 1;
}

