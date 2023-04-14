/*
 * Copyright (c) 2020, Andreas Kling <kling@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "Region.h"
#include "Emulator.h"

namespace UserspaceEmulator {

Region::Region(FlatPtr base, FlatPtr size, bool mmap)
    : m_emulator(Emulator::the())
    , m_range(Range { VirtualAddress { base }, size })
    , m_mmap(mmap)
{
}

}
