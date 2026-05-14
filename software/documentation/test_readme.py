from generate_pdf import parse_readme_md

if __name__ == "__main__":
    data = parse_readme_md("README.md")
    errors = []

    for key, value in data.items():
        if isinstance(value, str):
            if "\\begin{itemize}" in value and "\\item" not in value:
                errors.append(f"ERROR: Sección '{key}' contiene una lista vacía.")
            elif value.strip() == "":
                errors.append(f"ADVERTENCIA: Sección '{key}' está completamente vacía.")
            elif "coming soon" in value.lower():
                errors.append(f"ADVERTENCIA: Sección '{key}' contiene contenido placeholder.")

    if errors:
        print("\n".join(errors))
        print("README.md falló la validación previa a la compilación.")
        exit(1)
    else:
        print("README.md validado correctamente. Todo listo para compilar.")
