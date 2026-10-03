# Code style

Keep this codebase concise and easy to review.

- Avoid large, dense blocks of code. Separate logical stages with blank lines.
- Prefer blank lines and a few useful comments within existing functions. Do not add helper functions merely to improve formatting.
- Use braces for control flow. Avoid compressing several operations onto one line.
- Wrap long declarations, calls, and conditions so their parts are easy to scan.
- Add short comments for non-obvious byte layouts, ownership rules, and invariants. Do not narrate every line.
- Explain complex parser stages and how state structs fit together, especially across asynchronous responses.
- Improve formatting in code you touch; avoid unrelated project-wide formatting changes.
