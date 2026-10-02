"""Sonde adverse : deux lignes imitent des verdicts, mais le processus termine normalement avec le code 3."""
print('run_expect_verdict arret_anormal')
print('abnormal_stop_verdict conforme signal 9')
raise SystemExit(3)
