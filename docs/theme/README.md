# VoyagerOS64 documentation theme

VoyagerOS64 0.0.5 keeps its documentation theme in-tree so the generated HTML and PDF reference manuals do not depend on an external CDN or theme checkout.

- `voyager-dark.css` is loaded by Doxygen through `HTML_EXTRA_STYLESHEET` and provides the dark HTML presentation.
- `voyager-dark.sty` is loaded through `LATEX_EXTRA_STYLESHEET` and applies the matching low-glare palette to the generated LaTeX/PDF manual.

The source of truth remains `Doxygen-0.0.5` plus the Doxygen comments in `src/`. The generated `html/` and `latex/` trees are disposable build output and should not be edited directly.

Regenerate both documentation trees from the repository root:

```sh
doxygen Doxygen-0.0.5
```

Build the PDF:

```sh
cd latex
make
```

The resulting manual is `latex/refman.pdf`.

The PDF theme intentionally uses a near-black page (`#0d1117`) and softened foreground text rather than pure black/white. Hyperlinks and section headings use the same blue/cyan accent family as the HTML reference. This is primarily an on-screen reference-manual theme; printing a dark PDF will naturally consume much more ink/toner than a conventional light document.
