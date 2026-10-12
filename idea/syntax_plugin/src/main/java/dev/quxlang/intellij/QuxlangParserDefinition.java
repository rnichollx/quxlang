package dev.quxlang.intellij;

import com.intellij.lang.ASTNode;
import com.intellij.lang.ParserDefinition;
import com.intellij.lang.PsiBuilder;
import com.intellij.lang.PsiParser;
import com.intellij.lexer.Lexer;
import com.intellij.openapi.project.Project;
import com.intellij.psi.FileViewProvider;
import com.intellij.psi.PsiElement;
import com.intellij.psi.PsiFile;
import com.intellij.psi.tree.IFileElementType;
import com.intellij.psi.tree.TokenSet;

/** Builds resilient PSI atoms; the file's scope parser supplies declaration semantics. */
public final class QuxlangParserDefinition implements ParserDefinition {
    @Override public Lexer createLexer(Project project) { return new QuxlangLexer(); }
    @Override public IFileElementType getFileNodeType() { return QuxlangPsiTypes.FILE; }
    @Override public TokenSet getCommentTokens() { return QuxlangPsiTypes.COMMENTS; }
    @Override public TokenSet getStringLiteralElements() { return QuxlangPsiTypes.STRINGS; }
    @Override public PsiFile createFile(FileViewProvider provider) { return new QuxlangFile(provider); }
    @Override public PsiElement createElement(ASTNode node) { return new QuxlangElement(node); }

    @Override
    public PsiParser createParser(Project project) {
        return (root, builder) -> {
            PsiBuilder.Marker file = builder.mark();
            while (!builder.eof()) {
                PsiBuilder.Marker atom = builder.mark();
                builder.advanceLexer();
                atom.done(QuxlangPsiTypes.ATOM);
            }
            file.done(root);
            return builder.getTreeBuilt();
        };
    }
}
