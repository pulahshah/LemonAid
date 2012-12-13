#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>   // exit
#include <signal.h>
#include <string.h>
#include <stdlib.h>   // strtoul
#include <fcntl.h>    // O_RDWR
#include <unistd.h>

#include <libpcan.h>
#include <src/common.h>
#include <ctype.h>
#include <src/parser.h>

//****************************************************************************
// DEFINES

#define DEFAULT_NODE "/dev/pcanusb0"

//****************************************************************************
// GLOBALS
HANDLE h;
const char *current_release;
std::list<TPCANMsg> *List;
int nExtended = CAN_INIT_TYPE_ST;

//****************************************************************************
// CODE

// do, what has to be done at programm exit
void do_exit(int error)
{
  if (h)
  {
    print_diag("cantransmitter");
    CAN_Close(h);
  }
  printf("cantransmitter: finished (%d).\n\n", error);
  exit(error);
}

// handle manual break signals
void signal_handler(int signal)
{
  do_exit(0);
}

// do, what has to be done at program init
void init()
{
  // install signal handlers
  signal(SIGTERM, signal_handler);
  signal(SIGINT, signal_handler);
}

// loop writing to CAN-Bus
int write_loop(__u32 dwMaxTimeInterval, __u32 dwMaxLoop)
{
  __u32 l;
  // write out endless loop until Ctrl-C
  double scale = (dwMaxTimeInterval * 1000.0) / (RAND_MAX + 1.0);

  //while (1)
  for (l=0; ; l++)
  {
    std::list<TPCANMsg>::iterator iter;
    int i;

    if (dwMaxLoop)
		if (l >= dwMaxLoop) break;

    for (iter = List->begin(); iter != List->end(); iter++)
    {
      // test for standard frames only
      if ((nExtended == CAN_INIT_TYPE_EX) || !(iter->MSGTYPE & MSGTYPE_EXTENDED))
      {
        // send the message
	//sleep(1);
        if ((errno = CAN_Write(h, &(*iter))))
        {
          perror("cantransmitter: CAN_Write()");
          return errno;
        }
        // wait some time before the invocation
        if (dwMaxTimeInterval)
          usleep((__useconds_t)(scale * rand()));
      }
    }
  }

  return 0;
}

static void hlpMsg(void)
{
  printf("cantransmitter - a small test program which sends CAN messages.\n");
  printf("usage:   cantransmitter filename {[-f=devicenode] | {[-t=type] [-p=port [-i=irq]]}} [-b=BTR0BTR1] [-e] [-r=msec] [-?]\n");
  printf("options: filename - mandatory name of message description file.\n");
  printf("         -f - devicenode - path to devicefile, default=%s\n", DEFAULT_NODE);
  printf("         -t - type of interface, e.g. 'pci', 'sp', 'epp' ,'isa', 'pccard' or 'usb' (default: pci).\n");
  printf("         -p - port in hex notation if applicable, e.g. 0x378 (default: 1st port of type).\n");
  printf("         -i - irq in dec notation if applicable, e.g. 7 (default: irq of 1st port).\n");
  printf("         -b - BTR0BTR1 code in hex, e.g. 0x001C (default: 500 kbit).\n");
  printf("         -e - accept extended frames. (default: standard frames)\n");
  printf("         -r - messages are send at random times with maximum time in milliseconds (msec)\n");
  printf("         -? or --help - this help\n");
  printf("\n");
}

