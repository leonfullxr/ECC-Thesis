# Building the LaTeX Project

The thesis source is in `Plantilla_TFG_latex/`. The main file is `proyecto.tex`.

## Recommended: latexmk (automatic multi-pass)

`latexmk` handles the full compilation sequence automatically — no need to think about ordering or how many passes are needed.

```bash
cd docs/Plantilla_TFG_latex
latexmk -pdf -cd proyecto.tex
```

To do a clean rebuild from scratch (fixes any broken state):

```bash
latexmk -C proyecto.tex          # deletes all generated files
latexmk -pdf -cd proyecto.tex    # full rebuild
```

## Manual compilation order

If you need to run the steps manually, the order matters. Always run `pdflatex` **before** `bibtex`:

```bash
cd docs/Plantilla_TFG_latex
pdflatex proyecto.tex    # pass 1: writes citation keys to .aux
bibtex proyecto          # reads .aux, builds .bbl from .bib
pdflatex proyecto.tex    # pass 2: incorporates bibliography
pdflatex proyecto.tex    # pass 3: resolves remaining cross-references
```

### Why this order?

| Step | Reads | Writes | Purpose |
|------|-------|--------|---------|
| `pdflatex` (1) | `.tex` | `.aux`, `.toc`, `.lof`, `.lot` | Collects all `\cite{}` keys and cross-references |
| `bibtex` | `.aux`, `.bib` | `.bbl` | Formats only the cited bibliography entries |
| `pdflatex` (2) | `.tex`, `.bbl` | `.aux` | Inserts bibliography into document |
| `pdflatex` (3) | `.tex`, `.aux` | `.pdf` | Resolves TOC page numbers, `\ref{}` labels, etc. |

Running `bibtex` before `pdflatex` means bibtex reads a stale `.aux` and builds an incomplete `.bbl`, causing `Citation '...' undefined` warnings and a broken bibliography.

## VS Code (LaTeX Workshop extension)

The workspace is configured to use `latexmk` automatically. Trigger a build with `Ctrl+Alt+B` or by saving a `.tex` file.

If the build produces a broken PDF, do a clean rebuild:

1. Open the Command Palette (`Ctrl+Shift+P`)
2. Run **LaTeX Workshop: Clean up auxiliary files**
3. Then **LaTeX Workshop: Build LaTeX project**

## Output files

Build artifacts (`.aux`, `.bbl`, `.log`, `.toc`, etc.) are gitignored. The only files that should be committed are `.tex`, `.bib`, and image/figure assets.
