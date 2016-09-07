

#include <stdio.h>
#include <windows.h>
#include <HexFile.h>

// http://www.keil.com/support/docs/1584/

// Record Format
// An Intel HEX file is composed of any number of HEX records. Each record is made up of five fields that are arranged in the following format:

// :llaaaatt[dd...]cc
// Each group of letters corresponds to a different field, and each letter represents a single hexadecimal digit. Each field is composed of at least two hexadecimal digits-which make up a byte-as described below:

// : is the colon that starts every Intel HEX record.
// ll is the record-length field that represents the number of data bytes (dd) in the record.
// aaaa is the address field that represents the starting address for subsequent data in the record.
// tt is the field that represents the HEX record type, which may be one of the following:
// 00 - data record
// 01 - end-of-file record
// 02 - extended segment address record
// 04 - extended linear address record
// 05 - start linear address record (MDK-ARM only)
// dd is a data field that represents one byte of data. A record may have multiple data bytes. The number of data bytes in the record must match the number specified by the ll field.
// cc is the checksum field that represents the checksum of the record. The checksum is calculated by summing the values of all hexadecimal digit pairs in the record modulo 256 and taking the two's complement.



//Data Records
//
//The Intel HEX file is made up of any number of data records that are terminated with a carriage return and a linefeed. Data records appear as follows:
//:10246200464C5549442050524F46494C4500464C33
//
//This record is decoded as follows:
//:10246200464C5549442050524F46494C4500464C33
//|||||||||||                              CC->Checksum
//|||||||||DD->Data
//|||||||TT->Record Type
//|||AAAA->Address
//|LL->Record Length
//:->Colon
//where:
//    * 10 is the number of data bytes in the record.
//    * 2462 is the address where the data are to be located in memory.
//    * 00 is the record type 00 (a data record).
//    * 464C...464C is the data.
//    * 33 is the checksum of the record.



#define LOGERR(mm) fprintf(stderr,mm);

#define MAXSEG 1000

