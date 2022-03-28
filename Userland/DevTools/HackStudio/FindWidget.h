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

class FindWidget final : public GUI::Widget {
    C_OBJECT(FindWidget)
public:
    ~FindWidget() = default;

    void show();
    void hide();
    bool visible() const { return m_visible; }

private:
    FindWidget(NonnullRefPtr<Editor>);

    enum class Direction {
        Forward,
        Backward,
    };
    void find_next(Direction);
    void on_find_results(GUI::TextRange current, Vector<GUI::TextRange> all_results);
    GUI::TextDocumentSpan* span_at(GUI::TextPosition);
    void reset_results();

    static constexpr auto widget_height = 25;

    NonnullRefPtr<Editor> m_editor;
    RefPtr<GUI::TextBox> m_input_field;
    RefPtr<GUI::Button> m_next;
    RefPtr<GUI::Button> m_previous;
    bool m_visible { false };
    Optional<GUI::TextRange> m_previous_result;
    Vector<GUI::TextRange> m_previous_results;
};

}
