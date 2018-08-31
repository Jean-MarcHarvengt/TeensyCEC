// File     : zx_filetyp_z80.c
// Datum    : 27.01.2014
// Version  : 1.0
// Autor    : UB
// EMail    : mc-4u(@)t-online.de
// Web      : www.mikrocontroller-4u.de
// CPU      : STM32F429
// IDE      : CooCox CoIDE 1.7.4
// GCC      : 4.7 2012q4
// Module   : keine
// Funktion : Handle von ZX-Spectrum Files im Format "*.Z80"
//
// Hinweis  : Die Entpack-Funktion prueft NICHT das Fileformat
//            beim falschem Format event. Systemabsturz !!
// Translation : makapuf 

//--------------------------------------------------------------
// Includes
//--------------------------------------------------------------
#include "zx_filetyp_z80.h"

//-------------------------------------------------------------
extern uint8_t out_ram;

//--------------------------------------------------------------
// interne Funktionen
//--------------------------------------------------------------
const uint8_t* p_decompFlashBlock(const uint8_t *block_adr);
uint8_t* p_decompRamBlock(uint8_t *block_adr);
uint8_t p_cntEqualBytes(uint8_t wert, uint32_t adr);



//--------------------------------------------------------------
// Unpack data from a file ( type = * .Z80 ) from flash
// And copy them to the memory of the ZX - Spectrum
//
// Data = pointer to the start of data
// Length = number of bytes
//--------------------------------------------------------------
void ZX_ReadFromFlash_Z80(Z80 *R, const uint8_t *data, uint16_t length)
{
  const uint8_t *ptr;
  const uint8_t *akt_block,*next_block;
  uint8_t value1,value2;
  uint8_t flag_version=0;
  uint8_t flag_compressed=0;
  uint16_t header_len;
  uint16_t cur_addr;

  if(length==0) return;
  if(length>0xC020) return;

  //----------------------------------
  // parsing header
  // Byte : [0...29]
  //----------------------------------

  // Set pointer to data beginning
  ptr=data;

  R->AF.B.h=*(ptr++); // A [0]
  R->AF.B.l=*(ptr++); // F [1]
  R->BC.B.l=*(ptr++); // C [2]
  R->BC.B.h=*(ptr++); // B [3]
  R->HL.B.l=*(ptr++); // L [4]
  R->HL.B.h=*(ptr++); // H [5]

  // PC [6+7]
  value1=*(ptr++); 
  value2=*(ptr++);
  R->PC.W=(value2<<8)|value1;
  if(R->PC.W==0x0000) {
    flag_version=1;
  }
  else {
    flag_version=0;
  } 

  // SP [8+9]
  value1=*(ptr++);
  value2=*(ptr++);
  R->SP.W=(value2<<8)|value1;

  R->I=*(ptr++); // I [10]
  R->R=*(ptr++); // R [11]
  
  // Comressed-Flag & Border [12]
  value1=*(ptr++); 
  value2=((value1&0x0E)>>1);
  OutZ80(0xFE, value2); // BorderColor
  if((value1&0x20)!=0) {
    flag_compressed=1;
  } else {
    flag_compressed=0;
  }

  R->DE.B.l=*(ptr++); // E [13]
  R->DE.B.h=*(ptr++); // D [14]
  R->BC1.B.l=*(ptr++); // C1 [15]
  R->BC1.B.h=*(ptr++); // B1 [16]
  R->DE1.B.l=*(ptr++); // E1 [17]
  R->DE1.B.h=*(ptr++); // D1 [18]
  R->HL1.B.l=*(ptr++); // L1 [19]
  R->HL1.B.h=*(ptr++); // H1 [20]
  R->AF1.B.h=*(ptr++); // A1 [21]
  R->AF1.B.l=*(ptr++); // F1 [22]
  R->IY.B.l=*(ptr++); // Y [23]
  R->IY.B.h=*(ptr++); // I [24]
  R->IX.B.l=*(ptr++); // X [25]
  R->IX.B.h=*(ptr++); // I [26]

  // Interrupt-Flag [27]
  value1=*(ptr++); 
  if(value1!=0) {
    // EI
    R->IFF|=IFF_2|IFF_EI;
  }
  else {
    // DI
    R->IFF&=~(IFF_1|IFF_2|IFF_EI);
  }
  value1=*(ptr++); // nc [28]
  // Interrupt-Mode [29]
  value1=*(ptr++);  
  if((value1&0x01)!=0) {
    R->IFF|=IFF_IM1;
  }
  else {
    R->IFF&=~IFF_IM1;
  }
  if((value1&0x02)!=0) {
    R->IFF|=IFF_IM2;
  }
  else {
    R->IFF&=~IFF_IM2;
  }
  
  // restliche Register
  R->ICount   = R->IPeriod;
  R->IRequest = INT_NONE; 
  R->IBackup  = 0;

  //----------------------------------
  // save the data in RAM
  // Byte : [30...n]
  //----------------------------------

  cur_addr=0x4000; // RAM start
  

  if(flag_version==0) {
    //-----------------------
    // old Version
    //-----------------------
    if(flag_compressed==1) {
      //-----------------------
      // compressed
      //-----------------------      
      while(ptr<(data+length)) {
        value1=*(ptr++);
        if(value1!=0xED) {
          WrZ80(cur_addr++, value1);
        }
        else {
          value2=*(ptr++);
          if(value2!=0xED) { 
            WrZ80(cur_addr++, value1);
            WrZ80(cur_addr++, value2);
          }
          else {
            value1=*(ptr++);
            value2=*(ptr++);
            while(value1--) {
              WrZ80(cur_addr++, value2);
            }
          }
        }
      }
    }
    else {
      //-----------------------
      // raw (uncompressed)
      //-----------------------
      while(ptr<(data+length)) {
        value1=*(ptr++);
        WrZ80(cur_addr++, value1);
      }
    }
  }
  else {
    //-----------------------
    // new Version
    //-----------------------
    // Header Laenge [30+31]
    value1=*(ptr++); 
    value2=*(ptr++);
    header_len=(value2<<8)|value1;
    akt_block=(uint8_t*)(ptr+header_len); 
    // PC [32+33]
    value1=*(ptr++); 
    value2=*(ptr++);
    R->PC.W=(value2<<8)|value1;
    
    //------------------------
    // 1st block parsing
    //------------------------
    next_block=p_decompFlashBlock(akt_block);
    //------------------------
    // all other parsing
    //------------------------
    while(next_block<data+length) {
      akt_block=next_block;
      next_block=p_decompFlashBlock(akt_block); 
    } 
  }
}


