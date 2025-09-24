# Why the error happens

When the input path token appears *outside* the C string literal:

```
... "-o \"%s\" "
\"workspace/book-draft.md\",
```

the compiler sees a raw backslash-escaped quote that doesn't belong to a string,
triggering **`stray '\' in program`**.

**Correct form (inside the string):**
```
... "-o \"%s\" "
\"workspace/book-draft.md\",
```

or, more readably:
```
"-o \"%s\" "
"\"workspace/book-draft.md\",
```

Use the patch or the fixer scripts to rewrite it.
