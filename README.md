# Warpus Kong

Synthé polyphonique / monophonique (32 voix réelles) : Karplus-Strong dont l'excitation est un sample (sweep laser / « krafty »).

## Principe d'accord
La boucle de la corde dure exactement `sampleRate / fréquence` samples (retard fractionnaire par filtre
passe-tout, compensation du retard du filtre d'amortissement) : un DO joué sonne DO sur tout le clavier.
La longueur de boucle est donc plus courte pour les notes aiguës, plus longue pour les graves.

* **Mode Window** : une fenêtre du sample, lue à vitesse native, de la longueur d'une période, à partir de START.
* **Mode Stretch** : la portion START..START+SPAN du sample est comprimée dans une période (le sweep devient une forme d'onde accordée).
* **HOLD** : réinjecte la boucle à chaque période (0 = Karplus-Strong pur, 1 = boucle entretenue).
* **DECAY** : temps de décroissance (T60) constant en secondes sur tout le clavier.
* **FEEDBACK** : pousse le gain de boucle vers 1 (sustain quasi infini). **COLOR** : sombre ↔ brillant (amortissement des aigus dans la boucle).
* **STIFFNESS** : dispersion (filtre passe-tout dans la boucle) → partiels inharmoniques, timbre métallique / cloche.
* **PICKUP** : position du « micro » le long de la corde (filtre en peigne). **NOISE** : bruit blanc ajouté au pincement.
* **SAMPLE EQ** (LOW / MID / MID FREQ / HIGH) : égaliseur appliqué uniquement au sample d'excitation, pas à la sortie.
* **PLAY** : Poly, Mono (re-pincement à chaque note) ou Legato (glisse sans re-pincer). **GLIDE** : portamento.
* **UNISON** (1–5 copies), **DETUNE**, **WIDE** : les copies sont désaccordées et panoramiquées de gauche à droite.

## Compilation (CMake + compilateur C++17)

    # VST3 64 bits (+ Standalone)
    cmake -B build -DCMAKE_BUILD_TYPE=Release
    cmake --build build --config Release

    # Windows 32 bits
    cmake -B build32 -A Win32
    cmake --build build32 --config Release

    # VST2 (uniquement si vous possédez le SDK VST2 sous licence Steinberg)
    cmake -B buildv2 -DVST2_SDK_PATH=C:/SDKs/vstsdk2.4
    cmake --build buildv2 --config Release

Les binaires sont dans `build/WarpusKong_artefacts/Release/`.

## Sauvegarde
Tous les paramètres sont dans l'`AudioProcessorValueTreeState` ; le sample (mono, 16 bits, 20 s max) est
embarqué dans l'état : le projet se recharge à l'identique, même si le fichier d'origine a disparu.
