# FreeBSD Rootkits

「Designing BSD Rootkits」の演習問題を2020年にやってみました。開発環境の構築に関しては [`setup-ja.md`](./setup-ja.md) をご確認ください。I tried completing the exercises from "Designing BSD Rootkits" in 2020. Environment setup instructions are in [`setup-en.md`](./setup-en.md).

演習問題に取り組んでいた同じ頃に、友達の xcellerator さんも同じ本を拾って、問題を自分のレポジトリに挙げました。コードのクオリティが非常に高いので、気になる方はぜひ見てくださいね！ Around the same time I did these exercises, my friend xcellerator picked up the book and published their answers to their own repository. The code quality is really high, so be sure to have a look!

https://github.com/xcellerator/freebsd_kernel_hacking

## Additional Kernel Hacking Resources

カーネルハッキングを学んでみたい方のために役に立ちそうなリソースをまとめてみました。

Here are some resources I have gathered that could be helpful for the aspiring kernel hacker.

### General
- FreeBSD 12.2 src: https://github.com/freebsd/freebsd-src/tree/releng/12.2/
- Linux 5.8 src: https://git.kernel.org/pub/scm/linux/kernel/git/stable/linux.git/tree/?h=v5.8.10
- [英] FreeBSD man pages: https://www.freebsd.org/cgi/man.cgi
- [日] FreeBSDのmanページ: https://kaworu.jpn.org/doc/FreeBSD/jman.php
- [英] Linux Kernel Documentation (5.8): https://www.kernel.org/doc/html/v5.8/

### Free Resources
- [英] Linux Rootkits Blog Series: https://xcellerator.github.io/posts/linux_rootkits_01/
- [英] Complete Linux Loadable Kernel Modules: http://www.ouah.org/LKM_HACKING.html
- [英] Runtime Kernel KMEM Patching: http://www.ouah.org/runtime-kernel-kmem-patching.txt
- [英] Linux on-the-fly kernel patching without LKM: http://phrack.org/issues/58/7.html

### Books

#### Designing BSD Rootkits
- Official: https://nostarch.com/rootkits.htm
- Amazon (Japan): https://www.amazon.co.jp/Designing-BSD-Rootkits-Introduction-Hacking/dp/1593271425
- Preview: https://books.google.co.jp/books?id=lyY-7VEo9j8C&printsec=frontcover

#### FreeBSD Device Drivers
- Official: https://nostarch.com/bsddrivers.htm
- Amazon (Japan): https://www.amazon.co.jp/FreeBSD-Device-Drivers-Guide-Intrepid/dp/1593272049
- Preview: https://books.google.co.jp/books?id=2oMSU-69b7wC&printsec=frontcover

#### Linux Device Drivers
- Official: https://shop.oreilly.com/product/9780596005900.do
- Amazon (Japan): https://www.amazon.co.jp/Linux-Device-Drivers-Kernel-Hardware/dp/0596005903
- Preview: https://books.google.co.jp/books?id=M7RHMACEkg4C&printsec=frontcover

#### Linuxカーネルドライバ
- 公式ページ: https://www.oreilly.co.jp/books/4873112532/
- アマゾン(日本): https://www.amazon.co.jp/Linuxデバイスドライバ-第3版-Jonathan-Corbet/dp/4873112532
- プレビュー版: https://books.google.co.jp/books?id=8H03deIPDecC&printsec=frontcover
