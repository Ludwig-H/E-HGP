# Critique des trois verdicts et plan révisé : sortie paramétrée de `mhgp11`, hiérarchie des supports

4 octobre 2026, 19 h 46 UTC (heure lue par `date -u`). Rôle : synthèse critique du workflow. Lecture seule : rien
n'a été construit, modifié ni commité ; ce fichier est la seule écriture. **GCP non utilisé.**

```text
phase=exploration_v11_hors_registre
backend=cpu_reference
profile=quantized_u21_input_only   (défaut de compilation ; u18 et u24 qualifiés à part)
public_status=not_claimed
```

**Sources.**
- Code : worktree `build/v11-claude-20261003/morsehgp3D_v11`, à `57dd21be1`.
- Reçus et audits : `main`, à `1bf4be68f`.
- Dossier, lu en entier : cinq lectures, deux conceptions, `SPECIFICATION_FINALE.md` (dite « la spec »), trois
  critiques, `decisions_finales.json` et `slices_finales.json`.
- Thèse : texte de la Déf. 21–22 extrait du PDF.

Sauf mention, les chemins sont relatifs à `morsehgp3D_v11/`.

---

## 0. Synthèse critique

**Architecture (« tout natif ») : d'accord.**
- La spec livre déjà le natif par étapes, et l'état final reste tout natif. Rien ne va contre l'utilisateur.
- Je reprends ses cinq amendements :
  - `plat` passe en dernier et peut être différé, sur décision écrite ;
  - $m(1)$ et $\kappa$ sont figés dès le contrat ;
  - `Big` travaille à longueur utile et se juge contre l'`int` de Python ;
  - la table des racines est mesurée ;
  - une sortie non encore livrée est refusée par `parameter_out_of_range`.
- Un erratum mineur. Pour chaque rang, la racine porte sur un nombre d'environ 172 bits en u21, et non 310. Ce sont
  le dividende $N\cdot 2^{128}$ et la division qui atteignent environ 308 bits.

**Géométrie (décisions 2 et 3) : d'accord, et j'adopte son apport principal.**
- Le lemme H n'est pas un résultat nouveau. C'est la fin de la preuve de P3, rendue datée et rattachée : « la
  couverture complète est l'union des populations des cellules fortes qu'elle contient »
  (`docs/MATHEMATIQUES.md:273-275`). J'en ai revérifié chaque pas.
- Publier $P_b=I_b\cup U_b$ rend donc exactement les $K$-polyèdres de la thèse, recouvrements compris. La Déf. 21 les
  définit comme « l'ensemble des points de $X$ apparaissant dans une composante connexe de $\Gamma_K$ ».
- C'est aussi le canal « retours observés » de `Zoltan/FoundationModel/JETON.md:57`.
- Le format sobre contient davantage et pèse deux fois moins : environ 39 Mo par trame à K5, contre environ 80 Mo
  pour la v1 de la spec.
- Erratum : l'affirmation « 21 boules par trame ont plus d'un support » est fausse. Il y en a au plus 6, 2 et 13 sur
  ng00, ng01 et ng02, soit 21 en tout. C'est une mesure locale sur $\mathrm{Cat}_7$, non reçue.

**Périmètre (« l'arbre d'ordre K ») : d'accord sur presque tout, sauf sur l'argument central.**
- J'approuve :
  - l'objet : $T_K$ seul pour `supports`, `points` et `plat`, FULL inchangé ;
  - les sondes de banc figées ;
  - quatre livraisons G4 au lieu d'environ neuf ;
  - la liste des qualifications.
- Je **conteste** son argument central, le seul point marqué « contre l'utilisateur ».
  - Le rapport de 2,4 à 3,3 compare la voie par lots **pour les cinq ordres** au pipeline **pour les cinq ordres**.
  - Le même reçu apparié publie les temps **ordre par ordre** (§ 1). À W48, l'ordre 5 seul sur la voie par lots coûte
    environ 193, 147 et 195 ms. La forêt FULL entière en coûte 171, 133 et 158 après la tranche 3, et 241, 164 et 197
    au commit c40.
  - À W8 et W1, où W1 est le défaut du CLI, l'ordre 5 seul est 27 à 45 % plus rapide.
  - À W48, la part de l'ordre 5 dans l'étage forêt de la voie par lots est de 30 à 34 %. Les « 60 à 80 % » de la
    projection ne tiennent donc pas.
