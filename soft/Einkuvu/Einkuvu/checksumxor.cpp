#include <QMainWindow>
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#define CheckSumWordEven          0x39ea2e76
#define CheckSumWordOdd           0x82b453c3
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
__int32 MakeOneCheckText(__int32 Data,__int32 Count)
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
//--------------------------------------------------------------------------------
__int32 MakeCheckSumText(char* pData,__int32 Length)
{
    __int32 iWords,This_ids;
    __int32 Index;
    __int32 MakeSum,ReadData;
    iWords = (Length)/4;
    This_ids = 0;
    MakeSum = 0;
    Index = 0;
    while(iWords)
    {
        ReadData = (__int32)pData[Index]&0x000000ff;
        ReadData = ReadData<<8;
        ReadData = ReadData|((__int32)pData[Index+1]&0x000000ff);
        ReadData = ReadData<<8;
        ReadData = ReadData|((__int32)pData[Index+2]&0x000000ff);
        ReadData = ReadData<<8;
        ReadData = ReadData|((__int32)pData[Index+3]&0x000000ff);
        MakeSum+=MakeOneCheckText(ReadData,This_ids);
        Index = Index+4;
        iWords--;
        This_ids++;
    }
    return(MakeSum);
}
