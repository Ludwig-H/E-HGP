# Calendrier filtré et consultations historiques : preuve privée bornée

Ce paquet conserve un certificat sparse de connexité, sa reconstruction en
vraies multifusions simultanées et un index de consultations historiques en
lots CPU. Les sources restent privées : aucune intégration au moteur HGP,
aucune exécution GPU, aucun benchmark ni contrat de vitesse n'est revendiqué.
Cadre : `phase=exploration_v7_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only` pour le chantier (cette gate reçoit des
graphes abstraits, pas des points), `mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

## Ce qui est conservé et testé

Le graphe réduit porte des naissances natives distinctes et des arêtes dont
la date est strictement postérieure aux deux extrémités. Les hubs non natifs
sont des marques datées, disponibles seulement à leur admission. Une forêt
couvrante minimale conserve les partitions H0 à toutes les coupes ouvertes
et fermées. Le vérificateur contrôle séparément les identités, les dates,
l'acyclicité et la connexion de chaque arête source à sa propre date.
La reconstruction séquentielle ferme chaque plateau d'un coup : pas de
chaîne binaire de nœuds de durée nulle. La note source détaillée reste dans
`sources/README.md`.

La forêt minimale seule perd les arêtes non retenues et leurs incidences.
Les marques gardent explicitement leur identité, représentant et admission ;
elles ne reconstituent ni les contributions, ni les représentants bruts,
ni les références verticales. L'extraction depuis un vrai census, l'atlas
exact des rangs et la complétude géométrique ne sont pas qualifiés ici.

Les chaînes lourdes ajoutent O(N) stockage et O(log N) par consultation,
avec construction séquentielle et requêtes indépendantes sur 1, 2 ou 4 CPU.
`History` est empruntée : elle doit rester vivante et immuable. `Less` doit
être exact, pur et utilisable simultanément par les threads. Les trois
tableaux d'indices portent 24N octets logiques, pas une mesure de RAM/RSS.
Les corps des deux helpers ont été relus ; cela ne remplace pas les gates.
Les coûts sont exprimés en taille du graphe/de l'histoire, pas en nombre de
points 3D : aucun résultat universel sous-quadratique en n n'est déduit.

## Captures fermées

| Capture | Portée | Résultat |
| --- | --- | --- |
| `o2_r1` | Première gate calendrier, conservée comme histoire | 264 cas, 468 473 contrôles |
| `o2_r2` et `san_root_r1` | Gate calendrier renforcée, même source et mêmes sorties | 264 cas, 474 524 contrôles, 6 570 coupes, 17 refus |
| `chains_o2_root_r1` et `chains_san_root_r1` | Consultations HLD, même source et mêmes sorties | 5 histoires, 922 537 contrôles, 307 500 requêtes, CPU 1/2/4 |

La gate calendrier utilise BFS par seuil comme oracle indépendant, avec
fractions continues pour comparer les rationnels côté oracle contre produit
croisé u128 côté helper. Le renforcement r2 ajoute 128 comparaisons physiques
sous permutation, deux corruptions du calendrier réfutées, une autre forêt
minimale admissible et une instanciation sur rangs u64. Les marques, les
racines déconnectées, les triangles à date égale, les naissances futures,
les identités au-delà de 2^32 et les extrêmes rationnels u64 sont exercés.
Les compteurs d'événements r2 incluent aussi la reconstruction alternative.
La gate HLD compare aux consultations séquentielles de l'histoire : elle
n'est pas un deuxième oracle géométrique indépendant. Ses 9 cas négatifs
signifient **8 refus API/structure et 1 mutation sémantique d'admission**,
pas 9 exceptions.

Les compilations utilisent C++20 et `-Wall -Wextra -Wpedantic -Werror` ;
HLD ajoute `-pthread`. SAN active ASan, UBSan et LSan sans désactivation.
Les commandes et environnements SAN exacts sont conservés. La version du
compilateur est capturée dans les deux runs HLD ; les trois runs calendrier
ne contiennent pas de commande séparée `g++ --version`.

Le premier essai de compilation HLD a échoué à `-Werror=sign-compare` dans
la gate : une borne `std::bit_width` signée était comparée à un compteur u64.
Le cast explicite vers `fc::Id` corrige la gate, sans modifier le helper.
`history/initial_chains_compile_failure.json` conserve le retour combiné de
l'outil et son code 1, pas de faux flux stdout/stderr séparés. Le TU initial
n'avait pas été figé : aucune source initiale reconstituée n'est présentée
comme capturée. O2 r1 conserve, lui, son ancien TU et sa note, y compris
une coquille rédactionnelle corrigée seulement dans r2.

## Lecture et reproduction

Depuis ce dossier :

```bash
python3 -B verify.py
python3 -B -O verify.py
python3 -B verify.py --extract /tmp/filtered-calendar-package-copy
```

Le lecteur ne compile rien, n'exécute pas les gates et n'appelle ni Git ni
GCP. Il vérifie tous les octets, les pins de sources/captures, les flags,
les codes exacts, les flux et les non-vacuités. L'extraction est une copie
physique intégrale dans un répertoire neuf ; on peut y relancer le lecteur.
`MANIFEST.json` associe chaque chemin logique capturé à son stockage
physique pour dédupliquer les fichiers identiques sans perdre les originaux.
Les exécutables ELF sont omis avec taille et SHA ; aucun vendor n'est inclus.
Les empreintes attestent les captures, pas une nouvelle exécution.

Rejouer les sources conservées dans un nouvel environnement est une nouvelle
qualification. Les recorders choisissent un sous-répertoire neuf de `sources/` :

```bash
python3 -B sources/record.py --out replay_o2
python3 -B sources/record.py --out replay_san --san
python3 -B sources/record_chains.py --out replay_chains_o2
python3 -B sources/record_chains.py --out replay_chains_san --san
```

Faire ces replays dans une copie de travail, pas dans ce paquet fermé.
Il faut g++ avec C++20, la bibliothèque standard et les runtimes sanitizers ;
aucune dépendance Boost, CUDA ou réseau. Les sources sont autonomes et ne
consomment aucun header actif de la v7. GCP non utilisé pour ce jalon.