int main(int argc, char *argv[])
{
  char *ptr;
  int i;
  int nType = HW_PCI;
  __u32 dwPort = 0;
  __u16 wIrq = 0;
  __u16 wBTR0BTR1 = 0;
  __u32 dwMaxTimeInterval = 0, dwMaxLoop = 0;
  char *filename = NULL;
  const char *szDevNode = DEFAULT_NODE;
  bool bDevNodeGiven = false;
  bool bTypeGiven = false;
  parser MyParser;
  char txt[VERSIONSTRING_LEN];

  errno = 0;

  current_release = "rttep";
  disclaimer("cantransmitter");

  init();

  // decode command line arguments
  for (i = 1; i < argc; i++) 
  {
    char c;

    ptr = argv[i];

    if (*ptr == '-') 
    {
      while (*ptr == '-')
        ptr++;

      c = *ptr;
      ptr++;

      if (*ptr == '=')
      ptr++;

      switch(tolower(c)) 
      {
        case 'f':
          szDevNode = ptr;
          bDevNodeGiven = true;
          break;
        case 't':
          nType = getTypeOfInterface(ptr);
          if (!nType) 
          {
            errno = EINVAL;
            printf("cantransmitter: unknown type of interface\n");
            goto error;
          }
          bTypeGiven = true;
          break;
        case 'p':
          dwPort = strtoul(ptr, NULL, 16);
          break;
        case 'i':
          wIrq   = (__u16)strtoul(ptr, NULL, 10);
          break;
        case 'e':
          nExtended = CAN_INIT_TYPE_EX;
          break;
        case '?': 
        case 'h':
          hlpMsg();
          goto error;
          break;
        case 'b':
          wBTR0BTR1 = (__u16)strtoul(ptr, NULL, 16);
          break;
        case 'r':
          dwMaxTimeInterval = strtoul(ptr, NULL, 10);
          break;
        case 'n':
          dwMaxLoop = strtoul(ptr, NULL, 10);
          break;
        default:
          errno = EINVAL;
          printf("cantransmitter: unknown command line argument\n");
          goto error;
          break;
      }
    }
    else
      filename = ptr;
  }

  // test for filename
  if (filename == NULL) 
  {
    errno = EINVAL;
    perror("cantransmitter: no filename given");
    goto error;
  }

  // test device node and type
  if (bDevNodeGiven && bTypeGiven) 
  {
    errno = EINVAL;
    perror("cantransmitter: device node and type together is useless");
    goto error;
  }

  // give the filename to my parser
  MyParser.setFileName(filename);

  // tell some information to the user
  if (!bTypeGiven) 
  {
    printf("cantransmitter: device node=\"%s\"\n", szDevNode);
  }
  else {
    printf("cantransmitter: type=%s", getNameOfInterface(nType));
    if (nType == HW_USB) 
      printf(", Serial Number=default, Device Number=%d\n", dwPort); 
    else {
      if (dwPort) 
      {
        if (nType == HW_PCI)
          printf(", %d. PCI device", dwPort);
        else
          printf(", port=0x%08x", dwPort);
      }
      else
        printf(", port=default");
      
      if ((wIrq) && !(nType == HW_PCI))
        printf(" irq=0x%04x\n", wIrq);
      else
        printf(", irq=default\n");
    }
  }

  if (nExtended == CAN_INIT_TYPE_EX)
    printf("             Extended frames are sent");
  else
    printf("             Only standard frames are sent");
  
  if (wBTR0BTR1)
    printf(", init with BTR0BTR1=0x%04x\n", wBTR0BTR1);
  else
    printf(", init with 500 kbit/sec.\n");
    printf("             Data will be read from \"%s\".\n", filename);

  if (dwMaxTimeInterval)
    printf("             Messages are send in random time intervalls with a max. gap time of %d msec.\n", dwMaxTimeInterval);
  
  /* get the list of data from parser */
  List = MyParser.Messages();
  if (!List)
  {
    errno = MyParser.nGetLastError();
    perror("cantransmitter: error at file read");
    goto error;
  }
  
  /* open CAN port */
  if ((bDevNodeGiven) || (!bDevNodeGiven && !bTypeGiven)) 
  {
    h = LINUX_CAN_Open(szDevNode, O_RDWR);
    if (!h)
    {
      printf("cantransmitter: can't open %s\n", szDevNode);
      goto error;
    }
  }
  else {
    // please use what is appropriate  
    // HW_DONGLE_SJA 
    // HW_DONGLE_SJA_EPP 
    // HW_ISA_SJA 
    // HW_PCI 
    h = CAN_Open(nType, dwPort, wIrq);
    if (!h)
    {
      printf("cantransmitter: can't open %s device.\n", getNameOfInterface(nType));
      goto error;
    }
  }

  /* clear status */
  CAN_Status(h);
  
  // get version info
  errno = CAN_VersionInfo(h, txt);
  if (!errno)
    printf("cantransmitter: driver version = %s\n", txt);
  else {
    perror("cantransmitter: CAN_VersionInfo()");
    goto error;
  }
  
  // init to a user defined bit rate
  if (wBTR0BTR1) 
  {
    errno = CAN_Init(h, wBTR0BTR1, nExtended);
    if (errno) 
    {
      perror("cantransmitter: CAN_Init()");
      goto error;
    }
  }
  // enter in the write loop
  errno = write_loop(dwMaxTimeInterval, dwMaxLoop);
  if (!errno)
    return 0;

  error:
    do_exit(errno);
    return errno;
}
