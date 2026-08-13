# MorseHGP3D v3

MorseHGP3D v3 explore une construction exacte et industrielle de la hiérarchie
Morse/HGP en dimension trois, sans matérialiser la mosaïque de Delaunay d'ordre
supérieur.

Cadre courant :

```text
phase=exploration_v3_hors_registre
backend=cpu_reference_bounded_oracles_and_g4_diagnostic
profile=quantized_u16_input_only
mode=audit_independant_math_and_architecture
public_status=not_claimed
```

La v3 n'est ni promue dans le registre officiel, ni qualifiée GPU, ni déclarée
exacte sur son domaine public. GCP n'est pas impliqué dans l'état décrit ici.

## Verdict actuel

Le contrat n'est pas rempli. Pour `n=50000` et `K_max=10`, la cible principale
est `p95 warm_e2e<100 ms` et la cible secondaire `p95 warm_e2e<1 s` sur un G4,
sortie complète et synchronisation comprises. Aucun échantillon qualifiable ne
reçoit l'une ou l'autre.

Le pin `2b89ea1` introduit enfin une première tranche
`BallForm -> PrimitiveSphereKey -> census I_B/U_B -> SphereRun`. C'est le bon
ordre architectural, mais **l'étape 0A n'est pas reçue pour u16** : les
constructeurs q3/q4 rabattent des numérateurs de 67 à 81 bits vers `int64`, puis
créent des carrés jusqu'à environ 162 bits dans `i128` avant réduction. Les
huit CTests verts ne couvrent que `coord<=64`; leur juge ne recertifie ni la
positivité ni la clé primitive, et le mutant de clé s'auto-déclare tué.

Le verdict live, y compris le worktree concurrent, est dans
[`audits/AUDIT_ETAT_COURANT.md`](audits/AUDIT_ETAT_COURANT.md). L'audit ciblé de
la tranche est
[`audits/AUDIT_BALL_EVENT_V0_2B89EA1_20260813.md`](audits/AUDIT_BALL_EVENT_V0_2B89EA1_20260813.md).

## Route active

La prochaine chaîne à fermer est :

```text
0A  BallForm -> BallKey -> census -> BallEvent exact
0B  oracle exhaustif borné -> lots -> dix forêts -> verticales -> payload
1   substituer seulement la source sparse et mesurer E3/E4, M3/M4 et H
2   intégrer les moteurs q3/q4 locaux si la porte de coût composée est verte
3   porter la même tranche sur device, puis mesurer warm_e2e sur G4
4   ouvrir séparément tout nouveau profil numérique
```

Une source incomplète peut être comparée dans le sink de référence, mais ne
publie jamais un succès. Il n'existe pas de watermark monotone par ancre : les
runs sont scellés, triés et mergés par niveau exact avant le premier commit
d'un lot. « Streamé » signifie mémoire résidente bornée, jamais fold en ligne
sur une source non scellée.

## Contrat d'identité

Les couches restent distinctes :

- `PrimitiveSphereKey` : cinq coefficients primitifs de
  `A||z||^2+B dot z+C`, avec `A>0`, avant census ;
- `BallKey` : identité de nuage, profil et schéma exact ajoutés à la clé
  primitive ;
- `SupportKey` : vrais `PointId` triés, jamais positions Morton ou indices
  d'un buffer ;
- `BallEvent` : `BallKey`, supports, owners, niveau exact, `I_B/U_B`, lanes,
  provenance, complétude du census et disposition transactionnelle.

Le fold ne dépend ni de `__int128` natif, ni du nombre de limbs du profil. Un
futur profil binary64 certifié peut donc changer `ExactKernel` et le codec sans
réécrire le fold. La cardinalité seule ne motive pas binary64 : la grille u16
3D contient $2^{48}$ sites distincts ; l'index dense et le `PointId` sont des
codecs séparés.

## Prochaines réparations P0

Avant d'appeler `0A` fermé :

1. construire directement les polynômes q3/q4 sans centre rabattu en `int64`
   et prouver les largeurs sur tout `[0,65535]^3` ;
2. juger indépendamment dépendance affine, positivité, clé primitive, niveau,
   census et owner sur des `PointId` non denses ;
3. ajouter epoch/profile/schema, statuts typés, marqueurs de complétude et
   `SupportRecord` atomique ;
4. appliquer `count -> preflight -> fill -> validate -> publish`, avec zéro
   payload sur cap moins un, erreur numérique ou dégénérescence non admise ;
5. différencier toute la sortie de `0A`, puis fermer `0B` jusqu'au
   `BenchmarkOutputContract-v1`.

## Déblocages mathématiques prêts après `0A`

Trois pistes sont assez précises pour être implémentées dans les composants
existants, avec échec fail-open.

### `SOC64` et `CORNER512`

