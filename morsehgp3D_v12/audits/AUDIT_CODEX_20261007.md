# Audit Codex — état courant v12

8 octobre 2026. Mesures M : `957e9784f` ; défaut cache livré `72f622a55` ; terminaison corrigée `e78904c49`.
Cadre : `exploration_v12_hors_registre`, `cpu_reference ; cuda_g4 pour le catalogue`,
`full_pi0`, `quantized_u21_input_only`, `not_claimed`. Autorité : [registre](CONSTATS.md).

**Derniers temps LiDAR : FULL M admise, contrat 100 ms non tenu sur les 37 trames.**
[Provenance et arrêt certifiés](../receipts/audit_reponses_20261008/session_m_provenance/README.md) :
356 sources et 7 pilotes/fichiers ciblés égaux au Git957, cinq commandes closes à code0, 722 portes socle.
[Admission FULL](../receipts/audit_reponses_20261008/session_m_admission/README.md) : 38 processus, 610 passes,
392 chaudes. Les lecteurs livré et renforcé rendent les mêmes objets/statistiques. u21/W48, trames sans sol
ng00/01/02 : 39 885 / 35 551 / 45 845 sites. Médianes des médianes chaudes par processus, en ms :

| FULL | ng00 | ng01 | ng02 |
| --- | ---: | ---: | ---: |
| GPU K5, cache désactivé | **94,66** | **78,26** | **94,76** |
| GPU K5, cache 8 Gio, bras apparié | **87,46** | **71,97** | **88,39** |
| CPU K5, cache désactivé | **381,80** | **323,10** | **385,94** |
| GPU K10, cache désactivé | **593,26** | **442,82** | **504,79** |

GPU K5 sans cache : maximum des médianes processus **95,99 ms**, sous-contrat des trois trames tenu selon
la règle préétablie ; trois des 135 prises chaudes dépassent néanmoins 100 ms (maximum brut101,61).
**37 trames, six séquences, sans cache : médiane160,64 ms, pire médiane par trame319,78 ms** ;
maximum contractuel des médianes processus **358,86 ms** (ici aussi maximum des 185 chaudes).
25/37 médianes par trame et 127/185 prises dépassent100 ms. Le contrat global reste non tenu.
FULL couvre Cloud/index→T/M/V/R ; lecture, masque, validation, FUL1 et libération hors mur.
G recouvert est une fenêtre, TMVR la queue après G : ne pas sommer les médianes de postes.

**[Cache adopté dans le banc apparié](../receipts/audit_reponses_20261008/session_m_apparie/README.md)** :
140 journaux/1 816 passes, A/A conforme ; rapports géométriques0,929/0,924/0,918, bornes hautes IC95<1.
Séquentiel rejeté :1,515/1,465/1,529. Sur37 trames, deux secondes visites par trame, cache :
**152,62 ms** de médiane et **305,89 ms** de pire médiane par trame ; bloc informatif, hors règle d'adoption.
RSS maximal des deux processus environ **3,81 Gio**, contre2,56–2,63 sans cache ; budget actif partagé excluant
les blocs inactifs, RSS et capacité GPU distingués. Aucun hash ELF final archivé par les pilotes ; aucun indice
supplémentaire de substitution. Environnement « après » apparié antérieur aux huit Sessions informatives.
Le [défaut cache72](../receipts/audit_reponses_20261008/cache_defaut_raccord/README.md) vaut aussi sur CPU/K10 :
leurs temps avec cache restent **non mesurés**. Future ablation : ref/A-A explicitement `--cache=0`.
M précède le correctif0241 et les corrections de lecteurs proposées. Aucun transfert aux petits nuages/massifs.

**Priorité CPU : [catalogue et coût total](../receipts/audit_reponses_20261008/cpu_feuilles_finition/README.md).**
C vaut318,77/272,70/320,92 ms, soit83–84 % du mur par passe. La finition est déjà parallèle ; même gratuite,
le FULL conditionnel resterait321,28/268,14/320,04 ms. Les paires uniques et l'arrêt du census CPU sont intégrés.
Feuille16 contre24 a déjà été mesurée dans I : C légèrement plus lent, parcours accru malgré moins de travail
aux feuilles. Priorité : ventiler levels/sort/assemble/table avec les diagnostics existants, puis mesurer FULL.
Comparaison historique v11 non appariée : GPU plus rapide, CPU toujours plus lent ; aucune causalité déduite.

