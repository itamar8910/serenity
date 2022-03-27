/*
 * Copyright (c) 2022, Itamar S. <itamar8910@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "FindWindow.h"
#include "Editor.h"
#include <LibGUI/BoxLayout.h>

namespace HackStudio {

FindWindow::FindWindow(EditorWrapper& wrapper, NonnullRefPtr<Editor> editor)
    : GUI::Window((Core::Object*)&wrapper)
    , m_editor(editor)
{
    set_window_type(GUI::WindowType::Tooltip);
    set_rect(0, 0, 300, 50);
    m_main_widget = set_main_widget<GUI::Widget>();
    m_main_widget->set_fill_with_background_color(true);
    auto& layout = m_main_widget->set_layout<GUI::HorizontalBoxLayout>();
    layout.set_margins(4);
    m_input_field = m_main_widget->add<GUI::TextBox>();
    m_input_field->set_enabled(true);
    m_next = m_main_widget->add<GUI::Button>();
    m_previous = m_main_widget->add<GUI::Button>();
    m_next->set_icon_from_path("/res/icons/16x16/go-up.png");
    m_previous->set_icon_from_path("/res/icons/16x16/go-down.png");
    m_next->set_max_width(15);
    m_previous->set_max_width(15);
}

void FindWindow::show()
{
    dbgln("FindWindow::show()");
    move_to(m_editor->screen_relative_rect().top_left().translated(m_editor->screen_relative_rect().width() - width() - m_editor->width_occupied_by_vertical_scrollbar() - 5, 5));
    Window::show();
    Window::move_to_front();
    m_input_field->set_focus(true);
    dbgln("focused? {}", m_input_field->is_focused());
    dbgln("is_active: {}, active_input:{}", Window::is_active(), Window::is_active_input());
}

}
