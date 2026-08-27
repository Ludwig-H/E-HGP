# État courant — audit de MorseHGP3D v5

- **Date :** 27 août 2026
- **Auditeur :** Codex, avec relecture critique du brouillon de l'autre auditeur
- **Pin fonctionnel exécuté :** `10c46c87bbda13a3fda697c9dedb94fead273faa` ; les sources produit sont identiques dans `b79c001b` et `a0d13420`
- **Tip documentaire et reçu audité :** `a0d134205b5b4364ada1e6c12995f979f59698b4`
- **Reçu G4 le plus récent :** [`campagne_g4_v5_20260827_adaptatif`](../receipts/campagne_g4_v5_20260827_adaptatif/RECU.txt), source `8f95df2effd07ffa7a8aa7cf7fe79be1be9c7b2c`, publié par `a0d134205b5b4364ada1e6c12995f979f59698b4`
- **Worktree observé hors verdict :** le probe racine `.codex_fold_contract_probe.cpp` appartient à un autre auditeur
- **Cadre :** `phase=exploration_v5_hors_registre`, `backend=cpu_reference`, `profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`, `public_status=not_claimed`
- **GCP :** non utilisé par l'auditeur ; le nouveau reçu affirme l'arrêt ciblé, mais ne conserve pas la sortie de certification correspondante

## Verdict

La v5 reste **orange et avance dans la bonne direction**. Le nouveau reçu brut
ferme empiriquement les deux anciens OOM à 50 000 points au pin `8f95df2e` :
les quatre exécutions `--gpu` terminent avec le code 0 et leurs
`digest_balls` **et** `digest_all` sont identiques aux sorties CPU appariées.
La porte lane brute présente quatre cas Q3 et quatre cas Q4 non vides sans
désaccord, et le mutant Q3 device est tué avec le code 4.

La formulation correcte est toutefois **égalité bornée observée au pin
`8f95df2e`**, pas « lane exacte » en général. La campagne est partielle : 24
runs sur 25, adaptatif `scanline_single_pass` absent, journal de session perdu,
trap non exécuté et aucun validateur final exécuté. Le reçu ne qualifie pas le
précomptage Q4, le layout SoA réduit ni le routage hôte direct, tous postérieurs
à sa source.

La factorisation `10c46c87` est utile et **a été exécutée sur une archive
propre** : 156/156 portes Release passent, dont 7 oracles, puis 9/9 portes
ciblées passent sous ASan+UBSan. Aucun P0 CPU, UB ou défaut de concurrence n'a
été reproduit. Les routes mixtes actuelles sont non vides dans les probes
manuels, mais les CTests ne contractualisent pas encore cette propriété. Les
trois P1 restent ouverts : capacité avant matérialisation device,
intégrité/autorité des résultats Q4, et protocole de preuve reproductible avec
non-vacuité démontrée.

## Résultats actuels

| Périmètre | Résultat établi | Portée exacte |
|---|---|---|
| CPU `10c46c87` | Release `gate` : **156/156**, dont 7 portes `oracle` ; ASan+UBSan ciblé : **9/9** | archive propre de `b79c001b`, dont les sources produit sont identiques à `10c46c87` et `a0d13420` |
| Routage CPU courant | probes Q3/Q4 mixtes et Q4 tout-hôte : vecteurs et compteurs conformes, aucun mismatch | preuves manuelles de non-vacuité, pas encore des assertions CTest suffisantes |
| G4 source `8f95df2e` | témoin device code 0, lane Q3/Q4 sans désaccord brut, mutant code 4, quatre contrats GPU 50 k code 0 | artefacts bruts cohérents, mais campagne partielle et non validée automatiquement |
| Adaptatif source `8f95df2e` | `eight_clusters` exerce les deux routes et conserve les deux digests | `scanline_single_pass` absent ; ancien chemin hôte matérialisé |
| Documentation et registre | 210 Markdown actifs et 20 phases passent leurs vérificateurs | les vérificateurs ne détectent pas les claims GPU ni les dérives sémantiques listées plus bas |

