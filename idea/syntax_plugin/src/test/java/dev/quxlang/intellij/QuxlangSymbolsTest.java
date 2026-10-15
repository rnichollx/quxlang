package dev.quxlang.intellij;

import org.junit.jupiter.api.Test;
import static org.junit.jupiter.api.Assertions.*;
import static dev.quxlang.intellij.QuxlangSymbols.Kind.*;

/** Checks lexical binding, conservative lookup, and call delimiter classification. */
final class QuxlangSymbolsTest {
    private record Marked(String source, int declaration, int reference) {
        static Marked parse(String input) {
            StringBuilder source = new StringBuilder();
            int declaration = -1;
            int reference = -1;
            for (int i = 0; i < input.length();) {
                if (input.startsWith("<d>", i)) {
                    declaration = source.length();
                    i += 3;
                } else if (input.startsWith("<r>", i)) {
                    reference = source.length();
                    i += 3;
                } else {
                    source.append(input.charAt(i++));
                }
            }
            return new Marked(source.toString(), declaration, reference);
        }
    }

    private void assertResolution(String source, QuxlangSymbols.Kind kind) {
        Marked marked = Marked.parse(source);
        QuxlangSymbols.Binding binding = new QuxlangSymbols(marked.source()).bindingAt(marked.reference());
        assertNotNull(binding, marked.source());
        assertEquals(kind, binding.kind());
        assertEquals(marked.declaration(), binding.declarationOffset());
    }

    private void assertUnresolved(String source) {
        Marked marked = Marked.parse(source);
        assertNull(new QuxlangSymbols(marked.source()).bindingAt(marked.reference()), marked.source());
    }

    @Test void argumentsShadowGlobals() {
        assertResolution("::value VAR I32; ::run FUNCTION(@<d>value I32) { RETURN <r>value; }", ARGUMENT);
    }

    @Test void nestedLocalsShadowAndExpire() {
        assertResolution("::run FUNCTION(@value I32) { { VAR <d>value I32 := 2; RETURN <r>value; } }", LOCAL);
        assertResolution("::run FUNCTION(@<d>value I32) { { VAR value I32 := 2; } RETURN <r>value; }", ARGUMENT);
    }

    @Test void localVisibilityStartsAfterInitializer() {
        assertResolution("::<d>value VAR I32; ::run FUNCTION() { VAR value I32 := <r>value; }", GLOBAL);
        assertResolution("::<d>value VAR I32; ::run FUNCTION() { print(<r>value); VAR value I32 := 1; }", GLOBAL);
    }

    @Test void parametersDoNotLeakBetweenFunctions() {
        assertUnresolved("::first FUNCTION(@value I32) { RETURN value; } ::second FUNCTION() { RETURN <r>value; }");
        assertResolution("::run FUNCTION() { RETURN <r>later; } ::<d>later VAR I32;", GLOBAL);
    }

    @Test void aliasesAndPositionalPacksBindTheirLocalNames() {
        assertResolution("::run FUNCTION(@OTHER:<d>local I32, %other I32) { RETURN <r>local; }", ARGUMENT);
        assertResolution("::run FUNCTION(%<d>other I32) { RETURN <r>other; }", ARGUMENT);
        assertResolution("::run FUNCTION(%...<d>values I32) { RETURN PACK_SIZE(<r>values); }", ARGUMENT);
        assertUnresolved("::run FUNCTION(@OTHER:local I32) { RETURN <r>OTHER; }");
    }

    @Test void namedCallArgumentsAreNotDeclarations() {
        assertUnresolved("::run FUNCTION() { other(@value 1); RETURN <r>value; }");
    }

    @Test void procedureSignaturesDoNotPublishOuterBindings() {
        assertUnresolved("::run FUNCTION(@callback PROCEDURE(@value I32: I32)) { RETURN <r>value; }");
        assertUnresolved("::run FUNCTION() { VAR callback PROCEDURE(@value I32: I32); RETURN <r>value; }");
    }

    @Test void namespaceQualificationAndReopening() {
        assertResolution("::outer NAMESPACE { ::inner NAMESPACE { ::<d>value VAR I32; } } ::run FUNCTION() { RETURN outer::inner::<r>value; }", GLOBAL);
        assertResolution("::outer NAMESPACE { ::<d>value VAR I32; } ::outer NAMESPACE { ::run FUNCTION() { RETURN <r>value; } }", GLOBAL);
        assertUnresolved("::outer NAMESPACE { ::value VAR I32; } ::run FUNCTION() { RETURN missing::<r>value; }");
    }

