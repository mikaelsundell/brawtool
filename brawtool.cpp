//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022 - present Mikael Sundell.
//

#include <iostream>
#include <fstream>
#include <vector>
#include <variant>

// openimageio
#include <OpenImageIO/imageio.h>
#include <OpenImageIO/typedesc.h>
#include <OpenImageIO/argparse.h>
#include <OpenImageIO/filesystem.h>
#include <OpenImageIO/sysutil.h>

#include <OpenImageIO/imagebuf.h>
#include <OpenImageIO/imagebufalgo.h>

using namespace OIIO;

// opencolorio
#include <OpenColorIO/OpenColorIO.h>
using namespace OpenColorIO_v2_3;

// braw
#include <BlackmagicRawAPI.h>

// boost
#include <boost/filesystem.hpp>
#include <boost/uuid/detail/md5.hpp>
#include <boost/algorithm/hex.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/regex.hpp>

using namespace boost::property_tree;

// prints
template <typename T>
static void
print_info(std::string param, const T& value = T()) {
    std::cout << "info: " << param << value << std::endl;
}

static void
print_info(std::string param) {
    print_info<std::string>(param);
}

template <typename T>
static void
print_warning(std::string param, const T& value = T()) {
    std::cout << "warning: " << param << value << std::endl;
}

static void
print_warning(std::string param) {
    print_warning<std::string>(param);
}

template <typename T>
static void
print_error(std::string param, const T& value = T()) {
    std::cerr << "error: " << param << value << std::endl;
}

static void
print_error(std::string param) {
    print_error<std::string>(param);
}

// braw tool
struct BrawTool
{
    bool help = false;
    bool verbose = false;
    bool clonebraw = false;
    bool cloneproxy = false;
    bool apply3dlut = false;
    bool applymetadata = false;
    boost::optional<float> exposure;
    boost::optional<int> kelvin;
    boost::optional<int> tint;
    boost::optional<int> width;
    boost::optional<int> height;
    std::string inputfilename;
    std::string outputdirectory;
    std::string override3dlut;
    int code = EXIT_SUCCESS;
};

static BrawTool tool;

static int
set_inputfilename(int argc, const char* argv[])
{
    OIIO_DASSERT(argc == 2);
    tool.inputfilename = argv[1];
    return 0;
}

static int
set_kelvin(int argc, const char* argv[])
{
    OIIO_DASSERT(argc == 2);
    tool.kelvin = Strutil::stoi(argv[1]);
    return 0;
}

static int
set_tint(int argc, const char* argv[])
{
    OIIO_DASSERT(argc == 2);
    tool.tint = Strutil::stoi(argv[1]);
    return 0;
}

static int
set_exposure(int argc, const char* argv[])
{
    OIIO_DASSERT(argc == 2);
    tool.exposure = Strutil::stof(argv[1]);
    return 0;
}

static int
set_width(int argc, const char* argv[])
{
    OIIO_DASSERT(argc == 2);
    tool.width = Strutil::stoi(argv[1]);
    return 0;
}

static int
set_height(int argc, const char* argv[])
{
    OIIO_DASSERT(argc == 2);
    tool.height = Strutil::stoi(argv[1]);
    return 0;
}

static int
set_override3dlut(int argc, const char* argv[])
{
    OIIO_DASSERT(argc == 2);
    tool.override3dlut = argv[1];
    return 0;
}

static int
set_outputdirectory(int argc, const char* argv[])
{
    OIIO_DASSERT(argc == 2);
    tool.outputdirectory = argv[1];
    return 0;
}

static void
print_help(ArgParse& ap)
{
    ap.print_help();
}

// utils - dates
std::string datetime()
{
    std::time_t now = time(NULL);
    struct tm tm;
    Sysutil::get_local_time(&now, &tm);
    char datetime[20];
    strftime(datetime, 20, "%Y-%m-%d %H:%M:%S", &tm);
    return std::string(datetime);
}

std::string str_by_float(float value)
{
    std::stringstream stream;
    stream << std::fixed << std::setprecision(1) << value;
    return stream.str();
}

std::string str_by_int(int value)
{
    return std::to_string(value);
}

// utils - core foundation
CFStringRef cfstr_by_str(const std::string& str) {
    CFStringRef ref = CFStringCreateWithBytes(
        kCFAllocatorDefault,
        reinterpret_cast<const UInt8*>(str.c_str()),
        str.length(),
        kCFStringEncodingUTF8,
        false
    );
    return ref;
}

// utils - filesystem
std::string filename(const std::string& path)
{
    return Filesystem::filename(path);
}

std::string extension(const std::string& path, const std::string& extension)
{
    return Filesystem::replace_extension(path, extension);
}

bool exists(const std::string& path)
{
    return Filesystem::exists(path);
}

