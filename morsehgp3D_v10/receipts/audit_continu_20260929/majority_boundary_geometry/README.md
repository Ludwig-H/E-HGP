# Majority → frontière : contre-épreuve géométrique close

29 septembre 2026. Huit fixtures gravées de quatre sites, K2, grille entière
u18, aucune graine de benchmark. Quatre sont collinéaires, quatre de dimension
affine 3. `public_status=not_claimed`. GCP non utilisé ; moteur inchangé.

## Résultat

32 exports natifs : huit fixtures × W1/W2 × interprétation normal/−O.
Code 0 pour les deux runners, sorties exactes W1/W2 identiques, résultats
Fraction normal/−O identiques. 2 496 contrôles de paires entre coupes.

La tête d'audit majoritaire (θ=1/2, dénominateur fixe, témoins `p+q_min≤K`)
reste laminaire dans les deux modes. **Masses uniformes : zéro groupe sur
les deux récupéré avant première fusion FULL, pour chacune des huit
fixtures. Masses 1/β : les deux récupérés dans les huit fixtures.**
Un témoin-pont très tardif reçoit autant de poids qu'un témoin local en
mode uniforme ; l'observation frontière n'atteint jamais la majorité
précoce. La géométrie indépendante du catalogue et Γ2 confirme les deux
groupes de couverture au début. Ce résultat ne prouve ni une optimalité
du choix 1/β ni une victoire sur HDBSCAN.

## Sources et exécution

- `check_geometry.py` : tête d'audit exacte et contrôles, n'écrit que dans
  un **nouveau** répertoire de capture, refuse une sonde non épinglée.
- `fixtures/` : huit entrées texte, sans aucun retour retiré.
- `source_snapshot/hgp10_ref.py` : juge indépendant en Fraction, fixé
  avant les commandes ; non modifié par l'expérience.
- `source_snapshot/native_exporter.cpp` et `native_build_observed.json` :
  provenance déjà publiée de l'exporteur lié à l'archive `6206d1d11`.
  La sonde native existante est `/tmp/mhgp10-cover-native-r2.XfedhHF8/probe`,
  SHA256 `1c2ce0d73c5871783f2fc3e04a3940a53250af7a0d60a1689ec8a4c3a9787a85`.
  Aucune nouvelle compilation ; les reçus ne contiennent pas le binaire.
- `normal/receipt.json`, `optimized/receipt.json` : commandes natives,
  codes exacts, stdout/stderr complets par commande, hashes des sources et
  binaire avant/après, traces de toutes les coupes et diagnostics.

Commandes closes, depuis le worktree `build/v9-open-worktree` :

```text
python3 -B morsehgp3D_v10/receipts/audit_continu_20260929/majority_boundary_geometry/check_geometry.py /tmp/mhgp10-cover-native-r2.XfedhHF8/probe morsehgp3D_v10/receipts/audit_continu_20260929/majority_boundary_geometry/source_snapshot/hgp10_ref.py morsehgp3D_v10/receipts/audit_continu_20260929/majority_boundary_geometry/normal
python3 -B -O morsehgp3D_v10/receipts/audit_continu_20260929/majority_boundary_geometry/check_geometry.py /tmp/mhgp10-cover-native-r2.XfedhHF8/probe morsehgp3D_v10/receipts/audit_continu_20260929/majority_boundary_geometry/source_snapshot/hgp10_ref.py morsehgp3D_v10/receipts/audit_continu_20260929/majority_boundary_geometry/optimized
```

Pour rejouer, remplacer uniquement le chemin de sortie par un répertoire
neuf et vérifier le hash de la sonde. La géométrie et les résultats stockés
peuvent être relus sans le binaire ; une nouvelle exécution native demande
ce binaire ou une recompilation explicitement requalifiée.

## Portée

Les partitions sont calculées en Python sur les exports : aucune tête
majoritaire dans le moteur, aucun EOM, aucune ARI, aucun test de qualité
statistique, aucun chrono FULL/G4. Le choix de poids d'incidence `1/β`
est distinct de l'exposant EOM. Le lot ne qualifie pas l'intégration des
correctifs contemporains du pool, des CLI, de SiteTree ou des juges.
Interprétation et consigne : [section 9 de la note courante](../../../audits/audit_continu_20260929/AUDIT_LAMINARITE_POINTS_20260929.md).
