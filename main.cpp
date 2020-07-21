#include "cpl_conv.h" // for CPLMalloc()
#include "cpl_string.h"
#include "gdal_priv.h"
#include <iostream>

using namespace std;

int main(int argc, char *argv[])
{
    cout << "Start Pixel Remover with factor " << argv[1] << endl;
    cout << "Parameters number " << argc << endl;
    const char *pszFilename = "/mnt/disk2/routes/0041_0102_22511_1_02624_06_10_1.tif";
    GDALDataset *src_poDataset;
    GDALAllRegister();
    src_poDataset = (GDALDataset *) GDALOpen(pszFilename, GA_ReadOnly);
    if (src_poDataset == NULL) {
        cout << "src_poDataset is null";
        exit(3);
    }
    //Чтение информации о TIFF в целом
    double adfGeoTransform[6];
    printf("Driver: %s/%s\n",
           src_poDataset->GetDriver()->GetDescription(),
           src_poDataset->GetDriver()->GetMetadataItem(GDAL_DMD_LONGNAME));
    printf("Size is %dx%dx%d\n",
           src_poDataset->GetRasterXSize(),
           src_poDataset->GetRasterYSize(),
           src_poDataset->GetRasterCount());
    if (src_poDataset->GetProjectionRef() != NULL)
        printf("Projection is `%s'\n", src_poDataset->GetProjectionRef());
    if (src_poDataset->GetGeoTransform(adfGeoTransform) == CE_None) {
        printf("Origin = (%.6f,%.6f)\n", adfGeoTransform[0], adfGeoTransform[3]);
        printf("Pixel Size = (%.6f,%.6f)\n", adfGeoTransform[1], adfGeoTransform[5]);
    }
    //Информация о растровом канале
    GDALRasterBand *src_poBand;
    int nBlockXSize, nBlockYSize;
    int bGotMin, bGotMax;
    double adfMinMax[2];
    src_poBand = src_poDataset->GetRasterBand(1);
    src_poBand->GetBlockSize(&nBlockXSize, &nBlockYSize);
    printf("Block=%dx%d Type=%s, ColorInterp=%s\n",
           nBlockXSize,
           nBlockYSize,
           GDALGetDataTypeName(src_poBand->GetRasterDataType()),
           GDALGetColorInterpretationName(src_poBand->GetColorInterpretation()));
    adfMinMax[0] = src_poBand->GetMinimum(&bGotMin);
    adfMinMax[1] = src_poBand->GetMaximum(&bGotMax);
    if (!(bGotMin && bGotMax))
        GDALComputeRasterMinMax((GDALRasterBandH) src_poBand, TRUE, adfMinMax);
    printf("Min=%.3fd, Max=%.3f\n", adfMinMax[0], adfMinMax[1]);
    if (src_poBand->GetOverviewCount() > 0)
        printf("Band has %d overviews.\n", src_poBand->GetOverviewCount());
    if (src_poBand->GetColorTable() != NULL)
        printf("Band has a color table with %d entries.\n",
               src_poBand->GetColorTable()->GetColorEntryCount());

    //Read raster line
    float *pafScanline;
    int src_xSize = src_poBand->GetXSize();
    int src_ySize = src_poBand->GetYSize();
    cout << "X size=" << src_xSize << " Y size=" << src_ySize << endl;

    pafScanline = (float *) CPLMalloc(sizeof(float) * src_xSize);
    CPLErr scanLineReadResult;
    //    scanLineReadResult = src_poBand->RasterIO(
    //        GF_Read, 0, 0, src_xSize, 1, pafScanline, src_xSize, 1, GDT_Float32, 0, 0);
    //    cout << scanLineReadResult << endl;
    ////////////////////////////////////

    //Creation TIFF check
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

    CPLErr writeRes;
    float reduce_coeff = 2.0f;
    float *reducedScanLine;
    int dst_xSize = int(src_xSize / reduce_coeff);
    int dst_ySize = int(src_ySize / reduce_coeff);
    reducedScanLine = (float *) CPLMalloc(sizeof(float) * dst_xSize);
    //Create TIFF
    const char *pszDstFilename = "/mnt/disk2/routes/teat.tif";
    GDALDataset *dst_poDataset;
    char **papszOptions = NULL;
    dst_poDataset = poDriver
                        ->Create(pszDstFilename, dst_xSize, dst_ySize, 1, GDT_UInt16, papszOptions);

    double nf_adfGeoTransform[6] = {444720, 30, 0, 3751320, 0, -30};
    OGRSpatialReference oSRS;
    char *pszSRS_WKT = NULL;

    GDALRasterBand *nf_poBand;
    //    GByte abyRaster[512 * 512];

    //GeoData
    dst_poDataset->SetGeoTransform(nf_adfGeoTransform);
    oSRS.SetUTM(11, TRUE);
    oSRS.SetWellKnownGeogCS("NAD27");
    oSRS.exportToWkt(&pszSRS_WKT);
    dst_poDataset->SetProjection(pszSRS_WKT);
    CPLFree(pszSRS_WKT);
    ///

    nf_poBand = dst_poDataset->GetRasterBand(1);

    for (int i = 0; i < dst_ySize; i++) {
        scanLineReadResult = src_poBand->RasterIO(GF_Read,
                                                  0,
                                                  int(i * reduce_coeff),
                                                  src_xSize,
                                                  1,
                                                  pafScanline,
                                                  src_xSize,
                                                  1,
                                                  GDT_UInt16,
                                                  0,
                                                  0);
        for (int j = 0; j < dst_xSize; j++) {
            reducedScanLine[j] = pafScanline[int(j * reduce_coeff)];
        }
        writeRes = nf_poBand->RasterIO(
            GF_Write, 0, i, dst_xSize, 1, reducedScanLine, dst_xSize, 1, GDT_UInt16, 0, 0);
    }

    CPLFree(reducedScanLine);
    CPLFree(pafScanline);
    cout << writeRes;
    GDALClose((GDALDatasetH) dst_poDataset);

    exit(0);
}