std::string hash_file(const std::string& path)
{
    boost::filesystem::path filepath(path);
    std::ifstream file(filepath.string(), std::ios::binary);
    std::stringstream buffer;
    buffer << file.rdbuf();
    boost::uuids::detail::md5 hash;
    boost::uuids::detail::md5::digest_type digest;
    hash.process_bytes(buffer.str().data(), buffer.str().size());
    hash.get_digest(digest);
    const char* chardigest = reinterpret_cast<const char*>(&digest);
    std::string result;
    boost::algorithm::hex(chardigest,
        chardigest + sizeof(boost::uuids::detail::md5::digest_type),
        std::back_inserter(result)
    );
    return result;
}

bool copy_file(const std::string& input, const boost::filesystem::path& output)
{
    boost::filesystem::path inputpath(input);
    boost::filesystem::path outputpath(output);
    try {
        if (!boost::filesystem::exists(outputpath.parent_path())) {
            boost::filesystem::create_directories(outputpath.parent_path());
        }
        boost::filesystem::copy_file(inputpath, outputpath, boost::filesystem::copy_options::overwrite_existing);
        return true;
    } catch (const boost::filesystem::filesystem_error& e) {
        return false;
    }
}

bool file_compare(const std::string& source, const std::string& target)
{
    return hash_file(source) == hash_file(target);
}

bool create_path(const std::string& path)
{
    std::string error;
    return Filesystem::create_directory(path, error);
}

std::string filename_path(const std::string& path)
{
    return Filesystem::parent_path(path);
}

std::string program_path(const std::string& path)
{
    return Filesystem::parent_path(Sysutil::this_program_path()) + path;
}

std::string font_path(const std::string& font)
{
    return Filesystem::parent_path(Sysutil::this_program_path()) + "/fonts/" + font;
}

std::string resources_path(const std::string& resource)
{
    return Filesystem::parent_path(Sysutil::this_program_path()) + "/resources/" + resource;
}

std::string combine_path(const std::string& path, const std::string& filename)
{
    return path + "/" + filename;
}
// braw metadata

// utils - metadata
struct BrawMetadata
{
    std::string key;
    std::string name;
    TypeDesc type;
    int x;
    int y;
};

ROI draw_metadata(ImageBuf& imageBuf, BrawMetadata metadata)
{
    int width = imageBuf.spec().width;
    int height = imageBuf.spec().height;
    std::string font = "Roboto.ttf";
    float fontsize = imageBuf.spec().height * 0.02;
    float margin = fontsize * 0.2f; // 20% of fontsize for decenders
    float padding = fontsize * 0.2f; // 20% of fontsize for decenders
    float fontcolor[] = { 1, 1, 1, 1 };
    ROI size = ImageBufAlgo::text_size(metadata.name, fontsize, font_path(font));
    ROI roi(
        metadata.x - padding,
        metadata.x + size.width() + padding,
        metadata.y - size.height() - padding,
        metadata.y + margin + padding
    );
    ImageBufAlgo::fill(
        imageBuf,
        { 0, 0, 0, 0.5f },
        roi
    );
    ImageBufAlgo::render_text(
        imageBuf,
        metadata.x,
        metadata.y,
        metadata.name,
        fontsize,
        font_path(font),
        fontcolor,
        ImageBufAlgo::TextAlignX::Left,
        ImageBufAlgo::TextAlignY::Baseline
    );
    return roi;
}


// braw callback
class BrawCallback : public IBlackmagicRawCallback
{
    public:
        explicit BrawCallback() = default;
        virtual ~BrawCallback()
        {
            assert(m_refCount == 0);
            SetFrame(nullptr);
            m_imageBuf.clear();
        }
    
        virtual void ReadComplete(IBlackmagicRawJob* job, HRESULT result, IBlackmagicRawFrame* frame) {
            IBlackmagicRawJob* decodeAndProcessJob = nullptr;
            BlackmagicRawResourceFormat format = blackmagicRawResourceFormatRGBF32; // we always read float 32
            if (result == S_OK) {
                frame->SetResourceFormat(format);
            }
            if (result == S_OK) {
                IBlackmagicRawFrameProcessingAttributes* frameProcessingAttributes;
                frame->CloneFrameProcessingAttributes(&frameProcessingAttributes);
                if (m_kelvin.has_value()) {
                    Variant variant;
                    variant.vt = blackmagicRawVariantTypeU32;
                    variant.uintVal = m_kelvin.value();
                    frameProcessingAttributes->SetFrameAttribute(blackmagicRawFrameProcessingAttributeWhiteBalanceKelvin, &variant);
                }
                if (m_tint.has_value()) {
                    Variant variant;
                    variant.vt = blackmagicRawVariantTypeS16;
                    variant.uintVal = m_tint.value();
                    frameProcessingAttributes->SetFrameAttribute(blackmagicRawFrameProcessingAttributeWhiteBalanceTint, &variant);
                }
                if (m_exposure.has_value()) {
                    Variant variant;
                    variant.vt = blackmagicRawVariantTypeFloat32;
                    variant.fltVal = m_exposure.value();
                    frameProcessingAttributes->SetFrameAttribute(blackmagicRawFrameProcessingAttributeExposure, &variant);
                }
                result = frame->CreateJobDecodeAndProcessFrame(nullptr, frameProcessingAttributes, &decodeAndProcessJob);
            }
            if (result == S_OK) {
                result = decodeAndProcessJob->Submit();
            }
            if (result != S_OK) {
                if (decodeAndProcessJob)
                    decodeAndProcessJob->Release();
            }
            if (result == S_OK)
            {
                SetFrame(frame);
            }
            job->Release();
        }

