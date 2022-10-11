#ifndef BINARYTOHEXSTRING_H
#define BINARYTOHEXSTRING_H
//-------------------------------------------------------------------------------
#include <QString>
#include <QByteArray>
#define __int32u unsigned int
//-------------------------------------------------------------------------------
class binaryToHexString
{
public:
    binaryToHexString();
    void create_hexString(__int32u in_startAddr,QByteArray in_hexBinarySrc,QString* out_pHexStringDst);
private:
    unsigned char make_hexBinaryCheckSum(unsigned char* in_Hex_Data,unsigned char in_Length);
};

#endif // BINARYTOHEXSTRING_H