    @Test void absoluteNamesBypassLocalBindings() {
        assertResolution("::<d>value VAR I32; ::run FUNCTION(@value I32) { RETURN ::<r>value; }", GLOBAL);
    }

    @Test void fieldsResolveForKnownInstances() {
        assertResolution("::counter STRUCT { .<d>increment FUNCTION() {} } ::run FUNCTION(@item counter) { item.<r>increment(); }", MEMBER_FUNCTION);
        assertResolution("::counter STRUCT { ::<d>create FUNCTION() {} } ::run FUNCTION() { counter::<r>create(); }", FREE_FUNCTION);
        assertResolution("::counter STRUCT { .<d>value VAR I32; .run FUNCTION() MUT { RETURN .<r>value; } }", FIELD);
        assertResolution("::counter STRUCT { .<d>value VAR I32; } ::run FUNCTION(@item counter) { RETURN item.<r>value; }", FIELD);
        assertResolution("::counter STRUCT { .<d>value VAR I32; } ::run FUNCTION() { VAR item counter; RETURN item.<r>value; }", FIELD);
        assertUnresolved("::counter STRUCT { .value VAR I32; .run FUNCTION() { RETURN unknown.<r>value; } }");
        assertUnresolved("::counter STRUCT { .value VAR I32; .run FUNCTION() { RETURN create().<r>value; } }");
    }

    @Test void templatesKeepKnownParametersButDoNotGuessSelections() {
        assertResolution("::box TEMPLATE(@<d>T TYPE AUTO) STRUCT { .item VAR <r>T; }", TEMPLATE_PARAMETER);
        assertUnresolved("::box TEMPLATE(@T TYPE AUTO) STRUCT { .item VAR T; } ::run FUNCTION(@b box#(I32)) { RETURN b.<r>item; }");
        assertUnresolved("::run TEMPLATE(@T TYPE AUTO) FUNCTION() { RETURN T::<r>member; }");
    }

    @Test void overloadsAndAliasesStayUnresolved() {
        assertUnresolved("::pick FUNCTION(@value I32) {} ::pick FUNCTION(@value I64) {} ::run FUNCTION() { <r>pick(1); }");
        assertUnresolved("::value ALIAS external; ::run FUNCTION() { RETURN <r>value; }");
    }

    @Test void lambdaParametersAreNested() {
        assertResolution("::run FUNCTION(@value I32) { VAR fn AUTO := -<(@<d>value I64) { RETURN <r>value; }; }", ARGUMENT);
        assertResolution("::run FUNCTION(@<d>value I32) { VAR fn AUTO := -<(@value I64) { RETURN value; }; RETURN <r>value; }", ARGUMENT);
    }

    @Test void commentsAndDocumentationDoNotIntroduceDeclarations() {
        assertResolution("::<d>value DOC <$ @fake I32; ::value STRUCT {} $> VAR I32; // @fake\n ::run FUNCTION() { RETURN <r>value; }", GLOBAL);
    }

    @Test void callColorIsLimitedToBalancedCallParentheses() {
        String source = "::call FUNCTION(@arg I32) {} ::run FUNCTION() { IF (TRUE) { call(1); missing(call(2)); } }";
        QuxlangSymbols symbols = new QuxlangSymbols(source);
        int call = source.indexOf("call(1)");
        assertEquals(FREE_FUNCTION, symbols.bindingAt(call).kind());
        assertEquals(CALL_PAREN, symbols.bindingAt(call + 4).kind());
        assertEquals(CALL_PAREN, symbols.bindingAt(call + 6).kind());
        assertNull(symbols.bindingAt(source.indexOf("FUNCTION(") + 8));
        assertNull(symbols.bindingAt(source.indexOf("IF (") + 3));
        int unknown = source.indexOf("missing(");
        assertNull(symbols.bindingAt(unknown));
        assertEquals(CALL_PAREN, symbols.bindingAt(unknown + 7).kind());
    }

    @Test void templateArgumentParenthesesKeepTheirOrdinaryColor() {
        String source = "::run FUNCTION() { factory#(I32)(1); }";
        QuxlangSymbols symbols = new QuxlangSymbols(source);
        assertNull(symbols.bindingAt(source.indexOf("#(") + 1));
        assertEquals(CALL_PAREN, symbols.bindingAt(source.indexOf(")(1)") + 1).kind());
    }

    @Test void dereferencedUnknownReceiversDoNotResolveAsThis() {
        assertUnresolved("::counter STRUCT { .value VAR I32; .run FUNCTION() { RETURN unknown->.<r>value; } }");
        assertResolution("::counter STRUCT { .<d>value VAR I32; .run FUNCTION(@p ->counter) { RETURN p->.<r>value; } }", FIELD);
    }

