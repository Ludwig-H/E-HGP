# Microbanc MES-G1 : saut certifié sans census (levier G-L3)

7 octobre 2026. Mesure hors ligne de la tranche T2-a ([`CONTRAT_TOUR.md`](../../docs/CONTRAT_TOUR.md) § 4.3 et § 12),
**hors produit**, sur les vidages complets de `mhgp12_vidage` ([`../mes_m3_m4_tour/README.md`](../mes_m3_m4_tour/README.md)
§ 3 et § 4).

```text
phase=exploration_v12_hors_registre
backend=cpu_reference (microbanc hors produit, lié à la v11 gelée ac081a06f)
quantification=quantized_u21_input_only
public_status=not_claimed
GCP non utilisé
```

## 1. Question et règle

Pour une partie de descente F dont la v11 fait un **census saturé** (route 2 de `PARTINF` : support local de
`bounded_meb` absent de la table S* → boule, au moins k sites strictement intérieurs), on cherche k sites strictement
intérieurs parmi des **candidats locaux**, testés en exact contre la plus petite boule de F ; s'ils existent, ils
prouvent p ≥ k sans census, et les k plus petits `SiteIdx` d'entre eux font le saut (une k-partie de I : pas valide du
théorème D). Règle d'adoption de `G-L3`, écrite au contrat : au moins la moitié des censuses saturés disparaissent à K5
sur ng00–02, **et** le temps de G à un fil baisse (borne haute de l'IC 95 % du rapport sous 1). `mhgp12_mes_g1` mesure
la première moitié hors ligne, en comptes déterministes ; la seconde moitié se mesure par le quatrième bras
`replique_v12_saut` de `mhgp12_vidage`, que pilote et juge `pilote_g1.py` (§ 7).

## 2. Méthode

**Admission stricte du vidage**, avant tout calcul de proportions (résidu de `CST-0018`, reçu
[`audit_t2_20261007/mesures`](../../receipts/audit_t2_20261007/mesures/README.md)) : la route annoncée par `PARTINF`
n'est jamais crue. Catalogue vérifié (arité de S* dans 2..4, sites de S* et des populations dans le nuage, décalages
cohérents avec p + m) ; chaque partie des ordres joués est recalculée en exact (`bounded_meb` et census de la v11) et
confrontée au catalogue :

- route 1 : boule présente, support local de `bounded_meb(F)` dans la table et désignant cette boule,
  S*(b) ⊆ F ⊆ P_b (inclusions sur le catalogue), sphère de F égale à celle de b (centre et niveau exacts),
  `sstar_in_f` = 1, action intérieure si et seulement si p(b) ≥ k ;
- routes 2 et 3 : support local absent de la table, `sstar_in_f` = 0, census de seuil k du bon genre (saturé avec
  exactement k intérieurs et action intérieure ; complet avec p < k et action trace ou terminale) ; boule annoncée au
  catalogue vérifiée (S*(b) ⊄ F, F ⊆ P_b, même sphère, et même population pour la route 3) ; boule annoncée hors
  catalogue vérifiée (route 2 : census de seuil K saturé, ou complet avec S* global absent de la table et
  p + q ≥ K + 2 ; route 3 : de même sur la coquille complète) ; route 2 : la cible de la v11 est la partie suivante
  de la trace, ou la trace finit par la table de populations.

Toute incohérence est un refus explicite (code 3) avant toute ligne d'ordre. Un ordre choisi hors de 2..K est une
erreur d'usage (code 2) ; un bilan sans partie de route 2 ni de route 3 dans les ordres joués est refusé (code 3).

Puis, pour chaque partie de route 2 (k sites, `SiteIdx` croissants) :

