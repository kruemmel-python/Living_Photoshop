#!/usr/bin/env python3
"""
PPM → PNG Konverter
Unterstützt P3 (ASCII) und P6 (Binary)
Python 3.12
"""

from pathlib import Path
from PIL import Image
import sys


def convert_ppm_to_png(ppm_path: Path, png_path: Path) -> None:
    """
    Konvertiert eine PPM-Datei (P3 oder P6) nach PNG.

    :param ppm_path: Pfad zur Eingabe-PPM-Datei
    :param png_path: Pfad zur Ausgabe-PNG-Datei
    """

    if not ppm_path.exists():
        raise FileNotFoundError(f"Eingabedatei existiert nicht: {ppm_path}")

    if ppm_path.suffix.lower() != ".ppm":
        raise ValueError("Eingabedatei ist keine .ppm-Datei")

    try:
        with Image.open(ppm_path) as img:
            img.save(png_path, format="PNG")

    except Exception as exc:
        raise RuntimeError(f"Konvertierung fehlgeschlagen: {exc}") from exc


def main() -> None:
    """
    CLI-Einstiegspunkt
    """

    match sys.argv:
        case [_, ppm_file]:
            ppm_path = Path(ppm_file)
            png_path = ppm_path.with_suffix(".png")

        case [_, ppm_file, png_file]:
            ppm_path = Path(ppm_file)
            png_path = Path(png_file)

        case _:
            print("Usage:")
            print("  python ppm_to_png.py input.ppm [output.png]")
            sys.exit(1)

    try:
        convert_ppm_to_png(ppm_path, png_path)
        print(f"✔ Konvertiert: {ppm_path} → {png_path}")

    except Exception as error:
        print(f"✖ Fehler: {error}")
        sys.exit(2)


if __name__ == "__main__":
    main()
