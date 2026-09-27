# Construction directe des bandes — résultats appariés

27 septembre 2026. Autorité : `r2/capture.json`, **completed, 30 commandes**,
quinze mesures appariées. Code/propriété/preuve :
[audit direct](../../audits/b_q34_direct_bands_20260927/README.md).
GCP non utilisé. Moteur inchangé, prototypes CPU mono hors registre,
`not_claimed` : **pas de gain chronométré du moteur, de S2, de FULL ou de G4**.

La tentative initiale à la racine est conservée **failed** : douze commandes,
puis arrêt de LeakSanitizer sous ptrace. R2 emploie exactement les mêmes
sources dans deux builds neufs, hors sandbox ; ASan, UBSan et LSan restent
activés. Aucun défaut du code n'est déduit de cette erreur d'environnement.

## Qualification

Gates Release et Clang ASan/UBSan/LSan identiques : 18 928 cas,
4 457 784 paires, 1 439 204 cellules, 96 fronts, 17 488 vrais rectangles,
56 refus. Les vingt compteurs de préparation/géométrie sont comparés sur
chaque rectangle (349 760 champs dans chaque gate). Les vecteurs de
propositions, crédits, rangs groupés et classes sont comparés intégralement.

Trois mutations natives tuées dans chaque build, en code 1 et par leur
réponse erronée, non par un crash : bande dupliquée, masque 6 abusif dans
le décodeur du propriétaire, rang traité comme ID original. La construction
directe doit déclarer zéro ancienne cellule, en plus de l'égalité des
masses et de tous les masques de paires.

Le lecteur lie les commandes à leur recette, vérifie les hashes avant/après
et recontrôle les entrées existantes sans copier KITTI. Normal/−O et huit
mutations du lecteur sont enregistrés sous `r2/checks/`. Le harnais du
prototype précédent est réutilisé explicitement et épinglé ; il appelle
**le nouveau lecteur**, pas celui de l'ancien protocole.

## Temps réellement appariés

Chaque rectangle est construit dans l'ordre **ancien, direct, direct,
ancien**. Les colonnes AB/BA ci-dessous sont les sommes des constructeurs
sur tous les rectangles préparés. Allocations incluses ; vérification,
destruction et front/filtre partagé exclus de ces colonnes et payés dans le
mur complet. Ce ne sont pas deux répétitions indépendantes de tout le
pipeline. L'hôte CPU est partagé.

| cas | ancien AB / BA, ms | direct AB / BA, ms | ratio des moyennes |
| --- | ---: | ---: | ---: |
| sans-sol 08/000000 K5/s8 | 998,728 / 951,377 | 661,538 / 614,168 | 1,529 |
| sans-sol 08/000100 K5/s8 | 662,204 / 624,261 | 454,514 / 410,144 | 1,488 |
| sans-sol 08/000200 K5/s8 | 1151,682 / 1091,781 | 774,337 / 705,659 | 1,516 |
| sans-sol 08/000000 K10/s8 | 2028,178 / 1970,986 | 1251,522 / 1188,756 | 1,639 |
| sans-sol 08/000000 K5/s10 | 884,570 / 841,438 | 579,870 / 532,907 | 1,551 |
| sans-sol 08/000000 K5/s12 | 834,891 / 789,083 | 552,373 / 509,723 | 1,529 |

Les trois scènes ci-dessus sont trois trames de **la seule séquence 08**,
pas plusieurs séquences. Elles gardent leurs 39 885 / 35 551 / 45 845 sites
sans sol, grille 1 mm. Masques figés, aucune segmentation ni comparaison
aux labels refaite. Tous les sept morceaux et IDs de chaque trame sont
contrôlés pour la provenance, seules les trames entières sont mesurées ici.

Ces résultats comparent deux **préparations complètes de prototype**.
Le corps porté appelle `certify(..., None)` avec un mode constant, tandis
que la référence garde le branchement de mutant du prototype historique.
Les compteurs géométriques sont égaux mais le code machine peut différer
(spécialisation/inlining notamment). **Ne pas attribuer les 337 ms d'écart
moyen de 000000 aux seules bandes.** Un contrôle avec exactement le même
corps compilé de préparation serait requis pour cette attribution causale.
Le gain n'est pas transférable au moteur/GPU, qui n'utilise pas encore ce
prototype de Pool. Les fortes différences AB/BA sur les minuscules plans
synthétiques confirment également la sensibilité au cache et à l'ordre.

