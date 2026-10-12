package dev.quxlang.intellij;

import com.intellij.psi.TokenType;
import com.intellij.psi.tree.IElementType;
import java.util.ArrayList;
import java.util.List;
import org.junit.jupiter.api.Test;
import static org.junit.jupiter.api.Assertions.*;

/** Exercises real source forms, edit recovery, and the incremental lexer contract. */
final class QuxlangLexerTest {
    private record Token(IElementType type, String text, int start, int end, int state) {}

    private static List<Token> scan(String text) {
        QuxlangLexer lexer = new QuxlangLexer();
        lexer.start(text);
        List<Token> result = new ArrayList<>();
        int previousEnd = 0;
        while (lexer.getTokenType() != null) {
            assertEquals(previousEnd, lexer.getTokenStart());
            assertTrue(lexer.getTokenEnd() > lexer.getTokenStart());
            result.add(new Token(lexer.getTokenType(), lexer.getTokenText(), lexer.getTokenStart(), lexer.getTokenEnd(), lexer.getState()));
            previousEnd = lexer.getTokenEnd();
            lexer.advance();
        }
        assertEquals(text.length(), previousEnd);
        return result;
    }

    private static List<IElementType> wordTypes(String text) {
        return scan(text).stream()
            .filter(token -> token.type() != TokenType.WHITE_SPACE && token.type() != QuxlangToken.OPERATOR)
            .map(Token::type).toList();
    }

    @Test
    void identifiesMembersParametersAndLocals() {
        assertEquals(List.of(QuxlangToken.MEMBER, QuxlangToken.ARGUMENT, QuxlangToken.ARGUMENT,
                QuxlangToken.KEYWORD, QuxlangToken.LOCAL, QuxlangToken.TYPE),
            wordTypes(".value @amount @OTHER VAR local I32"));
        assertEquals(List.of(QuxlangToken.MEMBER, QuxlangToken.KEYWORD, QuxlangToken.IDENTIFIER),
            wordTypes(".value VAR user_type"));
        assertEquals(List.of(QuxlangToken.IDENTIFIER, QuxlangToken.KEYWORD, QuxlangToken.IDENTIFIER),
            wordTypes("::global VAR user_type"));
    }

    @Test
    void keepsContextAcrossCommentsAndWhitespace() {
        List<Token> tokens = scan("@ // parameter\n amount . // field\r\n value");
        assertEquals(QuxlangToken.ARGUMENT, tokens.stream().filter(t -> t.text().equals("amount")).findFirst().orElseThrow().type());
        assertEquals(QuxlangToken.MEMBER, tokens.getLast().type());
        List<Token> documented = scan(".value DOC <$ Field documentation. $> VAR user_type");
        assertEquals(QuxlangToken.IDENTIFIER, documented.getLast().type());
    }

    @Test
    void separatesCallsAndDecimalsFromMembers() {
        List<Token> tokens = scan("object.method(2.5); .value := 10; call();");
        assertEquals(QuxlangToken.MEMBER, tokens.stream().filter(t -> t.text().equals("method")).findFirst().orElseThrow().type());
        assertEquals(QuxlangToken.IDENTIFIER, tokens.stream().filter(t -> t.text().equals("call")).findFirst().orElseThrow().type());
        assertEquals(List.of("2.5", "10"), tokens.stream().filter(t -> t.type() == QuxlangToken.NUMBER).map(Token::text).toList());
    }

    @Test
    void recognizesBuiltinTypesAndKeywordClass() {
        assertEquals(List.of(QuxlangToken.TYPE, QuxlangToken.TYPE, QuxlangToken.TYPE, QuxlangToken.TYPE,
                QuxlangToken.TYPE, QuxlangToken.TYPE, QuxlangToken.KEYWORD, QuxlangToken.KEYWORD),
            wordTypes("I128 U8 F64 F16E5 ADDRESS BOOL RETURN STATIC_TEST"));
    }

    @Test
    void protectsLiteralAndDocumentationContents() {
        String source = "DOC <$ .member @arg // prose\n \"text\" $> \"escaped\\\" // @text\" '\\'' // line\nRETURN";
        List<Token> tokens = scan(source);
        assertEquals(1, tokens.stream().filter(t -> t.type() == QuxlangToken.DOC).count());
        assertEquals(2, tokens.stream().filter(t -> t.type() == QuxlangToken.STRING).count());
        assertEquals(1, tokens.stream().filter(t -> t.type() == QuxlangToken.COMMENT).count());
        assertTrue(tokens.stream().noneMatch(t -> t.type() == QuxlangToken.MEMBER || t.type() == QuxlangToken.ARGUMENT));
        assertEquals(QuxlangToken.KEYWORD, tokens.getLast().type());
    }

    @Test
    void consumesIncompleteAndInvalidInput() {
        for (String source : List.of("", "@", "DOC <$ unfinished", "\"unfinished\\", "'", "$", "\uD83D\uDE00")) {
            scan(source);
        }
        assertEquals(TokenType.BAD_CHARACTER, scan("$").getFirst().type());
    }

    @Test
    void restartsAtEveryTokenBoundary() {
        String source = "::item STRUCT { .field DOC <$ Field $> VAR user_type; .run FUNCTION(@arg I32) { VAR local I32 := .field; } }\n"
            + "DOC // description\n <$ docs\n @name \"text\" $> call(\"a\\\"b\", 2.5); @ // comment\n OTHER";
        List<Token> tokens = scan(source);
        for (int index = 0; index < tokens.size(); ++index) {
            Token start = tokens.get(index);
            QuxlangLexer restarted = new QuxlangLexer();
            restarted.start(source, start.start(), source.length(), start.state());
            for (int remaining = index; remaining < tokens.size(); ++remaining) {
                Token expected = tokens.get(remaining);
                assertEquals(expected.type(), restarted.getTokenType(), "Restart at " + start.start());
                assertEquals(expected.start(), restarted.getTokenStart());
                assertEquals(expected.end(), restarted.getTokenEnd());
                assertEquals(expected.state(), restarted.getState());
                restarted.advance();
            }
            assertNull(restarted.getTokenType());
        }
    }

    @Test
    void respectsSliceBoundaries() {
        QuxlangLexer lexer = new QuxlangLexer();
        lexer.start("skip DOC <$ tail $>", 5, 12, 0);
        while (lexer.getTokenType() != null) {
            assertTrue(lexer.getTokenEnd() <= 12);
            lexer.advance();
        }
        assertEquals(12, lexer.getTokenEnd());
    }

    @Test
    void callNamesRetainOrdinaryLexicalCategories() {
        assertEquals(QuxlangToken.IDENTIFIER, scan("name   (").getFirst().type());
        assertEquals(QuxlangToken.IDENTIFIER, scan("name   ;").getFirst().type());
        assertTrue(scan("name()").stream().noneMatch(t -> t.type() == QuxlangToken.CALL));
    }
}
