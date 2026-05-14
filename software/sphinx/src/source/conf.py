# Configuration file for the Sphinx documentation builder.
#
# For the full list of built-in configuration values, see the documentation:
# https://www.sphinx-doc.org/en/master/usage/configuration.html

# -- Project information -----------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#project-information


import os
import sys

sys.path.insert(0, os.path.abspath('.'))
import os
import re
import shutil

def setup(app):
    def copy_schematics(app, exception):
        if app.builder.name == "latex":
            src = os.path.join(app.srcdir, "_static", "hardware","unit_sch_V_0_1_2_ue0072_touch_dot_s3.pdf")
            dst = os.path.join(app.outdir, "unit_sch_V_0_1_2_ue0072_touch_dot_s3.pdf")
            if os.path.exists(src):
                shutil.copyfile(src, dst)
                print("[sphinx hook] Copied schematic PDF for LaTeX")

    def copy_latex_raw_images(app, exception):
        if app.builder.name != "latex":
            return

        latex_build_dir = app.outdir
        tex_file = os.path.join(latex_build_dir, 'programmer.tex')

        if not os.path.isfile(tex_file):
            print("[sphinx hook] .tex file not found")
            return

        with open(tex_file, 'r', encoding='utf-8') as f:
            tex_content = f.read()

        image_files = re.findall(r'\\includegraphics(?:\[[^\]]*\])?{([^}]+)}', tex_content)
        image_files = set(image_files)

        for img in image_files:
            found = False
            # Ruta original de la imagen como aparece en LaTeX
            img_clean = img.replace('\\', '/')
            basename = os.path.basename(img_clean)

            # Intenta buscar en múltiples ubicaciones base
            for base in ["_static", os.path.join("_static", "hardware"), "images", ""]:
                candidate = os.path.join(app.srcdir, base, img_clean)
                if os.path.exists(candidate):
                    dst = os.path.join(latex_build_dir, basename)
                    shutil.copyfile(candidate, dst)
                    print(f"[sphinx hook] Copied image: {candidate} -> {dst}")
                    found = True
                    break
            if not found:
                print(f"[sphinx hook] WARNING: Could not find image for LaTeX: {img_clean}")

    app.connect("build-finished", copy_latex_raw_images)
    app.connect("build-finished", copy_schematics)


project = 'Template  User Guide and Technical Reference'
copyright = '2025, Unit Electronics'
author = 'R&D and Innovation Department'
release = '0.0.1'
extensions = [
    'sphinx.ext.autodoc',
    'sphinx.ext.viewcode',
    'sphinx.ext.napoleon',
    'sphinx.ext.doctest',
    'sphinx.ext.intersphinx',
    'sphinx.ext.todo',
    'sphinx.ext.coverage',
    'sphinx.ext.mathjax',
    'sphinx.ext.ifconfig',
    'sphinx.ext.githubpages',
    'sphinx_togglebutton',
    'rst2pdf.pdfbuilder',
    'sphinx_tabs.tabs',
    'sphinx_copybutton',  # Add this extension


]
copybutton_prompt_text = ">>> "  # Removes prompts from code blocks
copybutton_only_copy_prompt_lines = False  # Copies all lines, including those without a prompt


templates_path = ['_templates']
html_theme_options = {
    "repository_url": "https://github.com/UNIT-Electronics-MX//unit_repository_template",  # URL of your project's repository
    "repository_branch": "main",  # Rama principal de tu repositorio
      "path_to_docs": "docs/",  # Ruta a la documentación dentro del repositorio
    "use_repository_button": True,  # Muestra un botón que enlaza al repositorio
    "use_issues_button": True,  # Muestra un botón que enlaza a la sección de issues
    "use_edit_page_button": True,  # Muestra un botón para editar la página actual
}
latex_documents = [
    ('index', 'programmer.tex', 'Template  User Guide and Technical Reference',
     'Department of Research, Innovation, and Development', 'manual'),
]

latex_elements = {
    'classoptions': ',twocolumn',
    'preamble': r'''
\usepackage{fancyhdr}
\usepackage{titlesec}
\usepackage{graphicx}
\usepackage{etoolbox}
\usepackage{tocloft}
\usepackage{pdfpages}
\usepackage{eso-pic}
\usepackage{xcolor}

% Títulos justificados a la izquierda
\titleformat{\section}[block]{\Large\bfseries\raggedright}{\thesection}{1em}{}
\titleformat{\subsection}[block]{\large\bfseries\raggedright}{\thesubsection}{1em}{}

% Encabezado tipo datasheet
\pagestyle{fancy}
\fancyhf{}
\fancyhead[L]{CH552 Multi-Protocol Programmer}
\fancyhead[R]{Unit Electronics}
\fancyfoot[C]{\thepage}

% Tabla de contenidos como "capítulo" centrado
\let\twocolumnbeforecontents\twocolumn
\renewcommand\twocolumnbeforecontents{%
  \cleardoublepage
  \onecolumn
  \chapter*{Contents}
  \addcontentsline{toc}{chapter}{Contents}
  \markboth{Contents}{Contents}
}

% Página de notas con marca de agua
\newcommand{\blanknotepage}{%
  \clearpage
  \AddToShipoutPictureFG*{%
    \AtPageCenter{%
      \rotatebox{45}{\textcolor[gray]{0.55}{\fontsize{2cm}{2cm}\selectfont NOTES}}%
    }%
  }%
  \null
  \thispagestyle{empty}
  \clearpage
}
''',
}




exclude_patterns = []
master_doc = 'index'
numfig = True

html_theme = 'sphinx_book_theme'
html_logo = "_static/logo.png"

html_static_path = ['_static']
latex_logo = "_static/logo.png" 
html_css_files = [
    'custom.css',
]
