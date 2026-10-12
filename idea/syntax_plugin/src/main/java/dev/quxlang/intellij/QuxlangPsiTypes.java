package dev.quxlang.intellij;

import com.intellij.psi.tree.IElementType;
import com.intellij.psi.tree.IFileElementType;
import com.intellij.psi.tree.TokenSet;

/** Holds the PSI node and trivia categories independently of parser initialization. */
final class QuxlangPsiTypes {
    static final IFileElementType FILE = new IFileElementType(QuxlangLanguage.INSTANCE);
    static final IElementType ATOM = new IElementType("ATOM", QuxlangLanguage.INSTANCE);
    static final TokenSet COMMENTS = TokenSet.create(QuxlangToken.COMMENT, QuxlangToken.DOC);
    static final TokenSet STRINGS = TokenSet.create(QuxlangToken.STRING);

    private QuxlangPsiTypes() {}
}
