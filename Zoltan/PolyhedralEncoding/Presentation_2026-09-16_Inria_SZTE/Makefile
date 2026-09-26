SHELL := /bin/bash
JOB := HGP_LiDAR_Inria_SZTE_2026-09-16

.PHONY: all assets clean
all: assets
	latexmk -lualatex -interaction=nonstopmode -halt-on-error -jobname=$(JOB) main.tex

assets:
	python3 prepare_assets.py

clean:
	rm -f $(JOB).aux $(JOB).log $(JOB).nav $(JOB).out $(JOB).snm $(JOB).toc $(JOB).fls $(JOB).fdb_latexmk $(JOB).synctex.gz
