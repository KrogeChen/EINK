#include "mergebinarydata.h"
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
mergeBinaryData::mergeBinaryData()
{

}
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#define __int32u unsigned int
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#define CheckSumWordEven          0x39ea2e76
#define CheckSumWordOdd           0x82b453c3
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
__int32u MakeOneCheckText(__int32u Data,__int32u Count)
{
    if(Count&0x00000001)
    {
        Data=Data^CheckSumWordOdd;
    }
    else
    {
        Data=Data^CheckSumWordEven;
    }

    return(Data);
}
//-------------------------------------------------------------------------------
__int32u MakeCheckSumText(char* pData,__int32u Length)
{
    __int32u iWords,This_ids;
    __int32u Index;
    __int32u MakeSum,ReadData;
    iWords = (Length)/4;
    This_ids = 0;
    MakeSum = 0;
    Index = 0;
    while(iWords)
    {
        ReadData = (__int32u)pData[Index]&0x000000ff;
        ReadData = ReadData<<8;
        ReadData = ReadData|((__int32u)pData[Index+1]&0x000000ff);
        ReadData = ReadData<<8;
        ReadData = ReadData|((__int32u)pData[Index+2]&0x000000ff);
        ReadData = ReadData<<8;
        ReadData = ReadData|((__int32u)pData[Index+3]&0x000000ff);
        MakeSum+=MakeOneCheckText(ReadData,This_ids);
        Index = Index+4;
        iWords--;
        This_ids++;
    }
    return(MakeSum);
}
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//出口false 合并失败
//----------------------------------------------------------------------------------------
bool mergeBinaryData::mergeBinary_createMerge(binaryTransfer_def in_binaryOrigin,QByteArray* out_mergeBin,unsigned char in_resver_detlais)
{
    unsigned int i;
    unsigned char* pBootArray;
    unsigned char* pAppArray;
    bool mergeRight = true;

    binaryTransfer_def read_binaryOrigin;
    read_binaryOrigin = in_binaryOrigin;

    pBootArray =(unsigned char*)malloc(read_binaryOrigin.boot_max_size_byte);
    memset(pBootArray,0,read_binaryOrigin.boot_max_size_byte);
    pAppArray =(unsigned char*)malloc(read_binaryOrigin.app_max_size_byte);
    memset(pAppArray,0,read_binaryOrigin.app_max_size_byte);

    unsigned int appBinaryLength;
    appBinaryLength = read_binaryOrigin.in_appArea_length + (128-(read_binaryOrigin.in_appArea_length)%128); //aligning to 128byts

    if((read_binaryOrigin.in_bootArea_length > (read_binaryOrigin.boot_max_size_byte-4)) || (read_binaryOrigin.in_appArea_length > (read_binaryOrigin.app_max_size_byte -32)))
    {
        mergeRight = false;
    }
    else
    {

        for(i = 0;i < read_binaryOrigin.boot_max_size_byte;i++)//boot
        {
            if(i < read_binaryOrigin.in_bootArea_length)
            {
                pBootArray[i] = read_binaryOrigin.in_pBootArea_data[i];
            }
            else
            {
                pBootArray[i] = in_resver_detlais;
            }
        }

        for(i = 0;i < read_binaryOrigin.app_max_size_byte;i++)//app
        {
            if(i < read_binaryOrigin.in_appArea_length)
            {
                pAppArray[i] = read_binaryOrigin.in_pAppArea_data[i];
            }
            else if(i < appBinaryLength)
            {
                pAppArray[i] = 0xff;
            }
            else
            {
                pAppArray[i] = in_resver_detlais;
            }
        }
        //pBootArray[read_binaryOrigin.boot_max_size_byte-4] = 0x8d;
        //pBootArray[read_binaryOrigin.boot_max_size_byte-3] = 0x35;
        //pBootArray[read_binaryOrigin.boot_max_size_byte-2] = 0x00;
        //pBootArray[read_binaryOrigin.boot_max_size_byte-1] = 0x01;
        __int32u CheckSum;
        CheckSum = MakeCheckSumText((char*)&pAppArray[0],appBinaryLength);

        pAppArray[read_binaryOrigin.app_max_size_byte-16] = 0xfe; //resver
        pAppArray[read_binaryOrigin.app_max_size_byte-15] = 0xfe;
        pAppArray[read_binaryOrigin.app_max_size_byte-14] = 0xfe;
        pAppArray[read_binaryOrigin.app_max_size_byte-13] = 0xfe;

        pAppArray[read_binaryOrigin.app_max_size_byte-12] = 0xfe; //upgrade flag
        pAppArray[read_binaryOrigin.app_max_size_byte-11] = 0xfe;
        pAppArray[read_binaryOrigin.app_max_size_byte-10] = 0xfe;
        pAppArray[read_binaryOrigin.app_max_size_byte-9] = 0xfe;

        pAppArray[read_binaryOrigin.app_max_size_byte-8] = CheckSum >>24;  //check sum
        pAppArray[read_binaryOrigin.app_max_size_byte-7] = CheckSum >>16;
        pAppArray[read_binaryOrigin.app_max_size_byte-6] = CheckSum >>8;
        pAppArray[read_binaryOrigin.app_max_size_byte-5] = CheckSum;

        pAppArray[read_binaryOrigin.app_max_size_byte-4] = appBinaryLength>>24;
        pAppArray[read_binaryOrigin.app_max_size_byte-3] = appBinaryLength>>16;
        pAppArray[read_binaryOrigin.app_max_size_byte-2] = appBinaryLength>>8;
        pAppArray[read_binaryOrigin.app_max_size_byte-1] = appBinaryLength;

        for(i = 0;i < 16;i++) //反码校验
        {
            pAppArray[read_binaryOrigin.app_max_size_byte - 32 + i] = ~pAppArray[read_binaryOrigin.app_max_size_byte - 16 + i];
        }

        out_mergeBin->clear();

        for(i=0;i<read_binaryOrigin.boot_max_size_byte;i++)
        {
            out_mergeBin->append(pBootArray[i]);
        }
        for(i=0;i<read_binaryOrigin.app_max_size_byte;i++)
        {
           out_mergeBin->append(pAppArray[i]);
        }
        //for(i=0;i<read_binaryOrigin.app_max_size_byte;i++)  //update area data
        //{
        //   out_mergeBin->append(pAppArray[i]);
        //}
    }
    return(mergeRight);
}
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
