/**
 *-----------------------------------------------------------------------------
 * Title      : DMA read utility
 * ----------------------------------------------------------------------------
 * File       : dmaRead.cpp
 * Author     : Ryan Herbst, rherbst@slac.stanford.edu
 * Created    : 2016-08-08
 * Last update: 2016-08-08
 * ----------------------------------------------------------------------------
 * Description:
 * This program will open up a AXIS DMA port and attempt to read data.
 * ----------------------------------------------------------------------------
 * This file is part of the aes_stream_drivers package. It is subject to 
 * the license terms in the LICENSE.txt file found in the top-level directory 
 * of this distribution and at: 
 *    https://confluence.slac.stanford.edu/display/ppareg/LICENSE.html. 
 * No part of the aes_stream_drivers package, including this file, may be 
 * copied, modified, propagated, or distributed except according to the terms 
 * contained in the LICENSE.txt file.
 * ----------------------------------------------------------------------------
**/

#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>
#include <stdlib.h>
#include <argp.h>
#include <AxisDriver.h>
#include <PrbsData.h>
using namespace std;

const  char * argp_program_version = "dmaRate 1.0";
const  char * argp_program_bug_address = "rherbst@slac.stanford.edu";

struct PrgArgs {
   const char * path;
   uint32_t     getCnt;
};

static struct PrgArgs DefArgs = { "/dev/datadev_0", 1 };

static char   args_doc[] = "";
static char   doc[]      = "";

static struct argp_option options[] = {
   { "path",    'p', "PATH",   OPTION_ARG_OPTIONAL, "Path of pgpcard device to use. Default=/dev/axi_stream_dma_0.",0},
   { "count",   'c', "COUNT",  OPTION_ARG_OPTIONAL, "Number of buffers to get per read.",0},
   {0}
};

error_t parseArgs ( int key,  char *arg, struct argp_state *state ) {
   struct PrgArgs *args = (struct PrgArgs *)state->input;

   switch(key) {
      case 'p': args->path   = arg; break;
      case 'c': args->getCnt = atoi(arg); break;
      default: return ARGP_ERR_UNKNOWN; break;
   }
   return(0);
}

static struct argp argp = {options,parseArgs,args_doc,doc};

int main (int argc, char **argv) {
   uint8_t       mask[DMA_MASK_SIZE];
   int32_t       s;
   int32_t       ret;
   uint32_t      count;
   int32_t       rate;
   fd_set        fds;
   uint32_t      rxFlags[DMA_INDEX_CNT];
   PrbsData      prbs(32,4,1,2,6,31);
   bool          prbRes;
   void **       dmaBuffers;
   uint32_t      dmaSize;
   uint32_t      dmaCount;
   uint32_t      dmaIndex[DMA_INDEX_CNT];
   int32_t       dmaRet[DMA_INDEX_CNT];
   time_t        lTime;
   time_t        cTime;
   int32_t       x;
   int32_t       last;
   float         bw;

   struct PrgArgs args;

   struct timeval timeout;

   memcpy(&args,&DefArgs,sizeof(struct PrgArgs));
   argp_parse(&argp,argc,argv,0,0,&args);

   if ( (s = open(args.path, O_RDWR)) <= 0 ) {
      printf("Error opening %s\n",args.path);
      return(1);
   }

   dmaInitMaskBytes(mask);
   memset(mask,0xFF,DMA_MASK_SIZE);
   dmaSetMaskBytes(s,mask);

   if ( (dmaBuffers = dmaMapDma(s,&dmaCount,&dmaSize)) == NULL ) {
      printf("Failed to map dma buffers!\n");
      return(0);
   }

   bw     = 0;
   rate   = 0;
   count  = 0;
   prbRes = 0;
   time(&lTime);
   do {

      // DMA Read
      ret = dmaReadBulkIndex(s,args.getCnt,dmaRet,dmaIndex,rxFlags,NULL,NULL);

      for (x=0; x < ret; x++) {

         if ( dmaRet[x] > 0 ) {
            count++;
            rate++;
            last = dmaRet[x];
            bw += (last * 8.0);
         }
      }

      if ( ret > 0 ) dmaRetIndexes(s,ret,dmaIndex);

      time(&cTime);
	   if ( cTime != lTime ) {
            printf("Read size=%i, Bulk=%i, prbs=%i, count=%i, rate=%i, bw=%1.3e\n",last,ret,prbRes,count,rate,bw);
	    rate = 0;
       bw   = 0;
	    lTime = cTime;
      }
   } while ( 1 );

   dmaUnMapDma(s,dmaBuffers);

   close(s);
   return(0);
}