//--------------------------------------------------------------
// Internal function
// Unpack and store a block of data
// ( From flash ) the new version
//--------------------------------------------------------------
const uint8_t* p_decompFlashBlock(const uint8_t *block_adr)
{
  const uint8_t *ptr;
  const uint8_t *next_block;
  uint8_t value1,value2;
  uint16_t block_len;
  uint8_t flag_compressed=0;
  uint8_t flag_page=0;
  uint16_t cur_addr=0;

  // pointer auf Blockanfang setzen
  ptr=block_adr;

  // Laenge vom Block
  value1=*(ptr++);
  value2=*(ptr++);
  block_len=(value2<<8)|value1;
  if(block_len==0xFFFF) {
    block_len=0x4000;
    flag_compressed=0;
  }
  else {
    flag_compressed=1;
  }
 
  // Page vom Block
  flag_page=*(ptr++);
  
  // next Block ausrechnen
  next_block=(uint8_t*)(ptr+block_len); 

  // Startadresse setzen
  if(flag_page==4) cur_addr=0x8000;
  if(flag_page==5) cur_addr=0xC000;
  if(flag_page==8) cur_addr=0x4000;

  if(flag_compressed==1) {
    //-----------------------
    // komprimiert
    //-----------------------
    while(ptr<(block_adr+3+block_len)) {
      value1=*(ptr++);
      if(value1!=0xED) {
        WrZ80(cur_addr++, value1);
      }
      else {
        value2=*(ptr++);
        if(value2!=0xED) { 
          WrZ80(cur_addr++, value1);
          WrZ80(cur_addr++, value2);
        }
        else {
          value1=*(ptr++);
          value2=*(ptr++);
          while(value1--) {
            WrZ80(cur_addr++, value2);
          }
        }
      }
    }
  }
  else {
    //-----------------------
    // nicht komprimiert
    //-----------------------
    while(ptr<(block_adr+3+block_len)) {
      value1=*(ptr++); 
      WrZ80(cur_addr++, value1);  
    }
  }

  return(next_block); 
}


