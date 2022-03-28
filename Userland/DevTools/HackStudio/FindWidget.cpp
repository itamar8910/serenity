/*
 * Copyright (c) 2022, Itamar S. <itamar8910@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "FindWidget.h"
#include "Editor.h"
#include <DevTools/HackStudio/FindWidgetGML.h>
#include <LibGUI/BoxLayout.h>
#include <LibGfx/Palette.h>

namespace HackStudio {

FindWidget::FindWidget(NonnullRefPtr<Editor> editor)
    : m_editor(move(editor))
{
    load_from_gml(find_widget_gml);
    set_fixed_height(widget_height);
    m_input_field = find_descendant_of_type_named<GUI::TextBox>("input_field");
    m_next = find_descendant_of_type_named<GUI::Button>("next");
    m_previous = find_descendant_of_type_named<GUI::Button>("previous");

    VERIFY(m_input_field);
    VERIFY(m_next);
    VERIFY(m_previous);

    m_next->on_click = [this](auto) {
        find_next(Direction::Forward);
    };
    m_previous->on_click = [this](auto) {
        find_next(Direction::Backward);
    };
}

void FindWidget::show()
{
    set_visible(true);
    set_focus(true);
    m_input_field->set_focus(true);
    m_editor->vertical_scrollbar().set_value(m_editor->vertical_scrollbar().value() + widget_height, GUI::AllowCallback::Yes, GUI::Scrollbar::DoClamp::No);
    m_visible = !m_visible;
}

void FindWidget::hide()
{
    set_visible(false);
    set_focus(false);
    m_visible = !m_visible;
    m_editor->vertical_scrollbar().set_value(m_editor->vertical_scrollbar().value() - widget_height, GUI::AllowCallback::Yes, GUI::Scrollbar::DoClamp::No);
    m_editor->set_focus(true);
    reset_results();
}

void FindWidget::find_next(Direction direction)
{
    auto needle = m_input_field->text();
    if (needle.is_empty())
        return;
    GUI::TextRange range {};
    if (direction == Direction::Forward)
        range = m_editor->document().find_next(needle,
            m_previous_result.has_value() ? m_previous_result->end() : GUI::TextPosition {},
            GUI::TextDocument::SearchShouldWrap::Yes, false, true);
    else
        range = m_editor->document().find_previous(needle,
            m_previous_result.has_value() ? m_previous_result->start() : GUI::TextPosition {},
            GUI::TextDocument::SearchShouldWrap::Yes, false, true);

    if (range.is_valid()) {
        auto all_results = m_editor->document().find_all(needle, false, true);
        on_find_results(range, all_results);
    }
}

GUI::TextDocumentSpan* FindWidget::span_at(GUI::TextPosition position)
{
    return m_editor->document().span_at({ position.line(), position.column() + 1 });
}

void FindWidget::on_find_results(GUI::TextRange current, Vector<GUI::TextRange> all_results)
{
    reset_results();
    for (auto& result : all_results) {
        if (auto span = span_at(result.start()); span != nullptr) {
            span->attributes.background_color = palette().hover_highlight();
        }
    }

    auto current_span = span_at(current.start());
    if (current_span) {
        current_span->attributes.underline = true;
        m_editor->set_cursor(current.start());
    }

    m_previous_result = current;
    m_previous_results = move(all_results);

    m_editor->update();
}

void FindWidget::reset_results()
{
    if (m_previous_result.has_value()) {
        if (auto previous_span = span_at(m_previous_result->start()); previous_span != nullptr) {
            previous_span->attributes.underline = false;
            previous_span->attributes.background_color = {};
        }
    }
    for (auto& result : m_previous_results) {
        if (auto span = span_at(result.start()); span != nullptr) {
            span->attributes.underline = false;
            span->attributes.background_color = {};
        }
    }
}

}
