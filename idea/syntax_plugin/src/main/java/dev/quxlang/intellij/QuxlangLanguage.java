package dev.quxlang.intellij;

import com.intellij.lang.Language;

/** Identifies Quxlang files and their editor extensions. */
public final class QuxlangLanguage extends Language {
    public static final QuxlangLanguage INSTANCE = new QuxlangLanguage();

    private QuxlangLanguage() {
        super("Quxlang");
    }
}