| synthétique K5/s8 | ancien moyen, ms | direct moyen, ms | E, identique |
| --- | ---: | ---: | ---: |
| uniforme 8k | 0,457 | 0,359 | 435 709 |
| uniforme 16k | 1,158 | 0,912 | 908 050 |
| uniforme 32k | 2,241 | 1,742 | 1 876 820 |
| terrain 8k | 0,453 | 0,399 | 140 079 |
| terrain 16k | 0,822 | 0,692 | 285 967 |
| terrain 32k | 1,130 | 0,900 | 591 278 |
| amas 8k | 52,593 | 30,705 | 2 091 410 |
| amas 16k | 104,310 | 57,880 | 7 787 691 |
| amas 32k | 205,692 | 112,561 | 30 699 080 |

Les masses P/E/E3/E4, F, les propositions et les tests de coins retrouvent
exactement les quinze reçus Pool publiés. Sur amas, E fait encore ×3,724
puis ×3,942. La croissance quasi quadratique du **résidu** n'est pas
corrigée par le changement de représentation, même lorsque la préparation
et son stockage progressent.

## Mémoire complète, pas seulement les descripteurs

Sommes des capacités des objets complets, séquentiellement détruits, Mo
décimaux. Elles ne représentent ni une RSS ni une VRAM de pic.

| cas | ancien → direct, Mo | descripteurs seuls, Mo |
| --- | ---: | ---: |
| 000000 K5/s8 | 125,200 → 101,118 | 34,810 → 4,913 |
| 000100 K5/s8 | 105,362 → 85,697 | 28,773 → 3,939 |
| 000200 K5/s8 | 160,840 → 129,030 | 45,478 → 6,239 |
| 000000 K10/s8 | 178,802 → 87,753 | 102,704 → 8,369 |
| 000000 K5/s10 | 116,060 → 92,709 | 33,207 → 4,602 |
| 000000 K5/s12 | 112,333 → 89,137 | 32,670 → 4,434 |

À K5/s8 sur 000000 : 659 165 anciens blocs remplacés par 331 733 bandes ;
1 046 663 tests de cellules anciennes réellement supprimés côté direct.
Le regroupement paie à la place 297 784 classes B, 636 419 tests de lignes
et 330 498 étapes binaires ; les tables marginales ajoutent exactement
22 cases et dix étapes par rectangle préparé. Les 103 840 plans gardent
**934 560 buffers retenus dans les deux cas** : aucune réduction du nombre
de ces buffers n'est acquise. Ce compteur n'est pas le nombre d'appels
malloc/réallocations, qui n'est pas instrumenté.

Le plus gros objet K5/s8 passe de 14 370 à 11 108 octets ; K10 de 50 996
à 13 164. Les deux objets coexistent pendant la comparaison, donc leurs
maxima individuels ne donnent pas le pic réel du processus de test.

## Limites et suites

Pour 000000 K5/s8, le mur complet de la sonde est **21,690 s**, dont
14,246 s de filtre rectangle et 122,980 ms de vérification. Il paie quatre
constructions par rectangle et n'exécute ni S2 sur les paires résiduelles,
ni les lanes, ni FULL : ne pas en déduire un nouveau contrat de tour.
K10/s8 et s10/s12 sont aussi publiés sans masquer leur hausse du front.

Prochaines preuves nécessaires avant promotion : contrôler l'attribution
du gain de préparation, arène collective, ordre/sidecars conservés, coût
total avec S2 et aval, puis même comparaison G4. Les anciennes cellules ne
doivent pas être réintroduites comme oracle dans le chemin produit.

Builds désormais épinglés :

- `build/v9-audit-q34-direct-bands-20260927_{release,sanitize}` : échec initial ;
- `build/v9-audit-q34-direct-bands-r2-20260927_{release,sanitize}` : autorité r2.

Sources, archives générateur et entrées existantes sont liées par hashes ;
les lecteurs sont LIVE et exigent leurs chemins absolus. Aucun octet KITTI
ni binaire de build n'est ajouté aux fichiers versionnés de cette capture.
