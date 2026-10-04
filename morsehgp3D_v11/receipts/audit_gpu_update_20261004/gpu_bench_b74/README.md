# Non-vacuité du banc GPU au pin b74f9ea3a

Contre-exemples encore valables sur la source publiée exacte
`b74f9ea3a0b986f659d388b8140067b0b35be0c2` : reps0 et warm_passes1 sont
acceptés, ainsi qu'un flux simulé sans lignes des trois passes demandées.
Dump et ledger finaux sont identiques dans chaque scénario ; verdict conforme.
Le témoin de contrôle a une prise froide et trois passes complètes.

**75 gardes**, Python standard normal/optimisé identiques. Sources hachées,
AST du banc exécuté avec processus entièrement simulés. La fonction Williams
est extraite du blob ab_g4 adjacent, sans exécuter son main ; le sys.path.insert
du banc est omis dans le harness. Les imports/helpers restants sont conservés.
Aucun programme natif, download, CUDA, profiler ou GCP. Cette capsule ne
qualifie pas la nouvelle écriture scratch du produit b74.

Pour revendiquer froid et chaud, demander au moins une prise froide et deux
passes, contrôler exactement les numéros1..P et le succès. Une portée partielle
reste possible avec un verdict explicite. L'absence de lignes est un défaut
simulé du producteur pour tester le lecteur ; aucun tel défaut natif n'est
établi. Le producteur sérialise uniquement la dernière passe : l'identité des
passes intermédiaires reste hors de la preuve fournie par ce hash.

Rejeu : `python3 -B -S check.py` et `python3 -B -O -S check.py` ;
`sha256sum -c SHA256SUMS`. Aucun test native/G4 n'est joué.