- **Les faits ne vont donc pas contre l'utilisateur.** Je garde le calcul de l'ordre K seul, comme la spec, avec la
  porte d'identité I10 et une mesure appariée dès L2, sous une règle écrite à l'avance.

**Ce qui revient vraiment à l'utilisateur** (§ 4) :
- publier ou non $P_b$ ;
- le sens de « nombreuses liaisons » : à K fixé, un support porte 0 ou 1 liaison, sauf sur les coquilles
  cosphériques ;
- l'ordre de livraison entre `supports` et `points` natif.

---

## 1. Le fait qui renverse le verdict « périmètre » : les temps par ordre

Les données viennent du reçu `receipts/audit_deep_20261004/performance/selected/c40_paired/results/cmd/002_paired_full/files/full_paired.json`,
champ `order_stage_ms`, en médianes de trois prises, avec K5, u21 et des trames entières sans sol.
- La colonne « tranche 3 » vient de `receipts/developpement_20261003/pipeline_g4/README.md:42-44`.
- Pour la voie par lots, l'ordre 5 seul vaut classification, plus naissances, plus plateaux. Les plateaux comprennent
  `prepare_states` et `finish` (`src/tower/forest_build.cpp:251-265`).

Tous les temps sont en ms.

| Trame | Lots 2047, ordre 5 seul, W48 | FULL 16379, toute la forêt, W48 (c40) | FULL après tranche 3, W48 | Lots, ordre 5 seul, W8 | FULL, W8 | Lots, ordre 5 seul, W1 | FULL, W1 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| ng00 | 193 | 241 | 171 | 500 | 685 | 2 874 | 4 882 |
| ng01 | 147 | 164 | 133 | 372 | 519 | 2 115 | 3 702 |
| ng02 | 195 | 197 | 158 | 457 | 629 | 2 464 | 4 467 |

La part de l'ordre 5 dans l'étage forêt de la voie par lots vaut 34, 32 et 30 % à W48, puis 43, 41 et 39 % à W8, et
51, 49 et 47 % à W1.

Les MEB, elles, se concentrent à 70 % sur l'ordre 5, mais leur part ne prédit pas le temps mural. Chaque ordre paie
ses vidanges : environ 291 pour l'ordre 5 de ng00, soit 184 lots et 107 cellules étendues.
- Mécanique : `src/tower/forest_parallel.cpp:252-285` et `:380-402`.
- Comptes : `parallel.regular_batches` et `extended_cells`, dans le même reçu.

**Réserves.**
- Ce sont des sommes de médianes de phases indépendantes. Le reçu le signale lui-même (`timing_scope.phase_medians`).
- La configuration mesurée est 2047 (avec mémo, sans table de populations), et non 8187, celle que prendrait
  `build_order` (16379 sans le bit 8192, `bench/full_probe.cpp:364-388`). Aucun reçu de performance n'existe pour 8187 :
  ses occurrences dans `receipts/` ne sont que des fragments de sha256.
- Côté correction, la combinaison « table de populations sans ordres concurrents » est déjà jugée :
  `tests/tower/population_concurrent_test.cpp:100-125` compare les forêts à W1, W4 et W48.

Ces chiffres ne prouvent donc pas une vitesse. Ils montrent seulement que le facteur 2 avancé ne résiste pas à
l'examen.

**Le vrai coût de l'ordre K seul n'est pas le temps, c'est la maintenance.**
- La façade aurait deux configurations du moteur : `full` en pipeline, et les trois autres sorties sur la voie par
  lots.
- Les progrès du chantier 100 ms sur le pipeline ne profiteraient pas d'eux-mêmes à `build_order`.

Je l'accepte pour trois raisons :
- la porte I10 garde l'identité de la forêt ;
- la mesure de L2 tranche par une règle écrite à l'avance ;
- `build_order` ne touche ni `build_full`, ni `build_concurrent`, ni le pipeline, c'est-à-dire le code chaud du
  chantier 100 ms. C'est un découplage utile.

