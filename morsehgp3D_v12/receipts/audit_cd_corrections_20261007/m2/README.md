# Contre-audit des corrections MES-M2 — 7 octobre 2026

Épingle auditée : `07ee13ef6bebc0b6b85da90207f3f067ffff1755`.
Correctif examiné : `320db4a125a21c0c71c7a6e18af21aaff9f72da5` ; les fichiers M2
concernés sont identiques entre ce correctif et l'épingle. Les commits ultérieurs
ne sont pas qualifiés par ce reçu.

`phase=exploration_v12_hors_registre`, `backend=cpu_reference`,
`quantification=quantized_u21_input_only`, `public_status=not_claimed`.

**Conclusion : CST-0215 peut être clos pour les défauts d'admission documentés.
CST-0018 progresse, mais reste ouvert même sur M2 : trois preuves contradictoires
conduisent encore à une adoption. Aucune clôture transférée à M5 ou M6.**

## CST-0215 : corrections confirmées

Le nouveau `dump_format.hpp` contrôle l'en-tête et la taille physique avant les
allocations dépendant des comptes (`:219`, `:415`), le profil du binaire, K, les
limites, le pavage des feuilles, les indices, les statuts, les compteurs et les
plages de populations (`:282–385`). Le pavage remplace l'ancienne addition non
contrôlée `begin + m` (`:306`). Le besoin `p+m` est vérifié contre la plage restante
avant lecture de la population (`:357`). L'objet est remis à zéro avant lecture
(`:393`). Les tailles bornées de l'en-tête maintiennent les produits et sommes
de sections dans `u64` ; aucune allocation géante n'a été tentée.

Une seule TU C++20 Release contient le vrai lecteur et **le vrai point d'entrée de
`host/leaf_identity.cpp`**, renommé par macro pour l'appeler sans second build.
Le corps produit reste inchangé. Dix-huit lignes de contrôle sont vérifiées :

- Un vidage singleton valide est admis ; les formes J3 et cohérente donnent
  l'identité avec code 0. Une référence q2 à deux sites est admise par le lecteur.
- Les six fichiers invalides historiques sont tous refusés.
- Les six fautes sont aussi isolées sur une base maintenant valide : SiteIdx hors
  nuage, `begin=UINT64_MAX`, K nul, profil 24 lu par binaire 21, coordonnées hors
  profil et population d'un enregistrement trop courte. Chaque **motif précis**
  est contrôlé. Ce second lot est essentiel : l'ancien témoin avait
  `leaf_size=1`, désormais hors domaine, ce qui masque plusieurs autres fautes.
- Sur chacun de ces douze fichiers, le vrai point d'entrée rend **code 2 sans
  ligne de résultat de forme**, seul puis précédé du fichier valide : aucun
  résultat partiel avant le refus. Cela exerce l'admission de tous les vidages
  avant les noyaux (`leaf_identity.cpp:229–241`). Les fichiers invalides ne sont
  passés à l'outil qu'après confirmation de leur refus par le lecteur.
- Quatre contrôles voisins sont refusés : en-tête géant dans un fichier minuscule,
  statut sans référence, compteur d'émissions incohérent et population répétée.
  Pour l'en-tête géant, les vecteurs restent vides : refus avant allocation.

Les fichiers ont un FNV correct ; au plus **352 octets**. Les 25 appels du point
d'entrée original restent sur ces seules entrées synthétiques. La relecture et
le contrôle d'empreinte avant calcul sont présents dans `leaf_identity.cpp:248–253`.
Les chemins MES-S, arène et CUDA appellent aussi le lecteur durci, vérifiés par
lecture du code ; ils ne sont pas tous reconstruits ou exécutés dans ce lot.

La clôture proposée concerne l'admission, pas l'exactitude géométrique générale,
les autres profils, la capacité mémoire globale ou l'exécution CUDA. Pas de
campagne des 46 mutants ni de sanitizer rejouée localement.

## CST-0018 : corrections du pilote confirmées

