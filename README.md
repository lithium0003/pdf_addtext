[日本語](README.ja.md)

# pdf_addtext
Prepare jpeg image file and json file (OCR result) and make PDF file including searchable text.

## Prepare
Install libraries, zlib openssl and libpng.
libjpeg is as a submodule.
```
# Ubuntu
sudo apt-get install zlib1g-dev libssl-dev libpng-dev

# Mac
brew install zlib openssl libpng
```

And compile.
```
make
```

## Run(create new PDF)
Prepare JPEG image(s), for example "test1.jpg".
And prepare OCR result file, json file, for example this file name should be "test1.jpg.json".

```
./pdf_addtext create test1.pdf test1.jpg
```
and output test1.pdf

In Sample folder, OCR result json file sample and sample result pdf.

## Run(extract images from existing PDF)
For pdf file with image only, first extract images form PDF.

```
./pdf_addtext extract some.pdf tmpoutput_dir
```
output images are in tmpoutput_dir.
