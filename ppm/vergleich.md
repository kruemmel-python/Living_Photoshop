(.venv) PS G:\HELO_V1\Living_Photoshop_build\Release> .\living_photoshop.exe --input G:\HELO_V1\ppm\ichMantel_neu.ppm --output G:\HELO_V1\ppm\out\ichMantel_neu_4k.ppm --size 4000 --commands commands_all_examples.txt
[sql] deleted=2933268
[sql] deleted=5191503
[sql] deleted=15562469
[sql] deleted=1748585
[sql] count=18583
[sql] count=7167972
[sql] count=2508
[sql] count=11561641
OK: G:\HELO_V1\ppm\out\ichMantel_neu_4k.ppm
(.venv) PS G:\HELO_V1\Living_Photoshop_build\Release> cd..
(.venv) PS G:\HELO_V1\Living_Photoshop_build> cd..
(.venv) PS G:\HELO_V1> cd pmm
(.venv) PS G:\HELO_V1\ppm> python vergleich.py
=== Objektive Bildmetriken ===
MSE       : 0.000060
PSNR_dB   : 42.217561
SSIM      : 0.995254
(.venv) PS G:\HELO_V1\ppm>



Angewande Einstellungen der Datenbank!
# Living_Photoshop commands.txt
# Format: eine Anweisung pro Zeile. Leerzeilen und # Kommentare sind erlaubt.
# Unterstuetzte Kommandos:
#   goto x y radius
#   sql DELETE FROM Pixels WHERE <field> <op> <value>
#   sql SELECT COUNT FROM Pixels WHERE <field> <op> <value>
# Erlaubte Felder: energy, resource, danger, mycel
# Erlaubte Operatoren: < und >
# Hinweis: Nur eine Bedingung pro Zeile (kein AND/OR).
#
# Nutzung mit living_photoshop.exe (Windows):
#   Living_Photoshop_build\Release\living_photoshop.exe --input in.ppm --output out.ppm --commands commands_all_examples.txt
# Beispiel mit bestehender Datei aus ppm\slice_334.ppm:
#   Living_Photoshop_build\Release\living_photoshop.exe --input ..\ppm\slice_334.ppm --output out_slice_334.ppm --commands commands_all_examples.txt

# -------------------------------------------------------------------
# 1) Fokusbereich setzen (x y radius)
# Bedeutet: Fokus/Selektion wird auf diesen Bereich gelenkt.
# Beispiel: Bildmitte (1920x1080) mit Radius 500

goto 960 540 500

# -------------------------------------------------------------------
# 2) DELETE Beispiele (harte Selektion)
# Bedeutet: Pixel/Agents, die die Bedingung nicht erfuellen, werden entfernt.
# Achtung: harte Filterung, kann das Bild stark ausduennen.

# Entferne niedrige Energie
sql DELETE FROM Pixels WHERE energy < 0.2

# Entferne zu geringe Ressourcen
sql DELETE FROM Pixels WHERE resource < 0.25

# Entferne zu geringe Gefahr (falls du Gefahr als Struktur-Antrieb brauchst)
sql DELETE FROM Pixels WHERE danger < 0.15

# Entferne schwaches Mycel
sql DELETE FROM Pixels WHERE mycel < 0.3

# -------------------------------------------------------------------
# 3) SELECT COUNT Beispiele (Statistik/Debug)
# Bedeutet: zaehlt Pixel, die die Bedingung erfuellen.
# Das ist nuetzlich, um Schwellenwerte zu justieren.

# Wie viele Pixel haben hohe Energie?
sql SELECT COUNT FROM Pixels WHERE energy > 0.7

# Wie viele Pixel haben genug Ressourcen?
sql SELECT COUNT FROM Pixels WHERE resource > 0.6

# Wie viele Pixel haben hohe Gefahr (Kanten/Struktur)?
sql SELECT COUNT FROM Pixels WHERE danger > 0.6

# Wie viele Pixel haben starkes Mycel (Textur)?
sql SELECT COUNT FROM Pixels WHERE mycel > 0.5

# -------------------------------------------------------------------
# 4) Kombinierte Beispiel-Sequenz
# Fokus setzen, dann hart filtern, dann zaehlen.
# (Die Reihenfolge im File ist die Ausfuehrungsreihenfolge.)

# Fokus in der Mitte
# goto 960 540 300
# sql DELETE FROM Pixels WHERE energy < 0.25
# sql DELETE FROM Pixels WHERE mycel < 0.2
# sql SELECT COUNT FROM Pixels WHERE danger > 0.5

