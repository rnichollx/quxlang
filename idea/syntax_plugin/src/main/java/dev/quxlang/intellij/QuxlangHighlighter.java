package dev.quxlang.intellij;

import com.intellij.lexer.Lexer;
import com.intellij.openapi.editor.DefaultLanguageHighlighterColors;
import com.intellij.openapi.editor.HighlighterColors;
import com.intellij.openapi.editor.colors.TextAttributesKey;
import com.intellij.openapi.fileTypes.SyntaxHighlighterBase;
import com.intellij.psi.TokenType;
import com.intellij.psi.tree.IElementType;
import java.util.Collections;
import java.util.LinkedHashMap;
import java.util.Map;

/** Maps token categories to configurable, theme-aware editor colors. */
public final class QuxlangHighlighter extends SyntaxHighlighterBase {
    static final Map<IElementType, TextAttributesKey> COLORS;
    static {
        Map<IElementType, TextAttributesKey> colors = new LinkedHashMap<>();
        colors.put(QuxlangToken.KEYWORD, TextAttributesKey.createTextAttributesKey("QUXLANG_KEYWORD", DefaultLanguageHighlighterColors.KEYWORD));
        colors.put(QuxlangToken.TYPE, TextAttributesKey.createTextAttributesKey("QUXLANG_TYPE", DefaultLanguageHighlighterColors.CLASS_NAME));
        colors.put(QuxlangToken.IDENTIFIER, TextAttributesKey.createTextAttributesKey("QUXLANG_IDENTIFIER", DefaultLanguageHighlighterColors.IDENTIFIER));
        colors.put(QuxlangToken.MEMBER, TextAttributesKey.createTextAttributesKey("QUXLANG_MEMBER", DefaultLanguageHighlighterColors.INSTANCE_FIELD));
        colors.put(QuxlangToken.ARGUMENT, TextAttributesKey.createTextAttributesKey("QUXLANG_ARGUMENT", DefaultLanguageHighlighterColors.PARAMETER));
        colors.put(QuxlangToken.LOCAL, TextAttributesKey.createTextAttributesKey("QUXLANG_LOCAL", DefaultLanguageHighlighterColors.LOCAL_VARIABLE));
        colors.put(QuxlangToken.GLOBAL, TextAttributesKey.createTextAttributesKey("QUXLANG_GLOBAL", DefaultLanguageHighlighterColors.GLOBAL_VARIABLE));
        colors.put(QuxlangToken.FREE_FUNCTION, TextAttributesKey.createTextAttributesKey("QUXLANG_FREE_FUNCTION", DefaultLanguageHighlighterColors.FUNCTION_DECLARATION));
        colors.put(QuxlangToken.MEMBER_FUNCTION, TextAttributesKey.createTextAttributesKey("QUXLANG_MEMBER_FUNCTION", DefaultLanguageHighlighterColors.INSTANCE_METHOD));
        colors.put(QuxlangToken.NAMESPACE, TextAttributesKey.createTextAttributesKey("QUXLANG_NAMESPACE", DefaultLanguageHighlighterColors.CLASS_NAME));
        colors.put(QuxlangToken.TEMPLATE_PARAMETER, TextAttributesKey.createTextAttributesKey("QUXLANG_TEMPLATE_PARAMETER", DefaultLanguageHighlighterColors.PARAMETER));
        colors.put(QuxlangToken.CALL, TextAttributesKey.createTextAttributesKey("QUXLANG_CALL", DefaultLanguageHighlighterColors.FUNCTION_CALL));
        colors.put(QuxlangToken.NUMBER, TextAttributesKey.createTextAttributesKey("QUXLANG_NUMBER", DefaultLanguageHighlighterColors.NUMBER));
        colors.put(QuxlangToken.STRING, TextAttributesKey.createTextAttributesKey("QUXLANG_STRING", DefaultLanguageHighlighterColors.STRING));
        colors.put(QuxlangToken.COMMENT, TextAttributesKey.createTextAttributesKey("QUXLANG_COMMENT", DefaultLanguageHighlighterColors.LINE_COMMENT));
        colors.put(QuxlangToken.DOC, TextAttributesKey.createTextAttributesKey("QUXLANG_DOC", DefaultLanguageHighlighterColors.DOC_COMMENT));
        colors.put(QuxlangToken.OPERATOR, TextAttributesKey.createTextAttributesKey("QUXLANG_OPERATOR", DefaultLanguageHighlighterColors.OPERATION_SIGN));
        colors.put(QuxlangToken.BRACES, TextAttributesKey.createTextAttributesKey("QUXLANG_BRACES", DefaultLanguageHighlighterColors.BRACES));
        colors.put(QuxlangToken.PARENTHESES, TextAttributesKey.createTextAttributesKey("QUXLANG_PARENTHESES", DefaultLanguageHighlighterColors.PARENTHESES));
        colors.put(QuxlangToken.BRACKETS, TextAttributesKey.createTextAttributesKey("QUXLANG_BRACKETS", DefaultLanguageHighlighterColors.BRACKETS));
        colors.put(QuxlangToken.COMMA, TextAttributesKey.createTextAttributesKey("QUXLANG_COMMA", DefaultLanguageHighlighterColors.COMMA));
        colors.put(QuxlangToken.SEMICOLON, TextAttributesKey.createTextAttributesKey("QUXLANG_SEMICOLON", DefaultLanguageHighlighterColors.SEMICOLON));
        colors.put(TokenType.BAD_CHARACTER, TextAttributesKey.createTextAttributesKey("QUXLANG_BAD_CHARACTER", HighlighterColors.BAD_CHARACTER));
        COLORS = Collections.unmodifiableMap(colors);
    }

    @Override public Lexer getHighlightingLexer() { return new QuxlangLexer(); }

    @Override
    public TextAttributesKey[] getTokenHighlights(IElementType type) {
        return pack(COLORS.get(type));
    }
}
