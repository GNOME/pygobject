DIAS = $(wildcard images/*.dia)
DIA_SVGS = $(patsubst %.dia,%.svg,$(DIAS))

all: _build

images/%.svg: images/%.dia
	dia $< --export=$@ --filter=dia-svg

_build: Makefile *.rst devguide/*.rst guide/*/*.rst guide/*.rst conf.py images/*.png images/*.svg $(DIA_SVGS) ../README.rst ../NEWS
	python3 -m sphinx -b html . _build

linkcheck:
	python3 -m sphinx -b linkcheck -n . _build

clean:
	rm -R _build
