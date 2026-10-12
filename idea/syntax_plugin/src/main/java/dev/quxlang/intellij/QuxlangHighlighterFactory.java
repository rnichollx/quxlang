package dev.quxlang.intellij;

import com.intellij.openapi.fileTypes.SyntaxHighlighter;
import com.intellij.openapi.fileTypes.SyntaxHighlighterFactory;
import com.intellij.openapi.project.Project;
import com.intellij.openapi.vfs.VirtualFile;

/** Creates an independent syntax highlighter for each editor. */
public final class QuxlangHighlighterFactory extends SyntaxHighlighterFactory {
    @Override
    public SyntaxHighlighter getSyntaxHighlighter(Project project, VirtualFile file) {
        return new QuxlangHighlighter();
    }
}
