package dev.quxlang.intellij;

import com.intellij.openapi.progress.ProgressManager;
import com.intellij.psi.TokenType;
import com.intellij.psi.tree.IElementType;
import java.util.ArrayDeque;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Set;

/**
 * Parses declaration headers and lexical scopes in one source file. Lookup uses
 * syntax alone: overloads, aliases, dependent selections, and inferred receiver
 * types have no target. Source offsets identify declarations in the current PSI.
 */
final class QuxlangSymbols {
    enum Kind { ARGUMENT, LOCAL, GLOBAL, FIELD, FREE_FUNCTION, MEMBER_FUNCTION, TYPE, NAMESPACE, TEMPLATE_PARAMETER, CALL_PAREN, UNKNOWN }
    record Binding(Kind kind, int declarationOffset) {}
    private enum Mode { DECLARATIONS, STATEMENTS, EXPRESSION }
    private record Token(String text, int offset) {}
    private record Region(int start, int end, Scope scope, Mode mode) {}
    private static final Set<String> DECLARATIONS = Set.of(
        "FUNCTION", "STRUCT", "CLASS", "NAMESPACE", "VAR", "STATIC", "ALIAS", "ENUM", "FLAGSET",
        "UNION", "VARIANT", "INTERFACE", "UNIT_TEST", "STATIC_TEST", "DUAL_TEST", "BENCHMARK", "OPTION");
    private static final Set<String> SYNTAX = Set.of(
        "RETURN", "VAR", "STATIC", "STATIC_VAR", "FUNCTION", "TEMPLATE", "PROCEDURE", "DOC",
        "IF", "ELSE", "WHILE", "FOR", "LOOP", "BREAK", "CONTINUE", "AS", "TYPE", "CLASS", "VALUE");
    private static final Set<String> TYPE_QUALIFIERS = Set.of("CONST", "MUT", "TEMP", "WRITE");

    /** Named and instance declarations occupy separate lookup tables. */
    private static final class Scope {
        final Scope parent;
        final boolean declarationScope;
        final Map<String, List<Symbol>> names = new HashMap<>();
        final Map<String, List<Symbol>> members = new HashMap<>();
        Scope instanceOwner;
        boolean barrier;

        Scope(Scope parent, boolean declarationScope) {
            this.parent = parent;
            this.declarationScope = declarationScope;
            this.instanceOwner = parent == null ? null : parent.instanceOwner;
        }
    }

    /** A declaration retains its visibility boundary and optional explicit type. */
    private static final class Symbol {
        final String name;
        final Kind kind;
        final int token;
        final int visibleFrom;
        final Scope owner;
        final boolean dependent;
        Scope contents;
        int typeStart = -1;
        int typeEnd = -1;

        Symbol(String name, Kind kind, int token, int visibleFrom, Scope owner, boolean dependent) {
            this.name = name;
            this.kind = kind;
            this.token = token;
            this.visibleFrom = visibleFrom;
            this.owner = owner;
            this.dependent = dependent;
        }
    }

    private final List<Token> tokens = new ArrayList<>();
    private final Map<Integer, Binding> bindings = new HashMap<>();
    private final Map<Integer, Symbol> declarations = new HashMap<>();
    private final Scope root = new Scope(null, true);
    private final ArrayDeque<Region> pending = new ArrayDeque<>();
    private final int[] close;
    private final Scope[] scopes;
    private final boolean[] suppressed;
    private final Symbol[] resolved;

    QuxlangSymbols(CharSequence source) {
        QuxlangLexer lexer = new QuxlangLexer();
        lexer.start(source);
        while (lexer.getTokenType() != null) {
            ProgressManager.checkCanceled();
            IElementType type = lexer.getTokenType();
            if (type != TokenType.WHITE_SPACE && type != QuxlangToken.COMMENT && type != QuxlangToken.DOC) {
                tokens.add(new Token(lexer.getTokenText(), lexer.getTokenStart()));
            }
            lexer.advance();
        }
        close = new int[tokens.size()];
        Arrays.fill(close, tokens.size());
        scopes = new Scope[tokens.size()];
        suppressed = new boolean[tokens.size()];
        resolved = new Symbol[tokens.size()];
        pairDelimiters();
        pending.push(new Region(0, tokens.size(), root, Mode.DECLARATIONS));
        while (!pending.isEmpty()) {
            ProgressManager.checkCanceled();
            parseRegion(pending.pop());
        }
        resolveNames();
        classifyCalls();
    }

    Binding bindingAt(int offset) {
        return bindings.get(offset);
    }

