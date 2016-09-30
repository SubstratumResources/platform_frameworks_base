//
// Copyright 2006 The Android Open Source Project
//
// Information about assets being operated on.
//
#ifndef __AOPT_ASSETS_H
#define __AOPT_ASSETS_H

#include <androidfw/AssetManager.h>
#include <androidfw/ResourceTypes.h>
#include <stdlib.h>
#include <set>
#include <utils/KeyedVector.h>
#include <utils/RefBase.h>
#include <utils/SortedVector.h>
#include <utils/String8.h>
#include <utils/Vector.h>

#include "AoptConfig.h"
#include "Bundle.h"
#include "ConfigDescription.h"
#include "SourcePos.h"
#include "ZipFile.h"

using namespace android;

extern const char * const gDefaultIgnoreAssets;
extern const char * gUserIgnoreAssets;

bool valid_symbol_name(const String8& str);

class AoptAssets;

enum {
    AXIS_NONE = 0,
    AXIS_MCC = 1,
    AXIS_MNC,
    AXIS_LOCALE,
    AXIS_SCREENLAYOUTSIZE,
    AXIS_SCREENLAYOUTLONG,
    AXIS_ORIENTATION,
    AXIS_UIMODETYPE,
    AXIS_UIMODENIGHT,
    AXIS_DENSITY,
    AXIS_TOUCHSCREEN,
    AXIS_KEYSHIDDEN,
    AXIS_KEYBOARD,
    AXIS_NAVHIDDEN,
    AXIS_NAVIGATION,
    AXIS_SCREENSIZE,
    AXIS_SMALLESTSCREENWIDTHDP,
    AXIS_SCREENWIDTHDP,
    AXIS_SCREENHEIGHTDP,
    AXIS_LAYOUTDIR,
    AXIS_VERSION,

    AXIS_START = AXIS_MCC,
    AXIS_END = AXIS_VERSION,
};

struct AoptLocaleValue {
     char language[4];
     char region[4];
     char script[4];
     char variant[8];

     AoptLocaleValue() {
         memset(this, 0, sizeof(AoptLocaleValue));
     }

     // Initialize this AoptLocaleValue from a config string.
     bool initFromFilterString(const String8& config);

     int initFromDirName(const Vector<String8>& parts, const int startIndex);

     // Initialize this AoptLocaleValue from a ResTable_config.
     void initFromResTable(const ResTable_config& config);

     void writeTo(ResTable_config* out) const;

     int compare(const AoptLocaleValue& other) const {
         return memcmp(this, &other, sizeof(AoptLocaleValue));
     }

     inline bool operator<(const AoptLocaleValue& o) const { return compare(o) < 0; }
     inline bool operator<=(const AoptLocaleValue& o) const { return compare(o) <= 0; }
     inline bool operator==(const AoptLocaleValue& o) const { return compare(o) == 0; }
     inline bool operator!=(const AoptLocaleValue& o) const { return compare(o) != 0; }
     inline bool operator>=(const AoptLocaleValue& o) const { return compare(o) >= 0; }
     inline bool operator>(const AoptLocaleValue& o) const { return compare(o) > 0; }
private:
     void setLanguage(const char* language);
     void setRegion(const char* language);
     void setScript(const char* script);
     void setVariant(const char* variant);
};

/**
 * This structure contains a specific variation of a single file out
 * of all the variations it can have that we can have.
 */
struct AoptGroupEntry
{
public:
    AoptGroupEntry() {}
    AoptGroupEntry(const ConfigDescription& config) : mParams(config) {}

    bool initFromDirName(const char* dir, String8* resType);

    inline const ConfigDescription& toParams() const { return mParams; }

    inline int compare(const AoptGroupEntry& o) const { return mParams.compareLogical(o.mParams); }
    inline bool operator<(const AoptGroupEntry& o) const { return compare(o) < 0; }
    inline bool operator<=(const AoptGroupEntry& o) const { return compare(o) <= 0; }
    inline bool operator==(const AoptGroupEntry& o) const { return compare(o) == 0; }
    inline bool operator!=(const AoptGroupEntry& o) const { return compare(o) != 0; }
    inline bool operator>=(const AoptGroupEntry& o) const { return compare(o) >= 0; }
    inline bool operator>(const AoptGroupEntry& o) const { return compare(o) > 0; }

