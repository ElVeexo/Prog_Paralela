# Planificación: Conversión de PDF a LaTeX y Traducción al Español

## 1. Análisis del PDF objetivo

Se realizó un análisis automatizado del archivo `Lectura_2_.pdf` utilizando PyMuPDF. Estos son los hallazgos:

| Propiedad | Valor |
|---|---|
| **Archivo** | `Lectura_2_.pdf` |
| **Tamaño** | ~360 KB |
| **Páginas** | 8 |
| **Formato PDF** | PDF 1.5 |
| **Creador** | `TeX` / `pdfTeX-1.40.14` |
| **Fecha de creación** | 2014-08-06 |
| **Tipo de contenido** | Texto real (NO escaneado) |
| **Imágenes rasterizadas** | 0 (las figuras son gráficos vectoriales) |
| **Caracteres totales** | 36,443 |
| **Caracteres/página** | ~4,555 |
| **Símbolos matemáticos** | 151 |
| **Citas bibliográficas `[N]`** | 57 |
| **Secciones detectadas** | 8 (numeración romana: I–VI+) |
| **Idioma** | Inglés |
| **Tamaño de página** | 612 × 792 pts (carta US) |

### Contenido del documento

- **Título**: *"GPU maps for the space of computation in triangular domain problems"*
- **Autores**: Cristobal A. Navarro, Nancy Hitschfeld (Universidad de Chile)
- **Tipo**: Paper académico estilo IEEE, 2 columnas
- **Tema**: Estrategias de mapeo en GPU para problemas de dominio triangular (computación paralela)

### Elementos estructurales identificados

| Elemento | Presente | Observaciones |
|---|---|---|
| Formato 2 columnas | ✅ | Estilo IEEE conference |
| Título y autores | ✅ | Con afiliaciones y emails |
| Abstract | ✅ | Precedido por "Abstract—" |
| Secciones numeradas (I–VI) | ✅ | Numeración romana |
| Subsecciones | ✅ | Por confirmar profundidad |
| Fórmulas matemáticas | ✅ | ~151 símbolos, ecuaciones numeradas |
| Variables griegas | ✅ | α, β, τ, λ, etc. |
| Subíndices/superíndices | ✅ | Frecuentes en fórmulas |
| Tablas | ⚠️ | Posibles, por confirmar |
| Figuras | ✅ | Referenciadas (Fig. 1, 2, 3...), vectoriales |
| Gráficos con datos | ✅ | Gráficos de rendimiento con múltiples series |
| Referencias bibliográficas | ✅ | 18+ referencias, formato `[N]` |
| Abreviaciones técnicas | ✅ | BB, UTM, LTM, REC, RB, EDM, GPU, etc. |

> **HALLAZGO CLAVE**: El PDF fue generado originalmente desde LaTeX (`pdfTeX-1.40.14`). Esto significa que el texto extraído será de alta calidad y la reconstrucción en LaTeX será más precisa que con un PDF de otro origen.

---

## 2. Evaluación de viabilidad

### Veredicto: ✅ VIABLE con alta precisión

El flujo de trabajo propuesto es **completamente viable** y se espera una precisión alta por las siguientes razones:

1. **El PDF es texto real** (no escaneado) → No se necesita OCR
2. **Fue generado por pdfTeX** → La estructura del texto es limpia y predecible
3. **0 imágenes rasterizadas** → Las figuras son vectoriales, se pueden extraer o referenciar
4. **Estructura clara** → Secciones con numeración romana, formato IEEE estándar
5. **Herramientas maduras disponibles** → PyMuPDF, Pandoc, pdflatex ya en el sistema

### Nivel de precisión esperado por componente