    private void pairDelimiters() {
        ArrayDeque<Integer> opens = new ArrayDeque<>();
        for (int i = 0; i < tokens.size(); ++i) {
            String text = text(i);
            if (isOpen(i)) {
                opens.push(i);
            } else if (!opens.isEmpty() && matches(text(opens.peek()), text)) {
                close[opens.pop()] = i;
            }
        }
    }

    private static boolean matches(String open, String end) {
        return open.equals("(") && end.equals(")") || open.equals("[") && end.equals("]")
            || open.equals("{") && end.equals("}");
    }

    private void parseRegion(Region region) {
        boolean statementStart = true;
        for (int i = region.start(); i < region.end();) {
            ProgressManager.checkCanceled();
            scopes[i] = region.scope();
            String text = text(i);
            if (statementStart && (text.equals("::") || text.equals(".")) && isWord(i + 1)) {
                int after = parseDeclaration(i, region);
                if (after > i) {
                    i = after;
                    statementStart = true;
                    continue;
                }
            }
            if (text.equals("-") && text(i + 1).equals("<")) {
                int after = parseLambda(i, region);
                if (after > i) {
                    i = after;
                    statementStart = false;
                    continue;
                }
            }
            if (region.mode() == Mode.STATEMENTS && statementStart
                && (text.equals("VAR") || text.equals("STATIC") || text.equals("STATIC_VAR")) && isWord(i + 1)) {
                int end = statementEnd(i + 2, region.end());
                Symbol symbol = declare(i + 1, Kind.LOCAL, region.scope(), false, end + 1, false);
                symbol.typeStart = i + 2;
                symbol.typeEnd = typeEnd(i + 2, end);
            }
            // Unsupported binding constructs shadow outer names conservatively.
            if (text.equals("AS") && isWord(i + 1)) {
                declare(i + 1, Kind.UNKNOWN, region.scope(), false, i + 2, false);
            }
            if (text.equals("PROCEDURE")) {
                int args = i + 1;
                while (args < region.end() && !text(args).equals("(") && !text(args).equals(";")) {
                    ++args;
                }
                if (text(args).equals("(")) {
                    int end = Math.min(close[args], region.end());
                    Scope signature = new Scope(region.scope(), false);
                    assign(i, end, signature);
                    parseParameters(args, end, signature, Kind.ARGUMENT);
                    i = Math.min(end + 1, region.end());
                    continue;
                }
            }
            if (region.mode() == Mode.DECLARATIONS && text.equals("PRIVATE")) {
                int body = i + 1;
                if (text(body).equals("(")) {
                    body = Math.min(close[body] + 1, region.end());
                }
                assign(i, body, region.scope());
                if (text(body).equals("{")) {
                    int end = Math.min(close[body], region.end());
                    pending.push(new Region(body + 1, end, region.scope(), Mode.DECLARATIONS));
                    i = Math.min(end + 1, region.end());
                } else {
                    i = body;
                }
                statementStart = true;
                continue;
            }
            if (isOpen(i)) {
                int end = Math.min(close[i], region.end());
                boolean block = text.equals("{");
                Scope child = block ? new Scope(region.scope(), false) : region.scope();
                if (block && region.mode() == Mode.DECLARATIONS) {
                    child.barrier = true;
                }
                pending.push(new Region(i + 1, end, child, block ? region.mode() : Mode.EXPRESSION));
                i = Math.min(end + 1, region.end());
                statementStart = block;
                continue;
            }
            statementStart = text.equals(";") || text.equals("}");
            ++i;
        }
    }

