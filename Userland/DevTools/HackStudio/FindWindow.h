/*
 * Copyright (c) 2022, Itamar S. <itamar8910@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include "Editor.h"
#include <LibGUI/Button.h>
#include <LibGUI/TextBox.h>
#include <LibGUI/Widget.h>
#include <LibGUI/Window.h>

namespace HackStudio {

class Editor;
class EditorWrapper;

class FindWindow : public GUI::Window {

public:
    static NonnullRefPtr<FindWindow> create(EditorWrapper& wrapper, NonnullRefPtr<Editor> editor)
    {
        return adopt_ref(*new FindWindow(wrapper, move(editor)));
    }
    ~FindWindow() = default;

    void show();

private:
    FindWindow(EditorWrapper&, NonnullRefPtr<Editor>);

    NonnullRefPtr<Editor> m_editor;
    RefPtr<GUI::Widget> m_main_widget;
    RefPtr<GUI::TextBox> m_input_field;
    RefPtr<GUI::Button> m_next;
    RefPtr<GUI::Button> m_previous;
};

}