### Temps de bout en bout sur 50 000 points

| Famille | CPU 48 fils | GPU 48 fils | Surcoût GPU | Pic RSS CPU / GPU | Digests appariés |
|---|---:|---:|---:|---:|---|
| `uniform` | 78 s | 89 s | +14 % | 19,3 / 19,0 Go | `balls` et `all` |
| `terrain` | 23 s | 44 s | +91 % | 3,68 / 5,31 Go | `balls` et `all` |
| `scanline_single_pass` | 38 s | 96 s | +153 % | 3,10 / 7,25 Go | `balls` et `all` |
| `eight_clusters` | 246 s | 718 s | +192 % | 17,6 / 17,5 Go | `balls` et `all` |

L'adaptatif `eight_clusters` à `min_sites=256` prend **713 s** et 18,2 Go.
Il n'envoie pas « presque toutes les ancres » au device : environ 70,7 % en Q3
et 31,6 % en Q4, soit 59,9 % ensemble. En revanche, il y envoie 99,1 % des
seeds Q3 et 91,3 % des seeds Q4 ; le seuil par taille de cover laisse donc la
quasi-totalité du travail coûteux sur le GPU.

### Pourquoi le GPU est plus lent dans ce reçu

Le pin mesuré fait encore sur CPU la descente WSPD, les covers par ancre,
l'énumération des seeds, les formes et la matérialisation des lots, puis copie
les SoA et résultats. Sur `eight_clusters`, la route tout-device traverse
18,22 milliards de seeds Q3 et 1,49 milliard de seeds Q4. Le CPU de production
peut tuer tôt ces seeds sans fabriquer les enregistrements intermédiaires ; le
chemin GPU paie cette préparation avant que le kernel ne puisse aider.

Les chiffres étayent fortement ce diagnostic : la génération passe de 189 s
CPU à 659 s GPU, tandis que `kernel_ms=111196,5` est un cumul d'événements de
48 exécuteurs, pas un mur GPU. Ils ne suffisent toutefois pas à isoler une
cause unique : les fenêtres Q4 incluent aussi transferts, synchronisations et
compaction hôte. Écrire « matérialisation et orchestration probablement
dominantes » jusqu'à disposer de murs séparés préparation/H2D/kernel/D2H.

## Requalification de `10c46c87`

Le changement ferme bien un sous-problème : il n'existe plus deux lots hôte et
device simultanément par ouvrier. Une ancre routée hôte passe immédiatement par
le même corps sémantique que la production ; seules les ancres routées device
sont matérialisées. Cette factorisation réduit la duplication et donne une
bonne base pour comparer les backends.

Quatre limites doivent être explicites :

- Le reçu adaptatif à 713 s mesure l'ancien second lot hôte de `8f95df2e`, pas
  la route directe actuelle. Fermer seulement le **tout-device matérialisé au
  pin mesuré** comme voie de gain ; mesurer `10c46c87` avant de conclure sur
  l'adaptatif courant.
- Le callback générique de `generate_q3_batched_with` ou
  `generate_q4_batched_with` ne voit désormais que les ancres device ; la route
  hôte le contourne au profit du corps de production. Documenter cette
  sémantique de backend, surtout pour les callbacks de mesure ou mutants.
- Une émission hôte immédiate peut dépasser les émissions d'un lot device déjà
  en attente. Seule la sortie post-RLE reste canonique en routage mixte ; ne pas
  promettre l'ordre brut général.
- `anchors_host` et `anchors_device` ne décrivent plus la même population. La
  route hôte compte aussi les ancres sans seed Q3 et, en Q4, celles ensuite
  tuées par W4 ou sans seed ; la route device ne compte que les ancres
  matérialisées avec seeds. Pour prouver un mix, exiger `seeds_host > 0`,
  `seeds_device > 0` et des lancements device, ou séparer compteurs « routés »
  et « traités ». `host_flushes` est maintenant un champ mort toujours nul.