    /** Recognizes a name, optional template header, and the owning declaration body. */
    private int parseDeclaration(int start, Region region) {
        int name = start + 1;
        int kindIndex = name + 1;
        boolean template = false;
        while (kindIndex < region.end() && !DECLARATIONS.contains(text(kindIndex))) {
            if (text(kindIndex).equals("TEMPLATE")) {
                template = true;
            } else if (!Set.of("DOC", "ENABLE_IF", "INCLUDE_IF", "PRIVATE").contains(text(kindIndex))) {
                return start;
            }
            ++kindIndex;
            if (text(kindIndex).equals("(")) {
                kindIndex = Math.min(close[kindIndex] + 1, region.end());
            }
        }
        if (kindIndex >= region.end()) {
            return start;
        }
        String keyword = text(kindIndex);
        boolean instance = text(start).equals(".");
        Kind kind = switch (keyword) {
            case "FUNCTION", "UNIT_TEST", "STATIC_TEST", "DUAL_TEST", "BENCHMARK" -> instance ? Kind.MEMBER_FUNCTION : Kind.FREE_FUNCTION;
            case "STRUCT", "CLASS", "ENUM", "FLAGSET", "UNION", "VARIANT", "INTERFACE" -> Kind.TYPE;
            case "NAMESPACE" -> Kind.NAMESPACE;
            case "VAR", "STATIC" -> instance ? Kind.FIELD : Kind.GLOBAL;
            default -> Kind.UNKNOWN;
        };
        int boundary = kindIndex + 1;
        while (boundary < region.end() && !text(boundary).equals("{") && !text(boundary).equals(";")) {
            boundary = isOpen(boundary) ? Math.min(close[boundary] + 1, region.end()) : boundary + 1;
        }
        int body = text(boundary).equals("{") ? boundary : -1;
        int end = body < 0 ? boundary : Math.min(close[body] + 1, region.end());
        boolean conditional = false;
        for (int i = name + 1; i < boundary; ++i) {
            conditional |= text(i).equals("INCLUDE_IF") || text(i).equals("ENABLE_IF");
        }
        Symbol symbol = declare(name, kind, region.scope(), instance, 0, template || conditional);
        Scope header = new Scope(region.scope(), false);
        Scope contents = header;
        if (kind == Kind.TYPE || kind == Kind.NAMESPACE) {
            contents = new Scope(header, true);
            symbol.contents = contents;
        }
        if (kind == Kind.NAMESPACE && !template) {
            List<Symbol> siblings = region.scope().names.get(symbol.name);
            if (siblings.size() > 1 && siblings.stream().allMatch(s -> s.kind == Kind.NAMESPACE && !s.dependent)) {
                symbol.contents = siblings.getFirst().contents;
                contents = symbol.contents;
            }
        }
        if (kind == Kind.FREE_FUNCTION || kind == Kind.MEMBER_FUNCTION) {
            header.instanceOwner = instance ? nearestDeclarationScope(region.scope()) : null;
        }
        int headerEnd = body < 0 ? end : body;
        assign(start, headerEnd, header);
        scopes[name] = region.scope();
        for (int i = name + 1; i < headerEnd; ++i) {
            suppressed[i] = SYNTAX.contains(text(i)) || DECLARATIONS.contains(text(i));
            if ((text(i).equals("TEMPLATE") || text(i).equals("FUNCTION")) && text(i + 1).equals("(")) {
                int last = Math.min(close[i + 1], headerEnd);
                parseParameters(i + 1, last, header,
                    text(i).equals("TEMPLATE") ? Kind.TEMPLATE_PARAMETER : Kind.ARGUMENT);
                i = last;
            }
        }
        if (kind == Kind.FIELD || kind == Kind.GLOBAL) {
            symbol.typeStart = kindIndex + 1;
            symbol.typeEnd = typeEnd(symbol.typeStart, headerEnd);
        }
        if (body >= 0) {
            Mode mode = kind == Kind.TYPE || kind == Kind.NAMESPACE ? Mode.DECLARATIONS : Mode.STATEMENTS;
            pending.push(new Region(body + 1, Math.min(close[body], region.end()), contents, mode));
        }
        return end < region.end() && text(end).equals(";") ? end + 1 : end;
    }

    /** Parameter separators are inspected only at the signature's outermost level. */
    private void parseParameters(int open, int end, Scope scope, Kind kind) {
        suppressed[open] = true;
        if (end < suppressed.length) {
            suppressed[end] = true;
        }
        boolean atStart = true;
        for (int i = open + 1; i < end; ++i) {
            if (atStart && (text(i).equals("@") || text(i).equals("%"))) {
                int name = i + 1;
                while (text(name).equals(".")) {
                    ++name;
                }
                if (isWord(name) && !text(name).equals("IGNORED")) {
                    int local = text(name + 1).equals(":") && isWord(name + 2) ? name + 2 : name;
                    Symbol symbol = declare(local, kind, scope, false, local + 1, false);
                    bindings.put(tokens.get(name).offset(), new Binding(kind, tokens.get(local).offset()));
                    if (name != local) {
                        suppressed[name] = true;
                    }
                    symbol.typeStart = local + 1;
                    symbol.typeEnd = parameterEnd(local + 1, end);
                    i = local;
                }
            }
            if (isOpen(i)) {
                int last = Math.min(close[i], end);
                // Names declared by a nested procedure signature never enter this scope.
                if (text(i - 1).equals("PROCEDURE")) {
                    Scope signature = new Scope(scope, false);
                    assign(i + 1, last, signature);
                    // Parameter names in procedure types are labels, not outer bindings.
                    for (int inner = i + 1; inner < last; ++inner) {
                        if (text(inner - 1).equals("@") || text(inner - 1).equals("%")) {
                            declare(inner, Kind.ARGUMENT, signature, false, inner + 1, false);
                        }
                    }
                }
                i = last;
            }
            atStart = text(i).equals(",");
        }
    }

