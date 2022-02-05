/*
* Copyright (c) 2022, Itamar S. <itamar8910@gmail.com>
* Copyright (c) 2022, the SerenityOS developers.
*
* SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include "SyntaxHighlighter.h"
#include "Token.h"
#include <LibSyntax/Highlighter.h>
#include <LibGUI/AutocompleteProvider.h>
#include <LibThreading/Mutex.h>

namespace Cpp {


// TODO: Add another syntax highlighter named SemanticSyntaxHighlighter (no need for a baseclass rn)

class SemanticSyntaxHighlighter final : public Syntax::Highlighter {

public:
    SemanticSyntaxHighlighter() { }
    virtual ~SemanticSyntaxHighlighter() override = default;

    virtual bool is_identifier(u64 token) const override;
    virtual bool is_navigatable(u64 token) const override;

    virtual Syntax::Language language() const override { return Syntax::Language::Cpp; }
    virtual void rehighlight(Palette const&) override;

    void update_tokens_info(Vector<GUI::AutocompleteProvider::TokenInfo>);

    virtual bool is_cpp_semantic_highlighter() const override { return true; }

    enum class EditType {
        Insert,
        Remove
    };
    void set_last_edited_range(GUI::TextRange, EditType);


protected:
    virtual Vector<MatchingTokenPair> matching_token_pairs_impl() const override { return m_simple_syntax_highlighter.matching_token_pairs_impl(); };
    virtual bool token_types_equal(u64 token1, u64 token2) const override { return m_simple_syntax_highlighter.token_types_equal(token1, token2); };

private:
    void update_spans(Vector<GUI::AutocompleteProvider::TokenInfo> const&);

    Cpp::SyntaxHighlighter m_simple_syntax_highlighter;
    Vector<GUI::AutocompleteProvider::TokenInfo> m_tokens_info;
    String m_saved_tokens_text;
    Vector<Token> m_saved_tokens;
    Threading::Mutex m_lock;
    Optional<Gfx::Palette> m_current_palette;
    Optional<GUI::TextRange> m_last_edited_range;
    EditType m_last_edit_type {EditType::Insert};


};
}
template<>
inline bool Syntax::Highlighter::fast_is<Cpp::SemanticSyntaxHighlighter>() const { return is_cpp_semantic_highlighter(); }
