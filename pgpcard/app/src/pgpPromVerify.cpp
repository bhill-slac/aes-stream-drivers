/**
 *-----------------------------------------------------------------------------
 * Title      : PGP Firmware Verify Utility
 * ----------------------------------------------------------------------------
 * File       : pgpPromVerify.cpp
 * Author     : Ryan Herbst, rherbst@slac.stanford.edu
 * Created    : 2016-08-08
 * Last update: 2016-08-08
 * ----------------------------------------------------------------------------
 * Description:
 * Utility to verify the firmware loaded in the PGP Card
 * ----------------------------------------------------------------------------
 * This file is part of the aes_stream_drivers package. It is subject to 
 * the license terms in the LICENSE.txt file found in the top-level directory 
 * of this distribution and at: 
    * https://confluence.slac.stanford.edu/display/ppareg/LICENSE.html. 
 * No part of the aes_stream_drivers package, including this file, may be 
 * copied, modified, propagated, or distributed except according to the terms 
 * contained in the LICENSE.txt file.
 * ----------------------------------------------------------------------------
**/
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <linux/types.h>

#include <fcntl.h>
#include <sstream>
#include <string>
#include <iomanip>
#include <iostream>
#include <string.h>
#include <stdlib.h>
#include <argp.h>

#include <PgpCardProm.h>

using namespace std;

const  char * argp_program_version = "pgpPromVerify 1.0";
const  char * argp_program_bug_address = "rherbst@slac.stanford.edu";

struct PrgArgs {
   const char * path;
   const char * file;
};

static struct PrgArgs DefArgs = { "/dev/pgpcard_0", "" };

static char   args_doc[] = "promFile";
static char   doc[]      = "\n   PromFile is the appropriate .mcs file for the card.";

static struct argp_option options[] = {
   { "path", 'p', "PATH", OPTION_ARG_OPTIONAL, "Path of pgpcard device to use. Default=/dev/pgpcard_0.",0},
   {0}
};

error_t parseArgs ( int key,  char *arg, struct argp_state *state ) {
   struct PrgArgs *args = (struct PrgArgs *)state->input;

   switch(key) {
      case 'p': args->path = arg; break;
      case ARGP_KEY_ARG: 
          switch (state->arg_num) {
             case 0: args->file = arg; break;
             default: argp_usage(state); break;
          }
          break;
      case ARGP_KEY_END: 
          if ( state->arg_num < 1) argp_usage(state);
          break;
      default: return ARGP_ERR_UNKNOWN; break;
   }
   return(0);
}

static struct argp argp = {options,parseArgs,args_doc,doc};

int main (int argc, char **argv) {
   int fd;
   PgpCardProm *prom;
   struct PrgArgs args;

   memcpy(&args,&DefArgs,sizeof(struct PrgArgs));
   argp_parse(&argp,argc,argv,0,0,&args);

   if ( (fd = open(args.path, O_RDWR)) <= 0 ) {
      printf("Error opening %s\n",args.path);
      return(1);
   }

   // Create the PgpCardProm object
   prom = new PgpCardProm(fd,args.file);
   
   // Check if the .mcs file exists
   if(!prom->fileExist()){
      cout << "Error opening: " << args.file << endl;
      delete prom;
      close(fd);
      return(1);   
   }   
   
   // Check if the PCIe device is a generation 2 card
   if(!prom->checkFirmwareVersion()){
      delete prom;
      close(fd);
      return(1);   
   }    

   // Compare the .mcs file with the PROM
   if(!prom->verifyBootProm()) {
      cout << "Error in prom->writeBootProm() function" << endl;
      delete prom;
      close(fd);
      return(1);     
   }

	// Close all the devices
   delete prom;
   close(fd);   
   return(0);
}

