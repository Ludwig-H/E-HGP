# MES-D6 : objet mathématique et admission des prises

Lecture indépendante du commit `9b2747eff364d56215b589c782b1a4e51d59a576`, 7 octobre 2026.
Cadre : `exploration_v12_hors_registre`, `cpu_reference`, `public_status=not_claimed`.
Aucun calcul HGP, donnée privée, build natif ou GCP. Les JSON sont des doubles publics de sondes ; leurs
`wall_ns` sont des sentinelles de schéma, sans valeur de mesure. Aucune falsification de prise historique alléguée.
La provenance et le plan des comparaisons sont traités dans le reçu voisin `../pilotage/`.

**Défaut concret : le code 0 « contrôles conformes » admet une prise incomplète.** Le vrai `take`, puis le vrai
`main` (seule la construction est remplacée par de minuscules exécutables Python), acceptent P=5 avec seulement
deux passes, aucun ordre G, une ligne `NOT_JSON` et un `cat.bin` composé de 64 octets `X`. `body_sha` rend alors
l'empreinte du corps vide. `main` finit au code 0, trois prises, zéro écart. Deux contre-cas isolés sont aussi
admis : `status=resource_exhausted` explicite avec code processus 0 ; `wall_ns=true` et empreinte `true`.
Cause : `run` ignore les lignes invalides ; `summarize` ne vérifie que code 0, au moins deux durées et une valeur
d'empreinte distincte ; `body_sha` saute 64 octets sans valider le format. Voir pilote lignes 90–124 et 148–166.

Correctif minimal conseillé, avant toute nouvelle qualification : donner à `summarize` les P/K/fils/profil
attendus ; exiger exactement les passes 0..P−1, succès explicites et sortie finale cohérente, nombres entiers
non booléens et non négatifs, métadonnées appariées à la commande, SHA-256 de 64 chiffres hexadécimaux. Exiger
exactement les ordres 1..min(K,sites), uniques, avec les cinq comptes typés ; rejeter les JSON illisibles et
champs requis absents. Valider l'en-tête et les sections de l'export avant de neutraliser le profil dans la
comparaison ; conserver K/sites/trame comme liens à la prise. Une prise refusée doit avoir `chaud_ms=null`
et être exclue des rapports. Ces règles ne changent ni géométrie ni périmètre des chronos.

**Portée de l'invariance.** Pour α positif, sans dépassement de domaine, `x ↦ αx` envoie la boule `(c,R²)` sur
`(αc,α²R²)` ; le signe de `||x−c||²−R²` est multiplié par α². Intérieurs, coquilles, supports minimaux,
incidences et inclusions sont donc conservés, les niveaux étant multipliés par α². Pour les facteurs du pilote
α=2^j, l'entrelacement binaire donne `Morton(αx)=Morton(x) << 3j` : l'ordre des sites et le départage par
PointId sont conservés. La convention S* par positions, les rangs de niveaux et les identifiants catalogue
canoniques restent ainsi transportables. Pour un α entier quelconque, ne pas supposer l'ordre Morton conservé :
il faudrait appliquer la permutation des sites lors de la comparaison.

Multiplier une coordonnée déjà quantifiée par 8 ajoute trois bits nuls, sans information subgrille. Remplacer
« grille huit fois plus fine » par « mêmes coordonnées dilatées par 8, stress de l'étendue arithmétique ».
Une requantification des retours originaux sur une grille plus fine serait une expérience différente.

L'égalité des comptes est nécessaire, mais n'identifie pas les cibles ou les associations : deux hiérarchies
étiquetées peuvent fusionner `{0,1}/{2,3}` ou `{0,2}/{1,3}` avec les mêmes effectifs. Le témoin
`dilation_changed_digest_same_counts` change l'empreinte G à ×8 en gardant les comptes : `checks` l'admet,
conformément à sa portée limitée. Ce n'est pas une preuve que le moteur produit un objet incorrect.
Pour renforcer ce contrôle sans refaire l'oracle, comparer les sections catalogue après division exacte de
SITEXYZ par α ; les supports et coordonnées déterminent les niveaux exacts même s'ils ne sont pas exportés.
Comparer aussi BKEY…TARG pour G sous les identifiants canoniques conservés. CNTR contient des compteurs de travail
en plus des cinq comptes d'objet : l'empreinte G actuelle n'est pas une empreinte purement sémantique.

**Limite documentaire avérée :** le catalogue émet son digest à chaque passe ; G émet ordres et digest seulement
à la dernière (`bench/tower_probe.cpp:193–205`). La phrase du pilote « constante sur ses passes » n'est donc pas
prouvée pour G. Corriger cette portée, ou produire le digest à chaque passe hors chrono ; ne pas rejeter les
sorties G actuelles en leur imposant P digests sans modifier leur producteur.

Rejeu léger depuis le dépôt, Python standard :

```sh
python morsehgp3D_v12/receipts/audit_d6_20261007/math/check.py --out /tmp/d6-normal.json
python -O morsehgp3D_v12/receipts/audit_d6_20261007/math/check.py --out /tmp/d6-opt.json
cmp /tmp/d6-normal.json /tmp/d6-opt.json
```

Les six observations sont identiques en normal/−O ; [verification.json](verification.json) contient la capture
commune, [sources.json](sources.json) les hashes des sources immuables. Le script rend 0 lorsque les observations
sur la version épinglée sont reproduites ; ce n'est pas un verdict de conformité du pilote. Aucun correctif produit
n'est livré par ce reçu. Le contrôle d'identité à ×1 est mathématiquement pertinent une fois ces prises validées.
