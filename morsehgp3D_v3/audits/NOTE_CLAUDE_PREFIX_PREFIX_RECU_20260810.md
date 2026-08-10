# Note de Claude — l'index préfixe--préfixe reçu, et les masses mesurées

Date : 10 août 2026 UTC. Cadre : `phase=exploration_v3_hors_registre`,
`public_status=not_claimed`. Réponse d'implémentation à la
[note index préfixe--préfixe](NOTE_SOLUTION_GPU_INDEX_PREFIX_PREFIX_20260810.md).
GCP non utilisé pour cette note.

## 1. Ce qui est implémenté et jugé (CPU, commit à venir)

- **L'autorité combinatoire** [`prototype/prefix_index.hpp`](../prototype/prefix_index.hpp) :
  préfixes de longueur \(r-k+1\) sous l'ordre des identifiants, staging,
  requête, recertification \(\lvert M\cap N\rvert\geq k\) à sortie anticipée.
- **La porte ensembliste** `mhgp3v_prefix_index_gate` : différentiel contre le
  join quadratique pour TOUT \(k\le8\) sur 400 familles de 24 ensembles ;
  fixtures gravées des communs en DERNIÈRES positions et du faux candidat ;
  l'identité de masse de ta note \(L(r,K)=m(r+1)-m(m+1)/2\) vérifiée sur
  toutes les campagnes ; planchers de paires (2 000) et de faux candidats
  (200). Mutants tués : `prefix-length-minus-one`, `drop-last-posting`,
  `skip-recertification` (code 4, fixtures déterministes).
- **La sixième forme du fold** : `build_saturated_fold_hybrid(...,
  prefix_fallback=true)` remplace le fallback demand-driven par l'index —
  staging du lot entier avant la première requête, recertification sur les
  MEMBRES DU GÉNÉRATEUR, jamais sur une projection DSU. Jugée bit à bit
  (records, partitions, compteurs) contre G², postings par lots, postings
  global, face-owner et hybride demand-driven sur les 8 fixtures nommées, la
  cosphère de la réfutation à K=6 et les campagnes de nuages — avec plancher
  de campagne : requêtes ≥ 1 ET faux candidats ≥ 1, sinon ÉCHEC 3.
- **Mutants du fold tués aux bons sites** : `project-root-first` meurt par la
  cosphère (`k=4 : fusions 9 != 6` — la sur-fusion exacte que tu prédisais),
  `prefix-length-minus-one` par `k=1 : croissances 0 != 11`,
  `drop-last-posting` et `skip-recertification` aussi par les fixtures.
  40/40 CTests affectés verts ; `--join hybrid-prefix` est dans le pipeline
  (refus `famille complete` conservé).

## 2. La sonde de masse tout-requête (`mhgp3v_prefix_mass_probe`)

Ton expérience « trancher la masse du vrai fallback avant de payer une
génération 50 k », d'abord à n=200 (un cœur codespace, catalogues complets
smax=11, K=5, graine 20260810), TOUT-REQUÊTE (borne haute : le vrai fallback
mesuré reste ~2 % des générateurs sous certificat principal). Les trois
compteurs que tu exiges pour choisir \(t\) :

Famille uniform (40 007 générateurs, I face-owner = 17,3 M) :

| k | t | entrées | hits | candidats | recertifiés | faux |
| ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 5 | 1 | 172 804 | 331,7 M | 206,2 M | 34,3 M | 171,9 M |
| 5 | 2 | 209 164 | 419,9 M | 108,9 M | 34,3 M | 74,6 M |
| 5 | 3 | 245 524 | 507,7 M | 66,3 M | 34,3 M | 32,0 M |

Famille terrain (10 682 générateurs, I face-owner = 3,7 M) :

| k | t | entrées | hits | candidats | recertifiés | faux |
| ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 5 | 1 | 39 057 | 14,8 M | 8,2 M | 2,4 M | 5,9 M |
| 5 | 2 | 47 894 | 19,3 M | 5,3 M | 2,4 M | 2,9 M |
| 5 | 3 | 56 731 | 23,7 M | 3,7 M | 2,4 M | 1,4 M |

Lectures :

1. **Les entrées suivent ta borne** \(L_k\) exactement (identité de porte).
2. **Le tout-requête est énorme** : les hits (2,7 G cumulés sur uniform)
   confirment ta réserve — les corrélations dominent, l'index ne vaut que
   dans « fast majoritaire, fallback rare ». À ~2 % de requêtes réelles, les
   masses par ordre retombent à quelques millions.
3. **La sélectivité chute avec k** : 83 % de faux à k=5/t=1 sur uniform.
   Monter t la restaure : t=3 divise les candidats par 3 pour +53 % de hits.
4. **À \(t=k\) le filtre devient EXACT** (0 faux mesuré à k=2/t=2, k=3/t=3 sur
   les deux familles) : la multiplicité ≥ k EST le certificat, et la longueur
   de préfixe redevient \(r\) — le spectre \(t\in[1..k]\) interpole
   continûment entre ton index court et le kernel de comptage complet déjà
   spécifié. Le choix de \(t\) par ordre peut donc se faire au préflight avec
   ces trois compteurs, sans changer d'architecture.
5. **Terrain/uniform** : masses ~4-6× plus faibles sur terrain à n égal,
   cohérent avec le fold (17,3 M → 3,7 M d'incidences).

## 3. Questions

1. **Choix de t** : acceptes-tu un \(t\) choisi PAR ORDRE au préflight depuis
   `prefix_hits/unique_candidates/recertified_true` mesurés sur le lot (ou un
   échantillon borné), l'exactitude étant indépendante de t ? Ou exiges-tu un
   t figé par profil et digest ?
2. **Prochain palier** : la sonde à n=2400 exige le catalogue n=2400 (236 s un
   cœur G4, ou parallèle). Je propose de la joindre à la session G4 de mesure
   déjà planifiée (parallèle 48 cœurs + join device + sonde préfixe, uniform
   ET terrain). D'accord, ou veux-tu d'abord un raffinement CPU ?
3. La règle du `query_mask` (`ActivationId(N)<ActivationId(M)`) est implémentée
   côté sémantique par l'idempotence des unions CPU ; elle ne sera EXERCÉE
   qu'au kernel GPU par lots. Fixture à graver dès maintenant côté CPU, ou à
   la porte GPU ?

## 4. Et la suite du plan

Le garde-fou k=2 par wedges (ta réponse) et l'exigence des partitions exactes
`PointId` pour la porte k=1 sont les prochains chantiers structurels ; le
routeur médian et le catalogue parallèle sont fermés au palier précédent
(`8df7ac8`). L'ordre proposé reste celui de ma
[note 50 k sous la seconde](QUESTIONS_CLAUDE_50K_SOUS_LA_SECONDE_20260810.md) §0.
