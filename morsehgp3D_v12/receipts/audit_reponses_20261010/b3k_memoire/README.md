# B3-K : compte mémoire et proposition locale

10 octobre 2026 ; pin `2aaed1847e63ff86550db1fb1ba6e36313aa58bc`.
Exploration v12 hors registre, CPU de référence / CUDA G4 pour le catalogue,
FULL π₀, quantized_u21_input_only, public_status=not_claimed.
Lecture de 22 sources épinglées et modèles Python seulement : aucun moteur,
compilateur, test natif, cloud ni donnée de nuage exécuté ou copié.

**Aucune omission de compte ou d'admission B3-K démontrée.** Les clés hôte
occupent nominalement 16 octets par boule, dans `Buffer<TableKey>` :

| Voie | Compte et coexistence relus au pin |
|---|---|
| CPU complète | `finish_driver.hpp:248` alloue via le budget ; `slices.hpp:53` ajoute les clés à la borne supérieure. `exec_host.hpp:107–115` ne peut adopter `Key2` vers `TableKey` : la copie et ses deux tableaux restent comptés. |
| GPU complète | `finish_outputs.hpp:137` réserve les clés hôte, premier toucher et `outputs_bytes` compris ; le flux compte leur rapatriement. Les clés GPU préexistaient dans les tableaux du tri. |
| CPU/GPU découpés | `slices.cpp:403` alloue les clés sous budget après les tranches. `sliced_host_bytes` décrit seulement les sorties pendant les tranches ; chaque réservation reste contrôlée. |
| Tour FULL | Le catalogue et ses clés vivent déjà dans le budget hôte utilisé par la tour (`full_probe.cpp:292–299,457`). Les ajouter à `region_bytes` les compterait deux fois. |

Les trois sources visées par la proposition CST-0244 sont identiques à
`aa6338ee8` : port direct, sans qualification nouvelle ni fermeture CST-0245.
Les portes de pénurie existantes contrôlent limites et libération, mais ne
localisent pas un refus à la nouvelle allocation des clés. La référence
`device_finish_test.cpp:140–165` ne compare pas directement `table_keys` ;
les identités globales recherchent chaque support (`device_support.hpp:280`)
et les quatre mutants B3 couvrent aussi transmission et clés découpées.

## Proposition isolée, non appliquée

[proposition.patch](proposition.patch) ajoute seulement `cursor.reset()`
après la dernière utilisation séquentielle du curseur dans `host_table`.
Les callbacks `TableRows` et `TableKeys` ne le reçoivent pas. Le Pool attend
ses ouvriers avant retour (`sched/pool.cpp:80–105`) ; aucune tâche ne conserve
cet alias. Refus/allocations des clés restent au même emplacement.

Sans cache, sans clés déjà anticipées et à autres tableaux vivants constants
`D`, avec `S` sites et `B` boules, le pic **local de réservations** devient
`D + max(8S,16B)` au lieu de `D + 8S + 16B`. Réduction `min(8S,16B)` ;
4 225 couples synthétiques vérifiés. Ce n'est ni une trace native, ni une
preuve de baisse du pic de construction entière ou du contrat LiDAR massif.

**Cache analysé explicitement.** La sonde FULL active par défaut 8 Gio de
cache (`full_probe.cpp:90`), tandis que `MemoryBudget` seul le désactive par
défaut. `reset` retire la réservation vivante de `used`, mais un bloc de
classe conservé reste dans `held`/les inactifs (`buffer.cpp:130–145`).
L'acquisition suivante peut le réutiliser ; sous pression, `hold` évince les
inactifs avant le refus, puis l'acquisition tente la taille exacte si la
classe ne tient pas (`169–218`). Une restitution au cache ne garantit donc
**aucune baisse de RSS, de mémoire engagée ou de held**. Même le pic de
`used` n'a pas de gain monotone universel : libérer le curseur peut permettre
une classe arrondie des clés là où l'ancien chemin prenait la taille exacte.
Le modèle donne alors 1 270 160 → 1 286 720 octets. Trois témoins comptables
couvrent réemploi, éviction évitant un refus et ce changement de classe ;
ils supposent un seul pilote et aucune panne de l'allocateur système, sans
affirmer l'existence d'une fixture géométrique correspondante.

Une éventuelle adoption demandera une porte native ciblée sur le refus des
clés et les restitutions, avec/sans cache. Pas de nouveau constat majeur ;
amélioration conditionnelle de coexistence, aucune mesure de temps acquise.

Relecture sans écriture : `python check.py /workspaces/E-HGP`, puis `python -O`
avec les mêmes arguments. [capture.json](capture.json) épingle sources,
patch et postimage ; [SHA256SUMS](SHA256SUMS) ferme ce petit reçu.