**CST-0241 : [correctif livré](../receipts/audit_reponses_20261008/a_terminaison_raccord/README.md).**
Postimage exacte `last = fetch_sub(...) == 1` ; cinq fonctions du modèle portées fidèlement.
Python normal/−O passé ; mutant refusé par contrôle textuel, **pas par entrelacement natif**.
[Preuve pour N et protocole natif déterministe](../receipts/audit_reponses_20261008/a_terminaison_porte/README.md)
transmis ; porte native ouverte. Aucun incident natif observé, aucun lien établi avec MES-M0.

**[D6 M admise](../receipts/audit_reponses_20261008/session_m_d6/README.md)** : 126 journaux/630 passes,
504 chaudes. Sur mêmes coordonnées, C u24/u32 +0,8–2,5 % ; G jusqu'à+13,6 %. C et G CPU séparés, pas FULL/GPU :
seuil produit<3 % non qualifié. ×2048 triple presque C et change les classes de feuilles ; coordonnées<2²⁹,
pas tout le domaine u32 ni une précision physique nouvelle. Les corps exportés sont seulement déclarés, puis effacés.

**Lecteurs : raccord livré85db49890, contrelecture en cours.** [Triple correctif apparié](../receipts/audit_reponses_20261008/apparie_composition_triple/README.md) :
identité/résumés, cohorte/minima et fermeture ELF ; les contre-exemples sont refusés, positif simulé conservé.
[Gardes LF recouvert](../receipts/audit_reponses_20261008/lf_recouvert_gardes/README.md) : neuf horloges fausses admises
par le lecteur mesuré957, quatre mutants causaux ; vraies passes A et M compatibles.
[Fixtures corrigées](../receipts/audit_reponses_20261008/pilotes_fixtures_recouvert/README.md),
[bascule des commandes A](../receipts/audit_reponses_20261008/t2d_a_bascule_cloture/README.md)
et [présentation MES-FULL](../receipts/audit_reponses_20261008/pilotes_bascule/README.md).
Le nouveau juge exige le hash ELF final absent de M ; les prises sont relues sans inventer cette fermeture.

**Autres régimes : dernières campagnes antérieures à M.**
[Petits nuages C2](../receipts/audit_reponses_20261008/session_c2_admission/README.md),147 nuages, deux chaudes :
K5 CPU/GPU W48 **28,95/10,15 ms**, K10 **53,01/24,58 ms** ; C1–C3 non tenus, huit refus `wide_leaf`.
[Massifs L1r](../receipts/audit_reponses_20261008/session_l1r_admission/README.md) : Boreas sans sol1,51M,
GPU **10,025s** / CPU **22,344s**, une chaude ; dix succès/huit refus mémoire.
[L2 CPU froide](../receipts/audit_reponses_20261008/session_l2_admission/README.md) : Paris9,11M **112,866s**,
14,55M **165,789s** ; ETH3D `wide_leaf`, Lyon `memory_budget`. Aucun plafond universel en sites.
[Extension de feuille](../receipts/audit_reponses_20261008/feuille_large_proposition/README.md) et
[raffinement certifié des centres](../receipts/audit_reponses_20261008/feuille_large_raffinement/README.md) proposés ;
égalités massives persistantes, aucune borne globale ni accélération acquise.

Historique : [A](../receipts/audit_reponses_20261008/session_t2da_admission/README.md),
[diagnostic des fins A](../receipts/audit_reponses_20261008/session_a_diagnostic/README.md),
[B sur902](../receipts/audit_reponses_20261008/session_b_admission/README.md),
[census retenu957](../receipts/audit_reponses_20261008/b_census_raccord/README.md). M remplace leurs temps pour
son régime précis, sans réattribuer les gains. Le détail des constats reste dans le registre et les reçus.
Audit sources/Python uniquement ; aucun moteur ni GCP lancé par l'auditeur.
