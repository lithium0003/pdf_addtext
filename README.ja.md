[English](README.md)

# pdf_addtext
jpeg画像と、OCR処理結果のjsonファイルから、透明テキストを埋め込んだPDFファイルを作成します。

## 準備
zlib, openssl, libpng, libjpeg, openjpeg が必要です。
```
# Ubuntu
sudo apt-get install zlib1g-dev libssl-dev libpng-dev libjpeg-dev libopenjp2-7-dev

# Mac
brew install zlib openssl libpng jpeg openjpeg
```

コンパイルします。
```
make
```

## 実行
### 新たなPDFに埋め込む
任意のJPEG画像を用意します。test1.jpgとします。
OCR結果のjsonファイルを用意します。このファイル名を、test1.jpg.jsonとしてください。

```
./pdf_addtext create test1.pdf test1.jpg
```
とすると、test1.pdfが生成されます。

jsonファイルのフォーマットや、サンプル出力結果は Sample フォルダのファイルを参照ください。

### 既存のPDFから画像を抽出する
画像のみのPDFファイルでは、まず含まれる画像を展開します。

```
./pdf_addtext extract input.pdf image_dir
```
output_dir以下に、画像が出力されます。

### 展開したPDFをOCRする
任意のOCRエンジンで、文字の座標と内容を抽出します。
jsonファイルのフォーマットは、Sample フォルダのファイルを参照してください。

### 既存のPDFに文字を埋め込む(画像ごとにjsonファイル)
抽出した画像の名前にjsonを付加した (page0001_Im1.png に対して page0001_Im1.png.json)
ファイルを画像の数だけ output_dir 以下に置くと、jsonファイルに含まれるOCR情報を埋め込むことができます。

```
./pdf_addtext process output.pdf intput.pdf image_dir
```

### 既存のPDFに文字を埋め込む(1つのjsonファイル)
任意の名前のjsonファイルに、複数の画像のOCR結果と画像ファイル名をnameキーに入れたものを用意すると、
1つのjsonファイルからOCR情報を埋め込むことができます。

```
./pdf_addtext process output.pdf intput.pdf input_all.json
```
