#include <stdexcept>

#include "mvs/parser.h"
#include "mvs/lexer.h"

namespace mvs{
    bool Parser::_at_end() const {
        return idx_ >= tokens_.size();
    }

    const Token& Parser::_peek() const{
        if(_at_end()){
            throw std::out_of_range("no more token idx: " + std::to_string(idx_));
        }
        return tokens_[idx_];
    }

    void Parser::_advance() const {
        if(!_at_end()){
            idx_++;
        }
    }

    void Parser::_skip_end_tokens() const {
        while(!_at_end && _peek().type == TokenKind::END) _advance();
    }
} // namespace mvs