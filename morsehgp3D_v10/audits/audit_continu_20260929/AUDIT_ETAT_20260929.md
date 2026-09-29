# Audit continu v10 — état et actions proposées

29 septembre 2026. Code de départ `6206d1d11` ; point d'entrée auditeurs
`ed7be3bc3` relu. Les commits ultérieurs ne sont pas implicitement qualifiés.
`phase=exploration_v10_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u18_input_only`, `mode=audit_v10`, `public_status=not_claimed`.

Moteur inchangé. Aucune session GCP lancée par cet audit. Les fichiers des
autres auditeurs sont préservés. Répondre par un nouveau `REPONSE_CLAUDE_*`.

## Priorité scientifique de l'utilisateur

Passage de FULL aux partitions emboîtées des points, avec fidélité au modèle
de densité et robustesse. Le [rapport mathématique](AUDIT_LAMINARITE_POINTS_20260929.md)
est livré avec contre-vérification indépendante et deux contre-exemples natifs.
Verrou confirmé indépendamment : les couvertures de deux composantes peuvent
se recouvrir. À K=2, sites colinéaires 0,2,4, rayon 1 : composantes de centres
{1} et {3}, couvertures {0,2} et {2,4}. La première couverture choisit un
propriétaire ; ce choix n'est pas une conséquence de la seule connexité.

## À corriger avant une nouvelle campagne coûteuse

1. **Délai d'exécution** : `scale_run.run_json` tue `/usr/bin/time` mais
   laisse son calcul enfant actif. Reproduction courte avec enfant finalement
   arrêté et récolté, normal et `python -O`, dans [timeout/README.md](timeout/README.md).
   Aucun timeout n'a été observé en session 5 : défaut prospectif, pas une
   invalidation rétroactive de ses temps.
2. **Exception du pool** : sortie exceptionnelle de l'appelant avant fermeture
   des utilisateurs du descripteur ; ASan `stack-use-after-scope` reproduit.
   La correction de la course sur le chemin normal reste cohérente.
3. **Export** : `mhgp10_tower --no-points --dump=...` segfaulte sur trois points.
4. **Oracle des verticales** : une image fausse avant l'entrée des points
   survit au juge. La vraie sortie sur la fixture est correcte ; renforcer
   la porte avant de fonder une nouvelle tête sur les verticales.

Les preuves 2–4, ainsi qu'un carré évitable dans la tête sur un arbre en
peigne, sont dans [l'audit pool/tête/verticales](pool_head/AUDIT_POOL_TETE_VERTICAL_20260929.md).
Le peigne n'est pas présenté comme un régime LiDAR mesuré.

[L'audit du catalogue](catalogue/AUDIT_CATALOGUE_J2_J2C_20260929.md) confirme
15 195 enregistrements exacts sur 432 petits appels, niveaux et support minimal
compris. Il reproduit l'admission de fins de fichiers tronquées, et distingue
un paramètre de feuille pathologique du chemin par défaut.

L'autre auditeur indépendant, dans
`../audit_independant_20260929/AUDIT_CONSTATS_INTERMEDIAIRES_20260929.md`
(publication autonome en cours), confirme plusieurs de ces frontières et
ferme le recomptage du lot C :
30 720 couples, aucun manquant, doublon ou refus. Sa reproduction du
validateur acceptant une campagne incomplète évite de dupliquer ce test ici.
Le défaut est prospectif, pas une invalidation du lot C existant.

## Mesures : acquis et corrections de portée

[Recalcul des sessions G4 4 et 5](timeout/AUDIT_ECHELLE.md) : hashes et
exposants publiés concordent ; croissance empirique sous-quadratique sur
les tailles effectivement closes. Pas de preuve de coût constant par boule.

- Les 0,218–0,263 s de `mhgp10_cluster` portent sur **un ordre K5 + tête**,
  pas FULL 1..5 + tête. Les mesures FULL distinctes restent à citer.
- FULL K5 sans attaches : catalogue + tour 0,2042–0,2536 s sur trois trames
  sans sol d'une seule séquence, hors préparation. CPU 48 fils/24 cœurs,
  pas exécution GPU. Ni 100 ms ni plusieurs séquences qualifiés.
- Le grand cas clusters/K10 atteint 303,6 octets/boule de RSS,
  145,26 Go = 135,28 Gio ; corriger unités et extrapolation « 1,4 M LiDAR ».

## Organisation

Suivi après `0bce6cc00` : le développeur a accepté les constats et prépare
des régressions permanentes. Les errata de `2aacfa2e5` corrigent déjà plusieurs
portées ; la livraison des correctifs moteur reste à auditer.

Suivi `695934464` : le README dit désormais « les six K testés » pour le
lot C, conformément au recomptage des 30 720 lignes : K={1,2,3,5,8,10}.
La passation restreint la qualification CUDA au comparateur de produits.
Ces deux corrections documentaires sont closes. Le code de la sonde remplace
les boucles signées par des chaînes modulaires non signées ; le nouveau débit
n'est pas mesuré ici et aucun résultat ancien ne se transfère à cette version.

- [Ancrage et certificat local](ADDENDUM_ANCRAGE_ET_CERTIFICAT_20260929.md) :
  réponse à la règle proposée par le développeur et petit oracle exact livré.
- [Sonde CUDA et grands K](timeout/AUDIT_ADDENDUM_GPU_GRANDK_20260929.md) :
  comparateur positif conservé ; boucles de débit avec débordements signés ;
  campagne grands K complète mais tour exécutée seulement à K10.

- `pool_head/`, `timeout/`, `catalogue/` : rapports et contre-vérifications.
- [Captures archivées](../../receipts/audit_continu_20260929/README.md) :
  sources des petits tests, sorties fermées et hashes, sans doublons ici.
- [Projection laminaire](AUDIT_LAMINARITE_POINTS_20260929.md) : recommandations,
  preuves et limites ; [références statistiques](timeout/AUDIT_CIBLES_STATISTIQUES_20260929.md) :
  les diagnostics MAP/Morse ne sont pas des plafonds d'ARI.

Ce dossier ne doit pas devenir une seconde passation du moteur : il conserve
les objections vérifiables, les preuves et leurs réponses, pas des chronos hérités.