//--------------------------------------------------------------
// Unpack data from a file ( type = * .Z80 ) from the transfer RAM
// And copy them to the memory of the ZX - Spectrum
//
// Data = pointer to the start of data
// Length = number of bytes
//--------------------------------------------------------------
void ZX_ReadFromTransfer_Z80(Z80 *R, uint8_t *data, uint16_t length)
{
  uint8_t *ptr;
  uint8_t *akt_block,*next_block;
  uint8_t value1,value2;
  uint8_t flag_version=0;
  uint8_t flag_compressed=0;
  uint16_t header_len;
  uint16_t cur_addr;

  if(length==0) return;
  if(length>0xC020) return;

  //----------------------------------
  // Auswertung vom Header
  // Byte : [0...29]
  //----------------------------------

  // pointer auf Datenanfang setzen
  ptr=data;

  R->AF.B.h=*(ptr++); // A [0]
  R->AF.B.l=*(ptr++); // F [1]
  R->BC.B.l=*(ptr++); // C [2]
  R->BC.B.h=*(ptr++); // B [3]
  R->HL.B.l=*(ptr++); // L [4]
  R->HL.B.h=*(ptr++); // H [5]

  // PC [6+7]
  value1=*(ptr++); 
  value2=*(ptr++);
  R->PC.W=(value2<<8)|value1;
  if(R->PC.W==0x0000) {
    flag_version=1;
  }
  else {
    flag_version=0;
  } 

  // SP [8+9]
  value1=*(ptr++);
  value2=*(ptr++);
  R->SP.W=(value2<<8)|value1;

  R->I=*(ptr++); // I [10]
  R->R=*(ptr++); // R [11]
  
  // Comressed-Flag und Border [12]
  value1=*(ptr++); 
  value2=((value1&0x0E)>>1);
  OutZ80(0xFE, value2); // BorderColor
  if((value1&0x20)!=0) {
    flag_compressed=1;
  }
  else {
    flag_compressed=0;
  }

  R->DE.B.l=*(ptr++); // E [13]
  R->DE.B.h=*(ptr++); // D [14]
  R->BC1.B.l=*(ptr++); // C1 [15]
  R->BC1.B.h=*(ptr++); // B1 [16]
  R->DE1.B.l=*(ptr++); // E1 [17]
  R->DE1.B.h=*(ptr++); // D1 [18]
  R->HL1.B.l=*(ptr++); // L1 [19]
  R->HL1.B.h=*(ptr++); // H1 [20]
  R->AF1.B.h=*(ptr++); // A1 [21]
  R->AF1.B.l=*(ptr++); // F1 [22]
  R->IY.B.l=*(ptr++); // Y [23]
  R->IY.B.h=*(ptr++); // I [24]
  R->IX.B.l=*(ptr++); // X [25]
  R->IX.B.h=*(ptr++); // I [26]

  // Interrupt-Flag [27]
  value1=*(ptr++); 
  if(value1!=0) {
    // EI
    R->IFF|=IFF_2|IFF_EI;
  }
  else {
    // DI
    R->IFF&=~(IFF_1|IFF_2|IFF_EI);
  }
  value1=*(ptr++); // nc [28]
  // Interrupt-Mode [29]
  value1=*(ptr++);  
  if((value1&0x01)!=0) {
    R->IFF|=IFF_IM1;
  }
  else {
    R->IFF&=~IFF_IM1;
  }
  if((value1&0x02)!=0) {
    R->IFF|=IFF_IM2;
  }
  else {
    R->IFF&=~IFF_IM2;
  }
  
  // restliche Register
  R->ICount   = R->IPeriod;
  R->IRequest = INT_NONE; 
  R->IBackup  = 0;

  //----------------------------------
  // speichern der Daten im RAM
  // Byte : [30...n]
  //----------------------------------

  cur_addr=0x4000; // start vom RAM
  

  if(flag_version==0) {
    //-----------------------
    // alte Version
    //-----------------------
    if(flag_compressed==1) {
      //-----------------------
      // komprimiert
      //-----------------------      
      while(ptr<(data+length)) {
        value1=*(ptr++);
        if(value1!=0xED) {
          WrZ80(cur_addr++, value1);
        }
        else {
          value2=*(ptr++);
          if(value2!=0xED) { 
            WrZ80(cur_addr++, value1);
            WrZ80(cur_addr++, value2);
          }
          else {
            value1=*(ptr++);
            value2=*(ptr++);
            while(value1--) {
              WrZ80(cur_addr++, value2);
            }
          }
        }
      }
    }
    else {
      //-----------------------
      // nicht komprimiert
      //-----------------------
      while(ptr<(data+length)) {
        value1=*(ptr++);
        WrZ80(cur_addr++, value1);
      }
    }
  }
  else {
    //-----------------------
    // neue Version
    //-----------------------
    // Header Laenge [30+31]
    value1=*(ptr++); 
    value2=*(ptr++);
    header_len=(value2<<8)|value1;
    akt_block=(uint8_t*)(ptr+header_len); 
    // PC [32+33]
    value1=*(ptr++); 
    value2=*(ptr++);
    R->PC.W=(value2<<8)|value1;
    
    //------------------------
    // ersten block auswerten
    //------------------------
    next_block=p_decompRamBlock(akt_block);
    //------------------------
    // alle anderen auswerten
    //------------------------
    while(next_block<data+length) {
      akt_block=next_block;
      next_block=p_decompRamBlock(akt_block); 
    } 
  }
}


