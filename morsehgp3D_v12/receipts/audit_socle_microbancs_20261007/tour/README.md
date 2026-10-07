# Audit des microbancs de tour M3 et M4

7 octobre 2026. Pin `95247cf4baf2ebd0856c1ac75670f643d24daa6e`, ajout examiné
`d8407e374`. Cadre `exploration_v12_hors_registre`, `cpu_reference`,
`quantized_u21_input_only`, `full_pi0`, `public_status=not_claimed`.
GCP non utilisé. Aucun gros vidage ni donnée externe lu ; aucune modification
de produit, du canal d'audit ou du registre. Les exécutions sont de petites
portes de microbanc, pas une mesure de vitesse.

**Bilan :** le certificat M3 et la contraction M4 sont conformes aux preuves
relues, sous les préconditions de leurs entrées. La garde de codage de
`CST-0212` est effective dans M4. Un défaut du juge est reproduit : une section
`FLOWER` absente rend le contrôle T6 silencieusement vide, avec code final 0
(`CST-0214`). Cela ne réfute pas l'algorithme de T6 lorsque ses données sont là.

Dans les références ci-dessous, `M/` désigne
`morsehgp3D_v12/microbancs/mes_m3_m4_tour/`.

## Contrôles exécutés

Quatre binaires ont été compilés depuis les sources inchangées du pin, avec
GCC, C++20, `-O1`, avertissements stricts et pthread. M3 lie la bibliothèque
v11 préexistante, dont SHA-256
`050532a95c322cc2d064eef5bfcba74b3920227f98dc21d157a0d675732cf7ca`
est identique au reçu historique de ce microbanc. Le moteur v11 n'a pas été
reconstruit. `builds.json`, `gates.json` et `sources.json` conservent commandes,
compilateur, empreintes des binaires, bibliothèque et sources.

| Contrôle | Résultat |
| --- | --- |
| M3 `--porte`, dix témoins dont carré T1 | code 0 |
| M3 mutant sans `S ⊆ F`, mêmes témoins | code 1, carré causalement faux |
| M4 `--porte 120`, trois plateaux nommés et 120 hypergraphes | code 0 |
| M4 mutant sans contraction, mêmes entrées | code 1 |
| Carré synthétique K1..4, `FLOWER` correct, contraction à deux fils | code 0 ; six naissances T6 contrôlées, zéro écart |
| Même carré, une image T6 fausse | code 1 |
| Même carré, `FLOWER` absent aux ordres 2, 3, 4 | **code 0 ; zéro naissance T6 contrôlée** |

Les JSONL des portes et `m4_t6_sections.json` sont les sorties réellement
obtenues. Le générateur `check_dumps.py` ne lit aucun jeu extérieur : carré de
quatre sites, cinq boules critiques, petits fichiers temporaires supprimés en
fin d'essai. Les empreintes des fichiers synthétiques sont conservées.
Le script a aussi été exécuté avec Python `-O` : mêmes trois verdicts explicites,
sans dépendre d'`assert`. Pin, sources, manifeste, script et binaire sont contrôlés
avant/après chaque exécution (`m4_t6_sections_optimized.json`).

## M3 : certificat complet et repli

`M/mes_m3/meb_cert.hpp:245–258` trie la proposition, refuse répétitions et
absence d'un site de `S` dans la partie triée `F`, puis teste l'appartenance de
tout `F` à `I_b ∪ U_b` avant le succès T1. Le contrôle `S ⊆ F` précède aussi
le certificat géométrique : un support étranger ne peut donc pas certifier une
autre boule par la seconde voie. La porte du carré exige la route de repli et
la raison `s_hors_de_f`, en plus de l'identité exacte (`mes_m3.cpp:138–172`).

Le certificat géométrique contrôle q3 strictement aigu, q4 strictement
intérieur, puis le côté exact de tous les sites (`meb_cert.hpp:194–235,276–311`).
Une proposition dégénérée, non stricte ou non englobante retourne au MEB exact
v11 (`:321–333,363–370`). Le flottant peut donc faire varier le travail, pas
la décision géométrique. La canonisation examine les sites de `F` réellement
sur la sphère, dans l'ordre cardinal puis lexicographique ; si leur nombre
égale celui d'un support affine indépendant strict, il n'existe pas de support
propre et la canonisation omise est justifiée (`:217–228`).

Le raccourci T1 exige un catalogue sémantiquement exact. Il n'en re-prouve ni
la complétude ni les populations : c'est normal pour ce microbanc alimenté par
la v11 figée. Le juge compare centre, niveau, support local et identification
au catalogue (`:430–447`), avec `bounded_meb` comme référence et repli partagé.
Ces contrôles ne constituent pas une nouvelle preuve indépendante de toutes
les primitives numériques de la v11.

Le support local reste ordonné par `SiteIdx` de Morton afin de reproduire la
v11. C'est le domaine annoncé de M3 ; ce n'est pas encore la qualification du
départage v12 par coordonnées, ni une clôture de `WIT-TRANSL`, ni un port u32.

