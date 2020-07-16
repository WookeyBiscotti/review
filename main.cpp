#include "cpl_conv.h" // for CPLMalloc()
#include "cpl_string.h"
#include "gdal_priv.h"
#include <iostream>

using namespace std;

int main()
{
    cout << "Start Pixel Remover!!!" << endl;
    const char *pszFilename = "/mnt/disk2/routes/0041_0102_22511_1_02624_06_10_1.tif";
    GDALDataset *poDataset;
    GDALAllRegister();
    poDataset = (GDALDataset *) GDALOpen(pszFilename, GA_ReadOnly);
    if (poDataset == NULL) {
        cout << "poDataset is null";
        return 0;
    }
    //Чтение информации о TIFF в целом
    double adfGeoTransform[6];
    printf("Driver: %s/%s\n",
           poDataset->GetDriver()->GetDescription(),
           poDataset->GetDriver()->GetMetadataItem(GDAL_DMD_LONGNAME));
    printf("Size is %dx%dx%d\n",
           poDataset->GetRasterXSize(),
           poDataset->GetRasterYSize(),
           poDataset->GetRasterCount());
    if (poDataset->GetProjectionRef() != NULL)
        printf("Projection is `%s'\n", poDataset->GetProjectionRef());
    if (poDataset->GetGeoTransform(adfGeoTransform) == CE_None) {
        printf("Origin = (%.6f,%.6f)\n", adfGeoTransform[0], adfGeoTransform[3]);
        printf("Pixel Size = (%.6f,%.6f)\n", adfGeoTransform[1], adfGeoTransform[5]);
    }
    //Информация о растровом канале
    GDALRasterBand *poBand;
    int nBlockXSize, nBlockYSize;
    int bGotMin, bGotMax;
    double adfMinMax[2];
    poBand = poDataset->GetRasterBand(1);
    poBand->GetBlockSize(&nBlockXSize, &nBlockYSize);
    printf("Block=%dx%d Type=%s, ColorInterp=%s\n",
           nBlockXSize,
           nBlockYSize,
           GDALGetDataTypeName(poBand->GetRasterDataType()),
           GDALGetColorInterpretationName(poBand->GetColorInterpretation()));
    adfMinMax[0] = poBand->GetMinimum(&bGotMin);
    adfMinMax[1] = poBand->GetMaximum(&bGotMax);
    if (!(bGotMin && bGotMax))
        GDALComputeRasterMinMax((GDALRasterBandH) poBand, TRUE, adfMinMax);
    printf("Min=%.3fd, Max=%.3f\n", adfMinMax[0], adfMinMax[1]);
    if (poBand->GetOverviewCount() > 0)
        printf("Band has %d overviews.\n", poBand->GetOverviewCount());
    if (poBand->GetColorTable() != NULL)
        printf("Band has a color table with %d entries.\n",
               poBand->GetColorTable()->GetColorEntryCount());

    //Read raster line
    float *pafScanline;
    int nXSize = poBand->GetXSize();
    int nYSize = poBand->GetYSize();
    cout << "X size=" << nXSize << " Y size=" << nYSize << endl;

    pafScanline = (float *) CPLMalloc(sizeof(float) * nXSize);
    auto scanLineReadResult
        = poBand->RasterIO(GF_Read, 0, 0, nXSize, 1, pafScanline, nXSize, 1, GDT_Float32, 0, 0);
    cout << scanLineReadResult << endl;

    //Create TIFF check
    const char *pszFormat = "GTiff";
    GDALDriver *poDriver;
    char **papszMetadata;
    poDriver = GetGDALDriverManager()->GetDriverByName(pszFormat);
    if (poDriver == NULL)
        exit(1);
    papszMetadata = poDriver->GetMetadata();
    if (CSLFetchBoolean(papszMetadata, GDAL_DCAP_CREATE, FALSE))
        printf("Driver %s supports Create() method.\n", pszFormat);
    if (CSLFetchBoolean(papszMetadata, GDAL_DCAP_CREATECOPY, FALSE))
        printf("Driver %s supports CreateCopy() method.\n", pszFormat);
    ;

    //Create TIFF
    const char *pszDstFilename = "/mnt/disk2/routes/teat.tif";
    GDALDataset *poDstDS;
    char **papszOptions = NULL;
    poDstDS = poDriver->Create(pszDstFilename, 512, 512, 1, GDT_Byte, papszOptions);

    double nf_adfGeoTransform[6] = {444720, 30, 0, 3751320, 0, -30};
    OGRSpatialReference oSRS;
    char *pszSRS_WKT = NULL;

    GDALRasterBand *nf_poBand;
    GByte abyRaster[512 * 512];

    poDstDS->SetGeoTransform(nf_adfGeoTransform);
    oSRS.SetUTM(11, TRUE);
    oSRS.SetWellKnownGeogCS("NAD27");
    oSRS.exportToWkt(&pszSRS_WKT);
    poDstDS->SetProjection(pszSRS_WKT);
    CPLFree(pszSRS_WKT);

    nf_poBand = poDstDS->GetRasterBand(1);
    auto res = nf_poBand->RasterIO(GF_Write, 0, 0, 512, 512, pafScanline, 512, 512, GDT_Byte, 0, 0);
    cout << res;
    /* Once we're done, close properly the dataset */
    GDALClose((GDALDatasetH) poDstDS);

    return 0;
}