    String8 toString() const { return mParams.toString(); }
    String8 toDirName(const String8& resType) const;

    const String8 getVersionString() const { return AoptConfig::getVersion(mParams); }

private:
    ConfigDescription mParams;
};

inline int compare_type(const AoptGroupEntry& lhs, const AoptGroupEntry& rhs)
{
    return lhs.compare(rhs);
}

inline int strictly_order_type(const AoptGroupEntry& lhs, const AoptGroupEntry& rhs)
{
    return compare_type(lhs, rhs) < 0;
}

class AoptGroup;
class FilePathStore;

/**
 * A single asset file we know about.
 */
class AoptFile : public RefBase
{
public:
    AoptFile(const String8& sourceFile, const AoptGroupEntry& groupEntry,
             const String8& resType)
        : mGroupEntry(groupEntry)
        , mResourceType(resType)
        , mSourceFile(sourceFile)
        , mData(NULL)
        , mDataSize(0)
        , mBufferSize(0)
        , mCompression(ZipEntry::kCompressStored)
        {
            //printf("new AoptFile created %s\n", (const char*)sourceFile);
        }
    virtual ~AoptFile() {
        free(mData);
    }

    const String8& getPath() const { return mPath; }
    const AoptGroupEntry& getGroupEntry() const { return mGroupEntry; }

    // Data API.  If there is data attached to the file,
    // getSourceFile() is not used.
    bool hasData() const { return mData != NULL; }
    const void* getData() const { return mData; }
    size_t getSize() const { return mDataSize; }
    void* editData(size_t size);
    void* editData(size_t* outSize = NULL);
    void* editDataInRange(size_t offset, size_t size);
    void* padData(size_t wordSize);
    status_t writeData(const void* data, size_t size);
    void clearData();

    const String8& getResourceType() const { return mResourceType; }

    // File API.  If the file does not hold raw data, this is
    // a full path to a file on the filesystem that holds its data.
    const String8& getSourceFile() const { return mSourceFile; }

    String8 getPrintableSource() const;

    // Desired compression method, as per utils/ZipEntry.h.  For example,
    // no compression is ZipEntry::kCompressStored.
    int getCompressionMethod() const { return mCompression; }
    void setCompressionMethod(int c) { mCompression = c; }
private:
    friend class AoptGroup;

    String8 mPath;
    AoptGroupEntry mGroupEntry;
    String8 mResourceType;
    String8 mSourceFile;
    void* mData;
    size_t mDataSize;
    size_t mBufferSize;
    int mCompression;
};

/**
 * A group of related files (the same file, with different
 * vendor/locale variations).
 */
class AoptGroup : public RefBase
{
public:
    AoptGroup(const String8& leaf, const String8& path)
        : mLeaf(leaf), mPath(path) { }
    virtual ~AoptGroup() { }

    const String8& getLeaf() const { return mLeaf; }

    // Returns the relative path after the AoptGroupEntry dirs.
    const String8& getPath() const { return mPath; }

    const DefaultKeyedVector<AoptGroupEntry, sp<AoptFile> >& getFiles() const
        { return mFiles; }

    status_t addFile(const sp<AoptFile>& file, const bool overwriteDuplicate=false);
    void removeFile(size_t index);

    void print(const String8& prefix) const;

    String8 getPrintableSource() const;

private:
    String8 mLeaf;
    String8 mPath;

    DefaultKeyedVector<AoptGroupEntry, sp<AoptFile> > mFiles;
};

/**
 * A single directory of assets, which can contain files and other
 * sub-directories.
 */
class AoptDir : public RefBase
{
public:
    AoptDir(const String8& leaf, const String8& path)
        : mLeaf(leaf), mPath(path) { }
    virtual ~AoptDir() { }

    const String8& getLeaf() const { return mLeaf; }

    const String8& getPath() const { return mPath; }

    const DefaultKeyedVector<String8, sp<AoptGroup> >& getFiles() const { return mFiles; }
    const DefaultKeyedVector<String8, sp<AoptDir> >& getDirs() const { return mDirs; }

