// -*- mode:C++; tab-width:8; c-basic-offset:2; indent-tabs-mode:t -*-
// vim: ts=8 sw=2 smarttab

#include <iostream>
#include "Types.h"
#include "common/ceph_context.h"
#include "include/Context.h"

#define dout_subsys ceph_subsys_rbd
#undef dout_prefix
#define dout_prefix *_dout << "librbd::cache::rwl::Types: " << this << " " \
                           <<  __func__ << ": "

namespace librbd {

namespace cache {

namespace rwl {

DeferredContexts::~DeferredContexts() {
  finish_contexts(nullptr, contexts, 0);
}

void DeferredContexts::add(Context* ctx) {
  contexts.push_back(ctx);
}

/*
 * A BlockExtent identifies a range by first and last.
 *
 * An Extent ("image extent") identifies a range by start and length.
 *
 * The ImageCache interface is defined in terms of image extents, and
 * requires no alignment of the beginning or end of the extent. We
 * convert between image and block extents here using a "block size"
 * of 1.
 */
const BlockExtent block_extent(const uint64_t offset_bytes, const uint64_t length_bytes)
{
  return BlockExtent(offset_bytes,
                     offset_bytes + length_bytes - 1);
}

const BlockExtent WriteLogPmemEntry::block_extent() {
  return BlockExtent(librbd::cache::rwl::block_extent(image_offset_bytes, write_bytes));
}

bool WriteLogPmemEntry::is_sync_point() {
  return sync_point;
}

bool WriteLogPmemEntry::is_discard() {
  return discard;
}

bool WriteLogPmemEntry::is_writesame() {
  return writesame;
}

bool WriteLogPmemEntry::is_write() {
  /* Log entry is a basic write */
  return !is_sync_point() && !is_discard() && !is_writesame();
}

bool WriteLogPmemEntry::is_writer() {
  /* Log entry is any type that writes data */
  return is_write() || is_discard() || is_writesame();
}

const uint64_t WriteLogPmemEntry::get_offset_bytes() {
  return image_offset_bytes;
}

const uint64_t WriteLogPmemEntry::get_write_bytes() {
  return write_bytes;
}

std::ostream& operator<<(std::ostream& os,
                         const WriteLogPmemEntry &entry) {
  os << "entry_valid=" << (bool)entry.entry_valid << ", "
     << "sync_point=" << (bool)entry.sync_point << ", "
     << "sequenced=" << (bool)entry.sequenced << ", "
     << "has_data=" << (bool)entry.has_data << ", "
     << "discard=" << (bool)entry.discard << ", "
     << "writesame=" << (bool)entry.writesame << ", "
     << "sync_gen_number=" << entry.sync_gen_number << ", "
     << "write_sequence_number=" << entry.write_sequence_number << ", "
     << "image_offset_bytes=" << entry.image_offset_bytes << ", "
     << "write_bytes=" << entry.write_bytes << ", "
     << "ws_datalen=" << entry.ws_datalen << ", "
     << "entry_index=" << entry.entry_index;
  return os;
};

template <typename ExtentsType>
ExtentsSummary<ExtentsType>::ExtentsSummary(const ExtentsType &extents) {
  total_bytes = 0;
  first_image_byte = 0;
  last_image_byte = 0;
  if (extents.empty()) return;
  /* These extents refer to image offsets between first_image_byte
   * and last_image_byte, inclusive, but we don't guarantee here
   * that they address all of those bytes. There may be gaps. */
  first_image_byte = extents.front().first;
  last_image_byte = first_image_byte + extents.front().second;
  for (auto &extent : extents) {
    /* Ignore zero length extents */
    if (extent.second) {
      total_bytes += extent.second;
      if (extent.first < first_image_byte) {
        first_image_byte = extent.first;
      }
      if ((extent.first + extent.second) > last_image_byte) {
        last_image_byte = extent.first + extent.second;
      }
    }
  }
}

template <typename T>
std::ostream &operator<<(std::ostream &os,
                                const ExtentsSummary<T> &s) {
  os << "total_bytes=" << s.total_bytes << ", "
     << "first_image_byte=" << s.first_image_byte << ", "
     << "last_image_byte=" << s.last_image_byte << "";
  return os;
};

} // namespace rwl
} // namespace cache
} // namespace librbd