L'audit va dans le même sens : « L'arbre K seul est une base cohérente »
(`audits/AUDIT_REPONSES_AUX_VERROUS_MOTEUR_20261002.md:13`).

---

## 2. Plan révisé

Le plan regroupe les tranches de la spec en cinq livraisons.
- **L0** ne demande pas de session.
- **L1 à L4** prennent chacune une session G4 gardée.
- **L2b et L5** sont conditionnelles.

Règles communes, inchangées :
- commits sur `main`, sans branche ;
- vérifier que `git diff --cached` est vide avant tout `git add` ;
- un reçu immuable et une note aux deux auditeurs par livraison ;
- aucun natif dans le Codespace (`README.md:52`) ;
- portes Python en bibliothèque standard, vérifiées sous `python3 -S` ;
- sessions de 4 200 s au plus. Si la matrice dépasse ce temps, on scinde la livraison par unité, sans changer
  l'ordre.

### L0 : contrat et vérité bornée (S0 et S1 ; aucune session G4)

**Contenu de S0 (spec), avec ces ajouts.**
- **`docs/MATHEMATIQUES.md` § 10.**
  - Le lemme H, comme corollaire daté et rattaché de P3. Pour $K\geq 2$, $\mathrm{pts}(C)=\bigcup\lbrace P_b : b\in W_K,\ \mathrm{att}(b)\preceq v,\ \lambda_b\leq a\rbrace$, que l'on peut restreindre aux boules fortes. À $K=1$, ce sont les feuilles du sous-arbre.
  - La phrase de la spec, l. 217, devient : « une boule hors de $W_K$ ne change ni les composantes de $\Gamma_K$ ni
    les ensembles de points des $K$-polyèdres ». En effet, elle ajoute des sommets et des liaisons non-Gabriel.
  - Une **liaison** est une $(K+1)$-partie. À K fixé, un support porte 0 ou 1 liaison, sauf sur les coquilles
    cosphériques.
  - Les boules de $p+q\in\lbrace K+2,K+3\rbrace$ (J3, `:350-358`) sont décrites comme une extension non livrée.
- **`docs/SORTIES.md`.**
  - `MHGP11SP` v1 au format sobre (§ 2.1 ci-dessous).
  - Les comptes portent les noms de l'audit `1bf4be68f`. Ce sont des fonctions de l'API C++ et du lecteur ; ils ne
    sont pas stockés.
  - Le manifeste publie des agrégats.
  - Une sortie non livrée est refusée par `parameter_out_of_range`.
  - § 2.8 de la spec : l'identité à l'octet vaut sous permutation et sous réétiquetage. Sous une translation entière,
    seuls les ensembles sont invariants, car l'ordre de Morton des lignes change (`src/cloud/morton.hpp:1`).
- **`docs/HIERARCHIE_POINTS.md`.** $m(1)=1$ et $\kappa=1$ sont figés. L'alignement de la chaîne plate Python est
  prévu en L3.
- **Règle de décision de L2**, écrite à l'avance.
- **Clause de report de `plat`**, dans `README.md` et dans la réponse aux auditeurs.

**Contenu de S1 (spec), avec ces ajouts.**
- L'oracle calcule $P_b$ et $\mathrm{pts}(C)$ par la Déf. 21, puis contrôle le lemme H sur chaque nœud et à chaque
  niveau d'événement.
- Un nouveau mutant d'oracle, `populations_naissances_seules`, doit être tué par `growth_ABCZ`. P3 dit en effet que
  « l'union des seules populations de naissance ne suffit pas ».

**Portes et critères.**
- Celles de S0 et S1 : `check_style` (`[table]`, `[module]`), `check_docs`, `mhgp11_reference_supports` et sa jumelle
  `-O`, avec les planchers de la spec.
- En plus, le lemme H, sans aucun écart, sur tous les nœuds des 150 nuages au moins.
- Les mutants d'oracle doivent sortir avec le code 4.
- Exécution locale sous `python3 -S -B` puis sous `-O`.

**Sortie.** La réponse écrite des deux auditeurs, avant toute ligne native.