int scanHex(char *hexfl, UINT32 **seglst){
  UINT32 strans=0,ll=0, lcnt = 0, addr=0, scnt=0, ssz[MAXSEG]={0};
  UINT32 curlen=0, curaddr=0, extAddr=0, startAddr=0,dcnt=0, *sptr;
  UINT8 loadState = 0, typ=99,chks=0,trm=0;
  char line[MAXLN];
  
  FILE *fp=fopen(hexfl,"r");
  if(!fp){
    fprintf(stderr,"FAILED to open %s\n",hexfl);
    exit(-1);
  }

  fgets (line, MAXLN, fp);
  while(!feof(fp)){
    if(line[0]!=':'){
      char msg[256];
      sprintf(msg,"Hello %d no colon %s\n",lcnt,line);
      LOGERR(msg);
    }
    ll = (HEXCHAR(line[1])<<4)+HEXCHAR(line[2]);
    typ = (HEXCHAR(line[7])<<4)+HEXCHAR(line[8]);
    chks = 0;
    for(int j=1;j<8;j+=2)
      chks += (UINT8)((HEXCHAR(line[j])<<4)+HEXCHAR(line[j+1])) ;
    //    printf("ll %02x chk %02x\n", (UINT8)ll,(UINT8)chks);

    addr = 0;
    for(int j=3;j<7;j++){
      addr <<= 4;
      addr += HEXCHAR(line[j]);
    }
    switch (typ){
    case 0:{
      dcnt++;
      curaddr = addr+extAddr;
      //      printf("loadSTate %d\n",loadState);
      switch (loadState){
      case 0:
	// 	  memorySegments.Add(memoryOfLine);
	curlen = ll;
	loadState = 1;
	strans = 0;
	break;
      case 1:
	if (curlen < 16383) curlen += ll;
	//	  if (last->llen < 16383) last.appendMemory(memoryOfLine);
	else
	  {
	    printf("line complete dcnt %d addr %d llen %d\n",dcnt,curaddr,curlen);
	    //	      memorySegments.Add(memoryOfLine);
	    ssz[scnt++] = curlen;
	    curlen = ll;
	    if (strans > 2)
	      {
		loadState = 2;
		strans = 0;
		//      endOfFile = true;
	      }
	    else strans++;

	  }
	break;
      case 2:{
	if (curlen< 65535) curlen += ll;
	//	  if (last->llen < 65535) last.appendMemory(memoryOfLine);
	else
	  {
	    printf("line complete dcnt %d addr %d llen %d\n",dcnt,curaddr,curlen);
	    ssz[scnt++] = curlen;
	    curlen = ll;
	    loadState = 3;
	  }
	break;
      }
      case 3:{
	if (curlen< 131071) curlen += ll;
	//	  if (last.data.Length < 131071) last.appendMemory(memoryOfLine);
	else
	  {
	    printf("line complete dcnt %d addr %d llen %d scnt %d\n",dcnt,curaddr,curlen,scnt);
	    ssz[scnt++] = curlen;
	    curlen = ll;
	    loadState = 3;
	  }
	break;
      }
      } // switch loadState
      // char bla[300]={0},w=8, *bp;
      // sprintf(bla,":%02X%04X%02X",ll,addr,typ);
      // bp = bla+9;
      for(int i=9;i<(9+((ll+1)*2));i+=2){
	chks += (HEXCHAR(line[i])<<4)+HEXCHAR(line[i+1]);
	// w = (HEXCHAR(line[i])<<4)+HEXCHAR(line[i+1]);
	// sprintf(bp,"%02X",(UINT8)w);
	// printf("ch %d-%c %d-%c %02X chk %02x\n",i,line[i],i+1,line[i+1], (UINT8)w,(UINT8)chks);
	// bp += 2;
      }
      if(chks!=0){
	char mm[512] = {0};
	sprintf(mm,"data record chks fail %02X -- %s\n",chks,line);
	LOGERR(mm);
	exit(0);
      }

      break;
    } // typ case 0
    case 1:{// eof

      if(strcmp(":00000001FF",line)!=0){
	char mm[300]={0};
	sprintf(mm,"HEXFILE -- bad EOF record %s\n",line);
	LOGERR(mm);
      }
      printf("EOF line complete dcnt %d addr %X llen %d scnt %d\n",dcnt,curaddr,curlen,scnt);
      ssz[scnt++] = curlen;
      break;
    }
    case 4:{// extended lin addr
      UINT8 hxb = 0;
      extAddr = 0;
      for(int j=0;j<4;j++){
	hxb = HEXCHAR(line[j+9]);
	extAddr = (extAddr<<4) + hxb;
	chks += hxb*(((j+1)%2)*15+1);
      }
      extAddr <<= 16;
      printf("exaddr %04X - dcnt %d lcnt %d\n",extAddr,dcnt,lcnt);
      printf("curadrr %04x extaddr %04x df %d\n",curaddr,extAddr,extAddr-curaddr);
      if((extAddr-curaddr)>16 && dcnt>0){
	printf("hello addr skip %d lcnt %d\n",extAddr-curaddr,lcnt);
	exit(extAddr-curaddr);
      }
	

      chks += (HEXCHAR(line[13])<<4)+HEXCHAR(line[14]);
      if(chks!=0){
	char mm[300]={0};
	sprintf(mm,"HEXFILE -- typ %02X checksum %02X failed record %s\n",typ,chks,line);
	LOGERR(mm);
	exit(typ);
      }

      break;
    }
    case 5:{
      BYTE hxb = 0;
      UINT32 chk=0;
      startAddr = 0;
      for(int j=0;j<8;j++){
	hxb = HEXCHAR(line[j+9]);
	startAddr = (startAddr<<4) + hxb;
	chks += hxb*(((j+1)%2)*15+1);
      }

      chks += (HEXCHAR(line[17])<<4)+HEXCHAR(line[18]);
      if(chks!=0){
	char mm[300]={0};
	sprintf(mm,"HEXFILE -- typ %02X checksum %02X failed record %s\n",typ,chks,line);
	LOGERR(mm);
	exit(typ);
      }
      break;
    }
    default:{
      char msg[256];
      sprintf(msg,"HEXFILE -- invalid record type %02X\n",typ);
      LOGERR(msg);
      exit(7);
    }
      
    } // typ switch
    fgets (line, MAXLN, fp);
    lcnt++;
  } // while

  printf("scan complete scnt %d\n",scnt);
  *seglst = sptr = (UINT32*)malloc(scnt*sizeof(UINT32));
  for(int i=0;i<scnt;i++)
    sptr[i] = ssz[i];

  return scnt;
}// scanHex

