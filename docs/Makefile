DIAS = $(wildcard images/*.dia)
DIA_SVGS = $(patsubst %.dia,%.svg,$(DIAS))

all: _build

images/%.svg: images/%.dia
	dia $< --export=$@ --filter=dia-svg

_build: Makefile *.rst devguide/*.rst guide/*.rst conf.py images/*.png $(DIA_SVGS) ../README.rst ../NEWS
	sphinx-build -b html . _build

linkcheck:
	sphinx-build -b linkcheck -n . _build

clean:
	rm -R _build