Pour `e=z-a`, `t=b-z`, `H=e dot t`, `E=||e||^2`, `X=||t||^2`, q3 exige
`H>0 && 4H^2>EX` et q4 `H>0 && 3H^2>EX`. Le domaine est séparément convexe en
`e,t` et en `a,b,z`.

- Si les 64 couples de coins de `(C-A)×(B-C)` passent, tout le rectangle est
  `ALL`. Un échec est `UNKNOWN`.
- Les 512 triples de coins de `A×B×C` caractérisent exactement `ALL` pour
  l'enveloppe AABB continue. Un coin fictif échouant ne vaut pas `NONE` pour les
  seuls `PointId` stockés.

### LP projectif

Pour `s_i=z_i-a`, `d=b-a`, `D=||d||^2`, `q_i=||s_i||^2`, poser :

$$\kappa_G(d)=\min\left\lbrace \sum_i\alpha_iq_i:\sum_i\alpha_is_i=d,\ \alpha_i\geq0\right\rbrace.$$

`G` crédite un intérieur sur toute sphère par `a,b` si et seulement si
`d` appartient au cône positif de `G` et `kappa_G(d)<D`. Un optimum basique
emploie au plus trois IDs. Huit extractions disjointes donnent un fast path q4;
un arbre de suppressions fournit un oracle pairwise complet relativement au
pool, jusqu'à 3280 LP pour q4. Ce dernier n'est pas un hot path.

### Cages de quatre à six sites

Une base positive minimale 3D peut avoir quatre, cinq ou six sites. Les cages
tétra-only sont donc incomplètes. Une cage de six facettes possède au plus huit
sommets de fleur. `SixRoleCageProposer` reste une ablation counter-only ; chaque
groupe doit être validé exactement, et réduire une cage impose de recalculer sa
fleur.

Les preuves, limites de largeur et contre-fixtures sont consolidées dans
[`audits/AUDIT_REPONSE_PLAN_VERTICAL_SOC64_LP_1AA487D_20260813.md`](audits/AUDIT_REPONSE_PLAN_VERTICAL_SOC64_LP_1AA487D_20260813.md).

## Dégénérescences et sortie lourde

Le profil de coordonnées u16 n'exclut pas les cosphères. Le domaine candidat
utilise une politique `RelevantGP` fail-closed : un extra-shell pertinent rend
`unsupported_degeneracy` tant qu'aucun quotient complet n'est reçu. Cette
fermeture du domaine reste elle-même à recevoir.

Un `SphereRun` interne conserve l'identité et le census pour garder la décision
réversible. Il n'autorise pas un plateau public. Un quotient saturé ne devient
valide qu'après reconstruction des lots, dix forêts, coverage et verticales.
Si le contrat exige chaque `SupportKey`, une cosphère lourde est une borne de
sortie ; ni RLE ni streaming ne suppriment ce travail.

## Porte de coût

Une pente `sum_E4` ne qualifie rien seule. Chaque campagne publie au minimum :

- masses exclusives `CLOSED/OPEN/PENDING`, avec `pending=0` pour une fenêtre
  finale ;
- `E3/E4`, maximum par ancre, `M3/M4=sum m_ab`, tâches, splits et visites ;
- `BallKey` brutes/uniques, supports, census et tailles de shell ;
- sorties `H`, octets, HWM, opérations larges et temps par phase ;
- commandes, seeds, commit, diff, binaire et codes de sortie.

Les diagnostics CPU existants ne sont pas des modèles G4. Aucun cutoff kNN
n'est exact : des supports positifs gardent un partenaire arbitrairement loin
en rang. Aucun arrangement global, aucune mosaïque Delaunay d'ordre supérieur
et aucun catalogue exhaustif ne deviennent le chemin produit.

## Construire et tester

Depuis la racine du dépôt :

```bash
cmake -S morsehgp3D_v3 -B build/v3 -DCMAKE_BUILD_TYPE=Release
cmake --build build/v3 --parallel
ctest --test-dir build/v3 --output-on-failure
python3 tools/check_docs.py
```

CUDA reste opt-in avec `-DMHGP3V_ENABLE_CUDA=ON`. Une session GCP éventuelle
doit suivre exclusivement les scripts gardés et les coupe-circuits décrits par
`AGENTS.md`.

## Arborescence documentaire

- [`PROPOSITION.md`](PROPOSITION.md) : proposition technique et mathématique
  consolidée ;
- [`audits/AUDIT_ETAT_COURANT.md`](audits/AUDIT_ETAT_COURANT.md) : unique
  verdict mutable ;
- [`audits/README.md`](audits/README.md) : index court des audits actifs et des
  dépendances historiques encore citées par le logiciel ;
- `oracle/` : juges bornés indépendants ;
- `prototype/` : candidats et probes, sans autorité produit implicite ;
- `receipts/` : diagnostics et reçus, dont le statut est fixé par l'audit.
