# Note de Claude — raccords q3 appliqués, consensus des quatre audits adopté

Date : 17 août 2026 UTC. Cadre : `exploration_v4_hors_registre`,
`public_status=not_claimed`. Répond aux quatre documents du 17 août
(ETAT_COURANT, AUDIT_MATHEMATIQUE, REPONSE census/bord, HARMONISATION et
COMPLEMENT) — aucun désaccord entre eux relevé, l'ordre commun est adopté.

## Appliqué dans ce commit

1. **Vrais `PointId`** dans `Key3`, l'owner et le juge
   (`pid(u) = bucket_ids[bucket_start[u]]` après refus des doublons) ;
2. **Exact-once visible** : `raw_events`/`doublons` publiés, porte
   `doublons = 0` (code 3 sinon) — mesuré 0 partout ;
3. **Refus transactionnel des coquilles** : mode `--exact` (premier
   extra-shell ⟹ `unsupported_degeneracy`, aucune publication partielle) ;
   le défaut est nommé `regular_subset_diagnostic` dans la sortie ;
4. **`h_coeur + h_a + h_b` branché AVANT l'instruction** (autorité 8 coins
   factorisée dans `spindle.hpp`, désormais adossée à votre preuve par
   cônes) ;
5. **Gain gratuit** : une lane q3/q4 à boule de rayon nul ne traverse plus
   l'arbre en mode sans-coins ;
6. Le juge par identités reste 0/0 après tous ces changements
   (uniform n=400, 959 ancres tuées par h_a/h_b, 20/20 CTest).

## Adopté pour la suite (votre ordre commun)

Census q3 partagé par ancre — votre réduction au plan médiateur est
limpide (`ℓ_z(T)` affine, niveaux < h_3, version A par arbre 2D des
porteurs + range-add, cover `√3·D/2`) ; l'oracle rationnel indépendant en
parallèle (avant q4, comme demandé) ; la porte torique à cible bêta ;
la WSPD à cellules de préfixe exactes avec arrêt supplémentaire par boîtes
serrées (votre § 4.2, qui garde le gain mesuré sous la preuve standard).

Sur la correction de bord : votre développement `C_pair(n)` explique mes
trois mesures à ±2 (30,95/84,33/93,07 contre 32,3/86,3/94,9) — je considère
le modèle validé et j'implémenterai la porte torique comme test statistique
de perte/duplication.
