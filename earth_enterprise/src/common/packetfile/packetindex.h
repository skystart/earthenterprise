/*
 * Copyright 2017 Google Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


#ifndef COMMON_PACKETFILE_PACKINDEX_H__
#define COMMON_PACKETFILE_PACKINDEX_H__

// PacketIndex - classes for the packet file index.  Used by
// PacketFileWriter and PacketFileReader.

#include <quadtreepath.h>
#include <geFilePool.h>
#include <khThread.h>

class LittleEndianReadBuffer;
class EndianReadBuffer;
class EndianWriteBuffer;

// PacketIndexEntry - define entry for each record in index to packet file

class PacketIndexEntry {
 public:
  // Size of buffer to store in disk format (including CRC32)
  static const size_t kStoreSize = QuadtreePath::kStoreSize + sizeof(uint64)
      + sizeof(uint32) + sizeof(uint32) + sizeof(uint32);

  // Constructors - the first constructor expects a little-endian
  // buffer in the format created by Push. Throws exception if bad data.
  PacketIndexEntry(EndianReadBuffer *buffer) {
    Pull(*buffer);
  }
  PacketIndexEntry(const QuadtreePath &qt_path,
                   const uint64 position,
                   const uint32 record_size,
                   const uint32 extra = 0)
      : qt_path_(qt_path),
        position_(position),
        record_size_(record_size),
        extra_(extra) {
  }
  PacketIndexEntry()
      : qt_path_(),
        position_(0),
        record_size_(0),
        extra_(0) {
  }

  void Push(EndianWriteBuffer& buf) const;
  void Pull(EndianReadBuffer& buf);

  bool operator>(const PacketIndexEntry &other) const {
    return qt_path_ > other.qt_path_;
  }
  bool operator<(const PacketIndexEntry &other) const {
    return qt_path_ < other.qt_path_;
  }
  bool operator==(const PacketIndexEntry &other) const {
    return qt_path_ == other.qt_path_;
  }
  bool operator!=(const PacketIndexEntry &other) const {
    return qt_path_ != other.qt_path_;
  }

  QuadtreePath qt_path() const { return qt_path_; }
  void set_qt_path(QuadtreePath new_path) { qt_path_ = new_path; }
  uint64 position() const { return position_; }
  void set_position(uint64 new_pos) { position_ = new_pos; }
  uint32 record_size() const { return record_size_; }
  void set_record_size(uint32 new_size) { record_size_ = new_size; }
  uint32 extra() const { return extra_; }
  void set_extra(uint32 new_extra) { extra_ = new_extra; }
 private:
  QuadtreePath qt_path_;                // path to this record
  uint64 position_;                     // pos of record in file bundle
  uint32 record_size_;                  // size of data (incl. CRC)
  uint32 extra_;                        // extra id (not used by all)
};


// PacketIndexReader - reader for index file.  Allows sequential or
// random reading of index file.
//
// Note:Using ReadNext with multiple threads will read each record
// exactly once, but which thread gets each record will depend
// completely on timing.  Using Seek with multiple threads (except
// during initialization) is generally a Bad Idea.

class PacketIndexReader {
 public:
  PacketIndexReader(geFilePool &file_pool, const std::string &index_path);
  virtual ~PacketIndexReader() {}
  bool data_has_crc() const { return data_has_crc_; }
  const std::string &file_path() const { return file_path_; }
  bool ReadNext(PacketIndexEntry *entry);
  uint32 ReadNextN(PacketIndexEntry *entries, uint32 count,
                   LittleEndianReadBuffer &buffer);
  off64_t Filesize() const { return file_size_; }
  void Seek(off64_t position);
  uint64 NumPackets(void) const;
  static uint64 NumPackets(const std::string &index_path);
 private:
  std::string file_path_;               // path to index
  off64_t index_pos_;                   // position for next read
  khDeleteGuard<geFilePool::Reader> index_reader_;
  off64_t file_size_;

  bool data_has_crc_;                   // true if data has CRC32

  khMutex modify_lock_;

  // This method should only be called while holding the mutex
  bool AtEnd() const { return index_pos_ >= file_size_; }

  DISALLOW_COPY_AND_ASSIGN(PacketIndexReader);
};

#endif  // COMMON_PACKETFILE_PACKINDEX_H__
