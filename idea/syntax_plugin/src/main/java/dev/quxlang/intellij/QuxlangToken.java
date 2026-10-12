package dev.quxlang.intellij;

import com.intellij.psi.tree.IElementType;

/** Token categories used by the lexical highlighter. */
public final class QuxlangToken {
    public static final IElementType KEYWORD = new IElementType("KEYWORD", QuxlangLanguage.INSTANCE);
    public static final IElementType TYPE = new IElementType("TYPE", QuxlangLanguage.INSTANCE);
    public static final IElementType IDENTIFIER = new IElementType("IDENTIFIER", QuxlangLanguage.INSTANCE);
    public static final IElementType MEMBER = new IElementType("MEMBER", QuxlangLanguage.INSTANCE);
    public static final IElementType ARGUMENT = new IElementType("ARGUMENT", QuxlangLanguage.INSTANCE);
    public static final IElementType LOCAL = new IElementType("LOCAL", QuxlangLanguage.INSTANCE);
    public static final IElementType GLOBAL = new IElementType("GLOBAL", QuxlangLanguage.INSTANCE);
    public static final IElementType FREE_FUNCTION = new IElementType("Free function", QuxlangLanguage.INSTANCE);
    public static final IElementType MEMBER_FUNCTION = new IElementType("Member function", QuxlangLanguage.INSTANCE);
    public static final IElementType NAMESPACE = new IElementType("NAMESPACE", QuxlangLanguage.INSTANCE);
    public static final IElementType TEMPLATE_PARAMETER = new IElementType("TEMPLATE_PARAMETER", QuxlangLanguage.INSTANCE);
    public static final IElementType CALL = new IElementType("CALL", QuxlangLanguage.INSTANCE);
    public static final IElementType NUMBER = new IElementType("NUMBER", QuxlangLanguage.INSTANCE);
    public static final IElementType STRING = new IElementType("STRING", QuxlangLanguage.INSTANCE);
    public static final IElementType COMMENT = new IElementType("COMMENT", QuxlangLanguage.INSTANCE);
    public static final IElementType DOC = new IElementType("DOC", QuxlangLanguage.INSTANCE);
    public static final IElementType OPERATOR = new IElementType("OPERATOR", QuxlangLanguage.INSTANCE);
    public static final IElementType BRACES = new IElementType("BRACES", QuxlangLanguage.INSTANCE);
    public static final IElementType PARENTHESES = new IElementType("PARENTHESES", QuxlangLanguage.INSTANCE);
    public static final IElementType BRACKETS = new IElementType("BRACKETS", QuxlangLanguage.INSTANCE);
    public static final IElementType COMMA = new IElementType("COMMA", QuxlangLanguage.INSTANCE);
    public static final IElementType SEMICOLON = new IElementType("SEMICOLON", QuxlangLanguage.INSTANCE);

    private QuxlangToken() {}
}