1. plus petite boule exacte de F par `bounded_meb` de la v11 (liée) ;
2. **cible de la v11** par le census de la v11 (`CensusWorkspace`, seuil k) : saturé, il rend exactement les k plus
   petits `SiteIdx` de I (parcours préfixe de l'index, listes croissantes). Contrôle contre le vidage : c'est la partie
   suivante de la trace, ou la trace finit par la table de populations (graine de fin 2) ;
3. ensembles de candidats, triés sans doublon, testés par `SiteIdx` croissant au prédicat exact `num::side` de la v11 :
   **strictement intérieur = côté < 0** ; un site sur la sphère ne compte jamais ; arrêt au k-ième intérieur :
   - `voisins` : F et les K plus proches voisins de chaque site de F (K du catalogue, le site exclu, ordre total
     (distance carrée entière, `SiteIdx`)), calculés une fois pour tous les sites par un arbre k-d exact
     ([`voisins.hpp`](voisins.hpp)) et jugés contre la force brute sur un échantillon (512 sites par défaut) ;
   - `fenetre_8`, `fenetre_16` : les sites de `SiteIdx` dans [i − W, i + W] pour chaque i de F (ordre de Morton) ;
   - `voisins_ou_fenetre_16` : réunion des deux, mesurée en plus (informative) ;
4. partie **certifiée** par un ensemble : au moins k candidats strictement intérieurs ; **cible G1** = les k plus
   petits `SiteIdx` d'entre eux ; comparée à la cible de la v11 ; **naissance directe** = population triée I ∪ U d'une
   boule de `cat.bin` à p + m = k (`LEM-POP`, table refaite par ordre) ;
5. **juge** (hors décision) : chaque site d'une cible est re-testé par la voie `LatticeSphere` de la v11 (celle du
   census) ; cible égale à celle de la v11 ⟺ cible de la v11 incluse dans les candidats ; partie non certifiée ⟹ cible
   de la v11 absente des candidats.

Pour chaque partie de route 3 (census complet) : census de seuil k (complet exigé), p = |I|, q_min par le support
canonique de la coquille entière (`mebcert::canonical_support`), sphères distinctes par (centre exact, rayon carré
exact) **et** par S* global (les deux comptes doivent égaler). **`LEM-HORS-CAT`** n'est jugé que sur les parties dont
la sphère est hors de Cat_K (`PARTINF.ball = 0xFFFFFFFF`) : route 2, p ≥ K − 2 (trivial si k ≥ K − 2, sinon census de
seuil K − 2 saturé exigé) ; route 3, p ≥ K − 2, k ≥ K − 1, p = K − 2 ⟹ q_min = 4, p + q_min ≥ K + 2. Un contre-exemple
est une contradiction mathématique : ligne `contradiction`, coordonnées écrites dans le dossier du vidage (jamais
versé), code 1, arrêt immédiat. Les censuses complets d'une sphère **du catalogue** (S* ⊄ F, hors de l'hypothèse du
lemme) sont publiés à part, par ordre, et recoupés avec le catalogue (p, m, q, S*, et S* ⊄ F).

## 3. Construire et jouer

```bash
cmake -S mes_g1_saut -B <build> -DCMAKE_BUILD_TYPE=Release \
      -DMHGP11_SOURCE=<sources>/morsehgp3D_v11 -DMHGP11_BUILD=<construction Release u21 de la v11>
cmake --build <build> -j 3
ctest --test-dir <build> --output-on-failure          # porte (0), quatre mutants (1), usage (2), juge du pilote (0)
<build>/mhgp12_mes_g1 <vidage complet> [--ordres k1,k2,...] [--echantillon N] > mes_g1_<cas>.jsonl
```

La construction donne aussi `mhgp12_vidage` (même source que `mes_m3_m4_tour`, quatrième bras compris) et le mutant de
son contrôle de forêt, utilisés par le pilote (§ 7).

Le vidage complet est celui de `mhgp12_vidage` (`cat.bin` et `ordre_<k>.bin`, sans effacer les ordres). Un fil ;
comptes déterministes, indépendants de la machine : ils se jouent en local, seuls les temps exigent G4.

## 4. Sorties