//--------------------------------------------------------------
// Internal function
// Unpack and store a block of data
// ( From transfer - RAM ) of the new version
//--------------------------------------------------------------
uint8_t* p_decompRamBlock(uint8_t *block_adr)
{
  uint8_t *ptr;
  uint8_t *next_block;
  uint8_t value1,value2;
  uint16_t block_len;
  uint8_t flag_compressed=0;
  uint8_t flag_page=0;
  uint16_t cur_addr=0;

  // pointer auf Blockanfang setzen
  ptr=block_adr;

  // Laenge vom Block
  value1=*(ptr++);
  value2=*(ptr++);
  block_len=(value2<<8)|value1;
  if(block_len==0xFFFF) {
    block_len=0x4000;
    flag_compressed=0;
  }
  else {
    flag_compressed=1;
  }
 
  // Page vom Block
  flag_page=*(ptr++);
  
  // next Block ausrechnen
  next_block=(uint8_t*)(ptr+block_len); 

  // Startadresse setzen
  if(flag_page==4) cur_addr=0x8000;
  if(flag_page==5) cur_addr=0xC000;
  if(flag_page==8) cur_addr=0x4000;

  if(flag_compressed==1) {
    //-----------------------
    // compressed
    //-----------------------
    while(ptr<(block_adr+3+block_len)) {
      value1=*(ptr++);
      if(value1!=0xED) {
        WrZ80(cur_addr++, value1);
      }
      else {
        value2=*(ptr++);
        if(value2!=0xED) { 
          WrZ80(cur_addr++, value1);
          WrZ80(cur_addr++, value2);
        }
        else {
          value1=*(ptr++);
          value2=*(ptr++);
          while(value1--) {
            WrZ80(cur_addr++, value2);
          }
        }
      }
    }
  }
  else {
    //-----------------------
    // uncompressed
    //-----------------------
    while(ptr<(block_adr+3+block_len)) {
      value1=*(ptr++); 
      WrZ80(cur_addr++, value1);  
    }
  }

  return(next_block);
}


