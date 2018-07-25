
cd docs
del index.html
pandoc --mathjax  src/index.md -o ./index.html --css src/github-pandoc.css --standalone