| Componente | Precisión esperada | Riesgo | Notas |
|---|---|---|---|
| Texto plano | 🟢 95–99% | Bajo | Extracción directa, formato limpio |
| Orden de lectura (2 cols) | 🟡 85–95% | Medio | Riesgo de mezcla en zonas de transición columna↔ancho completo |
| Secciones y subsecciones | 🟢 95–99% | Bajo | Numeración romana clara |
| Fórmulas inline | 🟡 80–90% | Medio | Variables y símbolos pueden perder formato |
| Fórmulas en bloque | 🟡 75–90% | Medio-Alto | Ecuaciones multi-línea y alineaciones son complejas |
| Tablas | 🟡 80–90% | Medio | Depende de la complejidad |
| Figuras | 🔴 60–75% | Alto | Gráficos vectoriales difíciles de extraer; se referenciarán |
| Referencias/citas | 🟢 90–95% | Bajo | Formato `[N]` estándar |
| Compilación LaTeX | 🟡 85–95% | Medio | Puede requerir ajustes manuales |
| Traducción al español | 🟢 90–98% | Bajo | Solo texto natural; fórmulas intactas |

### Limitaciones conocidas

> **⚠️ ADVERTENCIA — Limitaciones que afectarán el resultado:**

1. **Figuras vectoriales**: Los gráficos de rendimiento (Fig. 3, etc.) son vectoriales y no pueden extraerse trivialmente como imágenes. Se incluirán como placeholders con `\includegraphics` apuntando a las figuras extraídas como imágenes del PDF.

2. **Fórmulas complejas**: PyMuPDF extrae texto plano, no LaTeX. Fórmulas como `n′ = ⌈√(n(n+1)/2)⌉` se extraen como texto Unicode y deben reconstruirse manualmente en LaTeX (`n' = \lceil\sqrt{n(n+1)/2}\rceil`).

3. **Orden de columnas**: En zonas donde el texto transiciona entre 1 y 2 columnas (ej: abstract, figuras de ancho completo), la extracción puede mezclar columnas.

4. **Formato exacto**: No se puede replicar el CSS/estilo visual exacto del IEEE (márgenes, fuentes, espaciado), pero sí la estructura lógica.

5. **Ligaduras y caracteres especiales**: Algunos caracteres como `ﬁ`, `ﬂ` pueden extraerse como ligaduras y necesitar corrección.

---

## 3. Herramientas necesarias

### Ya disponibles en el sistema (WSL)

| Herramienta | Versión | Uso |
|---|---|---|
| Python 3 | 3.10.12 | Scripting y procesamiento |
| pip | 22.0.2 | Gestión de paquetes Python |
| PyMuPDF (fitz) | 1.27.2 | Extracción de texto y análisis de PDF |
| Pillow | 11.3.0 | Procesamiento de imágenes |
| pdflatex | TeX Live 2022 | Compilación de LaTeX |
| texlive-latex-extra | 2021 | Paquetes LaTeX adicionales |
| Pandoc | 2.9.2.1 | Conversión de formatos |

### A instalar (pip install en WSL)

| Herramienta | Comando | Propósito | Justificación |
|---|---|---|---|
| **pdfplumber** | `pip3 install pdfplumber` | Extracción de tablas del PDF | Especializada en detección y reconstrucción de tablas; PyMuPDF no tiene esta capacidad nativa |
| **marker-pdf** | `pip3 install marker-pdf` | Conversión PDF→Markdown con LaTeX | Pipeline moderna (2024-2026) que maneja ecuaciones, columnas y tablas con alta fidelidad; 10x más rápida que Nougat |

> **NOTA sobre marker-pdf**: Es la herramienta más recomendada actualmente para conversión de PDFs académicos. Usa Surya (OCR moderno) internamente, detecta columnas automáticamente, y preserva ecuaciones en formato LaTeX. Es open-source, mantenida activamente, y ampliamente usada en la comunidad académica.

### Herramientas descartadas

| Herramienta | Razón de descarte |
|---|---|
| **Tesseract/OCR** | No necesario: el PDF tiene texto real |
| **Nougat** | Abandonado/sin mantenimiento; propenso a alucinaciones |
| **MinerU** | Dependencias pesadas (`uv` + modelos); sobredimensionado para este caso |
| **Mathpix** | API comercial de pago; no necesaria para este volumen |

