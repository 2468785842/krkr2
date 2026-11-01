#include "storage/impl/ArchiveFilename.h"
#include <algorithm>

void TVPFreeArchiveHandlePoolByPointer(void *pointer);

void TVPReleaseCachedArchiveHandle(void *pointer, tTJSBinaryStream *stream);

//---------------------------------------------------------------------------
// tar archive
//---------------------------------------------------------------------------
tjs_uint64 parseOctNum(const char *oct, int length) {
    tjs_uint64 num = 0;
    for(int i = 0; i < length; i++) {
        char c = oct[i];
        if('0' <= c && c <= '9') {
            num = num * 8 + (c - '0');
        }
    }
    return num;
}

#include "tar.h"

class TARArchive : public tTVPArchive {
    struct EntryInfo {
        ttstr filename;
        tjs_uint64 offset{};
        tjs_uint64 size{};
    };
    std::vector<EntryInfo> filelist;

    friend class TARArchiveStream;

public:
    TARArchive(const ttstr &arcname) : tTVPArchive(arcname) {}

    ~TARArchive() override { TVPFreeArchiveHandlePoolByPointer(this); }

    bool init(tTJSBinaryStream *_instr, bool normalizeFileName) {
        if(_instr) {
            tjs_uint64 archiveSize = _instr->GetSize();
            TAR_HEADER tar_header;
            // check first header
            if(_instr->Read(&tar_header, sizeof(tar_header)) !=
               sizeof(tar_header)) {
                // delete _instr;
                return false;
            }
            unsigned int checksum = parseOctNum(tar_header.dbuf.chksum,
                                                sizeof(tar_header.dbuf.chksum));
            if(checksum != tar_header.compsum() &&
               (int)checksum != tar_header.compsum_oldtar()) {
                // delete _instr;
                return false;
            }
            _instr->SetPosition(0);
            while(_instr->GetPosition() <= archiveSize - sizeof(tar_header)) {
                if(_instr->Read(&tar_header, sizeof(tar_header)) !=
                   sizeof(tar_header))
                    break;
                tjs_uint64 original_size = parseOctNum(
                    tar_header.dbuf.size, sizeof(tar_header.dbuf.size));
                std::vector<char> filename;
                if(tar_header.dbuf.typeflag ==
                   LONGLINK) { // tar_header.dbuf.name ==
                               // "././@LongLink"
                    const unsigned int readsize = static_cast<unsigned int>(
                        (original_size + (TBLOCK - 1)) & ~(TBLOCK - 1));
                    std::vector<char> block(readsize);
                    if(_instr->Read(block.data(), readsize) != readsize)
                        break;

                    size_t name_len = static_cast<size_t>(original_size);
                    if(name_len > block.size())
                        name_len = block.size();
                    const char *end = static_cast<const char *>(
                        memchr(block.data(), '\0', name_len));
                    if(end)
                        name_len = static_cast<size_t>(end - block.data());
                    while(name_len > 0 &&
                          (block[name_len - 1] == '\n' ||
                           block[name_len - 1] == '\0'))
                        name_len--;

                    filename.resize(name_len + 1);
                    if(name_len > 0)
                        memcpy(filename.data(), block.data(), name_len);
                    filename[name_len] = '\0';
                    if(_instr->Read(&tar_header, sizeof(tar_header)) !=
                       sizeof(tar_header))
                        break;
                    original_size = parseOctNum(tar_header.dbuf.size,
                                                sizeof(tar_header.dbuf.size));
                }
                if(tar_header.dbuf.typeflag != REGTYPE)
                    continue; // only accept regular file
                if(filename.empty()) {
                    filename.resize(101);
                    memcpy(&filename[0], tar_header.dbuf.name,
                           sizeof(tar_header.dbuf.name));
                    filename[100] = 0;
                }
                EntryInfo item;
                storeFilename(item.filename, &filename[0], ArchiveName);
                if(normalizeFileName)
                    NormalizeInArchiveStorageName(item.filename);
                item.size = original_size;
                item.offset = _instr->GetPosition();
                filelist.emplace_back(item);
                tjs_uint64 readsize =
                    (original_size + (TBLOCK - 1)) & ~(TBLOCK - 1);
                _instr->SetPosition(item.offset + readsize);
            }
            if(normalizeFileName) {
                std::sort(filelist.begin(), filelist.end(),
                          [](const EntryInfo &a, const EntryInfo &b) {
                              return a.filename < b.filename;
                          });
            }
            TVPReleaseCachedArchiveHandle(this, _instr);
            return true;
        }
        return false;
    }

    tjs_uint GetCount() override { return filelist.size(); }

    ttstr GetName(tjs_uint idx) override { return filelist[idx].filename; }

    tTJSBinaryStream *CreateStreamByIndex(tjs_uint idx) override;
};

tTJSBinaryStream *TARArchive::CreateStreamByIndex(tjs_uint idx) {
    const EntryInfo &info = filelist[idx];
    return new TArchiveStream(this, info.offset, info.size);
}

tTVPArchive *TVPOpenTARArchive(const ttstr &name, tTJSBinaryStream *st,
                               bool normalizeFileName) {
    auto *arc = new TARArchive(name);
    if(!arc->init(st, normalizeFileName)) {
        delete arc;
        return nullptr;
    }
    return arc;
}
