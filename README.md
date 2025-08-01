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

## Run
### create new PDF
Prepare JPEG image(s), for example "test1.jpg".
And prepare OCR result file, json file, for example this file name should be "test1.jpg.json".

```
./pdf_addtext create test1.pdf test1.jpg
```
and output test1.pdf

In Sample folder, OCR result json file sample and sample result pdf.

### extract images from existing PDF
For pdf file with image only, first extract images form PDF.

```
./pdf_addtext extract input.pdf output_dir
```
output images are in output_dir.

### OCR
Process OCR by any OCR engine, find text and its position.
JSON file format reference is in Sample folder.

### insert text to existing PDF (a json file per a image)
JSON files for extracted images; which name is appending ".json" (image: page0001_Im1.png to json: page0001_Im1.png.json)
are placed in output_dir folder, and insert text data to existing PDF.

```
./pdf_addtext process output.pdf intput.pdf image_dir
```

### insert text to existing PDF (one json file)
Prepare a JSON file which has OCR result and image file name for all images,
and insert text data to existing PDF.

```
./pdf_addtext process output.pdf intput.pdf input_all.json
```
