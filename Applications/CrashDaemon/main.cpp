/*
 * Copyright (c) 2020, Itamar S. <itamar8910@gmail.com>
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

#include "CoreDumpReader.h"
#include <AK/Assertions.h>
#include <AK/LogStream.h>
#include <AK/ScopeGuard.h>
#include <LibCore/DirectoryWatcher.h>
#include <fcntl.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>

int main()
{
    static constexpr const char* coredumps_dir = "/tmp/coredump";
    Core::DirectoryWatcher watcher { LexicalPath(coredumps_dir) };
    while (true) {
        auto event = watcher.wait_for_event();
        ASSERT(event.has_value());
        if (event.value().type != Core::DirectoryWatcher::Event::Type::ChildAdded)
            continue;
        auto coredump_path = event.value().child_path;
        dbgln("New coredump: {}", coredump_path.string());
        // nanosleep(100 * 1000000);
        // TODO: make file non readable at the beginning, and wait for it here to become readable
        sleep(1);
        // TODO: Do something with this coredump!
        auto reader = CoreDumpReader::create(coredump_path);
        reader->for_each_memory_region_info([](const ELF::Core::MemoryRegionInfo* region_info) {
            dbgln("{:p}: {}", (const char*)region_info->region_name, (const char*)region_info->region_name);
            return IterationDecision::Continue;
        });
    }
}