### Herramientas opcionales (según necesidad)

| Herramienta | Cuándo instalar | Comando |
|---|---|---|
| **latexmk** | Si se necesita automatizar múltiples pasadas de compilación | `sudo apt install latexmk` |
| **bibtex/biber** | Si las referencias necesitan compilación BibTeX | Ya incluido en texlive |

---

## 4. Pipeline de trabajo: 5 fases

### Fase 1: Análisis del PDF ✅ (COMPLETADA)

> **Estado**: Ya realizada durante la planificación.

- [x] Detectar si el PDF es texto real o escaneado → **Texto real**
- [x] Identificar número de páginas → **8 páginas**
- [x] Identificar creador → **pdfTeX (originalmente LaTeX)**
- [x] Contar símbolos matemáticos → **151**
- [x] Contar citas bibliográficas → **57 citas `[N]`**
- [x] Identificar estructura de secciones → **8 secciones (I–VI+)**
- [x] Verificar presencia de imágenes → **0 rasterizadas, figuras vectoriales**

---

### Fase 2: Extracción y reconstrucción en LaTeX original

**Objetivo**: Obtener un archivo `.tex` que represente fielmente el contenido del PDF.

#### Paso 2.1 — Extracción con marker-pdf
```bash
# Instalar marker-pdf
pip3 install marker-pdf

# Ejecutar conversión
marker_single "/mnt/c/Users/vicen/OneDrive/Documentos/Cosas Varias/Antigravity/Lectura_2_.pdf" \
  --output_dir "/mnt/c/Users/vicen/OneDrive/Documentos/Cosas Varias/Antigravity/output_marker"
```
- Genera Markdown con ecuaciones LaTeX embebidas
- Maneja automáticamente el orden de lectura en 2 columnas
- Preserva estructura de secciones

#### Paso 2.2 — Extracción de tablas con pdfplumber
```bash
pip3 install pdfplumber
```
- Script Python para detectar y extraer tablas
- Convertir a formato `tabular` de LaTeX

#### Paso 2.3 — Extracción de figuras como imágenes
```python
# Renderizar cada página y recortar regiones de figuras
import fitz
doc = fitz.open("Lectura_2_.pdf")
for page in doc:
    pix = page.get_pixmap(dpi=300)
    # Guardar como PNG para referencia
```
- Extraer figuras como imágenes PNG de alta resolución
- Nombrarlas según su referencia en el texto (fig1.png, fig2.png, etc.)

#### Paso 2.4 — Construcción del archivo .tex

Estructura del archivo LaTeX resultante:

```latex
\documentclass[conference]{IEEEtran}
\usepackage[utf8]{inputenc}
\usepackage{amsmath, amssymb, amsfonts}
\usepackage{graphicx}
\usepackage{cite}
% ... paquetes adicionales según necesidad

\title{GPU maps for the space of computation in triangular domain problems}
\author{Cristobal A. Navarro \and Nancy Hitschfeld}

\begin{document}
\maketitle

\begin{abstract}
...
\end{abstract}

\section{Introduction}  % I.
...
\section{Related Work}   % II.
...
% ... demás secciones

\begin{thebibliography}{18}
\bibitem{ref1} ...
\end{thebibliography}

\end{document}
```

#### Paso 2.5 — Reconstrucción manual de fórmulas

- Comparar el texto extraído con el PDF original
- Reconstruir cada fórmula en LaTeX, por ejemplo:
  - Texto extraído: `n′ = ⌈√(n(n + 1)/2)⌉`
  - LaTeX correcto: `$n' = \lceil\sqrt{n(n+1)/2}\rceil$`
- Ecuaciones numeradas con entorno `equation`
- Alineaciones con entorno `align`

**Archivos resultantes**:
- `Lectura_2_original.tex` — archivo LaTeX principal
- `figures/` — carpeta con figuras extraídas
- Posiblemente `references.bib` — si se decide usar BibTeX

