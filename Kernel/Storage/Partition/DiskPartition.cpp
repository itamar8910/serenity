/*
 * Copyright (c) 2018-2020, Andreas Kling <kling@serenityos.org>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <Kernel/FileSystem/FileDescription.h>
#include <Kernel/Storage/Partition/DiskPartition.h>

// #define OFFD_DEBUG

namespace Kernel {

NonnullRefPtr<DiskPartition> DiskPartition::create(BlockDevice& device, unsigned minor_number, DiskPartitionMetadata metadata)
{
    return adopt(*new DiskPartition(device, minor_number, metadata));
}

DiskPartition::DiskPartition(BlockDevice& device, unsigned minor_number, DiskPartitionMetadata metadata)
    : BlockDevice(100, minor_number, device.block_size())
    , m_device(device)
    , m_metadata(metadata)
{
}

DiskPartition::~DiskPartition()
{
}

const DiskPartitionMetadata& DiskPartition::metadata() const
{
    return m_metadata;
}

void DiskPartition::start_request(AsyncBlockDeviceRequest& request)
{
    request.add_sub_request(m_device->make_request<AsyncBlockDeviceRequest>(request.request_type(),
        request.block_index() + m_metadata.start_block(), request.block_count(), request.buffer(), request.buffer_size()));
}

KResultOr<size_t> DiskPartition::read(FileDescription& fd, size_t offset, UserOrKernelBuffer& outbuf, size_t len)
{
    unsigned adjust = m_metadata.start_block() * block_size();

#ifdef OFFD_DEBUG
    klog() << "DiskPartition::read offset=" << fd.offset() << " adjust=" << adjust << " len=" << len;
#endif

    return m_device->read(fd, offset + adjust, outbuf, len);
}

bool DiskPartition::can_read(const FileDescription& fd, size_t offset) const
{
    unsigned adjust = m_metadata.start_block() * block_size();

#ifdef OFFD_DEBUG
    klog() << "DiskPartition::can_read offset=" << offset << " adjust=" << adjust;
#endif

    return m_device->can_read(fd, offset + adjust);
}

KResultOr<size_t> DiskPartition::write(FileDescription& fd, size_t offset, const UserOrKernelBuffer& inbuf, size_t len)
{
    unsigned adjust = m_metadata.start_block() * block_size();

#ifdef OFFD_DEBUG
    klog() << "DiskPartition::write offset=" << offset << " adjust=" << adjust << " len=" << len;
#endif

    return m_device->write(fd, offset + adjust, inbuf, len);
}

bool DiskPartition::can_write(const FileDescription& fd, size_t offset) const
{
    unsigned adjust = m_metadata.start_block() * block_size();

#ifdef OFFD_DEBUG
    klog() << "DiskPartition::can_write offset=" << offset << " adjust=" << adjust;
#endif

    return m_device->can_write(fd, offset + adjust);
}

const char* DiskPartition::class_name() const
{
    return "DiskPartition";
}

}
