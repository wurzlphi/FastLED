#include "fl/file_system.h"
#include "fl/unused.h"
#include "fl/warn.h"

#ifdef __EMSCRIPTEN__
#include "platforms/wasm/fs_wasm.h"
#define FASTLED_HAS_SDCARD 1
#elif __has_include(<SD.h>) && __has_include(<fs.h>)
// Include Arduino SD card implementation when SD library is available
#include "platforms/fs_sdcard_arduino.hpp"
#define FASTLED_HAS_SDCARD 1
#else
#define FASTLED_HAS_SDCARD 0
#endif

#include "fl/json.h"
#include "fl/namespace.h"
#include "fl/screenmap.h"
#include "fl/unused.h"

namespace fl {

class NullFileHandle : public FileHandle {
  public:
    NullFileHandle() = default;
    ~NullFileHandle() override {}

    bool available() const override { return false; }
    fl::sz size() const override { return 0; }
    fl::sz read(uint8_t *dst, fl::sz bytesToRead) override {
        FASTLED_UNUSED(dst);
        FASTLED_UNUSED(bytesToRead);
        return 0;
    }
    fl::sz pos() const override { return 0; }
    const char *path() const override { return "NULL FILE HANDLE"; }
    bool seek(fl::sz pos) override {
        FASTLED_UNUSED(pos);
        return false;
    }
    void close() override {}
    bool valid() const override {
        FASTLED_WARN("NullFileHandle is not valid");
        return false;
    }
};

class NullFileSystem : public FsImpl {
  public:
    NullFileSystem() {
        FASTLED_WARN("NullFileSystem instantiated as a placeholder, please "
                     "implement a file system for your platform.");
    }
    ~NullFileSystem() override {}

    bool begin() override { return true; }
    void end() override {}

    void close(FileHandlePtr file) override {
        // No need to do anything for in-memory files
        FASTLED_UNUSED(file);
        FASTLED_WARN("NullFileSystem::close");
    }

    FileHandlePtr openRead(const char *_path) override {
        FASTLED_UNUSED(_path);
        FileHandlePtr out = FileHandlePtr::TakeOwnership(new NullFileHandle());
        return out;
    }
};

bool FileSystem::beginSd(int cs_pin) {
    mFs = make_sdcard_filesystem(cs_pin);
    if (!mFs) {
        return false;
    }
    mFs->begin();
    return true;
}

bool FileSystem::begin(FsImplPtr platform_filesystem) {
    mFs = platform_filesystem;
    if (!mFs) {
        return false;
    }
    mFs->begin();
    return true;
}

fl::sz FileHandle::bytesLeft() const { return size() - pos(); }

FileSystem::FileSystem() : mFs() {}

void FileSystem::end() {
    if (mFs) {
        mFs->end();
    }
}

bool FileSystem::readJson(const char *path, JsonDocument *doc) {
    string text;
    if (!readText(path, &text)) {
        return false;
    }
    return parseJson(text.c_str(), doc);
}

bool FileSystem::readScreenMaps(const char *path,
                                FixedMap<string, ScreenMap, 16> *out, string *error) {
    string text;
    if (!readText(path, &text)) {
        FASTLED_WARN("Failed to read file: " << path);
        if (error) {
            *error = "Failed to read file: ";
            error->append(path);
        }
        return false;
    }
    string err;
    bool ok = ScreenMap::ParseJson(text.c_str(), out, &err);
    if (!ok) {
        FASTLED_WARN("Failed to parse screen map: " << err.c_str());
        *error = err;
        return false;
    }
    return true;
}

bool FileSystem::readScreenMap(const char *path, const char *name,
                               ScreenMap *out, string *error) {
    string text;
    if (!readText(path, &text)) {
        FASTLED_WARN("Failed to read file: " << path);
        if (error) {
            *error = "Failed to read file: ";
            error->append(path);
        }
        return false;
    }
    string err;
    bool ok = ScreenMap::ParseJson(text.c_str(), name, out, &err);
    if (!ok) {
        FASTLED_WARN("Failed to parse screen map: " << err.c_str());
        *error = err;
        return false;
    }
    return true;
}

void FileSystem::close(FileHandlePtr file) { mFs->close(file); }

FileHandlePtr FileSystem::openRead(const char *path) {
    return mFs->openRead(path);
}
Video FileSystem::openVideo(const char *path, fl::sz pixelsPerFrame, float fps,
                            fl::sz nFrameHistory) {
    Video video(pixelsPerFrame, fps, nFrameHistory);
    FileHandlePtr file = openRead(path);
    if (!file) {
        video.setError(fl::string("Could not open file: ").append(path));
        return video;
    }
    video.begin(file);
    return video;
}

bool FileSystem::readText(const char *path, fl::string *out) {
    FileHandlePtr file = openRead(path);
    if (!file) {
        FASTLED_WARN("Failed to open file: " << path);
        return false;
    }
    fl::sz size = file->size();
    out->reserve(size + out->size());
    bool wrote = false;
    while (file->available()) {
        uint8_t buf[64];
        fl::sz n = file->read(buf, sizeof(buf));
        // out->append(buf, n);
        out->append((const char *)buf, n);
        wrote = true;
    }
    file->close();
    FASTLED_DBG_IF(!wrote, "Failed to write any data to the output string.");
    return wrote;
}
} // namespace fl

namespace fl {
#if !FASTLED_HAS_SDCARD
// Weak fallback implementation when SD library is not available
__attribute__((weak)) FsImplPtr make_sdcard_filesystem(int cs_pin) {
    FASTLED_UNUSED(cs_pin);
    FsImplPtr out = FsImplPtr::TakeOwnership(new NullFileSystem());
    return out;
}
#endif

} // namespace fl
