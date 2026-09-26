#!/bin/bash
# Abre el panel de aprendizaje ESP32-CAM en Mac (doble clic en Finder).
# Equivale a iniciar.bat de Windows. No requiere instalar nada: usa Perl, que viene con macOS.
cd "$(dirname "$0")" || exit 1
echo "Panel ESP32-CAM"
if ! command -v perl >/dev/null 2>&1; then
    echo "[ERROR] No encontre Perl en esta Mac. Abri Terminal y escribi: xcode-select --install"
    read -r -p "Enter para salir"
    exit 1
fi
perl panel/servidor.pl "$@"
