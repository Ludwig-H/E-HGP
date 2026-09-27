# Contre-audit — collecte et tri des survivants S2

27 septembre 2026. Lecture seule des sources et relecture indépendante de
[R1](../b_q34_resident_survivors_20260927/checks/r1/capture.json).
Aucun moteur, source gelée ou reçu d'autrui modifié ; aucun GCP.

## Verdict limité

Lecteurs LIVE normal et `-O` **PASS, 26 commandes**. Le candidat et la
véritable référence sont comparés dans le même processus, en ABBA.
Release, Clang ASan/UBSan/LSan et le binaire lié CUDA produisent les mêmes
résultats **portables**. CUDA 12.9 / CUB sont réellement compilés, mais
aucun kernel device n'est exécuté dans cette porte. Aucun gain GPU, mur
FULL ou contrat 100 ms n'est acquis.

Couverture : 85 cas natifs, 1 360 appels ABBA, 18 cas de collecteur,
33 refus, deux mutants compilés tués par le motif prévu. Les 101 320
requêtes et 39 868 survivants agrègent le premier candidat B de chaque
couple cas/Q, soit 340 passages, **pas les 1 360 appels cumulés**. Un appel
valide d'ownership reste hors ce compteur. Les champs non vides couvrent
376 inversions, 72 passages E=0, quatre E>0/S=0, 63 plans et 1 411 replis.
Les 2 076 clés hautes, 4 134 vagues vides des lots clairsemés et 52 croissances
relèvent des petits tests portables, pas d'une exécution device géante.

## Invariants relus

Le port garde l'identité privée de session, décision et préparation.
Les endpoints, orientation et masque suivent toujours la clé u64 originale.
La sortie `Q34FilterBatch` est convertie sans nouveau tri hôte, avec les
mêmes masques rectangles, P logique, E physique, rejets par voie et visites.
Le backend refuse un doublon d'ordinal au lieu de le dédupliquer.

Les trois kernels de filtrage/compactage/rectangles sont épinglés identiques
à la référence par [provenance.py](../b_q34_resident_survivors_20260927/provenance.py).
Le travail géométrique n'est pas diminué par ce changement de collecteur.
La vérification actuelle donne donc un raccord d'ordre/transport, pas une
nouvelle preuve de couverture WSPD ou de croissance sous-quadratique.

La croissance géométrique est liée à S cumulé, jamais au nombre de vagues.
La capacité reste O(S+Q), et le ledger prend en compte l'ancien et le nouveau
buffer pendant une croissance. Aucun tableau global P/E ni Q conservé par
vague ne réintroduit silencieusement une mémoire en E.

Les métadonnées et tampons de vague sont libérés avant la phase de tri.
Le split possède encore Edge24 et les deux paires de buffers clé/payload ;
Edge est libéré avant allocation du scratch de tri. Les buffers de tri
font 40S octets logiques, plus scratch et contrôle. Le téléchargement normal
porte sur Payload12 ; les clés diagnostiques sont optionnelles. Tous ces
buffers de collecte/tri sont détruits avant retour, sauf sortie hôte et masques retenus.
Le `Ledger` vit plus longtemps que les `DeviceBuffer` qui le référencent.

Les grandes **valeurs** de clé et les formules au-delà d'INT_MAX ne sont
pas un essai device de grande **cardinalité**. La surcharge CUB compilée
utilise un nombre d'items 64 bits ; le scan local de vague reste limité à
int, avec progression jusqu'à EOF. Une porte réelle de tri device avec
clés hautes et doublons reste nécessaire avant toute campagne de gain G4.

## Temps, mémoire et échecs

`array_peak_bytes` portable et CUDA ont des périmètres différents : le
premier couvre le collecteur et sa vague, le second les allocations device
du résident, index inclus. Aucun n'est le pic global hôte+device ni le RSS.
Les sorties ABBA simultanées empêchent aussi de lire le RSS du gate comme
celui d'un opérateur isolé.

En portable, `waves_ms` contient `append_ms` ; en CUDA, l'ajout D2D est
distinct des vagues. `order_ms` hôte ajoute la conversion aux opérations de
tri, et les champs de destruction peuvent être emboîtés. Il ne faut pas
additionner uniformément ces sous-chronos. Aucun chrono n'est annoncé ici.

L'injection `bad_alloc` portable précède une vraie croissance mais ne fait
pas échouer un allocateur CUDA. L'état est fermé après échec et aucune
reprise partielle n'est promise. Cette porte n'est pas une qualification
TSan, d'allocation device défaillante ou de nouvelle concurrence.

## Fermeture de compilation et du lecteur

Les inventaires `-M` reprennent les commandes CMake effectives avant chaque
build ; les `.o.d` compilés doivent être inclus et inchangés. Sont notamment
épinglés les headers, outils CUDA internes et libdevice, les archives CUDA,
gen et rt, puis les fichiers de compilation/lien et binaires. Le `link.txt`
CUDA R1 a été vérifié : `lib64` résout vers les fichiers `lib` épinglés ;
les commandes `c++` et `g++` résolvent le même compilateur local GCC13.
Cette fermeture porte sur les fichiers listés, pas sur un environnement
système intégralement hermétique ou toutes les bibliothèques dynamiques.

Nuance conservée : CMake Release place `-O3` après `-O1` dans la cible
sanitizer, donc son optimisation effective est O3, instrumentation active.
Ce n'est pas un motif de relance ni une qualification à O1.

Les correctifs de lecteur obtenus avant gel imposent la chronologie des
commandes, la liaison de chaque binaire à son build parmi exactement trois
profils, et les types/champs exacts du gate. Le selftest supplémentaire
vit hors des sources compilées directes et reste une preuve distincte de
ces deux lectures LIVE ; aucune qualification de ses nouveaux cas n'est
ajoutée par la présente note.

## Reproduction et identités

```sh
python3 -B morsehgp3D_v9/audits/b_q34_resident_survivors_20260927/run.py --readback morsehgp3D_v9/audits/b_q34_resident_survivors_20260927/checks/r1
python3 -B -O morsehgp3D_v9/audits/b_q34_resident_survivors_20260927/run.py --readback morsehgp3D_v9/audits/b_q34_resident_survivors_20260927/checks/r1
```

- `collector.hpp` : `093bd986331d92aa9c77598bac3417917fc25e43487b1864c492e035c5743654`.
- `device_cuda.cu` : `af9259f6104cf0a0f12116af6be2860311b2cc32a989cf6fe70b78529bc86666`.
- `host.hpp` : `666ea61575dd4afe4c0ca9164c8af2f92d3455cbe286c10f45549be2437fbc30`.
- `probe.cpp` : `3b18532a4e41b5b920bdae36f6472a63c64bb398be000feb290d7aae4a7ae13e`.
- `run.py` : `a9b75fca4c0598b22841b8dd57a91a120dc475f2fb84d558aad75b7d1abc8156`.
- R1 `capture.json` : `d2c3c0813ac32bbbfd0b4e1c7d58e207395e7553090b33e40cbc016a39c23585`.
- R1 `summary.json` : `8a96e75cf0cdf824232589fd67a44b2021117c243bde3c0f0b3691b8fe30ab0c`.

Note close ; aucun blocage supplémentaire repéré dans ce périmètre.
