# FreeBSDカーネルモジュール開発環境の構築手順

## 前書き

「Designing BSD Rootkits」という本を読んでいたとき、演習するための仮想環境を構築しました。この章で、Vagrantを用いたFreeBSDの仮想環境の構築手順を書き留めています。最初に自分用の備忘録として書いていましたが、他の人の役にも立ちそうだったので、少し整理して公開しました。

## 仮想マシン

VM (仮想マシン) を管理するために [Vagrant](https://www.vagrantup.com/) を使います。Vagrant は沢山のハイパーバイザーソフト (VirtualBoxやKVM含) に対応していて、VM の細かい設定を行ってくれます。なお VM の定義は `Vagrantfile` というファイルで行います。

```ruby
$provision_script = <<-SCRIPT
echo 'Downloading sources...'
fetch -qo /tmp https://download.freebsd.org/ftp/releases/amd64/12.2-RELEASE/src.txz
echo 'Extracting sources...'
tar Jxf /tmp/src.txz -C /
SCRIPT

Vagrant.configure("2") do |config|
  config.vm.box = "freebsd/FreeBSD-12.2-RELEASE"
  config.vm.synced_folder "./chapters", "/home/vagrant/chapters", 
    type: "rsync", rsync__args: ["-zz", "--delete", "--archive"]
  config.vm.provision "shell", inline: $provision_script
end
```

- `generic/freebsd12` は[テンプレート](https://app.vagrantup.com/freebsd/boxes/FreeBSD-12.2-RELEASE)の名前です
- `synced_folder` で共有フォルダを追加します。 `rsync` は自分の好みです。
- `provision` は構築時に実行されるスクリプトです。読みやすさのため変数を使っています

上記のままでは共有フォルダは rsync を使用しますが、指定したオプションによって転送は一方的になります。VM 側で起こったファイルの変更は次の同期に上書きされるので、開発は必ずホスト側で行うように気をつけてください。この設計の利点としては、ビルドの時点で沢山のファイルが生成されても、ホスト側の開発環境は綺麗のままです。なお、この方法を使うと、VM の起動と別で同期を開始する必要があります。

Vagrant の基本コマンド一覧

- `vagrant up` VM を立ち上げる
- `vagrant halt` VM を停止する
- `vagrant reload` 設定ファイルを再読込、VMを再起動する
- `vagrant rsync-auto` rsync による同期を開始する

※ `vagrant rsync-auto &` を使うと便利です

## 開発環境の確認

開発環境を確認するために `chapters/01/hello_world/` をコンパイルしてロードしてみます。問題がなければ、アウトプットは以下のようになるでしょう。

```
vagrant@freebsd:~/chapters/01/hello_world % sudo kldload ./hello_world.ko
vagrant@freebsd:~/chapters/01/hello_world % sudo kldunload ./hello_world.ko
vagrant@freebsd:~/chapters/01/hello_world % dmesg | tail -n 2
[Addr: 0xffffffff82772119] Hello, world!
Goodbye, cruel world...
vagrant@freebsd:~/chapters/01/hello_world %
```
