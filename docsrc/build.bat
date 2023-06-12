@echo off
echo Building html..
call asciidoctor --destination-dir ../doc/ dialogtree.adoc
echo Building pdf..
call asciidoctor-pdf --destination-dir ../doc/ dialogtree.adoc
echo Building epub3..
call asciidoctor-epub3 --destination-dir ../doc/ dialogtree.adoc
echo Building mobi..
call asciidoctor-epub3 --destination-dir ../doc/ -a ebook-format=kf8 dialogtree.adoc
echo Done.