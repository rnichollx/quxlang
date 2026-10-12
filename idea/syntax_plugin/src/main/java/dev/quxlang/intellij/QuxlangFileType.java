package dev.quxlang.intellij;

import com.intellij.icons.AllIcons;
import com.intellij.openapi.fileTypes.LanguageFileType;
import javax.swing.Icon;

/** Associates .qxs source files with the Quxlang language. */
public final class QuxlangFileType extends LanguageFileType {
    public static final QuxlangFileType INSTANCE = new QuxlangFileType();

    private QuxlangFileType() {
        super(QuxlangLanguage.INSTANCE);
    }

    @Override public String getName() { return "Quxlang"; }
    @Override public String getDescription() { return "Quxlang source"; }
    @Override public String getDefaultExtension() { return "qxs"; }
    @Override public Icon getIcon() { return AllIcons.FileTypes.Text; }
}
