# Suivis clos condensés au pin 6a8b6f9a8

8 octobre 2026, Codex. Hygiène seulement : les dix-huit suivis clos les plus longs sont condensés. Les neuf premières colonnes de chaque ligne (identifiant, énoncé, date, rôle, classe, gravité, pin, témoin, état) restent **octet pour octet identiques**. Le pin de clôture, la preuve principale et sa portée restent visibles au registre ; les développements ci-dessous conservent chaque lien et le texte antérieur, avec seulement les chemins relatifs adaptés.

Source : `morsehgp3D_v12/audits/CONSTATS.md` à **6a8b6f9a8c47a40dc2beb4b397652f41c2e4db7d**. Le mapping lie chaque ligne et ses hashes à son ancre. Aucun constat actif, aucune note d'auditeur ni aucun produit modifié par la proposition. Les anciens reçus restent sur place : leurs liens sont conservés, leurs contenus ne sont pas recopiés. Aucun changement d'état ni nouvelle qualification.

**Gain : 3377 octets ; canal 65461 → 62084 octets** au pin. Un seul nouveau reçu. Le patch est à réconcilier si main avance ; il ne doit pas écraser les ajouts suivants.

```sh
python check.py --repo /chemin/depot
python -O check.py --repo /chemin/depot
# application facultative par le responsable du canal :
git apply --unidiff-zero proposition.patch
```

Le lecteur vérifie les liens déplacés, les hashes, les colonnes intactes et l'application isolée. Il ne rejoue aucune preuve de clôture. Avant un commit du canal, le responsable exécute aussi `tools/check_constats.py` dans la version réconciliée.

## CST-0004

Source Git 6a8b6f9a8, ligne 12 ; date, rôle, pin et témoin conservés au registre.

`tools/g4_matrix.py` lance `ctest --output-on-failure` (option désormais réservée au script) en plus du rapport JUnit et de `LastTest.log` ; deux contrôles ajoutés à `mhgp12_support_g4_matrix` (37 → 39), 7 octobre ; [contre-audit `274592a30`](../../audit_t2_20261007/corrections/README.md) : 39 contrôles normal/−O, commandes externes simulées

## CST-0007

Source Git 6a8b6f9a8, ligne 15 ; date, rôle, pin et témoin conservés au registre.

`7b7d025b3` ; [clôture et pins](../../audit_clotures_20261007/README.md) : comptage physique, admission +1/8, éviction concurrente et poison contre-éprouvés ; [porte causale](../../audit_reponses_20261007/cache/README.md), 12 contrôles, mutant de relecture tué ; ancien fil conservé au reçu

## CST-0009

Source Git 6a8b6f9a8, ligne 17 ; date, rôle, pin et témoin conservés au registre.

[Repli livré](../../audit_reponses_20261008/repli_unresolved_0009/README.md), sources `9c5656029` égales au snapshot I : deux passes Pool par lots, espaces privés, tampons comptés ; portes Pool et GPU (neuf témoins, 217 contrôles) passées. Clôture de la boucle sérielle obligatoire, sans gain isolé ni préadmission globale ; groupes de profondeur encore successifs. `0008` reste ouvert.

## CST-0024

Source Git 6a8b6f9a8, ligne 32 ; date, rôle, pin et témoin conservés au registre.

