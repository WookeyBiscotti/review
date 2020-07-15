#include "cpl_conv.h" // for CPLMalloc()
#include "gdal_priv.h"
#include <iostream>

using namespace std;

int main()
{
    cout << "Start Pixel Remover!!!" << endl;
    const char *pszFilename = "";
    GDALDataset *poDataset;
    GDALAllRegister();
    //    poDataset = (GDALDataset *) GDALOpen(pszFilename, GA_ReadOnly);
    return 0;
}
