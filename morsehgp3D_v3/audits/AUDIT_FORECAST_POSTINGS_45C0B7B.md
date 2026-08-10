# Audit constructif du forecast postings à 50 k

Date : 10 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_and_bounded_oracles`,
`profile=quantized_u16_input_only`, `mode=audit_independant`, aucun statut
public. Source auditée : commit
`45c0b7bfe4e908cd05c6269ae2e651e629370e6d` et
[`NOTE_CLAUDE_BENCHMARK_POSTINGS_FORECAST_20260810.md`](NOTE_CLAUDE_BENCHMARK_POSTINGS_FORECAST_20260810.md).

## Verdict utile

La conclusion de planification de Claude est robuste : le join monolithique
plein est un **NO-GO prévisionnel** à 50 k, même si l'exposant ajusté reste très
incertain. La note sépare correctement famille tronquée, mesure, forecast et
exactitude scientifique. Les deux réductions proposées — filtrage par ordre
avec `q_min` sous source complète, puis arbre exact à `k=1` — attaquent la masse
elle-même et sont donc les bons prochains leviers avant tout kernel.

Une correction de contrat est néanmoins obligatoire : `P_post` exact est une
autorité de **travail** relativement au catalogue fourni, mais le
`predicted_peak_bytes` courant n'est pas une autorité de **mémoire**. Il omet
des états et payloads introduits par les records; les expressions « pic
conservateur » et « préflight seul juge du GO/NO-GO mémoire » sont donc fausses
sur `45c0b7b`. Employer « pic estimé » jusqu'à une borne complète ou un
allocateur plafonné.

## Recalcul indépendant du modèle

La régression linéaire de `log(P_post)` sur `log(n)` des cinq lignes donne une
pente `1,66424`, proche du `~1,68` descriptif annoncé, et prévoit
`3,71e12` occurrences à 50 k. L'arrondi `~4e12` est donc correct.

Les résidus multiplicatifs aux cinq tailles sont respectivement environ
`0,955`, `1,047`, `1,017`, `1,012`, `0,972`. Ils sont petits sur la fenêtre,
mais les pentes locales valent `1,93`, `1,58`, `1,65`, `1,55`; extrapoler de
400 à 50 000 multiplie la taille par 125. En ancrant simplement les deux pentes
locales extrêmes au point `n=400`, le scénario va déjà de `2,0e12` à
`1,3e13`. Cette largeur interdit un forecast précis, mais ne sauve aucune forme
monolithique.

Avec `4e12` occurrences :

- à 5--10 M occurrences/s sur un cœur : environ 4,6--9,3 jours;
- à 48 cœurs avec accélération linéaire idéale : environ 2,3--4,6 heures;
- à 1 G occurrence/s : environ 1,1 heure.

La formulation « jours / heures / ordre de l'heure » de Claude est donc
arithmétiquement juste et même optimiste pour le multi-cœurs, où bande passante
et merge réduisent l'accélération.

## Rejeu positif d'une ligne

Commande rejouée localement :

```text
mhgp3v_saturated_pipeline --points 100 --smax 11 --max-order 5 --seed 20260810 --join postings
```

Le payload combinatoire reproduit exactement la première ligne : `G=14 954`,
pool `121 269`, `P_post=114 337 554`, plus gros lot `119 104`, unions
`71 928`, maximum de posting `3 915`, identités respectées. Le timing local
n'est pas comparable : le rejeu partageait les deux CPU avec un build Release
et la régression complète; il donne 9,40 s de catalogue et 20,32 s de fold.
Ce résultat reçoit les masses, pas la chronométrie publiée.

## Mesure décisive avant toute nouvelle optimisation

Le prochain sweep ne doit pas refaire seulement `P_post` plein. Le préflight
peut publier, sans émettre une seule paire, pour chaque ordre `k` :

- `G_k`, nombre de générateurs avec `rank>=k` et `q_min<=k+1`;
- `L_k`, leur masse de membres;
- minimum, maximum et histogramme des degrés restreints `d_x^(k)`;
- `P_k`, somme des `C(d_x^(k),2)`;
- à `k=1`, la masse d'étoiles `S_1`, somme des `max(0,d_x^(1)-1)`;
- histogramme des fenêtres utiles des paires, de
  `max(1,q_M-1,q_N-1)` à `min(K,rank_M,rank_N,|M intersection N|)`.

Les quatre premiers champs coûtent un balayage des membres par ordre. Ils
répondent immédiatement à la vraie question : quelle fraction de la masse
pleine disparaît grâce au théorème `q_min`, avant d'investir dans threads,
runs ou GPU? `S_1` est une borne de travail avant déduplication des arêtes
répétées entre postings.

Cette mesure n'autorise la réduction que si `q_min_certified` et
`complete_for_order[k]` sont présents. Sur la famille `smax=11` de la note,
elle reste un diagnostic contrefactuel; l'appliquer au fold modifierait le
raffinement partiel.

## Protocole minimal pour rendre le forecast rejouable

Inutile de payer plusieurs nouveaux runs `n=400` avant la réduction. Pour la
table existante, ajouter un sidecar avec commit, commande exacte, compilateur,
machine, digest d'entrée, sortie brute et état de charge. Pour estimer la
variabilité, trois graines aux tailles `100,141,200,283` suffisent d'abord;
publier médiane, min/max et pentes locales. Le point 400 actuel peut rester un
ancrage unique explicitement étiqueté.

## Corrections documentaires immédiates

1. Remplacer dans la note et le pipeline « pic conservateur » par « pic
   estimé ».
2. Écrire : « `P_post` exact juge l'admission en travail; le budget mémoire
   reste expérimental jusqu'à une borne complète ou un allocateur plafonné ».
3. Retirer du pipeline la phrase « séparation `q_min` en cours de réception » :
   le commit `45c0b7b` reçoit déjà les records sur campagnes bornées; ce qui
   reste ouvert est leur provenance runtime et le raccord du pipeline.
4. Conserver `forecast_only` et `partial_refinement` dans chaque export ou
   sidecar; ne jamais réduire ces deux libellés à une note de bas de page.

## Décision GPU

GCP non utilisé. C'est la bonne décision : aucun kernel de runs bornés n'existe
encore et le profil par ordre manque. Une G4 mesurerait une constante sur la
mauvaise masse, sans fermer ni source, ni mémoire, ni reprise.