        virtual void ProcessComplete(IBlackmagicRawJob* job, HRESULT result, IBlackmagicRawProcessedImage* processedImage)
        {
            unsigned int width = 0;
            unsigned int height = 0;
            unsigned int sizeBytes = 0;
            void* imageData = nullptr;
            if (result == S_OK) {
                result = processedImage->GetWidth(&width);
            }
            if (result == S_OK) {
                result = processedImage->GetHeight(&height);
            }
            if (result == S_OK) {
                result = processedImage->GetResourceSizeBytes(&sizeBytes);
            }
            if (result == S_OK) {
                result = processedImage->GetResource(&imageData);
            }
            if (result == S_OK) {
                ProcessImage(width, height, sizeBytes, imageData);
            }
            job->Release();
        }
    
        virtual void ProcessImage(uint32_t width, uint32_t height, uint32_t size, void* image)
        {
            const int channels = 3;
            const OIIO::TypeDesc format = OIIO::TypeDesc::FLOAT;
            ImageSpec spec(width, height, channels, format);
            m_imageBuf = ImageBuf(spec);
            m_imageBuf.set_pixels(
                OIIO::ROI(0, width, 0, height, 0, 1, 0, channels),
                format,
                image
            );
        }
    
        void ProcessMetaData(IBlackmagicRawMetadataIterator* metadataIterator)
        {
            char buffer[m_buffersize];
            const char *str = nullptr;
            CFStringRef key = nullptr;
            Variant value;
            HRESULT result;
            std::string attribute;
            while (SUCCEEDED(metadataIterator->GetKey(&key)))
            {
                if (CFStringGetCString(key, buffer, m_buffersize, kCFStringEncodingMacRoman))
                {
                    str = buffer;
                    attribute.clear();
                    attribute += str;
                }
                VariantInit(&value);
                result = metadataIterator->GetData(&value);
                if (result != S_OK)
                {
                    print_warning("could not get data from meta data iterator");
                    break;
                }
                BlackmagicRawVariantType variantType = value.vt;
                std::ostringstream valueStream;
                switch (variantType)
                {
                    case blackmagicRawVariantTypeS16:
                        m_imageBuf.specmod().attribute(attribute, value.iVal);
                        break;
                    case blackmagicRawVariantTypeU16:
                        m_imageBuf.specmod().attribute(attribute, value.uiVal);
                        break;
                    case blackmagicRawVariantTypeS32:
                        m_imageBuf.specmod().attribute(attribute, value.intVal);
                        break;
                    case blackmagicRawVariantTypeU32:
                        m_imageBuf.specmod().attribute(attribute, value.uintVal);
                        break;
                    case blackmagicRawVariantTypeFloat32:
                        m_imageBuf.specmod().attribute(attribute, value.fltVal);
                        break;
                    case blackmagicRawVariantTypeString:
                        if (CFStringGetCString(value.bstrVal, buffer, m_buffersize, kCFStringEncodingMacRoman))
                        {
                            m_imageBuf.specmod().attribute(attribute, buffer);
                        }
                        break;
                    case blackmagicRawVariantTypeSafeArray:
                    {
                        SafeArray* safeArray = value.parray;
                        void* safeArrayData = nullptr;
                        result = SafeArrayAccessData(safeArray, &safeArrayData);
                        if (result != S_OK)
                        {
                            print_warning("could not get safe array access in meta data iterator");
                            break;
                        }

                        BlackmagicRawVariantType arrayVarType;
                        result = SafeArrayGetVartype(safeArray, &arrayVarType);
                        if (result != S_OK)
                        {
                            print_warning("could not get variant type from safe array in meta data iterator");
                            SafeArrayUnaccessData(safeArray);
                            break;
                        }
                        long lBound, uBound;
                        SafeArrayGetLBound(safeArray, 1, &lBound);
                        SafeArrayGetUBound(safeArray, 1, &uBound);
                        std::ostringstream arrayValues;
                        for (long i = lBound; i <= uBound; i++)
                        {
                            switch (arrayVarType)
                            {
                                case blackmagicRawVariantTypeU8:
                                    arrayValues << static_cast<unsigned char*>(safeArrayData)[i - lBound];
                                    break;
                                case blackmagicRawVariantTypeS16:
                                    arrayValues << static_cast<short*>(safeArrayData)[i - lBound];
                                    break;
                                case blackmagicRawVariantTypeU16:
                                    arrayValues << static_cast<unsigned short*>(safeArrayData)[i - lBound];
                                    break;
                                case blackmagicRawVariantTypeS32:
                                    arrayValues << static_cast<int*>(safeArrayData)[i - lBound];
                                    break;
                                case blackmagicRawVariantTypeU32:
                                    arrayValues << static_cast<unsigned int*>(safeArrayData)[i - lBound];
                                    break;
                                case blackmagicRawVariantTypeFloat32:
                                    arrayValues << static_cast<float*>(safeArrayData)[i - lBound];
                                    break;
                            }
                            if (i < uBound) arrayValues << ", "; // Add a comma except for the last element
                        }
                        SafeArrayUnaccessData(safeArray);
                        m_imageBuf.specmod().attribute(attribute, arrayValues.str());
                    }
                    break;
                    default:
                        break;
                }
                VariantClear(&value);
                metadataIterator->Next();
            }
        }
        virtual void DecodeComplete(IBlackmagicRawJob*, HRESULT) {}
        virtual void TrimProgress(IBlackmagicRawJob*, float) {}
        virtual void TrimComplete(IBlackmagicRawJob*, HRESULT) {}
        virtual void SidecarMetadataParseWarning(IBlackmagicRawClip*, CFStringRef, uint32_t, CFStringRef) {}
        virtual void SidecarMetadataParseError(IBlackmagicRawClip*, CFStringRef, uint32_t, CFStringRef) {}
        virtual void PreparePipelineComplete(void*, HRESULT) {}
        virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID, LPVOID*)
        {
            return E_NOTIMPL;
        }
        virtual ULONG STDMETHODCALLTYPE AddRef(void)
        {
            return ++m_refCount;
        }

        virtual ULONG STDMETHODCALLTYPE Release(void)
        {
            const int32_t newRefValue = --m_refCount;
            if (newRefValue == 0) {
                delete this;
            }
            assert(newRefValue >= 0);
            return newRefValue;
        }
        float GetKelvin() const
        {
            return m_kelvin.value();
        }
        void SetKelvin(float kelvin)
        {
            m_kelvin = kelvin;
        }
        float GetTint() const
        {
            return m_tint.value();
        }
        void SetTint(float tint)
        {
            m_tint = tint;
        }
        float GetExposure() const
        {
            return m_exposure.value();
        }
        void SetExposure(float exposure)
        {
            m_exposure = exposure;
        }
        ImageBuf GetImageBuf()
        {
            return m_imageBuf;
        }
        IBlackmagicRawFrame* GetFrame()
        {
            return m_frame;
        }
        void SetFrame(IBlackmagicRawFrame* frame)
        {
            if (m_frame != nullptr) {
                m_frame->Release();
            }
            m_frame = frame;
            if (m_frame != nullptr) {
                m_frame->AddRef();
            }
        }
    private:
        boost::optional<int> m_kelvin;
        boost::optional<int> m_tint;
        boost::optional<float> m_exposure;
        IBlackmagicRawFrame* m_frame = nullptr;
        ImageBuf m_imageBuf;
        std::atomic<int32_t> m_refCount = {0};
        const int m_buffersize = 1024;
};