    /** Lambda parameters have their own scope; capture-dependent lookup stays unknown. */
    private int parseLambda(int start, Region region) {
        int cursor = start + 2;
        if (text(cursor).equals("[")) {
            cursor = Math.min(close[cursor] + 1, region.end());
        }
        Scope lambda = new Scope(nearestDeclarationScope(region.scope()), false);
        lambda.instanceOwner = null;
        lambda.barrier = true;
        int parameters = text(cursor).equals("(") ? cursor : -1;
        if (parameters >= 0) {
            cursor = Math.min(close[cursor] + 1, region.end());
        }
        while (cursor < region.end() && !text(cursor).equals("{") && !text(cursor).equals("=") && !text(cursor).equals(";")) {
            ++cursor;
        }
        if (cursor >= region.end() || text(cursor).equals(";")) {
            return start;
        }
        assign(start, cursor, lambda);
        if (parameters >= 0) {
            parseParameters(parameters, Math.min(close[parameters], cursor), lambda, Kind.ARGUMENT);
        }
        boolean block = text(cursor).equals("{");
        int end = block ? Math.min(close[cursor], region.end()) : parameterEnd(cursor + 1, region.end());
        pending.push(new Region(cursor + 1, end, lambda, block ? Mode.STATEMENTS : Mode.EXPRESSION));
        return block ? Math.min(end + 1, region.end()) : end;
    }

    private Symbol declare(int token, Kind kind, Scope scope, boolean member, int visibleFrom, boolean dependent) {
        Symbol symbol = new Symbol(text(token), kind, token, visibleFrom, scope, dependent);
        Map<String, List<Symbol>> table = member ? scope.members : scope.names;
        table.computeIfAbsent(symbol.name, ignored -> new ArrayList<>()).add(symbol);
        declarations.put(token, symbol);
        bindings.put(tokens.get(token).offset(), new Binding(kind, tokens.get(token).offset()));
        return symbol;
    }

    private void resolveNames() {
        for (int i = 0; i < tokens.size(); ++i) {
            ProgressManager.checkCanceled();
            if (!isWord(i)) {
                continue;
            }
            if (declarations.containsKey(i)) {
                resolved[i] = declarations.get(i);
                continue;
            }
            if (suppressed[i] || scopes[i] == null || SYNTAX.contains(text(i))) {
                continue;
            }
            Symbol target;
            if (text(i - 1).equals("@")) {
                continue;
            } else if (text(i - 1).equals(".")) {
                Scope owner = scopes[i].instanceOwner;
                if (isReceiver(i - 2)) {
                    owner = text(i - 2).equals("THIS") ? owner : receiverType(resolved[i - 2]);
                } else if (text(i - 2).equals(">") && text(i - 3).equals("-")) {
                    owner = i >= 4 ? receiverType(resolved[i - 4]) : null;
                } else if (text(i - 2).equals("::")) {
                    Symbol receiver = i >= 3 ? resolved[i - 3] : null;
                    owner = receiver == null || receiver.dependent ? null : receiver.contents;
                }
                target = owner == null ? null : unique(owner.members.get(text(i)), i);
            } else if (text(i - 1).equals("::")) {
                if (isReceiver(i - 2)) {
                    Symbol receiver = resolved[i - 2];
                    Scope owner = receiver == null || receiver.dependent ? null : receiver.contents;
                    target = owner == null ? null : unique(owner.names.get(text(i)), i);
                } else {
                    target = unique(root.names.get(text(i)), i);
                }
            } else {
                target = lookup(scopes[i], text(i), i);
            }
            if (target != null && (target.dependent || target.kind == Kind.UNKNOWN)) {
                target = null;
            }
            resolved[i] = target;
            if (target != null) {
                bindings.put(tokens.get(i).offset(), new Binding(target.kind, tokens.get(target.token).offset()));
            }
        }
    }

