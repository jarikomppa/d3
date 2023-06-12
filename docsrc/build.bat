@echo off
echo Building html..
call asciidoctor -r asciidoctor-diagram --destination-dir ../doc/ dialogtree.adoc
echo Building pdf..
call asciidoctor-pdf -r asciidoctor-diagram --destination-dir ../doc/ dialogtree.adoc
echo Building epub3..
call asciidoctor-epub3 -r asciidoctor-diagram --destination-dir ../doc/ dialogtree.adoc
echo Building mobi..
call asciidoctor-epub3 -r asciidoctor-diagram --destination-dir ../doc/ -a ebook-format=kf8 dialogtree.adoc
echo Done.