    virtual status_t addFile(const String8& name, const sp<AoptGroup>& file);

    void removeFile(const String8& name);
    void removeDir(const String8& name);

    /*
     * Perform some sanity checks on the names of files and directories here.
     * In particular:
     *  - Check for illegal chars in filenames.
     *  - Check filename length.
     *  - Check for presence of ".gz" and non-".gz" copies of same file.
     *  - Check for multiple files whose names match in a case-insensitive
     *    fashion (problematic for some systems).
     *
     * Comparing names against all other names is O(n^2).  We could speed
     * it up some by sorting the entries and being smarter about what we
     * compare against, but I'm not expecting to have enough files in a
     * single directory to make a noticeable difference in speed.
     *
     * Note that sorting here is not enough to guarantee that the package
     * contents are sorted -- subsequent updates can rearrange things.
     */
    status_t validate() const;

    void print(const String8& prefix) const;

    String8 getPrintableSource() const;

private:
    friend class AoptAssets;

    status_t addDir(const String8& name, const sp<AoptDir>& dir);
    sp<AoptDir> makeDir(const String8& name);
    status_t addLeafFile(const String8& leafName,
                         const sp<AoptFile>& file,
                         const bool overwrite=false);
    virtual ssize_t slurpFullTree(Bundle* bundle,
                                  const String8& srcDir,
                                  const AoptGroupEntry& kind,
                                  const String8& resType,
                                  sp<FilePathStore>& fullResPaths,
                                  const bool overwrite=false);

    String8 mLeaf;
    String8 mPath;

    DefaultKeyedVector<String8, sp<AoptGroup> > mFiles;
    DefaultKeyedVector<String8, sp<AoptDir> > mDirs;
};

/**
 * All information we know about a particular symbol.
 */
class AoptSymbolEntry
{
public:
    AoptSymbolEntry()
        : isPublic(false), isJavaSymbol(false), typeCode(TYPE_UNKNOWN)
    {
    }
    AoptSymbolEntry(const String8& _name)
        : name(_name), isPublic(false), isJavaSymbol(false), typeCode(TYPE_UNKNOWN)
    {
    }
    AoptSymbolEntry(const AoptSymbolEntry& o)
        : name(o.name), sourcePos(o.sourcePos), isPublic(o.isPublic)
        , isJavaSymbol(o.isJavaSymbol), comment(o.comment), typeComment(o.typeComment)
        , typeCode(o.typeCode), int32Val(o.int32Val), stringVal(o.stringVal)
    {
    }
    AoptSymbolEntry operator=(const AoptSymbolEntry& o)
    {
        sourcePos = o.sourcePos;
        isPublic = o.isPublic;
        isJavaSymbol = o.isJavaSymbol;
        comment = o.comment;
        typeComment = o.typeComment;
        typeCode = o.typeCode;
        int32Val = o.int32Val;
        stringVal = o.stringVal;
        return *this;
    }
    
    const String8 name;
    
    SourcePos sourcePos;
    bool isPublic;
    bool isJavaSymbol;
    
    String16 comment;
    String16 typeComment;
    
    enum {
        TYPE_UNKNOWN = 0,
        TYPE_INT32,
        TYPE_STRING
    };
    
    int typeCode;
    
    // Value.  May be one of these.
    int32_t int32Val;
    String8 stringVal;
};

/**
 * A group of related symbols (such as indices into a string block)
 * that have been generated from the assets.
 */
class AoptSymbols : public RefBase
{
public:
    AoptSymbols() { }
    virtual ~AoptSymbols() { }

    status_t addSymbol(const String8& name, int32_t value, const SourcePos& pos) {
        if (!check_valid_symbol_name(name, pos, "symbol")) {
            return BAD_VALUE;
        }
        AoptSymbolEntry& sym = edit_symbol(name, &pos);
        sym.typeCode = AoptSymbolEntry::TYPE_INT32;
        sym.int32Val = value;
        return NO_ERROR;
    }

    status_t addStringSymbol(const String8& name, const String8& value,
            const SourcePos& pos) {
        if (!check_valid_symbol_name(name, pos, "symbol")) {
            return BAD_VALUE;
        }
        AoptSymbolEntry& sym = edit_symbol(name, &pos);
        sym.typeCode = AoptSymbolEntry::TYPE_STRING;
        sym.stringVal = value;
        return NO_ERROR;
    }