---

### Fase 3: Revisión y corrección del LaTeX original

**Objetivo**: Verificar que el LaTeX generado es fiel al PDF.

#### Paso 3.1 — Compilación inicial
```bash
pdflatex Lectura_2_original.tex
# Ejecutar 2 veces para resolver referencias cruzadas
pdflatex Lectura_2_original.tex
```

#### Paso 3.2 — Revisión visual comparativa
- Compilar el `.tex` a PDF
- Comparar lado a lado con el PDF original
- Checklist de verificación:

| Elemento | Verificación |
|---|---|
| Orden de lectura | ¿Los párrafos siguen el orden correcto? |
| Columnas | ¿Se mezcló contenido entre columnas? |
| Fórmulas inline | ¿Variables y símbolos son correctos? |
| Fórmulas en bloque | ¿Ecuaciones numeradas coinciden? |
| Tablas | ¿Datos y estructura son correctos? |
| Figuras | ¿Se referencian correctamente? |
| Referencias | ¿Las citas `[N]` son consistentes? |
| Abreviaciones | ¿BB, UTM, LTM, GPU, etc. están intactas? |

#### Paso 3.3 — Corrección de errores
- Corregir mezcla de columnas si la hay
- Ajustar fórmulas que no coincidan con el original
- Verificar que las figuras están en la posición correcta
- Corregir ligaduras rotas (`ﬁ` → `fi`, `ﬂ` → `fl`)

---

### Fase 4: Traducción al español

**Objetivo**: Generar `Lectura_2_espanol.tex` traduciendo solo texto natural.

#### Reglas de traducción

| Tipo de contenido | ¿Traducir? | Ejemplo |
|---|---|---|
| Texto narrativo | ✅ Sí | "In this work, we study..." → "En este trabajo, estudiamos..." |
| Títulos de secciones | ✅ Sí | "Introduction" → "Introducción" |
| Abstract | ✅ Sí | Traducir contenido |
| Fórmulas matemáticas | ❌ No | `$n' = \lceil\sqrt{n(n+1)/2}\rceil$` → sin cambios |
| Variables | ❌ No | `$\beta$`, `$\tau$`, `$n$` → sin cambios |
| Nombres de métodos | ❌ No | "bounding box (BB)", "LTM", "UTM" → sin cambios |
| Nombres de datasets | ❌ No | Mantener en inglés |
| Nombres de algoritmos | ❌ No | "Newton-Raphson" → sin cambios |
| Métricas | ❌ No | "speedup", "throughput" → sin cambios |
| Abreviaciones técnicas | ❌ No | GPU, BB, LTM, EDM → sin cambios |
| Comandos LaTeX | ❌ No | `\section{}`, `\ref{}`, `\cite{}` → sin cambios |
| Etiquetas y labels | ❌ No | `\label{eq:1}` → sin cambios |
| Referencias bibliográficas | ❌ No | Mantener en idioma original |
| Nombres propios | ❌ No | "Navarro", "Hitschfeld" → sin cambios |
| Nombres de hardware | ❌ No | "GTX680", "Tesla K40" → sin cambios |

#### Paso 4.1 — Duplicar el archivo LaTeX
```bash
cp Lectura_2_original.tex Lectura_2_espanol.tex
```

#### Paso 4.2 — Traducción asistida por IA
- Procesar sección por sección
- Identificar texto natural vs. contenido técnico
- Traducir únicamente el texto natural
- Marcar con `% TODO: REVISAR` cualquier caso ambiguo

#### Paso 4.3 — Ajustes de idioma en LaTeX
```latex
\usepackage[spanish]{babel}      % Soporte para español
\usepackage[utf8]{inputenc}       % Acentos y ñ
```
- Agregar paquete `babel` para español
- Verificar que los acentos se renderizan correctamente
- Ajustar comillas y puntuación si es necesario

---

### Fase 5: Compilación y verificación final

