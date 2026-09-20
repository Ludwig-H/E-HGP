# Reprise v8 du20 septembre2026

Source de départ `3e94c868`, sur `main`, `public_status=not_claimed`.
GCP non utilisé. Ce dossier distingue l'audit des preuves anciennes de
la qualification des nouveaux octets.

- [Audit des preuves19–21](AUDIT_PREUVES.md) : lectures et hashes, pas
  réexécution des qualifications historiques.
- [Préflights](PREFLIGHT.md) : incidents de configuration et observations
  avant gel, non substitués aux captures closes.
- [Famille q4](../q4_family_20260920/README.md) : nouvelle gate Release et
  Clang ASan/UBSan, sondes32 et mesures8k/16k/32k,135 sources épinglées.
- [Régression Release](regression_0rxnfnth/MANIFEST.json) : commande unique
  CTest,82/82 PASS, exécutables et cache épinglés ;
  [fermeture](regression_0rxnfnth/COMPLETION.json) et
  [résultats XML](regression_0rxnfnth/result.xml) conservés.
- [Lectures closes](readers_lwn6nyk0/COMPLETION.json) : dix commandes,
  trois captures q4 et régression relues avec `--check-live`, selftest du
  lecteur, sorties normal/−O identiques ;135 sources inchangées.

`record_regression.py` ne construit rien et refuse un cache autre que
Release sans sanitizer. Il conserve les sorties même sur interruption et
vérifie le XML, les82 noms et les empreintes. Son `read --check-live`
vérifie également les fichiers actuels. Le script `close_reads.py` ferme
séparément les lectures/analyses normal et `-O`, avec les captures nommées
explicitement ; il n'exécute aucun benchmark.

Ni la régression ni les sondes d'une seule famille ne qualifient un
générateur q4 complet, FULL, le GPU ou les contrats50k et massif.
