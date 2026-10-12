package dev.quxlang.intellij;

import com.intellij.lang.annotation.AnnotationHolder;
import com.intellij.lang.annotation.Annotator;
import com.intellij.lang.annotation.HighlightSeverity;
import com.intellij.openapi.editor.colors.TextAttributesKey;
import com.intellij.psi.PsiElement;
import com.intellij.psi.tree.IElementType;

/** Applies scope-aware colors without diagnosing unsupported or incomplete source. */
public final class QuxlangAnnotator implements Annotator {
    @Override
    public void annotate(PsiElement element, AnnotationHolder holder) {
        if (!(element instanceof QuxlangElement) || !(element.getContainingFile() instanceof QuxlangFile file)) {
            return;
        }
        QuxlangSymbols.Binding binding = file.symbols().bindingAt(element.getTextOffset());
        IElementType token;
        if (binding != null) {
            token = switch (binding.kind()) {
                case ARGUMENT -> QuxlangToken.ARGUMENT;
                case LOCAL -> QuxlangToken.LOCAL;
                case GLOBAL -> QuxlangToken.GLOBAL;
                case FIELD -> QuxlangToken.MEMBER;
                case FREE_FUNCTION -> QuxlangToken.FREE_FUNCTION;
                case MEMBER_FUNCTION -> QuxlangToken.MEMBER_FUNCTION;
                case TYPE -> QuxlangToken.TYPE;
                case NAMESPACE -> QuxlangToken.NAMESPACE;
                case TEMPLATE_PARAMETER -> QuxlangToken.TEMPLATE_PARAMETER;
                case CALL_PAREN -> QuxlangToken.CALL;
                case UNKNOWN -> QuxlangToken.IDENTIFIER;
            };
        } else {
            IElementType lexical = element.getNode().getFirstChildNode().getElementType();
            if (lexical != QuxlangToken.MEMBER && lexical != QuxlangToken.LOCAL) {
                return;
            }
            token = QuxlangToken.IDENTIFIER;
        }
        TextAttributesKey color = QuxlangHighlighter.COLORS.get(token);
        holder.newSilentAnnotation(HighlightSeverity.INFORMATION).range(element).textAttributes(color).create();
    }
}
