# Defense deck — condensed test version (`_v2`)

`presentacion_defensa_v2.tex` is a **trial restructure** of `presentacion_defensa.tex`
aimed at the **15–20 min** window. Same theme, same `imagenes/`, same facts — the
content was **cut, merged, and re-emphasised**, not rewritten. The original file is
untouched so you can compare side by side.

- **Build:** `pdflatex presentacion_defensa_v2.tex` (twice). Compiles clean; 36 pages
  (15 content frames + título + índice + 4 section dividers + gracias, then 13 backup).
- **Original:** ~28 content frames (≈30–40 min). **v2:** 15 content frames (≈18 min).

## What changed

**Added**
- *Motivación*: precise **quantum** bullet — Shor breaks **RSA *and* ECC equally**,
  needs hardware that doesn't exist yet, transition is **hybrid** (consistent with the
  closing slide; avoids the "but Shor breaks ECC too" trap).
- *Objetivo*: explicit **Aportación** block (separate algebra from engineering).
- Results slide **titles now state the finding** (e.g. "ECC genera claves ~138× más
  rápido"), so the headline lands before the chart.

**Merged (2→1 frame each)**
- Ley de grupo + estructura/Hasse → one frame.
- Afines/Jacobianas + "por qué eficientes" → one frame (sets up Eje 2).
- Mult. escalar + ECDLP + tabla NIST de tamaños de clave → one frame.
- ECDH + ECDSA → one high-level frame (nonce warning kept).
- Metodología + compiler-flags finding → one frame.
- Eje 3 (primos vs binarios) + validación OpenSSL → one frame ("artefacto de software").
- Conclusiones + limitaciones → one frame.
- ECC hoy + trabajo futuro + horizonte post-cuántico → one frame.

**Moved to `\appendix` (backup, answer-on-demand)**
- Derivación general→corta de Weierstrass · curvas binarias en detalle · fórmulas
  explícitas Fₚ/F₂ᵐ · rho de Pollard (detalle + figura) · parámetros del dominio ·
  ECDH (diagrama tikz) y ECDSA completos · prueba de corrección ECDSA · tabla OpenSSL ·
  CRT · asociatividad · tabla-resumen de tiempos · ratios de rendimiento.

## Approx. timing budget (~18 min)

| Section | Frames | min |
|---|---|---|
| Motivación + objetivo | 2 | 2.5 |
| Fundamentos | 4 | 5.5 |
| Protocolos + implementación + metodología | 3 | 4 |
| Resultados (3 ejes) | 4 | 5 |
| Conclusiones + ECC hoy | 2 | 3 |

## Open polish items (optional)

- Curve slide (`¿Qué es una curva elíptica?`) sits ~28pt close to the bottom margin —
  renders fine, but trim a line if you want more breathing room.
- Decide whether to keep the índice frame (saves ~20 s if cut).
- If you prefer this version, rename it to `presentacion_defensa.tex` and archive the long one.