Treize simulations passent par le **vrai `main()` et le vrai juge** de
`scripts/g4_leaf_bench.py`. Les fabriques de fichiers et doubles de commandes du
test officiel `tests/test_juge_m2.py` sont importés et épinglés ; les mutations et
attendus du présent reçu sont indépendants. Son `main()` n'est pas appelé : aucun
reçu de mesures réelles n'est relu. Tout processus externe éventuel est interdit
durant ces simulations. Le bootstrap conserve ses 10 000 tirages.

Huit situations auparavant dangereuses conduisent maintenant à `refuse`, sans
choix : identité hôte fausse, outil hôte de code 2 avec lignes pourtant positives,
ancien fichier d'un autre dump non réécrit, mauvais nonce, binaire modifié, grille
sans ng02, admission native refusée, temps non finis. Après refus d'admission, ni
identité ni prise de banc ne sont lancées. L'ancien fichier est effacé et son
absence devient explicitement une raison de refus.

Contrôles positifs : le scénario complet donne `adopte`, choix J3 et ratio 0,1.
Des temps J3 bruts de 60 ms avec médiane déclarée de 6 ms sont jugés depuis les
valeurs brutes : face au témoin de 100 ms, ratio 0,6 et `rejete`. Le correctif ne
se fie donc plus à une médiane déclarée pour sa statistique.

## CST-0018 : trois fausses adoptions résiduelles

Ces défauts restent rattachés à CST-0018, gravité majeure pour la qualification.
Chaque injection est isolée et conserve les autres preuves conformes.

| Injection | Observation du vrai pilote |
| --- | --- |
| Les trois JSON Compute Sanitizer ont `forms=[]` et `leaves=0`, mais gardent nonce, FNV et `identity=true` | Les trois preuves sont déclarées conformes ; `adopte`, choix J3, aucun refus. |
| La prise d'échauffement rend code 1 alors que toutes les identités de son JSON sont vraies | `bench_discarded.proof=true`, puis `adopte`, choix J3, aucun refus. |
| Les formes déclarent `identity=true`, mais aussi `mismatched_counts=1` et `overflow=true` | Prises admises, `adopte`, choix J3, aucun refus. |

Causes précises :

1. `scripts/g4_leaf_bench.py:757–770` ne recoupe pour Compute Sanitizer que nonce,
   code/identité, nombre de cas et FNV. Ni formes effectivement jouées, ni nombre
   de feuilles, ni autres paramètres de la commande ne sont exigés.
2. `:787–794` accepte les codes 0 et 1 pour l'échauffement et n'applique pas la
   cohérence code/identité pourtant contrôlée pour les prises normales (`:813`).
3. `check_bench_json`, `:421–460`, valide identité comme booléen et compte de
   feuilles non résolues comme entier positif ou nul, mais ne vérifie pas sa
   cohérence avec les écarts et débordements pourtant publiés dans la même forme.

Corrections attendues : une validation commune des prises, paramétrée par la
couverture autorisée pour Compute Sanitizer ; couverture et formes conformes à
la commande ; même contrôle code/identité sur l'échauffement ; compteurs et état
de débordement cohérents avant admission. Ajouter ces trois mutations aux portes.
Nous établissons une admission de **preuves contradictoires**, pas une panne des
noyaux ni une fausse mesure lors d'une campagne historique réelle.

## Rejeu et empreintes

```sh
python3 morsehgp3D_v12/receipts/audit_cd_corrections_20261007/m2/run.py
python3 -O morsehgp3D_v12/receipts/audit_cd_corrections_20261007/m2/run.py
```

Captures normales et `-O`, contrôles par exceptions explicites, sans `assert`.
Chaque rejeu compile une seule petite TU Release, vérifie exactement ses 18
lignes et joue les 13 simulations. Les temporaires sont supprimés ; aucun binaire
ni dump conservé dans ce reçu.

`MANIFEST.json` épingle 14 sources et les trois témoins. Le lecteur vérifie leurs
hashes avant et après, ainsi qu'un HEAD stable. `-MMD` ferme les six en-têtes locaux,
la source originale d'identité et la TU ; les en-têtes système ne sont pas
épinglés, le compilateur et le binaire sont identifiés dans les captures.
Aucun appel GPU/GCP/réseau, aucune donnée réelle ni modification produit.