#### Paso 5.1 — Compilar versión en español
```bash
pdflatex Lectura_2_espanol.tex
pdflatex Lectura_2_espanol.tex  # Segunda pasada
```

#### Paso 5.2 — Corregir errores de compilación
- Errores comunes esperados:
  - Conflictos de `babel` con paquetes matemáticos
  - Caracteres UTF-8 no reconocidos
  - Referencias rotas por cambios de etiquetas

#### Paso 5.3 — Revisión final
- Verificar que las fórmulas compilan igual que en la versión original
- Verificar que el texto en español es coherente y natural
- Verificar que no se tradujeron elementos que debían conservarse
- Revisar los marcadores `% TODO: REVISAR`

---

## 5. Estructura de archivos resultante

```
Antigravity/
├── Lectura_2_.pdf                  # PDF original (ya existente)
├── Lectura_2_original.tex          # LaTeX fiel al PDF
├── Lectura_2_espanol.tex           # LaTeX traducido al español
├── figures/                        # Figuras extraídas
│   ├── fig1.png
│   ├── fig2.png
│   ├── fig3.png
│   └── ...
├── output_marker/                  # Salida intermedia de marker-pdf
│   └── ...
├── Lectura_2_original.pdf          # PDF compilado desde LaTeX original
├── Lectura_2_espanol.pdf           # PDF compilado desde LaTeX español
└── planificacion_pdf_a_latex.md    # Este documento
```

---

## 6. Estimación de esfuerzo

| Fase | Tiempo estimado | Automatizable |
|---|---|---|
| Fase 1: Análisis | ✅ Completada | Sí |
| Fase 2: Extracción y reconstrucción | 30–60 min | Parcialmente (marker + scripts + revisión manual de fórmulas) |
| Fase 3: Revisión y corrección | 15–30 min | No (revisión visual) |
| Fase 4: Traducción | 20–40 min | Parcialmente (traducción IA + revisión) |
| Fase 5: Compilación y verificación | 10–20 min | Parcialmente |
| **Total estimado** | **~1.5–2.5 horas** | |

---

## 7. Decisiones pendientes del usuario

> **IMPORTANTE — Antes de proceder, necesito tu decisión sobre estos puntos:**

1. **Figuras**: ¿Quieres que extraiga las figuras como imágenes PNG de alta resolución del PDF, o prefieres solo placeholders con comentarios indicando qué figura va en cada lugar?

2. **Clase de documento**: ¿Usar `IEEEtran` (replica el estilo IEEE del original) o `article` (más simple y genérico)?

3. **Referencias**: ¿Prefieres un bloque `thebibliography` simple en el `.tex`, o un archivo `.bib` separado con BibTeX?

4. **¿Instalar marker-pdf?** Es una herramienta de ~200 MB con dependencias de deep learning. Alternativamente, puedo hacer la extracción solo con PyMuPDF + Pandoc (más ligero, pero menor precisión en fórmulas y columnas). ¿Procedo con la instalación?

5. **Limpieza**: ¿Quieres que elimine los archivos intermedios (`output_marker/`, scripts temporales) al finalizar?

---

## 8. Resumen de viabilidad

| Pregunta | Respuesta |
|---|---|
| ¿Es viable el flujo? | **Sí**, completamente viable |
| ¿Qué limitaciones tiene? | Figuras vectoriales, fórmulas complejas requieren reconstrucción manual |
| ¿Qué precisión esperar? | **85–95% global** (texto ~98%, fórmulas ~85%, figuras ~70%) |
| ¿Qué herramientas se usarían? | PyMuPDF, marker-pdf (o Pandoc), pdfplumber, pdflatex |
| ¿Conviene hacerlo en fases? | **Sí**, el enfoque de 5 fases es el más seguro y controlable |

> **VENTAJA CLAVE**: El PDF fue generado originalmente desde LaTeX, lo que significa que la extracción de texto será excepcionalmente limpia comparada con PDFs generados por Word u otros procesadores. Esto eleva significativamente la precisión esperada en todo el pipeline.