    /** Colors only call delimiters, leaving every name to ordinary symbol lookup. */
    private void classifyCalls() {
        Set<String> nonCalls = Set.of("IF", "UNLESS", "WHILE", "FOR", "SWITCH", "MATCH", "VISIT",
            "FUNCTION", "TEMPLATE", "PROCEDURE", "ENABLE_IF", "INCLUDE_IF", "PRIVATE", "PUBLIC",
            "ALIGN", "ALIGNED", "LABEL", "CATCH", "POSTTEST", "NAMESPACE", "STRUCT", "CLASS",
            "RETURN", "THROW", "ELSE", "CASE", "LIKELY", "UNLIKELY", "STATIC_IF", "STATIC_WHILE",
            "STDCALL", "CCALL", "NOEXCEPT");
        for (int i = 0; i < tokens.size(); ++i) {
            if (!text(i).equals("(") || suppressed[i] || nonCalls.contains(text(i - 1))) {
                continue;
            }
            boolean callable = isWord(i - 1) || text(i - 1).equals(")") || text(i - 1).equals("]");
            if (callable) {
                bindings.put(tokens.get(i).offset(), new Binding(Kind.CALL_PAREN, -1));
                if (close[i] < tokens.size()) {
                    bindings.put(tokens.get(close[i]).offset(), new Binding(Kind.CALL_PAREN, -1));
                }
            }
        }
    }

    private Scope receiverType(Symbol value) {
        if (value == null || value.typeStart < 0 || value.dependent) {
            return null;
        }
        int i = value.typeStart;
        while (i < value.typeEnd && (TYPE_QUALIFIERS.contains(text(i)) || Set.of("&", "-", ">").contains(text(i)))) {
            ++i;
        }
        Symbol type = lookup(value.owner, text(i), value.token);
        ++i;
        while (i + 1 < value.typeEnd && text(i).equals("::") && type != null && type.contents != null && !type.dependent) {
            type = unique(type.contents.names.get(text(i + 1)), value.token);
            i += 2;
        }
        return i == value.typeEnd && type != null && type.kind == Kind.TYPE && !type.dependent ? type.contents : null;
    }

    private Symbol lookup(Scope scope, String name, int at) {
        for (Scope current = scope; current != null; current = current.parent) {
            List<Symbol> candidates = current.names.get(name);
            if (candidates != null && candidates.stream().anyMatch(s -> s.visibleFrom <= at)) {
                return unique(candidates, at);
            }
            if (current.barrier) {
                return null;
            }
        }
        return null;
    }

    private Symbol unique(List<Symbol> candidates, int at) {
        if (candidates == null) {
            return null;
        }
        Symbol result = null;
        for (Symbol candidate : candidates) {
            if (candidate.visibleFrom <= at) {
                if (result != null && !(result.kind == Kind.NAMESPACE && candidate.kind == Kind.NAMESPACE)) {
                    return null;
                }
                result = candidate;
            }
        }
        return result;
    }

    private Scope nearestDeclarationScope(Scope scope) {
        for (Scope current = scope; current != null; current = current.parent) {
            if (current.declarationScope) {
                return current;
            }
        }
        return root;
    }

    private int statementEnd(int start, int limit) {
        int i = start;
        while (i < limit && !text(i).equals(";")) {
            i = isOpen(i) ? Math.min(close[i] + 1, limit) : i + 1;
        }
        return i;
    }

    private int parameterEnd(int start, int limit) {
        int i = start;
        while (i < limit && !Set.of(",", ";", "DEFAULT").contains(text(i))) {
            i = isOpen(i) ? Math.min(close[i] + 1, limit) : i + 1;
        }
        return i;
    }

    private int typeEnd(int start, int limit) {
        int i = start;
        while (i < limit && !text(i).equals(":")) {
            i = isOpen(i) ? Math.min(close[i] + 1, limit) : i + 1;
        }
        return i;
    }

    private void assign(int start, int end, Scope scope) {
        for (int i = start; i < end; ++i) {
            scopes[i] = scope;
        }
    }

    private boolean isReceiver(int i) {
        return isWord(i) && !SYNTAX.contains(text(i)) || text(i).equals(")") || text(i).equals("]");
    }

    private boolean isWord(int i) {
        String word = text(i);
        return !word.isEmpty() && (word.charAt(0) >= 'a' && word.charAt(0) <= 'z'
            || word.charAt(0) >= 'A' && word.charAt(0) <= 'Z' || word.charAt(0) == '_');
    }

    private boolean isOpen(int i) {
        return text(i).equals("(") || text(i).equals("[") || text(i).equals("{");
    }

    private String text(int i) {
        return i < 0 || i >= tokens.size() ? "" : tokens.get(i).text();
    }
}