    status_t makeSymbolPublic(const String8& name, const SourcePos& pos) {
        if (!check_valid_symbol_name(name, pos, "symbol")) {
            return BAD_VALUE;
        }
        AoptSymbolEntry& sym = edit_symbol(name, &pos);
        sym.isPublic = true;
        return NO_ERROR;
    }

    status_t makeSymbolJavaSymbol(const String8& name, const SourcePos& pos) {
        if (!check_valid_symbol_name(name, pos, "symbol")) {
            return BAD_VALUE;
        }
        AoptSymbolEntry& sym = edit_symbol(name, &pos);
        sym.isJavaSymbol = true;
        return NO_ERROR;
    }

    void appendComment(const String8& name, const String16& comment, const SourcePos& pos) {
        if (comment.size() <= 0) {
            return;
        }
        AoptSymbolEntry& sym = edit_symbol(name, &pos);
        if (sym.comment.size() == 0) {
            sym.comment = comment;
        } else {
            sym.comment.append(String16("\n"));
            sym.comment.append(comment);
        }
    }

    void appendTypeComment(const String8& name, const String16& comment) {
        if (comment.size() <= 0) {
            return;
        }
        AoptSymbolEntry& sym = edit_symbol(name, NULL);
        if (sym.typeComment.size() == 0) {
            sym.typeComment = comment;
        } else {
            sym.typeComment.append(String16("\n"));
            sym.typeComment.append(comment);
        }
    }
    
    sp<AoptSymbols> addNestedSymbol(const String8& name, const SourcePos& pos) {
        if (!check_valid_symbol_name(name, pos, "nested symbol")) {
            return NULL;
        }
        
        sp<AoptSymbols> sym = mNestedSymbols.valueFor(name);
        if (sym == NULL) {
            sym = new AoptSymbols();
            mNestedSymbols.add(name, sym);
        }

        return sym;
    }

    status_t applyJavaSymbols(const sp<AoptSymbols>& javaSymbols);

    const KeyedVector<String8, AoptSymbolEntry>& getSymbols() const
        { return mSymbols; }
    const DefaultKeyedVector<String8, sp<AoptSymbols> >& getNestedSymbols() const
        { return mNestedSymbols; }

    const String16& getComment(const String8& name) const
        { return get_symbol(name).comment; }
    const String16& getTypeComment(const String8& name) const
        { return get_symbol(name).typeComment; }

private:
    bool check_valid_symbol_name(const String8& symbol, const SourcePos& pos, const char* label) {
        if (valid_symbol_name(symbol)) {
            return true;
        }
        pos.error("invalid %s: '%s'\n", label, symbol.string());
        return false;
    }
    AoptSymbolEntry& edit_symbol(const String8& symbol, const SourcePos* pos) {
        ssize_t i = mSymbols.indexOfKey(symbol);
        if (i < 0) {
            i = mSymbols.add(symbol, AoptSymbolEntry(symbol));
        }
        AoptSymbolEntry& sym = mSymbols.editValueAt(i);
        if (pos != NULL && sym.sourcePos.line < 0) {
            sym.sourcePos = *pos;
        }
        return sym;
    }
    const AoptSymbolEntry& get_symbol(const String8& symbol) const {
        ssize_t i = mSymbols.indexOfKey(symbol);
        if (i >= 0) {
            return mSymbols.valueAt(i);
        }
        return mDefSymbol;
    }

    KeyedVector<String8, AoptSymbolEntry>           mSymbols;
    DefaultKeyedVector<String8, sp<AoptSymbols> >   mNestedSymbols;
    AoptSymbolEntry                                 mDefSymbol;
};

class ResourceTypeSet : public RefBase,
                        public KeyedVector<String8,sp<AoptGroup> >
{
public:
    ResourceTypeSet();
};

// Storage for lists of fully qualified paths for
// resources encountered during slurping.
class FilePathStore : public RefBase,
                      public Vector<String8>
{
public:
    FilePathStore();
};

/**
 * Asset hierarchy being operated on.
 */
