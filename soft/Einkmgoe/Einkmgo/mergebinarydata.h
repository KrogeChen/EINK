#ifndef MERGEBINARYDATA_H
#define MERGEBINARYDATA_H

#include <QByteArray>
typedef struct
{
    unsigned char* in_pBootArea_data;
    unsigned int in_bootArea_length;
    unsigned int boot_max_size_byte;
    unsigned char* in_pAppArea_data;
    unsigned int in_appArea_length;
    unsigned int app_max_size_byte;

}binaryTransfer_def;
class mergeBinaryData
{
public:
    mergeBinaryData();

    bool mergeBinary_createMerge(binaryTransfer_def in_binaryOrigin,QByteArray* out_mergeBin,unsigned char in_resver_detlais);
};

#endif // MERGEBINARYDATA_H
