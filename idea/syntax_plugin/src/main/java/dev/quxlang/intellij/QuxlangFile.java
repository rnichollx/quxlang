package dev.quxlang.intellij;

import com.intellij.extapi.psi.PsiFileBase;
import com.intellij.openapi.fileTypes.FileType;
import com.intellij.psi.FileViewProvider;
import com.intellij.psi.util.CachedValueProvider;
import com.intellij.psi.util.CachedValuesManager;

/** Owns a scope model invalidated whenever this file's PSI changes. */
public final class QuxlangFile extends PsiFileBase {
    public QuxlangFile(FileViewProvider provider) {
        super(provider, QuxlangLanguage.INSTANCE);
    }

    @Override public FileType getFileType() { return QuxlangFileType.INSTANCE; }

    QuxlangSymbols symbols() {
        return CachedValuesManager.getCachedValue(this,
            () -> CachedValueProvider.Result.create(new QuxlangSymbols(getViewProvider().getContents()), this));
    }
}