Course v10 figée `c764e121a`, corrigée par `8e3b76245` ; porte limitée à un fil. [Contre-audit `274592a30`](../../audit_t2_20261007/corrections/README.md) : 34 sources, TSan 2/0/1 courses contre cinq traces mono sans course ; stress trois instances, avant 2/18 défauts (dump/expiration), après 0/45. Préfixe/continuation distingués ; aucun stress rejoué. [Historique](../../audit_canal_20261008/suivis/README.md#cst-0024).

## CST-0105

Source Git 6a8b6f9a8, ligne 37 ; date, rôle, pin et témoin conservés au registre.

Énoncé corrigé (`a0e31abfe`) ; [livraison u21 `7398aed7d`](../../audit_reponses_20261008/tmvr_livraison/README.md) : précondition dans le header, refus `tower_query_domain` dans `component_at`, porte `verticales` (coupe hors domaine puis deux coupes valides) et `requetes` passées. Clôture du domaine et du rangement ; validateur d’historiques altérés distinct (`0240`).

## CST-0107

Source Git 6a8b6f9a8, ligne 39 ; date, rôle, pin et témoin conservés au registre.

Énoncé corrigé (`a0e31abfe`) ; [livraison u21 `7398aed7d`](../../audit_reponses_20261008/tmvr_livraison/README.md) identique à repo6 : permutation canonique `{1,0,3,2}` contrôlée par `verticales`, mutant `naissances_par_cle` tué. MES-M0 neuf cas à l’octet avec graines v11 ; chaîne CPU produit conforme sémantiquement, W1/W8 identiques. Profils larges et temps FULL G4 exclus.

## CST-0115

Source Git 6a8b6f9a8, ligne 47 ; date, rôle, pin et témoin conservés au registre.

`95247cf4b` corrige la classe de `reference/tests.cmake` et distingue 406 portes du socle / 412 après intégration ; [contre-lecture Codex au pin `c3de9d73d`](../../audit_u32_20261007/qualification/README.md) : 209 empreintes v11 et 139 copies/renommages conformes à cette correction, fichier de portes inchangé depuis ; le retard ultérieur des autres ports est distinct (`CST-0226`)

## CST-0202

Source Git 6a8b6f9a8, ligne 49 ; date, rôle, pin et témoin conservés au registre.

clé complète choisie (`0dbf69347`), u32 admis par `6a38f7e4b` ; [socle CPU u32 `c3de9d73d`](../../audit_u32_20261007/index/README.md) : 128 clés (96 bits contrôlés), six préparations Cloud, positions hautes distinctes et doublons intercalés conservés avec tous les PointId ; clôture de l’identité Morton/Cloud, refus D8 au point d’entrée distinct

## CST-0212

Source Git 6a8b6f9a8, ligne 57 ; date, rôle, pin et témoin conservés au registre.

[M4 `d8407e374`](../../audit_socle_microbancs_20261007/tour/README.md) : contrôles avant allocation/conversion, offsets u64, refus aux bornes ; clôture du microbanc. [Produit u21 `7398aed7d`](../../audit_reponses_20261008/tmvr_livraison/README.md) : codage naissance/cellule distinct, domaine 31 bits/sentinelle et porte `domaine` aux bornes passés, refus avant allocation. [Historique](../../audit_canal_20261008/suivis/README.md#cst-0212).

## CST-0213

Source Git 6a8b6f9a8, ligne 58 ; date, rôle, pin et témoin conservés au registre.

`320db4a12`, [pilote contre-lu au pin `07ee13ef6`](../../audit_cd_corrections_20261007/m34/README.md) : deux campagnes de cinq PID distincts, dix journaux préservés et identifiant réutilisé refusé ; [session D](../../audit_cd_corrections_20261007/campagnes/README.md) : cinq prises réelles par cas, seuil M3 K10 vérifié ; B reste historique

## CST-0216

Source Git 6a8b6f9a8, ligne 61 ; date, rôle, pin et témoin conservés au registre.

`1b40c0411`, [contre-audit `1f7642e10`](../../audit_juges_emst_20261007/donnees/README.md) : même pilote auparavant refusé, maintenant deux manifestes synthétiques vérifiés depuis un autre répertoire ; épingle des outils et refus des entrées absentes contrôlés ; clôture du raccord, sans préparation réelle rejouée

## CST-0218

Source Git 6a8b6f9a8, ligne 63 ; date, rôle, pin et témoin conservés au registre.

`1b40c0411` corrige l’outil (32 sélections/7 nuages) ; [contre-lecture `f601b36ac`](../../audit_reprise_20261007/donnees/README.md) : régénération des 69 découpes/24 scènes, manifestes/paquets/tables tracée, 8 manifestes relus, raccord 9 découpes/11 prises E. Clôture outil/régénération ; aucun rehash de payload ni recalcul géométrique réel indépendant. [Historique](../../audit_canal_20261008/suivis/README.md#cst-0218).

## CST-0220

Source Git 6a8b6f9a8, ligne 65 ; date, rôle, pin et témoin conservés au registre.

`1b40c0411`, [contre-audit `1f7642e10`](../../audit_juges_emst_20261007/donnees/README.md) : lien dur causal avant/après, refus avant éviction et trois cas indépendants plancher/plafond/protection ; liens et octets préservés ; clôture du défaut observé, espace physique libéré non certifié

## CST-0222

Source Git 6a8b6f9a8, ligne 67 ; date, rôle, pin et témoin conservés au registre.

`1b40c0411`, [contre-audit `1f7642e10`](../../audit_juges_emst_20261007/m5/README.md) : scan/Emit exacts sur 520 enregistrements bornés ; six totaux injectés dans le vrai Driver vérifient les refus avant réservation des tampons et Scatter/Emit ; aucun grand nuage, voie CUDA et budget Session non qualifiés

## CST-0223

Source Git 6a8b6f9a8, ligne 68 ; date, rôle, pin et témoin conservés au registre.

`1b40c0411`, [contre-audit `1f7642e10`](../../audit_juges_emst_20261007/m5/README.md) : quatre anciennes admissions deviennent des refus, 18 résultats historiques et 21 cas livrés conformes ; bornes G1, somme u64, enveloppes et terminaison contrôlées ; format strict ne signifie pas recalcul autonome de G1

## CST-0228

Source Git 6a8b6f9a8, ligne 73 ; date, rôle, pin et témoin conservés au registre.

`76adb8fa9`, [contre-lecture du contrat](../../audit_juges_emst_20261007/t2_corrections/README.md) : compteurs d’objet, de travail par politique/ordre fixés et physiques séparés ; travail hors empreinte intrinsèque, concordance W1/W48 toujours exigée ; clôture de l’énoncé, natif non qualifié

## CST-0232

Source Git 6a8b6f9a8, ligne 75 ; date, rôle, pin et témoin conservés au registre.

`98ca07556`, [contre-audit `f601b36ac`](../../audit_reprise_20261007/emst/README.md) : même FULL 586 octets aux IDs 17/17 refusé avec et sans référence ; 107 appels indépendants par mode, 141 contrôles officiels, six permutations et 24 nuages géométriques inchangés ; clôture identité/ordre un, grands vidages et ordres supérieurs non contre-qualifiés

## CST-0238

Source Git 6a8b6f9a8, ligne 81 ; date, rôle, pin et témoin conservés au registre.

Livraison `28cf75cd1` : cinq cohortes + cinq cas normal/−O. Clôture séparation/cohorte seulement ; limite du coût fixe H conservée. [MES-C v12](../../audit_reponses_20261008/mes_c_statistique/README.md) : pentes descriptives, pas de borne par nuage ; régime chaud/frontière distincts de MES-P. [Preuves et historique](../../audit_canal_20261008/suivis_9c9c/README.md#cst-0238).
