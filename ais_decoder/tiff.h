#ifndef AIS_TIFF_H
#define AIS_TIFF_H

#include <tiff.h>
#include <tiffio.h>
#include <vector>


/*
     Write out TIFF file to disk.
     Writes gray-scale one sample per pixel with 16-bits per sample.
 */
int writeTiffFileInt16(const char *_pszFilename, int _iWidth, int _iHeight, const uint16_t *_pImageData)
{
    TIFF *image = TIFFOpen(_pszFilename, "w");
    TIFFSetField(image, TIFFTAG_IMAGEWIDTH, _iWidth);
    TIFFSetField(image, TIFFTAG_IMAGELENGTH, _iHeight);
    TIFFSetField(image, TIFFTAG_BITSPERSAMPLE, 16);
    TIFFSetField(image, TIFFTAG_SAMPLESPERPIXEL, 1);
    TIFFSetField(image, TIFFTAG_ROWSPERSTRIP, 1);
    TIFFSetField(image, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);
    TIFFSetField(image, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
    TIFFSetField(image, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_MINISBLACK);
    TIFFSetField(image, TIFFTAG_SAMPLEFORMAT, SAMPLEFORMAT_UINT);
    TIFFSetField(image, TIFFTAG_COMPRESSION, COMPRESSION_NONE);

    auto scan_line = std::vector<uint32>(_iWidth);

    for (int i = 0; i < _iHeight; i++) {
        memcpy(scan_line.data(), &_pImageData[i * _iWidth], _iWidth * sizeof(uint16_t));
        TIFFWriteScanline(image, scan_line.data(), i, 0);
    }

    TIFFClose(image);
    return 0;
}


#endif // #ifndef AIS_TIFF_H
