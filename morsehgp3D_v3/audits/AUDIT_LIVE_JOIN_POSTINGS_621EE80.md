# Audit constructif — join postings et solution du transcript Gamma

Date : 10 août 2026 UTC.

Cadre annoncé : `phase=exploration_v3_hors_registre`, `backend=CPU de
référence + oracles bornés`, `profile=quantized_u16_input_only`,
`mode=audit_indépendant_sans_édition_du_prototype`. La porte d'exploration est
satisfaite; aucune phase publique, admission G4 ou promotion d'exactitude n'est
ouverte.

## Périmètre épinglé

L'autorité committée reste
`HEAD=origin/main=2b4801c2b2a7fed0e91dfc8aabed1d11998e8787`. Claude développe
le palier suivant dans un worktree non committé. Le présent audit pince :

| fichier produit | SHA-256 audité |
| --- | --- |
| `CMakeLists.txt` | `1b9e35d513c033aeb84da1a4c98e2d2ec6cdaf0b1fa69ca53a56dfb15dbf8d28` |
| `prototype/saturated_fold.hpp` | `621ee80a8c213612f8b5da221ec6a33c9e4b7c91c082fbf88233d4f53b977f94` |
| `prototype/saturated_pipeline.cpp` | `57f252d30d0e8e88e4dd67740d038b1c2d06ba8a5468960fe0d492186ceabd43` |
| `prototype/postings_join_gate.cpp` | `71538434340dc50f3ca35a59064a7be16ee489c772300ec1f90154e5a589e3f4` |

Aucun de ces quatre fichiers n'a été modifié par l'auditeur. Toute empreinte
différente exige un nouveau pincement avant de réutiliser ce verdict.

## Verdict utile

**GO borné comme join CPU exact relativement au catalogue fourni et équivalent
au fold `G^2` sur les portes exercées. NO-GO comme transcript Gamma, certificat
de source complète, reçu canonique de chaque poids, architecture 50 k ou
admission G4.**

Le résultat positif est substantiel : l'ancien--nouveau, le
nouveau--nouveau, le tri-réduction, le seuil `w>=k`, l'atomicité des lots et la
capture locale des racines strictes forment désormais un candidat cohérent.
Aucun défaut de connectivité n'a été trouvé sur les entrées certifiées des
portes. La limite n'est plus un doute vague sur le join; elle est localisée dans
le transcript, la provenance de la source et le passage à l'échelle.

## 1. Réception CPU positive

En Release, avec GCC 13.3.0 et CMake 4.3.4, les cibles
`mhgp3v_postings_join_gate`, `mhgp3v_saturated_pipeline` et
`mhgp3v_gamma_judge` se construisent sans diagnostic. La commande ciblée :

```text
ctest --test-dir build/v3 --output-on-failure -R '^mhgp3v_(postings_gate_|saturated_pipeline_reject_join$|gamma_judge_fold_(generic|saturated)$)'
```

rend `15/15` tests verts en `8,74 s` : deux portes Gamma historiques, six
fixtures regroupées dans la porte postings, deux campagnes différentielles, six
mutants, trois refus de CLI pour la porte et un refus de join du pipeline.

Les deux campagnes postings rendent :

| campagne | nuages | générateurs | `P_post` | somme des poids | unions réussies | anciens « silencieux » | niveaux |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| générique, `n=8`, graine 7 | 30 | 1 950 | 110 390 | 110 390 | 4 916 | 3 311 | 4 782 |
| grille saturée, `n=9`, graine 31337 | 20 | 1 623 | 129 661 | 129 661 | 4 200 | 1 889 | 2 386 |

Les six mutants nommés meurent avec le code attendu 4 : seuil strict, oubli
nouveau--nouveau, dernière posting omise, membres tronqués, collecte d'un lot
sans croissance et séquentialisation des niveaux égaux. Le refus des membres
`PointId` négatifs est aussi présent avant leur indexation.

Sur le même petit catalogue complet `n=8`, les joins `g2` et `postings`
produisent tous deux 171 niveaux, 31 naissances, 20 fusions, 128 événements
actuellement dits silencieux et le digest diagnostique
`16352832568416173980`. Le candidat postings reçoit en plus
`P_post=poids=4611`, 1 959 paires réduites et 177 unions réussies. Cette égalité
est une bonne réception de câblage; les temps d'un run unique ne sont pas un
benchmark.

## 2. Ce que le join résout réellement

Pour chaque point, l'émission des paires de générateurs de sa posting, puis la
réduction par paire canonique, donne bien le poids exact :

