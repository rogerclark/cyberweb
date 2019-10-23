#pragma once

enum HtmlElementType {
	HTML_ELEMENT_TYPE_NONE = 0,
	HTML_ELEMENT_TYPE_ROOT,
    HTML_ELEMENT_TYPE_TEXT,
	HTML_ELEMENT_TYPE_HTML,
	HTML_ELEMENT_TYPE_HEAD,
	HTML_ELEMENT_TYPE_TITLE,
    HTML_ELEMENT_TYPE_BODY,
	HTML_ELEMENT_TYPE_H1,
	HTML_ELEMENT_TYPE_IMG,
	HTML_ELEMENT_TYPE_P,
	HTML_ELEMENT_TYPE_B,
	HTML_ELEMENT_TYPE_A
};

struct HtmlElement;
typedef RefPtr<HtmlElement> HtmlElementPtr;

struct HtmlElement {
	HtmlElementType type;
	StringTable attributes;
	Vector<HtmlElementPtr> children;
    String text;

    ~HtmlElement() {
        OutputDebugStringA("destroying");
    }

    static Range TypeToString(HtmlElementType type) {
        Range result;
        switch (type) {
            case HTML_ELEMENT_TYPE_NONE: result = RS("none"); break;
            case HTML_ELEMENT_TYPE_ROOT: result = RS("(root node)"); break;
            case HTML_ELEMENT_TYPE_TEXT: result = RS("(text node)"); break;
            case HTML_ELEMENT_TYPE_HTML: result = RS("HTML"); break;
            case HTML_ELEMENT_TYPE_HEAD: result = RS("HEAD"); break;
            case HTML_ELEMENT_TYPE_TITLE: result = RS("TITLE"); break;
            case HTML_ELEMENT_TYPE_BODY: result = RS("BODY"); break;
            case HTML_ELEMENT_TYPE_H1: result = RS("H1"); break;
            case HTML_ELEMENT_TYPE_IMG: result = RS("IMG"); break;
            case HTML_ELEMENT_TYPE_P: result = RS("P"); break;
            case HTML_ELEMENT_TYPE_B: result = RS("B"); break;
            case HTML_ELEMENT_TYPE_A: result = RS("A"); break;
            default:
                CWASSERT("unknown type" != 0);
                break;
        }
        return result;
    }

    BOOL CreateWithType(HtmlElementType newType) {
        BOOL success = FALSE;
        type = newType;
        if (attributes.CreateWithCapacity(8)) {
            if (children.CreateWithCapacity(8)) {
                success = TRUE;
            }
        }
        return success;
    }

    void PrintSpaces(COUNT indentLevel) {
        for (COUNT i = 0; i < indentLevel; i++) {
            OutputDebugStringA("  ");
        }
    }

    void Print(COUNT indentLevel) {
        enum { DEBUG_BUFFER_MAX = 256 };
        char debugBuffer[DEBUG_BUFFER_MAX];
        Range typeString = HtmlElement::TypeToString(type);
        const char* innerText = text.Buffer != NULL ? text.Buffer : "(none)";

        PrintSpaces(indentLevel);
        wsprintfA(debugBuffer, "Node type: %s\r\n", typeString);
        OutputDebugStringA(debugBuffer);

        PrintSpaces(indentLevel);
        wsprintfA(debugBuffer, "Number of children: %d\r\n", children.Length);
        OutputDebugStringA(debugBuffer);

        PrintSpaces(indentLevel);
        wsprintfA(debugBuffer, "Number of attributes: %d\r\n", attributes.Table.Length);
        OutputDebugStringA(debugBuffer);

        for (COUNT a = 0; a < attributes.Table.Length; a++) {
            PrintSpaces(indentLevel);
            StringTable::StringPair* pair = attributes.Table.Items[a];
            wsprintfA(debugBuffer, "attribute %s: %s\r\n",
                pair->Key.ToRange().Start, pair->Value.ToRange().Start);
            OutputDebugStringA(debugBuffer);
        }

        PrintSpaces(indentLevel);
        wsprintfA(debugBuffer, "Text: %s\r\n", innerText);
        OutputDebugStringA(debugBuffer);

        for (COUNT i = 0; i < children.Length; i++) {
            HtmlElementPtr elem = children.Items[i];
            elem->Print(indentLevel++);
        }
    }
};