**Ce qui change, et pourquoi.**
- La spec faisait de S0 et S1 deux tranches ; elles forment ici une seule livraison, vérifiable sans session.
- Le contrat du format doit être fixé avant le premier octet natif.

### L1 : arbre K, rattachement et supports (S2, S3 et S6 ; session G4 n° 1)

**Contenu.**
- S2 : en-tête parapluie `tower.hpp` et `meb.hpp`, sans changer un octet.
- S3, tel que la spec le décrit :
  - `build_order` sur la voie par lots existante ;
  - journal des graines gardé dans `cell` et `regular_cell` (`src/tower/forest_plateau.cpp:39-67,100-120`) ;
  - `WindowAttachment` ;
  - E2 comme juge de test.
- S6, qui produit les sections `POP` et les masques de $\mathcal{Q}_b$. Les comptes sont des fonctions.
- Les sondes JSON de test.

**Portes.**
- Celles de S2, S3 et S6 dans la spec.
- I10 : la forêt de `build_order` est identique à l'ordre K de `build_full`.
- E1 = E2.
- L'export validé : les incidences fortes doivent être identiques à l'octet au bloc `MHGP11PH`. C'est aussi le lemme H
  à l'échelle.
- Le juge d'échantillon.
- Les mutants `tower` et `supports`.

**Qualification G4, indispensable.**
- GCC u21 et la suite `fast` complète.
- Rejeu du manifeste des mutants `tower`, puis les nouveaux mutants.
- ASan et UBSan sur `supports`.
- TSan ciblé si l'énumération est parallèle.
- `scale8000` avec E1 = E2 sur toutes les boules, et 32 000 points sur 2 000 boules tirées.
- ng00 à ng02 à K5 : I1 à I6, I10 et I11.
- W1, W4 et W48, plus une permutation de l'entrée.
- Une construction u24 avec sa suite `fast`.
- Aucune mesure de temps.

**Ce qui change.** Trois tranches tiennent dans une seule session, et S6 publie aussi les populations.

### L2 : io, api et CLI, avec `full` et `supports` ensemble (S4, S5 et S7 ; session G4 n° 2)

**Contenu.**
- S4, S5 et S7 de la spec.
- `--sortie=points` et `--sortie=plat` sont refusées par `parameter_out_of_range`.
- L'écrivain `MHGP11SP` au format sobre.
- Un lecteur en bibliothèque standard : $P_v$ et ses instantanés en tranches contiguës, lemme H, comptes dérivés,
  niveau exact retrouvé par le témoin $S^*$.

**Mesure appariée, préenregistrée en L0.**
- On compare l'étage `tree` de `--sortie=supports` (ordre K seul) à celui de `--sortie=full`.
- Trames ng00 à ng02 à K5, à W1 et W48, trois prises chacune. K10 une fois, sur ng00.
- **Règle.** `build_order` reste le défaut si, à W48, son étage `tree` ne dépasse pas de plus de 10 % celui de `full`
  sur au moins deux trames. Sinon, on passe par L2b.

**Portes.**
- Celles de S4, S5 et S7 :
  - identité de `full` avec le banc ;
  - contrat du CLI, avec au moins 20 refus et sans dossier publié ;
  - transaction `RENAME_NOREPLACE` ;
  - oracle via le CLI ;
  - déterminisme et réétiquetage.
- Qualifications : celles de L1, plus K10 sur LiDAR une fois et l'échelle 16 000.

**Ce qui change, et pourquoi.**
- `full` ne prend plus de session à elle seule : elle n'apportait rien de neuf, puisque le banc produit déjà ce
  fichier.
- La mesure prévue en S7 devient une décision réglée à l'avance.
- La tranche S11 n'est plus la seule issue.

### L2b : journal dans `build_full` (conditionnelle, petite)

- **Déclenchement** : seulement si la règle de L2 le demande.
- **Contenu** : un pointeur facultatif vers le journal, posé sur le constructeur d'ordre K de `build_full`
  (`src/tower/forest_vertical.cpp:336`) et de `build_concurrent` (`src/tower/forest_concurrent.cpp:243`). Les
  publieurs sont uniques par ordre (`:27-56`).