$$w(M,N)=\lvert M\cap N\rvert.$$

Le code émet séparément les paires ancien--nouveau et nouveau--nouveau du lot,
puis n'unit qu'après réduction. La publication des postings intervient après la
fermeture du lot. Cette séquence respecte le niveau d'activation du générateur
le plus tardif et ne dépend pas de l'ordre interne du lot.

La capture par époque est également une bonne réponse au mur
`niveaux*racines`. Chaque racine stricte est enregistrée au premier contact,
avant sa première union; les racines non touchées ne peuvent pas influer sur le
lot. Avec `keep_partitions=false`, la classification n'effectue plus de scan de
toutes les racines vivantes.

Enfin, le candidat évite bien les objets interdits par l'invariant
d'architecture : aucune mosaïque de Delaunay d'ordre supérieur, aucune
énumération des `k`-faces, aucune matérialisation des graphes de Johnson. Il
matérialise en revanche toutes les occurrences d'un lot, leur tri et les paires
réduites; cette masse doit être admise explicitement avant toute revendication
50 k.

## 3. Verrou mathématique résolu pour Claude

La classification par variation de couverture n'est pas réparable par un
nouveau compteur. Une continuation Gamma peut garder exactement la même
couverture. La solution démontrée dans
[`NOTE_SOLUTION_TRANSCRIPT_GAMMA_QMIN_20260810.md`](NOTE_SOLUTION_TRANSCRIPT_GAMMA_QMIN_20260810.md)
est structurelle.

Pour un générateur de boule `B`, saturé `M` et support de cardinalité minimale
`q_min(B)`, il existe une nouvelle `k`-face ou `(k+1)`-coface exactement quand :

$$\lvert M\rvert\geq k\quad\text{et}\quad q_{\min}(B)\leq k+1.$$

Plus fort, sous une source certifiée complète pour l'ordre `k` :

$$\Gamma_k(a)=\bigcup_{\substack{(B,M):\beta(B)\leq a,\ \lvert M\rvert\geq k,\ q_{\min}(B)\leq k+1}}J_k(M).$$

Les générateurs `q_min>k+1` ont leurs `k`-faces et `(k+1)`-cofaces strictement
avant leur propre niveau. Ils peuvent être exclus du `DSU_k` et de ses postings
sans perdre d'attache future. Après fermeture du lot, seules les racines finales
atteintes par un générateur admissible sont marquées; `0/1/>=2` handles stricts
donnent respectivement naissance, continuation et multifusion.

Cette preuve corrige aussi la fixture `A/B/S/N` : `S` n'est pas forcément
silencieux pour Gamma. Si `q_min(S)<=k+1`, c'est une continuation sans
croissance de couverture et sa posting doit rester. Si `q_min(S)>k+1` dans une
source complète, des carriers strictement antérieurs la rendent redondante. En
source partielle, la collecte peut changer la sous-filtration, mais aucun
transcript Gamma exact n'est alors autorisé.

Le chemin `flat_catalogue` calcule actuellement un support canonique de
cardinalité minimale sur la coquille complète; `CriticalSphere.n_support` peut
donc jouer le rôle de `q_min` sur ce prototype borné. Le contrat produit doit
toutefois certifier cette provenance et la complétude par ordre. Les fixtures
synthétiques de la porte forcent `n_support=1` sans géométrie : elles ne peuvent
pas recevoir ce nouveau théorème.

## 4. Pourquoi la porte ne reçoit pas encore Gamma

`build_saturated_fold_postings` active actuellement chaque générateur de
capacité suffisante, sans lire `n_support` ni former
`event_generators_k`. Pour une seule racine stricte, il décide entre croissance
et silence par le cardinal de couverture. Il absorbe donc encore les 87 et 65
continuations Gamma observées dans la catégorie silencieuse.

La porte compare le candidat au fold `G^2` qui porte la même classification.
Ses « transcripts gravés » sont des attentes indépendantes pour cette
sémantique interne, pas une vérité Gamma. La phrase imprimée par le pipeline,
« les lots silencieux sont [...] JAMAIS des continuations Gamma », est donc
fausse sur le snapshot pincé. La formulation sûre est : « les continuations
sans croissance et les activations redondantes ne sont pas encore séparées ».

Proposition de porte scientifique : sur des catalogues géométriques complets,
comparer pour chaque ordre et niveau les nouvelles faces, cofaces, racines
marquées, handles stricts et types `birth/continuation/multifusion` à l'oracle
Gamma exhaustif. Ajouter un cas `q=k+1` multifusion et le cube à supports
multiples. Le mutant central force alors `q_min+1`, au lieu de reproduire le
compteur de couverture du fold `G^2`.

