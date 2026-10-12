package dev.quxlang.intellij;

import com.intellij.openapi.editor.colors.TextAttributesKey;
import com.intellij.openapi.fileTypes.SyntaxHighlighter;
import com.intellij.openapi.options.colors.AttributesDescriptor;
import com.intellij.openapi.options.colors.ColorDescriptor;
import com.intellij.openapi.options.colors.ColorSettingsPage;
import java.util.Map;
import javax.swing.Icon;

/** Exposes the Quxlang palette and a live syntax preview in editor settings. */
public final class QuxlangColorSettingsPage implements ColorSettingsPage {
    @Override public Icon getIcon() { return QuxlangFileType.INSTANCE.getIcon(); }
    @Override public SyntaxHighlighter getHighlighter() { return new QuxlangHighlighter(); }
    @Override public String getDisplayName() { return "Quxlang"; }
    @Override public ColorDescriptor[] getColorDescriptors() { return ColorDescriptor.EMPTY_ARRAY; }
    @Override
    public Map<String, TextAttributesKey> getAdditionalHighlightingTagToDescriptorMap() {
        return Map.of(
            "call", QuxlangHighlighter.COLORS.get(QuxlangToken.CALL),
            "freeFunction", QuxlangHighlighter.COLORS.get(QuxlangToken.FREE_FUNCTION),
            "memberFunction", QuxlangHighlighter.COLORS.get(QuxlangToken.MEMBER_FUNCTION),
            "argument", QuxlangHighlighter.COLORS.get(QuxlangToken.ARGUMENT),
            "local", QuxlangHighlighter.COLORS.get(QuxlangToken.LOCAL),
            "type", QuxlangHighlighter.COLORS.get(QuxlangToken.TYPE));
    }

    @Override
    public AttributesDescriptor[] getAttributeDescriptors() {
        return QuxlangHighlighter.COLORS.entrySet().stream()
            .map(entry -> new AttributesDescriptor(entry.getKey().toString(), entry.getValue()))
            .toArray(AttributesDescriptor[]::new);
    }

    @Override
    public String getDemoText() {
        return """
            LANGUAGE QUXLANG EN 0.0;
            // A counter with a named argument.
            ::<type>counter</type> DOC <$ Stores the current count. $> STRUCT
            {
                .value VAR I32;
                .<memberFunction>add</memberFunction> FUNCTION(@amount I32) MUT: I32
                {
                    VAR next I32 := .value + <argument>amount</argument>;
                    .value := <local>next</local>;
                    RETURN .value;
                }
            }
            ::<freeFunction>example</freeFunction> FUNCTION()
            {
                VAR item <type>counter</type>;
                <local>item</local>.<memberFunction>add</memberFunction><call>(</call>@amount 2<call>)</call>;
                print<call>(</call>"Count updated\\n"<call>)</call>;
            }
            """;
    }
}
