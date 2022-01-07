#include "artraw.h"

#include <functional>
#include <sstream>
#include <cassert>
#include <exception>
#include <cstring>


ArtRaw::ArtRaw(const std::string &filepath)
{
    std::ifstream       ifs(filepath, std::ifstream::in);
    size_t              width, height;
    size_t              n_channels;
    std::vector<double> bounds;

    if (!readHeader(ifs, width, height, n_channels, bounds)) {
        throw std::runtime_error("Cannot read ARTRAW header");
    }

    if (!readImageData(ifs, width, height, n_channels, bounds)) {
        throw std::runtime_error("Cannot read ARTRAW file content");
    }
}


bool ArtRaw::readHeader(
    std::istream        &is,
    size_t              &width,
    size_t              &height,
    size_t              &n_channels,
    std::vector<double> &bounds)
{
    std::function<void(std::string & s)> trim = [](std::string &s) {
        size_t p = s.find_first_not_of(" \t\n");
        s.erase(0, p);
        p = s.find_last_not_of(" \t\n");
        if (std::string::npos != p) {
            s.erase(p + 1);
        }
    };

    std::string       trash;
    std::string       strBuff;
    std::stringstream strStream;

    std::getline(is, strBuff);
    strStream = std::stringstream(strBuff.substr(std::strlen("ART RAW image format ")));
    strStream >> _version;

    // Check version
    if (_version != 2.2f && _version != 2.3f && _version != 2.4f && _version != 2.5f) {
        return false;
    }

    // Metadata
    std::getline(is, strBuff);   // <empty>

    if (_version < 2.4f) {
        // Created by version:
        std::getline(is, strBuff);
        _creation_version = strBuff.substr(std::strlen(TOKEN_CREATION_VERSION));
        trim(_creation_version);

        // Creation date:
        std::getline(is, strBuff);
        _creation_date = strBuff.substr(std::strlen(TOKEN_CREATION_DATE));
        trim(_creation_date);
    } else {
        // File created by:
        std::getline(is, strBuff);
        _program = strBuff.substr(std::strlen(TOKEN_PROGRAM));
        trim(_program);

        // Platform:
        std::getline(is, strBuff);
        _platform = strBuff.substr(std::strlen(TOKEN_PLATFORM));
        trim(_platform);

        // Command line:
        std::getline(is, strBuff);
        _command_line = strBuff.substr(std::strlen(TOKEN_COMMAND_LINE));
        trim(_command_line);

        // Creation date:
        std::getline(is, strBuff);
        _creation_date = strBuff.substr(std::strlen(TOKEN_CREATION_DATE));
        trim(_creation_date);

        // Render time:
        std::getline(is, strBuff);
        _render_time = strBuff.substr(std::strlen(TOKEN_RENDER_TIME));
        trim(_render_time);

        // Samples per pixel:
        std::getline(is, strBuff);
        _samples_per_pixel = strBuff.substr(std::strlen(TOKEN_SAMPLES_PER_PIXEL));
        trim(_samples_per_pixel);
    }

    // width height
    std::getline(is, strBuff);
    strStream = std::stringstream(strBuff.substr(std::strlen("Image size:         ")));
    strStream >> width;
    strStream >> trash;
    strStream >> height;
    assert(width > 0 && height > 0);

    std::getline(is, strBuff);

    // DPI if specified, default to 72dpi
    if (strBuff[0] == 'D') {
        strStream = std::stringstream(strBuff.substr(std::strlen("DPI:                ")));
        strStream >> _dpiX;
        strStream >> trash;
        strStream >> _dpiY;
    }

    assert(_dpiX > 0.f && _dpiY > 0.f);

    if (strBuff[0] == 'W') {
        // TODO
        return false;
    }

    // Polarisation & colour information
    std::string polarisationState, dataType;

    std::getline(is, strBuff);
    strStream = std::stringstream(strBuff.substr(std::strlen("Image type:         ")));
    strStream >> polarisationState;   // [plain / polarised]
    strStream >> dataType;            // [spectrum / CIEXYZ]
    strStream >> trash;               // with
    strStream >> n_channels;          // N
    strStream >> trash;               // samples

    if (polarisationState == "plain") {
        _spectrumType = SpectrumType::SPECTRUM_EMISSIVE;
    } else if (polarisationState == "polarised") {
        _spectrumType = SpectrumType::SPECTRUM_EMISSIVE_POLARISED;
    } else {
        throw std::runtime_error("Cannot read polarisation state");
    }

    if (dataType == "CIEXYZ") {
        _spectral = false;
    } else if (dataType == "spectrum") {
        _spectral = true;
    } else {
        throw std::runtime_error("Cannot read data type");
    }

    std::getline(is, strBuff);   // <empty>

    // Wavelength bounds
    std::getline(is, strBuff);

    strStream = std::stringstream(strBuff.substr(std::strlen("Sample bounds in nanometers: ")));
    bounds.resize(n_channels + 1);

    for (size_t c = 0; c < n_channels + 1; c++) {
        strStream >> bounds[c];
    }

    std::getline(is, strBuff);   // \n
    std::getline(is, strBuff);   // Big-endian binary coded IEEE float pixel values in scanline order follow:

    char c;
    is.get(c);   // X

    if (c != 'X') {
        return false;
    }

    return true;
}

