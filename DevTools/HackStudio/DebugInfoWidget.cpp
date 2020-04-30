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

#include "DebugInfoWidget.h"
#include "Debugger.h"
#include <LibGUI/BoxLayout.h>
#include <LibGUI/Model.h>
#include <LibGUI/TableView.h>

String DebugInfoModel::column_name(int column) const
{
    switch (column) {
    case Column::VariableName:
        return "Name";
    case Column::VariableType:
        return "Type";
    case Column::VariableValue:
        return "Value";
    default:
        ASSERT_NOT_REACHED();
    }
}

GUI::Variant DebugInfoModel::data(const GUI::ModelIndex& index, Role role) const
{
    if (role == Role::Display) {
        auto& variable = m_variables.at(index.row());
        switch (index.column()) {
        case Column::VariableName:
            return variable.name;
        case Column::VariableType:
            return variable.type;
        case Column::VariableValue:
            return "N/A";
        }
    }
    return {};
}

static RefPtr<DebugInfoModel> create_model(const PtraceRegisters& regs)
{
    Vector<DebugInfo::VariableInfo> variables;
    auto scope_info = Debugger::the().session()->debug_info().get_scope_info(regs.eip);
    ASSERT(scope_info.has_value());
    for (const auto& var : scope_info.value().variables) {
        variables.append(var);
    }
    return adopt(*new DebugInfoModel(move(variables)));
}

DebugInfoWidget::DebugInfoWidget()
{
    set_layout<GUI::VerticalBoxLayout>();
    m_info_view = add<GUI::TableView>();
    m_info_view->set_size_columns_to_fit_content(true);
    m_info_view->set_model(*(new DebugInfoModel));
}

void DebugInfoWidget::update_variables(const PtraceRegisters& regs)
{
    auto model = create_model(regs);
    m_info_view->set_model(model);
}