## M4 : T4 et domaine des opérandes

Le noyau (`M/mes_m4/mes_m4.cpp:92–138`) enregistre les sommets courants avant
chaque union réussie, unit par taille et conserve le minimum canonique dans un
champ distinct. Les attaches historiques sont séparées des pointeurs compressés.
La contraction séquentielle relie les événements producteurs/consommateurs de
même rang, puis numérote les classes par rang et minimum de naissance
(`:151–233`). Les enfants internes au plateau sont éliminés. La version à deux
fils, dont les tranches sont alignées sur les rangs, est identique sur le carré
exécuté ; les écritures par enfant ont un propriétaire unique dans la forêt.

**CST-0212 peut être clos dans le périmètre de ce microbanc.** La limite
`nb≤2^31−1` est définie et vérifiée avant tout `resize` du noyau (`:44–55,93`)
et avant conversion du compte u64 lu dans le fichier (`:639–648`). Avec cette
limite, les événements et leurs opérandes ne touchent ni le bit de genre ni
la sentinelle ; les nœuds finaux sont au plus `2nb−1≤2^32−3`. Les offsets de
représentants et d'enfants sont u64. La porte teste réellement les refus
`nb=2^31` et `nb=2^32−1`, en vérifiant que les tableaux restent vides
(`:532–548`). La borne admise est vérifiée par prédicat, sans allouer plusieurs
milliards de cellules. Le produit futur devra porter et requalifier ces gardes.

La garde ne qualifie pas tous les contenus possibles d'un vidage corrompu ;
les preuves T4 supposent toujours des représentants et rangs valides. Les
limites de ressources et la robustesse générale des lecteurs sont distinctes
de cette clôture du codage.

## T6 correct avec données présentes ; CST-0214 confirmé

`M/mes_m4/mes_m4.cpp:775–782` traduit les sommets bruts par `f.nid`, donc par
le quotient final du plateau. La lecture T6 (`:753–766`) remonte au plus un
parent de même rang. La branche où la cellule inférieure est elle-même une
naissance est bien conservée.

Le carré en donne un contrôle géométrique précis : quatre naissances à K2,
une naissance au cercle à K3, puis une naissance au même cercle à K4. Le dernier
cas prend la branche `ball_birth_node` et publie
`images_par_naissance_basse=1`. Les six images sont correctes ; modifier une
image dans `FLOWER` tue la comparaison. La porte contrôle ici les **images de
naissances**, comme annoncé, pas les images de fusions ni la naturalité complète.
T5 `component_at` n'est pas implanté/qualifié par ce microbanc : écrire les
attaches ne suffit pas à qualifier les requêtes d'historique.

**CST-0214 — majeur pour le juge, nouveau, ouvert.** La condition
`k >= 2 && prev.valid && forest.has("FLOWER")` (`:748`) saute tout le bloc si
la section manque. Le code final reste 0, la forêt est déclarée identique et
le compteur de naissances jugées reste 0. Le README du format décrit pourtant
`FLOWER` pour chaque ordre au moins deux (`M/README.md:148`), et le contrôle T6
fait partie du verdict annoncé. Le défaut est un succès sans preuve, pas une
image calculée incorrectement.

Correction attendue : requérir `FLOWER` à chaque ordre `k≥2`, contrôler sa
taille et exiger `t6_checked==nb` avant le verdict. La future porte doit rejouer
les trois variantes présentes dans ce reçu : correct, image fausse, section
absente. Le dernier cas doit rendre un refus explicite, jamais un succès.

## Portée et rejeu

Les gros comptes du reçu initial (29 millions de parties, 17 millions
d'images) n'ont pas été rejoués ici. Les temps locaux de ce reçu ne permettent
aucune adoption sur G4. La réplication, les juges Python et la provenance des
campagnes font l'objet de l'autre volet d'audit.

Pour le témoin autonome, depuis la racine du dépôt :

```sh
g++ -std=c++20 -O1 -Wall -Wextra -Wpedantic -Werror -pthread \
  morsehgp3D_v12/microbancs/mes_m3_m4_tour/mes_m4/mes_m4.cpp -o /tmp/audit_mes_m4
python3 morsehgp3D_v12/receipts/audit_socle_microbancs_20261007/tour/check_dumps.py \
  --binary /tmp/audit_mes_m4
python3 -O morsehgp3D_v12/receipts/audit_socle_microbancs_20261007/tour/check_dumps.py \
  --binary /tmp/audit_mes_m4
```

Le script exige les comportements observés au pin audité ; après correction,
son attente du cas absent devra passer de code 0 à un code de refus. Les sources
et le script sont hachés ; les binaires restent hors du dépôt.
Le script exige les sources du manifeste au pin enregistré ; un HEAD ultérieur
est admis si ces sources sont identiques et si HEAD reste fixe pendant le test.
