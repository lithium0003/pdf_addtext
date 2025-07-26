# pdf_addtext
jpeg画像と、OCR処理結果のjsonファイルから、透明テキストを埋め込んだPDFファイルを作成します。

## 準備
libjpeg と zlib が必要です。
```
# Ubuntu
sudo apt-get install libjpeg-dev zlib1g-dev

# Mac
brew install jpeg zlib
```

コンパイルします。
```
make
```

## 実行
任意のJPEG画像を用意します。test1.jpgとします。
OCR結果のjsonファイルを用意します。このファイル名を、test1.jpg.jsonとしてください。

```
./pdf_addtext test1.pdf test1.jpg
```
とすると、test1.pdfが生成されます。

jsonファイルのフォーマットや、サンプル出力結果は Sample フォルダのファイルを参照ください。
