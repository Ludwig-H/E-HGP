# FULL sans continuations : première occurrence du parent

27 septembre 2026, prototype audit-only, moteur inchangé, aucun GCP.
Cadre `exploration_v9_hors_registre`, `cpu_reference`,
`quantized_u18_input_only`, `mode=first_parent_structural_encoding`,
`public_status=not_claimed`.

## Pourquoi cette variante

Les [quatre vraies tours capturées](../b_full_real_drafts_20260927/README.md)
(LiDAR00 sans sol, uniforme 8k/16k/32k, K1..5) ne contiennent aucune
continuation. Le prototype général à tri d'incidences y est exact mais
plus lent que le natif scalaire. Le présent prototype utilise cette
condition **vérifiée globalement**, pas supposée d'après une famille de
nuages, pour supprimer le tri et deux tableaux de préfixes.

Si une action a exactement un parent, même dans le dernier lot ou dans
un draft finalement refusé, il appelle intégralement l'encodeur général.
Les naissances, coquilles étendues, niveaux et K n'ont pas été restreints.
Ce n'est pas une nouvelle approximation du problème FULL.

## Algorithme et preuve

Après contrôle de forme CSR et domaine, vérifier qu'aucune action n'a
un parent unique. Alors **chaque action crée un nœud**, donc V=A,
`node_id=a`, `prior_count=boffset[b]`, et les offsets de parents de sortie
sont exactement `draft.parent_begin`. Pas de préfixes supplémentaires.

Initialiser `first[V]` à l'absent. Pour chaque occurrence j de parent p,
si `p<V`, réduire `first[p]` par le minimum de j. Le tableau CSR des parents
est déjà dans l'ordre lot/action/position. Une deuxième passe refuse
chaque occurrence autre que la première, en plus des tests locaux et
du contrôle strict `p<boffset[b]`. Un p hors de V ne sert **jamais** d'indice.

Sans continuation, toute utilisation d'un parent est une fusion qui le
consomme définitivement. La première utilisation d'un parent préexistant
est donc valide ; toute utilisation suivante est exactement le refus de
`live`. Le minimum numérique j attribue la faute au deuxième usage, pas
au premier ni au premier thread arrivé. Un parent `p<V` créé trop tard
peut participer au minimum : son erreur locale antérieure domine tout
effet induit dans le suffixe. Le premier refus est toujours réduit par
lot, en-tête/action, action, étape, position, instruction.

Après validation, copier `draft.parent` dans la sortie, écrire chaque
nœud à son ID d'action et ses contributions au même segment. Chaque
parent admis a une seule occurrence, donc l'écriture de son successeur
est sans conflit. Les niveaux gardent leurs mots/dénominateurs exacts.

Contre-exemple interdisant la généralisation : naissance de 0,
continuation de 0, fusion avec 0. Les deux usages sont légaux ; c'est
pourquoi l'absence de continuation est testée **sur tout le draft avant**
le chemin rapide et pourquoi l'autre cas repart sur la référence générale.

## Coût et portée

Chemin spécialisé : O(B+A+P+C) opérations, plus sorties, sans tri.
Scratch demandé sur ce build : `batch[A]` et `first[V]`, soit 16A octets
en 64 bits. Allocations, copie des parents et écriture des sorties restent
payées ; ni métadonnées d'allocateur ni pile ne sont comptées dans ces 16A.
Le repli paie en plus une vérification de forme/domaine et la détection
d'une continuation avant le chemin général O(P log P).

La réduction minimum est ici **scalaire**, exercée en ordre normal et
inverse. Un futur `atomicMin` est possible, mais aucun thread/GPU n'est
exécuté par cette sonde. Ce sont des bornes en taille du draft, pas une
borne sous-quadratique en nombre de sites. Aucun gain chronométrique n'est
encore acquis par les reçus de qualification ci-dessous.

Même contrat que le prototype précédent : banque authentique immuable,
types et comparateur exact empruntés au produit, premier refus sémantique
comparé lorsque les allocations réussissent. Pas d'égalité du N-ième échec
mémoire, ni géométrie, ni verticales recalculées.

## Attribution et qualification

`first.hpp` porte explicitement les contrôles locaux de
`b_full_batch_encoder_20260927/encode.hpp`, le nouveau critère remplaçant
seulement sa gestion des incidences. Le fallback est ce prototype figé,
pas le constructeur natif appelé pour décider. `fixture_helpers.hpp`
est le port mécanique attribué des fixtures/comparateurs du gate précédent,
sans son main ; un helper mutant conservé est marqué `maybe_unused`.
Le runner réutilise son collecteur local par import configuré, sans éditer
les sources anciennes. La bibliothèque géométrique n'est pas liée ici.

Capture `receipts/full_first_parent_20260927/r1` close dès le premier
essai : **16 commandes**, Release GCC 13.3 et Clang 18.1
ASan/UBSan/LSan explicites, sorties égales. Par binaire :

- 9 647 entrées × deux calendriers = 19 294 comparaisons ;
- 805 acceptées, 8 842 refusées ;
- 5 642 passages spécialisés, 13 622 replis ; 30 refus de forme/domaine
  avant sélection de chemin ;
- quatre mutants spécialisés réfutés par objet/motif incorrect, code 1,
  sans crash : doublon ignoré, parent du même lot admis, premier défaut
  rencontré au lieu du minimum canonique, dernière occurrence au lieu
  du minimum d'ordinal.

Les cas prioritaires couvrent le parent UINT64_MAX, le parent futur dans
la taille V, les doubles défauts, les références contrôlées par ordinal,
la fusion avec population invalide avant réemploi et le fallback tardif.
Les contrôles ciblés nouveaux sont bien sans continuation lorsqu'ils
doivent éprouver le chemin spécialisé. Les 400 historiques additionnels
retirent les continuations sans renuméroter les nœuds, puis leurs lots
devenus vides ; ils servent de drafts structurels, **pas de LiDAR**.

Lectures normal/−O réussies. Sources et dépendances compilées hachées
avant/après, argv/env/sorties/codes liés. Le lecteur vérifie l'ensemble
exact des deux binaires, sans rejouer les exécutions. Build désormais
épinglé : `/workspaces/E-HGP/build/v9-audit-full-first-parent-20260927-r1`.

```bash
python3 -B morsehgp3D_v9/audits/b_full_first_parent_20260927/run.py check
python3 -B -O morsehgp3D_v9/audits/b_full_first_parent_20260927/run.py check
```

Prochaine mesure : une sonde réelle distincte, clairement attribuée, qui
compare natif/général/spécialisé sur les mêmes vrais drafts et publie
génération, copie, allocations et mémoire. Les captures existantes ne sont
pas réinterprétées en performance du nouveau chemin.
