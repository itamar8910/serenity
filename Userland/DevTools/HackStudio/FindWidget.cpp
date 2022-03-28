/*
 * Copyright (c) 2022, Itamar S. <itamar8910@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "FindWidget.h"
#include "Editor.h"
#include <AK/QuickSort.h>
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

    m_input_field->on_change = [this]() {
        reset_results();
        find_next(Direction::Forward);
    };

    m_input_field->on_return_pressed = [this]() {
        find_next(Direction::Forward);
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
    reset_results_and_update_ui();
}

void FindWidget::find_next(Direction direction)
{
    auto needle = m_input_field->text();
    if (needle.is_empty()) {
        reset_results_and_update_ui();
        return;
    }
    GUI::TextRange range {};
    if (direction == Direction::Forward)
        range = m_editor->document().find_next(needle,
            m_current_result.has_value() ? m_current_result->end() : GUI::TextPosition {},
            GUI::TextDocument::SearchShouldWrap::Yes, false, true);
    else
        range = m_editor->document().find_previous(needle,
            m_current_result.has_value() ? m_current_result->start() : GUI::TextPosition {},
            GUI::TextDocument::SearchShouldWrap::Yes, false, true);

    if (!range.is_valid()) {
        reset_results_and_update_ui();
        return;
    }

    auto all_results = m_editor->document().find_all(needle, false, true);
    on_find_results(range, all_results);
}

GUI::TextDocumentSpan* FindWidget::span_at(GUI::TextPosition position)
{
    return m_editor->document().span_at({ position.line(), position.column() + 1 });
}

void FindWidget::on_find_results(GUI::TextRange current, Vector<GUI::TextRange> all_results)
{
    reset_results();
    m_editor->set_cursor(current.start());
    m_current_result = current;
    m_current_results = move(all_results);

    m_editor->force_rehighlight();
    m_editor->update();
}

void FindWidget::reset_results()
{
    m_current_result = {};
    m_current_results.clear();
}

Vector<GUI::TextDocumentSpan> FindWidget::update_spans(Vector<GUI::TextDocumentSpan> spans) const
{
    dbgln("spans:");
    for (auto& span : spans) {
        dbgln("{} ({})", span.range, m_editor->document().text_in_range(span.range));
    }

    //    auto inclusive_range = span.range;
    //    inclusive_range.set_end({span.range.end().line(), span.range.end().column()  == 0 ? 0 : span.range.end().column() - 1});

    for (auto& result : m_current_results) {
        GUI::TextDocumentSpan span;
        span.range = result;
        span.attributes.background_color = palette().hover_highlight();
        if (result == m_current_result)
            span.attributes.underline = true;
        spans.append(span);
    }

    auto is_find_result = [this](auto& span) -> bool {
        return span.attributes.background_color.has_value() && span.attributes.background_color.value() == palette().hover_highlight();
    };

    quick_sort(spans, [&](auto& a, auto& b) {
        if (a.range.start() == b.range.start()) {
            // We want the regular span to come first in this case because
            // we need to copy its color to the "find result" span that will follow it.
            return !is_find_result(a);
        }
        return a.range.start() < b.range.start();
    });

    auto adjust_end = [](auto span) -> GUI::TextDocumentSpan {
        span.range.set_end({ span.range.end().line(), span.range.end().column() == 0 ? 0 : span.range.end().column() - 1 });
        return span;
    };


    Vector<GUI::TextDocumentSpan> new_spans;

    dbgln("all spans:");
    for (auto& span : spans) {
        dbgln("{} ({}): {}", span.range, m_editor->document().text_in_range(span.range), span.attributes.background_color == palette().hover_highlight());
        if (new_spans.is_empty()) {
            new_spans.append(span);
            continue;
        }
        auto last_span = new_spans.last();
        if (adjust_end(span).range.start() > adjust_end(last_span).range.end()) {
            new_spans.append(span);
            continue;
        }
       if (is_find_result(span)) {
           new_spans.take_last();
           auto last_span_copy = last_span;
           last_span.range.set_end(span.range.start());
           span.attributes.color = last_span.attributes.color;
           span.attributes.bold = last_span.attributes.bold;
           new_spans.append(last_span);
           new_spans.append(span);
           if (adjust_end(span).range.end() < adjust_end(last_span_copy).range.end()) {
               last_span_copy.range.set_start(span.range.end());
               new_spans.append(last_span_copy);
           }
           continue;
       }
       if (adjust_end(span).range.end() < adjust_end(last_span).range.end())
           continue;
       span.range.set_start(last_span.range.end());
       new_spans.append(span);
    }

    return new_spans;
}

void FindWidget::reset_results_and_update_ui()
{
    reset_results();
    m_editor->force_rehighlight();
    m_editor->update();
}

}
