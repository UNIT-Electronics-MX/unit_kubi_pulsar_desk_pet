# File Names
NAME = Documentation
file = src
DIRECTORIO = ./pdf/latex
DIR_HTML = ./src/build 

# Detect OS
ifeq ($(OS),Windows_NT)
    # Detect Git Bash
    ifeq ($(shell uname -s),Linux)
        RM = rm -rf
        MKDIR = mkdir -p
        TOUCH = touch
        COPY = cp -R
        DELETE_NON_PDF = find $(DIRECTORIO) -type f ! -name '*.pdf' -exec rm -f {} \;
    else
        # Native Windows (cmd.exe)
        RM = rmdir /S /Q
        MKDIR = mkdir
        TOUCH = copy NUL
        COPY = xcopy /E /I /Y
        DELETE_NON_PDF = for /R $(DIRECTORIO) %f in (*) do if NOT "%%~xf"==".pdf" del "%%f"
    endif
else
    # Linux/macOS
    RM = rm -rf
    MKDIR = mkdir -p
    TOUCH = touch
    COPY = cp -R
    DELETE_NON_PDF = find $(DIRECTORIO) -type f ! -name '*.pdf' -exec rm -f {} \;
endif

# Symbolic Targets
help:
	@echo "Use the following commands:"
	@echo "make all     create files and build the project"
	@echo "make pdfx    create the project environment"
	@echo "make clean   remove all files"

all: build

build:
	@$(RM) docs 2>/dev/null || true
	@$(MKDIR) docs
	@cd src && make clean && make html
	@$(COPY) src/build/html/* docs && $(TOUCH) docs/.nojekyll
	@echo "Documentation built and copied to docs"
	@echo "All files removed"
	@$(DELETE_NON_PDF)
	@$(COPY) pdf/latex/* pdf/ && $(RM) pdf/doctrees 
	@$(RM) $(DIRECTORIO)
	@echo "Non-PDF files removed in $(DIRECTORIO)"
	@$(RM) $(DIR_HTML)

pdfx:
	@$(RM) pdf 2>/dev/null || true
	@$(MKDIR) pdf
	@cd src && make clean && make html && sphinx-build -M latexpdf source ../pdf
	@echo "PDF built and copied to docs"
	@$(MAKE) build

clean:
	@$(RM) docs 2>/dev/null || true
	@$(RM) pdf 2>/dev/null || true