    @Test void privateGroupsRetainTheirOwner() {
        assertResolution("::counter STRUCT { PRIVATE(MODULE) { .<d>value VAR I32; } .run FUNCTION() { RETURN .<r>value; } }", FIELD);
        assertResolution("::counter STRUCT { PRIVATE(CLASS) .<d>value VAR I32; PRIVATE(CLASS) .run FUNCTION() { RETURN .<r>value; } }", FIELD);
        assertResolution("::counter STRUCT { PRIVATE(CLASS) .run FUNCTION(@<d>value I32) { RETURN <r>value; } }", ARGUMENT);
    }

    @Test void groupingParenthesesAreNotCalls() {
        String source = "::run FUNCTION() { RETURN (1 + 2); IF LIKELY (TRUE) {} }";
        QuxlangSymbols symbols = new QuxlangSymbols(source);
        assertNull(symbols.bindingAt(source.indexOf("RETURN (") + 7));
        assertNull(symbols.bindingAt(source.indexOf("LIKELY (") + 7));
    }

    @Test void conditionalDeclarationsAndCapturesRequireMoreInformation() {
        assertUnresolved("::pick INCLUDE_IF(TARGET) FUNCTION() {} ::run FUNCTION() { <r>pick(); }");
        assertUnresolved("::pick FUNCTION() ENABLE_IF(TARGET) {} ::run FUNCTION() { <r>pick(); }");
        assertUnresolved("::value VAR I32; ::run FUNCTION(@value I32) { VAR f AUTO := -< { RETURN <r>value; }; }");
    }

    @Test void castsPreserveTypeLookup() {
        assertResolution("::<d>constructor_conversion_choice STRUCT {} ::run FUNCTION() { VAR x AUTO := 1 AS <r>constructor_conversion_choice; }", TYPE);
        assertResolution("::<d>constructor_conversion_choice STRUCT {} ::run FUNCTION() { VAR x AUTO := 1 AS constructor_conversion_choice; <r>constructor_conversion_choice(1); }", TYPE);
    }

    @Test void loopBindingsShadowOnlyWithinTheirLoop() {
        for (String clause : new String[]{"VALUE", "ITEM", "INDEX", "ITER"}) {
            assertResolution("::value VAR I32; ::run FUNCTION() { LOOP " + clause + "(<d>value) IN(items) DO { RETURN <r>value; } }", LOCAL);
            assertResolution("::<d>value VAR I32; ::run FUNCTION() { LOOP " + clause + "(value) IN(<r>value) DO {} }", GLOBAL);
            assertResolution("::<d>value VAR I32; ::run FUNCTION() { LOOP " + clause + "(value) IN(items) DO {} RETURN <r>value; }", GLOBAL);
            assertResolution("::run FUNCTION() { LOOP " + clause + "(<d>value) IN(items) FILTER(<r>value) DO {} }", LOCAL);
        }
        assertResolution("::run FUNCTION() { LOOP VALUE(<d>value) FROM(0) UNTIL(2) DO { LOOP VALUE(value) FROM(0) UNTIL(2) DO {} RETURN <r>value; } }", LOCAL);
        assertResolution("::run FUNCTION() { LOOP VALUE(value) FROM(0) UNTIL(2) DO { LOOP VALUE(<d>value) FROM(0) UNTIL(2) DO { RETURN <r>value; } } }", LOCAL);
        assertUnresolved("::run FUNCTION() { LOOP VALUE(value) FROM(0) UNTIL(2) DO {} RETURN <r>value; }");
        assertResolution("::run FUNCTION() { LOOP ITER(cursor) INDEX(index) ITEM(<d>item) IN(items) DO { RETURN <r>item; } }", LOCAL);
        assertResolution("::run FUNCTION() { LOOP ITER(<d>cursor) INIT { prepare(); } START(0) STEP { advance(<r>cursor); } DO {} }", LOCAL);
    }

    @Test void unfinishedSyntaxIsTolerated() {
        for (String source : new String[]{"", "::", "::f FUNCTION(@x I32", "::f FUNCTION() { VAR x I32 :=", "-<(@x I32) {", "{ [ ( ] }", "::f FUNCTION() { LOOP VALUE(x", "::f FUNCTION() { LOOP VALUE(x) DO {"}) {
            assertDoesNotThrow(() -> new QuxlangSymbols(source), source);
        }
    }
}