bool ArtRaw::readImageData(
    std::istream              &is,
    size_t                     width,
    size_t                     height,
    size_t                     n_channels,
    const std::vector<double> &bounds)
{
    std::function<float(const char *buff)> getBuffer = [](const char *buff) {
        union
        {
            float        f;
            unsigned int i;
        } value;

        value.i = buff[3] & 0xff;
        value.i <<= 8;
        value.i += buff[2] & 0xff;
        value.i <<= 8;
        value.i += buff[1] & 0xff;
        value.i <<= 8;
        value.i += buff[0] & 0xff;

        return value.f;
    };

    // Populate wavelengths
    _wavelengths.resize(n_channels);

    for (size_t c = 0; c < n_channels; c++) {
        _wavelengths[c] = static_cast<unsigned int>((bounds[c] + bounds[c + 1]) / 2);
    }

    // Allocate memory for image
    _emissiveData.resize(width * height * n_channels);

    // Allocate memory for alpha channel
    _alpha.resize(width * height);

    // if (polarised()) {
    //     char *readBytes = new char[4 * (n_channels * 4 + 1)]; // 4 stokes and 1 alpha

    //     // Convert pixels bytes to float
    //     for (size_t y = 0; y < height; y++) {
    //         for (size_t i = 0; i < width / 8 + 1; i++) {
    //             // This flag byte determines if there is any polarisation data for the
    //             // next 8 pixels
    //             char flagByte = 0;
    //             is.read(&flagByte, 1);

    //             for (size_t j = 0; j < 8; j++) {
    //                 const size_t x = i * 8 + j;

    //                 if (x < width) {
    //                     const size_t n_stokes_at_px = (flagByte & 0x80) ? 4 : 1;
    //                     is.read(readBytes, 4 * (n_channels * n_stokes_at_px + 1));

    //                     for (size_t lambda = 0; lambda < n_channels; lambda++) {
    //                         for (size_t s = 0; s < n_stokes_at_px; s++) {
    //                             emissiveFramebuffer(s)[lambda](y, x) = getBuffer(&readBytes[4 * (n_channels * s + lambda)]);
    //                         }
    //                     }

    //                     _alphaChannel(y, x) = getBuffer(&readBytes[4 * (n_channels * n_stokes_at_px)]);
    //                 }

    //                 flagByte = flagByte << 1;
    //             }
    //         }
    //     }

    //     delete[] readBytes;
    // } else {
    // For non polarised image, we can determine size directly making it simpler to read
    const size_t      dataSize = 4 * width * height * (n_channels + 1);
    std::vector<char> bufferedImage(dataSize);

    // Read pixel values
    is.read(bufferedImage.data(), dataSize);

    // Convert pixels bytes to float
    for (size_t y = 0; y < height; y++) {
        for (size_t x = 0; x < width; x++) {
            for (size_t lambda = 0; lambda < n_channels; lambda++) {
                // Notice file nChannels + 1: there is an alpha channel in ARTRAWs
                const size_t fileOffset                              = (y * width + x) * (n_channels + 1) + lambda;
                _emissiveData[n_channels * (y * width + x) + lambda] = getBuffer(&bufferedImage[4 * fileOffset]);
            }
            _alpha[y * width + x] = getBuffer(&bufferedImage[4 * ((y * width + x) * (n_channels + 1) + n_channels)]);
        }
    }
    // }

    return true;
}