#include "binarytohexstring.h"

binaryToHexString::binaryToHexString()
{

}
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
const char AsciiTable[16]={
                              0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,
                              0x41,0x42,0x43,0x44,0x45,0x46,
                          };
//-------------------------------------------------------------------------------------------
unsigned char binaryToHexString::make_hexBinaryCheckSum(unsigned char* in_Hex_Data,unsigned char in_Length)
{
    unsigned char Index,CheckSum;

    Index = 0;
    CheckSum = 0;
    while(in_Length)
    {
        CheckSum += in_Hex_Data[Index];
        Index++;
        in_Length--;
    }
    CheckSum = 0 - CheckSum;
    return(CheckSum);
}
//-------------------------------------------------------------------------------------------
#define oneLine_dataSize           oneLine_hexBinaryData[0]
#define oneLine_addrH              oneLine_hexBinaryData[1]
#define oneLine_addrL              oneLine_hexBinaryData[2]
#define oneLine_type               oneLine_hexBinaryData[3]
#define oneLine_startDateRecord    oneLine_hexBinaryData[4]
#define oneLine_checksum           oneLine_hexBinaryData[oneLine_hexBinaryData[0]+4]
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
void binaryToHexString::create_hexString(__int32u in_startAddr,QByteArray in_hexBinarySrc,QString* out_pHexStringDst)
{
    __int32u Index,length;
    bool extenedLineAddr_add;
    unsigned char oneLine_hexBinaryData[21];
    unsigned char the_dataSize;
    unsigned char i;

    extenedLineAddr_add = false;
    length = in_hexBinarySrc.size();
    Index = 0;
    if(in_startAddr > 0x0000FFFF)
    {
        extenedLineAddr_add = true;
    }

    out_pHexStringDst->clear();

    while(length)
    {

        if(length<16)
        {
            the_dataSize = length;
        }
        else
        {
            the_dataSize = 16;
        }

        __int32u temp_addr;
        unsigned char temp_size,new_size;
        temp_addr = in_startAddr;
        temp_size = the_dataSize;
        new_size = 0;

        while(temp_size)
        {
            if(0==(temp_addr & 0x0000000F))  //aligning
            {
                if(0 != new_size)
                {
                    the_dataSize = new_size;
                }
            }
            temp_addr = temp_addr+1;
            temp_size--;
            new_size++;
        }

        temp_addr = in_startAddr;
        temp_size = the_dataSize;
        new_size = 0;
        while(temp_size)
        {
            if(0==(temp_addr & 0x0000FFFF))
            {
                if(0 != new_size)
                {
                    the_dataSize = new_size;
                }
                else
                {
                    extenedLineAddr_add = true;  //
                }
                break;
            }
            temp_addr = temp_addr+1;
            temp_size--;
            new_size++;
        }

        if(extenedLineAddr_add)
        {
            extenedLineAddr_add = false;

            oneLine_dataSize = 0x02;
            oneLine_addrH = 0x00;
            oneLine_addrL = 0x00;
            oneLine_type = 0x04;                               //add a extended linear address record
            oneLine_hexBinaryData[4] = (in_startAddr >> 24) & 0x000000FF;
            oneLine_hexBinaryData[5] = (in_startAddr >> 16) & 0x000000FF;

            oneLine_checksum = make_hexBinaryCheckSum(&oneLine_hexBinaryData[0],(oneLine_dataSize+4));  //checksum

            out_pHexStringDst->append(":");
            for(i=0;i<(oneLine_dataSize+5);i++)
            {
                out_pHexStringDst->append(AsciiTable[oneLine_hexBinaryData[i]>>4]);
                out_pHexStringDst->append(AsciiTable[oneLine_hexBinaryData[i]&0x0f]);
            }
            out_pHexStringDst->append("\r\n");
        }

        oneLine_dataSize = the_dataSize;
        oneLine_addrH = (in_startAddr >> 8) & 0x000000FF;
        oneLine_addrL = (in_startAddr) & 0x000000FF;
        oneLine_type = 0x00;     //add a data record
        in_startAddr += oneLine_dataSize;


        for(i=0;i<oneLine_dataSize;i++)
        {
            oneLine_hexBinaryData[i+4] = (unsigned char)(in_hexBinarySrc[Index]);

            Index++;
            length--;
            if(length == 0)
            {
                break;
            }
        }

        oneLine_checksum = make_hexBinaryCheckSum(&oneLine_hexBinaryData[0],(oneLine_dataSize+4));  //checksum

        out_pHexStringDst->append(":");
        for(i=0;i<(oneLine_dataSize+5);i++)
        {
            out_pHexStringDst->append(AsciiTable[oneLine_hexBinaryData[i]>>4]);
            out_pHexStringDst->append(AsciiTable[oneLine_hexBinaryData[i]&0x0f]);
        }

        out_pHexStringDst->append("\r\n");
    }
    out_pHexStringDst->append(":00000001FF");
}
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
