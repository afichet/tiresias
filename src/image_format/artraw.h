#pragma once

#include <string>
#include <vector>
#include <fstream>
#include <cstddef>

class ArtRaw
{
  public:
    enum SpectrumType
    {
        SPECTRUM_EMISSIVE           = 1 << 0,
        SPECTRUM_REFLECTVE          = 1 << 1,
        SPECTRUM_POLARISED          = 1 << 2,
        SPECTRUM_EMISSIVE_POLARISED = SPECTRUM_EMISSIVE | SPECTRUM_POLARISED,
    };

    ArtRaw(const std::string &filepath);


  protected:
    bool readHeader(
        std::istream        &is,
        size_t              &width,
        size_t              &height,
        size_t              &n_channels,
        std::vector<double> &bounds);

    bool readImageData(
        std::istream              &is,
        size_t                     width,
        size_t                     height,
        size_t                     n_channels,
        const std::vector<double> &bounds);


    // Image data
    SpectrumType _spectrumType;
    size_t       _width, _height;

    std::vector<unsigned int> _wavelengths;
    std::vector<float>        _emissiveData;
    std::vector<float>        _alpha;

    float _version;

    // Metadata
    std::string _creation_date;
    const char *TOKEN_CREATION_DATE = "Creation date:";

    // v2.4
    std::string _program;
    std::string _platform;
    std::string _command_line;
    std::string _render_time;
    std::string _samples_per_pixel;

    const char *TOKEN_PROGRAM           = "File created by:";
    const char *TOKEN_PLATFORM          = "Platform:";
    const char *TOKEN_COMMAND_LINE      = "Command line:";
    const char *TOKEN_RENDER_TIME       = "Render time:";
    const char *TOKEN_SAMPLES_PER_PIXEL = "Samples per pixel:";

    // < v2.4
    std::string _creation_version;

    const char *TOKEN_CREATION_VERSION = "Created by version:";

    float _dpiX, _dpiY;
    bool  _spectral;
    bool  _polarised;
};