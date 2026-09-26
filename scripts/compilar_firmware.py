"""Compila lecciones y genera el .bin unificado que carga el panel (firmware/NN_nombre.bin).

Solo lo usa quien mantiene el repo (requiere PlatformIO). Por cada leccion:
  1. `pio run`, 2. une bootloader + particiones + boot_app0 + app en un .bin desde 0x0,
  3. registra en firmware/manifiesto.json el hash del codigo fuente que lo produjo.
`--verificar` no compila: falla si algun .bin quedo desalineado de su codigo (Constitution 3).

Uso:  python scripts/compilar_firmware.py [01_blink ...]     (sin args = todas)
      python scripts/compilar_firmware.py --verificar
"""
import hashlib
import json
import subprocess
import sys
from pathlib import Path

try:
    sys.stdout.reconfigure(encoding="utf-8", errors="replace")
except Exception:
    pass

RAIZ = Path(__file__).resolve().parent.parent
LECCIONES = RAIZ / "lecciones"
SALIDA = RAIZ / "firmware"
MANIFIESTO = SALIDA / "manifiesto.json"
PIO_HOME = Path.home() / ".platformio"
PIO = PIO_HOME / "penv" / "Scripts" / "pio.exe"
PYTHON_PIO = PIO_HOME / "penv" / "Scripts" / "python.exe"
ESPTOOL = PIO_HOME / "packages" / "tool-esptoolpy" / "esptool.py"
BOOT_APP0 = PIO_HOME / "packages" / "framework-arduinoespressif32" / "tools" / "partitions" / "boot_app0.bin"


def hash_fuente(leccion: Path) -> str:
    """Hash de todo lo que define el binario: platformio.ini + src/ + lib/ compartida."""
    archivos = [leccion / "platformio.ini", *sorted((leccion / "src").rglob("*"))]
    lib = RAIZ / "lib"
    if lib.exists():
        archivos += sorted(lib.rglob("*"))
    h = hashlib.sha256()
    for a in archivos:
        if a.is_file() and a.name != "secrets.h.example":
            h.update(a.relative_to(RAIZ).as_posix().encode())
            # normaliza CRLF/LF: el mismo codigo debe dar el mismo hash en cualquier checkout
            h.update(a.read_bytes().replace(b"\r\n", b"\n"))
    return h.hexdigest()


def compilar(leccion: Path) -> dict:
    """Compila una leccion y escribe su .bin unificado. Devuelve la entrada del manifiesto."""
    if (leccion / "src" / "secrets.h").exists():
        # Un .bin publicado con secrets.h adentro publicaria la clave del WiFi.
        raise SystemExit(f"[ERROR] {leccion.name}: existe src/secrets.h. Movelo antes de compilar "
                         "firmware publicable (la clave quedaria dentro del .bin).")
    print(f"[..] compilando {leccion.name}")
    subprocess.run([str(PIO), "run", "-d", str(leccion)], check=True, capture_output=True)
    build = leccion / ".pio" / "build" / "esp32cam"
    destino = SALIDA / f"{leccion.name}.bin"
    subprocess.run([str(PYTHON_PIO), str(ESPTOOL), "--chip", "esp32", "merge_bin", "-o", str(destino),
                    "--flash_mode", "dio", "--flash_freq", "40m", "--flash_size", "4MB",
                    "0x1000", str(build / "bootloader.bin"), "0x8000", str(build / "partitions.bin"),
                    "0xe000", str(BOOT_APP0), "0x10000", str(build / "firmware.bin")],
                   check=True, capture_output=True)
    print(f"[OK] {destino.name} ({destino.stat().st_size} bytes)")
    return {"fuente_sha256": hash_fuente(leccion), "bytes": destino.stat().st_size}


def leer_manifiesto() -> dict:
    return json.loads(MANIFIESTO.read_text(encoding="utf-8")) if MANIFIESTO.exists() else {}


def verificar() -> int:
    """Devuelve 1 si algun .bin no corresponde a su codigo fuente actual."""
    manifiesto = leer_manifiesto()
    fallas = 0
    for nombre, entrada in sorted(manifiesto.items()):
        leccion = LECCIONES / nombre
        binario = SALIDA / f"{nombre}.bin"
        if not binario.exists():
            print(f"[ERROR] {nombre}: falta {binario.name}"); fallas += 1
        elif hash_fuente(leccion) != entrada["fuente_sha256"]:
            print(f"[ERROR] {nombre}: el codigo cambio y el .bin no se regenero"); fallas += 1
        else:
            print(f"[OK] {nombre}")
    return 1 if fallas else 0


def main() -> int:
    if "--verificar" in sys.argv:
        return verificar()
    nombres = [a for a in sys.argv[1:] if not a.startswith("-")]
    lecciones = [LECCIONES / n for n in nombres] if nombres else sorted(p for p in LECCIONES.iterdir() if p.is_dir())
    SALIDA.mkdir(exist_ok=True)
    manifiesto = leer_manifiesto()
    for leccion in lecciones:
        manifiesto[leccion.name] = compilar(leccion)
    MANIFIESTO.write_text(json.dumps(manifiesto, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    return verificar()


if __name__ == "__main__":
    sys.exit(main())