### P1 preuve — les portes de routage peuvent rester vertes à vide

Les probes manuels confirment que le code courant emprunte réellement les deux
branches : Q3 `uniform n=1200, min_sites=256` compte 5 204 ancres device et
117 228 hôte, avec 474 887 et 2 899 077 seeds ; Q4 compte 20 920 et 106 341
ancres, avec 4 605 159 et 1 366 207 seeds. Le probe Q4 tout-hôte donne zéro
ancre/seeds device et 34 876 ancres, 508 979 seeds hôte. Les vecteurs et tous
les compteurs comparés concordent.

Ces observations ne sont pas encore verrouillées. Les portes `route_256`
n'exigent pas de travail hôte ; leur `min_flushes=1` ne prouve qu'un vidage
device, car `host_flushes` n'a plus de producteur. La porte Q4 « tout hôte »
accepte `min_flushes=0` sans imposer `anchors_device == 0` ni un travail hôte
non nul, et Q3 n'a pas de porte tout-hôte. Ajouter des assertions min/max sur
les deux routes et leurs seeds, plus un mutant qui ignore le seuil.

Enfin, la route hôte et la production appellent maintenant exactement
`scan_anchor_q3` et `process_anchor_q4`. Leur égalité prouve l'orchestration,
pas indépendamment la sémantique mathématique. Les shaped gates, les oracles et
les campagnes différentielles v4/v5 restent les autorités distinctes.

## P1 — capacité device avant matérialisation

Le chemin hôte direct réduit la résidence, mais le chemin device garde le
défaut principal. En Q3 et Q4, une ancre complète est encore copiée dans les
SoA avant le test des seuils. Les conversions `size_t -> u32` précèdent le
validateur ; en Q4, `anchor_seeds * lens_count` et son addition au cumul ne
sont pas vérifiés. Une ancre isolée peut donc dépasser arbitrairement les caps
de sites ou de paires.

Fermeture recommandée :

1. précompter sites, seeds et paires avec casts, multiplication et addition
   vérifiés avant toute écriture ;
2. vider le lot avant l'ajout qui dépasserait le cap ;
3. tuiler l'ancre isolée, la basculer vers le corps hôte, ou rendre un refus de
   ressource structuré avant allocation ;
4. appliquer le même contrat aux buffers, à la grille et aux sorties de la
   future lane par rectangle ;
5. conserver une fixture de frontière sans allocation géante lorsque les tests
   seront de nouveau autorisés.

## P1 — résultats Q4 et autorité

`validate_q4_results_view` vérifie une somme d'étages et l'ordre sommaire des
émissions, mais n'impose pas `st.emit == n_emits`, ne recalcule pas le nombre
exact de complétions admissibles et ne vérifie pas que `y_site` est dans la
lentille et distinct de `x_site`, `skip_a` et `skip_b`. Les vues synthétiques
doivent aussi refuser tout pointeur nul associé à un compte non nul et les
sommes débordantes. Les mutants minimaux restent : émission perdue, `y=x`,
`y=skip`, `y` hors lentille et compteurs débordants.

À la frontière produit, les `LaneOverride` publics peuvent ne rien émettre et
laisser malgré tout le pipeline atteindre un statut terminal. Sceller ces
callbacks comme backend interne, ou rendre toute sortie externe explicitement
expérimentale et non autoritative. Un callback vide ne doit pas pouvoir produire
`complete_regular`.

## P1 preuve — rendre la prochaine campagne terminale

Le nouveau reçu compense manuellement plusieurs faiblesses : les deux digests
concordent sur les quatre couples bruts, et l'adaptatif `eight_clusters` a des
seeds des deux côtés. Le contrat automatisé reste insuffisant :

- le validateur n'impose que `digest_all`, pas `digest_balls` ;
- il n'exige ni `min_sites=256`, ni seeds hôte/device non nulles, ni lancement
  device pour les runs adaptatifs ;