- **Porte** : `MHGP11SP` identique à l'octet par les deux voies.
- **Qualification** : TSan sur `mhgp11_tower_pipeline` et rejeu du manifeste des mutants.
- **Garantie** : l'objet et les octets de sortie ne changent pas.

### L3 : `num` et `points` natif (S8 et S9 ; session G4 n° 3, paquets `pinned` pour les différentiels `long`)

**Amendements (critique d'architecture).**
- `Big` travaille à longueur utile. Sa capacité de 16 384 + 1 024 bits n'est qu'un plafond de refus.
- Une porte `fast` le compare à l'`int` de Python, sous `python3 -S`.
- La table des racines est mesurée à 8 000, 16 000 et 32 000 points, puis sur les trames.
- En repli : une racine proposée en binary64, puis certifiée en entier par $s^2\leq X<(s+1)^2$. Le flottant propose,
  l'entier décide : la règle reste conforme à F1.
- Avant tout différentiel natif, $m(1)=1$ est aligné dans `bench/points_flat_gate.py:117` et F13 est rejouée. Le bras A
  est `HDBSCAN(min_samples=k)` (`:84-88`).

**Portes.** Celles de S8 et S9 dans la spec.

**Ce qui change.** Ces quatre amendements. `num` et `points` partagent une session, car rien en amont ne dépend de
`num`.

### L4 : `plat` natif (S10 ; session G4 n° 4)

**Report possible.** Cette livraison se reporte par décision écrite tant que la règle E1-bis reste ouverte
(`docs/SORTIE_PLATE.md:51-52`, « Ouvert » `:155`). Le tokenizer n'en a pas l'usage : sa condensation est à seuil
relatif (`Zoltan/FoundationModel/SPECIFICATION.md:64-80`).

**Remplacement d'ici là.** Le banc Python exact sert aux comparaisons avec HDBSCAN.

**Portes.** Celles de S10 dans la spec.

### L5 : pipeline à un ordre (S11 ; facultative, chantier 100 ms)

Elle n'est engagée que si L2, ou le cas K10, montre qu'elle rapporte. Le défaut ne change que sur reçu.

### Ce qui ne bouge pas

- Les sondes `mhgp11_full_bench` et `mhgp11_points_export`, avec leurs argv et leurs portes à ligne exacte. Le
  premier reste l'instrument A/B du chantier 100 ms ; le second, le témoin des incidences fortes.
- Les formats `MHGP11FUL1` et `MHGP11PH`.
- `build_full`, sauf en L2b.
- Le catalogue.
- Le registre `docs/implementation_status.toml`.

### 2.1 `MHGP11SP` v1 au format sobre (normatif en S0)

| Section | Contenu |
| --- | --- |
| `SITES` | `x`, `y`, `z`, `point_id` en `u32`, dans l'ordre des `SiteIdx` (Morton) |
| `NODES` (numérotation canonique) | `parent u32`, `rank u32`, `kind u8`, `ball_off u64[N+1]` indexé par rang de postordre (enfants visités par `NodeIdx` croissant) ; ligne de site des feuilles à K = 1 |
| `BALLS` | rangées par (postordre du nœud, rang, `BallIdx`) : `rank u32`, `role u8`, `p u8`, `m u8`, `qmin u8`, `prior_count u32` |
| `POP` | $\sum(p+m)$ lignes de `SITES` : $I_b$ croissant puis $U_b$ croissante, recopiées de `src/catalogue/catalogue.hpp:160-166` |
| `SUPPORTS` | coquilles étendues seulement : (ordinal de boule, nombre, masques `u32` sur les positions de $U_b$), comme `bench/catalogue_euler.hpp:137-165` ; pour une coquille régulière, $\mathcal{Q}_b=\lbrace U_b\rbrace$ implicitement |
| `PRIOR` | nœuds de $\mathrm{ant}(b)$, pour le rôle fusion seulement |

**Champs non stockés.** L'API C++ et le lecteur les calculent :
- enfants, `post`, `size` ;
- `k_parts`, `compressed_parts`, `strict_traces`, `components` ;
- `cofaces` par boule et par support, et les comptes de Gabriel.

**Manifeste.** Il porte des agrégats :
- le nombre de supports et leurs arités ;
- les coquilles étendues ;
- la somme et le maximum des cofaces par support ;
- le nombre de supports qui ont plus d'une coface.

**Taille estimée (ng00).**
- Environ 39 Mo à K5, contre 80 à 84 Mo pour la v1 de la spec.
- Environ 145 Mo à K10 (extrapolé), contre environ 220 Mo.

---

## 3. Ce qui change par rapport à `SPECIFICATION_FINALE.md`

| Point | Spec | Révisé | Pourquoi |
| --- | --- | --- | --- |
| Calcul de $T_K$ | `build_order` sur la voie par lots ; mesure en S7 ; S11 | inchangé ; mesure en L2 sous règle écrite ; repli L2b | temps par ordre (§ 1) : quasi-égalité à W48, gain à W8 et W1 ; la critique de périmètre comparait des totaux sur cinq ordres |
| Contenu de `supports` | $\mathcal{Q}_b$ et comptes stockés, sans populations | $P_b$ (`POP`) et $\mathcal{Q}_b$ en masques ; comptes calculés | lemme H (P3) : c'est l'objet de la thèse ; options 1 et 2 de JETON ; $\mathrm{conv}(U_b)$ reste calculable ; deux fois moins d'octets |
| Format | 41 o par nœud, 48 par boule, 21 par support | sobre (§ 2.1) | champs dérivables retirés |
| Périmètre $W_K$ (l. 217) | « ne change pas $\Gamma_K$ » | « ne change ni les composantes ni les ensembles de points » | une boule hors fenêtre ajoute des sommets et des liaisons non-Gabriel |
| Invariance (§ 2.8) | octets identiques sous les symétries de la grille | octets identiques sous permutation et réétiquetage ; sous translation, ensembles invariants seulement | l'ordre de Morton n'est pas invariant par translation |
| Liaisons | question 1 | définition et constat écrits ; question réduite au sens voulu | le fait est vérifié ; le sens appartient à l'utilisateur |
| Livraisons | S0 à S11, environ neuf sessions | L0 sans G4, L1 à L4, L2b et L5 conditionnelles | le vrai coût est le nombre de sessions ; `full` seule n'apporte rien de neuf |
| `plat` | S10 obligatoire | en dernier, reportable par décision écrite | règle E1-bis ouverte ; aucun usage pour le tokenizer |
| $m(1)$ et $\kappa$ | alignés en S9 | figés en S0, alignés avant le différentiel natif | sinon le différentiel compare deux conventions |
| `Big` | capacité fixe de 272 mots | longueur utile ; la capacité sert de plafond ; porte `fast` contre `int` | évite un facteur d'environ 7 sur le chemin courant |
| Table des racines | supposée bon marché | mesurée en L3 ; repli binary64 certifié en entier | coût non mesuré : par rang, une division d'environ 308 par 134 bits et une racine d'environ 172 bits (u21) |
| Sorties non livrées | non précisé | refus `parameter_out_of_range` | pas de raison morte après L4 |
| Oracle S1 | sans lemme H | lemme H et mutant `populations_naissances_seules` | il contrôle l'objet publié |

---

## 4. Questions pour l'utilisateur

1. **Populations dans la sortie `supports`.** Faut-il publier aussi, pour chaque boule, $P_b=I_b\cup U_b$, avec
   $\mathcal{Q}_b$ codé en masques sur $U_b$ ?
   - **A (recommandé).** Oui. La sortie rend alors exactement les $K$-polyèdres datés de la thèse (Déf. 21 et 22 ;
     c'est P3) et le canal « retours observés » de JETON. Elle pèse environ 39 Mo par trame à K5.
   - **B.** Non : on garde le squelette $\mathcal{Q}_b$ seul, environ 31 Mo.
2. **« Un même support peut correspondre à de nombreuses liaisons » : quel sens visiez-vous ?** Tous sont calculés par
   l'API. Votre choix fixe seulement le nom et le poids $a_Q$ par défaut. La question n'est pas bloquante ; le défaut
   est A.
   - **A.** Les liaisons de la thèse à K fixé, c'est-à-dire les $(K+1)$-parties qui contiennent $Q$. Elles valent 1 à
     une jonction et 0 à une naissance. Elles ne dépassent 1 que sur les coquilles cosphériques : 107, 49 et 239 boules
     à K5 sur ng00, ng01 et ng02.
   - **B.** Les simplexes de même boule minimale à travers les ordres : l'intervalle $[Q,P_b]$, de taille
     $2^{p+m-\lvert Q\rvert}$.
   - **C.** Les $K$-parties reliées : $K+1$ à chaque jonction.
3. **Ordre de livraison après L1.**
   - **A (recommandé).** `supports` en sortie (L2), puis `points` natif (L3), puis `plat` (L4), reportable.
   - **B.** `points` natif juste après L1, pour accélérer les démos face à HDBSCAN ; `supports` en sortie ensuite.

---

## 5. Annexe : vérifications décisives

| Affirmation | Où | Verdict |
| --- | --- | --- |
| La voie par lots vide avant chaque cellule étendue et tous les 4 096 travaux | `src/tower/forest_parallel.cpp:252-285,380-402` ; cellules en `:275`, `:301` | confirmé |
| Le pipeline publie une tâche par ordre, en ordre `BallIdx` ; il est exclu pour un seul ordre | `src/tower/forest_concurrent.cpp:27-56` ; `src/tower/forest_pipeline.cpp:169,178,235` | confirmé |
| `build_full` : boucle non concurrente, un `ForestBuilder` par ordre | `src/tower/forest_vertical.cpp:281-356` (boucle `:334-352`) | confirmé |
| Masques 2047, 16379 et 8187 | `bench/full_probe.cpp:364-388` ; `bench/points_export.cpp:376-383` | confirmé ; 8187 jamais mesuré |
| Étage forêt à W48 : 571, 460 et 645 ms (2047), contre 240,8, 163,6 et 197,2 ms (16379) | `receipts/qualification_performance_20261003/review/metrics.optimized.json` | confirmé, mais le reçu précise « no isolated attribution » |
| $\lvert W_5\rvert$ = 789 886, 652 958 et 832 386 ; continuations 212 858 et plus (27 %) ; coquilles étendues 107, 49 et 239 | `full_paired.json` (`work.cells`, `continuations`, `parallel.extended_cells`) | confirmé |
| Le lemme H est déjà P3 | `docs/MATHEMATIQUES.md:263-278` | confirmé |
| Un $K$-polyèdre est un ensemble de points | thèse, Déf. 21 (p. 58) | confirmé |
| JETON : socle, $\mathcal{Q}_b$, $\mathrm{conv}(U_b)$ ; populations en CSR ; ordres $\lbrace 1,2,3,5\rbrace$ avec cartes verticales | `Zoltan/FoundationModel/JETON.md:57-59,67-71,78-81` ; `SPECIFICATION.md:300-302,321-322` | confirmé |
| Plafonds de `num` et absence de racine et de PGCD ; aucune dépendance externe | `src/num/wide.hpp:16` ; `src/num/integer.hpp:14` ; `CMakeLists.txt:53` | confirmé |
| Le Python suppose l'arrondi au plus proche et des `PointId` denses | `bench/points_radius.py:136-146` ; `bench/points_flat.py:42,882` | confirmé |
| Divergence de $m(1)$ | `docs/HIERARCHIE_POINTS.md:95-96` contre `bench/points_flat_gate.py:117` | confirmé |
| G4 tourne en Python nu ; `pinned` en option | `tools/g4_matrix.py:2` ; `gcp-migration/v11_session.py:100,726-728` | confirmé |
| « 21 boules par trame à plusieurs supports » | `build/v11-local/euler_tmp/lidar_ng00.k5.json` : 326 supports pour 320 coquilles | faux : 6, 2 et 13 au plus |

## 6. Ce que je n'affirme pas

- Je n'ai mesuré aucun temps natif nouveau. Les temps par ordre sont des sommes de médianes indépendantes, prises
  dans la configuration 2047.
- Les tailles de fichier sont des estimations : à K5, elles reposent sur des comptes reçus ; à K10, elles sont
  extrapolées.
- La répartition par $m$ et le nombre de supports multiples viennent de sorties locales non reçues.
