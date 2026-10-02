# Test-only witness: a normal exit must never qualify as an abnormal stop.
print("run_expect_verdict arret_anormal")
raise SystemExit(3)