// braw colorspace
struct BrawColorspace
{
    std::string description;
    std::string filename;
};



// main
int 
main( int argc, const char * argv[])
{
    // Helpful for debugging to make sure that any crashes dump a stack
    // trace.
    Sysutil::setup_crash_stacktrace("stdout");

    Filesystem::convert_native_arguments(argc, (const char**)argv);
    ArgParse ap;

    ap.intro("brawtool -- a set of utilities for processing braw encoded images\n");
    ap.usage("brawtool [options] filename...")
      .add_help(false)
      .exit_on_error(true);
    
    ap.separator("General flags:");
    ap.arg("--help", &tool.help)
      .help("Print help message");
    
    ap.arg("-v", &tool.verbose)
      .help("Verbose status messages");
    
    ap.arg("--inputfilename %s:OUTFILENAME")
      .help("Input filename of braw file")
      .action(set_inputfilename);
    
    ap.arg("--kelvin %s:KELVIN")
      .help("Input white balance kelvin adjustment")
      .action(set_kelvin);
    
    ap.arg("--tint %s:TINT")
      .help("Input white balance tint adjustment")
      .action(set_tint);
    
    ap.arg("--exposure %s:EXPOSURE")
      .help("Input linear exposure adjustment")
      .action(set_exposure);

    ap.separator("Output flags:");
    ap.arg("--outputdirectory %s:OUTFILENAME")
      .help("Output directory of braw files")
      .action(set_outputdirectory);
    
    ap.arg("--clonebraw", &tool.clonebraw)
      .help("Clone braw file to output directory");
    
    ap.arg("--cloneproxy", &tool.cloneproxy)
      .help("Clone proxy directory to output directory");
    
    ap.arg("--apply3dlut", &tool.apply3dlut)
      .help("Apply 3dlut to preview image");
    
    ap.arg("--applymetadata", &tool.applymetadata)
      .help("Apply metadata to preview image");
    
    ap.arg("--override3dlut %s:OVERRIDE3DLUT")
      .help("Override 3dlut for preview image")
      .action(set_override3dlut);

    ap.arg("--width %s:WIDTH")
      .help("Output width of preview image")
      .action(set_width);
    
    ap.arg("--height %s:HEIGHT")
      .help("Output height of preview image")
      .action(set_height);

    // clang-format on
    if (ap.parse_args(argc, (const char**)argv) < 0) {
        print_error("Could no parse arguments: ", ap.geterror());
        print_help(ap);
        ap.abort();
        return EXIT_FAILURE;
    }
    if (ap["help"].get<int>()) {
        print_help(ap);
        ap.abort();
        return EXIT_SUCCESS;
    }
    if (argc <= 1) {
        ap.briefusage();
        print_error("For detailed help: brawtool --help");
        return EXIT_FAILURE;
    }
    
    // braw program
    print_info("brawtool -- a set of utilities for processing braw encoded images");
    
    // read colorspaces
    print_info("reading braw colorspaces");
    std::map<std::string, BrawColorspace> colorspaces;
    {
        std::string jsonfile = resources_path("brawtool.json");
        std::ifstream json(jsonfile);
        if (json.is_open()) {
            ptree pt;
            read_json(jsonfile, pt);
            for (const std::pair<const ptree::key_type, ptree&>& item : pt) {
                std::string name = item.first;
                const ptree data = item.second;
                
                BrawColorspace colorspace {
                    resources_path(data.get<std::string>("description", "")),
                    resources_path(data.get<std::string>("filename", "")),
                };
                
                if (!Filesystem::exists(colorspace.filename)) {
                    print_warning("'filename' does not exist for colorspace: ", colorspace.filename);
                    continue;
                }
            
                colorspaces[name] = colorspace;
            }
        } else {
            print_warning("could not open colorspaces file: ", jsonfile);
            ap.abort();
            return EXIT_FAILURE;
        }
        
        if (tool.override3dlut.size()) {
            if (!colorspaces.count(tool.override3dlut)) {
                print_error("unknown override 3dlut: ", tool.override3dlut);
                ap.abort();
                return EXIT_FAILURE;
            }
        }
    }
    
    // read braw data
    print_info("reading braw data from file: ", tool.inputfilename);
    ImageBuf imageBuf;
    {
        HRESULT result = S_OK;
        IBlackmagicRawFactory* factory = nullptr;
        factory = CreateBlackmagicRawFactoryInstanceFromPath(CFSTR(BlackmagicRaw_LIBRARY_PATH));
        if (factory == nullptr) {
            print_error("could not initialize blackmagic factory from path: ", BlackmagicRaw_LIBRARY_PATH);
            return EXIT_FAILURE;
        }

        IBlackmagicRaw* codec = nullptr;
        result = factory->CreateCodec(&codec);
        if (result != S_OK) {
            print_error("could not create codec from blackmagic api");
            return EXIT_FAILURE;
        }
        
        IBlackmagicRawClip* clip = nullptr;
        CFStringRef clipfilename = cfstr_by_str(tool.inputfilename);
        result = codec->OpenClip(clipfilename, &clip);
        if (result != S_OK) {
            print_error("could not open input filename: ", tool.inputfilename);
            return EXIT_FAILURE;
        }
        
        BrawCallback* callback = new BrawCallback();
        callback->AddRef();
        if (tool.kelvin.has_value()) {
            callback->SetKelvin(tool.kelvin.value());
        }
        if (tool.tint.has_value()) {
            callback->SetTint(tool.tint.value());
        }
        if (tool.exposure.has_value()) {
            callback->SetExposure(tool.exposure.value());
        }
        
        result = codec->SetCallback(callback);
        if (result != S_OK) {
            print_error("could not set callback for input filename: ", tool.inputfilename);
            return EXIT_FAILURE;
        }
        
        IBlackmagicRawMetadataIterator* clipMetadataIterator = nullptr;
        result = clip->GetMetadataIterator(&clipMetadataIterator);
        if (result != S_OK) {
            print_error("could not set get clip meta data for input filename: ", tool.inputfilename);
            return EXIT_FAILURE;
        }
        
        IBlackmagicRawJob* job = nullptr;
        long time = 0;
        result = clip->CreateJobReadFrame(time, &job);
        if (result != S_OK) {
            print_error("could not read frame for input filename: ", tool.inputfilename);
            return EXIT_FAILURE;
        }
        
        result = job->Submit();
        job->Release();
        if (result != S_OK)
        {
            print_error("could not submit job for input filename: ", tool.inputfilename);
            return EXIT_FAILURE;
        }
        codec->FlushJobs();
        IBlackmagicRawFrame* frame = callback->GetFrame();
        if (frame == nullptr) {
            print_error("could not get frame for input filename: ", tool.inputfilename);
            return EXIT_FAILURE;
        }

        IBlackmagicRawMetadataIterator* frameMetadataIterator = nullptr;
        result = frame->GetMetadataIterator(&frameMetadataIterator);
        if (result != S_OK) {
            print_error("could not get frame meta data for input filename: ", tool.inputfilename);
            return EXIT_FAILURE;
        }
        
        // metadata
        callback->ProcessMetaData(clipMetadataIterator);
        callback->ProcessMetaData(frameMetadataIterator);

        imageBuf = callback->GetImageBuf(); // this is a deep copy
        if (imageBuf.has_error()) {
            print_error("could not read image buffer from filename: ", tool.inputfilename);
        }
        if (callback->GetFrame() != nullptr) {
            callback->SetFrame(nullptr); // needed to force codec to release callback, reported to bm support
        }
        if (callback != nullptr) {
            callback->Release();
        }
        if (clipMetadataIterator != nullptr) {
            clipMetadataIterator->Release();
        }
        if (frameMetadataIterator != nullptr) {
            frameMetadataIterator->Release();
        }
        if (clip != nullptr) {
            clip->Release();
        }
        if (codec != nullptr) {
            codec->Release();
        }
        if (factory != nullptr) {
            factory->Release();
        }
        CFRelease(clipfilename);
        
        if (tool.width.has_value() > 0 ||
            tool.height.has_value() > 0) {
            
            ImageSpec spec = imageBuf.spec();
            int width = tool.width.value();
            int height = tool.height.value();
        
            float aspectratio = static_cast<float>(spec.width) / spec.height;
            float resizeaspectratio = static_cast<float>(width) / height;

            int resizewidth, resizeheight;
            if (aspectratio > resizeaspectratio) {
                resizewidth = width;
                resizeheight = static_cast<int>(width / aspectratio);
            } else {
                resizewidth = static_cast<int>(height * aspectratio);
                resizeheight = height;
            }
            ImageBuf resizedbuf;
            ImageBufAlgo::resize(resizedbuf, imageBuf, "triangle", 0, ROI(0, resizewidth, 0, resizeheight));

            ImageSpec copyspec(width, height, spec.nchannels, spec.format);
            ImageBuf copybuf(copyspec);
            ImageBufAlgo::zero(copybuf);
            
            int xoffset = (width - resizewidth) / 2;
            int yoffset = (height - resizeheight) / 2;

            ImageBufAlgo::paste(copybuf, xoffset, yoffset, 0, 0, resizedbuf);
            imageBuf.copy(copybuf);
        }
    }

    // read sidecar
    std::string sidecarfile = combine_path(
        filename_path(tool.inputfilename) + "/Proxy",
        filename(extension(tool.inputfilename, "sidecar"))
    );
    
    print_info("reading braw sidecardata from file: ", sidecarfile);
    std::ifstream sidecar(sidecarfile);
    std::string lutfile;
    if (sidecar.is_open()) {
        std::string line;
        std::string name, title, data;
        int lutSize = 0;
        boost::regex multispaces("\\s{2,}");
        boost::regex leadingspaces("^\\s+");
        bool datarun = false;
        while (getline(sidecar, line)) {
            if (datarun) {
               if (line.find("\"") != std::string::npos) {
                   std::string value = line.substr(0, line.rfind("\""));
                   value = boost::regex_replace(value, multispaces, " ");
                   value = boost::regex_replace(value, leadingspaces, "");
                   data += value;
                   break;
               }
               std::string value = boost::regex_replace(line, multispaces, " ");
               value = boost::regex_replace(value, leadingspaces, "");
               data += value + "\n";
               continue;
            }
            if (line.find("\"post_3dlut_sidecar_name\":") != std::string::npos) {
                boost::regex expr(R"("post_3dlut_sidecar_name"\s*:\s*"([^"]*)\")");
                boost::smatch what;
                if (boost::regex_search(line, what, expr)) {
                    name = what[1];
                }
            } else if (line.find("\"post_3dlut_sidecar_title\":") != std::string::npos) {
                boost::regex expr(R"("post_3dlut_sidecar_title"\s*:\s*"([^"]*)\")");
                boost::smatch what;
                if (boost::regex_search(line, what, expr)) {
                    title = what[1];
                }
            } else if (line.find("\"post_3dlut_sidecar_data\":") != std::string::npos) {
                datarun = true;
                size_t startPos = line.find("\"", line.find(":")) + 1;
                if (startPos != std::string::npos && startPos < line.size()) {
                    std::string value = line.substr(startPos);
                    if (!value.empty() && value.find("\"") != std::string::npos) {
                        value = value.substr(0, value.find("\""));
                    }
                    value = boost::regex_replace(value, multispaces, " ");
                    value = boost::regex_replace(value, leadingspaces, "");
                    data += value + (value.empty() ? "" : "\n");
                }
            }
        }
        lutfile = combine_path(
            filename_path(tool.inputfilename) + "/3DLut", name
        );
        
        if (!exists(lutfile)) {
            std::ofstream outputFile(lutfile);
            if (outputFile)
            {
                outputFile << "BMD_TITLE " << title << std::endl;
                outputFile << std::endl;
                outputFile << "LUT_3D_SIZE " << std::to_string(lutSize) << std::endl;
                outputFile << data;
                outputFile.close();
                
            } else {
                print_error("could not open output false color cube (lut) file: ", lutfile);
                return EXIT_FAILURE;
            }
        }
    } else {
        print_warning("could not find sidecar file: ", sidecarfile);
        return EXIT_FAILURE;
    }
    
    // apply 3dlut
    if (tool.apply3dlut)
    {
        print_info("applying 3dlut from file from sidecar");
        {
            ConstConfigRcPtr config = Config::CreateRaw();
            ConstCPUProcessorRcPtr colorspaceProcessor;
            
            FileTransformRcPtr transform = FileTransform::Create();
            transform->setSrc(lutfile.c_str());
            transform->setInterpolation(INTERP_BEST);
            
            ConstProcessorRcPtr processor = config->getProcessor(transform);
            colorspaceProcessor = processor->getDefaultCPUProcessor();
            {
                const ImageSpec &spec = imageBuf.spec();
                int xres = spec.width;
                int yres = spec.height;
                int channels = spec.nchannels;
                ROI roi = ROI(0, xres, 0, yres, 0, 1, 0, channels);
                std::vector<float> pixels(roi.width() * roi.height() * roi.nchannels());
                
                if (!imageBuf.get_pixels(roi, TypeDesc::FLOAT, &pixels[0])) {
                    print_error("failed to get pixel data from the image buffer");
                    return EXIT_FAILURE;
                }
                PackedImageDesc imgDesc(&pixels[0], roi.width(), roi.height(), roi.nchannels());

                // apply color transformation
                colorspaceProcessor->apply(imgDesc);
                imageBuf.set_pixels(roi, TypeDesc::FLOAT, &pixels[0]);
            }
        }
    }
    
    // apply metadata
    if (tool.applymetadata)
    {
        print_info("applying metadata from attributes");
        {
            std::vector<BrawMetadata> metadatas =
            {
                BrawMetadata() = { "filename", "filename", TypeDesc::STRING, 0, 0 },
                BrawMetadata() = { "exposure", "exposure", TypeDesc::STRING, 0, 0 },
                BrawMetadata() = { "sensor_rate", "fps", TypeDesc::STRING, 0, 0 },
                BrawMetadata() = { "shutter_value", "shutter", TypeDesc::STRING, 0, 0 },
                BrawMetadata() = { "aperture", "iris", TypeDesc::STRING, 0, 0 },
                BrawMetadata() = { "iso", "iso", TypeDesc::INT, 0, 0 },
                BrawMetadata() = { "white_balance_kelvin", "wb", TypeDesc::INT, 0, 0 },
                BrawMetadata() = { "white_balance_tint", "tint", TypeDesc::INT, 0, 0 },
                BrawMetadata() = { "lens_type", "lens", TypeDesc::STRING, 0, 0 },
                BrawMetadata() = { "focal_length", "focal length", TypeDesc::STRING, 0, 0 },
                BrawMetadata() = { "distance", "focus", TypeDesc::STRING, 0, 0 },
                BrawMetadata() = { "date_recorded", "date", TypeDesc::STRING, 0, 0 },
            };
            int width = imageBuf.spec().width;
            int height = imageBuf.spec().height;
            int x = width * 0.02;
            int y = height * 0.04f;
            for(BrawMetadata metadata : metadatas)
            {
                const ImageSpec &spec = imageBuf.spec();
                int width = imageBuf.spec().width;
                int height = imageBuf.spec().height;
                if (metadata.key == "filename") {
                    metadata.name = filename(tool.inputfilename);
                } else {
                    const ParamValue* attr = spec.find_attribute(metadata.key);
                    if (attr) {
                        std::string value;
                        const TypeDesc type = attr->type();
                        switch (type.basetype) {
                            case TypeDesc::STRING:
                                value = *(const char**)attr->data();
                                break;
                            case TypeDesc::FLOAT:
                                value = str_by_float(*(const float*)attr->data());
                                break;
                            case TypeDesc::INT8:
                                value = std::to_string(*(const char*)attr->data());
                                break;
                            case TypeDesc::UINT8:
                                value = std::to_string(*(const unsigned char*)attr->data());
                                break;
                            case TypeDesc::INT16:
                                value = std::to_string(*(const short*)attr->data());
                                break;
                            case TypeDesc::UINT16:
                                value = std::to_string(*(const unsigned short*)attr->data());
                                break;
                            case TypeDesc::INT:
                                value = std::to_string(*(const int*)attr->data());
                                break;
                            case TypeDesc::UINT:
                                value = std::to_string(*(const unsigned int*)attr->data());
                                break;
                        }
                        if (metadata.key == "exposure") {
                            if (tool.exposure.has_value()) {
                                metadata.name = metadata.name + ": " + str_by_float(tool.exposure.value()) + " (" + value + ")";
                            } else {
                                metadata.name = metadata.name + ": " + value;
                            }
                        } else if (metadata.key == "white_balance_kelvin") {
                            if (tool.kelvin.has_value()) {
                                metadata.name = metadata.name + ": " + std::to_string(tool.kelvin.value()) + " (" + value + ")";
                            } else {
                                metadata.name = metadata.name + ": " + value;
                            }
                        } else if (metadata.key == "white_balance_tint") {
                            if (tool.tint.has_value()) {
                                metadata.name = metadata.name + ": " + std::to_string(tool.tint.value()) + " (" + value + ")";
                            } else {
                                metadata.name = metadata.name + ": " + value;
                            }
                        } else {
                            metadata.name = metadata.name + ": " + value;
                        }
                    }
                }
                metadata.x = x;
                metadata.y = y;
                y += draw_metadata(imageBuf, metadata).height() + height * 0.01f;
            }
        }
    }
    
    // clone braw
    if (tool.clonebraw)
    {
        std::string clonefilename = combine_path(
            tool.outputdirectory,
            filename(tool.inputfilename)
        );
        copy_file(tool.inputfilename, clonefilename);
        if (!file_compare(tool.inputfilename, clonefilename)) {
            print_error("failed when trying to clone input file to: ", clonefilename);
            return EXIT_FAILURE;
        }
    }
    
    // clone proxy
    if (tool.cloneproxy)
    {
        std::string proxydirname = combine_path(
            tool.outputdirectory,
            "Proxy"
        );
        if (!exists(proxydirname)) {
            if (!create_path(proxydirname)) {
                print_error("could not create proxy directory: ", proxydirname);
                return EXIT_FAILURE;
            }
        }
        
        // mp4
        std::string mp4file = combine_path(
            filename_path(tool.inputfilename) + "/Proxy",
            filename(extension(tool.inputfilename, "mp4"))
        );
        if (exists(mp4file)) {
            std::string mp4outputfile = combine_path(
                proxydirname,
                filename(mp4file)
            );
            copy_file(mp4file, mp4outputfile);
            if (!file_compare(mp4file, mp4outputfile)) {
                print_error("failed when trying to clone mp4 file to: ", mp4outputfile);
                return EXIT_FAILURE;
            }
            
        } else {
            print_warning("could not find proxy mp4 file: ", mp4file);
        }

        // sidecar
        std::string sidecarfile = combine_path(
            filename_path(tool.inputfilename) + "/Proxy",
            filename(extension(tool.inputfilename, "sidecar"))
        );
        if (exists(sidecarfile)) {
            std::string sidecaroutputfile = combine_path(
                proxydirname,
                filename(sidecarfile)
            );
            copy_file(sidecarfile, sidecaroutputfile);
            if (!file_compare(sidecarfile, sidecaroutputfile)) {
                print_error("failed when trying to clone sidecar file to: ", sidecaroutputfile);
                return EXIT_FAILURE;
            }
            
        } else {
            print_warning("could not find proxy mp4 file: ", mp4file);
        }
    }

    std::string outputfilename = combine_path(
        tool.outputdirectory,
        filename(extension(tool.inputfilename, ".png"))
    );

    print_info("writing output file: ", outputfilename);

    if (!imageBuf.write(outputfilename)) {
        print_error("could not write file: ", imageBuf.geterror());
        return EXIT_FAILURE;
    }
    
    return EXIT_SUCCESS;
}
