TEMP=output
FILE=presentation
SRC=$(wildcard *.tex)

all: $(TEMP)/$(FILE).pdf

$(TEMP)/$(FILE).pdf: $(SRC)
	pdflatex -quiet -interaction nonstopmode -output-directory=./$(TEMP) $(FILE).tex

clean:
	rm temp/*
