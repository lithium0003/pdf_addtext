[日本語](README.ja.md)

# pdf_addtext
Prepare jpeg image file and json file (OCR result) and make PDF file including searchable text.

## Prepare
Install libraries, libjpeg and zlib and openssl.
```
# Ubuntu
sudo apt-get install libjpeg-dev zlib1g-dev libssl-dev

# Mac
brew install jpeg zlib openssl
```

And compile.
```
make
```

## Run
Prepare JPEG image(s), for example "test1.jpg".
And prepare OCR result file, json file, for example this file name should be "test1.jpg.json".

```
./pdf_addtext test1.pdf test1.jpg
```
and output test1.pdf

In Sample folder, OCR result json file sample and sample result pdf.
