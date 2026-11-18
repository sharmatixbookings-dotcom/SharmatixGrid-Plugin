# SharmatixGrid

Generador de patrones y melodías MIDI para música electrónica / psytrance.
Cuadrícula interactiva de 16 pasos, acotada a una escala, con exportación por **Drag & Drop** del `.mid` directamente a la pista del DAW.

Formatos: **VST3** (Windows / macOS) y **AU** (macOS), más Standalone para probar la interfaz.
macOS: binario universal (Apple Silicon + Intel).

## Funciones

- Escalas: Natural Minor, Minor Pentatonic, Phrygian Dominant (+ Phrygian y Harmonic Minor), con tónica, octava base y rango (1–3 octavas).
- **GENERATE**: crea un patrón nuevo caminando por los grados de la escala, así que todas las notas son armónicas.
- Editas a mano pintando/borrando bloques en la cuadrícula (click o arrastre).
- **DRAG MIDI**: mantén click y arrastra al DAW para soltar el clip como `.mid`.
- **SAVE .MID**: guarda el clip donde quieras.
- Arrastra un `.mid` encima del plugin para importarlo (primer compás, ajustado a la escala).
- Si cambias escala/tónica/octava, las notas existentes se ajustan a la nueva escala.
- Salida MIDI sincronizada con el transporte del host (loop de 1 compás) mediante el parámetro **MIDI OUT**.

## Compilar

Requisitos: CMake ≥ 3.22, un compilador C++17 (Xcode en macOS, Visual Studio 2022 en Windows) y conexión a internet la primera vez (CMake descarga JUCE).

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

Los plugins quedan en `build/SharmatixGrid_artefacts/Release/` (`VST3/`, `AU/`, `Standalone/`).
Para copiarlos automáticamente a las carpetas del sistema: `-DSHARMATIX_COPY_AFTER_BUILD=ON`.

## Uso en el DAW

- **Ableton Live**: carga SharmatixGrid en una pista MIDI como instrumento, pulsa GENERATE y arrastra **DRAG MIDI** a una pista MIDI (clip nuevo).
- **FL Studio**: cárgalo como plugin generador y arrastra el `.mid` al Piano Roll o al Playlist. Para usar la salida MIDI en vivo, asigna un puerto MIDI de salida en la configuración del wrapper del plugin.

## Estructura

```
CMakeLists.txt
Source/
  PluginProcessor.h/.cpp   motor de escalas, clip MIDI, export/import .mid, salida MIDI
  PluginEditor.h/.cpp      GUI psicodélica, cuadrícula, GENERATE, Drag MIDI
  PsyLookAndFeel.h         paleta neón, knobs y botones
.github/workflows/build.yml  compilación automática (macOS + Windows)
```

## Licencia

JUCE se distribuye bajo GPLv3 o licencia comercial. Si vas a distribuir el plugin, revisa los términos en https://juce.com/juce-8-licence/.
