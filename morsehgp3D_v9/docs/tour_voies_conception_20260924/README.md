# Tour et noyau des voies : conception après R16 (24 septembre 2026)

```text
phase=exploration_v9_hors_registre
backend=reference_cpu (+ cuda_g4 à qualifier)
profile=quantized_u18_input_only
mode=conception_puis_port
public_status=not_claimed
```

**GCP non utilisé pour la conception.** Après R16, deux postes dominent le
chemin critique de 08/000000 à K5 (1,53 s) : la **tour**, 566 ms, et
l'**appel des voies**, 275 ms, dont un noyau de 190 ms que les ouvriers
attendent. Un workflow a produit, pour chacun, trois conceptions
indépendantes et un juge. Les prototypes et les modèles restent hors dépôt :
ce ne sont pas des reçus. Toute durée G4 ci-dessous qui n'est pas lue dans
un reçu est une **projection**.

## Tour

Les trois conceptions : ordonnancement (tower-parallel), retrait de travail
(tower-work), phase 0 sur l'appareil (tower-gpu).

Le diagnostic retenu par le juge : sur G4, la tour est bornée par le
**série**, la **mémoire** et le **mono-fil par ordre**, pas par la
géométrie. Trois faits le fondent :
- 24 fils ≈ 48 fils pour la validation et la phase 0 (R13, R8) ;
- la phase 0 est proportionnelle aux requêtes, environ 73 ns par requête à
  K5 ;
- à K5, l'ordre critique de la fenêtre est K3 et non K5 : la phase 0
  séquentielle pèse autant que la phase A.

Plan par étapes :

| étape | contenu | projection tour K5 (opt. / pess.) |
| --- | --- | ---: |
| E0 + E1 | sous-chronos ; arène de requêtes sans mise à zéro ; images par rangs de plateau | 437 / 469 ms |
| E2 | pool persistant, sections série de la validation et du tri | 364 / 406 ms |
| E3 | phase A maigre séquentielle, numérotation produit | 247 / 327 ms |
| E4 | queue en pipeline, IDs de population par décalage statique | 194 / 276 ms |
| E5 | encodage scindé | 173 / 253 ms |
| E6 | phase 0 sur l'appareil, Kmax d'abord | 133 / 229 ms |

Écartés : la fusion de la phase 0 des ordres (une requête d'ordre K a K
sites, aucun travail partagé) et la validation différée. Différés : le
témoin d'intrus lu au catalogue et le saut D5 (auditeur C), ainsi que le
catalogue scellé, qui est une décision de frontière de confiance.

## Noyau des voies

Les trois conceptions : tâches (lanes-tasks), chronologie et recouvrements
(lanes-pipeline), réduction du travail (lanes-algorithm).

Le juge a recalé un modèle à débit plafonné sur les douze noyaux G4 de R15
et R16 (4,4 % d'erreur RMS). Il en tire un plancher de 67 à 78 ms à K5 avec
le travail actuel : des tâches seules ne suffisent pas, il faut aussi
réduire le travail de masse.

| étape | contenu | projection de l'appel K5 |
| --- | --- | ---: |
| 1 | hôte (H1) ; T1 (candidats indépendants) ; chronologie | ≈ 163 ms |
| 2 | tâches (arête, plage de graines), placement par balayage | ≈ 90 ms |
| 3 | plages du cover exportées par les certificats ; élagage L11 ; ordre L10 ; passe fusionnée L15 | ≈ 56–67 ms |

## Étape 1 réalisée (commit de cette conception)

**Tour.**
- **E1, arène de la phase 0** : les requêtes, les cibles statiques et les
  seaux du tri sont alloués **sans mise à zéro** (`src/common/raw_vector.hpp`).
  L'arène des requêtes est réutilisée d'un ordre à l'autre, puis libérée. Les
  builds de test remplissent ces tampons d'un motif avant la première
  écriture, pour qu'une case jamais écrite casse les condensés.
- **E1, images par rangs** : le curseur des images compare les rangs de
  plateau u32 (`level_run + 1`, 0 pour le niveau zéro) au lieu des niveaux
  U320. La voie séquentielle garde les niveaux exacts comme témoin. Deux
  mutants (coupe fermée lue ouverte ; niveau zéro au rang du plus bas niveau)
  sont tués par la porte des voies statiques.
- **E0** : sous-chronos de la validation (8 parts) et de la phase 0 par
  ordre (collecte, tris, groupes, résolution), publiés par la sonde v22.

Mesures locales entrelacées sur 08/000000/K5 (W8, hôte chargé, indicatif) :
phase 0 −11 % ; images de l'ordre 5 de 132–143 à 47–57 ms. Condensés et
`tower_work` identiques.

**Voies.**
- **H1** :
  - les buffers de l'appel sont résidents, ne font que grandir, et sont
    réservés pendant q2 (`warm_up_lanes`) : plus de `cudaMalloc`/`cudaFree`
    d'environ 9,5 Go par appel ;
  - les propriétés de l'appareil ne sont lues qu'une fois ;
  - le dimensionnement compte les buffers résidents comme libres : mêmes
    décisions quel que soit l'historique ;
  - plus d'initialisation par valeur des enregistrements, côté appareil et
    côté chaîne.
  - Laissé de côté : un anneau épinglé de D2H, parce que sa taille est
    inconnue au préchauffage.
- **T1** (`q4_lanes.hpp`) : chaque candidat non étranger d'un seau vivant
  résout seul sa classe de racines, avec **sortie anticipée** dès que la
  profondeur atteint T. Les membres lus sont marqués décidés. Les émissions
  du seau sont retriées par représentant : ce sont les mêmes enregistrements,
  dans le même ordre (lemme L9 au registre des preuves). Sur 08/000000 à K5,
  toutes les arêtes sont égales au moteur et `group_steps` baisse de 70 %.
- **Chronos** : installation et fin de l'appel hors événements, conversion
  par la chaîne (sonde v22).

La session G4 R17 mesure l'étape 1 des deux pistes.