void readHex(char *hexfl, HexDat *hxp){
  UINT32 strans=0,ll=0, lcnt = 0, addr=0, scnt=0, ssz[MAXSEG]={0};
  UINT32 curlen=0, curaddr=0, extAddr=0, startAddr=0,dcnt=0;
  BYTE loadState = 0, typ=99;
  char line[MAXLN];
  UINT32 *aptr = hxp->addr, *sptr=hxp->dlen, q=0, crq=0;
  UINT8 *dptr = hxp->data[0];

  FILE *fp=fopen(hexfl,"r");
  if(!fp){
    fprintf(stderr,"FAILED to open %s\n",hexfl);
    exit(-1);
  }


  printf("starting readhex\n");
  fgets (line, MAXLN, fp);
  while(!feof(fp)){
    if(line[0]!=':'){
      char msg[256];
      sprintf(msg,"Hello %d no colon %s\n",lcnt,line);
      LOGERR(msg);
    }
    ll = (HEXCHAR(line[1])<<4)+HEXCHAR(line[2]);
    typ = (HEXCHAR(line[7])<<4)+HEXCHAR(line[8]);
    addr = 0;
    for(int j=3;j<7;j++){
      addr <<= 4;
      addr += HEXCHAR(line[j]);
    }
    switch (typ){
    case 0:{
      dcnt++;
      curaddr = addr+extAddr;
      //      printf("loadSTate %d\n",loadState);
      switch (loadState){
      case 0:{
	// 	  memorySegments.Add(memoryOfLine);
	curlen = ll;
	q = 0;
	crq = sptr[scnt];
	aptr[scnt] = curaddr;      
	dptr = hxp->data[scnt++];
	loadState = 1;
	strans = 0;
	break;
      }
      case 1:{
	if (curlen < 16383){ 
	  curlen += ll;
	}
	//	  if (last->llen < 16383) last.appendMemory(memoryOfLine);
	else
	  {
	    printf("line complete dcnt %d addr %d llen %d\n",dcnt,curaddr,curlen);
	    //	      memorySegments.Add(memoryOfLine);
	    q = 0;
	    crq = sptr[scnt];
	    aptr[scnt] = curaddr;      
	    dptr = hxp->data[scnt++];
	    curlen = ll;
	    if (strans > 2)
	      {
		loadState = 2;
		strans = 0;
		//      endOfFile = true;
	      }
	    else strans++;

	  }
	break;
      }
      case 2:{
	if (curlen< 65535) curlen += ll;
	//	  if (last->llen < 65535) last.appendMemory(memoryOfLine);
	else
	  {
	    printf("line complete dcnt %d addr %d llen %d\n",dcnt,curaddr,curlen);
	    q = 0;
	    crq = sptr[scnt];
	    aptr[scnt] = curaddr;      
	    dptr = hxp->data[scnt++];
	      //	    ssz[scnt++] = curlen;
	    curlen = ll;
	    loadState = 3;
	  }
	break;
      }
      case 3:{
	if (curlen< 131071) curlen += ll;
	//	  if (last.data.Length < 131071) last.appendMemory(memoryOfLine);
	else
	  {
	    printf("line complete dcnt %d addr %d llen %d scnt %d\n",dcnt,curaddr,curlen,scnt);
	    if(q!=crq)
	      printf("miscount %d %d %d\n",scnt,q,crq);
	    q = 0;
	    crq = sptr[scnt];
	    aptr[scnt] = curaddr;      
	    dptr = hxp->data[scnt++];
	    //	    ssz[scnt++] = curlen;
	    curlen = ll;
	    loadState = 3;
	  }
	break;
      }
      } // switch loadState
      for(int i=9;i<(9+(ll*2)) && q<crq;i+=2)
	dptr[q++] = (HEXCHAR(line[i])<<4)+HEXCHAR(line[i+1]);
      //if(q>=crq)
	//	printf("hello %d %d\n",q,crq);
      break;
    } // typ case 0
    case 1:{// eof
      printf("end of readhx\n");
      return;
      break;
    }
    case 4:{// extended lin addr
      BYTE hxb = 0;
      UINT32 chk=0;
      extAddr = 0;
      for(int j=0;j<4;j++){
	hxb = HEXCHAR(line[j+9]);
	extAddr = (extAddr<<4) + hxb;
	chk += hxb;
      }
      extAddr <<= 16;
      printf("exaddr %04X - dcnt %d lcnt %d\n",extAddr,dcnt,lcnt);
      printf("curadrr %04x extaddr %04x df %d\n",curaddr,extAddr,extAddr-curaddr);
      break;
    }
    case 5:{
      BYTE hxb = 0;
      UINT32 chk=0;
      startAddr = 0;
      for(int j=0;j<8;j++){
	hxb = HEXCHAR(line[j+9]);
	startAddr = (startAddr<<4) + hxb;
	chk +=hxb;
      }
      break;
    }
    default:{
      char msg[256];
      sprintf(msg,"HEXFILE -- invalid record type %02X\n",typ);
      LOGERR(msg);
      exit(7);
    }
      
    } // typ switch
    fgets (line, MAXLN, fp);
    lcnt++;
  } // while

  printf("scan complete scnt %d\n",scnt);

}// readHex


BOOL HexFile(char *hexfl, HexDat *hxp){
  UINT32 scnt=0, *ssz;

  scnt = scanHex(hexfl, &ssz);
  hxp->datCnt = scnt;
  hxp->dlen = ssz;

  hxp->addr = (UINT32*)malloc(scnt*sizeof(UINT32));
  hxp->data = (UINT8**)malloc(scnt*sizeof(UINT8*));
  for (int i=0;i<scnt;i++)
    hxp->data[i] = (UINT8*)malloc(ssz[i]);
  
  printf("malloc complete\n");
  readHex(hexfl,hxp);
  printf("Found %d mem lines\n",scnt);
  return true;
} // HexFile


void FreeHex(HexDat *hxp){
  
  for(int i=0;i<hxp->datCnt;i++)
    free(hxp->data[i]);
  free(hxp->data);
  free(hxp->addr);
  free(hxp->dlen);
  hxp->data = 0;
  hxp->addr = 0;
  hxp->dlen = 0;
  hxp->datCnt = 0;

}
