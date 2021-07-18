/*
 * Copyright (c) 2018-2020, Andreas Kling <kling@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <LibCore/ArgsParser.h>
#include <LibCore/File.h>
#include <stdio.h>
#include <unistd.h>

int main(int argc, char** argv)
{
    if (pledge("stdio rpath cpath fattr", nullptr)) {
        perror("pledge");
        return 1;
    }

    Vector<const char*> paths;

    Core::ArgsParser args_parser;
    args_parser.set_general_help("Create a file, or update its mtime (time of last modification).");
    args_parser.add_positional_argument(paths, "Files to touch", "path", Core::ArgsParser::Required::Yes);
    args_parser.parse(argc, argv);

    for (auto path : paths) {
        if (!Core::File::touch(path))
            warnln("cannot touch {}", path);
    }
    return 0;
}