class AoptAssets : public AoptDir
{
public:
    AoptAssets();
    virtual ~AoptAssets() { delete mRes; }

    const String8& getPackage() const { return mPackage; }
    void setPackage(const String8& package) {
        mPackage = package;
        mSymbolsPrivatePackage = package;
        mHavePrivateSymbols = false;
    }

    const SortedVector<AoptGroupEntry>& getGroupEntries() const;

    virtual status_t addFile(const String8& name, const sp<AoptGroup>& file);

    sp<AoptFile> addFile(const String8& filePath,
                         const AoptGroupEntry& entry,
                         const String8& srcDir,
                         sp<AoptGroup>* outGroup,
                         const String8& resType);

    void addResource(const String8& leafName,
                     const String8& path,
                     const sp<AoptFile>& file,
                     const String8& resType);

    void addGroupEntry(const AoptGroupEntry& entry) { mGroupEntries.add(entry); }
    
    ssize_t slurpFromArgs(Bundle* bundle);

    sp<AoptSymbols> getSymbolsFor(const String8& name);

    sp<AoptSymbols> getJavaSymbolsFor(const String8& name);

    status_t applyJavaSymbols();

    const DefaultKeyedVector<String8, sp<AoptSymbols> >& getSymbols() const { return mSymbols; }

    String8 getSymbolsPrivatePackage() const { return mSymbolsPrivatePackage; }
    void setSymbolsPrivatePackage(const String8& pkg) {
        mSymbolsPrivatePackage = pkg;
        mHavePrivateSymbols = mSymbolsPrivatePackage != mPackage;
    }

    bool havePrivateSymbols() const { return mHavePrivateSymbols; }

    bool isJavaSymbol(const AoptSymbolEntry& sym, bool includePrivate) const;

    status_t buildIncludedResources(Bundle* bundle);
    status_t addIncludedResources(const sp<AoptFile>& file);
    const ResTable& getIncludedResources() const;
    AssetManager& getAssetManager();

    void print(const String8& prefix) const;

    inline const Vector<sp<AoptDir> >& resDirs() const { return mResDirs; }
    sp<AoptDir> resDir(const String8& name) const;

    inline sp<AoptAssets> getOverlay() { return mOverlay; }
    inline void setOverlay(sp<AoptAssets>& overlay) { mOverlay = overlay; }
    
    inline KeyedVector<String8, sp<ResourceTypeSet> >* getResources() { return mRes; }
    inline void 
        setResources(KeyedVector<String8, sp<ResourceTypeSet> >* res) { delete mRes; mRes = res; }

    inline sp<FilePathStore>& getFullResPaths() { return mFullResPaths; }
    inline void
        setFullResPaths(sp<FilePathStore>& res) { mFullResPaths = res; }

    inline sp<FilePathStore>& getFullAssetPaths() { return mFullAssetPaths; }
    inline void
        setFullAssetPaths(sp<FilePathStore>& res) { mFullAssetPaths = res; }

private:
    virtual ssize_t slurpFullTree(Bundle* bundle,
                                  const String8& srcDir,
                                  const AoptGroupEntry& kind,
                                  const String8& resType,
                                  sp<FilePathStore>& fullResPaths,
                                  const bool overwrite=false);

    ssize_t slurpResourceTree(Bundle* bundle, const String8& srcDir);
    ssize_t slurpResourceZip(Bundle* bundle, const char* filename);

    status_t filter(Bundle* bundle);

    String8 mPackage;
    SortedVector<AoptGroupEntry> mGroupEntries;
    DefaultKeyedVector<String8, sp<AoptSymbols> > mSymbols;
    DefaultKeyedVector<String8, sp<AoptSymbols> > mJavaSymbols;
    String8 mSymbolsPrivatePackage;
    bool mHavePrivateSymbols;

    Vector<sp<AoptDir> > mResDirs;

    bool mChanged;

    bool mHaveIncludedAssets;
    AssetManager mIncludedAssets;

    sp<AoptAssets> mOverlay;
    KeyedVector<String8, sp<ResourceTypeSet> >* mRes;

    sp<FilePathStore> mFullResPaths;
    sp<FilePathStore> mFullAssetPaths;
};

#endif // __AOPT_ASSETS_H

