# Fix: Pandoc input quoting in `src/main.c`

**Symptom (CI):**
```
error: expected ')' before 'workspace'
  "workspace/book-draft.md",
```

**Cause:** the input file token was placed **outside** the `snprintf` format string.

**Correct form:**
```c
snprintf(cmd1, sizeof(cmd1),
  "pandoc ... "
  "-o "%s" "
  ""workspace/book-draft.md"",
  book_title, author, extra_args, out_html);
```

Apply the patch or run the fixer, then build again.