- son faux pilote ignore encore le seuil et ne falsifie donc pas ce défaut ;
- la phase GPU n'est pas conditionnée par `gpu_lane code=0` et
  `gpu_mutant code=4` ;
- `--gpu-min-sites=-1` devient silencieusement `SIZE_MAX` au lieu d'être refusé
  avec le code 2 ;
- la campagne reçue manque le second adaptatif, le journal, le verdict du
  validateur, les codes session/rapatriement et la sortie certifiant l'arrêt.

La prochaine session doit recevoir séparément : petites portes device, mutant,
deux routes réellement non vides, quatre couples CPU/GPU avec les deux digests,
et stratégie adaptative. Le tout-device dense peut rester un diagnostic de
ressource distinct ; ne pas rendre son succès obligatoire pour promouvoir une
stratégie qui ne l'utilise pas.

## Contrat conseillé pour la livraison 7 par rectangle

La direction attaque le bon poste et reste compatible avec l'invariant
d'architecture : elle ne matérialise pas la mosaïque de Delaunay d'ordre
supérieur. Mais déplacer l'énumération sur le device ne suffit pas à garantir
la mémoire ni le gain. `rect_cover_handles` fournit une antichaîne locale dont
l'union contient les covers des ancres, mais ce « cover de rectangle » est un
**sur-ensemble fail-open**, pas le cover exact de chaque ancre. Le filtre exact
et l'ordre stable en 32 bins doivent donc être reproduits après projection.

Le claim `somme covers rectangles << somme covers ancres` n'est pas mesuré. Le
reçu `eight_clusters` ne montre qu'environ 4,7 ancres device par rectangle Q3
et 1,6 en Q4 ; un sur-ensemble lâche peut coûter plus que la somme des covers
exacts dans un rectangle peu peuplé. Il faut recevoir les tailles aplaties et
leurs quantiles avant d'attribuer un gain. Avant le kernel :

1. définir l'entrée comme le candidat rectangulaire issu des handles, pas comme
   un « cover du rectangle » supposé exact ; mesurer somme et maximum des sites,
   `|A| * |B|`, seeds, complétions et survivants ;
2. reproduire le filtre exact et l'ordre stable actuel des 32 seaux radiaux, ou
   redéfinir et requalifier les compteurs : les arrêts précoces rendent les
   compteurs dépendants de l'ordre de visite ;
3. employer précomptage/prefix-sum vérifié et tuilage déterministe pour ancres,
   seeds, complétions et survivants, avec reprise explicite d'overflow ;
4. ne conserver ni tous les covers aplatis ni une matrice rectangle × ancre ×
   point ; borner aussi le retour des survivants et agréger tous les compteurs ;
5. établir d'abord sur CPU l'égalité ensemble **et ordre** de chaque cover avec
   `cover_query`, puis l'égalité de la lane shaped brute, post-RLE et compteurs ;
6. si `Gd`, `Nd`, `bound` ou `Jlo/Jhi` passent sur device, certifier la
   conversion DI128 vers binary64 bit à bit aux frontières d'arrondi ;
7. mesurer séparément somme et maximum des handles/covers, visites, octets
   H2D/D2H, préparation, kernel et compaction aux tailles 8k à 50k.

Cette voie est prometteuse si elle partage réellement un candidat de cover
entre beaucoup d'ancres et émet peu de survivants. Ces deux rapports sont des
quantités à recevoir, pas des hypothèses à transformer en claim. Une variante
plus sobre à mesurer est un tableau global O(n) de positions/`PointId`, partagé
entre ouvriers, avec seulement les plages de handles transférées par fenêtre de
rectangles. Le dupliquer dans 48 exécuteurs recréerait un défaut de résidence.

## P2 — nettoyage utile

- Rafraîchir [`GPU.md`](../docs/GPU.md) : remplacer « exacte » par l'égalité
  bornée au pin `8f95df2e`, corriger « presque toutes les ancres » et « lots
  bornés », séparer le layout 8f mesuré du layout 10c courant et retirer ou
  dater les sections qui disent encore Q4 « en attente de G4 ». Le sujet du
  commit `a0d13420` et ses mentions `K=1..10 exact` ne sont pas des certificats.
