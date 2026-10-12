package dev.quxlang.intellij;

import com.intellij.lexer.LexerBase;
import com.intellij.psi.TokenType;
import com.intellij.psi.tree.IElementType;
import java.util.Set;
import java.util.regex.Pattern;

/**
 * Incremental lexer with bounded lexical context for identifier roles.
 * States describe the context before the current token. Strings and documentation
 * are single tokens, so restarts never require a hidden stack or a symbol table.
 * Uppercase words follow the compiler's keyword lexical class, including builtins.
 */
public final class QuxlangLexer extends LexerBase {
    private static final int NORMAL = 0;
    private static final int MEMBER_NAME = 1;
    private static final int ARGUMENT_NAME = 2;
    private static final int LOCAL_NAME = 3;
    private static final int NAMED_NAME = 4;
    private static final int NAMED_DECLARATION = 5;
    private static final int DOCUMENTATION = 8;
    private static final Set<String> TYPES = Set.of("VOID", "BOOL", "BYTE", "ADDRESS", "SZ");
    private static final Pattern SIZED_TYPE = Pattern.compile("(?:[IU][0-9]+|F[0-9]+(?:E[0-9]+)?)");

    private CharSequence buffer = "";
    private int bufferEnd;
    private int tokenStart;
    private int tokenEnd;
    private int tokenState;
    private int nextState;
    private IElementType tokenType;

    @Override
    public void start(CharSequence text, int startOffset, int endOffset, int initialState) {
        buffer = text;
        bufferEnd = endOffset;
        tokenEnd = startOffset;
        nextState = initialState;
        advance();
    }

    @Override public int getState() { return tokenState; }
    @Override public IElementType getTokenType() { return tokenType; }
    @Override public int getTokenStart() { return tokenStart; }
    @Override public int getTokenEnd() { return tokenEnd; }
    @Override public CharSequence getBufferSequence() { return buffer; }
    @Override public int getBufferEnd() { return bufferEnd; }

    @Override
    public void advance() {
        tokenStart = tokenEnd;
        tokenState = nextState;
        tokenType = null;
        if (tokenStart == bufferEnd) {
            return;
        }
        char first = buffer.charAt(tokenEnd++);
        if (Character.isWhitespace(first)) {
            while (tokenEnd < bufferEnd && Character.isWhitespace(buffer.charAt(tokenEnd))) {
                ++tokenEnd;
            }
            tokenType = TokenType.WHITE_SPACE;
            return;
        }
        if (first == '/' && at(tokenEnd, '/')) {
            while (tokenEnd < bufferEnd && !at(tokenEnd, '\n') && !at(tokenEnd, '\r')) {
                ++tokenEnd;
            }
            tokenType = QuxlangToken.COMMENT;
            return;
        }
        nextState = NORMAL;
        if ((tokenState & DOCUMENTATION) != 0 && first == '<' && at(tokenEnd, '$')) {
            ++tokenEnd;
            while (tokenEnd < bufferEnd && !(at(tokenEnd, '$') && at(tokenEnd + 1, '>'))) {
                ++tokenEnd;
            }
            tokenEnd = Math.min(tokenEnd + 2, bufferEnd);
            tokenType = QuxlangToken.DOC;
            nextState = tokenState & ~DOCUMENTATION;
            return;
        }
        if (first == '"' || first == '\'') {
            scanQuoted(first);
            tokenType = QuxlangToken.STRING;
            return;
        }
        if (isDigit(first)) {
            while (tokenEnd < bufferEnd && isDigit(buffer.charAt(tokenEnd))) {
                ++tokenEnd;
            }
            if (at(tokenEnd, '.') && tokenEnd + 1 < bufferEnd && isDigit(buffer.charAt(tokenEnd + 1))) {
                ++tokenEnd;
                while (tokenEnd < bufferEnd && isDigit(buffer.charAt(tokenEnd))) {
                    ++tokenEnd;
                }
            }
            tokenType = QuxlangToken.NUMBER;
            return;
        }
        if (isLetter(first) || first == '_') {
            while (tokenEnd < bufferEnd) {
                char current = buffer.charAt(tokenEnd);
                if (!isLetter(current) && !isDigit(current) && current != '_') {
                    break;
                }
                ++tokenEnd;
            }
            classifyWord();
            return;
        }
        tokenType = switch (first) {
            case '{', '}' -> QuxlangToken.BRACES;
            case '(', ')' -> QuxlangToken.PARENTHESES;
            case '[', ']' -> QuxlangToken.BRACKETS;
            case ',' -> QuxlangToken.COMMA;
            case ';' -> QuxlangToken.SEMICOLON;
            case '.', '@', ':', '+', '-', '*', '/', '%', '=', '!', '<', '>', '&', '|', '^', '~', '?', '#' -> QuxlangToken.OPERATOR;
            default -> TokenType.BAD_CHARACTER;
        };
        if (first == '.') {
            nextState = MEMBER_NAME;
        } else if (first == '@') {
            nextState = ARGUMENT_NAME;
        } else if (first == ':' && at(tokenEnd, ':')) {
            ++tokenEnd;
            nextState = NAMED_NAME;
        }
    }

    /** Consumes a quoted literal, including escaped delimiters and incomplete input. */
    private void scanQuoted(char quote) {
        while (tokenEnd < bufferEnd) {
            char current = buffer.charAt(tokenEnd++);
            if (current == quote) {
                return;
            }
            if (current == '\\' && tokenEnd < bufferEnd) {
                ++tokenEnd;
            }
        }
    }

    /** Assigns lexical identifier roles without performing declaration resolution. */
    private void classifyWord() {
        String word = buffer.subSequence(tokenStart, tokenEnd).toString();
        if (tokenState == ARGUMENT_NAME) {
            tokenType = QuxlangToken.ARGUMENT;
        } else if (tokenState == MEMBER_NAME) {
            tokenType = QuxlangToken.MEMBER;
            nextState = NAMED_DECLARATION;
        } else if (tokenState == NAMED_NAME) {
            tokenType = QuxlangToken.IDENTIFIER;
            nextState = NAMED_DECLARATION;
        } else if (tokenState == LOCAL_NAME) {
            tokenType = QuxlangToken.LOCAL;
        } else if (TYPES.contains(word) || SIZED_TYPE.matcher(word).matches()) {
            tokenType = QuxlangToken.TYPE;
        } else if (word.chars().noneMatch(c -> c >= 'a' && c <= 'z')) {
            tokenType = QuxlangToken.KEYWORD;
            if (word.equals("DOC")) {
                nextState = DOCUMENTATION | (tokenState == NAMED_DECLARATION ? NAMED_DECLARATION : NORMAL);
            } else if ((word.equals("VAR") || word.equals("STATIC_VAR")) && tokenState != NAMED_DECLARATION) {
                nextState = LOCAL_NAME;
            }
        } else {
            tokenType = QuxlangToken.IDENTIFIER;
        }

    }

    private boolean at(int offset, char expected) {
        return offset < bufferEnd && buffer.charAt(offset) == expected;
    }

    private static boolean isLetter(char character) {
        return character >= 'a' && character <= 'z' || character >= 'A' && character <= 'Z';
    }

    private static boolean isDigit(char character) {
        return character >= '0' && character <= '9';
    }
}