## 5. Reçu de poids à rendre indépendant

Les égalités `occurrences=somme des poids` et `P_post=somme des poids` sont
utiles, mais leurs deux côtés dérivent encore de la même émission. Une
redistribution compensée des poids entre paires, au-dessus des mêmes seuils,
peut préserver masse, unions et partitions.

La petite porte doit construire indépendamment, directement depuis les listes
de membres originales, la table canonique `(M,N)->|M inter N|` et comparer :

- chaque clef et chaque poids;
- le nombre de paires réduites;
- l'histogramme de `min(K,w)`;
- les arêtes acceptées par ordre;
- les masses ancien--nouveau et nouveau--nouveau calculées depuis les degrés
  pré-lot.

L'identité de degré indépendante à calculer avant le join est :

$$L_{\mathrm{sat}}=\sum_M\lvert M\rvert=\sum_x d_x=\mathrm{postings\_mass}.$$

Elle tue une posting omise même si les occurrences et les poids sont tous deux
tronqués de manière auto-cohérente. Le calcul de vérité borné `G^2` reste un
oracle de test; il ne devient pas l'architecture produit.

## 6. Préflight et mémoire 50 k

Le candidat conserve simultanément le vecteur complet `occurrences` du lot et
le vecteur `weights` après tri-réduction. `P_post` n'est calculé qu'après tous
les lots. Il n'existe encore ni entier large vérifié, ni budget, chunks, merge
de runs, high-water ou reprise.

Une seule posting dense de 50 000 générateurs contient :

$$\binom{50000}{2}=1\,249\,975\,000$$

paires, soit environ 10 Go bruts à huit octets par clef avant le second buffer,
le tri et les DSU. La bonne réponse n'est pas un `reserve` tardif :

1. compter les degrés et `L_sat` en arithmétique vérifiée;
2. calculer `P_post` et le pic conservateur avant toute émission;
3. refuser si le budget ou un offset déborde;
4. sinon émettre des runs bornés, trier, merger et réduire avec reprise;
5. publier prédit/réel, paires uniques, histogrammes et high-water par lot.

Cette étape décide objectivement si un kernel GPU mérite d'exister. Le code
audité est CPU-only; une G4 mesurerait aujourd'hui l'hôte et n'aiderait pas ce
verrou.

## 7. Contrats fail-closed à fermer

Trois frontières restent à traiter avant d'exposer l'API :

- `maximum_order<1` est converti en taille de vecteur, tandis qu'une valeur
  extrême peut aussi faire déborder les boucles; borner `K` avant toute
  allocation;
- `max_point+1` peut déborder pour `PointId=INT_MAX` et le tableau dense coûte
  `O(max(PointId))` sur des identifiants clairsemés; recevoir `n` et vérifier
  `[0,n)`, ou compresser les identifiants;
- le reçu n'est assigné qu'en succès; le remettre à zéro à l'entrée afin qu'un
  refus ne laisse pas observable un ancien `identities_ok=true`.

Tous les compteurs de masse sont encore en `long long`. La même arithmétique
checked que celle du préflight doit couvrir additions, produits, offsets et
conversion de `count` vers les identifiants `int`.

Enfin, `smax>=n` prouve seulement l'absence de censure par ce cap. Il ne prouve
pas `support_universe_complete`, `saturated_family_complete` ou
`join_complete`. Ces certificats, ainsi que `q_min_certified`, doivent être des
champs de provenance, pas une phrase inférée par le pipeline.

## 8. Séquence recommandée

1. Intégrer la fenêtre `q_min` et recevoir le transcript contre Gamma sur de
   petits catalogues géométriques complets.
2. Ajouter l'oracle indépendant de chaque poids et les identités de degré.
3. Fermer `K`, domaine des `PointId`, reçu de refus et arithmétique checked.
4. Rejouer les mêmes catalogues pour comparer `G^2` et postings, avec
   transcript Gamma et digest canonique.
5. Implémenter le préflight puis les runs chunkés; mesurer CPU avec high-water.
6. N'envisager une G4 SPOT gardée qu'après existence d'un kernel réel, manifeste
   mémoire admissible et différentiel CPU positif.

Cette route conserve le résultat déjà acquis, résout le verrou mathématique
avant l'optimisation et reste fidèle à l'architecture légère : aucune mosaïque
globale n'est construite.

GCP non utilisé pour cet audit.
