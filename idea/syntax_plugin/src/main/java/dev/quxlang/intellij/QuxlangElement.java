package dev.quxlang.intellij;

import com.intellij.extapi.psi.ASTWrapperPsiElement;
import com.intellij.lang.ASTNode;
import com.intellij.openapi.util.TextRange;
import com.intellij.psi.PsiElement;
import com.intellij.psi.PsiReference;
import com.intellij.psi.PsiReferenceBase;

/** An editor token whose reference navigates only to an unambiguous declaration. */
public final class QuxlangElement extends ASTWrapperPsiElement {
    public QuxlangElement(ASTNode node) {
        super(node);
    }

    @Override
    public PsiReference getReference() {
        if (!(getContainingFile() instanceof QuxlangFile file)) {
            return null;
        }
        QuxlangSymbols.Binding binding = file.symbols().bindingAt(getTextOffset());
        if (binding == null || binding.declarationOffset() < 0 || binding.declarationOffset() == getTextOffset()) {
            return null;
        }
        return new PsiReferenceBase<QuxlangElement>(this, TextRange.from(0, getTextLength()), true) {
            @Override
            public PsiElement resolve() {
                QuxlangSymbols.Binding current = file.symbols().bindingAt(getElement().getTextOffset());
                if (current == null || current.declarationOffset() < 0) {
                    return null;
                }
                PsiElement leaf = file.findElementAt(current.declarationOffset());
                return leaf == null ? null : leaf.getParent();
            }
        };
    }
}
