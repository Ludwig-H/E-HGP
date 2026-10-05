# Apports des auditeurs du 5 octobre 2026 (commit `9cbf805c6`, 06 h 17 UTC), à intégrer

Lus à 06 h 20 UTC (`date -u`). Ils répondent à la note § E (`9290cf3bf`). Sources sur `origin/main` :
- `morsehgp3D_v11/audits/AUDIT_REPONSES_AUX_VERROUS_MOTEUR_20261002.md` (note mathématique) ;
- `morsehgp3D_v11/audits/AUDIT_CONTRATS_NUMERIQUES_ET_CAPACITE_20261002.md`, section « Sorties paramétrées : avant
  intégration et G4 » (note moteur) ;
- reçu `morsehgp3D_v11/receipts/audit_supports_contract_20261005/` (capsules `tower`, `qb`, `evidence`, `api`).

## Verdict

- Contrat S0/S1 (`5adf6a59f`) : relecture **favorable** des lemmes P et W, des deux lectures de E5 et du témoin D2.
  Les 13 mutants S1 sont tués avec leur cause nommée. Aucun défaut mathématique nouveau.
- S3 (capture WIP) : la garde `published_traces(end−begin)` avant le cast `u32`, avec refus `tower_capacity`, est
  présente ; les fixtures D2 et E5 aussi. Le plafond 24 de `supports` reste distinct de FULL.
- S6a : la garde `p ≤ 11` avant `p+q` est présente.

## À faire pour S5 (qualification L2)

1. **SIGXFSZ.** Le CLI ignore SIGPIPE mais pas SIGXFSZ : sous `RLIMIT_FSIZE`, le signal tue l'appel et laisse un
   `D.pending` orphelin (constat C2 du contre-lecteur S5). Ignorer aussi SIGXFSZ pour que `EFBIG` passe par le refus
   contrôlé. Porte native : code 2, `output_unwritable`, ni `D` ni `D.pending`. Plus son mutant.
2. **`published_complete` et empreinte du manifeste fermé.** Le contrat les adopte ; la capture S5/io ne les a pas.
   Couvrir trois doubles échecs, en gardant le code de refus et l'empreinte du dossier complet :
   - `fsync` du parent en échec, retour arrière impossible ;
   - écriture de la sortie standard en échec, retrait impossible ;
   - `Session.close` en échec, retrait impossible.
3. **Juger le champ publié `tree_k_sha256`.** Le comparer à une sérialisation **indépendante** de la signature V2
   (lecteur en bibliothèque standard), à plusieurs K distincts. L'égalité entre appels ou le test d'une fonction
   isolée ne tuent pas un manifeste constant ou calculé à l'ordre 1.
4. **Portes causales** des gardes de la sortie standard (stat et inode), des liens symboliques et d'`O_RDONLY`,
   correctes par lecture.

## Matrice G4 (avis du moteur)

- **L1** qualifie S3 et S6 intégrés, sur une même source figée. `io`, `api`, `cli` et leurs fautes relèvent de la
  qualification **L2**, quand S5 et S7 sont intégrés.
- GCC Release u21 et u24 séparés ; ASan/UBSan et TSan selon les profils réellement joués ; mutants des modules
  modifiés ; identité `build_order` = `build_full` ; plateau E1 = E2 ; déterminisme W1, W4, W48.
- Inclure : coquille de 24 sites admise, coquille de 25 sites refusée pour l'appel `supports` entier, q4 à K1
  malgré zéro coface.
- S6a juge les primitives. Avec l'assemblage (S6b), ajouter budget et refus, et `count`/`fill` parallèles.
- u18 reste explicite s'il est livré, sans transfert de qualification.
- Échelle : 8k, 16k, 32k et LiDAR K5 restent des portes d'échelle, avec l'oracle exact sur les petits cas.
- **Budget de temps à fixer avant la session.** La matrice existante a un budget global de 1 380 s et des
  configurations à 1 200 et 1 300 s, sans réserve prouvée pour les jumelles Python `-O`, les mutants et les
  sanitizers. Séparer un lot `long` plutôt que laisser un délai censurer des portes.
- Mesure L2 : FULL et arbre K seul appariés, même masque, même entrée, même K ; étages `output` et `write` publiés
  à part.
- Une seule session gardée, arrêt ciblé certifié, reçu rejugé en mode normal et sous `-O`.

## Témoins de coquille (réponse à la question du § E)

- **Coquille mixte au plafond** : la sphère entière $x^2+y^2+z^2=5$ (24 sites), translatée de $(2,2,2)$, valide dans
  les trois profils. $\mathcal{Q}_b$ : 12 q2, 24 q3, 792 q4, soit 828 supports. $N_2=12$, $N_3=288$, $N_4=3906$.
  Avec $p=0$ :

  | K | `kparties_reliees` | `strict_traces` | `cofaces` par boule |
  | ---: | ---: | ---: | ---: |
  | 1 | 24 | 24 | 12 |
  | 2 | 276 | 264 | 288 |
  | 3 | 2024 | 1736 | 3906 |

  La somme des `cofaces` par support à K3 vaut 4 068. Les q4 restent publiés malgré zéro coface à K1.
- **Refus** : 25 des 30 sites de $x^2+y^2+z^2=9$, en **gardant les six points axiaux**. Refus
  `support_shell_capacity` de l'appel entier. Le plafond ne s'applique pas à FULL.
- **Portée.** La boule canonique de ce témoin vient d'un diamètre : il exerce les q3 et q4 sur une présentation q2,
  et ne remplace pas les portes numériques des centres de présentation q3 et q4.
- **Oracle `long` recommandé** : primitives $\mathcal{Q}_b$ et $N_j$ pour $j\leq 4$, dénombrées par combinaisons, sans
  les $2^{24}$ masques, jusqu'à K3. Remplacer le seul calcul de $N_j$ dans l'oracle S1 complet ne suffit pas :
  `_minimal_nonseparable` prolonge aussi les sous-parties séparables jusqu'à $m$. Lui donner un budget avec refus
  explicite ; aucune censure silencieuse, et aucune qualification de tout S1 à 24 sites par ce témoin.
