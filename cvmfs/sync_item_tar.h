/**
 * This file is part of the CernVM File System
 */

#ifndef CVMFS_SYNC_ITEM_TAR_H_
#define CVMFS_SYNC_ITEM_TAR_H_

#include "sync_union.h"

namespace publish {

// struct archive;

class SyncItemTar : public SyncItem {
 public:
  SyncItemTar(const string &relative_parent_path, const string &filename,
              struct archive *archive, struct archive_entry *entry,
              const SyncUnion *union_engine);

  SyncItemType GetScratchFiletype() const;
  catalog::DirectoryEntryBase CreateBasicCatalogDirent() const;

  inline unsigned int GetRdevMajor() const { return major(tar_stat_.st_rdev); }
  inline unsigned int GetRdevMinor() const { return minor(tar_stat_.st_rdev); }

  IngestionSource *GetIngestionSource() const;

  struct archive *archive_;
  struct archive_entry *archive_entry_;

 private:
  platform_stat64 GetStatFromTar() const;
  mutable platform_stat64 tar_stat_;
  mutable bool obtained_tar_stat_;
};

SyncItemTar::SyncItemTar(const string &relative_parent_path,
                         const string &filename, struct archive *archive,
                         struct archive_entry *entry,
                         const SyncUnion *union_engine)
    : SyncItem(relative_parent_path, filename, union_engine, kItemUnknown),
      archive_(archive),
      archive_entry_(entry) {
  scratch_type_ = GetScratchFiletype();
  GetStatFromTar();
}

SyncItemType SyncItemTar::GetScratchFiletype() const {
  assert(archive_entry_);
  switch (archive_entry_filetype(archive_entry_)) {
    case AE_IFREG: {
      return kItemFile;
      break;
    }
    case AE_IFLNK: {
      return kItemSymlink;
      break;
    }
    case AE_IFSOCK: {
      return kItemSocket;
      break;
    }
    case AE_IFCHR: {
      return kItemCharacterDevice;
      break;
    }
    case AE_IFBLK: {
      return kItemBlockDevice;
      break;
    }
    case AE_IFDIR: {
      return kItemDir;
      break;
    }
    case AE_IFIFO: {
      return kItemFifo;
      break;
    }
    default:
      return kItemUnknown;
      break;
  }
}

platform_stat64 SyncItemTar::GetStatFromTar() const {
  assert(archive_entry_);
  if (obtained_tar_stat_) return tar_stat_;

  const struct stat *entry_stat_ = archive_entry_stat(archive_entry_);
  platform_stat64 stat;

  stat.st_mode = entry_stat_->st_mode;
  stat.st_uid = entry_stat_->st_uid;
  stat.st_gid = entry_stat_->st_gid;
  stat.st_size = entry_stat_->st_size;
  stat.st_mtime = entry_stat_->st_mtime;

  obtained_tar_stat_ = true;
  tar_stat_ = stat;

  return stat;
}

catalog::DirectoryEntryBase SyncItemTar::CreateBasicCatalogDirent() const {
  catalog::DirectoryEntryBase dirent;

  // inode and parent inode is determined at runtime of client
  dirent.inode_ = catalog::DirectoryEntry::kInvalidInode;

  // this might mask the actual link count in case hardlinks are not supported
  // (i.e. on setups using OverlayFS)
  dirent.linkcount_ = this->tar_stat_.st_nlink;

  dirent.mode_ = this->tar_stat_.st_mode;
  dirent.uid_ = this->tar_stat_.st_uid;
  dirent.gid_ = this->tar_stat_.st_gid;
  dirent.size_ = this->tar_stat_.st_size;
  dirent.mtime_ = this->tar_stat_.st_mtime;
  dirent.checksum_ = this->GetContentHash();
  dirent.is_external_file_ = this->IsExternalData();
  dirent.compression_algorithm_ = this->GetCompressionAlgorithm();

  dirent.name_.Assign(this->filename().data(), this->filename().length());

  if (this->IsSymlink()) {
    char slnk[PATH_MAX + 1];
    const ssize_t length =
        readlink((this->GetUnionPath()).c_str(), slnk, PATH_MAX);
    assert(length >= 0);
    dirent.symlink_.Assign(slnk, length);
  }

  if (this->IsCharacterDevice() || this->IsBlockDevice()) {
    dirent.size_ = makedev(GetRdevMajor(), GetRdevMinor());
  }

  return dirent;
}

IngestionSource *SyncItemTar::GetIngestionSource() const {
  return new TarIngestionSource(archive_, archive_entry_);
};
}

#endif  // CVMFS_SYNC_ITEM_TAR_H_
