# État courant — audit de MorseHGP3D v5

- **Date :** 27 août 2026
- **Auditeur :** Codex
- **Pin fonctionnel relu statiquement :** `10c46c87bbda13a3fda697c9dedb94fead273faa`
- **Dernier pin exécuté par cet auditeur :** `94464cfb3ba23d8c65d780d981808f8a8e50ffa5`
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

La factorisation `10c46c87` est utile et ne montre pas de P0 évident en lecture
statique : la production et la route hôte partagent désormais
`scan_anchor_q3` et `process_anchor_q4`, et la route hôte ne construit plus un
second lot. Elle n'a pas été exécutée dans cet audit, conformément à la demande
de ne lancer aucun test. Les trois P1 restent donc ouverts : capacité avant
matérialisation device, intégrité/autorité des résultats Q4, et protocole de
preuve reproductible avec non-vacuité démontrée.

## Résultats actuels

| Périmètre | Résultat établi | Portée exacte |
|---|---|---|
| CPU `94464cfb` | Release `gate` : **156/156**, dont 7 portes `oracle` ; campagnes ciblées 36/36 et ASan+UBSan 10/10 | dernières exécutions de cet auditeur, antérieures à `10c46c87` |
| Code `10c46c87` | corps par ancre factorisés ; route hôte directe sans `Q3Batch`/`Q4Batch` ; un seul lot device par ouvrier | relecture statique seulement ; aucune preuve CUDA ni mesure 50 k à ce pin |
| G4 source `8f95df2e` | témoin device code 0, lane Q3/Q4 sans désaccord brut, mutant code 4, quatre contrats GPU 50 k code 0 | artefacts bruts cohérents, mais campagne partielle et non validée automatiquement |
| Adaptatif source `8f95df2e` | `eight_clusters` exerce les deux routes et conserve les deux digests | `scanline_single_pass` absent ; ancien chemin hôte matérialisé |

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
la mémoire ni le gain. Avant le kernel :

1. définir l'entrée comme le candidat rectangulaire issu des handles, pas comme
   un « cover du rectangle » supposé exact ; mesurer somme et maximum des sites,
   `|A| * |B|`, seeds, complétions et survivants ;
2. reproduire l'ordre stable actuel des 32 seaux radiaux, ou redéfinir et
   requalifier les compteurs : les arrêts précoces rendent les compteurs
   dépendants de l'ordre de visite ;
3. employer précomptage/prefix-sum vérifié et tuilage déterministe pour ancres,
   seeds, complétions et survivants, avec reprise explicite d'overflow ;
4. ne jamais matérialiser globalement le produit ancres × seeds ; borner aussi
   le retour des survivants et agréger tous les compteurs contractuels ;
5. établir d'abord sur CPU l'égalité par rectangle avec la production, puis
   comparer post-RLE, compteurs et `digest_balls` sur device ;
6. mesurer séparément préparation hôte, octets H2D/D2H, mur kernel et
   compaction, au même pin et contre la production CPU.

Cette voie est prometteuse si elle partage réellement un candidat de cover
entre beaucoup d'ancres et émet peu de survivants. Ces deux rapports sont des
quantités à recevoir, pas des hypothèses à transformer en claim.

## P2 — nettoyage utile

- Rafraîchir [`GPU.md`](../docs/GPU.md) : remplacer « exacte » par l'égalité
  bornée, corriger « presque toutes les ancres », séparer le layout 8f mesuré du
  layout 10c courant et retirer les sections qui disent encore Q4 « en attente
  de G4 ».
- Passer événements et allocations CUDA sous RAII et distinguer temps kernel,
  synchronisations, transferts et compaction.
- Corriger les commentaires « i64 et doubles exactes », l'estimation Q4 à
  environ 96 octets/site et le libellé « cumul des ouvriers » pour un temps
  mural.
- Retirer ou renommer `host_flushes`, puis aligner les compteurs imprimés et le
  validateur de campagne sur leur nouvelle sémantique.
- Conserver le mutant Q3 reçu et ajouter, lors d'une prochaine campagne
  autorisée, un mutant propre à la lane Q4 réellement exécuté sur device.

## Ordre de fermeture conseillé à Claude

1. Corriger immédiatement les claims et contradictions de `GPU.md`.
2. Durcir le validateur Q4 et l'autorité des callbacks.
3. Introduire le préflight vérifié et la politique d'ancre trop grande.
4. Écrire la porte CPU par rectangle avec ordre et capacités explicites.
5. Durcir le protocole sur les deux digests, les routes et les préconditions de
   phase, puis seulement programmer une nouvelle session G4 gardée.

## Reproduction et limites de cet audit

Lecture statique de `10c46c87`, de `a0d13420`, des sorties brutes du nouveau
reçu et du protocole. **Aucun build, test, validateur, benchmark ou accès GCP
n'a été lancé pendant cette mise à jour.** Les résultats locaux historiques du
pin `94464cfb` restent consultables dans l'historique de cet état ; les mesures
G4 restent dans le reçu épinglé. Le probe `.codex_fold_contract_probe.cpp` n'a
été ni modifié ni inclus.