- Passer événements et allocations CUDA sous RAII et distinguer temps kernel,
  synchronisations, transferts et compaction.
- Corriger les commentaires « i64 et doubles exactes », l'estimation Q4 à
  environ 96 octets/site et le libellé « cumul des ouvriers » pour un temps
  mural.
- Retirer ou renommer `host_flushes`, puis aligner les compteurs imprimés et le
  validateur de campagne sur leur nouvelle sémantique.
- Conserver le mutant Q3 reçu et ajouter, lors d'une prochaine campagne
  autorisée, un mutant propre à la lane Q4 réellement exécuté sur device.
- Corriger la tabulation qui ampute `S_\tau` dans
  [`ARCHITECTURE.md`](../docs/ARCHITECTURE.md), puis faire refuser par
  `check_docs.py` les tabulations et commandes LaTeX amputées.
- Mettre à jour [`MATHEMATIQUES.md`](../docs/MATHEMATIQUES.md) : relabeling,
  mutants de rendu et oracle de forêt ont désormais des portes. Cela ne livre
  pas pour autant le payload public de rendu, qui reste à distinguer.
- Aligner [`PLAN_DE_TESTS.md`](../docs/PLAN_DE_TESTS.md) sur le vrai périmètre
  de `mutants_gate` et sur la campagne G4 déjà reçue. Dans
  [`PROVENANCE.md`](../docs/PROVENANCE.md), remplacer les noms de cibles
  inexistants, unifier la classification de `device_forms` et borner le reçu
  GPU à sa source.
- Le pin différentiel `receipts/conformite_v4/familles_v4.txt` nomme un
  programme compilé hors dépôt sans source, commande, toolchain ni hash
  binaire. Le conserver comme historique, mais le régénérer de façon rejouable
  avant de lui donner davantage d'autorité.

## Ordre de fermeture conseillé à Claude

1. Corriger immédiatement les claims et contradictions de `GPU.md`.
2. Durcir le validateur Q4 et l'autorité des callbacks.
3. Introduire le préflight vérifié et la politique d'ancre trop grande.
4. Écrire la porte CPU par rectangle avec ordre et capacités explicites.
5. Durcir le protocole sur les deux digests, les routes et les préconditions de
   phase, puis seulement programmer une nouvelle session G4 gardée.

## Reproduction et limites de cet audit

Le code produit de `b79c001b` a été exporté dans une archive propre ; il est
identique à `10c46c87` et `a0d13420`. Résultats locaux :

```text
cmake -S <archive>/morsehgp3D_v5 -B <build-release> -DCMAKE_BUILD_TYPE=Release : code 0
cmake --build <build-release> --parallel 4 : code 0
ctest --test-dir <build-release> --output-on-failure --parallel 4 -L gate : 156/156, 146,45 s
ASan+UBSan, 9 portes API/batch/concurrence/routage/caps ciblées : 9/9, 229,24 s
python tools/check_docs.py : 210 Markdown actifs validés
python tools/check_implementation_status.py : 20 phases validées
```

Le validateur épinglé du reçu G4, rejoué localement avec les codes de transport
supposés nuls, rend `campaign_status=partial_or_failed` sur le seul artefact
adaptatif `scanline_single_pass` absent. Les hashes du payload source et du
manifeste se recomposent exactement. La comparaison indépendante de toutes les
lignes `digest_balls`, `digest_all` et des forêts K=1..10 confirme les accords
CPU/GPU décrits plus haut, sans étendre leur portée au tip.

`nvcc` est absent : aucune compilation CUDA courante n'est revendiquée. GCP
n'a pas été interrogé ni muté par cet audit ; l'arrêt raconté par le reçu n'a
donc pas été recertifié indépendamment. Le probe concurrent
`.codex_fold_contract_probe.cpp` n'a été ni ouvert, ni modifié, ni inclus.
