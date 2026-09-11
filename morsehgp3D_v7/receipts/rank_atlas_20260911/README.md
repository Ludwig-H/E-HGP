# Atlas exact privé : rangs, admissions et représentants

Prototype CPU **non intégré**, sous précondition du census complet exact
accepté par FULL. Aucune nouvelle autorité de géométrie, de complétude WSPD,
de forêt ou de performance ; `public_status=not_claimed`. GCP non utilisé.

Le helper [rank_atlas.hpp](rank_atlas.hpp) emprunte un seul catalogue
`BallData`, prépare ses rangs rationnels communs et ses fenêtres d'admission,
puis conserve les représentants sous forme de masques u16 avec offsets.
Voir [README.source](README.source) pour le contrat complet et ses limites.

## Résultat borné

O2 strict et ASan/UBSan/LSan ROOT : 11 commandes chacun, sources stables,
mêmes stdout/stderr sur les neuf commandes d'exécution. La compilation
utilise C++20, `-Wall -Wextra -Wpedantic -Werror`, un seul compilateur, sans
Boost. L'O2 a zéro avertissement ; aucun exécutable CUDA n'est produit.

La gate couvre 13 vrais census, dont n32 K1..10 avec s=8/10/12 et permutations,
square8 avec intérieurs/extra-shells, ABCZ, line3 et terminaux n=2/n=1 :

- 16 324 boules empruntées, 30 562 blocs et 52 469 représentants comparés ;
- 25 tables extra préparées et 82 rangs locaux extraits, une seule fois dans
  chaque atlas ; 14 212 naissances de boule et 226 naissances ponctuelles ;
- 66 couples de niveaux rationnellement égaux mais de représentation brute
  différente ; 19 coquilles extra dont l'ordre original n'est pas PointId trié ;
- 22 265 350 contrôles, dont un grand nombre de comparaisons pair-à-pair de
  rangs **uniquement dans la gate bornée**, pas dans le helper proposé.

Les six mutations d'état sont réfutées avec code 4 et cause exacte : date,
offset, masque, contribution, admission, permutation. Masque et contribution
mutés passent les contrôles de forme avant leur réfutation sémantique.
CLI inconnue/absente : code 2. Il ne s'agit pas de mutants de géométrie.

## Structure et portée de la preuve

La référence d'origine est le core FULL
`83f1c78e0656f08cd42522e4cd36d153ce283a6082246a36fe5225b3790c6366`, conservé
dans [base_full_ball_tower.hpp.source](base_full_ball_tower.hpp.source).
La copie privée dans `source/` ne diffère de ce fichier que par UNE déclaration
friend : [friend.patch](friend.patch). Le friend appelle les véritables
`validate_catalogue`, `programs` et `visit_block`. Aucune résolution terminale,
forêt ni carte verticale n'est comparée ici. Le validateur du catalogue est
celui de la référence, pas un oracle nouveau et pas un test de complétude.

Le fichier de base et tous les headers locaux sont présents, sans ELF ni
vendor. Les dépendances système C++/libc ne sont pas redistribuées ; le
compilateur est épinglé, la commande et les dépendances de projet `.d` sont
conservées. Ce paquet n'est pas une fermeture hermétique du système.

Les rangs entiers ne remplacent pas les niveaux bruts de chaque boule ni le
premier niveau brut des lots de chaque K. Le masque représentant est exprimé
dans l'ordre original BallData ; le masque de contribution reste dans l'ordre
PointId trié du contrat. Les identifiants de bloc internes ne sont pas les
numéros de populations/nœuds exportés.

Stockage O(C+A+R+K), zéro copie possédée de BallData. Sur ces cas, maximum
152 296 octets logiques / 173 256 octets de capacité persistante ; catalogue
emprunté maximum 605 024 octets, compté séparément. Ces maxima excluent
temporaire quotient/tri, contrôle de forme et référence de gate. **Ce n'est
pas le pic RSS ni un gain mémoire ou temporel industriel.** Pas de borne
sous-quadratique universelle sur A ou R.

Les uniques/scatter, terminaux, graphe filtré, composantes et contributions
datées, verticales et IDs de première utilisation restent à raccorder et à
qualifier. L'égalité physique complète des forêts n'est pas revendiquée.

## Lecture et reproduction

Depuis la racine du dépôt :

```bash
python3 -B morsehgp3D_v7/receipts/rank_atlas_20260911/verify.py
python3 -B -O morsehgp3D_v7/receipts/rank_atlas_20260911/verify.py
python3 -B morsehgp3D_v7/receipts/rank_atlas_20260911/record.py --out replay_o2
# Nouvelle destination, seulement après autorisation de la session CPU/SAN :
python3 -B morsehgp3D_v7/receipts/rank_atlas_20260911/record.py --out replay_san --san
```

Le lecteur vérifie les empreintes, la seule ligne friend, les sources avant/
après, les onze commandes et codes attendus, les causes et la non-vacuité,
puis l'identité O2/SAN des sorties de gate. Il ne refait pas une preuve
géométrique en lisant du JSON. Les pins historiques `README.md` se résolvent
vers `README.source`, sans réécriture des captures. Le recorder reproduit les
sources C++ dans une nouvelle destination ; les commandes absolues anciennes
restent historiques et ne doivent pas être utilisées pour écraser les runs.
