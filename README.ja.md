[English](README.md)

# pdf_addtext
jpeg画像と、OCR処理結果のjsonファイルから、透明テキストを埋め込んだPDFファイルを作成します。

## 準備
zlib, openssl, libpng が必要です。
libjpegは、サブモジュールとしています。
```
# Ubuntu
sudo apt-get install zlib1g-dev libssl-dev libpng-dev

# Mac
brew install zlib openssl libpng
```

コンパイルします。
```
make
```

## 実行
任意のJPEG画像を用意します。test1.jpgとします。
OCR結果のjsonファイルを用意します。このファイル名を、test1.jpg.jsonとしてください。

```
./pdf_addtext create test1.pdf test1.jpg
```
とすると、test1.pdfが生成されます。

jsonファイルのフォーマットや、サンプル出力結果は Sample フォルダのファイルを参照ください。

## Run(extract images from existing PDF)
画像のみのPDFファイルでは、まず含まれる画像を展開します。

```
./pdf_addtext extract some.pdf tmpoutput_dir
```
tmpoutput_dir以下に、画像が出力されます。