Lignes JSON : `entree` (trame, K, sites, boules) ; `voisins` (échantillon jugé contre la force brute, écarts) ; une
ligne `ordre` par k = 2..K (`route1` ; `route2` : parties, hors catalogue, cible de la v11 naissance directe, et par
ensemble : `certifiees`, part, candidats moyen et maximum, tests jusqu'au k-ième intérieur moyen et maximum, cible égale
à celle de la v11, cible naissance directe, cible de la v11 naissance sur les mêmes parties, écarts du juge ; `route3` :
parties, hors catalogue, `census_complets_sphere_au_catalogue_s_etoile_hors_de_f`, sphères distinctes, histogramme
(p, q_min) hors catalogue ; `lem_hors_cat` ; taille de la table de populations) ; `bilan` (sommes, sphères distinctes
tous ordres confondus, code). Aucune coordonnée ni aucun vidage n'est publié : comptes et empreintes seulement.

## 5. Porte et mutants

`mhgp12_mes_g1 --porte` : nuage gravé de 17 sites en trois grappes contiguës en `SiteIdx` (K = 3) : grappe 1, la
sphère de diamètre A1 B1 a trois intérieurs (dont deux à égale distance de A1, départagés par `SiteIdx`) et les voisins
suffisent (k = 2, et k = 3 avec un site de F intérieur) ; grappe 2, quatre sites juste dehors sont plus proches des
extrémités que le second intérieur : les voisins ne suffisent pas (repli sur le census), la fenêtre si ; grappe 3, E3
est **exactement sur la sphère** (côté nul par les deux voies de la v11) et parmi les candidats : rien ne certifie.
Voisins de l'arbre égaux à la force brute sur tous les sites, six listes de voisins gravées. Le mutant
`mhgp12_mes_g1_mutant_cote_nul` (côté nul admis, copie compilée à part) est **tué** par les quatre témoins (code 1) ; sur
ng00 K5 il est aussi tué par le juge (209 951 écarts sur l'ensemble des voisins, 827 294 sur les quatre ensembles,
code 1).

**Témoins d'admission** (vidages écrits en exact dans un dossier temporaire, banc rejoué dans le processus, sortie
capturée) : le témoin de l'auditeur, quatre sites alignés (0,0,0) à (3,0,0), K = 2, F = {0, 3} (boule hors
catalogue, deux intérieurs) : vidage honnête conforme (code 0, une partie de route 2 certifiée par les voisins) ; la
même partie changée de la route 2 en route 1 (`ball` restant `0xFFFFFFFF`) refusée (3) ; une route 1 forgée parmi des
parties honnêtes refusée (3) ; `--ordres 9` et `--ordres 2,9` sur ce catalogue K2 refusés (2) ; un ordre sans partie
refusé (3, bilan vide). Un mutant causal par garde, compilé à part, retire cette seule garde et doit être **tué**
(code 1) parce que son témoin est alors admis : `mhgp12_mes_g1_mutant_sans_garde_route` (la partie forgée parmi des
honnêtes rend 0 ; le témoin d'une seule partie forgée est déjà arrêté par la garde du bilan),
`mhgp12_mes_g1_mutant_sans_garde_ordres` (`--ordres 2,9` rend 0), `mhgp12_mes_g1_mutant_sans_garde_bilan` (l'ordre
vide rend 0). Ligne de fin de la porte :
`{"porte":"mes_g1","mutant_cote_nul":false,"mutant_garde":"aucun","temoins":4,"admission":6,"ecarts":0}`.

## 6. Codes et limites

Codes : 0 conforme ; 1 écart (juge, voisins contre force brute, contradiction) ; 2 usage (dont un ordre choisi hors de
2..K) ; 3 refus (section absente, vidage incohérent avec la v11 ou le catalogue, route annoncée incohérente, bilan
vide, refus arithmétique, exception).

Limites : la mesure suit les chaînes de la **v11** ; quand la cible G1 diffère de celle de la v11, la suite de la chaîne
diffère, et le nombre réel de censuses évités par une descente avec saut se mesure par un bras de résolution, pas ici.
Les tests de côté utilisent `num::side` de la v11 au profil 21 ; le budget mixte de la boule en repère local
([`CONTRAT_TOUR.md`](../../docs/CONTRAT_TOUR.md) § 6) n'est pas exercé.

## 7. Seconde moitié : quatrième bras, pilote et juge

**Quatrième bras.** `mhgp12_vidage … --chrono-resolution R --bras-saut` joue, entrelacé avec les trois bras de la
mesure de résolution et à un fil, le bras `replique_v12_saut` : la même réplique v12 (même fonction), dont le pas
intérieur hors catalogue tente d'abord les candidats `voisins` (mêmes définitions et même ordre qu'au § 2) et ne fait
le census qu'à défaut ; voisins calculés une fois avant les passes, temps publié à part (étage P) ; contrôle de la
forêt contre celle de la v11 à chaque ordre (écart : code 1) ; détail au § 5.6 de
[`../mes_m3_m4_tour/README.md`](../mes_m3_m4_tour/README.md).

