import sys
from clang.cindex import Index, CursorKind

def traverse(cursor, level=0):
    if cursor.kind == CursorKind.CLASS_DECL or cursor.kind == CursorKind.STRUCT_DECL:
        print("  " * level + f"Class: {cursor.spelling}")
    elif cursor.kind == CursorKind.FIELD_DECL:
        print("  " * level + f"Field: {cursor.spelling} (Type: {cursor.type.spelling})")
        # try to get tokens for the initializer
        tokens = list(cursor.get_tokens())
        if tokens:
            print("  " * (level+1) + "Tokens: " + " ".join(t.spelling for t in tokens))
            
    for child in cursor.get_children():
        traverse(child, level + 1)

index = Index.create()
tu = index.parse("firmware/application/ui_record_view.hpp", args=['-x', 'c++', '-std=c++17', '-I./firmware/application', '-I./firmware/common'])
traverse(tu.cursor)