//--------------------------------------------------------------
// Read data from the memory by the ZX - Spectrum , and pack
// Copy the file type ( type = * .Z80 ) in a transfer buffer
//
// Buffer_start = pointer to the start of the transfer buffer
// Return_wert = number of bytes in transfer buffer
//
// Note : Z80 version = old version , compression = ON
//--------------------------------------------------------------
uint16_t ZX_WriteToTransfer_Z80(Z80 *R, uint8_t *buffer_start)
{
  uint16_t ret_wert=0;
  uint8_t *ptr;
  uint8_t wert,anz;
  uint32_t cur_addr;  // muss 32bit sein !!

  //----------------------------------
  // Schreiben vom Header
  // Byte : [0...29]
  //----------------------------------

  // Startadresse setzen
  ptr=buffer_start;

  *(ptr++)=R->AF.B.h; // A [0]
  *(ptr++)=R->AF.B.l; // F [1]
  *(ptr++)=R->BC.B.l; // C [2]
  *(ptr++)=R->BC.B.h; // B [3]
  *(ptr++)=R->HL.B.l; // L [4]
  *(ptr++)=R->HL.B.h; // H [5]

  // PC [6+7] => fuer Version=0
  *(ptr++)=R->PC.B.l;
  *(ptr++)=R->PC.B.h;

  // SP [8+9]
  *(ptr++)=R->SP.B.l;
  *(ptr++)=R->SP.B.h;

  *(ptr++)=R->I; // I [10]
  *(ptr++)=R->R; // R [11]

  // Comressed-Flag und Border [12]
  wert=((out_ram&0x07)<<1);
  wert|=0x20; // fuer Compressed=1
  *(ptr++)=wert;

  *(ptr++)=R->DE.B.l; // E [13]
  *(ptr++)=R->DE.B.h; // D [14]
  *(ptr++)=R->BC1.B.l; // C1 [15]
  *(ptr++)=R->BC1.B.h; // B1 [16]
  *(ptr++)=R->DE1.B.l; // E1 [17]
  *(ptr++)=R->DE1.B.h; // D1 [18]
  *(ptr++)=R->HL1.B.l; // L1 [19]
  *(ptr++)=R->HL1.B.h; // H1 [20]
  *(ptr++)=R->AF1.B.h; // A1 [21]
  *(ptr++)=R->AF1.B.l; // F1 [22]
  *(ptr++)=R->IY.B.l; // Y [23]
  *(ptr++)=R->IY.B.h; // I [24]
  *(ptr++)=R->IX.B.l; // X [25]
  *(ptr++)=R->IX.B.h; // I [26]

  // Interrupt-Flag [27]
  if((R->IFF&IFF_EI)!=0) {
    // EI
    wert=0x01;
  }
  else {
    // DI
    wert=0x00;
  }
  *(ptr++)=wert;
  *(ptr++)=0x00; // nc [28]
  // Interrupt-Mode [29]
  wert=0x00;
  if((R->IFF&IFF_IM1)!=0) wert|=01;
  if((R->IFF&IFF_IM2)!=0) wert|=02;
  *(ptr++)=wert;

  //----------------------------------
  // schreiben der Daten vom RAM
  // Byte : [30...n]
  //----------------------------------

  // Startadresse setzen
  cur_addr=0x4000; // start vom RAM

  //-----------------------
  // Old version, compressed //
  //-----------------------

  do {
    wert=RdZ80(cur_addr);
    if(wert==0xED) {
      anz=p_cntEqualBytes(wert,cur_addr);
      if(anz>=2) {
        *(ptr++)=0xED;
        *(ptr++)=0xED;
        *(ptr++)=anz;
        *(ptr++)=wert;
        cur_addr+=anz;
      }
      else {
        *(ptr++)=wert;
        cur_addr++;
        if(cur_addr<=0xFFFF) {
          wert=RdZ80(cur_addr);
          *(ptr++)=wert;
          cur_addr++;
        }
      }
    }
    else {
      anz=p_cntEqualBytes(wert,cur_addr);
      if(anz>=5) {
        *(ptr++)=0xED;
        *(ptr++)=0xED;
        *(ptr++)=anz;
        *(ptr++)=wert;
        cur_addr+=anz;
      }
      else {
        *(ptr++)=wert;
        cur_addr++;
      }
    }
  }while(cur_addr<=0xFFFF);

  // Save end identifier
  *(ptr++)=0x00;
  *(ptr++)=0xED;
  *(ptr++)=0xED;
  *(ptr++)=0x00;

  // Number of bytes to calculate ( header + data )
  ret_wert=ptr-buffer_start;

  return(ret_wert);
}


//--------------------------------------------------------------
// Internal Function
// Determine the number of bytes equal
//--------------------------------------------------------------
uint8_t p_cntEqualBytes(uint8_t wert, uint32_t adr)
{
  uint8_t ret_wert=1;
  uint8_t n,test;

  // max 255 gleiche Werte
  for(n=0;n<254;n++) {
    adr++;
    if(adr>=0xFFFF) break;
    test=RdZ80(adr);
    if(test==wert) {
      ret_wert++;
    }
    else {
      break;
    }
  }

  return(ret_wert);
}
