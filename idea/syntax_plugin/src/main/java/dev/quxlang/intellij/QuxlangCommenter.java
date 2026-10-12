package dev.quxlang.intellij;

import com.intellij.lang.Commenter;

/** Supplies Quxlang's line-comment syntax to editor comment actions. */
public final class QuxlangCommenter implements Commenter {
    @Override public String getLineCommentPrefix() { return "//"; }
    @Override public String getBlockCommentPrefix() { return null; }
    @Override public String getBlockCommentSuffix() { return null; }
    @Override public String getCommentedBlockCommentPrefix() { return null; }
    @Override public String getCommentedBlockCommentSuffix() { return null; }
}
