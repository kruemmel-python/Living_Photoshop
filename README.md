# Living_Photoshop

Living_Photoshop fusioniert HALO (SIMD-Filter) mit MicroDB (Swarm/Mycel) zu einem
biologischen Bildgenerator. Jedes Pixel wird Teil eines agentenbasierten Systems:
Gradienten erzeugen Gefahr-Pheromone, Ressourcen steuern Erholung/Glattziehen,
und Mycel-Strukturen sch?rfen oder texturieren das Bild.

## Was es macht

- Liest ein RGB-Bild (PPM/P6, 8-bit)
- Berechnet Luminanz + Sobel-Gradienten (HALO)
- Fuettert MicroDB-Felder (Ressourcen/Danger), laesst Agents laufen
- Nutzt Mycel + Danger als Masken fuer Blur/Unsharp/Mutation
- Schreibt ein neues PPM als Ergebnis

## DNA-gestuetzte Morphogenese

MicroDB speichert nun erweiterte DNA (Genom-Regeln) im Evolutionspool.
Diese DNA wird nach jedem Lauf aggregiert (fitness-gewichtet) und steuert:
- Kanten-Orientierung (edge_seek)
- Glattziehen (soften_bias, blur_pref)
- Mycel-Antrieb (mycel_drive)
- Schaerfung (sharpen_pref)
- Farbverschiebung (color_shift_r/g/b)
- Kontrast-Puls (contrast_pulse)

Damit wird jedes Bild zum Ergebnis eines lernenden Genom-Archivs.

## Build

Im Projekt-Root:

```bash
cmake -S Living_Photoshop -B Living_Photoshop/build
cmake --build Living_Photoshop/build
```

Hinweise:
- OpenCL ist optional. Wenn installiert, wird MicroDB mit OpenCL gebaut.
- `kernels/diffuse.cl` muss im `Living_Photoshop` Ordner vorhanden sein.

## Nutzung

```bash
Living_Photoshop/build/living_photoshop --input in.ppm --output out.ppm
```

Optionen:
- `--steps N` Swarm-Iterationen
- `--agents N` Agentenzahl
- `--seed N` RNG Seed
- `--blur-sigma F` Blur fuer ruhige Bereiche
- `--unsharp-sigma F` Unsharp Basis
- `--unsharp-amount F`
- `--unsharp-threshold F`
- `--mutate F` Farbmutation
- `--mycel F` Mycel-Textur
- `--edge-gain F` Kantenverstaerkung
- `--blur-threshold F` Schwelle fuer Ressourcen-Blur
- `--blur-gain F` Blur-Gewicht
- `--size N` Ausgabe-Groesse (quadratisch, z.B. 2048)
- `--loop-damp-start N` Damping ab Loop N (default 3)
- `--loop-damp F` Detail-Daempfung pro Loop (default 0.9)
- `--commands PATH` Command/SQL-Datei
- `--dna-import PATH` DNA CSV laden
- `--dna-export PATH` DNA CSV speichern
- `--dna-style TAG` Style-Tag fuer neue DNA Eintraege
- `--demo-bootstrap PATH` Demo: DNA exportieren, dann importieren
- `--eternal` Endlos-Loop (neue Frames mit DNA-Merge)
- `--eternal-loops N` Endlos-Loop begrenzen (Default 10, 0 = Default)
  (bei N > 1 wird `_loopX` an den Dateinamen angehaengt)

## Command/SQL-Datei (Mini-Shell)

Die Datei wird zeilenweise gelesen. Unterstuetzt:

```text
# Fokusbereich (x y radius)
goto 320 240 120

# SQL-Subset
sql DELETE FROM Pixels WHERE energy < 0.1
sql SELECT COUNT FROM Pixels WHERE mycel > 0.7
```

Erlaubte Felder: `energy`, `resource`, `danger`, `mycel`
Erlaubte Operatoren: `<` und `>`

## DNA-CSV

Exportiert und importiert die evolvierte DNA (fitness-gewichtet). Format:
`pool,species,fitness,sense_gain,pheromone_gain,exploration_bias,edge_seek,soften_bias,mycel_drive,blur_pref,sharpen_pref,color_shift_r,color_shift_g,color_shift_b,contrast_pulse,style`

Style-Tags:
- `style` ist ein einfacher String (bitte ohne Kommas).
- `--dna-style TAG` setzt das Tag fuer neu erzeugte DNA-Eintraege.

Demo-Bootstrap:
```bash
living_photoshop --input in.ppm --output out.ppm --demo-bootstrap dna_boot.csv --dna-export dna_final.csv --dna-style DeepOcean
```

## Formate

- Input/Output: PPM P6 (8-bit RGB)
- Keine Alpha- oder HDR-Unterstuetzung in dieser Version
