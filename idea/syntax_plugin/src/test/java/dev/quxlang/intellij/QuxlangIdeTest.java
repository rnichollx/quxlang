package dev.quxlang.intellij;

import com.intellij.codeInsight.daemon.impl.HighlightInfo;
import com.intellij.openapi.command.WriteCommandAction;
import java.util.List;
import com.intellij.openapi.editor.Document;
import com.intellij.psi.PsiDocumentManager;
import com.intellij.psi.PsiElement;
import com.intellij.psi.PsiReference;
import com.intellij.testFramework.fixtures.BasePlatformTestCase;

/** Exercises parser registration, reference navigation, and cache invalidation in an IDE fixture. */
public final class QuxlangIdeTest extends BasePlatformTestCase {
    public void testArgumentNavigation() {
        String source = "::run FUNCTION(@value I32) { RETURN value; }";
        myFixture.configureByText(QuxlangFileType.INSTANCE, source);
        assertInstanceOf(myFixture.getFile(), QuxlangFile.class);
        PsiElement use = myFixture.getFile().findElementAt(source.lastIndexOf("value")).getParent();
        PsiReference reference = use.getReference();
        assertNotNull(reference);
        PsiElement target = reference.resolve();
        assertNotNull(target);
        assertEquals(source.indexOf("value"), target.getTextOffset());
        assertEquals("value", target.getText());
        myFixture.doHighlighting();
    }

    public void testCastPreservesTypeNavigationAndColor() {
        String source = "::constructor_conversion_choice STRUCT {} ::run FUNCTION() { AUTO value := 1 AS constructor_conversion_choice; constructor_conversion_choice(1); }";
        myFixture.configureByText(QuxlangFileType.INSTANCE, source);
        int use = source.lastIndexOf("constructor_conversion_choice");
        PsiReference reference = myFixture.getFile().findElementAt(use).getParent().getReference();
        assertNotNull(reference);
        PsiElement target = reference.resolve();
        assertNotNull(target);
        assertEquals(source.indexOf("constructor_conversion_choice"), target.getTextOffset());
        assertTrue(myFixture.doHighlighting().stream().anyMatch(info -> info.startOffset == use
            && info.forcedTextAttributesKey == QuxlangHighlighter.COLORS.get(QuxlangToken.TYPE)));
    }

    public void testLoopBindingNavigationAndColor() {
        for (String clause : new String[]{"VALUE", "ITEM", "INDEX", "ITER"}) {
            String source = "::value VAR I32; ::run FUNCTION() { LOOP " + clause + "(value) IN(values) DO { consume(value); } consume(value); }";
            myFixture.configureByText(QuxlangFileType.INSTANCE, source);
            int use = source.indexOf("consume(value)") + 8;
            PsiReference reference = myFixture.getFile().findElementAt(use).getParent().getReference();
            assertNotNull(reference);
            PsiElement target = reference.resolve();
            assertNotNull(target);
            assertEquals(source.indexOf(clause + "(value)") + clause.length() + 1, target.getTextOffset());
            assertTrue(myFixture.doHighlighting().stream().anyMatch(info -> info.startOffset == use
                && info.forcedTextAttributesKey == QuxlangHighlighter.COLORS.get(QuxlangToken.LOCAL)));
            PsiReference outside = myFixture.getFile().findElementAt(source.lastIndexOf("value")).getParent().getReference();
            assertNotNull(outside);
            assertNotNull(outside.resolve());
            assertEquals(source.indexOf("value"), outside.resolve().getTextOffset());
        }
    }

    public void testEditingInvalidatesScopeLookup() {
        String source = "::run FUNCTION(@value I32) { RETURN value; }";
        myFixture.configureByText(QuxlangFileType.INSTANCE, source);
        QuxlangFile file = (QuxlangFile) myFixture.getFile();
        QuxlangSymbols original = file.symbols();
        assertNotNull(original.bindingAt(source.lastIndexOf("value")));
        Document document = myFixture.getEditor().getDocument();
        int declaration = source.indexOf("value");
        WriteCommandAction.runWriteCommandAction(getProject(), () -> document.replaceString(declaration, declaration + 5, "other"));
        PsiDocumentManager.getInstance(getProject()).commitAllDocuments();
        assertNotSame(original, file.symbols());
        assertNull(file.symbols().bindingAt(source.lastIndexOf("value")));
        assertNull(file.findElementAt(source.lastIndexOf("value")).getParent().getReference());
    }