**Pilote** `pilote_g1.py` (bibliothèque standard, `python3 -S -O`), étapes `construire portes campagne rapport`
(`tout`) :

- `construire` : ce microbanc (quatre binaires), empreintes des binaires, de `libmhgp11.a` et des sources ;
- `portes` : porte gravée (témoins géométriques et d'admission), mutant « côté nul admis » et un mutant par garde
  d'admission ; quatrième bras sur le premier cas (forêt identique à chaque ordre) et mutant des cibles décalées (tué
  par une forêt différente, jamais par un simple code) ;
- `campagne` : `--processus` processus neufs par cas (5 par défaut), chacun avec `--passes` passes (minimum par bras et
  par ordre) ; journaux sous `<sortie>/<cas>/campagnes/<campagne>/p<i>/`, identifiant `c<date>_<hasard>`, jamais
  écrasés ; vidages hachés (mêmes entrées d'une prise à l'autre) puis effacés ;
- `rapport` : juge et tableaux (`rapport_g1.json`, `tableaux_g1.md`).

**Règle `REGLE_G1`** (écrite avant la mesure) : par processus, somme sur les ordres 2..K du minimum de R passes de
`replique_v12_saut`, rapportée à la même somme pour `replique_v12` ; moyenne géométrique des rapports par processus et
IC 95 % par bootstrap sur les processus (10 000 tirages, graine fixe), comme `REGLE_M3`. **Adopté** si, sur chacun de
ng00, ng01 et ng02 à K5, au moins 50 % des censuses saturés disparaissent (ordres 2..K), la forêt est identique à celle
de la v11 à chaque ordre de chaque prise et la borne haute de l'IC est sous 1 ; **rejeté** si une borne haute est au
moins 1, si la première moitié manque ou si une forêt diffère ; **refusé** si une preuve manque (construction, portes
conformes et mutants tués par les mêmes binaires, binaires hachés avant et après chaque exécution et égaux à la
construction, journaux gardés et inchangés, au moins cinq prises valides par cas, mêmes vidages et mêmes comptes d'une
prise à l'autre, sources inchangées pendant l'invocation). Le verdict est dans le rapport ; le code du pilote dit les
preuves (0 conforme, 1 écart, 2 usage, 3 refus).

**Porte du juge** `test_pilote_g1.py` (CTest `mhgp12_mes_g1_pilote_juge`, sorties synthétiques) : validation d'une
prise (conforme, ordre manquant, temps nul, forêt différente avec code 1 ou 0, contrôle non reproduit) et onze verdicts
(adopté ; rejeté par la borne, la première moitié ou la forêt ; refusé pour quatre prises, un binaire différent, un
journal modifié, un mutant de forêt absent, un mutant de garde absent, des comptes non déterministes, une construction
absente).
