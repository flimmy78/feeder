#ifndef _NAND_RW_H
#define _NAND_RW_H

struct _RW_FILE
{
    short rData;
    short wData;
    int   nandFile;
};
typedef struct _RW_FILE RWDATA;

#define MODELNANDFILE               "/config/nandfile.txt"


extern void Nand_Read_Config(char * file);
extern void Nand_Write_Config(char * file);



#endif