    public void testUnknownReceiversHaveNoNavigationTarget() {
        String source = "::type STRUCT { .field VAR I32; .run FUNCTION() { RETURN other.field; } }";
        myFixture.configureByText(QuxlangFileType.INSTANCE, source);
        assertNull(myFixture.getFile().findElementAt(source.lastIndexOf("field")).getParent().getReference());
        myFixture.doHighlighting();
    }
    public void testCallAnnotationsCoverOnlyParentheses() {
        String source = "::counter STRUCT { .step FUNCTION() {} } ::call FUNCTION(@arg I32) {} ::run FUNCTION(@value I32, @item counter) { call(value); item.step(); }";
        myFixture.configureByText(QuxlangFileType.INSTANCE, source);
        List<HighlightInfo> highlights = myFixture.doHighlighting();
        List<HighlightInfo> calls = highlights.stream()
            .filter(info -> info.forcedTextAttributesKey == QuxlangHighlighter.COLORS.get(QuxlangToken.CALL)).toList();
        assertEquals(4, calls.size());
        for (HighlightInfo call : calls) {
            String text = source.substring(call.startOffset, call.endOffset);
            assertTrue(text.equals("(") || text.equals(")"));
        }
        assertTrue(highlights.stream().anyMatch(info -> info.startOffset == source.lastIndexOf("value")
            && info.forcedTextAttributesKey == QuxlangHighlighter.COLORS.get(QuxlangToken.ARGUMENT)));
        assertTrue(highlights.stream().anyMatch(info -> info.startOffset == source.lastIndexOf("step")
            && info.forcedTextAttributesKey == QuxlangHighlighter.COLORS.get(QuxlangToken.MEMBER_FUNCTION)));
        assertTrue(highlights.stream().anyMatch(info -> info.startOffset == source.indexOf("step")
            && info.forcedTextAttributesKey == QuxlangHighlighter.COLORS.get(QuxlangToken.MEMBER_FUNCTION)));
        assertTrue(highlights.stream().anyMatch(info -> info.startOffset == source.lastIndexOf("call")
            && info.forcedTextAttributesKey == QuxlangHighlighter.COLORS.get(QuxlangToken.FREE_FUNCTION)));
    }

    public void testPrivateFunctionFromMemorySource() throws Exception {
        String source;
        try (java.io.InputStream input = getClass().getResourceAsStream("/fixtures/memory.qxs")) {
            assertNotNull(input);
            source = new String(input.readAllBytes(), java.nio.charset.StandardCharsets.UTF_8);
        }
        myFixture.configureByText(QuxlangFileType.INSTANCE, source);
        List<HighlightInfo> highlights = myFixture.doHighlighting();
        QuxlangLexer lexer = new QuxlangLexer();
        lexer.start(source);
        int function = source.indexOf("::layer5_mapping_size");
        int checked = 0;
        while (lexer.getTokenType() != null) {
            if (lexer.getTokenStart() >= function) {
                String name = lexer.getTokenText();
                com.intellij.psi.tree.IElementType category = switch (name) {
                    case "layer5_mapping_size" -> QuxlangToken.FREE_FUNCTION;
                    case "size" -> QuxlangToken.ARGUMENT;
                    case "remainder", "padding", "maximum_size" -> QuxlangToken.LOCAL;
                    default -> null;
                };
                if (category != null) {
                    int offset = lexer.getTokenStart();
                    assertTrue("Missing semantic color for " + name + " at " + offset,
                        highlights.stream().anyMatch(info -> info.startOffset == offset
                            && info.forcedTextAttributesKey == QuxlangHighlighter.COLORS.get(category)));
                    ++checked;
                }
            }
            lexer.advance();
        }
        assertTrue(checked >= 15);
    }

}
